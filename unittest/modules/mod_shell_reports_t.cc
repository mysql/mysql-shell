/*
 * Copyright (c) 2015, 2022, Oracle and/or its affiliates.
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

#include <memory>
#include <string>

#include "modules/mod_shell_reports.h"
#include "mysqlshdk/libs/utils/utils_general.h"

#include "unittest/gtest_clean.h"
#include "unittest/test_utils/mocks/gmock_clean.h"

namespace tests {

using mysqlsh::Report;
using mysqlsh::SessionType;
using mysqlsh::Shell_reports;
using mysqlsh::ShellBaseSession;
using mysqlshdk::db::Connection_options;
using mysqlshdk::db::ISession;
using shcore::Object_bridge_ref;
using shcore::Param_flag;
using shcore::Value;
using shcore::Value_type;

namespace {

static constexpr auto k_report_key = "report";
static constexpr auto k_test_report = "report";
static constexpr auto k_expected_value = "--OK--";

#define ASSERT_NOT_NULL(x) [&]() -> void { ASSERT_NE(nullptr, x); }()

class Mock_shell_base_session : public ShellBaseSession {
 public:
  MOCK_CONST_METHOD0(class_name, std::string());
  MOCK_METHOD1(connect, void(const Connection_options &));
  MOCK_METHOD0(close, void());
  MOCK_CONST_METHOD0(is_open, bool());
  MOCK_METHOD0(get_status, Value::Map_type_ref());
  MOCK_METHOD1(create_schema, void(const std::string &));
  MOCK_METHOD1(drop_schema, void(const std::string &));
  MOCK_METHOD1(set_current_schema, void(const std::string &));
  MOCK_METHOD2(execute_sql, std::shared_ptr<mysqlshdk::db::IResult>(
                                const std::string &, const shcore::Array_t &));
  MOCK_CONST_METHOD0(session_type, SessionType());
  MOCK_CONST_METHOD0(get_ssl_cipher, std::string());
  MOCK_METHOD3(db_object_exists, std::string(std::string &, const std::string &,
                                             const std::string &));
  MOCK_METHOD2(query_one_string, std::string(const std::string &, int));
  MOCK_METHOD0(get_current_schema, std::string());
  MOCK_METHOD0(start_transaction, void());
  MOCK_METHOD0(commit, void());
  MOCK_METHOD0(rollback, void());
  MOCK_METHOD0(kill_query, void());
  MOCK_CONST_METHOD0(get_core_session, std::shared_ptr<ISession>());
};

shcore::Dictionary_t make_report() {
  auto report = shcore::make_array();
  report->emplace_back(k_expected_value);

  const auto json_result = shcore::make_dict();
  json_result->emplace(k_report_key, std::move(report));

  return json_result;
}

}  // namespace

class Mod_shell_reports_test : public ::testing::Test {
 protected:
  void SetUp() override {
    const auto test_info =
        ::testing::UnitTest::GetInstance()->current_test_info();
    m_reports = std::make_unique<Shell_reports>(
        test_info->name(),
        test_info->test_case_name() + std::string{"."} + test_info->name());
    m_session = std::make_shared<Mock_shell_base_session>();

    EXPECT_CALL(*m_session, is_open())
        .Times(::testing::AtLeast(1))
        .WillRepeatedly(::testing::Return(true));
    EXPECT_CALL(*m_session, class_name())
        .Times(::testing::AtLeast(1))
        .WillRepeatedly(::testing::Return("Session"));
  }

  void TearDown() override {
    m_reports.reset();
    m_session.reset();
  }

  std::unique_ptr<Shell_reports> m_reports;
  std::shared_ptr<Mock_shell_base_session> m_session;
};

TEST_F(Mod_shell_reports_test, report_no_options_no_args) {
  auto report = std::make_unique<Report>(
      k_test_report, Report::Type::REPORT,
      [](const std::shared_ptr<ShellBaseSession> &, const shcore::Array_t &argv,
         const shcore::Dictionary_t &options) {
        SCOPED_TRACE("Running report");

        ASSERT_NOT_NULL(argv);
        EXPECT_EQ(0, argv->size());

        ASSERT_NOT_NULL(options);
        EXPECT_EQ(0, options->size());

        return make_report();
      });

  m_reports->register_report(std::move(report));

  EXPECT_THAT(m_reports->call_report(k_test_report, m_session, {}),
              ::testing::HasSubstr(k_expected_value));
}

TEST_F(Mod_shell_reports_test, report_no_options_args) {
  auto report = std::make_unique<Report>(
      k_test_report, Report::Type::REPORT,
      [](const std::shared_ptr<ShellBaseSession> &, const shcore::Array_t &argv,
         const shcore::Dictionary_t &options) {
        SCOPED_TRACE("Running report");

        ASSERT_NOT_NULL(argv);
        EXPECT_EQ(1, argv->size());
        EXPECT_EQ("argv0", (*argv)[0].as_string());

        ASSERT_NOT_NULL(options);
        EXPECT_EQ(0, options->size());

        return make_report();
      });
  report->set_argc(std::make_pair(1, 1));

  m_reports->register_report(std::move(report));

  EXPECT_THAT(m_reports->call_report(k_test_report, m_session, {"argv0"}),
              ::testing::HasSubstr(k_expected_value));
}

TEST_F(Mod_shell_reports_test, report_options_no_args) {
  auto report = std::make_unique<Report>(
      k_test_report, Report::Type::REPORT,
      [](const std::shared_ptr<ShellBaseSession> &, const shcore::Array_t &argv,
         const shcore::Dictionary_t &options) {
        SCOPED_TRACE("Running report");

        ASSERT_NOT_NULL(argv);
        EXPECT_EQ(0, argv->size());

        ASSERT_NOT_NULL(options);
        std::string sort;
        shcore::Option_unpacker{options}.required("sort", &sort).end();
        EXPECT_EQ("ASC", sort);

        return make_report();
      });
  report->set_options(
      {std::make_shared<Report::Option>("sort", Value_type::String, true)});

  m_reports->register_report(std::move(report));

  EXPECT_THAT(
      m_reports->call_report(k_test_report, m_session, {"--sort", "ASC"}),
      ::testing::HasSubstr(k_expected_value));
}

TEST_F(Mod_shell_reports_test, report_options_args) {
  auto report = std::make_unique<Report>(
      k_test_report, Report::Type::REPORT,
      [](const std::shared_ptr<ShellBaseSession> &, const shcore::Array_t &argv,
         const shcore::Dictionary_t &options) {
        SCOPED_TRACE("Running report");

        ASSERT_NOT_NULL(argv);
        EXPECT_EQ(1, argv->size());
        EXPECT_EQ("argv0", (*argv)[0].as_string());

        ASSERT_NOT_NULL(options);
        std::string sort;
        shcore::Option_unpacker{options}.required("sort", &sort).end();
        EXPECT_EQ("ASC", sort);

        return make_report();
      });
  report->set_argc(std::make_pair(1, 1));
  report->set_options(
      {std::make_shared<Report::Option>("sort", Value_type::String, true)});

  m_reports->register_report(std::move(report));

  EXPECT_THAT(m_reports->call_report(k_test_report, m_session,
                                     {"--sort", "ASC", "argv0"}),
              ::testing::HasSubstr(k_expected_value));
}

TEST_F(Mod_shell_reports_test, report_bool_option) {
  auto report = std::make_unique<Report>(
      k_test_report, Report::Type::REPORT,
      [](const std::shared_ptr<ShellBaseSession> &, const shcore::Array_t &argv,
         const shcore::Dictionary_t &options) {
        SCOPED_TRACE("Running report");

        ASSERT_NOT_NULL(argv);
        EXPECT_EQ(0, argv->size());

        ASSERT_NOT_NULL(options);
        bool json = false;
        shcore::Option_unpacker{options}.optional("json", &json).end();
        EXPECT_EQ(true, json);

        return make_report();
      });
  report->set_options(
      {std::make_shared<Report::Option>("json", Value_type::Bool)});

  m_reports->register_report(std::move(report));

  EXPECT_THAT(m_reports->call_report(k_test_report, m_session, {"--json"}),
              ::testing::HasSubstr(k_expected_value));
}

TEST_F(Mod_shell_reports_test, report_optional_string_option) {
  auto report = std::make_unique<Report>(
      k_test_report, Report::Type::REPORT,
      [](const std::shared_ptr<ShellBaseSession> &, const shcore::Array_t &argv,
         const shcore::Dictionary_t &options) {
        SCOPED_TRACE("Running report");

        ASSERT_NOT_NULL(argv);
        EXPECT_EQ(0, argv->size());

        ASSERT_NOT_NULL(options);
        EXPECT_EQ(0, options->size());

        return make_report();
      });
  report->set_options({std::make_shared<Report::Option>("sort")});

  m_reports->register_report(std::move(report));

  EXPECT_THAT(m_reports->call_report(k_test_report, m_session, {}),
              ::testing::HasSubstr(k_expected_value));
}

}  // namespace tests
