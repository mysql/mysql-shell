/*
 * Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
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
#if defined(WIN32)
#include <time.h>
#else
#include <sys/times.h>
#ifdef _SC_CLK_TCK  // For mit-pthreads
#undef CLOCKS_PER_SEC
#define CLOCKS_PER_SEC (sysconf(_SC_CLK_TCK))
#endif
#endif

#include "mysqlshdk/libs/utils/trandom.h"

namespace mysqlshdk {
namespace utils {

std::string Random::get_time_string() {
  uint64_t time;

#if defined(WIN32)
  time = clock();
#else
  struct tms tms_tmp;
  time = times(&tms_tmp);
#endif

  return std::to_string(time);
}

static Random *g_random = nullptr;

Random *Random::get() {
  if (!g_random) g_random = new Random();
  return g_random;
}

void Random::set(Random *random) {
  if (g_random) delete g_random;
  g_random = random;
}

}  // namespace utils
}  // namespace mysqlshdk
