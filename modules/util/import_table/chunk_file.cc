/*
 * Copyright (c) 2018, 2020, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
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

File_iterator::File_iterator(
    mysqlshdk::storage::IFile *file_descriptor, size_t file_size,
    size_t start_from_offset, Buffer *current_buffer, Buffer *next_buffer,
    Async_read_task *aio, size_t needle_size,
    shcore::Synchronized_queue<Async_read_task *> *task_queue)
    : m_current(current_buffer),
      m_next(next_buffer),
      m_offset(start_from_offset),
      m_fh(file_descriptor),
      m_file_size(file_size),
      m_aio(aio),
      m_task_queue(task_queue) {
  if (m_current) {
    m_current->reserved = needle_size;
    m_ptr = m_current->begin();

    if (m_offset + m_current->size() < m_file_size) {
      m_ptr_end = m_current->begin() + m_current->size() - m_current->reserved;
    } else {
      m_ptr_end = m_current->begin() + m_current->size();
    }
  }

  if (m_next) {
    m_next->reserved = needle_size;
  }

  if (start_from_offset < file_size) {
    m_aio->fh = m_fh;
    m_aio->length = BUFFER_SIZE;

    read_next(m_offset);
    await_next();
    this->swap();
    read_next(m_offset + m_current->size() - m_current->reserved);
  }
}

File_iterator &File_iterator::operator++() {
  ++m_offset;
  ++m_ptr;

  if (!(m_ptr < m_ptr_end)) {
    await_next();
    swap();
    if (!m_eof) {
      read_next(m_offset + m_current->size() - m_current->reserved);
    }
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

void File_iterator::read_next(size_t offset) {
  m_aio->buffer = m_next->buffer;
  m_aio->offset = offset;
  m_next->offset = offset;

  if (!(offset < m_file_size)) {
    m_next->return_value = 0;
    return;
  }

  m_aio->status = Async_read_task::Status::Pending;
  m_task_queue->push(m_aio);
}

void File_iterator::await_next() {
  m_aio->wait();
  m_next->return_value = m_aio->return_value;

  if (m_next->eof()) {
    m_eof = true;
  }

  if (m_next->return_value < 0) {
    throw std::runtime_error("Read error");
  }
}

void File_iterator::force_offset(size_t start_from_offset) {
  m_offset = std::min(start_from_offset, m_file_size);

  // todo(kg): We can try to cancel current m_aio task. This require cancel
  // functionality implementation for generic aio which isn't currently
  // supported.
  await_next();

  if (start_from_offset < m_file_size) {
    read_next(start_from_offset);
    await_next();
    this->swap();
    read_next(start_from_offset + m_current->size() - m_current->reserved);
  }
}

File_handler::File_handler(mysqlshdk::storage::IFile *fh) : m_fh(fh) {
  m_fh->open(mysqlshdk::storage::Mode::READ);
  m_file_size = m_fh->file_size();
  m_aio_worker = mysqlsh::spawn_scoped_thread([this]() -> void {
    while (true) {
      Async_read_task *r = m_task_queue.pop();

      // exit worker
      if (r == nullptr) {
        break;
      }

      {
        std::unique_lock<std::mutex> lock(r->mutex);
        off64_t offset = r->fh->seek(r->offset);
        if (offset == static_cast<off64_t>(-1)) {
          r->return_value = -1;
          r->status = Async_read_task::Status::Error;
        } else {
          auto ret = r->fh->read(r->buffer, r->length);
          r->return_value = ret;
          r->status = Async_read_task::Status::Ok;
        }
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

File_iterator File_handler::begin(size_t needle_size) const {
  return File_iterator(m_fh, size(), 0, &m_buffer[0], &m_buffer[1], &m_aio,
                       needle_size, &m_task_queue);
}
File_iterator File_handler::end(size_t needle_size) const {
  return File_iterator(m_fh, size(), size(), nullptr, nullptr, nullptr,
                       needle_size, &m_task_queue);
}

void Chunk_file::set_chunk_size(const size_t bytes) {
  constexpr const size_t min_bytes_per_chunk = 2 * BUFFER_SIZE;
  m_chunk_size = std::max(bytes, min_bytes_per_chunk);
}

void Chunk_file::start() {
  File_handler fh{m_file_handle};

  const size_t needle_size = m_dialect.lines_terminated_by.size();
  auto first = fh.begin(needle_size);
  auto last = fh.end(needle_size);

  if (m_dialect.fields_escaped_by.empty()) {
    if (m_skip_rows_count > 0) {
      first = skip_rows(first, last, m_dialect.lines_terminated_by,
                        m_skip_rows_count);
    }
    chunk_by_max_bytes(first, last, m_dialect.lines_terminated_by, m_chunk_size,
                       m_queue);
  } else {
    if (m_skip_rows_count > 0) {
      first = skip_rows(first, last, m_dialect.lines_terminated_by,
                        m_skip_rows_count, m_dialect.fields_escaped_by[0]);
    }
    chunk_by_max_bytes(first, last, m_dialect.lines_terminated_by,
                       m_dialect.fields_escaped_by[0], m_chunk_size, m_queue);
  }
}

}  // namespace import_table
}  // namespace mysqlsh
