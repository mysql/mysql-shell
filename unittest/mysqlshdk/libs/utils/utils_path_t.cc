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

#include <gtest/gtest.h>
#include "utils/utils_path.h"

namespace shcore {
namespace path {

#ifdef WIN32
TEST(utils_path, join_path) {
  // UNC path
  std::vector<std::string> vec{"//host/computer/dir", "user1"};
  std::string res = join_path(vec);
  EXPECT_EQ("//host/computer/dir\\user1", res);

  vec = {"//host/computer/dir", "user1", "subdir"};
  res = join_path(vec);
  EXPECT_EQ("//host/computer/dir\\user1\\subdir", res);
  // UNC path + another UNC path
  vec = {"//host/computer/dir", "stuff", "//host2/computer", "subdir"};
  res = join_path(vec);
  EXPECT_EQ("//host2/computer\\subdir", res);

  // UNC path + absolute path
  vec = {"//host/computer/dir", "computer/user1", "C:\\Users"};
  res = join_path(vec);
  EXPECT_EQ("C:\\Users", res);

  // UNC path + absolute path
  vec = {"//host/computer/dir", "computer/user1", "//host2/computer"};
  res = join_path(vec);
  EXPECT_EQ("//host2/computer", res);

  // UNC path + relative path
  vec = {"//host/computer/dir", "computer\\user1"};
  res = join_path(vec);
  EXPECT_EQ("//host/computer/dir\\computer\\user1", res);

  // same drive, different case (lower/upper)
  vec = {"C:\\Users", "c:"};
  res = join_path(vec);
  EXPECT_EQ("c:\\Users\\", res);

  vec = {"C:\\Users", "c:", "User1"};
  res = join_path(vec);
  EXPECT_EQ("c:\\Users\\User1", res);

  // Path separator is only added if needed
  vec = {"C:\\Users", "folder", "subfolder"};
  res = join_path(vec);
  EXPECT_EQ("C:\\Users\\folder\\subfolder", res);

  vec = {"C:\\Users", "folder\\", "subfolder"};
  res = join_path(vec);
  EXPECT_EQ("C:\\Users\\folder\\subfolder", res);

  vec = {"C:\\Users", "user1"};
  res = join_path(vec);
  EXPECT_EQ("C:\\Users\\user1", res);

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
  EXPECT_EQ("what", res);

  vec = {"", "what", ""};
  res = join_path(vec);
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

TEST(utils_path, normalize) {
}

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
  EXPECT_EQ("what", res);

  vec = {"/root", "dir", ""};
  res = join_path(vec);
  EXPECT_EQ("/root/dir/", res);

  // path separator is only added if needed
  vec = {"/root", "dir", "subdir"};
  res = join_path(vec);
  EXPECT_EQ("/root/dir/subdir", res);

  vec = {"/root", "dir/", "subdir"};
  res = join_path(vec);
  EXPECT_EQ("/root/dir/subdir", res);

  vec = {"/root", "dir", "/another/absolute", "path"};
  res = join_path(vec);
  EXPECT_EQ("/another/absolute/path", res);
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

TEST(utils_path, normalize) {
  EXPECT_EQ(normalize(""), ".");
  EXPECT_EQ(normalize("/"), "/");
  EXPECT_EQ(normalize("./"), ".");
  EXPECT_EQ(normalize("./../"), "..");
  EXPECT_EQ(normalize("./../a"), "../a");
  EXPECT_EQ(normalize("/a/b/c/../../../../../"), "/");
  EXPECT_EQ(normalize("/a/b/c/../../../../../d/e/"), "/d/e");
  EXPECT_EQ(normalize("/a/b/c/../../"), "/a");
  EXPECT_EQ(normalize("a/"), "a");
  EXPECT_EQ(normalize("a"), "a");
  EXPECT_EQ(normalize("a/b"), "a/b");
  EXPECT_EQ(normalize("a/b/c"), "a/b/c");
  EXPECT_EQ(normalize("a/b/./c"), "a/b/c");
  EXPECT_EQ(normalize("a/b/../c"), "a/c");
  EXPECT_EQ(normalize("a//b/../c"), "a/c");

  EXPECT_EQ(normalize("./a/b"), "a/b");
  EXPECT_EQ(normalize(".././a/b"), "../a/b");
  EXPECT_EQ(normalize("../.././a/b"), "../../a/b");
  EXPECT_EQ(normalize(".././.././a/b"), "../../a/b");
  EXPECT_EQ(normalize(".././.././a/b/../"), "../../a");
  EXPECT_EQ(normalize(".././.././a/b/../../"), "../..");
  EXPECT_EQ(normalize(".././.././a/b/../../../"), "../../..");
  EXPECT_EQ(normalize(".././.././a/b/../../../../"), "../../../..");
  EXPECT_EQ(normalize("a/../"), ".");
  EXPECT_EQ(normalize("a/../.."), "..");
  EXPECT_EQ(normalize("a/../../../"), "../..");

  EXPECT_EQ(normalize("///a/b/c"), "/a/b/c");
  EXPECT_EQ(normalize("////a/b/c"), "/a/b/c");
  EXPECT_EQ(normalize("/../a/b/c"), "/a/b/c");

  EXPECT_EQ(normalize("//"), "//");
  EXPECT_EQ(normalize("//a"), "//a");
  EXPECT_EQ(normalize("//a/"), "//a");
  EXPECT_EQ(normalize("//a/b/c"), "//a/b/c");
}
#endif
}  // namespace path
}  // namespace shcore
