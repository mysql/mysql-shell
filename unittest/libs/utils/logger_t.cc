// TODO(alfredo) this needs to be updated to be integrated into the rest of the tests

/* Copyright (c) 2015, 2017, Oracle and/or its affiliates. All rights reserved.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA */


#include <cctype>
#include <fstream>
#include <string>

#include "mysqlshdk/libs/utils/logger.h"
#include "gtest_clean.h"


using namespace ngcommon;


namespace ngcommon
{
namespace tests
{

  class LoggerTestProxy  // this class is a friend of Logger, thus allows testing of its private methods
  {
  public:
    static std::string format_message(const char* domain, const char* message, Logger::LOG_LEVEL log_level)
    {
      return Logger::format_message(domain, message, log_level);
    }
    static std::string format_message(const char* domain, const char* message, const std::exception& exc)
    {
      return Logger::format_message(domain, message, exc);
    }
  };

  const std::string *get_path(const char *filename)
  {
    std::string *path = new std::string(PROCESS_LAUNCHER_TESTS_DIR);
    *path += "/";
    *path += filename;
    return path;
  }

  const std::string *get_file_contents(const char *filename)
  {
    std::string path = std::string(PROCESS_LAUNCHER_TESTS_DIR);
    path += "/";
    path += filename;
    std::ifstream iff(path.c_str(), std::ios::in);

    if (iff)
    {
      std::string *s = new std::string();
      iff.seekg(0, iff.end);
      s->resize(static_cast<unsigned int>(iff.tellg()));
      iff.seekg(0, std::ios::beg);
      iff.read(&(*s)[0], s->size());
      iff.close();
      return s;
    }
    else
    {
      return NULL;
    }
  }

  int hook_executed = 0;

  void myhook(const char* message, Logger::LOG_LEVEL level, const char* domain)
  {
    hook_executed++;
  }

  TEST(Logger, simple)
	{
    const std::string* filename = get_path("mylog.txt");
    Logger::create_instance(filename->c_str(), true, Logger::LOG_WARNING);

    Logger *l= Logger::singleton();

    l->attach_log_hook(myhook);
    l->log(Logger::LOG_ERROR, "Unit Test Domain", "Error due to %s", "critical");
    l->detach_log_hook(myhook);
    // Debug message will not appear in the log
    l->log(Logger::LOG_DEBUG, "Unit Test Domain", "Memory deallocated" );
    l->log(Logger::LOG_WARNING, "Unit Test Domain", "Warning the file already exists");

    const std::string* contents = get_file_contents("mylog.txt");

    size_t idx = contents->find("Error: Unit Test Domain: Error due to critical\n");
    size_t idx2 = contents->find("Warning: Unit Test Domain: Warning the file already exists");
    size_t idx3 = contents->find("Memory deallocated");

    EXPECT_NE(idx , std::string::npos);
    EXPECT_NE(idx2, std::string::npos);
    EXPECT_EQ(idx3, std::string::npos);

    EXPECT_EQ(1, hook_executed);

    Logger::LOG_LEVEL ll = Logger::get_level_by_name("DEBUG");
    EXPECT_EQ((int)Logger::LOG_DEBUG, (int)ll);

    ll = Logger::get_level_by_name("WArning");
    EXPECT_EQ((int)Logger::LOG_WARNING, (int)ll);

    ll = Logger::get_level_by_name("error");
    EXPECT_EQ((int)Logger::LOG_ERROR, (int)ll);

    delete filename;
    delete contents;
	}

  TEST(Logger, log_open_failure)
  {
    try
    {
      const std::string* filename = get_path("");
      Logger::create_instance(filename->c_str(), true, Logger::LOG_WARNING);
      FAIL() << "Expected std::logic_error";
    }
    catch(const std::logic_error& e)
    {
    #ifdef _WIN32
      EXPECT_EQ(std::string("Error in Logger::Logger when opening file '")+PROCESS_LAUNCHER_TESTS_DIR+"/' for writing", e.what());
    #else
      EXPECT_EQ(std::string("Error in Logger::Logger when opening file '/tmp/' for writing"), e.what());
    #endif
    }
    catch(...) {
      FAIL() << "Expected std::logic_error";
    }
  }

  bool is_timestamp(const char* text)
  {
    // example timestamp: "2015-12-23 09:26:49"
    using std::isdigit;
    return
      isdigit(text[0]) && isdigit(text[1]) && isdigit(text[2]) && isdigit(text[3]) && '-' == text[4] && // 2015-
      isdigit(text[5]) && isdigit(text[6]) && '-' == text[ 7]  && // 12-
      isdigit(text[8]) && isdigit(text[9]) && ' ' == text[10]  && // 23<space>

      isdigit(text[11]) && isdigit(text[12]) && ':' == text[13] && // 09:
      isdigit(text[14]) && isdigit(text[15]) && ':' == text[16] && // 26:
      isdigit(text[17]) && isdigit(text[18]);                      // 49
  }

  TEST(Logger, format_message)
  {
    // I wish we could do these tests in a more civilised way (use <regex>),
    // but unfortunately it's only fully implemented in GCC 4.9 and later

    // example expected message: "2015-12-23 09:26:49: Debug: Unit Test Domain: Some message"
    {
      std::string s = LoggerTestProxy::format_message("Unit Test Domain", "Some message", Logger::LOG_DEBUG);
      EXPECT_STREQ(": Debug: Unit Test Domain: Some message", s.c_str()+19);  // +19 to skip the timestamp
      EXPECT_TRUE(is_timestamp(s.c_str()));
    }

    // example expected message: "2015-12-23 09:26:49: Info: Unit Test Domain: Some message"
    {
      std::string s = LoggerTestProxy::format_message("Unit Test Domain", "Some message", Logger::LOG_INFO);
      EXPECT_STREQ(": Info: Unit Test Domain: Some message", s.c_str()+19);  // +19 to skip the timestamp
      EXPECT_TRUE(is_timestamp(s.c_str()));
    }

    // example expected message: "2015-12-23 09:26:49: Warning: Unit Test Domain: Some message"
    {
      std::string s = LoggerTestProxy::format_message("Unit Test Domain", "Some message", Logger::LOG_WARNING);
      EXPECT_STREQ(": Warning: Unit Test Domain: Some message", s.c_str()+19);  // +19 to skip the timestamp
      EXPECT_TRUE(is_timestamp(s.c_str()));
    }

    // example expected message: "2015-12-23 09:26:49: Error: Unit Test Domain: Some message"
    {
      std::string s = LoggerTestProxy::format_message("Unit Test Domain", "Some message", Logger::LOG_ERROR);
      EXPECT_STREQ(": Error: Unit Test Domain: Some message", s.c_str()+19);  // +19 to skip the timestamp
      EXPECT_TRUE(is_timestamp(s.c_str()));
    }

    // example expected message: "2015-12-23 09:26:49: Error: Unit Test Domain: Some exception"
    {
      std::runtime_error e("Some exception");
      std::string s = LoggerTestProxy::format_message("Unit Test Domain", "Some message that will get ignored", e);
      EXPECT_STREQ(": Error: Unit Test Domain: Some exception", s.c_str()+19);  // +19 to skip the timestamp
      EXPECT_TRUE(is_timestamp(s.c_str()));
    }
  }

} // namespace tests
} // namespace ngcommon
