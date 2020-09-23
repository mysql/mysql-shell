/*
 * Copyright (c) 2018, 2020, Oracle and/or its affiliates.
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

#ifndef MODULES_UTIL_IMPORT_TABLE_CHUNK_FILE_H_
#define MODULES_UTIL_IMPORT_TABLE_CHUNK_FILE_H_

#include <atomic>
#include <cassert>
#include <condition_variable>
#include <memory>
#include <string>
#include <thread>
#include <utility>

#include "modules/util/import_table/dialect.h"
#include "modules/util/import_table/helpers.h"
#include "mysqlshdk/libs/oci/oci_options.h"
#include "mysqlshdk/libs/storage/ifile.h"
#include "mysqlshdk/libs/utils/synchronized_queue.h"

namespace mysqlsh {
namespace import_table {

struct Buffer final {
 public:
  Buffer() = default;
  Buffer(const Buffer &other) = delete;
  Buffer(Buffer &&other) = delete;

  Buffer &operator=(const Buffer &other) = delete;
  Buffer &operator=(Buffer &&other) = delete;

  ~Buffer() = default;

  // static constexpr const size_t BUFFER_SIZE = 1 << 16;
  uint8_t buffer[BUFFER_SIZE];
  int return_value = -1;  //< read() return value
  size_t offset = -1;     //< Global offset, e.g. from file beginning
  size_t reserved = 0;    //< Number of bytes reserved at the end of the buffer

  size_t size() const noexcept { return return_value > 0 ? return_value : 0; }

  constexpr static size_t capacity() noexcept { return BUFFER_SIZE; }

  bool eof() const noexcept { return return_value == 0; }

  uint8_t *begin() { return &buffer[0]; }
  uint8_t *end() { return &buffer[size()]; }
};

struct Async_read_task {
  enum class Status { Pending, Ok, Error, Cancelled };
  std::atomic<Status> status;     //< Request status
  mysqlshdk::storage::IFile *fh;  //< File handler
  int64_t offset;                 //< File offset
  void *buffer;                   //< Buffer location
  size_t length;                  //< Number of bytes to read
  int return_value;               //< read() return value

  std::mutex mutex;            //< mutex
  std::condition_variable cv;  //< Synchronization point

  /**
   * Wait for task to be ready
   */
  void wait() {
    if (this->status == Async_read_task::Status::Pending) {
      std::unique_lock<std::mutex> lock(this->mutex);
      this->cv.wait(lock, [this]() {
        return this->status != Async_read_task::Status::Pending;
      });
    }
  }
};

/**
 * File_handler iterator that asynchronously pre-loads file chunks to double
 * buffer.
 */
class File_iterator final {
 public:
  using difference_type = ssize_t;
  using value_type = uint8_t;
  using pointer = uint8_t *;
  using reference = uint8_t &;
  using iterator_category = std::forward_iterator_tag;

  File_iterator() = delete;
  File_iterator(mysqlshdk::storage::IFile *file_descriptor, size_t file_size,
                size_t start_from_offset, Buffer *current_buffer,
                Buffer *next_buffer, Async_read_task *aio, size_t needle_size,
                shcore::Synchronized_queue<Async_read_task *> *task_queue);

  File_iterator(const File_iterator &other) = default;
  File_iterator(File_iterator &&other) = default;

  File_iterator &operator=(const File_iterator &other) = default;
  File_iterator &operator=(File_iterator &&other) = default;

  bool operator!=(const File_iterator &other) const {
    return m_offset != other.m_offset;
  }

  bool operator==(const File_iterator &other) const {
    return m_offset == other.m_offset;
  }

  uint8_t operator*() const { return *m_ptr; }

  /**
   * Pre-increment operator advances buffer and file position. Buffer boundaries
   * are checked. Swaps double buffers and load next file chunk to next buffer
   * when end of the buffer is reached.
   *
   * @return Reference to File_iterator
   */
  File_iterator &operator++();

  /**
   * Post-increment operator advances buffer and file position. Use this when
   * you need pointer to reserved buffer area. Buffer boundaries are not checked
   * and might overflow buffer.
   *
   * @return Reference to File_iterator
   */
  File_iterator &operator++(int);

  /**
   * Post-decrement operator decrements buffer and file position. Use this only
   * when you found needle, but it is escaped and you need to step back. Buffer
   * boundaries are not checked and might underflow buffer.
   *
   * @return Reference to File_iterator
   */
  File_iterator &operator--(int);

  /**
   * Get iterator position from file beginning.
   *
   * @return File offset.
   */
  size_t offset() { return m_offset; }

  /**
   * Set iterator to file offset.
   *
   * @param start_from_offset File byte absolute value.
   */
  void force_offset(size_t start_from_offset);

  ~File_iterator() = default;

 private:
  Buffer *m_current = nullptr;
  Buffer *m_next = nullptr;
  uint8_t *m_ptr = nullptr;
  uint8_t *m_ptr_end = nullptr;
  size_t m_offset = 0;  //< Global file offset where m_ptr points
  mysqlshdk::storage::IFile *m_fh = nullptr;
  size_t m_file_size = 0;
  Async_read_task *m_aio{};
  bool m_eof = false;
  shcore::Synchronized_queue<Async_read_task *> *m_task_queue = nullptr;

  /**
   * Enqueue task that reads data from file offset to next buffer.
   *
   * @param offset File offset.
   */
  void read_next(size_t offset);

  /**
   * Wait for currently enqueued task finish work. After this call next buffer
   * is valid to read.
   */
  void await_next();

  /**
   * Swap double buffers and reset internal range pointers to proper values.
   */
  void swap() {
    std::swap(m_current, m_next);
    m_ptr = m_current->begin();
    if (m_offset + m_current->size() < m_file_size) {
      m_ptr_end = m_current->begin() + m_current->size() - m_current->reserved;
    } else {
      m_ptr_end = m_current->begin() + m_current->size();
    }
  }
};

/**
 * Asynchronous double buffered file reader.
 */
class File_handler final {
 public:
  File_handler() = default;
  explicit File_handler(mysqlshdk::storage::IFile *fh);

  File_handler(const File_handler &other) = delete;
  File_handler(File_handler &&other) = delete;

  File_handler &operator=(const File_handler &other) = delete;
  File_handler &operator=(File_handler &&other) = delete;

  ~File_handler();

  bool is_open() const { return m_fh->is_open(); }

  size_t size() const { return m_file_size; }

  File_iterator begin(size_t needle_size) const;
  File_iterator end(size_t needle_size) const;

 private:
  mutable Buffer m_buffer[2];  //< Double buffer
  mutable Async_read_task m_aio{};
  mutable std::thread m_aio_worker;
  mutable shcore::Synchronized_queue<Async_read_task *> m_task_queue;
  mysqlshdk::storage::IFile *m_fh;
  size_t m_file_size = 0;
};

struct File_import_info {
  std::string file_path;
  mysqlshdk::storage::IFile *file_handler = nullptr;
  mysqlshdk::utils::nullable<size_t> file_size;
  mysqlshdk::utils::nullable<size_t> content_size;
  bool range_read = false;
  std::pair<size_t, size_t> range{0, 0};
  bool is_guard = true;
};

/**
 * Stores find() function state for continuation
 *
 * @tparam T Underlying buffer value_type.
 */
template <typename T>
struct Find_context {
  bool needle_found = false;           //< Is needle found in haystack
  bool preceding_element_set = false;  //< Is preceding_element valid
  T preceding_element = T{};           //< Element before needle start
  T last_element = T{};                //< Last visited element
};

/**
 * Searches for an element equal to needle.
 *
 * @tparam ForwardIt Forward iterator type.
 * @param first Iterator to the first element.
 * @param last Iterator to the last element.
 * @param needle Value to compare elements to.
 * @return Returns one past first element from range [first, last) that
 * satisfies search criteria, with character before matching needle and boolean
 * flag indicating if needle was found.
 */
template <typename ForwardIt>
ForwardIt find(ForwardIt first, ForwardIt last, char needle,
               Find_context<typename ForwardIt::value_type> *context) {
  assert(context);
  for (; first != last; ++first) {
    context->last_element = *first;
    if (*first == needle) {
      ++first;
      context->needle_found = true;
      return first;
    }
    context->preceding_element_set = true;
    context->preceding_element = *first;
  }
  context->needle_found = false;
  return last;
}

/**
 * Searches for the first occurrence of the sequence of elements [needle_first,
 * needle_last) in the range [first, last).
 *
 * @tparam ForwardIt Forward iterator type.
 * @tparam ForwardIt2 Forward iterator type.
 * @param first Iterator to the first element of range to examine.
 * @param last Iterator to the last element of range to examine.
 * @param needle_first Iterator to first element of range to search for.
 * @param needle_last Iterator to last element of range to search for.
 * @return Returns one past first element from range [first, last) that
 * satisfies search criteria, with character before matching needle and boolean
 * flag indicating if needle was found.
 */
template <typename ForwardIt, typename ForwardIt2>
ForwardIt find(ForwardIt first, ForwardIt last, ForwardIt2 needle_first,
               ForwardIt2 needle_last,
               Find_context<typename ForwardIt::value_type> *context) {
  assert(context);
  for (;; ++first) {
    context->last_element = *first;
    ForwardIt it = first;
    for (ForwardIt2 needle_it = needle_first;; it++, ++needle_it) {
      if (needle_it == needle_last) {
        context->needle_found = true;
        return it;
      }
      if (it == last) {
        context->needle_found = false;
        return last;
      }
      if (!(*it == *needle_it)) {
        break;
      }
    }
    context->preceding_element_set = true;
    context->preceding_element = *first;
  }
}

/**
 * Skip count lines/rows delimited by needle.
 *
 * @tparam Iter Forward iterator.
 * @param first Iterator to first element in range.
 * @param last Iterator to last element in range.
 * @param needle Line/row terminator.
 * @param count Number of lines/rows to skip.
 * @param escape_char Escape character.
 * @return Returns iterator to first element in [first, last) range after count
 * lines/rows.
 */
template <typename Iter>
Iter skip_rows(Iter first, Iter last, const std::string &needle, uint64_t count,
               char escape_char) {
  Find_context<typename Iter::value_type> context{};

  while (count > 0 && first != last) {
    first = find(first, last, needle.begin(), needle.end(), &context);

    // needle found, but is escaped or escape state is unknown
    auto escaped = [&]() -> bool {
      return context.needle_found &&
             ((context.preceding_element_set &&
               context.preceding_element == escape_char) ||
              (!context.preceding_element_set));
    };

    if (!escaped()) {
      count--;
    } else {
      // move iterator after escaped char
      if (context.needle_found) {
        for (size_t i = 1; i < needle.size(); i++) {
          first--;
        }
      }
    }
    context.preceding_element_set = true;
    context.preceding_element = context.last_element;
  }
  return first;
}

/**
 * Skip count lines/rows delimited by needle.
 *
 * @tparam Iter Forward iterator.
 * @param first Iterator to first element in range.
 * @param last Iterator to last element in range.
 * @param needle Line/row terminator.
 * @param count Number of lines/rows to skip.
 * @return Returns iterator to first element in [first, last) range after count
 * lines/rows.
 */
template <typename Iter>
Iter skip_rows(Iter first, Iter last, const std::string &needle,
               uint64_t count) {
  Find_context<typename Iter::value_type> context{};
  while (count > 0 && first != last) {
    first = find(first, last, needle.begin(), needle.end(), &context);
    count--;
  }
  return first;
}

/**
 * Fill QueueContainer with file chunks offset that are roughly
 * max_bytes_per_chunk in size.
 *
 * @tparam Iter Forward iterator.
 * @tparam QueueContainer Target container type.
 * @param first Iterator to first element in range.
 * @param last Iterator to last element in range.
 * @param needle Line terminator string.
 * @param escape_char Escape character.
 * @param max_bytes_per_chunk Size of file chunks in bytes.
 * @param range_queue Queue where file chunk offset will be stored.
 */
template <typename Iter,
          class QueueContainer = shcore::Synchronized_queue<File_import_info>>
void chunk_by_max_bytes(Iter first, Iter last, const std::string &needle,
                        char escape_char, const size_t max_bytes_per_chunk,
                        QueueContainer *range_queue,
                        const File_import_info &stencil) {
  assert(range_queue);

  size_t current_offset = first.offset();

  while (first != last) {
    const size_t prev_offset = current_offset;
    const size_t next_offset = current_offset + max_bytes_per_chunk;
    first.force_offset(next_offset);
    Find_context<typename Iter::value_type> context{};

    first = find(first, last, needle.begin(), needle.end(), &context);

    // needle found, but is escaped or escape state is unknown
    auto escaped = [&]() -> bool {
      return context.needle_found &&
             ((context.preceding_element_set &&
               context.preceding_element == escape_char) ||
              (!context.preceding_element_set));
    };

    while (escaped()) {
      context.preceding_element_set = true;
      context.preceding_element = context.last_element;
      first = find(first, last, needle.begin(), needle.end(), &context);
    }

    current_offset = first.offset();
    File_import_info info = stencil;
    info.range = std::make_pair(prev_offset, current_offset);
    range_queue->push(std::move(info));
  }
}

/**
 * Fill QueueContainer with file chunks offset that are roughly
 * max_bytes_per_chunk in size.
 *
 * @tparam Iter Forward iterator.
 * @tparam QueueContainer Target container type.
 * @param first Iterator to first element in range.
 * @param last Iterator to last element in range.
 * @param needle Line terminator string.
 * @param max_bytes_per_chunk Size of file chunks in bytes.
 * @param range_queue Queue where file chunk offset will be stored.
 */
template <typename Iter,
          class QueueContainer = shcore::Synchronized_queue<File_import_info>>
void chunk_by_max_bytes(Iter first, Iter last, const std::string &needle,
                        const size_t max_bytes_per_chunk,
                        QueueContainer *range_queue,
                        const File_import_info &stencil) {
  assert(range_queue);

  size_t current_offset = first.offset();

  while (first != last) {
    size_t prev_offset = current_offset;
    const size_t next_offset = current_offset + max_bytes_per_chunk;
    first.force_offset(next_offset);
    Find_context<typename Iter::value_type> context{};

    first = find(first, last, needle.begin(), needle.end(), &context);

    // needle found, but escape state is unknown
    auto escaped = [&]() -> bool {
      return context.needle_found && !context.preceding_element_set;
    };

    while (escaped()) {
      first = find(first, last, needle.begin(), needle.end(), &context);
    }

    current_offset = first.offset();
    File_import_info info = stencil;
    info.range = std::make_pair(prev_offset, current_offset);
    range_queue->push(std::move(info));
  }
}

class Chunk_file final {
 public:
  Chunk_file() = default;
  Chunk_file(const Chunk_file &other) = default;
  Chunk_file(Chunk_file &&other) = default;

  Chunk_file &operator=(const Chunk_file &other) = default;
  Chunk_file &operator=(Chunk_file &&other) = default;

  ~Chunk_file() = default;

  void set_chunk_size(const size_t bytes);
  void set_file_handle(mysqlshdk::storage::IFile *fh) { m_file_handle = fh; }
  void set_dialect(const Dialect &dialect) { m_dialect = dialect; }
  void set_rows_to_skip(const size_t rows) { m_skip_rows_count = rows; }
  void set_output_queue(shcore::Synchronized_queue<File_import_info> *queue) {
    m_queue = queue;
  }
  void start();

 private:
  size_t m_chunk_size = 2 * BUFFER_SIZE;
  Dialect m_dialect;
  uint64_t m_skip_rows_count = 0;
  shcore::Synchronized_queue<File_import_info> *m_queue = nullptr;
  mysqlshdk::storage::IFile *m_file_handle;
};

}  // namespace import_table
}  // namespace mysqlsh

#endif  // MODULES_UTIL_IMPORT_TABLE_CHUNK_FILE_H_
