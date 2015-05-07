/*
 * Copyright (c) 2015, Oracle and/or its affiliates. All rights reserved.
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

#ifndef __mysh__utils_file__
#define __mysh__utils_file__

#include <string>

namespace shcore
{
  std::string get_user_config_path();
  bool file_exists(const std::string& filename);
  void ensure_dir_exists(const std::string& path);
  std::string get_last_error();
  bool load_text_file(const std::string& path, std::string& data);
}
#endif /* defined(__mysh__utils_file__) */
