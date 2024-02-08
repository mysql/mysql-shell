/*
 * Copyright (c) 2017, 2024, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/utils/debug.h"
#include <cstdlib>
#include <iostream>
#include <memory>
#include <vector>

namespace shcore {
namespace debug {

#ifndef NDEBUG

static std::vector<std::unique_ptr<Debug_object_info>> g_debug_object_list;
// static Debug_object_info *g_debug_object_dummy = nullptr;

Debug_object_info *debug_object_enable(const char *name) {
  for (const auto &c : g_debug_object_list) {
    if (c->name.compare(name) == 0) return c.get();
  }

  g_debug_object_list.push_back(std::make_unique<Debug_object_info>(name));
  return g_debug_object_list[g_debug_object_list.size() - 1].get();
}

bool debug_object_dump_report(bool verbose) {
  std::cout << "Instrumented mysqlsh object allocation report:\n";
  int count = 0;
  int fatal_found = 0;

  for (const auto &c : g_debug_object_list) {
    std::lock_guard<std::mutex> lock(c->m_mtx);
    if (verbose || c->allocs != c->m_deallocs) {
      std::cout << c->name << "\t" << (c->allocs - c->m_deallocs) << " leaks ("
                << c->allocs << " allocations vs " << c->m_deallocs
                << " deallocations)\n";
    }
    if (c->allocs != c->m_deallocs) {
      if (c->fatal_leaks) {
        std::cerr << "FATAL LEAK: " << c->name << "\n";
        fatal_found++;
      }
      c->dump();
      count += c->allocs - c->m_deallocs;
    }
  }
  if (count == 0)
    std::cout << "No instrumented allocation errors found.\n";
  else
    std::cout << count << " total instrumented leaks.\n";
  if (fatal_found > 0) abort();
  return count == 0;
}

Debug_object_info::Debug_object_info(const std::string &n) : name(n) {
  track_instances = false;
  if (const char *trace = getenv("DEBUG_OBJ_TRACE")) {
    if (std::string(":").append(trace).append(":").find(":" + n + ":") !=
        std::string::npos)
      track_instances = true;
  }
}

void Debug_object_info::on_alloc(void *p, const std::string &tag) {
  ++allocs;
  // if (name == "ShellBaseSession") std::abort();
  if (track_instances) {
    if (tag.empty()) {
      std::cout << "ALLOC " << name << "  " << p << "\n";
    } else {
      std::cout << "ALLOC " << name << "(" << tag << ")  " << p << "\n";
      instance_tags[p] = tag;
    }
    instances.insert(p);
  }
}

void Debug_object_info::on_alloc(void *p,
                                 std::function<std::string(void *)> get_debug_,
                                 const std::string &tag) {
  on_alloc(p, tag);
  this->get_debug = get_debug_;
}

void Debug_object_info::on_dealloc(void *p) {
  std::lock_guard<std::mutex> lock(m_mtx);
  ++m_deallocs;
  if (track_instances) {
    std::cout << "DEALLOC " << name << "  " << p << "\n";
    instances.erase(p);
    auto iter = instance_tags.find(p);
    if (iter != instance_tags.end()) instance_tags.erase(iter);
  }
}

void Debug_object_info::dump() {
  if (track_instances) {
    std::cout << "\tThe following instances of " << name << " are dangling:\n";
    for (auto &p : instances) {
      auto iter = instance_tags.find(p);
      if (iter == instance_tags.end()) {
        std::cout << "\t\t" << p << "\t";
      } else {
        std::cout << "\t\t" << p << "\t(" << iter->second << ")\t";
      }
      if (get_debug)
        std::cout << get_debug(p) << "\n";
      else
        std::cout << "\n";
    }
  }
}

#endif  // NDEBUG

}  // namespace debug
}  // namespace shcore
