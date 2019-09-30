/*
 * Copyright (c) 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "modules/mod_os.h"

#include "mysqlshdk/include/shellcore/utils_help.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_path.h"

#include "modules/mod_path.h"

namespace mysqlsh {

REGISTER_HELP_GLOBAL_OBJECT_MODE(os, shellapi, JAVASCRIPT);
REGISTER_HELP(OS_BRIEF,
              "Gives access to functions which allow to interact with the "
              "operating system.");
REGISTER_HELP(OS_GLOBAL_BRIEF, "${OS_BRIEF}");

/**
 * $(PATH_BRIEF)
 */
#if DOXYGEN_JS
Path Os::path;
#endif

Os::Os() : m_path{std::make_shared<Path>()} {
  add_property("path");

  expose("getenv", &Os::getenv, "name");
  expose("getcwd", &Os::getcwd);
  expose("sleep", &Os::sleep, "seconds");
  expose("loadTextFile", &Os::load_text_file, "path");
  expose("get_user_config_path", &Os::get_user_config_path);
  expose("get_mysqlx_home_path", &Os::get_mysqlx_home_path);
  expose("get_binary_folder", &Os::get_binary_folder);
  expose("file_exists", &Os::file_exists, "path");
  expose("load_text_file", &Os::load_text_file_, "path");
}

shcore::Value Os::get_member(const std::string &prop) const {
  if (prop == "path") {
    return shcore::Value(m_path);
  } else {
    return Cpp_object_bridge::get_member(prop);
  }
}

REGISTER_HELP_FUNCTION_MODE(getcwd, os, JAVASCRIPT);
REGISTER_HELP_FUNCTION_TEXT(OS_GETCWD, R"*(
Retrieves the absolute path of the current working directory.

@returns The absolute path of the current working directory.
)*");

#if DOXYGEN_JS
/**
 * $(OS_GETCWD_BRIEF)
 *
 * $(OS_GETCWD)
 */
String Os::getcwd() {}
#endif
std::string Os::getcwd() const { return shcore::path::getcwd(); }

REGISTER_HELP_FUNCTION_MODE(getenv, os, JAVASCRIPT);
REGISTER_HELP_FUNCTION_TEXT(OS_GETENV, R"*(
Retrieves the value of the specified environment variable.

@param name The name of the environment variable.

@returns The value of environment variable, or null if given variable does not
exist.
)*");

#if DOXYGEN_JS
/**
 * $(OS_GETENV_BRIEF)
 *
 * $(OS_GETENV)
 */
String Os::getenv(String name) {}
#endif
shcore::Value Os::getenv(const std::string &name) const {
  return shcore::Value(::getenv(name.c_str()));
}

REGISTER_HELP_FUNCTION_MODE(loadTextFile, os, JAVASCRIPT);
REGISTER_HELP_FUNCTION_TEXT(OS_LOADTEXTFILE, R"*(
Reads the contents of a text file.

@param path The path to the file to be read.

@returns The contents of the text file.

@throw RuntimeError If the specified file does not exist.
)*");

#if DOXYGEN_JS
/**
 * $(OS_LOADTEXTFILE_BRIEF)
 *
 * $(OS_LOADTEXTFILE)
 */
String Os::loadTextFile(String path) {}
#endif
std::string Os::load_text_file(const std::string &path) const {
  return shcore::get_text_file(path);
}

REGISTER_HELP_FUNCTION_MODE(sleep, os, JAVASCRIPT);
REGISTER_HELP_FUNCTION_TEXT(OS_SLEEP, R"*(
Stops the execution for the given number of seconds.

@param seconds The number of seconds to sleep for.
)*");

#if DOXYGEN_JS
/**
 * $(OS_SLEEP_BRIEF)
 *
 * $(OS_SLEEP)
 */
Undefined Os::sleep(Number seconds) {}
#endif
void Os::sleep(double sec) const { return shcore::sleep_ms(1000.0 * sec); }

REGISTER_HELP_FUNCTION_MODE(get_user_config_path, os, JAVASCRIPT);
REGISTER_HELP_FUNCTION_TEXT(OS_GET_USER_CONFIG_PATH, R"*(
Provides the path to the shell configuration folder.

@returns The path to the shell configuration folder.

@attention This function will be removed in a future release.
)*");

#if DOXYGEN_JS
/**
 * $(OS_GET_USER_CONFIG_PATH_BRIEF)
 *
 * $(OS_GET_USER_CONFIG_PATH)
 */
String Os::get_user_config_path() {}
#endif
std::string Os::get_user_config_path() const {
  log_warning(
      "'os.get_user_config_path()' is deprecated and will be removed in a "
      "future release.");
  return shcore::get_user_config_path();
}

REGISTER_HELP_FUNCTION_MODE(get_mysqlx_home_path, os, JAVASCRIPT);
REGISTER_HELP_FUNCTION_TEXT(OS_GET_MYSQLX_HOME_PATH, R"*(
Provides the path to the home folder for the shell.

@returns The path to the home folder for the shell.

@attention This function will be removed in a future release.
)*");

#if DOXYGEN_JS
/**
 * $(OS_GET_MYSQLX_HOME_PATH_BRIEF)
 *
 * $(OS_GET_MYSQLX_HOME_PATH)
 */
String Os::get_mysqlx_home_path() {}
#endif
std::string Os::get_mysqlx_home_path() const {
  log_warning(
      "'os.get_mysqlx_home_path()' is deprecated and will be removed in a "
      "future release.");
  return shcore::get_mysqlx_home_path();
}

REGISTER_HELP_FUNCTION_MODE(get_binary_folder, os, JAVASCRIPT);
REGISTER_HELP_FUNCTION_TEXT(OS_GET_BINARY_FOLDER, R"*(
Provides the path to the folder which contains the binary of shell.

@returns The path to the folder which contains the binary of shell.

@attention This function will be removed in a future release.
)*");

#if DOXYGEN_JS
/**
 * $(OS_GET_BINARY_FOLDER_BRIEF)
 *
 * $(OS_GET_BINARY_FOLDER)
 */
String Os::get_binary_folder() {}
#endif
std::string Os::get_binary_folder() const {
  log_warning(
      "'os.get_binary_folder()' is deprecated and will be removed in a future "
      "release.");
  return shcore::get_binary_folder();
}

REGISTER_HELP_FUNCTION_MODE(file_exists, os, JAVASCRIPT);
REGISTER_HELP_FUNCTION_TEXT(OS_FILE_EXISTS, R"*(
Checks if the given path exists and is a file.

@param path A file-system path.

@returns true if path points to an existing file.

@attention This function will be removed in a future release, use the <b>os.path.isfile()</b> function instead.
)*");

#if DOXYGEN_JS
/**
 * $(OS_FILE_EXISTS_BRIEF)
 *
 * $(OS_FILE_EXISTS)
 */
Bool Os::file_exists(String path) {}
#endif
bool Os::file_exists(const std::string &path) const {
  log_warning(
      "'os.file_exists()' is deprecated, use 'os.path.isfile()' instead.");
  return shcore::is_file(path);
}

REGISTER_HELP_FUNCTION_MODE(load_text_file, os, JAVASCRIPT);
REGISTER_HELP_FUNCTION_TEXT(OS_LOAD_TEXT_FILE, R"*(
Reads the contents of a text file.

@param path The path to the file to be read.

@returns The contents of the text file.

@attention This function will be removed in a future release, use the <b>loadTextFile()</b> function instead.

@throw RuntimeError If the specified file does not exist.
)*");

#if DOXYGEN_JS
/**
 * $(OS_LOAD_TEXT_FILE_BRIEF)
 *
 * $(OS_LOAD_TEXT_FILE)
 */
String Os::load_text_file(String path) {}
#endif
std::string Os::load_text_file_(const std::string &path) const {
  log_warning(
      "'os.load_text_file()' is deprecated, use 'os.loadTextFile()' instead.");
  return load_text_file(path);
}

}  // namespace mysqlsh
