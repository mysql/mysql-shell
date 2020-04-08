/*
 * Copyright (c) 2019, 2020 Oracle and/or its affiliates. All rights reserved.
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

#ifndef MYSQLSHDK_LIBS_UTILS_THREADS_H_
#define MYSQLSHDK_LIBS_UTILS_THREADS_H_

#include <atomic>
#include <functional>
#include <string>
#include <thread>
#include <utility>
#include <vector>
#include "mysqlshdk/include/shellcore/scoped_contexts.h"
#include "mysqlshdk/libs/utils/synchronized_queue.h"

namespace mysqlshdk {
namespace utils {
/**
 * Executes the map function on each value of the given list in parallel and
 * return the aggregation of their results, as computed by the reduce function.
 *
 * map is called in a worker thread, while reduce is called in the callers
 * thread.
 */
template <class OutputT, class IntermediateT, class InputIter, class MapF,
          class ReduceF>
OutputT map_reduce(InputIter begin, InputIter end, MapF map, ReduceF reduce) {
  shcore::Synchronized_queue<std::pair<size_t, IntermediateT>> results;
  std::vector<std::thread> workers;

  auto iter = begin;
  // start computation in threads
  while (iter != end) {
    size_t index = workers.size();
    workers.push_back(
        mysqlsh::spawn_scoped_thread([&results, map, iter, index]() {
          results.push({index, map(*iter)});
        }));
    ++iter;
  }

  // wait for threads to finish
  auto result = OutputT();
  int threads_left = workers.size();
  while (threads_left > 0) {
    std::pair<size_t, IntermediateT> r = results.pop();

    workers[r.first].join();
    threads_left--;

    result = reduce(result, r.second);
  }

  return result;
}

}  // namespace utils
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_UTILS_THREADS_H_
