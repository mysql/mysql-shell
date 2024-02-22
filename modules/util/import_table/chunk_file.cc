/*
 * Copyright (c) 2018, 2024, Oracle and/or its affiliates.
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

#include "modules/util/import_table/chunk_file.h"

#include <algorithm>
#include <cassert>

#include "mysqlshdk/include/shellcore/scoped_contexts.h"
#include "mysqlshdk/libs/utils/utils_file.h"

namespace mysqlsh {
namespace import_table {

File_iterator::File_iterator(File_handler *parent, size_t offset)
    : m_parent(parent),
      m_current(&parent->m_buffer[0]),
      m_next(&parent->m_buffer[1]),
      m_offset(offset) {
  if (offset >= m_parent->file_size()) {
    return;
  }

  enqueue_next(m_offset);
  read_more();
}

File_iterator &File_iterator::operator++() {
  ++m_offset;
  ++m_ptr;

  if (m_ptr >= m_ptr_end) {
    read_more();
  }

  return *this;
}

File_iterator &File_iterator::operator++(int) {
  ++m_offset;
  ++m_ptr;
  assert(m_ptr <= (m_ptr_end + m_current->reserved));
  return *this;
}

File_iterator &File_iterator::operator--(int) {
  --m_offset;
  --m_ptr;
  assert(m_ptr >= m_current->buffer);
  return *this;
}

void File_iterator::enqueue_next(size_t offset) {
  const auto aio = &m_parent->m_aio;
  aio->buffer = m_next->buffer;
  aio->offset = offset;

  if (!(offset < m_parent->file_size())) {
    aio->return_value = 0;
    aio->status = Async_read_task::Status::Ok;
    return;
  }

  aio->status = Async_read_task::Status::Pending;
  m_parent->m_task_queue.push(aio);
}

void File_iterator::await_next() {
  const auto aio = &m_parent->m_aio;
  aio->wait();
  m_next->return_value = aio->return_value;

  if (m_next->eof()) {
    m_eof = true;
  }

  if (m_next->return_value < 0) {
    if (aio->exception) {
      std::rethrow_exception(aio->exception);
    } else {
      throw std::runtime_error(
          "Failed to read " + aio->fh->full_path().masked() +
          ", error code: " + std::to_string(m_next->return_value));
    }
  }
}

void File_iterator::read_more() {
  await_next();
  swap();

  if (!m_eof) {
    enqueue_next(m_offset + m_current->size() - m_current->reserved);
  }
}

void File_iterator::swap() {
  std::swap(m_current, m_next);

  m_ptr = m_current->begin();

  if (m_offset + m_current->size() < m_parent->file_size()) {
    m_ptr_end = m_current->begin() + m_current->size() - m_current->reserved;
  } else {
    m_ptr_end = m_current->begin() + m_current->size();
  }
}

void File_iterator::force_offset(size_t start_from_offset) {
  m_offset = std::min(start_from_offset, m_parent->file_size());

  // todo(kg): We can try to cancel current m_aio task. This require cancel
  // functionality implementation for generic aio which isn't currently
  // supported.
  await_next();

  if (start_from_offset < m_parent->file_size()) {
    enqueue_next(m_offset);
    read_more();
  }
}

File_handler::File_handler(mysqlshdk::storage::IFile *fh) : m_fh(fh) {
  if (!m_fh->is_open()) {
    m_fh->open(mysqlshdk::storage::Mode::READ);
  }

  m_file_size = m_fh->file_size();
  m_aio.fh = m_fh;
  m_aio.length = Buffer::capacity();

  m_aio_worker = mysqlsh::spawn_scoped_thread([this]() -> void {
    while (true) {
      Async_read_task *r = m_task_queue.pop();

      // exit worker
      if (r == nullptr) {
        break;
      }

      try {
        std::unique_lock<std::mutex> lock(r->mutex);

        if (r->offset >= 0) {
          if (r->fh->seek(r->offset) < 0) {
            throw std::runtime_error("Failed to seek file " +
                                     r->fh->full_path().masked() + " to " +
                                     std::to_string(r->offset));
          }
        }

        r->return_value = r->fh->read(r->buffer, r->length);
        r->status = Async_read_task::Status::Ok;
      } catch (...) {
        r->exception = std::current_exception();
        r->return_value = -1;
        r->status = Async_read_task::Status::Error;
      }

      r->cv.notify_one();
    }
  });
}

File_handler::~File_handler() {
  m_task_queue.shutdown(1);
  m_aio_worker.join();
  m_fh->close();
}

std::pair<File_iterator, File_iterator> File_handler::iterators(
    size_t needle_size) {
  m_buffer[0].reserved = needle_size;
  m_buffer[1].reserved = needle_size;

  return {File_iterator{this, 0}, File_iterator{this, file_size()}};
}

void Chunk_file::set_chunk_size(const size_t bytes) {
  constexpr const size_t min_bytes_per_chunk = 2 * BUFFER_SIZE;
  m_chunk_size = std::max(bytes, min_bytes_per_chunk);
}

void Chunk_file::start() {
  File_handler fh{m_file_handle};

  auto [first, last] = fh.iterators(m_dialect.lines_terminated_by.size());

  File_import_info stencil;
  stencil.file_path = m_file_handle->full_path().real();
  stencil.range_read = true;
  stencil.is_guard = false;

  if (m_dialect.fields_escaped_by.empty()) {
    if (m_skip_rows_count > 0) {
      first = skip_rows(first, last, m_dialect.lines_terminated_by,
                        m_skip_rows_count);
    }
    chunk_by_max_bytes(first, last, m_dialect.lines_terminated_by, m_chunk_size,
                       m_queue, stencil);
  } else {
    if (m_skip_rows_count > 0) {
      first = skip_rows(first, last, m_dialect.lines_terminated_by,
                        m_skip_rows_count, m_dialect.fields_escaped_by[0]);
    }
    chunk_by_max_bytes(first, last, m_dialect.lines_terminated_by,
                       m_dialect.fields_escaped_by[0], m_chunk_size, m_queue,
                       stencil);
  }
}

}  // namespace import_table
}  // namespace mysqlsh
