/*
 * Copyright (c) 2018, 2025, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
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
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <rapidjson/document.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <iterator>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "mysql-secret-store/include/mysql-secret-store/api.h"
#include "mysqlshdk/libs/db/generic_uri.h"
#include "mysqlshdk/libs/db/uri_parser.h"
#include "mysqlshdk/libs/utils/process_launcher.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_json.h"
#include "mysqlshdk/libs/utils/utils_path.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "mysqlshdk/libs/utils/version.h"
#include "unittest/gtest_clean.h"
#include "unittest/test_utils/mocks/gmock_clean.h"
#include "unittest/test_utils/shell_test_env.h"
#include "unittest/test_utils/shell_test_wrapper.h"

using mysql::secret_store::api::get_available_helpers;
using mysql::secret_store::api::get_helper;
using mysql::secret_store::api::Helper_interface;
using mysql::secret_store::api::Helper_name;
using mysql::secret_store::api::Secret_spec;
using mysql::secret_store::api::Secret_type;

namespace tests {

namespace {

const auto g_format_parameter = [](const auto &info) {
  return shcore::str_replace(info.param, "-", "_");
};

template <typename T>
::testing::AssertionResult is_empty(const T &container) {
  if (container.empty()) {
    return ::testing::AssertionSuccess();
  } else {
    return ::testing::AssertionFailure()
           << "contains: " << shcore::str_join(container, ", ");
  }
}

constexpr auto k_secret_type_password = "password";
constexpr auto k_secret_type_generic = "generic";

std::string to_string(Secret_type type) {
  switch (type) {
    case Secret_type::PASSWORD:
      return k_secret_type_password;

    case Secret_type::GENERIC:
      return k_secret_type_generic;
  }

  throw std::runtime_error{"Unknown secret type"};
}

#define SCOPED_TRACE_TYPE(msg) \
  SCOPED_TRACE(msg + std::string{", type: "} + to_string(type))

class Helper_tester {
 public:
  virtual ~Helper_tester() = default;

  virtual bool uses_wrapper() const { return false; }
  virtual void use_wrapper(Shell_test_wrapper *) {}
  virtual void select_helper(const std::string &name) = 0;
  virtual void clear_store(std::optional<Secret_type> type = {}) = 0;
  virtual bool has_get() = 0;

  virtual bool store(Secret_type type, const std::string &id,
                     const std::string &secret) = 0;
  virtual bool get(Secret_type type, const std::string &id,
                   std::string *secret) = 0;
  virtual bool erase(Secret_type type, const std::string &id) = 0;

  virtual bool list(std::vector<Secret_spec> *specs,
                    std::optional<Secret_type> type = {}) = 0;

  virtual void expect_error(const std::string &msg) = 0;
  virtual void expect_no_error() = 0;

  virtual void expect_contains(Secret_type type, const std::string &id,
                               const std::string &secret) = 0;
  virtual void expect_not_contains(Secret_type type, const std::string &id) = 0;
};

template <typename T>
struct Helpers {
  static std::set<std::string> list;

 private:
  static std::set<std::string> list_helpers() {
    SCOPED_TRACE("Helpers::list_helpers()");
    auto helpers = T::list_helpers();
    std::set<std::string> ret{helpers.begin(), helpers.end()};
    EXPECT_TRUE(helpers.size() == ret.size())
        << "List of helpers contains duplicate values";
    return ret;
  }
};

template <typename T>
std::set<std::string> Helpers<T>::list = Helpers<T>::list_helpers();

class Mysql_secret_store_api_tester : public Helper_tester {
 public:
  static std::vector<std::string> list_helpers() {
    return call_get_available_helpers();
  }

  static std::vector<std::string> call_get_available_helpers(
      const std::string &path = "") {
    std::vector<std::string> names;
    auto helpers = get_available_helpers(path);

    std::transform(helpers.begin(), helpers.end(), std::back_inserter(names),
                   [](const Helper_name &name) { return name.get(); });

    return names;
  }

  void select_helper(const std::string &name) override {
    SCOPED_TRACE("Mysql_secret_store_api_tester::select_helper()");

    auto helpers = get_available_helpers();
    auto helper =
        std::find_if(helpers.begin(), helpers.end(),
                     [&name](const Helper_name &n) { return n.get() == name; });
    ASSERT_NE(helpers.end(), helper);
    m_helper = get_helper(*helper);
    ASSERT_NE(nullptr, m_helper);
  }

  void clear_store(std::optional<Secret_type> type = {}) override {
    SCOPED_TRACE("Mysql_secret_store_api_tester::clear_store()");

    std::vector<Secret_spec> specs;
    ASSERT_NE(nullptr, m_helper);
    EXPECT_TRUE(m_helper->list(&specs, type));
    expect_no_error();

    for (const auto &spec : specs) {
      SCOPED_TRACE("erasing: " + spec.id);
      EXPECT_TRUE(m_helper->erase(spec));
      expect_no_error();
    }
  }

  bool has_get() override { return true; }

  bool store(Secret_type type, const std::string &id,
             const std::string &secret) override {
    SCOPED_TRACE_TYPE("Mysql_secret_store_api_tester::store()");
    return m_helper->store({type, id}, secret);
  }

  bool get(Secret_type type, const std::string &id,
           std::string *secret) override {
    SCOPED_TRACE_TYPE("Mysql_secret_store_api_tester::get()");
    return m_helper->get({type, id}, secret);
  }

  bool erase(Secret_type type, const std::string &id) override {
    SCOPED_TRACE_TYPE("Mysql_secret_store_api_tester::erase()");
    return m_helper->erase({type, id});
  }

  bool list(std::vector<Secret_spec> *specs,
            std::optional<Secret_type> type = {}) override {
    SCOPED_TRACE("Mysql_secret_store_api_tester::list()");
    return m_helper->list(specs, type);
  }

  void expect_error(const std::string &msg) override {
    SCOPED_TRACE("Mysql_secret_store_api_tester::expect_error()");
    EXPECT_THAT(m_helper->get_last_error(), ::testing::HasSubstr(msg));
  }

  void expect_no_error() override {
    SCOPED_TRACE("Mysql_secret_store_api_tester::expect_no_error()");
    EXPECT_EQ("", m_helper->get_last_error());
  }

  void expect_contains(Secret_type type, const std::string &id,
                       const std::string &secret) override {
    SCOPED_TRACE_TYPE("Mysql_secret_store_api_tester::expect_contains()");
    std::string stored_secret;
    EXPECT_TRUE(get(type, id, &stored_secret));
    expect_no_error();
    EXPECT_EQ(secret, stored_secret);
  }

  void expect_not_contains(Secret_type type, const std::string &id) override {
    SCOPED_TRACE_TYPE("Mysql_secret_store_api_tester::expect_not_contains()");
    std::string stored_secret;
    EXPECT_FALSE(get(type, id, &stored_secret));
  }

  std::string name() const { return m_helper->name().get(); }

 private:
  std::unique_ptr<Helper_interface> m_helper;
};

class Shell_api_tester : public Helper_tester {
 public:
  static std::vector<std::string> list_helpers() {
    // this is invoked too early to call execute(), use library function
    return Mysql_secret_store_api_tester::call_get_available_helpers();
  }

  bool uses_wrapper() const override { return true; }

  void use_wrapper(Shell_test_wrapper *wrapper) override {
    m_wrapper = wrapper;
    m_wrapper->execute("\\js");
  }

  void select_helper(const std::string &name) override {
    SCOPED_TRACE("Shell_api_tester::select_helper()");

    execute("shell.options[\"credentialStore.helper\"] = " +
            shcore::quote_string(name, '\"') + ";");

    ASSERT_EQ("", output_handler().std_err);
  }

  void clear_store(std::optional<Secret_type> type = {}) override {
    SCOPED_TRACE("Shell_api_tester::clear_store()");

    if (type.has_value()) {
      EXPECT_EQ(m_config.type, *type);
    }

    expect_delete_all_credentials();
  }

  bool has_get() override { return m_config.get; }

  bool store(Secret_type type, const std::string &id,
             const std::string &secret) override {
    SCOPED_TRACE_TYPE("Shell_api_tester::store()");

    EXPECT_EQ(m_config.type, type);
    execute(shcore::str_format("shell.%s(%s, %s);", m_config.store,
                               shcore::quote_string(id, '\"').c_str(),
                               shcore::quote_string(secret, '\"').c_str()));

    return output_handler().std_err.empty();
  }

  bool get(Secret_type type, const std::string &id,
           std::string *secret) override {
    SCOPED_TRACE_TYPE("Shell_api_tester::get()");

    if (has_get()) {
      EXPECT_EQ(m_config.type, type);
      execute(shcore::str_format("print(shell.%s(%s));", m_config.get,
                                 shcore::quote_string(id, '\"').c_str()));

      *secret = output_handler().std_out;

      return output_handler().std_err.empty();
    } else {
      return false;
    }
  }

  bool erase(Secret_type type, const std::string &id) override {
    SCOPED_TRACE_TYPE("Shell_api_tester::erase()");

    EXPECT_EQ(m_config.type, type);
    execute(shcore::str_format("shell.%s(%s);", m_config.erase,
                               shcore::quote_string(id, '\"').c_str()));

    return output_handler().std_err.empty();
  }

  bool list(std::vector<Secret_spec> *specs,
            std::optional<Secret_type> type = {}) override {
    SCOPED_TRACE("Shell_api_tester::list()");

    if (type.has_value()) {
      EXPECT_EQ(m_config.type, *type);
    }

    execute(shcore::str_format("print(shell.%s());", m_config.list));

    for (const auto &id : parse_array_output()) {
      specs->emplace_back(Secret_spec{m_config.type, id});
    }

    return output_handler().std_err.empty();
  }

  void expect_error(const std::string &msg) override {
    SCOPED_TRACE("Shell_api_tester::expect_error()");
    output_handler().validate_stderr_content(msg, true);
  }

  void expect_no_error() override {
    SCOPED_TRACE("Shell_api_tester::expect_no_error()");
    EXPECT_EQ("", output_handler().std_err);
  }

  void expect_contains(Secret_type type, const std::string &id,
                       const std::string &) override {
    SCOPED_TRACE_TYPE("Shell_api_tester::expect_contains()");

    EXPECT_EQ(m_config.type, type);
    EXPECT_TRUE(contains_id(id));
  }

  void expect_not_contains(Secret_type type, const std::string &id) override {
    SCOPED_TRACE_TYPE("Shell_api_tester::expect_not_contains()");

    EXPECT_EQ(m_config.type, type);
    EXPECT_FALSE(contains_id(id));
  }

  void expect_delete_all_credentials() {
    SCOPED_TRACE("Shell_api_tester::expect_delete_all_credentials()");

    execute(shcore::str_format("shell.%s();", m_config.clear));

    EXPECT_EQ("", output_handler().std_err);
  }

 protected:
  struct Config {
    Secret_type type;
    const char *clear;
    const char *erase;
    const char *list;
    const char *get;
    const char *store;
  };

  explicit Shell_api_tester(Config &&config) : m_config(std::move(config)) {}

  void execute(const std::string &code) {
    output_handler().wipe_all();
    m_wrapper->execute(code);
  }

  inline Shell_test_output_handler &output_handler() {
    return m_wrapper->get_output_handler();
  }

 private:
  virtual std::string normalize_id(const std::string &id) const = 0;

  std::vector<std::string> parse_array_output() {
    std::vector<std::string> output;

    rapidjson::Document doc;

    doc.Parse(output_handler().std_out.c_str());

    if (!doc.HasParseError() && doc.IsArray()) {
      for (const auto &s : doc.GetArray()) {
        output.emplace_back(s.GetString());
      }
    }

    return output;
  }

  bool contains_id(const std::string &id) {
    SCOPED_TRACE("Shell_api_tester::contains_id()");

    std::vector<Secret_spec> specs;

    EXPECT_TRUE(list(&specs));
    expect_no_error();

    return specs.end() !=
           std::find(specs.begin(), specs.end(),
                     Secret_spec{m_config.type, normalize_id(id)});
  }

  Config m_config;
  Shell_test_wrapper *m_wrapper = nullptr;
};

class Shell_credential_api_tester : public Shell_api_tester {
 public:
  Shell_credential_api_tester()
      : Shell_api_tester({
            Secret_type::PASSWORD,
            "deleteAllCredentials",
            "deleteCredential",
            "listCredentials",
            nullptr,
            "storeCredential",
        }) {}

 private:
  std::string normalize_id(const std::string &id) const override {
    mysqlshdk::db::uri::Generic_uri copts;
    mysqlshdk::db::uri::Uri_parser parser(mysqlshdk::db::uri::Type::Generic);
    parser.parse(id, &copts);

    auto tokens = mysqlshdk::db::uri::formats::user_transport();

    if (copts.has_value(mysqlshdk::db::kScheme) &&
        (copts.get(mysqlshdk::db::kScheme) == "ssh" ||
         copts.get(mysqlshdk::db::kScheme) == "file")) {
      tokens.set(mysqlshdk::db::uri::Tokens::Scheme);
    }

    return copts.as_uri(tokens);
  }
};

class Shell_secret_api_tester : public Shell_api_tester {
 public:
  Shell_secret_api_tester()
      : Shell_api_tester({
            Secret_type::GENERIC,
            "deleteAllSecrets",
            "deleteSecret",
            "listSecrets",
            "readSecret",
            "storeSecret",
        }) {}

 private:
  std::string normalize_id(const std::string &id) const override { return id; }
};

class Shell_all_api_tester : public Helper_tester {
 public:
  Shell_all_api_tester() {
    m_testers[Secret_type::PASSWORD] =
        std::make_unique<Shell_credential_api_tester>();
    m_testers[Secret_type::GENERIC] =
        std::make_unique<Shell_secret_api_tester>();
  }

  static std::vector<std::string> list_helpers() {
    SCOPED_TRACE("Shell_all_api_tester::list_helpers()");
    return Shell_api_tester::list_helpers();
  }

  bool uses_wrapper() const override { return true; }

  void use_wrapper(Shell_test_wrapper *wrapper) override {
    for (const auto &tester : m_testers) {
      tester.second->use_wrapper(wrapper);
    }
  }

  void select_helper(const std::string &name) override {
    SCOPED_TRACE("Shell_all_api_tester::select_helper()");

    for (const auto &tester : m_testers) {
      tester.second->select_helper(name);
    }
  }

  void clear_store(std::optional<Secret_type> type = {}) override {
    SCOPED_TRACE("Shell_all_api_tester::clear_store()");

    for (const auto &tester : m_testers) {
      if (!type.has_value() || tester.first == *type) {
        tester.second->clear_store(type);
      }
    }
  }

  bool has_get() override {
    for (const auto &tester : m_testers) {
      if (!tester.second->has_get()) {
        return false;
      }
    }

    return true;
  }

  bool store(Secret_type type, const std::string &id,
             const std::string &secret) override {
    SCOPED_TRACE_TYPE("Shell_all_api_tester::store()");
    return tester(type)->store(type, id, secret);
  }

  bool get(Secret_type type, const std::string &id,
           std::string *secret) override {
    SCOPED_TRACE_TYPE("Shell_all_api_tester::get()");
    return tester(type)->get(type, id, secret);
  }

  bool erase(Secret_type type, const std::string &id) override {
    SCOPED_TRACE_TYPE("Shell_all_api_tester::erase()");
    return tester(type)->erase(type, id);
  }

  bool list(std::vector<Secret_spec> *specs,
            std::optional<Secret_type> type = {}) override {
    SCOPED_TRACE("Shell_all_api_tester::list()");

    for (const auto &tester : m_testers) {
      if (!type.has_value() || tester.first == *type) {
        if (!tester.second->list(specs, type)) {
          return false;
        }
      }
    }

    return true;
  }

  void expect_error(const std::string &msg) override {
    SCOPED_TRACE("Shell_all_api_tester::expect_error()");
    // it doesn't matter which tester we call here
    m_testers.begin()->second->expect_error(msg);
  }

  void expect_no_error() override {
    SCOPED_TRACE("Shell_all_api_tester::expect_no_error()");
    // it doesn't matter which tester we call here
    m_testers.begin()->second->expect_no_error();
  }

  void expect_contains(Secret_type type, const std::string &id,
                       const std::string &secret) override {
    SCOPED_TRACE_TYPE("Shell_all_api_tester::expect_contains()");
    tester(type)->expect_contains(type, id, secret);
  }

  void expect_not_contains(Secret_type type, const std::string &id) override {
    SCOPED_TRACE_TYPE("Shell_all_api_tester::expect_not_contains()");
    tester(type)->expect_not_contains(type, id);
  }

  inline Shell_api_tester *tester(Secret_type type) const {
    return m_testers.at(type).get();
  }

 private:
  std::unordered_map<Secret_type, std::unique_ptr<Shell_api_tester>> m_testers;
};

class Helper_invoker {
 public:
  explicit Helper_invoker(const std::string &path) : m_path{path} {}

  bool store(const std::string &input, std::string *output) const {
    return invoke("store", input, output);
  }

  bool get(const std::string &input, std::string *output) const {
    return invoke("get", input, output);
  }

  bool erase(const std::string &input, std::string *output) const {
    return invoke("erase", input, output);
  }

  bool list(std::string *output, const std::string &input = {}) const {
    return invoke("list", input, output);
  }

  bool version(std::string *output) const {
    return invoke("version", {}, output);
  }

  bool invoke(const char *command, const std::string &input,
              std::string *output) const {
    try {
      const char *const args[] = {m_path.c_str(), command, nullptr};
      shcore::Process_launcher app{args};

      app.start();

      if (!input.empty()) {
        app.write(input.c_str(), input.length());
      }

      app.finish_writing();

      *output = shcore::str_strip(app.read_all());

      return app.wait() == 0;
    } catch (const std::exception &ex) {
      *output = std::string{"Exception caught while running helper command '"} +
                command + "': " + ex.what();
      return false;
    }
  }

  const std::string m_path;
};

class Helper_executable_tester : public Helper_tester {
 public:
  static std::vector<std::string> list_helpers() {
    std::vector<std::string> output;
    auto paths = list_helper_paths();
    std::transform(paths.begin(), paths.end(), std::back_inserter(output),
                   get_helper_name);
    return output;
  }

  void select_helper(const std::string &name) override {
    SCOPED_TRACE("Helper_executable_tester::select_helper()");

    auto paths = list_helper_paths();
    auto helper = std::find_if(
        paths.begin(), paths.end(),
        [&name](const std::string &p) { return get_helper_name(p) == name; });
    ASSERT_NE(paths.end(), helper);
    m_invoker = std::make_unique<Helper_invoker>(*helper);
    ASSERT_NE(nullptr, m_invoker);
  }

  void clear_store(std::optional<Secret_type> type = {}) override {
    SCOPED_TRACE("Helper_executable_tester::clear_store()");

    ASSERT_NE(nullptr, m_invoker);

    std::string input;

    if (type.has_value()) {
      input = to_json(*type);
    }

    std::string output;

    if (!m_invoker->list(&output, input)) {
      ::testing::AssertionFailure() << output;
      return;
    }

    rapidjson::Document doc;

    doc.Parse(output.c_str());

    if (doc.HasParseError()) {
      ::testing::AssertionFailure() << "Failed to parse JSON";
      return;
    }

    if (!doc.IsArray()) {
      ::testing::AssertionFailure() << "Expected array";
      return;
    }

    for (const auto &s : doc.GetArray()) {
      output.clear();
      if (!m_invoker->erase(
              to_json(s[k_secret_type].GetString(), s[k_secret_id].GetString()),
              &output)) {
        ::testing::AssertionFailure() << output << "\n";
      }
    }
  }

  bool has_get() override { return true; }

  bool store(Secret_type type, const std::string &id,
             const std::string &secret) override {
    SCOPED_TRACE_TYPE("Helper_executable_tester::store()");

    std::string output;
    bool ret = m_invoker->store(to_json(type, id, secret), &output);

    if (ret) {
      clear_last_error();
    } else {
      set_last_error(output);
    }

    return ret;
  }

  bool get(Secret_type type, const std::string &id,
           std::string *secret) override {
    SCOPED_TRACE_TYPE("Helper_executable_tester::get()");

    std::string output;
    bool ret = m_invoker->get(to_json(type, id), &output);

    if (ret) {
      clear_last_error();
      Secret_spec spec;
      ret = to_secret(output, &spec, secret);

      if (spec.type != type || spec.id != id) {
        ret = false;
        set_last_error("Helper returned mismatched secret");
      }
    } else {
      set_last_error(output);
    }

    return ret;
  }

  bool erase(Secret_type type, const std::string &id) override {
    SCOPED_TRACE_TYPE("Helper_executable_tester::erase()");

    std::string output;
    bool ret = m_invoker->erase(to_json(type, id), &output);

    if (ret) {
      clear_last_error();
    } else {
      set_last_error(output);
    }

    return ret;
  }

  bool list(std::vector<Secret_spec> *specs,
            std::optional<Secret_type> type = {}) override {
    SCOPED_TRACE("Helper_executable_tester::list()");

    std::string input;

    if (type.has_value()) {
      input = to_json(*type);
    }

    std::string output;
    bool ret = m_invoker->list(&output, input);

    if (ret) {
      clear_last_error();
      ret = to_specs(output, specs);
    } else {
      set_last_error(output);
    }

    return ret;
  }

  void expect_error(const std::string &msg) override {
    SCOPED_TRACE("Helper_executable_tester::expect_error()");
    EXPECT_THAT(m_last_error, ::testing::HasSubstr(msg));
  }

  void expect_no_error() override {
    SCOPED_TRACE("Helper_executable_tester::expect_no_error()");
    EXPECT_EQ("", m_last_error);
  }

  void expect_contains(Secret_type type, const std::string &id,
                       const std::string &secret) override {
    SCOPED_TRACE_TYPE("Helper_executable_tester::expect_contains()");
    std::string stored_secret;
    EXPECT_TRUE(get(type, id, &stored_secret));
    expect_no_error();
    EXPECT_EQ(secret, stored_secret);
  }

  void expect_not_contains(Secret_type type, const std::string &id) override {
    SCOPED_TRACE_TYPE("Helper_executable_tester::expect_not_contains()");
    std::string stored_secret;
    EXPECT_FALSE(get(type, id, &stored_secret));
  }

  Helper_invoker &get_invoker() { return *m_invoker; }

  static std::string to_json(Secret_type type, const std::string &id,
                             const std::string &secret) {
    return to_json(to_string(type), id, secret);
  }

  static std::string to_json(const std::string &type, const std::string &id,
                             const std::string &secret) {
    shcore::JSON_dumper json;

    json.start_object();
    to_json(type, id, &json);
    json.append(k_secret, secret);
    json.end_object();

    return json.str();
  }

  static std::string to_json(Secret_type type, const std::string &id) {
    return to_json(to_string(type), id);
  }

  static std::string to_json(const std::string &type, const std::string &id) {
    shcore::JSON_dumper json;

    json.start_object();
    to_json(type, id, &json);
    json.end_object();

    return json.str();
  }

  static std::string to_json(Secret_type type) {
    return to_json(to_string(type));
  }

  static std::string to_json(const std::string &type) {
    shcore::JSON_dumper json;

    json.start_object();
    to_json(type, &json);
    json.end_object();

    return json.str();
  }

  bool to_secret(const std::string &json, Secret_spec *spec,
                 std::string *secret) {
    rapidjson::Document doc;

    doc.Parse(json.c_str());

    if (doc.HasParseError()) {
      set_last_error("Failed to parse JSON");
      return false;
    }

    if (spec && !to_spec(doc, spec)) {
      return false;
    }

    if (!validate_object(doc, {k_secret})) {
      return false;
    }

    secret->assign(doc[k_secret].GetString(), doc[k_secret].GetStringLength());

    return true;
  }

 private:
  static std::string get_helper_name(const std::string &path) {
    std::string name =
        shcore::path::basename(path).substr(strlen(k_exe_prefix));
    name = name.substr(0, name.find_last_of("."));

    return name;
  }

  static std::vector<std::string> list_helper_paths() {
    std::vector<std::string> helpers;
    const auto folder = shcore::get_binary_folder();

    if (!shcore::is_folder(folder)) {
      return helpers;
    }

    for (const auto &entry : shcore::listdir(folder)) {
      const std::string path = shcore::path::join_path(folder, entry);
      std::string version;

      if (entry.compare(0, strlen(k_exe_prefix), k_exe_prefix) == 0 &&
          !shcore::is_folder(path) && Helper_invoker{path}.version(&version)) {
        helpers.emplace_back(path);
      }
    }

    return helpers;
  }

  static void to_json(const std::string &type, const std::string &id,
                      shcore::JSON_dumper *json) {
    to_json(type, json);
    json->append(k_secret_id, id);
  }

  static void to_json(const std::string &type, shcore::JSON_dumper *json) {
    json->append(k_secret_type, type);
  }

  bool validate_object(const rapidjson::Value &object,
                       const std::set<std::string> &members) {
    if (!object.IsObject()) {
      set_last_error("Expected object");
      return false;
    }

    for (const auto &m : members) {
      if (!object.HasMember(m.c_str())) {
        set_last_error("Missing member: \"" + m + "\"");
      }

      if (!object[m.c_str()].IsString()) {
        set_last_error("Member \"" + m + "\" is not a string");
      }
    }

    return true;
  }

  bool to_secret_type(const std::string &type, Secret_type *output_type) {
    if (type == k_secret_type_password) {
      *output_type = Secret_type::PASSWORD;
    } else if (type == k_secret_type_generic) {
      *output_type = Secret_type::GENERIC;
    } else {
      set_last_error("Unknown secret type: \"" + type + "\"");
      return false;
    }

    return true;
  }

  bool to_spec(const rapidjson::Value &object, Secret_spec *spec) {
    if (!validate_object(object, {k_secret_type, k_secret_id})) {
      return false;
    }

    if (!to_secret_type(object[k_secret_type].GetString(), &spec->type)) {
      return false;
    }

    spec->id = object[k_secret_id].GetString();

    return true;
  }

  bool to_specs(const std::string &json, std::vector<Secret_spec> *specs) {
    rapidjson::Document doc;

    doc.Parse(json.c_str());

    if (doc.HasParseError()) {
      set_last_error("Failed to parse JSON");
      return false;
    }

    if (!doc.IsArray()) {
      set_last_error("Expected array");
      return false;
    }

    for (const auto &s : doc.GetArray()) {
      Secret_spec spec;

      if (!to_spec(s, &spec)) {
        return false;
      } else {
        specs->emplace_back(spec);
      }
    }

    return true;
  }

  void set_last_error(const std::string &str) const { m_last_error = str; }

  void clear_last_error() const { m_last_error.clear(); }

  static constexpr auto k_exe_prefix = "mysql-secret-store-";
  static constexpr auto k_secret = "Secret";
  static constexpr auto k_secret_type = "SecretType";
  static constexpr auto k_secret_id = "SecretID";

  std::unique_ptr<Helper_invoker> m_invoker;

  mutable std::string m_last_error;
};

template <typename T>
class Parametrized_helper_test : public ::testing::TestWithParam<std::string> {
 public:
  using Helper_t = T;

 protected:
  void SetUp() override {
    if (Shell_test_env::get_target_server_version() <
        mysqlshdk::utils::k_shell_version) {
      // this is needed to prevent gtest from invoking the body of a test
      GTEST_SKIP();
      SKIP_TEST("This test is only executed once, with latest server");
      return;
    }

    if (tester.uses_wrapper()) {
      m_wrapper = std::make_unique<Shell_test_wrapper>(false);
      tester.use_wrapper(m_wrapper.get());
    }

    tester.select_helper(helper_name());

    if (::getenv("TEST_DEBUG")) {
      mysql::secret_store::api::set_logger([](std::string_view msg) {
        fprintf(stderr, "%.*s\n", static_cast<int>(msg.size()), msg.data());
      });
    }

    clear_store();
  }

  void TearDown() override {
    if (this->IsSkipped()) {
      return;
    }

    clear_store();

    if (::getenv("TEST_DEBUG")) {
      mysql::secret_store::api::set_logger({});
    }

    m_wrapper.reset();
  }

  const std::string &helper_name() const { return GetParam(); }

  bool is_login_path() const { return "login-path" == helper_name(); }

  bool is_keychain() const { return "keychain" == helper_name(); }

  bool is_secret_service() const { return "secret-service" == helper_name(); }

  bool store(Secret_type type, const std::string &id,
             const std::string &secret) {
    return tester.store(type, id, secret);
  }

  bool get(Secret_type type, const std::string &id, std::string *secret) {
    return tester.get(type, id, secret);
  }

  bool erase(Secret_type type, const std::string &id) {
    return tester.erase(type, id);
  }

  void expect_list(Secret_type type, const std::set<std::string> &ids) {
    SCOPED_TRACE("Parametrized_helper_test::expect_list()");
    std::vector<Secret_spec> specs;
    EXPECT_TRUE(tester.list(&specs, type));
    expect_no_error();

    std::set<std::string> list_ids;

    for (const auto &s : specs) {
      SCOPED_TRACE(s.id + " -> " + to_string(s.type));
      EXPECT_EQ(type, s.type);
      list_ids.emplace(s.id);
    }

    EXPECT_EQ(ids, list_ids);
  }

  void expect_contains(Secret_type type, const std::string &id,
                       const std::string &secret) {
    tester.expect_contains(type, id, secret);
  }

  void expect_not_contains(Secret_type type, const std::string &id) {
    tester.expect_not_contains(type, id);
  }

  void expect_error(const std::string &msg) { tester.expect_error(msg); }

  void expect_no_error() { tester.expect_no_error(); }

  void clear_store() { tester.clear_store(); }

  T tester;

 private:
  std::unique_ptr<Shell_test_wrapper> m_wrapper;
};

template <typename T, Secret_type Type>
class Parametrized_helper_test_with_type : public Parametrized_helper_test<T> {
 protected:
  bool store(const std::string &id, const std::string &secret) {
    return Parametrized_helper_test<T>::store(Type, id, secret);
  }

  bool get(const std::string &id, std::string *secret) {
    return Parametrized_helper_test<T>::get(Type, id, secret);
  }

  bool erase(const std::string &id) {
    return Parametrized_helper_test<T>::erase(Type, id);
  }

  void expect_list(const std::set<std::string> &ids) {
    Parametrized_helper_test<T>::expect_list(Type, ids);
  }

  void expect_contains(const std::string &id, const std::string &secret) {
    Parametrized_helper_test<T>::expect_contains(Type, id, secret);
  }

  void expect_not_contains(const std::string &id) {
    Parametrized_helper_test<T>::expect_not_contains(Type, id);
  }
};

class Config_editor_invoker {
 public:
  void store(const std::string &name, const std::string &id,
             const std::string &secret) {
    std::vector<std::string> args = {"set", "--skip-warn", "--password"};
    args.emplace_back("--login-path=" + name);

    mysqlshdk::db::uri::Generic_uri options;
    mysqlshdk::db::uri::Uri_parser parser(mysqlshdk::db::uri::Type::Generic);
    parser.parse(id, &options);

    if (options.has_value(mysqlshdk::db::kUser)) {
      args.emplace_back("--user=" + options.get(mysqlshdk::db::kUser));
    }
    if (options.has_value(mysqlshdk::db::kHost)) {
      args.emplace_back("--host=" + options.get(mysqlshdk::db::kHost));
    }
    if (options.has_value(mysqlshdk::db::kPort)) {
      args.emplace_back("--port=" + std::to_string(options.get_numeric(
                                        mysqlshdk::db::kPort)));
    }
    if (options.has_value(mysqlshdk::db::kSocket)) {
      args.emplace_back("--socket=" + options.get(mysqlshdk::db::kSocket));
    }

    invoke(args, true, secret);
  }

  void clear() { invoke({"reset"}); }

  mysqlshdk::utils::Version version() const {
    return mysqlshdk::utils::Version{
        shcore::str_split(invoke({"--version"}), " ", -1, true)[2]};
  }

  std::string invoke(const std::vector<std::string> &args,
                     bool uses_terminal = false,
                     const std::string &password = "") const {
    std::vector<const char *> process_args;
    process_args.emplace_back("mysql_config_editor");

    for (const auto &arg : args) {
      process_args.emplace_back(arg.c_str());
    }

    process_args.emplace_back(nullptr);

    shcore::Process_launcher app{&process_args[0]};

    if (uses_terminal) {
      app.enable_child_terminal();
    }

    app.start();

    if (uses_terminal) {
      // wait for password prompt
      app.read_from_terminal();
      app.write_to_terminal(password.c_str(), password.length());
      app.write_to_terminal("\n", 1);
    }

    std::string output = shcore::str_strip(app.read_all());

    if (0 != app.wait()) {
      throw std::runtime_error{output};
    }

    return output;
  }
};

void verify_available_helpers(const std::set<std::string> &helpers) {
  SCOPED_TRACE("verify_available_helpers()");

  std::set<std::string> expected = {"plaintext"};

#ifdef _WIN32
  expected.emplace("windows-credential");
#else  // ! _WIN32
#ifdef __APPLE__
  expected.emplace("keychain");
#endif  // ! __APPLE__
  expected.emplace("login-path");
#endif  // ! _WIN32

  std::set<std::string> missing;
  std::set_difference(expected.begin(), expected.end(), helpers.begin(),
                      helpers.end(), std::inserter(missing, missing.begin()));

  EXPECT_TRUE(is_empty(missing));
}

void verify_available_helpers(const std::vector<std::string> &helpers) {
  verify_available_helpers(
      std::set<std::string>{helpers.begin(), helpers.end()});
}

template <typename T>
void test_available_helpers() {
  SCOPED_TRACE("test_available_helpers()");

  verify_available_helpers(Helpers<T>::list);
}

#define IGNORE_TEST(test_case_name, test_name)                 \
  class test_case_name##_##test_name : public test_case_name { \
   private:                                                    \
    void test_body();                                          \
  };                                                           \
  void test_case_name##_##test_name::test_body()

#define MY_EXPECT_ERROR(expression, error) \
  do {                                     \
    EXPECT_FALSE(expression);              \
    expect_error(error);                   \
  } while (false)

#define MY_EXPECT_NO_ERROR(expression) \
  do {                                 \
    EXPECT_TRUE(expression);           \
    expect_no_error();                 \
  } while (false)

#define GET_EXPECT_ERROR(id, secret, error) \
  if (tester.has_get()) {                   \
    std::string s;                          \
    MY_EXPECT_ERROR(get(id, &s), error);    \
  }

#define INVALID_URL_ERROR "Invalid URL"

#define STORE_INVALID_URL(url, secret)                      \
  do {                                                      \
    SCOPED_TRACE(url + std::string{" -> "} + secret);       \
    MY_EXPECT_ERROR(store(url, secret), INVALID_URL_ERROR); \
  } while (false)

#define GET_INVALID_URL(url, secret)                  \
  do {                                                \
    SCOPED_TRACE(url + std::string{" -> "} + secret); \
    GET_EXPECT_ERROR(url, secret, INVALID_URL_ERROR)  \
  } while (false)

#define ERASE_INVALID_URL(url, secret)                \
  do {                                                \
    SCOPED_TRACE(url + std::string{" -> "} + secret); \
    MY_EXPECT_ERROR(erase(url), INVALID_URL_ERROR);   \
  } while (false)

#define OP_INVALID_URL_TESTS(op)                                    \
  do {                                                              \
    op("host", "password");                                         \
    op("user@", "password");                                        \
    op("user@@", "password");                                       \
    op("user:secret@host", "password");                             \
    op("http://user@host", "password");                             \
    op("http://user@host:3306", "password");                        \
    op("mysql://user@host", "password");                            \
    op("mysql://user@host:3306", "password");                       \
    op("mysqlx://user@host", "password");                           \
    op("mysqlx://user@host:33060", "password");                     \
    op("user@host/schema", "password");                             \
    op("user@host:3306/schema", "password");                        \
    op("user@host?tls-version=xyz", "password");                    \
    op("user@host:3306?tls-version=xyz", "password");               \
    op("user@host/schema?tls-version=xyz", "password");             \
    op("user@host:3306/schema?tls-version=xyz", "password");        \
    op("http://user@host/schema?tls-version=xyz", "password");      \
    op("http://user@host:3306/schema?tls-version=xyz", "password"); \
  } while (false)

#define STORE_AND_CHECK(id, secret)                  \
  do {                                               \
    SCOPED_TRACE(id + std::string{" -> "} + secret); \
    MY_EXPECT_NO_ERROR(store(id, secret));           \
    expect_contains(id, secret);                     \
  } while (false)

#define STORE_AND_ERASE(id, secret)                  \
  do {                                               \
    SCOPED_TRACE(id + std::string{" -> "} + secret); \
    MY_EXPECT_NO_ERROR(store(id, secret));           \
    expect_contains(id, secret);                     \
    MY_EXPECT_NO_ERROR(erase(id));                   \
    expect_not_contains(id);                         \
  } while (false)

#ifdef _WIN32
#define URL_WITH_SOCKET "user@\\\\.\\named.pipe"
#define URL_WITH_SOCKET_AND_PARENTHESES "user@\\\\.\\(named.pipe)"
#else  // !_WIN32
#define URL_WITH_SOCKET "user@/socket"
#define URL_WITH_SOCKET_AND_PARENTHESES "user@(/socket)"
#endif  // !_WIN32

#define STORE_AND_OP_TESTS(op)                                              \
  do {                                                                      \
    op("empty@host", "");                                                   \
    op("user@host", "one");                                                 \
    op("user@host:3306", "two");                                            \
    op(URL_WITH_SOCKET, "three");                                           \
    op("user@host:33060", "five");                                          \
    op("user@host:33060", "six");                                           \
    op("ssh://user@host.com:22", "test");                                   \
    op("file:/user/host/com", "test");                                      \
    if (is_login_path()) {                                                  \
      op("user@host:55555",                                                 \
         "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"   \
         "abcdefghijklm");                                                  \
    } else {                                                                \
      op("user@host:55555",                                                 \
         "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"   \
         "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"   \
         "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"   \
         "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"   \
         "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"   \
         "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"   \
         "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"   \
         "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"   \
         "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"   \
         "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"   \
         "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"   \
         "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"   \
         "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"   \
         "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"   \
         "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"   \
         "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"   \
         "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"   \
         "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"   \
         "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"   \
         "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"   \
         "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"   \
         "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"   \
         "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"   \
         "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"   \
         "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"   \
         "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"   \
         "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"   \
         "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"   \
         "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"   \
         "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"); \
    }                                                                       \
    op("user@example.com", "zażółć gęślą jaźń");                            \
    op("user@host", "\"pass\"'word'");                                      \
    op("user@host1", "\\");                                                 \
    op("user@host2", "\\\\");                                               \
    op("user@host3", "\\\"\\'");                                            \
    op("user@host4", "\"password\"");                                       \
    op("user@host5", "'password'");                                         \
    op("user@[::1]", "seven");                                              \
    op("user@[::1]:7777", "eight");                                         \
    op("user@[fe80::850a:5a7c:6ab7:aec4%25enp0s3]", "nine");                \
    op("user@[fe80::850a:5a7c:6ab7:aec4%25enp0s3]:8888", "ten");            \
    op("test%21%23%24%26%28%29%2A%2B%2C%2F%3A%3B%3D%3F%40%5B%5D"            \
       "@example.com:9999",                                                 \
       "eleven");                                                           \
  } while (false)

#define ADD_TESTS(test_case_name)                                             \
  VALIDATION_TEST(test_case_name, store_invalid_url) {                        \
    OP_INVALID_URL_TESTS(STORE_INVALID_URL);                                  \
  }                                                                           \
                                                                              \
  VALIDATION_TEST(test_case_name, get_invalid_url) {                          \
    if (!tester.has_get()) {                                                  \
      return;                                                                 \
    }                                                                         \
    OP_INVALID_URL_TESTS(GET_INVALID_URL);                                    \
  }                                                                           \
                                                                              \
  VALIDATION_TEST(test_case_name, erase_invalid_url) {                        \
    OP_INVALID_URL_TESTS(ERASE_INVALID_URL);                                  \
  }                                                                           \
                                                                              \
  TEST_P(test_case_name, store_and_check) {                                   \
    STORE_AND_OP_TESTS(STORE_AND_CHECK);                                      \
  }                                                                           \
                                                                              \
  TEST_P(test_case_name, store_and_erase) {                                   \
    STORE_AND_OP_TESTS(STORE_AND_ERASE);                                      \
  }                                                                           \
                                                                              \
  TEST_P(test_case_name, list) {                                              \
    std::set<std::string> ids{"user@host", "user@host:3306",                  \
                              "user@host:33060", URL_WITH_SOCKET};            \
    for (const auto &id : ids) {                                              \
      STORE_AND_CHECK(id, id);                                                \
    }                                                                         \
    expect_list(ids);                                                         \
    /* overwrite a value */                                                   \
    STORE_AND_CHECK(*ids.begin(), "new pass");                                \
    expect_list(ids);                                                         \
  }                                                                           \
                                                                              \
  TEST_P(test_case_name, get_nonexistent_id) {                                \
    constexpr auto error = "Could not find the secret";                       \
    GET_EXPECT_ERROR("user@nonexistent.host", "secret", error);               \
  }                                                                           \
                                                                              \
  TEST_P(test_case_name, erase_nonexistent_id) {                              \
    constexpr auto error = "Could not find the secret";                       \
    MY_EXPECT_ERROR(erase("user@nonexistent.host"), error);                   \
  }                                                                           \
                                                                              \
  NORMALIZATION_TEST(test_case_name, url_normalization) {                     \
    STORE_AND_CHECK(URL_WITH_SOCKET, "one");                                  \
    STORE_AND_CHECK(URL_WITH_SOCKET_AND_PARENTHESES, "two");                  \
    expect_contains(URL_WITH_SOCKET, "two");                                  \
  }                                                                           \
                                                                              \
  TEST(Helpers, test_case_name##_list_helpers) {                              \
    test_available_helpers<test_case_name::Helper_t>();                       \
  }                                                                           \
                                                                              \
  PASSWORD_TEST(test_case_name, login_path_external_entry) {                  \
    if (!is_login_path()) {                                                   \
      return;                                                                 \
    }                                                                         \
    Config_editor_invoker invoker;                                            \
    invoker.clear();                                                          \
    std::set<std::string> urls{"user@host", "user@host:3306",                 \
                               "user@host:33060", URL_WITH_SOCKET};           \
    for (const auto &url : urls) {                                            \
      invoker.store(url, url, url);                                           \
      expect_contains(url, url);                                              \
    }                                                                         \
    expect_list(urls);                                                        \
    invoker.clear();                                                          \
  }                                                                           \
                                                                              \
  PASSWORD_TEST(test_case_name, login_path_external_and_helper_entries) {     \
    if (!is_login_path()) {                                                   \
      return;                                                                 \
    }                                                                         \
    Config_editor_invoker invoker;                                            \
    invoker.clear();                                                          \
    invoker.store("one", "user@host", "one");                                 \
    STORE_AND_CHECK("user@host", "two");                                      \
    expect_contains("user@host", "two");                                      \
    expect_list({"user@host"});                                               \
    erase("user@host");                                                       \
    expect_contains("user@host", "one");                                      \
    expect_list({"user@host"});                                               \
    erase("user@host");                                                       \
    expect_list({});                                                          \
    invoker.clear();                                                          \
  }                                                                           \
                                                                              \
  PASSWORD_TEST(test_case_name,                                               \
                login_path_external_entry_with_port_and_socket) {             \
    if (!is_login_path()) {                                                   \
      return;                                                                 \
    }                                                                         \
    Config_editor_invoker invoker;                                            \
    invoker.clear();                                                          \
    auto initialize = [&invoker]() {                                          \
      std::vector<std::string> args = {"set",         "--skip-warn",          \
                                       "--password",  "--login-path=one",     \
                                       "--user=user", "--host=localhost",     \
                                       "--port=3306", "--socket=/socket"};    \
      invoker.invoke(args, true, "one");                                      \
    };                                                                        \
    initialize();                                                             \
    expect_list({"user@localhost:3306", URL_WITH_SOCKET});                    \
    STORE_AND_CHECK("user@localhost:3306", "two");                            \
    STORE_AND_CHECK(URL_WITH_SOCKET, "three");                                \
    expect_list({"user@localhost:3306", URL_WITH_SOCKET});                    \
    erase(URL_WITH_SOCKET);                                                   \
    expect_contains(URL_WITH_SOCKET, "one");                                  \
    expect_contains("user@localhost:3306", "two");                            \
    expect_list({"user@localhost:3306", URL_WITH_SOCKET});                    \
    erase("user@localhost:3306");                                             \
    expect_contains(URL_WITH_SOCKET, "one");                                  \
    expect_contains("user@localhost:3306", "one");                            \
    expect_list({"user@localhost:3306", URL_WITH_SOCKET});                    \
    erase(URL_WITH_SOCKET);                                                   \
    expect_contains("user@localhost:3306", "one");                            \
    expect_list({"user@localhost:3306"});                                     \
    erase("user@localhost:3306");                                             \
    expect_list({});                                                          \
    invoker.clear();                                                          \
  }                                                                           \
                                                                              \
  PASSWORD_TEST(test_case_name, login_path_ipv6_stored_by_config_editor) {    \
    if (!is_login_path()) {                                                   \
      return;                                                                 \
    }                                                                         \
    Config_editor_invoker invoker;                                            \
    auto initialize = [&invoker](const std::string &host) {                   \
      std::vector<std::string> args = {                                       \
          "set",         "--skip-warn",    "--password", "--login-path=path", \
          "--user=user", "--host=" + host, "--port=3306"};                    \
      invoker.clear();                                                        \
      invoker.invoke(args, true, "pass");                                     \
    };                                                                        \
    initialize("::1");                                                        \
    expect_list({"user@[::1]:3306"});                                         \
    initialize("fe80::850a:5a7c:6ab7:aec4");                                  \
    expect_list({"user@[fe80::850a:5a7c:6ab7:aec4]:3306"});                   \
    initialize("fe80::850a:5a7c:6ab7:aec4%enp0s3");                           \
    expect_list({"user@[fe80::850a:5a7c:6ab7:aec4%25enp0s3]:3306"});          \
    invoker.clear();                                                          \
  }                                                                           \
                                                                              \
  PASSWORD_TEST(test_case_name, login_path_ipv6_stored_by_helper) {           \
    if (!is_login_path()) {                                                   \
      return;                                                                 \
    }                                                                         \
    Config_editor_invoker invoker;                                            \
    invoker.clear();                                                          \
    const auto quoted =                                                       \
        invoker.version() >= mysqlshdk::utils::Version(8, 0, 24);             \
    const auto host = [&quoted](const std::string &h) {                       \
      return "host = " + (quoted ? shcore::quote_string(h, '"') : h);         \
    };                                                                        \
    STORE_AND_CHECK("user@[::1]", "one");                                     \
    auto output = invoker.invoke({"print", "--all"});                         \
    EXPECT_THAT(output, ::testing::HasSubstr(host("::1")));                   \
    invoker.clear();                                                          \
    STORE_AND_CHECK("user@[fe80::850a:5a7c:6ab7:aec4]:4321", "two");          \
    output = invoker.invoke({"print", "--all"});                              \
    EXPECT_THAT(output,                                                       \
                ::testing::HasSubstr(host("fe80::850a:5a7c:6ab7:aec4")));     \
    EXPECT_THAT(output, ::testing::HasSubstr("port = 4321"));                 \
    invoker.clear();                                                          \
    STORE_AND_CHECK("user@[fe80::850a:5a7c:6ab7:aec4%25enp0s3]", "three");    \
    output = invoker.invoke({"print", "--all"});                              \
    EXPECT_THAT(output, ::testing::HasSubstr(                                 \
                            host("fe80::850a:5a7c:6ab7:aec4%enp0s3")));       \
    invoker.clear();                                                          \
  }

#define ADD_SEPARATION_TESTS(test_case_name)                               \
  TEST_P(test_case_name, separation_store_and_check) {                     \
    MY_EXPECT_NO_ERROR(store(Secret_type::PASSWORD, "user@host", "pass")); \
    MY_EXPECT_NO_ERROR(store(Secret_type::GENERIC, "root@host", "PASS"));  \
    expect_contains(Secret_type::PASSWORD, "user@host", "pass");           \
    expect_contains(Secret_type::GENERIC, "root@host", "PASS");            \
    expect_not_contains(Secret_type::GENERIC, "user@host");                \
    expect_not_contains(Secret_type::PASSWORD, "root@host");               \
  }                                                                        \
                                                                           \
  TEST_P(test_case_name, separation_store_and_erase) {                     \
    MY_EXPECT_NO_ERROR(store(Secret_type::PASSWORD, "user@host", "pass")); \
    MY_EXPECT_NO_ERROR(store(Secret_type::GENERIC, "root@host", "PASS"));  \
    MY_EXPECT_NO_ERROR(erase(Secret_type::PASSWORD, "user@host"));         \
    expect_not_contains(Secret_type::PASSWORD, "user@host");               \
    expect_contains(Secret_type::GENERIC, "root@host", "PASS");            \
    MY_EXPECT_NO_ERROR(store(Secret_type::PASSWORD, "user@host", "pass")); \
    MY_EXPECT_NO_ERROR(erase(Secret_type::GENERIC, "root@host"));          \
    expect_contains(Secret_type::PASSWORD, "user@host", "pass");           \
    expect_not_contains(Secret_type::GENERIC, "root@host");                \
  }                                                                        \
                                                                           \
  TEST_P(test_case_name, separation_store_and_get) {                       \
    MY_EXPECT_NO_ERROR(store(Secret_type::PASSWORD, "user@host", "pass")); \
    MY_EXPECT_NO_ERROR(store(Secret_type::GENERIC, "root@host", "PASS"));  \
    std::string secret;                                                    \
    EXPECT_FALSE(get(Secret_type::GENERIC, "user@host", &secret));         \
    EXPECT_FALSE(get(Secret_type::PASSWORD, "root@host", &secret));        \
  }                                                                        \
                                                                           \
  TEST_P(test_case_name, separation_store_and_get_same_id) {               \
    MY_EXPECT_NO_ERROR(store(Secret_type::PASSWORD, "user@host", "pass")); \
    MY_EXPECT_NO_ERROR(store(Secret_type::GENERIC, "user@host", "PASS"));  \
    std::string secret;                                                    \
    if (get(Secret_type::PASSWORD, "user@host", &secret)) {                \
      EXPECT_EQ("pass", secret);                                           \
    }                                                                      \
    if (get(Secret_type::GENERIC, "user@host", &secret)) {                 \
      EXPECT_EQ("PASS", secret);                                           \
    }                                                                      \
  }                                                                        \
                                                                           \
  TEST_P(test_case_name, separation_store_and_list) {                      \
    MY_EXPECT_NO_ERROR(store(Secret_type::PASSWORD, "user@host", "pass")); \
    MY_EXPECT_NO_ERROR(store(Secret_type::GENERIC, "root@host", "PASS"));  \
    expect_list(Secret_type::PASSWORD, {"user@host"});                     \
    expect_list(Secret_type::GENERIC, {"root@host"});                      \
  }

#define REGISTER_TESTS(test_case_name)                              \
  INSTANTIATE_TEST_SUITE_P(                                         \
      Helpers, test_case_name,                                      \
      ::testing::ValuesIn(Helpers<test_case_name::Helper_t>::list), \
      g_format_parameter)

}  // namespace

////////////////////////////////////////////////////////////////////////////////
// API tests which do not depend on a helper's type
////////////////////////////////////////////////////////////////////////////////

TEST(Helpers, Mysql_secret_store_api_test_secret_spec_operators) {
  Secret_spec one{Secret_type::PASSWORD, "first URL"};
  Secret_spec two{Secret_type::PASSWORD, "second URL"};

  EXPECT_FALSE(one == two);
  EXPECT_TRUE(one != two);

  one.id = two.id;

  EXPECT_TRUE(one == two);
  EXPECT_FALSE(one != two);
}

TEST(Helpers, Mysql_secret_store_api_test_get_available_helpers_custom_dir) {
  verify_available_helpers(
      Mysql_secret_store_api_tester::call_get_available_helpers(
          shcore::get_binary_folder()));
}

TEST(Helpers, Mysql_secret_store_api_test_get_available_helpers_invalid_dir) {
  auto home = shcore::get_home_dir();

  do {
    home = shcore::path::join_path(home, "random_name");
  } while (shcore::is_folder(home));

  EXPECT_TRUE(is_empty(
      Mysql_secret_store_api_tester::call_get_available_helpers(home)));
}

////////////////////////////////////////////////////////////////////////////////
// API tests which do not depend on a secrets's type
////////////////////////////////////////////////////////////////////////////////

class Mysql_secret_store_api_test
    : public Parametrized_helper_test<Mysql_secret_store_api_tester> {};

TEST_P(Mysql_secret_store_api_test, helper_name) {
  EXPECT_EQ(helper_name(), tester.name());
}

TEST_P(Mysql_secret_store_api_test, get_nullptr) {
  EXPECT_FALSE(tester.get(Secret_type::PASSWORD, "url", nullptr));
  expect_error("Invalid pointer");
}

TEST_P(Mysql_secret_store_api_test, list_nullptr) {
  EXPECT_FALSE(tester.list(nullptr));
  expect_error("Invalid pointer");
}

ADD_SEPARATION_TESTS(Mysql_secret_store_api_test);

REGISTER_TESTS(Mysql_secret_store_api_test);

////////////////////////////////////////////////////////////////////////////////
// API tests with Secret_type::PASSWORD
////////////////////////////////////////////////////////////////////////////////

#define VALIDATION_TEST TEST_P
#define NORMALIZATION_TEST TEST_P
#define PASSWORD_TEST TEST_P

class Mysql_secret_store_api_test_password
    : public Parametrized_helper_test_with_type<Mysql_secret_store_api_tester,
                                                Secret_type::PASSWORD> {};

ADD_TESTS(Mysql_secret_store_api_test_password)

#undef VALIDATION_TEST
#undef NORMALIZATION_TEST
#undef PASSWORD_TEST

REGISTER_TESTS(Mysql_secret_store_api_test_password);

////////////////////////////////////////////////////////////////////////////////
// API tests with Secret_type::GENERIC
////////////////////////////////////////////////////////////////////////////////

#define VALIDATION_TEST IGNORE_TEST
#define NORMALIZATION_TEST IGNORE_TEST
#define PASSWORD_TEST IGNORE_TEST

class Mysql_secret_store_api_test_generic
    : public Parametrized_helper_test_with_type<Mysql_secret_store_api_tester,
                                                Secret_type::GENERIC> {};

ADD_TESTS(Mysql_secret_store_api_test_generic)

#undef VALIDATION_TEST
#undef NORMALIZATION_TEST
#undef PASSWORD_TEST

REGISTER_TESTS(Mysql_secret_store_api_test_generic);

////////////////////////////////////////////////////////////////////////////////
// Separation tests of the shell.*Credential() and shell.*Secret() functions
////////////////////////////////////////////////////////////////////////////////

class Shell_all_api_test
    : public Parametrized_helper_test<Shell_all_api_tester> {};

ADD_SEPARATION_TESTS(Shell_all_api_test);

TEST_P(Shell_all_api_test, separation_delete_all_secrets) {
  MY_EXPECT_NO_ERROR(store(Secret_type::PASSWORD, "user@host", "pass"));
  MY_EXPECT_NO_ERROR(store(Secret_type::GENERIC, "root@host", "PASS"));

  tester.tester(Secret_type::PASSWORD)->expect_delete_all_credentials();
  expect_list(Secret_type::PASSWORD, {});
  expect_list(Secret_type::GENERIC, {"root@host"});

  MY_EXPECT_NO_ERROR(store(Secret_type::PASSWORD, "user@host", "pass"));

  tester.tester(Secret_type::GENERIC)->expect_delete_all_credentials();
  expect_list(Secret_type::PASSWORD, {"user@host"});
  expect_list(Secret_type::GENERIC, {});
}

REGISTER_TESTS(Shell_all_api_test);

////////////////////////////////////////////////////////////////////////////////
// Tests of the shell.*Credential() functions
////////////////////////////////////////////////////////////////////////////////

#define VALIDATION_TEST TEST_P
#define NORMALIZATION_TEST TEST_P
#define PASSWORD_TEST TEST_P

class Shell_credential_api_test
    : public Parametrized_helper_test_with_type<Shell_credential_api_tester,
                                                Secret_type::PASSWORD> {};

ADD_TESTS(Shell_credential_api_test)

#undef VALIDATION_TEST
#undef NORMALIZATION_TEST
#undef PASSWORD_TEST

TEST_P(Shell_credential_api_test, delete_all_credentials) {
  std::set<std::string> ids{"user@host", "user@host:3306", "user@host:33060",
                            URL_WITH_SOCKET};
  for (const auto &id : ids) {
    STORE_AND_CHECK(id, id);
  }
  tester.expect_delete_all_credentials();
  expect_list({});
}

REGISTER_TESTS(Shell_credential_api_test);

////////////////////////////////////////////////////////////////////////////////
// Tests of the shell.*Secret() functions
////////////////////////////////////////////////////////////////////////////////

#define VALIDATION_TEST IGNORE_TEST
#define NORMALIZATION_TEST IGNORE_TEST
#define PASSWORD_TEST IGNORE_TEST

class Shell_secret_api_test
    : public Parametrized_helper_test_with_type<Shell_secret_api_tester,
                                                Secret_type::GENERIC> {};

ADD_TESTS(Shell_secret_api_test)

#undef VALIDATION_TEST
#undef NORMALIZATION_TEST
#undef PASSWORD_TEST

TEST_P(Shell_secret_api_test, delete_all_secrets) {
  std::set<std::string> ids{"user@host", "user@host:3306", "user@host:33060",
                            URL_WITH_SOCKET};
  for (const auto &id : ids) {
    STORE_AND_CHECK(id, id);
  }
  tester.expect_delete_all_credentials();
  expect_list({});
}

REGISTER_TESTS(Shell_secret_api_test);

////////////////////////////////////////////////////////////////////////////////
// Tests of helper executables which do not depend on secret's type
////////////////////////////////////////////////////////////////////////////////

class Helper_executable_test
    : public Parametrized_helper_test<Helper_executable_tester> {};

TEST_P(Helper_executable_test, uri_should_not_be_validated) {
  std::string output;
  const auto secret_type = Secret_type::PASSWORD;
  const std::string secret_id = "some+string";
  auto &invoker = tester.get_invoker();

  EXPECT_TRUE(
      invoker.store(tester.to_json(secret_type, secret_id, "pass"), &output));
  EXPECT_EQ("", output);
  output.clear();

  EXPECT_TRUE(invoker.get(tester.to_json(secret_type, secret_id), &output));
  EXPECT_NE("", output);
  output.clear();

  EXPECT_TRUE(invoker.erase(tester.to_json(secret_type, secret_id), &output));
  EXPECT_EQ("", output);
  output.clear();
}

TEST_P(Helper_executable_test, secret_type_should_not_be_validated) {
  std::string output;
  const std::string secret_type = "some_type";
  const std::string secret_id = "some+string";
  auto &invoker = tester.get_invoker();

  EXPECT_TRUE(
      invoker.store(tester.to_json(secret_type, secret_id, "pass"), &output));
  EXPECT_EQ("", output);
  output.clear();

  EXPECT_TRUE(invoker.get(tester.to_json(secret_type, secret_id), &output));
  EXPECT_NE("", output);
  output.clear();

  EXPECT_TRUE(invoker.erase(tester.to_json(secret_type, secret_id), &output));
  EXPECT_EQ("", output);
  output.clear();
}

TEST_P(Helper_executable_test, invalid_command) {
  std::string output;
  auto &invoker = tester.get_invoker();

  EXPECT_FALSE(invoker.invoke(
      "unknown", tester.to_json(Secret_type::PASSWORD, "user@host"), &output));
  EXPECT_THAT(output, ::testing::HasSubstr("Unknown command"));
}

TEST_P(Helper_executable_test, missing_command) {
  std::string output;
  auto &invoker = tester.get_invoker();

  EXPECT_FALSE(invoker.invoke(
      nullptr, tester.to_json(Secret_type::PASSWORD, "user@host"), &output));
  EXPECT_THAT(output, ::testing::HasSubstr("Missing command"));
}

TEST_P(Helper_executable_test, invalid_json_input_misspelled_secret_id) {
  const std::string error_message = R"("SecretID" is missing)";
  auto &invoker = tester.get_invoker();
  std::string output;

  for (const auto &s : {"SecretId", "SEcretID", "SecretIDD", "ScretID"}) {
    SCOPED_TRACE(s);

    std::string spec =
        R"(")" + std::string{s} + R"(":"user@host","SecretType":"password")";

    EXPECT_FALSE(invoker.store("{" + spec + R"(,"Secret":"pass"})", &output));
    EXPECT_THAT(output, ::testing::HasSubstr(error_message));
    output.clear();

    EXPECT_FALSE(invoker.get("{" + spec + "}", &output));
    EXPECT_THAT(output, ::testing::HasSubstr(error_message));
    output.clear();

    EXPECT_FALSE(invoker.erase("{" + spec + "}", &output));
    EXPECT_THAT(output, ::testing::HasSubstr(error_message));
    output.clear();
  }
}

TEST_P(Helper_executable_test, invalid_json_input_misspelled_secret_type) {
  const std::string error_message = R"("SecretType" is missing)";
  auto &invoker = tester.get_invoker();
  std::string output;

  for (const auto &s :
       {"Secrettype", "SEcretType", "SecretTypee", "ScretType"}) {
    SCOPED_TRACE(s);

    std::string spec =
        R"("SecretID":"user@host",")" + std::string{s} + R"(":"password")";

    EXPECT_FALSE(invoker.store("{" + spec + R"(,"Secret":"pass"})", &output));
    EXPECT_THAT(output, ::testing::HasSubstr(error_message));
    output.clear();

    EXPECT_FALSE(invoker.get("{" + spec + "}", &output));
    EXPECT_THAT(output, ::testing::HasSubstr(error_message));
    output.clear();

    EXPECT_FALSE(invoker.erase("{" + spec + "}", &output));
    EXPECT_THAT(output, ::testing::HasSubstr(error_message));
    output.clear();
  }
}

TEST_P(Helper_executable_test, invalid_json_input_misspelled_secret) {
  const std::string error_message = R"("Secret" is missing)";
  auto &invoker = tester.get_invoker();
  std::string output;

  for (const auto &s : {"secret", "SEcret", "Secrett", "Scret"}) {
    SCOPED_TRACE(s);

    std::string spec = R"({"SecretID":"user@host","SecretType":"password",")" +
                       std::string{s} + R"(":"pass"})";

    EXPECT_FALSE(invoker.store(spec, &output));
    EXPECT_THAT(output, ::testing::HasSubstr(error_message));
    output.clear();
  }
}

TEST_P(Helper_executable_test, invalid_json_input_unknown_member) {
  auto &invoker = tester.get_invoker();
  std::string output;
  std::string spec =
      R"("SecretID":"user@host","SecretType":"password","InvalidMember":12345)";

  EXPECT_TRUE(invoker.store("{" + spec + R"(,"Secret":"pass"})", &output));
  EXPECT_EQ("", output);
  output.clear();

  EXPECT_TRUE(invoker.get("{" + spec + "}", &output));
  EXPECT_THAT(output, ::testing::HasSubstr(R"("Secret":"pass")"));
  output.clear();

  EXPECT_TRUE(invoker.erase("{" + spec + "}", &output));
  EXPECT_EQ("", output);
  output.clear();
}

TEST_P(Helper_executable_test, invalid_json_input_missing_secret_id) {
  const std::string error_message = R"("SecretID" is missing)";
  auto &invoker = tester.get_invoker();
  std::string output;
  std::string spec = R"("SecretType":"password")";

  EXPECT_FALSE(invoker.store("{" + spec + R"(,"Secret":"pass"})", &output));
  EXPECT_THAT(output, ::testing::HasSubstr(error_message));
  output.clear();

  EXPECT_FALSE(invoker.get("{" + spec + "}", &output));
  EXPECT_THAT(output, ::testing::HasSubstr(error_message));
  output.clear();

  EXPECT_FALSE(invoker.erase("{" + spec + "}", &output));
  EXPECT_THAT(output, ::testing::HasSubstr(error_message));
  output.clear();
}

TEST_P(Helper_executable_test, invalid_json_input_missing_secret_type) {
  const std::string error_message = R"("SecretType" is missing)";
  auto &invoker = tester.get_invoker();
  std::string output;
  std::string spec = R"("SecretID":"user@host")";

  EXPECT_FALSE(invoker.store("{" + spec + R"(,"Secret":"pass"})", &output));
  EXPECT_THAT(output, ::testing::HasSubstr(error_message));
  output.clear();

  EXPECT_FALSE(invoker.get("{" + spec + "}", &output));
  EXPECT_THAT(output, ::testing::HasSubstr(error_message));
  output.clear();

  EXPECT_FALSE(invoker.erase("{" + spec + "}", &output));
  EXPECT_THAT(output, ::testing::HasSubstr(error_message));
  output.clear();
}

TEST_P(Helper_executable_test, invalid_json_input_missing_secret) {
  const std::string error_message = R"("Secret" is missing)";
  auto &invoker = tester.get_invoker();
  std::string output;
  std::string spec = R"({"SecretID":"user@host","SecretType":"password"})";

  EXPECT_FALSE(invoker.store(spec, &output));
  EXPECT_THAT(output, ::testing::HasSubstr(error_message));
  output.clear();
}

TEST_P(Helper_executable_test, invalid_json_input_wrong_type_secret_id) {
  const std::string error_message = R"("SecretID" should be a string)";
  auto &invoker = tester.get_invoker();
  std::string output;
  std::string spec = R"("SecretID":false,"SecretType":"password")";

  EXPECT_FALSE(invoker.store("{" + spec + R"(,"Secret":"pass"})", &output));
  EXPECT_THAT(output, ::testing::HasSubstr(error_message));
  output.clear();

  EXPECT_FALSE(invoker.get("{" + spec + "}", &output));
  EXPECT_THAT(output, ::testing::HasSubstr(error_message));
  output.clear();

  EXPECT_FALSE(invoker.erase("{" + spec + "}", &output));
  EXPECT_THAT(output, ::testing::HasSubstr(error_message));
  output.clear();
}

TEST_P(Helper_executable_test, invalid_json_input_wrong_type_secret_type) {
  const std::string error_message = R"("SecretType" should be a string)";
  auto &invoker = tester.get_invoker();
  std::string output;
  std::string spec = R"("SecretID":"user@host","SecretType":false)";

  EXPECT_FALSE(invoker.store("{" + spec + R"(,"Secret":"pass"})", &output));
  EXPECT_THAT(output, ::testing::HasSubstr(error_message));
  output.clear();

  EXPECT_FALSE(invoker.get("{" + spec + "}", &output));
  EXPECT_THAT(output, ::testing::HasSubstr(error_message));
  output.clear();

  EXPECT_FALSE(invoker.erase("{" + spec + "}", &output));
  EXPECT_THAT(output, ::testing::HasSubstr(error_message));
  output.clear();
}

TEST_P(Helper_executable_test, invalid_json_input_wrong_type_secret) {
  const std::string error_message = R"("Secret" should be a string)";
  auto &invoker = tester.get_invoker();
  std::string output;
  std::string spec =
      R"({"SecretID":"user@host","SecretType":"password","Secret":false})";

  EXPECT_FALSE(invoker.store(spec, &output));
  EXPECT_THAT(output, ::testing::HasSubstr(error_message));
  output.clear();
}

TEST_P(Helper_executable_test, version_command) {
  auto &invoker = tester.get_invoker();
  std::string output;
  EXPECT_TRUE(invoker.version(&output));
  EXPECT_THAT(output, ::testing::HasSubstr(shcore::get_long_version()));
  output.clear();
}

TEST_P(Helper_executable_test, deprecated_server_url) {
  // ServerURL is deprecated, helpers still recognize it and reply with both
  // ServerURL and SecretID
  auto &invoker = tester.get_invoker();
  std::string output;
  std::string spec = R"("ServerURL":"user@host","SecretType":"password")";

  EXPECT_TRUE(invoker.store("{" + spec + R"(,"Secret":"pass"})", &output));
  EXPECT_EQ("", output);
  output.clear();

  EXPECT_TRUE(invoker.get("{" + spec + "}", &output));
  EXPECT_EQ(
      R"({"SecretType":"password","SecretID":"user@host","ServerURL":"user@host","Secret":"pass"})",
      output);
  output.clear();

  EXPECT_TRUE(invoker.erase("{" + spec + "}", &output));
  EXPECT_EQ("", output);
  output.clear();
}

TEST_P(Helper_executable_test, list_with_secret_type) {
  auto &invoker = tester.get_invoker();
  std::string output;

  EXPECT_TRUE(invoker.store(
      tester.to_json(Secret_type::PASSWORD, "user@host", "pass"), &output));
  EXPECT_EQ("", output);
  output.clear();

  EXPECT_TRUE(invoker.store(
      tester.to_json(Secret_type::GENERIC, "some-id", "PASS"), &output));
  EXPECT_EQ("", output);
  output.clear();

  EXPECT_TRUE(invoker.list(&output, R"({"SecretType":"password"})"));
  EXPECT_EQ(
      R"([{"SecretType":"password","SecretID":"user@host","ServerURL":"user@host"}])",
      output);
  output.clear();

  EXPECT_TRUE(invoker.list(&output, R"({"SecretType":"generic"})"));
  EXPECT_EQ(
      R"([{"SecretType":"generic","SecretID":"some-id","ServerURL":"some-id"}])",
      output);
  output.clear();
}

TEST_P(Helper_executable_test, valid_id_characters) {
  const std::unordered_set<char> unsupported_keychain = {
      '\x00',
      '\x0A',
  };
  const auto unsupported_secret_service = []() {
    std::unordered_set<char> invalid;

    // secret service only supports UTF-8 strings
    for (int i = 128; i <= 255; ++i) {
      invalid.emplace(static_cast<char>(i));
    }

    return invalid;
  }();
  std::unordered_set<char> unsupported;

  if (is_keychain()) {
    unsupported = unsupported_keychain;
  } else if (is_secret_service()) {
    unsupported = unsupported_secret_service;
  }

  const auto TEST_SECRET_ID = [&invoker = tester.get_invoker(),
                               this](const std::string &secret_id) {
    std::string output;
    const std::string secret = "pass";
    std::string actual_secret;

    EXPECT_TRUE(invoker.store(
        tester.to_json(Secret_type::GENERIC, secret_id, secret), &output));
    EXPECT_EQ("", output);
    output.clear();

    EXPECT_TRUE(
        invoker.get(tester.to_json(Secret_type::GENERIC, secret_id), &output));
    EXPECT_TRUE(tester.to_secret(output, nullptr, &actual_secret));
    EXPECT_EQ(secret, actual_secret);
    output.clear();
    actual_secret.clear();

    EXPECT_TRUE(invoker.erase(tester.to_json(Secret_type::GENERIC, secret_id),
                              &output));
    EXPECT_EQ("", output);
    output.clear();
  };

  const auto TEST_INVALID_SECRET_ID = [&invoker = tester.get_invoker(),
                                       this](const std::string &secret_id) {
    std::string output;
    const std::string secret = "pass";

    EXPECT_FALSE(invoker.store(
        tester.to_json(Secret_type::GENERIC, secret_id, secret), &output));
    EXPECT_EQ("Secret's ID contains a disallowed character", output);
    output.clear();
  };

  for (int i = 0; i <= 255; ++i) {
    const auto c = static_cast<char>(i);

    if (unsupported.count(c)) {
      continue;
    }

    SCOPED_TRACE("Character: " + std::to_string(c));
    TEST_SECRET_ID(std::string(1, c));
  }

  for (const std::string utf8 : {"¢", "€", "𐍈"}) {
    SCOPED_TRACE("String: " + utf8);
    TEST_SECRET_ID(utf8);
  }

  for (const auto c : unsupported) {
    SCOPED_TRACE("Character: " + std::to_string(c));
    TEST_INVALID_SECRET_ID(std::string(1, c));
  }
}

TEST_P(Helper_executable_test, valid_secret_characters) {
  const std::unordered_set<char> unsupported_login_path = {
      '\x00', '\x03', '\x04', '\x0A', '\x0D', '\x0F', '\x11', '\x12',
      '\x13', '\x15', '\x16', '\x17', '\x19', '\x1A', '\x1C', '\x7F',
  };
  const std::unordered_set<char> unsupported_keychain = {
      '\x00',
      '\x0A',
  };
  const auto unsupported_secret_service = []() {
    std::unordered_set<char> invalid;

    // secret service only supports UTF-8 strings
    for (int i = 128; i <= 255; ++i) {
      invalid.emplace(static_cast<char>(i));
    }

    return invalid;
  }();
  std::unordered_set<char> unsupported;

  if (is_login_path()) {
    unsupported = unsupported_login_path;
  } else if (is_keychain()) {
    unsupported = unsupported_keychain;
  } else if (is_secret_service()) {
    unsupported = unsupported_secret_service;
  }

  const auto TEST_SECRET = [&invoker = tester.get_invoker(),
                            this](const std::string &secret) {
    std::string output;
    const std::string secret_id = "some-id";
    std::string actual_secret;

    EXPECT_TRUE(invoker.store(
        tester.to_json(Secret_type::GENERIC, secret_id, secret), &output));
    EXPECT_EQ("", output);
    output.clear();

    EXPECT_TRUE(
        invoker.get(tester.to_json(Secret_type::GENERIC, secret_id), &output));
    EXPECT_TRUE(tester.to_secret(output, nullptr, &actual_secret));
    EXPECT_EQ(secret, actual_secret);
    output.clear();
    actual_secret.clear();

    EXPECT_TRUE(invoker.erase(tester.to_json(Secret_type::GENERIC, secret_id),
                              &output));
    EXPECT_EQ("", output);
    output.clear();
  };

  const auto TEST_INVALID_SECRET = [&invoker = tester.get_invoker(),
                                    this](const std::string &secret) {
    std::string output;
    const std::string secret_id = "some-id";

    EXPECT_FALSE(invoker.store(
        tester.to_json(Secret_type::GENERIC, secret_id, secret), &output));
    EXPECT_EQ("Secret contains a disallowed character", output);
    output.clear();
  };

  for (int i = 0; i <= 255; ++i) {
    const auto c = static_cast<char>(i);

    if (unsupported.count(c)) {
      continue;
    }

    SCOPED_TRACE("Character: " + std::to_string(c));
    TEST_SECRET(std::string(1, c));
  }

  for (const std::string utf8 : {"¢", "€", "𐍈"}) {
    SCOPED_TRACE("String: " + utf8);
    TEST_SECRET(utf8);
  }

  for (const auto c : unsupported) {
    SCOPED_TRACE("Character: " + std::to_string(c));
    TEST_INVALID_SECRET(std::string(1, c));
  }
}

ADD_SEPARATION_TESTS(Helper_executable_test);

REGISTER_TESTS(Helper_executable_test);

////////////////////////////////////////////////////////////////////////////////
// Tests of helper executables with Secret_type::PASSWORD
////////////////////////////////////////////////////////////////////////////////

#define VALIDATION_TEST IGNORE_TEST
#define NORMALIZATION_TEST IGNORE_TEST
#define PASSWORD_TEST TEST_P

class Helper_executable_test_password
    : public Parametrized_helper_test_with_type<Helper_executable_tester,
                                                Secret_type::PASSWORD> {};

ADD_TESTS(Helper_executable_test_password)

#undef VALIDATION_TEST
#undef NORMALIZATION_TEST
#undef PASSWORD_TEST

REGISTER_TESTS(Helper_executable_test_password);

////////////////////////////////////////////////////////////////////////////////
// Tests of helper executables with Secret_type::GENERIC
////////////////////////////////////////////////////////////////////////////////

#define VALIDATION_TEST IGNORE_TEST
#define NORMALIZATION_TEST IGNORE_TEST
#define PASSWORD_TEST IGNORE_TEST

class Helper_executable_test_generic
    : public Parametrized_helper_test_with_type<Helper_executable_tester,
                                                Secret_type::GENERIC> {};

ADD_TESTS(Helper_executable_test_generic)

#undef VALIDATION_TEST
#undef NORMALIZATION_TEST
#undef PASSWORD_TEST

REGISTER_TESTS(Helper_executable_test_generic);

}  // namespace tests
