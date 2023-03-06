/*
 * Copyright (c) 2023, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_LIBS_STORAGE_BACKEND_IN_MEMORY_ALLOCATOR_H_
#define MYSQLSHDK_LIBS_STORAGE_BACKEND_IN_MEMORY_ALLOCATOR_H_

#include <memory>
#include <mutex>
#include <set>
#include <vector>

namespace mysqlshdk {
namespace storage {
namespace in_memory {

/**
 * Allows to allocate/deallocate memory blocks of the given size.
 */
class Allocator final {
 public:
  /**
   * Initializes the allocator to serve memory blocks from pages of the given
   * size. The page size is rounded down to align with the block size.
   * Additional pages are managed dynamically.
   *
   * @param page_size Size of a single memory page.
   * @param block_size Size of a single block in the memory page.
   *
   * @throws std::runtime_error If input arguments are invalid.
   * @throws std::bad_alloc If memory cannot be allocated.
   */
  explicit Allocator(std::size_t page_size, std::size_t block_size = 8192);

  Allocator(const Allocator &) = delete;
  Allocator(Allocator &&) = delete;

  Allocator &operator=(const Allocator &) = delete;
  Allocator &operator=(Allocator &&) = delete;

  ~Allocator();

  /**
   * Provides size of a single memory block.
   *
   * @returns size of a single block
   */
  inline std::size_t block_size() const { return m_block_size; }

  /**
   * Allocates the requested memory. Memory is returned in blocks of a constant
   * size, which means that more memory can be allocated than requested.
   *
   * @param memory Size of the memory to be allocated.
   *
   * @returns allocated memory blocks
   */
  std::vector<char *> allocate(std::size_t memory);

  /**
   * Frees a single memory block.
   *
   * @param block A memory block to be freed.
   */
  void free(char *block);

  /**
   * Frees a series of memory blocks.
   *
   * @param blocks Memory blocks to be freed.
   */
  void free(const std::vector<char *> &blocks) {
    free(blocks.begin(), blocks.end());
  }

  /**
   * Frees a series of memory blocks.
   *
   * @tparam Iter Forward iterator.
   * @param blocks Memory blocks to be freed.
   */
  template <typename Iter>
  void free(Iter begin, Iter end) {
    std::lock_guard lock{m_mutex};

    while (begin != end) {
      free_block(*begin++);
    }
  }

 private:
  /**
   * A page of memory.
   */
  struct Page {
    /**
     * Creates a page. Allocates necessary memory.
     *
     * @param page_size Size of a page.
     * @param blocks Number of blocks in this page.
     * @param block_size Size of a single block.
     */
    Page(std::size_t page_size, std::size_t blocks, std::size_t block_size);

    /**
     * Uses the selected number of blocks.
     *
     * @param blocks Number of blocks to use.
     * @param result Container to store the blocks.
     *
     * @returns Number of blocks added to the container.
     */
    std::size_t use_blocks(std::size_t blocks, std::vector<char *> *result);

    // holds allocated memory
    std::unique_ptr<char[]> m_memory;
    // blocks available for allocation
    std::vector<char *> m_available_blocks;
  };

  /**
   * A transparent comparator, allows to find a page based on a memory block.
   */
  struct Compare_pages {
    using is_transparent = void;

    Compare_pages(std::size_t page_size) : m_page_size(page_size - 1) {}

    /**
     * Orders the elements by the address of allocated memory.
     */
    bool operator()(const std::unique_ptr<Page> &l,
                    const std::unique_ptr<Page> &r) const {
      return l->m_memory.get() < r->m_memory.get();
    }

    /**
     * These two allow to find a page based on a memory block.
     */
    bool operator()(const char *l, const std::unique_ptr<Page> &r) const {
      return l < r->m_memory.get();
    }

    bool operator()(const std::unique_ptr<Page> &l, const char *r) const {
      return l->m_memory.get() + m_page_size < r;
    }

   private:
    const std::size_t m_page_size;
  };

  using Pages = std::set<std::unique_ptr<Page>, Compare_pages>;

  /**
   * Adds a new page.
   */
  void add_page();

  /**
   * Removes a page.
   *
   * @param page Page to remove.
   */
  void remove_page(Pages::const_iterator page);

  /**
   * Frees the given block of memory.
   *
   * @param block A block to free.
   */
  void free_block(char *block);

  // number of blocks in a page
  const std::size_t m_blocks_per_page;
  // size of a single block
  const std::size_t m_block_size;
  // size of a page
  const std::size_t m_page_size;

  // all pages
  Pages m_pages;
  // full pages
  Pages m_full_pages;
  // empty page
  Page *m_empty_page = nullptr;

  // number of available blocks
  std::size_t m_available_blocks = 0;

  // controls access to memory
  std::mutex m_mutex;
};

}  // namespace in_memory
}  // namespace storage
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_STORAGE_BACKEND_IN_MEMORY_ALLOCATOR_H_
