/*
 * Copyright (c) 2023, 2024, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is designed to work with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms,
 * as designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have either included with
 * the program or referenced in the documentation.
 *
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "mysqlshdk/libs/storage/backend/in_memory/threaded_file.h"

#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <tuple>
#include <utility>

#include "mysqlshdk/include/shellcore/scoped_contexts.h"
#include "mysqlshdk/libs/storage/compressed_file.h"
#include "mysqlshdk/libs/utils/utils_path.h"

#include "mysqlshdk/libs/storage/backend/in_memory/allocator.h"

namespace mysqlshdk {
namespace storage {
namespace in_memory {

namespace {

class Compressed_config : public Config {
 public:
  Compressed_config(std::unique_ptr<Threaded_file> file,
                    Compression compression, Config_ptr config)
      : m_file(std::move(file)),
        m_compression(compression),
        m_config(std::move(config)) {}

  Compressed_config(const Compressed_config &) = delete;
  Compressed_config(Compressed_config &&) = default;

  Compressed_config &operator=(const Compressed_config &) = delete;
  Compressed_config &operator=(Compressed_config &&) = default;

  ~Compressed_config() override = default;

  bool valid() const override { return true; }

 private:
  std::string describe_self() const override { return m_config->description(); }

  std::string describe_url(const std::string &url) const override {
    return m_config->describe(url);
  }

  std::unique_ptr<IFile> file(const std::string &path) const override {
    if (!m_called) {
      // first call comes from the main thread, result is used to supervise the
      // operation (no reads)
      m_called = true;
      return storage::make_file(storage::make_file(path, m_config),
                                m_compression);
    }

    if (!m_file) {
      throw std::logic_error("Compressed_config::file() - too many calls");
    }

    // second call is from the thread, gets the file handle
    return storage::make_file(std::move(m_file), m_compression);
  }

  std::unique_ptr<IDirectory> directory(const std::string &) const override {
    throw std::logic_error("Compressed_config::directory() - not supported");
  }

  mutable std::unique_ptr<Threaded_file> m_file;
  Compression m_compression;
  Config_ptr m_config;
  mutable bool m_called = false;
};

}  // namespace

Threaded_file::Threaded_file(Threaded_file_config config)
    : m_config(std::move(config)) {
  if (!m_config.allocator) {
    throw std::invalid_argument("Threaded_file: allocator is missing");
  }

  if (m_config.max_memory < m_config.allocator->block_size()) {
    throw std::invalid_argument(
        "Threaded_file: maximum memory is smaller than the block size");
  }

  if (m_config.threads == 0) {
    throw std::invalid_argument("Threaded_file: need at least one thread");
  }

  m_file = make_file(m_config.file_path, m_config.config);
  m_file_size = m_file->file_size();
  m_compressed = dynamic_cast<Compressed_file *>(m_file.get());
  m_block_size = m_config.allocator->block_size();
}

Threaded_file::~Threaded_file() {
  if (m_file->is_open()) {
    close_impl();
  }
}

void Threaded_file::open(Mode m) {
  if (m != Mode::READ) {
    throw std::invalid_argument("Threaded_file: only READ mode is supported");
  }

  m_file->open(m);

  m_tasks = std::make_unique<shcore::Synchronized_queue<Block *>>();
  m_workers.resize(m_config.threads);

  for (std::size_t i = 0; i < m_config.threads; ++i) {
    m_workers[i] = mysqlsh::spawn_scoped_thread([this]() {
      const auto on_exception = [this]() {
        bool expected = false;

        if (m_worker_exception_is_set.compare_exchange_strong(expected, true)) {
          m_worker_exception = std::current_exception();
          m_has_exception = true;
        }

        {
          const auto l = lock();
          m_cv.notify_one();
        }
      };

      try {
        const auto file = make_file(m_config.file_path, m_config.config);

        file->open(Mode::READ);

        while (true) {
          auto block = m_tasks->pop();

          if (!block) {
            break;
          }

          // if file is compressed, we're reading using a single thread,
          // sequentially, no need to seek in that case
          if (!m_compressed) {
            file->seek(block->offset_in_file);
          }

          std::size_t bytes = 0;

          while (bytes != m_block_size) {
            const auto result =
                file->read(block->data.memory + bytes, m_block_size - bytes);

            if (result < 0) {
              throw std::runtime_error(
                  "Failed to read '" + file->full_path().masked() +
                  "', error: " + std::to_string(file->error()));
            }

            if (0 == result) {
              break;
            }

            bytes += result;
          }

          block->data.size = bytes;
          *block->ready = true;

          if (0 == bytes) {
            m_eof = true;
          }

          {
            const auto l = lock();
            m_cv.notify_one();
          }

          if (0 == bytes) {
            // end of file
            break;
          }
        }

        file->close();
      } catch (...) {
        on_exception();
      }
    });
  }

  std::size_t allocated = 0;

  while (allocated < m_config.max_memory) {
    if (schedule_block()) {
      allocated += m_block_size;
    } else {
      break;
    }
  }
}

ssize_t Threaded_file::read(void *buffer, size_t length) {
  ssize_t result = 0;
  auto cbuffer = static_cast<char *>(buffer);

  while (length > 0) {
    if (m_compressed) {
      // we always schedule a new block if file is compressed, because we
      // don't know the uncompressed data size
      schedule_block(true);
    }

    const auto block = front();

    if (!block) {
      break;
    }

    const auto bytes = std::min(length, block->data.size - block->consumed);

    if (0 == bytes) {
      break;
    }

    ::memcpy(cbuffer, block->data.memory + block->consumed, bytes);

    block->consumed += bytes;
    result += bytes;
    cbuffer += bytes;
    length -= bytes;

    if (block->consumed == block->data.size) {
      m_config.allocator->free(block->data.memory);
      m_blocks.pop_front();

      if (!m_compressed) {
        schedule_block();
      }
    }
  }

  return result;
}

void Threaded_file::close() { close_impl(); }

void Threaded_file::close_impl() {
  m_file->close();

  if (m_tasks) {
    m_tasks->shutdown(m_config.threads);
  }

  for (auto &worker : m_workers) {
    worker.join();
  }

  m_workers.clear();
  m_tasks.reset();

  m_has_exception = false;
  m_worker_exception = nullptr;
  m_worker_exception_is_set = false;

  for (const auto &block : m_blocks) {
    m_config.allocator->free(block.data.memory);
  }

  m_blocks.clear();
}

void Threaded_file::handle_exception() const {
  if (m_has_exception) {
    std::rethrow_exception(m_worker_exception);
  }
}

bool Threaded_file::schedule_block(bool force) {
  if (m_eof) {
    return false;
  }

  if (!(m_offset < m_file_size) && !force) {
    return false;
  }

  Block b;

  b.data.memory = m_config.allocator->allocate_block();
  b.offset_in_file = m_offset;

  m_offset += m_block_size;

  m_blocks.emplace_back(std::move(b));

  m_tasks->push(&m_blocks.back());

  return true;
}

Threaded_file::Block *Threaded_file::front() {
  handle_exception();

  if (m_blocks.empty() || m_read_eof) {
    return nullptr;
  }

  auto block = &m_blocks.front();

  if (!*block->ready) {
    auto l = lock();
    m_cv.wait(l, [block, this]() { return *block->ready || m_has_exception; });
  }

  handle_exception();

  if (0 == block->data.size) {
    m_read_eof = true;
  }

  return block;
}

Data_block Threaded_file::peek() {
  const auto block = front();

  if (!block) {
    return {};
  }

  return block->data;
}

void Threaded_file::pop_front() {
  if (m_compressed) {
    // we always schedule a new block if file is compressed, because we
    // don't know the uncompressed data size
    schedule_block(true);
  }

  // ensure that the first block is available
  const auto block = front();

  if (!block) {
    return;
  }

  m_blocks.pop_front();

  if (!m_compressed) {
    schedule_block();
  }
}

Scoped_data_block Threaded_file::extract() {
  if (m_compressed) {
    // we always schedule a new block if file is compressed, because we
    // don't know the uncompressed data size
    schedule_block(true);
  }

  auto block = front();

  if (!block) {
    return {};
  }

  auto data = std::move(block->data);
  m_blocks.pop_front();

  if (!m_compressed) {
    schedule_block();
  }

  return {data, m_config.allocator};
}

std::unique_ptr<Threaded_file> threaded_file(Threaded_file_config config) {
  Compression compression;

  try {
    compression = from_extension(
        std::get<1>(shcore::path::split_extension(config.file_path)));
  } catch (...) {
    compression = Compression::NONE;
  }

  const auto file = [&config]() {
    return std::unique_ptr<Threaded_file>{new Threaded_file{std::move(config)}};
  };

  if (Compression::NONE != compression) {
    auto compression_config = config;

    // we can only read sequentially
    compression_config.threads = 1;
    compression_config.config = std::make_shared<Compressed_config>(
        file(), compression, compression_config.config);

    config = std::move(compression_config);
  }

  return file();
}

}  // namespace in_memory
}  // namespace storage
}  // namespace mysqlshdk
