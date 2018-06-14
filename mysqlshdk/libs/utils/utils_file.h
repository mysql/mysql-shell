/*
 * Copyright (c) 2015, 2018, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MYSQLSHDK_LIBS_UTILS_UTILS_FILE_H_
#define MYSQLSHDK_LIBS_UTILS_UTILS_FILE_H_

#include <functional>
#include <string>
#include <vector>
#include "scripting/common.h"

namespace shcore {

std::string SHCORE_PUBLIC get_global_config_path();
std::string SHCORE_PUBLIC get_user_config_path();
std::string SHCORE_PUBLIC get_mysqlx_home_path();
std::string SHCORE_PUBLIC get_binary_path();
std::string SHCORE_PUBLIC get_binary_folder();
std::string SHCORE_PUBLIC get_share_folder();
std::string SHCORE_PUBLIC get_mp_path();
bool SHCORE_PUBLIC is_folder(const std::string &filename);
bool SHCORE_PUBLIC file_exists(const std::string &filename);
void SHCORE_PUBLIC ensure_dir_exists(const std::string &path);  // delme
void SHCORE_PUBLIC create_directory(const std::string &path,
                                    bool recursive = true);
void SHCORE_PUBLIC remove_directory(const std::string &path,
                                    bool recursive = true);
std::string SHCORE_PUBLIC get_last_error();
bool SHCORE_PUBLIC load_text_file(const std::string &path, std::string &data);
std::string SHCORE_PUBLIC get_text_file(const std::string &path);
void SHCORE_PUBLIC delete_file(const std::string &filename, bool quiet = true);
bool SHCORE_PUBLIC create_file(const std::string &name,
                               const std::string &content);
void SHCORE_PUBLIC copy_file(const std::string &from, const std::string &to,
                             bool copy_attributes = false);
void SHCORE_PUBLIC copy_dir(const std::string &from, const std::string &to);
void SHCORE_PUBLIC rename_file(const std::string &from, const std::string &to);
std::string SHCORE_PUBLIC get_home_dir();
std::vector<std::string> SHCORE_PUBLIC listdir(const std::string &path);

bool SHCORE_PUBLIC iterdir(const std::string &path,
                           const std::function<bool(const std::string &)> &fun);

void SHCORE_PUBLIC check_file_writable_or_throw(const std::string &filename);

}  // namespace shcore

#endif  // MYSQLSHDK_LIBS_UTILS_UTILS_FILE_H_
