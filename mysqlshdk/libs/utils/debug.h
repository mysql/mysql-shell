/*
 * Copyright (c) 2017, 2018, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MYSQLSHDK_LIBS_UTILS_DEBUG_H_
#define MYSQLSHDK_LIBS_UTILS_DEBUG_H_

#include <cstdint>
#include <functional>
#include <map>
#include <set>
#include <string>
#include "mysqlshdk/libs/utils/utils_string.h"

namespace shcore {
namespace debug {

#ifndef NDEBUG

/**
 * Debug support for tracking object allocations and deallocations.
 *
 * To use, call:
 *  DEBUG_OBJ_ENABLE(class) on the file with class implementation
 *  DEBUG_OBJ_ALLOC(class) on all contructors and
 *  DEBUG_OBJ_DEALLOC(class) on destructor
 */

#define DEBUG_OBJ_FOR_CLASS_(klass) , shcore::debug::Debug_object_for<klass>

#define DEBUG_OBJ_FOR_CLASS(klass) shcore::debug::Debug_object_for<klass>

#define DEBUG_OBJ_ENABLE(name) \
  static shcore::debug::Debug_object_info *g_debug_obj_##name = nullptr

#define DEBUG_OBJ_ALLOC_N(name, n)                             \
  do {                                                         \
    if (!g_debug_obj_##name) {                                 \
      g_debug_obj_##name =                                     \
          shcore::debug::debug_object_enable(STRINGIFY(name)); \
    }                                                          \
    g_debug_obj_##name->on_alloc(this, n);                     \
  } while (0)

#define DEBUG_OBJ_ALLOC(name)                                  \
  do {                                                         \
    if (!g_debug_obj_##name) {                                 \
      g_debug_obj_##name =                                     \
          shcore::debug::debug_object_enable(STRINGIFY(name)); \
    }                                                          \
    g_debug_obj_##name->on_alloc(this);                        \
  } while (0)

#define DEBUG_OBJ_ALLOC2(name, get_debug)                      \
  do {                                                         \
    if (!g_debug_obj_##name) {                                 \
      g_debug_obj_##name =                                     \
          shcore::debug::debug_object_enable(STRINGIFY(name)); \
    }                                                          \
    g_debug_obj_##name->on_alloc(this, get_debug);             \
  } while (0)

#define DEBUG_OBJ_DEALLOC(name) g_debug_obj_##name->on_dealloc(this)

#define DEBUG_OBJ_MALLOC(name, ptr)                            \
  do {                                                         \
    if (!g_debug_obj_##name) {                                 \
      g_debug_obj_##name =                                     \
          shcore::debug::debug_object_enable(STRINGIFY(name)); \
    }                                                          \
    g_debug_obj_##name->on_alloc(ptr);                         \
  } while (0)

#define DEBUG_OBJ_MALLOC_N(name, ptr, tag)                     \
  do {                                                         \
    if (!g_debug_obj_##name) {                                 \
      g_debug_obj_##name =                                     \
          shcore::debug::debug_object_enable(STRINGIFY(name)); \
    }                                                          \
    g_debug_obj_##name->on_alloc(ptr, tag);                    \
  } while (0)

#define DEBUG_OBJ_FREE(name, ptr) g_debug_obj_##name->on_dealloc(ptr)

class Debug_object_info {
 public:
  std::string name;
  uint32_t allocs = 0;
  uint32_t deallocs = 0;
  bool track_instances = false;
  bool fatal_leaks = false;
  std::set<void *> instances;
  std::map<void *, std::string> instance_tags;
  std::function<std::string(void*)> get_debug;
  void dump();

 public:
  explicit Debug_object_info(const std::string &n);
  void on_alloc(void *p, const std::string &tag = "");
  void on_alloc(void *p, std::function<std::string(void *)> get_debug,
                const std::string &tag = "");
  void on_dealloc(void *p);
};

Debug_object_info *debug_object_enable(const char *name);
Debug_object_info *debug_object_enable_fatal(const char *name);

template <typename C>
class Debug_object_for {
 public:
  Debug_object_for() {
    debug_object_enable(typeid(C).name())->on_alloc(this);
  }

  virtual ~Debug_object_for() {
    debug_object_enable(typeid(C).name())->on_dealloc(this);
  }
};

bool debug_object_dump_report(bool verbose);

#else  // NDEBUG

#define DEBUG_OBJ_FOR_CLASS_(klass)

#define DEBUG_OBJ_FOR_CLASS(klass)

#define DEBUG_OBJ_ENABLE(name)

#define DEBUG_OBJ_ALLOC(name) \
  do {                        \
  } while (0)

#define DEBUG_OBJ_ALLOC2(name, get_debug) \
  do {} while (0)

#define DEBUG_OBJ_ALLOC_N(name, n) \
  do {                        \
  } while (0)

#define DEBUG_OBJ_DEALLOC(name) \
  do {                          \
  } while (0)

#define DEBUG_OBJ_MALLOC(name, ptr) \
  do {                              \
  } while (0)

#define DEBUG_OBJ_MALLOC_N(name, ptr, n) \
  do {                                   \
  } while (0)

#define DEBUG_OBJ_FREE(name, ptr) \
  do {                            \
  } while (0)

#endif  // NDEBUG

}  // namespace debug
}  // namespace shcore

#endif  // MYSQLSHDK_LIBS_UTILS_DEBUG_H_
