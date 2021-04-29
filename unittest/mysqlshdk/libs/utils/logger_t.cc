/* Copyright (c) 2015, 2021, Oracle and/or its affiliates.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License, version 2.0,
   as published by the Free Software Foundation.

   This program is also distributed with certain software (including
   but not limited to OpenSSL) that is licensed under separate terms, as
   designated in a particular file or component or in included license
   documentation.  The authors of MySQL hereby grant you an additional
   permission to link the program and your derivative works with the
   separately licensed software that they have included with MySQL.
   This program is distributed in the hope that it will be useful,  but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
   the GNU General Public License, version 2.0, for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA */

#include <algorithm>
#include <deque>
#include <fstream>
#include <memory>
#include <string>

#include "unittest/gtest_clean.h"
#include "unittest/test_utils/mocks/gmock_clean.h"

#include "mysqlshdk/include/shellcore/scoped_contexts.h"
#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_path.h"

#ifndef _WIN32
#include <fcntl.h>
#endif  // !_WIN32

namespace shcore {

namespace {

bool is_timestamp(const char *text) {
  // example timestamp: "2015-12-23 09:26:49"
  using std::isdigit;
  return isdigit(text[0]) && isdigit(text[1]) && isdigit(text[2]) &&
         isdigit(text[3]) && '-' == text[4] &&                      // 2015-
         isdigit(text[5]) && isdigit(text[6]) && '-' == text[7] &&  // 12-
         isdigit(text[8]) && isdigit(text[9]) &&
         ' ' == text[10] &&  // 23<space>

         isdigit(text[11]) && isdigit(text[12]) && ':' == text[13] &&  // 09:
         isdigit(text[14]) && isdigit(text[15]) && ':' == text[16] &&  // 26:
         isdigit(text[17]) && isdigit(text[18]);                       // 49
}

}  // namespace

class Logger_test : public ::testing::Test {
 protected:
  static void log_hook(const Logger::Log_entry &, void *) { ++s_hook_executed; }
  static void log_all_hook(const Logger::Log_entry &, void *) {
    ++s_all_hook_executed;
  }

  static int hook_executed() { return s_hook_executed; }
  static int all_hook_executed() { return s_all_hook_executed; }

  static std::string get_log_file(const char *filename) {
    static const auto k_test_dir = getenv("TMPDIR");
    return shcore::path::join_path(k_test_dir, filename);
  }

  static bool get_log_file_contents(const char *filename,
                                    std::string *contents) {
    return shcore::load_text_file(get_log_file(filename), *contents);
  }

  void SetUp() override {
    m_previous_stderr_format = Logger::stderr_output_format();
  }

  void TearDown() override {
    Logger::set_stderr_output_format(m_previous_stderr_format);

    s_hook_executed = 0;
    s_all_hook_executed = 0;
  }

 private:
  static int s_hook_executed;
  static int s_all_hook_executed;

  std::string m_previous_stderr_format;
};

int Logger_test::s_hook_executed = 0;
int Logger_test::s_all_hook_executed = 0;

TEST_F(Logger_test, intialization) {
  const auto name = get_log_file("mylog.txt");
  shcore::on_leave_scope scope_leave([&name]() {
    if (!shcore::is_folder(name)) {
      shcore::delete_file(name);
    }
  });

  mysqlsh::Scoped_logger logger(
      Logger::create_instance(name.c_str(), false, Logger::LOG_DEBUG));

  const auto l = current_logger();

  EXPECT_EQ(name, l->logfile_name());
  EXPECT_EQ(Logger::LOG_DEBUG, l->get_log_level());

  l->set_log_level(Logger::LOG_WARNING);
  EXPECT_EQ(Logger::LOG_WARNING, l->get_log_level());
}

TEST_F(Logger_test, log_levels_and_hooks) {
  const auto name = get_log_file("mylog.txt");
  shcore::on_leave_scope scope_leave([&name]() {
    if (!shcore::is_folder(name)) {
      shcore::delete_file(name);
    }
  });

  mysqlsh::Scoped_logger logger(
      Logger::create_instance(name.c_str(), false, Logger::LOG_WARNING));

  const auto l = current_logger();
  Log_context ctx("Unit Test Domain");

  l->attach_log_hook(log_all_hook, nullptr, true);

  l->attach_log_hook(log_hook);
  l->log(Logger::LOG_ERROR, "Error due to %s", "critical");
  l->detach_log_hook(log_hook);
  // Debug message will not appear in the log
  l->log(Logger::LOG_DEBUG, "Memory deallocated");
  l->log(Logger::LOG_WARNING, "Warning the file already exists");

  l->detach_log_hook(log_all_hook);

  EXPECT_EQ(1, hook_executed());
  EXPECT_EQ(3, all_hook_executed());

  std::string contents;
  EXPECT_TRUE(get_log_file_contents("mylog.txt", &contents));

  EXPECT_THAT(
      contents,
      ::testing::HasSubstr("Error: Unit Test Domain: Error due to critical\n"));
  EXPECT_THAT(
      contents,
      ::testing::HasSubstr(
          "Warning: Unit Test Domain: Warning the file already exists"));
  EXPECT_THAT(contents,
              ::testing::Not(::testing::HasSubstr("Memory deallocated")));
}

TEST_F(Logger_test, parse_log_level) {
  EXPECT_THROW(Logger::parse_log_level("unknown"), std::invalid_argument);
  EXPECT_EQ(Logger::LOG_NONE, Logger::parse_log_level("NONE"));
  EXPECT_EQ(Logger::LOG_INTERNAL_ERROR, Logger::parse_log_level("internal"));
  EXPECT_EQ(Logger::LOG_ERROR, Logger::parse_log_level("error"));
  EXPECT_EQ(Logger::LOG_WARNING, Logger::parse_log_level("WArning"));
  EXPECT_EQ(Logger::LOG_INFO, Logger::parse_log_level("infO"));
  EXPECT_EQ(Logger::LOG_DEBUG, Logger::parse_log_level("DEBUG"));
  EXPECT_EQ(Logger::LOG_DEBUG2, Logger::parse_log_level("DEBUG2"));
  EXPECT_EQ(Logger::LOG_DEBUG3, Logger::parse_log_level("DEBUG3"));
  EXPECT_THROW(Logger::parse_log_level("0"), std::invalid_argument);
  EXPECT_EQ(Logger::LOG_NONE, Logger::parse_log_level("1"));
  EXPECT_EQ(Logger::LOG_INTERNAL_ERROR, Logger::parse_log_level("2"));
  EXPECT_EQ(Logger::LOG_ERROR, Logger::parse_log_level("3"));
  EXPECT_EQ(Logger::LOG_WARNING, Logger::parse_log_level("4"));
  EXPECT_EQ(Logger::LOG_INFO, Logger::parse_log_level("5"));
  EXPECT_EQ(Logger::LOG_DEBUG, Logger::parse_log_level("6"));
  EXPECT_EQ(Logger::LOG_DEBUG2, Logger::parse_log_level("7"));
  EXPECT_EQ(Logger::LOG_DEBUG3, Logger::parse_log_level("8"));
  EXPECT_THROW(Logger::parse_log_level("9"), std::invalid_argument);
}

TEST_F(Logger_test, log_open_failure) {
  std::string exctext;

#ifdef _WIN32
  exctext =
      "Error opening log file '%s' for writing: No such file or directory";
#else
  exctext = "Error opening log file '%s' for writing: Is a directory";
#endif

  EXPECT_THROW(
      {
        const std::string filename = get_log_file("");
        try {
          Logger::create_instance(filename.c_str(), false, Logger::LOG_WARNING);
        } catch (const std::runtime_error &e) {
          EXPECT_EQ(shcore::str_format(exctext.c_str(), filename.c_str()),
                    e.what());
          throw;
        }
      },
      std::runtime_error);
}

TEST_F(Logger_test, log_format) {
  const auto name = get_log_file("mylog.txt");
  shcore::on_leave_scope scope_leave([&name]() {
    if (!shcore::is_folder(name)) {
      shcore::delete_file(name);
    }
  });

  mysqlsh::Scoped_logger logger(Logger::create_instance(
      get_log_file("mylog.txt").c_str(), false, Logger::LOG_DEBUG));

  const auto l = current_logger();

  std::deque<std::pair<Logger::LOG_LEVEL, std::string>> tests = {
      {Logger::LOG_DEBUG, "Debug"},
      {Logger::LOG_INFO, "Info"},
      {Logger::LOG_WARNING, "Warning"},
      {Logger::LOG_ERROR, "Error"}};

  Log_context ctx("Unit Test Domain");

  for (const auto &p : tests) {
    l->log(p.first, "Some message");
  }

  std::string contents;
  EXPECT_TRUE(get_log_file_contents("mylog.txt", &contents));

  for (const auto &line : shcore::str_split(contents, "\n")) {
    if (line.empty()) {
      continue;
    }

    ASSERT_FALSE(tests.empty());

    EXPECT_EQ(": " + tests.front().second + ": Unit Test Domain: Some message",
              line.substr(19));
    EXPECT_TRUE(is_timestamp(line.c_str()));

    tests.pop_front();
  }

  EXPECT_TRUE(tests.empty());
}

#ifndef _WIN32
// on Windows Logger is using OutputDebugString() instead of stderr

namespace {

class Stderr_reader final {
 public:
  Stderr_reader() {
    m_pipe[0] = 0;
    m_pipe[1] = 0;

    if (pipe(m_pipe) == -1) {
      throw std::runtime_error{"Failed to create pipe"};
    }

    // copy stderr
    m_stderr = dup(fileno(stderr));
    // set read end to non-blocking
    fcntl(m_pipe[0], F_SETFL, O_NONBLOCK | fcntl(m_pipe[0], F_GETFL, 0));
    // replace stderr with write end
    dup2(m_pipe[1], fileno(stderr));
  }

  ~Stderr_reader() {
    // restore stderr
    dup2(m_stderr, fileno(stderr));

    // close file descriptors
    if (m_stderr > 0) {
      close(m_stderr);
    }

    if (m_pipe[0] > 0) {
      close(m_pipe[0]);
    }

    if (m_pipe[1] > 0) {
      close(m_pipe[1]);
    }
  }

  Stderr_reader(const Stderr_reader &) = delete;
  Stderr_reader(Stderr_reader &&) = delete;

  Stderr_reader &operator=(const Stderr_reader &) = delete;
  Stderr_reader &operator=(Stderr_reader &&) = delete;

  std::string output() const {
    static constexpr size_t k_size = 512;
    char buffer[k_size];
    std::string result;
    ssize_t bytes = 0;

    while ((bytes = ::read(m_pipe[0], buffer, k_size - 1)) > 0) {
      buffer[bytes] = '\0';
      result += buffer;
    }

    return result;
  }

 private:
  int m_pipe[2];
  int m_stderr;
};

}  // namespace

TEST_F(Logger_test, stderr_output) {
  Stderr_reader reader;

  Logger::set_stderr_output_format("table");
  mysqlsh::Scoped_logger logger(
      Logger::create_instance(get_log_file("mylog.txt").c_str(), true,
                              Logger::LOG_WARNING),
      [](const std::shared_ptr<Logger> &l) {
        if (!shcore::is_folder(l->logfile_name())) {
          shcore::delete_file(l->logfile_name());
        }
      });

  const auto l = current_logger();
  Log_context ctx("Unit Test Domain");

  l->log(Logger::LOG_DEBUG, "First");
  l->log(Logger::LOG_INFO, "Second");
  l->log(Logger::LOG_WARNING, "Third");
  l->log(Logger::LOG_ERROR, "Fourth");

  std::string contents;
  EXPECT_TRUE(get_log_file_contents("mylog.txt", &contents));

  std::string error = reader.output();
  EXPECT_EQ(contents, error);

  EXPECT_THAT(error, ::testing::Not(::testing::HasSubstr("First")));
  EXPECT_THAT(error, ::testing::Not(::testing::HasSubstr("Second")));
  EXPECT_THAT(error, ::testing::HasSubstr("Third"));
  EXPECT_THAT(error, ::testing::HasSubstr("Fourth"));
}

TEST_F(Logger_test, stderr_json_output) {
  Stderr_reader reader;

  Logger::set_stderr_output_format("json");

  mysqlsh::Scoped_logger logger(
      Logger::create_instance(get_log_file("mylog.txt").c_str(), true,
                              Logger::LOG_WARNING),
      [](const std::shared_ptr<Logger> &l) {
        if (!shcore::is_folder(l->logfile_name())) {
          shcore::delete_file(l->logfile_name());
        }
      });

  const auto l = current_logger();

  Log_context ctx("Unit Test Domain");

  l->log(Logger::LOG_DEBUG, "First");
  l->log(Logger::LOG_INFO, "Second");
  l->log(Logger::LOG_WARNING, "Third");
  l->log(Logger::LOG_ERROR, "Fourth");

  std::string contents;
  EXPECT_TRUE(get_log_file_contents("mylog.txt", &contents));
  const auto log_lines = shcore::str_split(contents, "\n");

  EXPECT_EQ(": Warning: Unit Test Domain: Third", log_lines[0].substr(19));
  EXPECT_TRUE(is_timestamp(log_lines[0].c_str()));

  EXPECT_EQ(": Error: Unit Test Domain: Fourth", log_lines[1].substr(19));
  EXPECT_TRUE(is_timestamp(log_lines[1].c_str()));

  const auto lines = shcore::str_split(reader.output(), "\n");

  EXPECT_EQ("{", lines[0]);
  EXPECT_EQ("    \"timestamp\": \"", lines[1].substr(0, 18));
  auto timestamp = lines[1].substr(18, 10);
  EXPECT_TRUE(std::all_of(timestamp.cbegin(), timestamp.cend(), ::isdigit));
  EXPECT_EQ("\",", lines[1].substr(28));
  EXPECT_EQ("    \"level\": \"Warning\",", lines[2]);
  EXPECT_EQ("    \"domain\": \"Unit Test Domain\",", lines[3]);
  EXPECT_EQ("    \"message\": \"Third\"", lines[4]);
  EXPECT_EQ("}", lines[5]);
  EXPECT_EQ("{", lines[6]);
  EXPECT_EQ("    \"timestamp\": \"", lines[7].substr(0, 18));
  timestamp = lines[7].substr(18, 10);
  EXPECT_TRUE(std::all_of(timestamp.cbegin(), timestamp.cend(), ::isdigit));
  EXPECT_EQ("\",", lines[7].substr(28));
  EXPECT_EQ("    \"level\": \"Error\",", lines[8]);
  EXPECT_EQ("    \"domain\": \"Unit Test Domain\",", lines[9]);
  EXPECT_EQ("    \"message\": \"Fourth\"", lines[10]);
  EXPECT_EQ("}", lines[11]);
}

TEST_F(Logger_test, stderr_json_raw_output) {
  Stderr_reader reader;

  Logger::set_stderr_output_format("json/raw");
  mysqlsh::Scoped_logger logger(
      Logger::create_instance(get_log_file("mylog.txt").c_str(), true,
                              Logger::LOG_WARNING),
      [](const std::shared_ptr<Logger> &l) {
        if (!shcore::is_folder(l->logfile_name())) {
          shcore::delete_file(l->logfile_name());
        }
      });

  const auto l = current_logger();

  Log_context ctx("Unit Test Domain");

  l->log(Logger::LOG_DEBUG, "First");
  l->log(Logger::LOG_INFO, "Second");
  l->log(Logger::LOG_WARNING, "Third");
  l->log(Logger::LOG_ERROR, "Fourth");

  std::string contents;
  EXPECT_TRUE(get_log_file_contents("mylog.txt", &contents));
  const auto log_lines = shcore::str_split(contents, "\n");

  EXPECT_EQ(": Warning: Unit Test Domain: Third", log_lines[0].substr(19));
  EXPECT_TRUE(is_timestamp(log_lines[0].c_str()));

  EXPECT_EQ(": Error: Unit Test Domain: Fourth", log_lines[1].substr(19));
  EXPECT_TRUE(is_timestamp(log_lines[1].c_str()));

  const auto lines = shcore::str_split(reader.output(), "\n");

  EXPECT_EQ("{\"timestamp\":\"", lines[0].substr(0, 14));
  auto timestamp = lines[0].substr(14, 10);
  EXPECT_TRUE(std::all_of(timestamp.cbegin(), timestamp.cend(), ::isdigit));
  EXPECT_EQ(
      "\",\"level\":\"Warning\",\"domain\":\"Unit Test "
      "Domain\",\"message\":\"Third\"}",
      lines[0].substr(24));

  EXPECT_EQ("{\"timestamp\":\"", lines[1].substr(0, 14));
  timestamp = lines[1].substr(14, 10);
  EXPECT_TRUE(std::all_of(timestamp.cbegin(), timestamp.cend(), ::isdigit));
  EXPECT_EQ(
      "\",\"level\":\"Error\",\"domain\":\"Unit Test "
      "Domain\",\"message\":\"Fourth\"}",
      lines[1].substr(24));
}

#endif  // !_WIN32

}  // namespace shcore
