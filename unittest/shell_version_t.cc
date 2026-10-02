/*
 * Copyright (c) 2026, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is designed to work with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms,
 * as designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have either included with
 * the program or referenced in the documentation.
 *
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#include <mysql.h>

#ifdef _WIN32
#include <windows.h>
#include <winver.h>
#endif

#include <string>
#include <utility>
#include <vector>

#include "my_config.h"
#include "mysqlshdk/libs/utils/process_launcher.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_path.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "mysqlshdk/libs/utils/version.h"
#include "unittest/gtest_clean.h"
#include "unittest/test_utils/command_line_test.h"
#include "unittest/test_utils/mocks/gmock_clean.h"

namespace tests {
namespace {

std::string expected_version(bool with_update) {
  return shcore::str_format(
      "Ver %s%s for %s on %s - for MySQL %s (%s)",
      with_update ? MYSH_VERSION_WITH_UPDATE : MYSH_VERSION, EXTRA_NAME_SUFFIX,
      SYSTEM_TYPE, MACHINE_TYPE,
      with_update ? mysql_get_client_info_with_version_update()
                  : mysql_get_client_info(),
      MYSQL_COMPILATION_COMMENT);
}

}  // namespace

class Shell_version_test : public Command_line_test {};

// WL17369-FR1-01: Exact version output using the linked library's own identity.
// WL17369-FR3-01: Both full versions match the configured build.
// WL17369-FR3-02: Shell and linked-library versions have independent sources.
// WL17369-FR3-03: Release and edition suffixes appear after the update.
// WL17369-HLS-01: Repeated options retain available build details.
// WL17369-NF2-01: Legacy CLI options preserve three-part versions.
TEST_F(Shell_version_test, command_line_versions) {
  ASSERT_EQ(0, execute({_mysqlsh, "--no-defaults", "--version", "--version",
                        nullptr}))
      << _output;
  const auto legacy_prefix = _mysqlsh_path + "   " + expected_version(false);
  ASSERT_THAT(_output, testing::StartsWith(legacy_prefix));
  const auto extra_details = _output.substr(legacy_prefix.size());

  if (*MYSH_BUILD_ID) {
    EXPECT_THAT(extra_details, testing::StartsWith(" - build " MYSH_BUILD_ID));
  } else {
    EXPECT_THAT(extra_details,
                testing::AnyOf(testing::Eq("\n"),
                               testing::Eq(" + session_recorder\n")));
  }

  const struct {
    std::vector<const char *> options;
    bool with_update;
    bool extra;
  } cases[] = {
      {{"--version"}, false, false},
      {{"-V"}, false, false},
      {{"--version-with-update"}, true, false},
      {{"--version", "--version"}, false, true},
      {{"-V", "-V"}, false, true},
      {{"--version", "-V"}, false, true},
      {{"--version-with-update", "--version-with-update"}, true, true},
      {{"--version", "--version-with-update"}, true, true},
      {{"--version-with-update", "--version"}, true, true},
      {{"-V", "--version-with-update"}, true, true},
      {{"--version-with-update", "-V"}, true, true},
      {{"--version", "--version-with-update", "--version"}, true, true}};

  for (const auto &entry : cases) {
    std::vector<const char *> args{_mysqlsh, "--no-defaults"};
    args.insert(args.end(), entry.options.begin(), entry.options.end());
    args.push_back(nullptr);
    SCOPED_TRACE(shcore::str_join(entry.options, " "));
    ASSERT_EQ(0, execute(args)) << _output;

    std::string expected =
        _mysqlsh_path + "   " + expected_version(entry.with_update);

    if (entry.extra) {
      EXPECT_EQ(expected + extra_details, _output);
    } else {
      EXPECT_THAT(
          _output,
          testing::AnyOf(testing::Eq(expected + "\n"),
                         testing::Eq(expected + " + session_recorder\n")));
    }
  }
}

// WL17369-FR1-02: Version printing ignores an unavailable connection endpoint.
TEST_F(Shell_version_test, version_without_connection) {
  for (const auto *protocol : {"--mysql", "--mysqlx"}) {
    SCOPED_TRACE(protocol);
    ASSERT_EQ(0, execute({_mysqlsh, "--no-defaults", protocol,
                          "--host=127.0.0.1", "--port=1", "--user=version_test",
                          "--no-password", "--connect-timeout=1000",
                          "--version-with-update", nullptr}))
        << _output;
    const auto expected = _mysqlsh_path + "   " + expected_version(true);
    EXPECT_THAT(_output, testing::AnyOf(
                             testing::Eq(expected + "\n"),
                             testing::Eq(expected + " + session_recorder\n")));
  }
}

// WL17369-FR1-03: An attached argument fails with a diagnostic.
TEST_F(Shell_version_test, version_rejects_argument) {
  EXPECT_NE(0, execute({_mysqlsh, "--no-defaults", "--version-with-update=1",
                        nullptr}));
  MY_EXPECT_CMD_OUTPUT_CONTAINS("--version-with-update");
  MY_EXPECT_CMD_OUTPUT_CONTAINS("does not require an argument");
  MY_EXPECT_CMD_OUTPUT_NOT_CONTAINS(expected_version(true));
}

#if defined(HAVE_JS) || defined(HAVE_PYTHON)
// WL17369-FR2-01: Properties match the Shell and client-library identities.
// WL17369-NF2-01: Legacy properties preserve three-part descriptions.
TEST_F(Shell_version_test, scripting_versions) {
  const struct {
    const char *mode;
    const char *property;
    const char *print;
  } cases[] = {
#ifdef HAVE_JS
      {"--js", "versionWithUpdate", "println"},
#endif
#ifdef HAVE_PYTHON
      {"--py", "version_with_update", "print"},
#endif
  };

  for (const auto &entry : cases) {
    SCOPED_TRACE(entry.mode);

    for (const bool with_update : {false, true}) {
      const auto expression = std::string{entry.print} + "(shell." +
                              (with_update ? entry.property : "version") + ")";
      ASSERT_EQ(0, execute({_mysqlsh, "--no-defaults", entry.mode, "-e",
                            expression.c_str(), nullptr}))
          << _output;
      EXPECT_EQ(expected_version(with_update) + "\n", _output);
    }
  }
}
#endif  // HAVE_JS || HAVE_PYTHON

// WL17369-NF2-01: The interactive banner retains the legacy Shell version.
TEST_F(Shell_version_test, legacy_banner) {
  ASSERT_EQ(0, execute({_mysqlsh, "--no-defaults", "--sql", "--interactive",
                        "-e", "\\quit", nullptr}))
      << _output;
  MY_EXPECT_CMD_OUTPUT_CONTAINS("MySQL Shell " MYSH_VERSION EXTRA_NAME_SUFFIX
                                "\n");
}

// WL17369-NF1-01: Missing, empty, and explicit zero metadata default to zero.
// WL17369-NF1-02: Nonzero updates survive the actual CMake configuration logic.
// WL17369-FR3-02: Metadata uses independent Shell and MySQL update values.
// WL17369-FR3-03: Configured release suffixes follow their own update values.
TEST(Shell_version_metadata, update_components) {
  const auto cmake = shcore::path::search_stdpath("cmake");
  const auto version_cmake =
      shcore::path::join_path(MYSQLX_SOURCE_HOME, "version.cmake");

  if (cmake.empty() || !shcore::is_file(version_cmake)) {
    SKIP_TEST("Metadata fixtures require CMake and the Shell source tree.");
  }

  const auto directory = shcore::create_temporary_folder();
  const shcore::Scoped_callback cleanup{
      [&directory]() { shcore::remove_directory(directory); }};
  shcore::create_directory(shcore::path::join_path(directory, "mysql"));
  shcore::create_directory(shcore::path::join_path(directory, "build"));
  const auto script = shcore::path::join_path(directory, "check.cmake");
  ASSERT_TRUE(shcore::create_file(script, R"cmake(
set(ROOT_PROJECT_DIR "${CMAKE_CURRENT_LIST_DIR}")
set(MYSQL_SOURCE_DIR "${ROOT_PROJECT_DIR}/mysql")
set(CMAKE_BINARY_DIR "${ROOT_PROJECT_DIR}/build")
include("${VERSION_CMAKE}")
file(WRITE "${ROOT_PROJECT_DIR}/actual.txt"
  "${MYSH_VERSION}\n${MYSH_VERSION_WITH_UPDATE}\n"
  "${MYSQL_VERSION}\n${MYSQL_VERSION_WITH_UPDATE}\n"
  "${MYSH_NO_DASH_VERSION}\n${MYSH_VERSION_WIN}\n")
)cmake"));

  const struct {
    const char *shell_update;
    const char *mysql_update;
    int expected_shell_update;
    int expected_mysql_update;
  } cases[] = {{nullptr, nullptr, 0, 0}, {"", "", 0, 0},   {"0", "0", 0, 0},
               {"2", "0", 2, 0},         {"0", "1", 0, 1}, {"2", "1", 2, 1},
               {"12", "34", 12, 34}};
  const auto write_metadata = [](const std::string &path, int major, int minor,
                                 int patch, const char *update,
                                 const char *suffix) {
    auto contents = shcore::str_format(
        "MYSQL_VERSION_MAJOR=%d\nMYSQL_VERSION_MINOR=%d\nMYSQL_VERSION_PATCH=%"
        "d\n"
        "MYSQL_VERSION_EXTRA=%s\n",
        major, minor, patch, suffix);

    if (update) {
      contents += std::string{"MYSQL_VERSION_UPDATE="} + update + "\n";
    }

    return shcore::create_file(path, contents);
  };

  for (const auto &entry : cases) {
    for (const bool suffixes : {false, true}) {
      const auto *shell_suffix = suffixes ? "-rc" : "";
      const auto *mysql_suffix = suffixes ? "-debug" : "";
      SCOPED_TRACE(shcore::str_format(
          "Shell update=%s, MySQL update=%s, suffixes=%d",
          entry.shell_update ? entry.shell_update : "<missing>",
          entry.mysql_update ? entry.mysql_update : "<missing>", suffixes));
      ASSERT_TRUE(
          write_metadata(shcore::path::join_path(directory, "MYSQL_VERSION"), 1,
                         2, 3, entry.shell_update, shell_suffix));
      ASSERT_TRUE(write_metadata(
          shcore::path::join_path(directory, "mysql", "MYSQL_VERSION"), 4, 5, 6,
          entry.mysql_update, mysql_suffix));
      const auto source_arg = "-DVERSION_CMAKE=" + version_cmake;
      const char *args[] = {cmake.c_str(), source_arg.c_str(), "-P",
                            script.c_str(), nullptr};
      shcore::Process_launcher process{args};
      process.start();
      const auto output = process.read_all();
      ASSERT_EQ(0, process.wait()) << output;
      auto expected = shcore::str_format(
          "1.2.3%s\n1.2.3.%d%s\n4.5.6%s\n4.5.6.%d%s\n1.2.3\n", shell_suffix,
          entry.expected_shell_update, shell_suffix, mysql_suffix,
          entry.expected_mysql_update, mysql_suffix);
#ifdef _WIN32
      expected += "1.2.3." + std::to_string(entry.expected_shell_update);
#endif
      expected += "\n";
      EXPECT_EQ(expected,
                shcore::get_text_file(
                    shcore::path::join_path(directory, "actual.txt"), false));
    }
  }
}

#ifdef _WIN32
// WL17369-FR4-01: The compiled binary contains the configured numeric update.
// WL17369-FR4-02: File and product version strings match their numeric fields.
TEST_F(Shell_version_test, windows_version_resource) {
  const auto library = LoadLibraryW(L"version.dll");
  ASSERT_NE(nullptr, library) << GetLastError();
  const shcore::Scoped_callback cleanup{[library]() { FreeLibrary(library); }};
  const auto get_size = reinterpret_cast<decltype(&GetFileVersionInfoSizeW)>(
      GetProcAddress(library, "GetFileVersionInfoSizeW"));
  const auto get_info = reinterpret_cast<decltype(&GetFileVersionInfoW)>(
      GetProcAddress(library, "GetFileVersionInfoW"));
  const auto query = reinterpret_cast<decltype(&VerQueryValueW)>(
      GetProcAddress(library, "VerQueryValueW"));
  ASSERT_NE(nullptr, get_size);
  ASSERT_NE(nullptr, get_info);
  ASSERT_NE(nullptr, query);

  const auto path = shcore::utf8_to_wide(_mysqlsh_path);
  const auto size = get_size(path.c_str(), nullptr);
  ASSERT_NE(0U, size) << GetLastError();
  std::vector<char> resource(size);
  ASSERT_TRUE(get_info(path.c_str(), 0, size, resource.data()))
      << GetLastError();
  void *data = nullptr;
  UINT length = 0;
  ASSERT_TRUE(query(resource.data(), L"\\", &data, &length));
  ASSERT_GE(length, sizeof(VS_FIXEDFILEINFO));
  const auto *info = static_cast<const VS_FIXEDFILEINFO *>(data);
  const mysqlshdk::utils::Version expected{MYSH_VERSION_WITH_UPDATE};

  for (const auto &[high, low] :
       {std::pair{info->dwFileVersionMS, info->dwFileVersionLS},
        std::pair{info->dwProductVersionMS, info->dwProductVersionLS}}) {
    EXPECT_EQ(expected.get_major(), HIWORD(high));
    EXPECT_EQ(expected.get_minor(), LOWORD(high));
    EXPECT_EQ(expected.get_patch(), HIWORD(low));
    EXPECT_EQ(expected.get_update(), LOWORD(low));
  }

  struct Translation {
    WORD language;
    WORD code_page;
  };
  ASSERT_TRUE(
      query(resource.data(), L"\\VarFileInfo\\Translation", &data, &length));
  ASSERT_GE(length, sizeof(Translation));
  const auto translation = *static_cast<const Translation *>(data);

  for (const auto *name : {"FileVersion", "ProductVersion"}) {
    const auto key = shcore::utf8_to_wide(
        shcore::str_format("\\StringFileInfo\\%04x%04x\\%s",
                           translation.language, translation.code_page, name));
    ASSERT_TRUE(query(resource.data(), key.c_str(), &data, &length));
    ASSERT_GT(length, 0U);
    EXPECT_EQ(expected.get_base_with_update(),
              shcore::wide_to_utf8(static_cast<const wchar_t *>(data)));
  }
}
#endif

}  // namespace tests
