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

#include "mysqlshdk/libs/storage/backend/in_memory/allocator.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <iterator>
#include <stdexcept>
#include <utility>

#include "mysqlshdk/include/scripting/shexcept.h"
#include "mysqlshdk/libs/utils/debug.h"

namespace mysqlshdk {
namespace storage {
namespace in_memory {

Allocator::Page::Page(std::size_t page_size, std::size_t blocks,
                      std::size_t block_size)
    : m_memory(std::make_unique<char[]>(page_size)) {
  auto ptr = m_memory.get() + page_size;
  m_available_blocks.reserve(blocks);

  for (std::size_t i = 0; i < blocks; ++i) {
    ptr -= block_size;
    m_available_blocks.emplace_back(ptr);
  }
}

std::size_t Allocator::Page::use_blocks(std::size_t blocks,
                                        std::vector<char *> *result) {
  assert(result);

  std::size_t used = 0;

  while (blocks > 0 && !m_available_blocks.empty()) {
    result->emplace_back(m_available_blocks.back());
    m_available_blocks.pop_back();
    --blocks;
    ++used;
  }

  return used;
}

Allocator::Allocator(std::size_t page_size, std::size_t block_size)
    : m_blocks_per_page([=]() {
        if (!block_size) {
          throw std::runtime_error("The block size cannot be 0");
        }

        return page_size / block_size;
      }()),
      m_block_size(block_size),
      m_page_size(m_blocks_per_page * m_block_size),
      m_pages(Compare_pages{m_page_size}),
      m_full_pages(Compare_pages{m_page_size}) {
  if (!m_blocks_per_page) {
    throw std::runtime_error("The page size cannot be 0");
  }

  add_page();
}

Allocator::~Allocator() {
  // we should only have one page, with all blocks free
  assert(1 == m_pages.size());
  assert(m_empty_page == m_pages.begin()->get());
  assert(m_blocks_per_page == m_available_blocks);
}

std::vector<char *> Allocator::allocate(std::size_t memory) {
  auto blocks = block_count(memory);
  std::vector<char *> result;
  result.reserve(blocks);

  {
    std::unique_lock lock{m_mutex};

    while (blocks > m_available_blocks) {
      add_page();
    }

    while (blocks > 0) {
      auto page = m_pages.begin()->get();

      const auto allocated = page->use_blocks(blocks, &result);

      // there are no full pages here, at least one block should be available
      assert(allocated > 0);

      if (m_empty_page == page) {
        m_empty_page = nullptr;
      }

      if (page->m_available_blocks.empty()) {
        // page is now full, move it to the other container
        m_full_pages.insert(m_pages.extract(m_pages.begin()));
      }

      blocks -= allocated;
      m_available_blocks -= allocated;
    }
  }

  return result;
}

void Allocator::free(char *block) {
  std::lock_guard lock{m_mutex};
  free_block(block);
}

void Allocator::add_page() {
  auto page =
      std::make_unique<Page>(m_page_size, m_blocks_per_page, m_block_size);

  m_empty_page = page.get();
  m_pages.emplace(std::move(page));

  m_available_blocks += m_blocks_per_page;
}

void Allocator::remove_page(Pages::const_iterator page) {
  // only an empty page can be removed
  assert(m_blocks_per_page == (*page)->m_available_blocks.size());
  assert(m_full_pages.end() == m_full_pages.find(*page));
  // but it's not the empty page we keep at hand
  assert(m_empty_page != page->get());

  m_pages.erase(page);

  m_available_blocks -= m_blocks_per_page;
}

void Allocator::free_block(char *block) {
  if (const auto full = m_full_pages.find(block); m_full_pages.end() != full) {
    // page is no longer going to be full, move it to the other container
    m_pages.insert(m_full_pages.extract(full));
  }

  const auto page_it = m_pages.find(block);
  assert(m_pages.end() != page_it);
  const auto &page = *page_it;
  assert(page->m_memory.get() <= block &&
         block < page->m_memory.get() + m_page_size);

  page->m_available_blocks.emplace_back(block);
  ++m_available_blocks;

  if (m_blocks_per_page == page->m_available_blocks.size()) {
    // page is completely empty
    if (m_empty_page) {
      // we already have an empty page, release this one
      remove_page(page_it);
    } else {
      // we keep a single empty page to avoid repeatedly adding and removing a
      // page when close to the capacity limit
      m_empty_page = page.get();
    }
  }
}

Scoped_data_block::Scoped_data_block() : m_allocator(nullptr) {}

Scoped_data_block::Scoped_data_block(Data_block block, Allocator *allocator)
    : m_block(std::move(block)), m_allocator(allocator) {}

Scoped_data_block::Scoped_data_block(Scoped_data_block &&other) {
  operator=(std::move(other));
}

Scoped_data_block &Scoped_data_block::operator=(Scoped_data_block &&other) {
  free();

  m_block = std::exchange(other.m_block, {});
  m_allocator = other.m_allocator;

  return *this;
}

Scoped_data_block::~Scoped_data_block() { free(); }

Data_block Scoped_data_block::relinquish() {
  return std::exchange(m_block, {});
}

Scoped_data_block Scoped_data_block::new_block(Allocator *allocator) {
  return Scoped_data_block{{allocator->allocate_block(), 0}, allocator};
}

}  // namespace in_memory
}  // namespace storage
}  // namespace mysqlshdk
