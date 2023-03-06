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

#include "mysqlshdk/libs/storage/backend/in_memory/allocator.h"

#include <algorithm>
#include <atomic>
#include <iterator>
#include <stdexcept>
#include <thread>
#include <vector>

#include "unittest/gtest_clean.h"
#include "unittest/test_utils.h"

#include "mysqlshdk/libs/utils/synchronized_queue.h"
#include "mysqlshdk/libs/utils/utils_general.h"

namespace mysqlshdk {
namespace storage {
namespace in_memory {

TEST(In_memory_allocator, constructor) {
  EXPECT_THROW(Allocator(0), std::runtime_error);
  EXPECT_THROW(Allocator(1024, 0), std::runtime_error);
  EXPECT_THROW(Allocator(1024, 1025), std::runtime_error);
}

TEST(In_memory_allocator, block_size) {
  {
    Allocator a{1024 * 1024};
    EXPECT_EQ(8192, a.block_size());
  }

  {
    Allocator a{1024 * 1024, 512};
    EXPECT_EQ(512, a.block_size());
  }

  {
    Allocator a{1024, 1024};
    EXPECT_EQ(1024, a.block_size());
  }
}

TEST(In_memory_allocator, allocate_free) {
  constexpr std::size_t number_of_blocks = 4;
  constexpr std::size_t block_size = 512;

  Allocator a{number_of_blocks * block_size, block_size};

  {
    // allocate 0 bytes
    const auto blocks = a.allocate(0);
    EXPECT_EQ(0, blocks.size());
  }

  {
    // allocate 1 byte
    const auto blocks = a.allocate(1);
    EXPECT_EQ(1, blocks.size());
    a.free(blocks);
  }

  {
    // allocate whole block
    const auto blocks = a.allocate(block_size);
    EXPECT_EQ(1, blocks.size());

    for (const auto &block : blocks) {
      a.free(block);
    }
  }

  {
    // allocate more than whole block
    const auto blocks = a.allocate(block_size + 1);
    EXPECT_EQ(2, blocks.size());
    a.free(blocks);
  }

  {
    // allocate whole page
    const auto blocks = a.allocate(number_of_blocks * block_size);
    EXPECT_EQ(number_of_blocks, blocks.size());

    for (const auto &block : blocks) {
      a.free(block);
    }
  }

  {
    // allocate more than whole page
    const auto blocks = a.allocate(number_of_blocks * block_size + 1);
    EXPECT_EQ(number_of_blocks + 1, blocks.size());
    a.free(blocks);
  }

  {
    // interleave allocates and frees
    std::vector<char *> all_blocks;
    const auto store = [&all_blocks](const auto &blocks) {
      std::copy(blocks.begin(), blocks.end(), std::back_inserter(all_blocks));
    };

    for (std::size_t i = 1; i <= number_of_blocks + 1; ++i) {
      store(a.allocate(block_size));
      EXPECT_EQ(i, all_blocks.size());
    }

    for (std::size_t i = 1; i <= number_of_blocks + 1; ++i) {
      a.free(all_blocks.back());
      all_blocks.pop_back();

      a.free(a.allocate(i * block_size));
    }
  }
}

TEST(In_memory_allocator, threads) {
  constexpr std::size_t number_of_blocks = 4;
  constexpr std::size_t block_size = 1;
  constexpr int thread_count = 2;

  Allocator a{number_of_blocks * block_size, block_size};

  std::vector<std::thread> threads;
  shcore::Synchronized_queue<char *> queue;
  std::atomic<int> done = 0;

  for (int t = 0; t < thread_count; ++t) {
    threads.emplace_back([&]() {
      constexpr int loop = 1'000;

      const auto allocate = [&]() {
        // allocate a new page each time
        for (auto b : a.allocate(number_of_blocks * block_size + 1)) {
          queue.push(b);
        }
      };

      const auto free = [&]() -> bool {
        auto b = queue.pop();

        if (b) {
          a.free(b);
        }

        return b;
      };

      // allocate only
      for (int i = 0; i < loop; ++i) {
        allocate();
      }

      // allocate more
      for (int i = 0; i < loop; ++i) {
        if (i % 3) {
          allocate();
        } else {
          free();
        }
      }

      // allocate and free at the same rate
      for (int i = 0; i < loop; ++i) {
        if (i % 2) {
          allocate();
        } else {
          free();
        }
      }

      // free more
      for (int i = 0; i < loop; ++i) {
        if (i % 3) {
          free();
        } else {
          allocate();
        }
      }

      // no more allocates
      if (++done == thread_count) {
        queue.shutdown(thread_count);
      }

      // synchronize with the other threads
      while (done != thread_count) {
        shcore::sleep_ms(10);
      }

      // free the rest
      while (free()) {
      }
    });
  }

  for (auto &t : threads) {
    t.join();
  }
}

}  // namespace in_memory
}  // namespace storage
}  // namespace mysqlshdk
