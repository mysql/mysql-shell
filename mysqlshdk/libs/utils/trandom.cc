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

#include "mysqlshdk/libs/utils/trandom.h"
#include "mysqlshdk/libs/utils/utils_time.h"

namespace mysqlshdk {
namespace utils {

std::string Random::get_time_string() {
  MySQL_timer timer;
  return std::to_string(timer.get_time());
}

static Random *g_random = nullptr;

Random *Random::get() {
  if (!g_random)
    g_random = new Random();
  return g_random;
}

void Random::set(Random *random) {
  if (g_random)
    delete g_random;
  g_random = random;
}

}  // namespace utils
}  // namespace mysqlshdk
