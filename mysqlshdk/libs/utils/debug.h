/*
 * Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#ifndef MYSQLSHDK_LIBS_UTILS_DEBUG_H_
#define MYSQLSHDK_LIBS_UTILS_DEBUG_H_

#include <cstdint>
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

#define DEBUG_OBJ_ENABLE(name) \
  static shcore::debug::Debug_object_info *g_debug_obj_##name = nullptr

#define DEBUG_OBJ_ALLOC(name)                                  \
  do {                                                         \
    if (!g_debug_obj_##name) {                                 \
      g_debug_obj_##name =                                     \
          shcore::debug::debug_object_enable(STRINGIFY(name)); \
    }                                                          \
    g_debug_obj_##name->on_alloc(this);                        \
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

#define DEBUG_OBJ_FREE(name, ptr) g_debug_obj_##name->on_dealloc(ptr)

class Debug_object_info {
 public:
  std::string name;
  uint32_t allocs = 0;
  uint32_t deallocs = 0;
  bool track_instances = false;
  std::set<void *> instances;
  void dump();

 public:
  explicit Debug_object_info(const std::string &n);
  void on_alloc(void *p);
  void on_dealloc(void *p);
};

Debug_object_info *debug_object_enable(const char *name);

bool debug_object_dump_report(bool verbose);

#else  // NDEBUG

#define DEBUG_OBJ_ENABLE(name)

#define DEBUG_OBJ_ALLOC(name) \
  do {                        \
  } while (0)

#define DEBUG_OBJ_DEALLOC(name) \
  do {                          \
  } while (0)


#define DEBUG_OBJ_MALLOC(name, ptr) \
  do {                              \
  } while (0)

#define DEBUG_OBJ_FREE(name, ptr) \
  do {                            \
  } while (0)

#endif  // NDEBUG

}  // namespace debug
}  // namespace shcore

#endif  // MYSQLSHDK_LIBS_UTILS_DEBUG_H_
