/*
 * Copyright (c) 2017, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include <gtest/gtest.h>
#include <tuple>
#include "utils/utils_path.h"

namespace shcore {
namespace path {

#ifdef WIN32
TEST(utils_path, join_path) {
  // UNC path
  std::vector<std::string> vec{"//host/computer/dir", "user1"};
  std::string res = join_path(vec);
  EXPECT_EQ("//host/computer/dir\\user1", res);
  EXPECT_EQ("//host/computer/dir\\user1",
            join_path("//host/computer/dir", "user1"));

  vec = {"//host/computer/dir", "user1", "subdir"};
  res = join_path(vec);
  EXPECT_EQ("//host/computer/dir\\user1\\subdir",
            join_path("//host/computer/dir", "user1", "subdir"));
  EXPECT_EQ("//host/computer/dir\\user1\\subdir", res);
  // UNC path + another UNC path
  vec = {"//host/computer/dir", "stuff", "//host2/computer", "subdir"};
  res = join_path(vec);
  EXPECT_EQ(
      "//host2/computer\\subdir",
      join_path("//host/computer/dir", "stuff", "//host2/computer", "subdir"));
  EXPECT_EQ("//host2/computer\\subdir", res);

  // UNC path + absolute path
  vec = {"//host/computer/dir", "computer/user1", "C:\\Users"};
  res = join_path(vec);
  EXPECT_EQ("C:\\Users",
            join_path("//host/computer/dir", "computer/user1", "C:\\Users"));
  EXPECT_EQ("C:\\Users", res);

  // UNC path + absolute path
  vec = {"//host/computer/dir", "computer/user1", "//host2/computer"};
  res = join_path(vec);
  EXPECT_EQ(
      "//host2/computer",
      join_path("//host/computer/dir", "computer/user1", "//host2/computer"));
  EXPECT_EQ("//host2/computer", res);

  // UNC path + relative path
  vec = {"//host/computer/dir", "computer\\user1"};
  res = join_path(vec);
  EXPECT_EQ("//host/computer/dir\\computer\\user1",
            join_path("//host/computer/dir", "computer\\user1"));
  EXPECT_EQ("//host/computer/dir\\computer\\user1", res);

  // same drive, different case (lower/upper)
  vec = {"C:\\Users", "c:"};
  res = join_path(vec);
  EXPECT_EQ("c:\\Users\\", join_path("C:\\Users", "c:"));
  EXPECT_EQ("c:\\Users\\", res);

  vec = {"C:\\Users", "c:", "User1"};
  res = join_path(vec);
  EXPECT_EQ("c:\\Users\\User1", join_path("C:\\Users", "c:", "User1"));
  EXPECT_EQ("c:\\Users\\User1", res);

  // Path separator is only added if needed
  vec = {"C:\\Users", "folder", "subfolder"};
  res = join_path(vec);
  EXPECT_EQ("C:\\Users\\folder\\subfolder",
            join_path("C:\\Users", "folder", "subfolder"));
  EXPECT_EQ("C:\\Users\\folder\\subfolder", res);

  vec = {"C:\\Users", "folder\\", "subfolder"};
  res = join_path(vec);
  EXPECT_EQ("C:\\Users\\folder\\subfolder",
            join_path("C:\\Users", "folder\\", "subfolder"));
  EXPECT_EQ("C:\\Users\\folder\\subfolder", res);

  vec = {"C:\\Users", "user1"};
  res = join_path(vec);
  EXPECT_EQ("C:\\Users\\user1", join_path("C:\\Users", "user1"));
  EXPECT_EQ("C:\\Users\\user1", res);

  vec = {"C:\\Users\\user1", "folder\\subfolder"};
  EXPECT_EQ("C:\\Users\\user1\\folder\\subfolder",
            join_path("C:\\Users\\user1", "folder\\subfolder"));
  EXPECT_EQ("C:\\Users\\user1\\folder\\subfolder", join_path(vec));

  vec = {};
  res = join_path(vec);
  EXPECT_EQ("", res);

  vec = {""};
  res = join_path(vec);
  EXPECT_EQ("", res);

  vec = {"C:\\what"};
  res = join_path(vec);
  EXPECT_EQ("C:\\what", res);

  vec = {"", "what"};
  res = join_path(vec);
  EXPECT_EQ("what", join_path("", "what"));
  EXPECT_EQ("what", res);

  vec = {"", "what", ""};
  res = join_path(vec);
  EXPECT_EQ("what\\", join_path("", "what", ""));
  EXPECT_EQ("what\\", res);
}

TEST(utils_path, splitdrive) {
  auto res = splitdrive("//host/computer/dir");
  EXPECT_EQ("//host/computer", res.first);
  EXPECT_EQ("/dir", res.second);

  // If drive part is invalid, then it should be empty and everything should go
  // to the path
  res = splitdrive("C\\");
  EXPECT_EQ("", res.first);
  EXPECT_EQ("C\\", res.second);

  res = splitdrive("C:");
  EXPECT_EQ("C:", res.first);
  EXPECT_EQ("", res.second);

  res = splitdrive("C:\\");
  EXPECT_EQ("C:", res.first);
  EXPECT_EQ("\\", res.second);

  res = splitdrive("c:\\dir");
  EXPECT_EQ("c:", res.first);
  EXPECT_EQ("\\dir", res.second);

  res = splitdrive("/");
  EXPECT_EQ("", res.first);
  EXPECT_EQ("/", res.second);

  res = splitdrive("//not//unc/n");
  EXPECT_EQ("", res.first);
  EXPECT_EQ("//not//unc/n", res.second);

  res = splitdrive("//not");
  EXPECT_EQ("", res.first);
  EXPECT_EQ("//not", res.second);

  res = splitdrive("//not/unc");
  EXPECT_EQ("//not/unc", res.first);
  EXPECT_EQ("", res.second);
}

TEST(utils_path, normalize) {}

#else

TEST(utils_path, join_path) {
  std::vector<std::string> vec{"/root/is/not/", "what/", "4/"};
  std::string res = join_path(vec);
  EXPECT_EQ("/root/is/not/what/4/", res);

  vec = {};
  res = join_path(vec);
  EXPECT_EQ("", res);

  vec = {"what"};
  res = join_path(vec);
  EXPECT_EQ("what", res);

  vec = {"", "what"};
  res = join_path(vec);
  EXPECT_EQ("what", join_path("", "what"));
  EXPECT_EQ("what", res);

  vec = {"/root", "dir", ""};
  res = join_path(vec);
  EXPECT_EQ("/root/dir/", join_path("/root", "dir", ""));
  EXPECT_EQ("/root/dir/", res);

  // path separator is only added if needed
  vec = {"/root", "dir", "subdir"};
  res = join_path(vec);
  EXPECT_EQ("/root/dir/subdir", join_path("/root", "dir", "subdir"));
  EXPECT_EQ("/root/dir/subdir", res);

  vec = {"/root", "dir/", "subdir"};
  res = join_path(vec);
  EXPECT_EQ("/root/dir/subdir", join_path("/root", "dir/", "subdir"));
  EXPECT_EQ("/root/dir/subdir", res);

  vec = {"/root", "dir", "/another/absolute", "path"};
  res = join_path(vec);
  EXPECT_EQ("/another/absolute/path",
            join_path("/root", "dir", "/another/absolute", "path"));
  EXPECT_EQ("/another/absolute/path", res);

  vec = {"/home/user", "folder/subfolder"};
  EXPECT_EQ("/home/user/folder/subfolder",
            join_path("/home/user", "folder/subfolder"));
  EXPECT_EQ("/home/user/folder/subfolder", join_path(vec));
}

TEST(utils_path, splitdrive) {
  // on unix, the drive (first part of the pair) is always empty
  auto res = splitdrive("");
  EXPECT_EQ("", res.first);
  EXPECT_EQ("", res.second);

  res = splitdrive("/root/drive/folder");
  EXPECT_EQ("", res.first);
  EXPECT_EQ("/root/drive/folder", res.second);
}

TEST(utils_path, split_extension) {
  EXPECT_EQ(std::make_tuple("", ""), split_extension(""));
  EXPECT_EQ(std::make_tuple("file", ""), split_extension("file"));
  EXPECT_EQ(std::make_tuple(".dot-file", ""), split_extension(".dot-file"));
  EXPECT_EQ(std::make_tuple("/home/user/", ""), split_extension("/home/user/"));
  EXPECT_EQ(std::make_tuple("/home/user/.dot-file", ""),
            split_extension("/home/user/.dot-file"));
  EXPECT_EQ(std::make_tuple("/home/user/..dot-file", ""),
            split_extension("/home/user/..dot-file"));
  EXPECT_EQ(std::make_tuple("binary", ".exe"), split_extension("binary.exe"));
  EXPECT_EQ(std::make_tuple("./binary", ".exe"),
            split_extension("./binary.exe"));
  EXPECT_EQ(std::make_tuple("/home/user/binary", ".exe"),
            split_extension("/home/user/binary.exe"));
  EXPECT_EQ(std::make_tuple("/home/name.surname/x", ".exe"),
            split_extension("/home/name.surname/x.exe"));
  EXPECT_EQ(std::make_tuple("/home/name.surname/.dot-file", ""),
            split_extension("/home/name.surname/.dot-file"));
  EXPECT_EQ(std::make_tuple("/home/name.surname/file", ""),
            split_extension("/home/name.surname/file"));
  EXPECT_EQ(std::make_tuple("/home/name.surname/.file", ".exe"),
            split_extension("/home/name.surname/.file.exe"));
  EXPECT_EQ(std::make_tuple("/home/name.surname/..file", ".exe"),
            split_extension("/home/name.surname/..file.exe"));
  EXPECT_EQ(std::make_tuple("/home/name.surname/...", ""),
            split_extension("/home/name.surname/..."));
  EXPECT_EQ(std::make_tuple("...", ""), split_extension("..."));
  EXPECT_EQ(std::make_tuple("...file", ".txt"), split_extension("...file.txt"));
  EXPECT_EQ(std::make_tuple(".", ""), split_extension("."));
  EXPECT_EQ(std::make_tuple("/a/b.x/c/test", ""),
            split_extension("/a/b.x/c/test"));
}

TEST(utils_path, normalize) {
  EXPECT_EQ(".", normalize(""));
  EXPECT_EQ("/", normalize("/"));
  EXPECT_EQ(".", normalize("./"));
  EXPECT_EQ("..", normalize("./../"));
  EXPECT_EQ("../a", normalize("./../a"));
  EXPECT_EQ("/", normalize("/a/b/c/../../../../../"));
  EXPECT_EQ("/d/e", normalize("/a/b/c/../../../../../d/e/"));
  EXPECT_EQ("/a", normalize("/a/b/c/../../"));
  EXPECT_EQ("a", normalize("a/"));
  EXPECT_EQ("a", normalize("a"));
  EXPECT_EQ("a/b", normalize("a/b"));
  EXPECT_EQ("a/b/c", normalize("a/b/c"));
  EXPECT_EQ("a/b/c", normalize("a/b/./c"));
  EXPECT_EQ("a/c", normalize("a/b/../c"));
  EXPECT_EQ("a/c", normalize("a//b/../c"));

  EXPECT_EQ("a/b", normalize("./a/b"));
  EXPECT_EQ("../a/b", normalize(".././a/b"));
  EXPECT_EQ("../../a/b", normalize("../.././a/b"));
  EXPECT_EQ("../../a/b", normalize(".././.././a/b"));
  EXPECT_EQ("../../a", normalize(".././.././a/b/../"));
  EXPECT_EQ("../..", normalize(".././.././a/b/../../"));
  EXPECT_EQ("../../..", normalize(".././.././a/b/../../../"));
  EXPECT_EQ("../../../..", normalize(".././.././a/b/../../../../"));
  EXPECT_EQ(".", normalize("a/../"));
  EXPECT_EQ("..", normalize("a/../.."));
  EXPECT_EQ("../..", normalize("a/../../../"));

  EXPECT_EQ("/a/b/c", normalize("///a/b/c"));
  EXPECT_EQ("/a/b/c", normalize("////a/b/c"));
  EXPECT_EQ("/a/b/c", normalize("/../a/b/c"));

  EXPECT_EQ("//", normalize("//"));
  EXPECT_EQ("//a", normalize("//a"));
  EXPECT_EQ("//a", normalize("//a/"));
  EXPECT_EQ("//a/b/c", normalize("//a/b/c"));
}
#endif

// both basename and dirname are supposed to behave the same as
// /usr/bin/basename and /usr/bin/dirname
TEST(utils_path, basename) {
  EXPECT_EQ("", basename(""));
  EXPECT_EQ("/", basename("/"));
  EXPECT_EQ("/", basename("////"));
  EXPECT_EQ("foo", basename("foo"));
  EXPECT_EQ("foo.txt", basename("foo.txt"));
  EXPECT_EQ("foo.txt", basename("/foo.txt"));
  EXPECT_EQ("foo.txt", basename("/bla/foo.txt"));
  EXPECT_EQ("foo", basename("/bla/foo/"));
  EXPECT_EQ("foo", basename("./foo"));
  EXPECT_EQ("foo", basename("/../foo"));
  EXPECT_EQ("foo.", basename("/../foo."));
  EXPECT_EQ(".foo", basename("/../.foo"));
  EXPECT_EQ(".", basename("/../."));
  EXPECT_EQ(".", basename("."));
  EXPECT_EQ(".", basename("./"));
  EXPECT_EQ("..", basename(".."));
  EXPECT_EQ("..", basename("/.."));
#ifdef _WIN32
  EXPECT_EQ("foo.txt", basename("\\foo.txt"));
  EXPECT_EQ("foo.txt", basename("bla\\foo.txt"));
  EXPECT_EQ("foo.txt", basename("\\bla\\foo.txt"));
  EXPECT_EQ("foo.txt", basename("\\foo.txt"));
  EXPECT_EQ("foo.txt", basename("bla\\foo.txt"));
  EXPECT_EQ("foo.txt", basename("\\bla/foo.txt"));
  EXPECT_EQ("foo.txt", basename("/bla\\foo.txt"));
  EXPECT_EQ("foo.txt", basename("c:foo.txt"));
  EXPECT_EQ("foo.txt", basename("c:\\foo.txt"));
  EXPECT_EQ("foo.txt", basename("c:\\bla\\foo.txt"));
#endif
}

TEST(utils_path, dirname) {
  EXPECT_EQ("/", dirname("/"));
  EXPECT_EQ("/", dirname("//"));
  EXPECT_EQ("/", dirname("/foo"));
  EXPECT_EQ("/", dirname("/foo/"));
  EXPECT_EQ("/", dirname("/foo//"));
  // In windows, //host/path
#ifndef _WIN32
  EXPECT_EQ("/", dirname("//foo/"));
  EXPECT_EQ("/", dirname("//foo//"));
#endif
  EXPECT_EQ("//foo", dirname("//foo//bar"));
  EXPECT_EQ("//foo", dirname("//foo//bar//"));
  EXPECT_EQ("//foo//bar", dirname("//foo//bar//."));

  EXPECT_EQ(".", dirname(""));
  EXPECT_EQ(".", dirname("."));
  EXPECT_EQ(".", dirname(".."));
  EXPECT_EQ(".", dirname(".."));
  EXPECT_EQ(".", dirname("./"));
  EXPECT_EQ(".", dirname(".////"));

  EXPECT_EQ("/..", dirname("/../.."));
  EXPECT_EQ("/..", dirname("/../../"));

  EXPECT_EQ("foo", dirname("foo/bar"));
  EXPECT_EQ("foo", dirname("foo/bar/"));
  EXPECT_EQ("foo/bar", dirname("foo/bar/bla"));
  EXPECT_EQ("foo//bar", dirname("foo//bar/bla"));
  EXPECT_EQ("../bla", dirname("../bla/ble"));
  EXPECT_EQ("./bla", dirname("./bla/ble"));

#ifdef _WIN32
  EXPECT_EQ("c:\\", dirname("c:\\foo"));
  EXPECT_EQ("\\", dirname("\\foo"));
  EXPECT_EQ("\\", dirname("\\"));
  EXPECT_EQ("\\", dirname("\\\\"));
  EXPECT_EQ("\\", dirname("\\foo"));
  EXPECT_EQ("\\", dirname("\\foo\\"));
  EXPECT_EQ("\\", dirname("\\foo\\\\"));
  EXPECT_EQ("c:\\", dirname("c:\\foo\\"));
  EXPECT_EQ("c:\\", dirname("c:\\foo\\\\"));

  EXPECT_EQ("\\..", dirname("\\..\\.."));
  EXPECT_EQ("\\..", dirname("\\..\\..\\"));

  EXPECT_EQ("foo", dirname("foo\\bar"));
  EXPECT_EQ("foo", dirname("foo\\bar\\"));
  EXPECT_EQ("foo\\bar", dirname("foo\\bar\\bla"));
  EXPECT_EQ("foo\\\\bar", dirname("foo\\\\bar\\bla"));
  EXPECT_EQ("..\\bla", dirname("..\\bla\\ble"));
  EXPECT_EQ(".\\bla", dirname(".\\bla\\ble"));

  EXPECT_EQ("bla/foo", dirname("bla/foo\\bar"));
  EXPECT_EQ("foo\\bar", dirname("foo\\bar\\bla/"));
  EXPECT_EQ("f/oo\\bar", dirname("f/oo\\bar\\bla"));
  EXPECT_EQ("..\\bla/..", dirname("..\\bla/..\\ble"));
  EXPECT_EQ(".\\bla", dirname(".\\bla/ble"));
  EXPECT_EQ(".", dirname(".\\bla/"));
#endif
}

TEST(utils_path, search_stdpath) {
#if defined(_WIN32)
  // Case insensitive comparison required on windows paths
  EXPECT_STRCASEEQ("C:\\windows\\system32\\cmd.exe",
                   search_stdpath("cmd.exe").c_str());
  EXPECT_STRCASEEQ("C:\\windows\\system32\\cmd.exe",
                   search_stdpath("cmd").c_str());
  EXPECT_EQ("", search_stdpath("bogus path"));
#elif defined(__sun)
  EXPECT_EQ("/usr/bin/bash", search_stdpath("bash"));
  EXPECT_EQ("", search_stdpath("bogus path"));
#else
  EXPECT_EQ("/bin/bash", search_stdpath("bash"));
  EXPECT_EQ("", search_stdpath("bogus path"));
#endif
}

TEST(utils_path, is_absolute) {
#if defined(_WIN32)
  EXPECT_TRUE(is_absolute("//path/to/share"));
  EXPECT_TRUE(is_absolute("\\Program Files\\"));
  EXPECT_TRUE(is_absolute("C:\\Documents\\"));
  EXPECT_TRUE(is_absolute("F:/files/"));

  EXPECT_FALSE(is_absolute("folders\\files"));
  EXPECT_FALSE(is_absolute("./folders\\files"));
  EXPECT_FALSE(is_absolute(".\\folders\\files"));
  EXPECT_FALSE(is_absolute("../folders\\files"));
  EXPECT_FALSE(is_absolute("..\\folders\\files"));
  EXPECT_FALSE(is_absolute("X:Documents\\"));

  // non-ascii support
  EXPECT_TRUE(is_absolute("//pąth/tÓ/śhąre"));
  EXPECT_TRUE(is_absolute("\\Prógrąm Filęs\\"));
  EXPECT_TRUE(is_absolute("C:\\Doćuments\\"));
  EXPECT_TRUE(is_absolute("F:/filęŚ/"));

  EXPECT_FALSE(is_absolute("fOldęrs\\filęś"));
  EXPECT_FALSE(is_absolute("./fólders\\filęś"));
  EXPECT_FALSE(is_absolute(".\\foldęrs\\filęs"));
  EXPECT_FALSE(is_absolute("../fólderś\\fiłęs"));
  EXPECT_FALSE(is_absolute("..\\foldęrs\\fiłes"));
  EXPECT_FALSE(is_absolute("X:Dócumęnts\\"));
#else   // !_WIN32
  EXPECT_TRUE(is_absolute("//path/to/file"));
  EXPECT_TRUE(is_absolute("/path/to/file"));
  EXPECT_TRUE(is_absolute("~/data"));

  EXPECT_FALSE(is_absolute("./folders/files"));
  EXPECT_FALSE(is_absolute("../folders/files"));

  // non-ascii support
  EXPECT_TRUE(is_absolute("//pąth/to/filę"));
  EXPECT_TRUE(is_absolute("/pąth/tó/file"));
  EXPECT_TRUE(is_absolute("~/dąta"));

  EXPECT_FALSE(is_absolute("./fólders/fiłęs"));
  EXPECT_FALSE(is_absolute("../foldęrs/fiłęs"));
#endif  // !_WIN32
}

}  // namespace path
}  // namespace shcore
