/*
 * Copyright (c) 2015, 2020, Oracle and/or its affiliates. All rights reserved.
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

/**
 * Tells if path is a regular file.
 *
 * @param path Path
 * @return true if path is a regular file.
 *         false if path is not a regular file.
 */
bool SHCORE_PUBLIC is_file(const char *path);
#ifdef _WIN32
bool SHCORE_PUBLIC is_file(const char *path, const size_t path_length);
#endif
bool SHCORE_PUBLIC is_file(const std::string &path);

/**
 * Tells if path is a FIFO file.
 *
 * @param path Path to the file
 * @return true if a path is a FIFO file.
 *         false if path is not a FIFO file.
 */
bool is_fifo(const char *path);
bool is_fifo(const std::string &path);

/**
 * Retrieves the size of the specified file under path, in bytes.
 *
 * @param path Path to file.
 * @return Size of file under the path, in bytes.
 */
size_t file_size(const char *path);
size_t file_size(const std::string &path);

bool SHCORE_PUBLIC is_folder(const std::string &filename);
bool SHCORE_PUBLIC path_exists(const std::string &path);
void SHCORE_PUBLIC ensure_dir_exists(const std::string &path);  // delme
void SHCORE_PUBLIC create_directory(const std::string &path,
                                    bool recursive = true, int mode = 0700);
void SHCORE_PUBLIC remove_directory(const std::string &path,
                                    bool recursive = true);
std::string SHCORE_PUBLIC get_last_error();
bool SHCORE_PUBLIC load_text_file(const std::string &path, std::string &data);
std::string SHCORE_PUBLIC get_text_file(const std::string &path);
void SHCORE_PUBLIC delete_file(const std::string &filename, bool quiet = true);
bool SHCORE_PUBLIC create_file(const std::string &name,
                               const std::string &content,
                               bool binary_mode = false);
void SHCORE_PUBLIC copy_file(const std::string &from, const std::string &to,
                             bool copy_attributes = false);
void SHCORE_PUBLIC copy_dir(const std::string &from, const std::string &to);
void SHCORE_PUBLIC rename_file(const std::string &from, const std::string &to);
std::string SHCORE_PUBLIC get_home_dir();
std::vector<std::string> SHCORE_PUBLIC listdir(const std::string &path);

bool SHCORE_PUBLIC iterdir(const std::string &path,
                           const std::function<bool(const std::string &)> &fun);

void SHCORE_PUBLIC check_file_writable_or_throw(const std::string &filename);
int SHCORE_PUBLIC make_file_readonly(const std::string &path);
int SHCORE_PUBLIC ch_mod(const std::string &path, int mode);
int SHCORE_PUBLIC set_user_only_permissions(const std::string &path);

std::string SHCORE_PUBLIC get_absolute_path(const std::string &file_path,
                                            const std::string &base_dir = "");

/**
 * Calculate a temporary file path based on a given file_path.
 *
 * This function determines the path for a temporary file to use. The
 * temporary cannot exist. First, a simple '.tmp' extension is appended to the
 * provided file, if that file already  exists it tries to also appends a
 * random generated integer to the extension (e.g., tmp123456). After 5
 * attempts, using a random number in the file name, if they all exist an
 * error is issued.
 *
 * This function is used by the write() function, since any new configuration
 * file is first wrote to a temporary file and only at the end copied/renamed
 * to the target location if no errors occurred. This avoids undesired
 * changes to the target file in case it already exist and an error occurred
 * during the write operation.
 *
 * @param file_path string with the path to the target on which the temporary
 * path will be based upon.
 * @return string with the path of the temporary file to use.
 * @throw std::runtime_error if the function fails to generate a non-existing
 *        temporary file (after 5 attempts using random numbers in the file
 *        name).
 */
std::string SHCORE_PUBLIC get_tempfile_path(const std::string &file_path);
}  // namespace shcore

#endif  // MYSQLSHDK_LIBS_UTILS_UTILS_FILE_H_
