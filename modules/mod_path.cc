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

#include "modules/mod_path.h"

#include <vector>

#include "mysqlshdk/include/scripting/type_info/generic.h"
#include "mysqlshdk/include/shellcore/utils_help.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_path.h"

namespace mysqlsh {

REGISTER_HELP_OBJECT_MODE(path, os, JAVASCRIPT);
REGISTER_HELP(PATH_BRIEF, "Gives access to path-related functions.");

Path::Path() {
  expose("basename", &Path::basename, "path");
  expose("dirname", &Path::dirname, "path");
  expose("isabs", &Path::isabs, "path");
  expose("isdir", &Path::isdir, "path");
  expose("isfile", &Path::isfile, "path");
  expose("join", &Path::join, "pathA", "pathB", "?pathC", "?pathD");
  expose("normpath", &Path::normpath, "path");
}

REGISTER_HELP_FUNCTION_MODE(basename, path, JAVASCRIPT);
REGISTER_HELP_FUNCTION_TEXT(PATH_BASENAME, R"*(
Provides the base name from the given path, by stripping the components
before the last path-separator character.

@param path A file-system path.

@returns The base name of the given path.
)*");

#if DOXYGEN_JS
/**
 * $(PATH_BASENAME_BRIEF)
 *
 * $(PATH_BASENAME)
 */
String Path::basename(String path) {}
#endif
std::string Path::basename(const std::string &path) const {
  return shcore::path::basename(path);
}

REGISTER_HELP_FUNCTION_MODE(dirname, path, JAVASCRIPT);
REGISTER_HELP_FUNCTION_TEXT(PATH_DIRNAME, R"*(
Provides the directory name from the given path, by stripping the component
after the last path-separator character.

@param path A file-system path.

@returns The directory name of the given path.
)*");

#if DOXYGEN_JS
/**
 * $(PATH_DIRNAME_BRIEF)
 *
 * $(PATH_DIRNAME)
 */
String Path::dirname(String path) {}
#endif
std::string Path::dirname(const std::string &path) const {
  return shcore::path::dirname(path);
}

REGISTER_HELP_FUNCTION_MODE(isabs, path, JAVASCRIPT);
REGISTER_HELP_FUNCTION_TEXT(PATH_ISABS, R"*(
Checks if the given path is absolute.

@param path A file-system path.

@returns true if the given path is absolute.
)*");

#if DOXYGEN_JS
/**
 * $(PATH_ISABS_BRIEF)
 *
 * $(PATH_ISABS)
 */
Bool Path::isabs(String path) {}
#endif
bool Path::isabs(const std::string &path) const {
  return shcore::path::is_absolute(path);
}

REGISTER_HELP_FUNCTION_MODE(isdir, path, JAVASCRIPT);
REGISTER_HELP_FUNCTION_TEXT(PATH_ISDIR, R"*(
Checks if the given path exists and is a directory.

@param path A file-system path.

@returns true if path points to an existing directory.
)*");

#if DOXYGEN_JS
/**
 * $(PATH_ISDIR_BRIEF)
 *
 * $(PATH_ISDIR)
 */
Bool Path::isdir(String path) {}
#endif
bool Path::isdir(const std::string &path) const {
  return shcore::is_folder(path);
}

REGISTER_HELP_FUNCTION_MODE(isfile, path, JAVASCRIPT);
REGISTER_HELP_FUNCTION_TEXT(PATH_ISFILE, R"*(
Checks if the given path exists and is a file.

@param path A file-system path.

@returns true if path points to an existing file.
)*");

#if DOXYGEN_JS
/**
 * $(PATH_ISFILE_BRIEF)
 *
 * $(PATH_ISFILE)
 */
Bool Path::isfile(String path) {}
#endif
bool Path::isfile(const std::string &path) const {
  return shcore::is_file(path);
}

REGISTER_HELP_FUNCTION_MODE(join, path, JAVASCRIPT);
REGISTER_HELP_FUNCTION_TEXT(PATH_JOIN, R"*(
Joins the path components using the path-separator character.

@param pathA A file-system path component.
@param pathB A file-system path component.
@param pathC Optional A file-system path component.
@param pathD Optional A file-system path component.

@returns The joined path.

If a component is an absolute path, the current result is discarded and
replaced with the current component.
)*");

#if DOXYGEN_JS
/**
 * $(PATH_JOIN_BRIEF)
 *
 * $(PATH_JOIN)
 */
String Path::join(String pathA, String pathB, String pathC, String pathD) {}
#endif
std::string Path::join(const std::string &first, const std::string &second,
                       const std::string &third,
                       const std::string &fourth) const {
  std::vector<std::string> paths{first, second};

  for (const auto &p : {third, fourth}) {
    if (!p.empty()) {
      paths.emplace_back(p);
    }
  }

  return shcore::path::join_path(paths);
}

REGISTER_HELP_FUNCTION_MODE(normpath, path, JAVASCRIPT);
REGISTER_HELP_FUNCTION_TEXT(PATH_NORMPATH, R"*(
Normalizes the given path, collapsing redundant path-separator characters and
relative references.

@param path A file-system path component.

@returns The normalized path.
)*");

#if DOXYGEN_JS
/**
 * $(PATH_NORMPATH_BRIEF)
 *
 * $(PATH_NORMPATH)
 */
String Path::normpath(String path) {}
#endif
std::string Path::normpath(const std::string &path) const {
  return shcore::path::normalize(path);
}

}  // namespace mysqlsh
