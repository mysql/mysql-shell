/*
 * Copyright (c) 2015, 2016, Oracle and/or its affiliates. All rights reserved.
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
#include "scripting/common.h"

namespace shcore {
std::string SHCORE_PUBLIC get_global_config_path();
std::string SHCORE_PUBLIC get_user_config_path();
std::string SHCORE_PUBLIC get_mysqlx_home_path();
std::string SHCORE_PUBLIC get_binary_folder();
bool SHCORE_PUBLIC is_folder(const std::string& filename);
bool SHCORE_PUBLIC file_exists(const std::string& filename);
void SHCORE_PUBLIC ensure_dir_exists(const std::string& path);
void SHCORE_PUBLIC remove_directory(const std::string& path);
std::string SHCORE_PUBLIC get_last_error();
bool SHCORE_PUBLIC load_text_file(const std::string& path, std::string& data);
void SHCORE_PUBLIC delete_file(const std::string& filename);
bool SHCORE_PUBLIC create_file(const std::string& name,
                               const std::string& content);
std::string SHCORE_PUBLIC get_home_dir();
}
#endif /* defined(__mysh__utils_file__) */
