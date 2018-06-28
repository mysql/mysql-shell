/*
 * Copyright (c) 2018, Oracle and/or its affiliates. All rights reserved.
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

#include <rapidjson/document.h>

#include <algorithm>
#include <iterator>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "mysql-secret-store/include/mysql-secret-store/api.h"
#include "mysqlshdk/libs/utils/process_launcher.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_path.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "unittest/gtest_clean.h"
#include "unittest/test_utils/mocks/gmock_clean.h"
#include "unittest/test_utils/shell_test_wrapper.h"

using mysql::secret_store::api::Helper_interface;
using mysql::secret_store::api::Helper_name;
using mysql::secret_store::api::Secret_spec;
using mysql::secret_store::api::Secret_type;
using mysql::secret_store::api::get_available_helpers;
using mysql::secret_store::api::get_helper;

namespace tests {

namespace {

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

std::string to_string(Secret_type type) {
  switch (type) {
    case Secret_type::PASSWORD:
      return k_secret_type_password;
  }

  throw std::runtime_error{"Unknown secret type"};
}

#define SCOPED_TRACE_TYPE(msg) \
  SCOPED_TRACE(msg + std::string{", type: "} + to_string(type))

class Helper_tester {
 public:
  virtual ~Helper_tester() = default;

  virtual void select_helper(const std::string &name) = 0;
  virtual void clear_store() = 0;
  virtual bool has_get() = 0;

  virtual bool store(Secret_type type, const std::string &url,
                     const std::string &secret) = 0;
  virtual bool get(Secret_type type, const std::string &url,
                   std::string *secret) = 0;
  virtual bool erase(Secret_type type, const std::string &url) = 0;

  virtual bool list(std::vector<Secret_spec> *specs) = 0;

  virtual void expect_error(const std::string &msg) = 0;
  virtual void expect_no_error() = 0;

  virtual void expect_contains(Secret_type type, const std::string &url,
                               const std::string &secret) = 0;
  virtual void expect_not_contain(Secret_type type, const std::string &url) = 0;
};

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

  void clear_store() override {
    SCOPED_TRACE("Mysql_secret_store_api_tester::clear_store()");

    std::vector<Secret_spec> specs;
    ASSERT_NE(nullptr, m_helper);
    EXPECT_TRUE(m_helper->list(&specs));
    expect_no_error();
    for (const auto &spec : specs) {
      EXPECT_TRUE(m_helper->erase(spec));
      expect_no_error();
    }
  }

  bool has_get() override { return true; }

  bool store(Secret_type type, const std::string &url,
             const std::string &secret) override {
    SCOPED_TRACE_TYPE("Mysql_secret_store_api_tester::store()");
    return m_helper->store({type, url}, secret);
  }

  bool get(Secret_type type, const std::string &url,
           std::string *secret) override {
    SCOPED_TRACE_TYPE("Mysql_secret_store_api_tester::get()");
    return m_helper->get({type, url}, secret);
  }

  bool erase(Secret_type type, const std::string &url) override {
    SCOPED_TRACE_TYPE("Mysql_secret_store_api_tester::erase()");
    return m_helper->erase({type, url});
  }

  bool list(std::vector<Secret_spec> *specs) override {
    SCOPED_TRACE("Mysql_secret_store_api_tester::list()");
    return m_helper->list(specs);
  }

  void expect_error(const std::string &msg) override {
    SCOPED_TRACE("Mysql_secret_store_api_tester::expect_error()");
    EXPECT_THAT(m_helper->get_last_error(), ::testing::HasSubstr(msg));
  }

  void expect_no_error() override {
    SCOPED_TRACE("Mysql_secret_store_api_tester::expect_no_error()");
    EXPECT_EQ("", m_helper->get_last_error());
  }

  void expect_contains(Secret_type type, const std::string &url,
                       const std::string &secret) override {
    SCOPED_TRACE_TYPE("Mysql_secret_store_api_tester::expect_contains()");
    std::string stored_secret;
    EXPECT_TRUE(get(type, url, &stored_secret));
    expect_no_error();
    EXPECT_EQ(secret, stored_secret);
  }

  void expect_not_contain(Secret_type type, const std::string &url) override {
    SCOPED_TRACE_TYPE("Mysql_secret_store_api_tester::expect_not_contain()");
    std::string stored_secret;
    EXPECT_FALSE(get(type, url, &stored_secret));
  }

  std::string name() const { return m_helper->name().get(); }

 private:
  std::unique_ptr<Helper_interface> m_helper;
};

class Shell_api_tester : public Helper_tester {
 public:
  Shell_api_tester() {}

  static std::vector<std::string> list_helpers() {
    // this is invoked too early to call execute(), use library function
    return Mysql_secret_store_api_tester::call_get_available_helpers();
  }

  std::vector<std::string> call_list_helpers() {
    execute("print(shell.listCredentialHelpers());");

    return parse_output();
  }

  void select_helper(const std::string &name) override {
    SCOPED_TRACE("Shell_api_tester::select_helper()");

    m_wrapper.reset(new Shell_test_wrapper{false});

    execute("shell.options[\"credentialStore.helper\"] = \"" + name + "\";");

    ASSERT_EQ("", m_wrapper->get_output_handler().std_err);
  }

  void clear_store() override {
    SCOPED_TRACE("Shell_api_tester::clear_store()");

    expect_delete_all_credentials();
  }

  bool has_get() override { return false; }

  bool store(Secret_type type, const std::string &url,
             const std::string &secret) override {
    SCOPED_TRACE_TYPE("Shell_api_tester::store()");

    EXPECT_EQ(Secret_type::PASSWORD, type);

    execute("shell.storeCredential(\"" + url + "\", " +
            shcore::quote_string(secret, '\"') + ");");
    return m_wrapper->get_output_handler().std_err.empty();
  }

  bool get(Secret_type, const std::string &, std::string *) override {
    ADD_FAILURE() << "This should not be called";
    return false;
  }

  bool erase(Secret_type type, const std::string &url) override {
    SCOPED_TRACE_TYPE("Shell_api_tester::erase()");

    EXPECT_EQ(Secret_type::PASSWORD, type);

    execute("shell.deleteCredential(\"" + url + "\");");

    return m_wrapper->get_output_handler().std_err.empty();
  }

  bool list(std::vector<Secret_spec> *specs) override {
    SCOPED_TRACE("Shell_api_tester::list()");

    execute("print(shell.listCredentials());");

    for (const auto &url : parse_output()) {
      specs->emplace_back(Secret_spec{Secret_type::PASSWORD, url});
    }

    return m_wrapper->get_output_handler().std_err.empty();
  }

  void expect_error(const std::string &msg) override {
    SCOPED_TRACE("Shell_api_tester::expect_error()");
    m_wrapper->get_output_handler().validate_stderr_content(msg, true);
  }

  void expect_no_error() override {
    SCOPED_TRACE("Shell_api_tester::expect_no_error()");
    EXPECT_EQ("", m_wrapper->get_output_handler().std_err);
  }

  void expect_contains(Secret_type type, const std::string &url,
                       const std::string &) override {
    SCOPED_TRACE_TYPE("Shell_api_tester::expect_contains()");

    EXPECT_EQ(Secret_type::PASSWORD, type);
    EXPECT_TRUE(has_url(url));
  }

  void expect_not_contain(Secret_type type, const std::string &url) override {
    SCOPED_TRACE_TYPE("Shell_api_tester::expect_not_contain()");

    EXPECT_EQ(Secret_type::PASSWORD, type);
    EXPECT_FALSE(has_url(url));
  }

  void expect_delete_all_credentials() {
    SCOPED_TRACE("Shell_api_tester::expect_delete_all_credentials()");

    execute("shell.deleteAllCredentials();");

    EXPECT_EQ("", m_wrapper->get_output_handler().std_err);
  }

 private:
  void execute(const std::string &code) {
    m_wrapper->get_output_handler().wipe_all();
    m_wrapper->execute(code);
  }

  std::vector<std::string> parse_output() {
    std::vector<std::string> output;

    rapidjson::Document doc;

    doc.Parse(m_wrapper->get_output_handler().std_out.c_str());

    if (!doc.HasParseError() && doc.IsArray()) {
      for (const auto &s : doc.GetArray()) {
        output.emplace_back(s.GetString());
      }
    }

    return output;
  }

  bool has_url(const std::string &url) {
    SCOPED_TRACE("Shell_api_tester::has_url()");

    std::vector<Secret_spec> specs;

    EXPECT_TRUE(list(&specs));
    expect_no_error();
    auto normalized = mysqlsh::Connection_options{url}.as_uri(
        mysqlshdk::db::uri::formats::user_transport());
    return specs.end() !=
           std::find(specs.begin(), specs.end(),
                     Secret_spec{Secret_type::PASSWORD, normalized});
  }

  std::unique_ptr<Shell_test_wrapper> m_wrapper;
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

  bool list(std::string *output) const { return invoke("list", {}, output); }

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
        app.finish_writing();
      }

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
    m_invoker = std::unique_ptr<Helper_invoker>{new Helper_invoker{*helper}};
    ASSERT_NE(nullptr, m_invoker);
  }

  void clear_store() override {
    SCOPED_TRACE("Helper_executable_tester::clear_store()");

    ASSERT_NE(nullptr, m_invoker);

    std::string output;

    if (!m_invoker->list(&output)) {
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
      if (!m_invoker->erase(to_json(s[k_secret_type].GetString(),
                                    s[k_server_url].GetString()),
                            &output)) {
        ::testing::AssertionFailure() << output << "\n";
      }
    }
  }

  bool has_get() override { return true; }

  bool store(Secret_type type, const std::string &url,
             const std::string &secret) override {
    SCOPED_TRACE_TYPE("Helper_executable_tester::store()");

    std::string output;
    bool ret = m_invoker->store(to_json(type, url, secret), &output);

    if (ret) {
      clear_last_error();
    } else {
      set_last_error(output);
    }

    return ret;
  }

  bool get(Secret_type type, const std::string &url,
           std::string *secret) override {
    SCOPED_TRACE_TYPE("Helper_executable_tester::get()");

    std::string output;
    bool ret = m_invoker->get(to_json(type, url), &output);

    if (ret) {
      clear_last_error();
      Secret_spec spec;
      ret = to_secret(output, &spec, secret);

      if (spec.type != type || spec.url != url) {
        ret = false;
        set_last_error("Helper returned mismatched secret");
      }
    } else {
      set_last_error(output);
    }

    return ret;
  }

  bool erase(Secret_type type, const std::string &url) override {
    SCOPED_TRACE_TYPE("Helper_executable_tester::erase()");

    std::string output;
    bool ret = m_invoker->erase(to_json(type, url), &output);

    if (ret) {
      clear_last_error();
    } else {
      set_last_error(output);
    }

    return ret;
  }

  bool list(std::vector<Secret_spec> *specs) override {
    SCOPED_TRACE("Helper_executable_tester::list()");

    std::string output;
    bool ret = m_invoker->list(&output);

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

  void expect_contains(Secret_type type, const std::string &url,
                       const std::string &secret) override {
    SCOPED_TRACE_TYPE("Helper_executable_tester::expect_contains()");
    std::string stored_secret;
    EXPECT_TRUE(get(type, url, &stored_secret));
    expect_no_error();
    EXPECT_EQ(secret, stored_secret);
  }

  void expect_not_contain(Secret_type type, const std::string &url) override {
    SCOPED_TRACE_TYPE("Helper_executable_tester::expect_not_contain()");
    std::string stored_secret;
    EXPECT_FALSE(get(type, url, &stored_secret));
  }

  Helper_invoker &get_invoker() { return *m_invoker; }

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

  static std::string to_json(Secret_type type, const std::string &url,
                             const std::string &secret) {
    auto json = to_json(type, url);
    json.pop_back();
    return json + ",\"" + k_secret +
           "\":" + shcore::quote_string(secret, '\"') + "}";
  }

  static std::string to_json(Secret_type type, const std::string &url) {
    return to_json(to_string(type), url);
  }

  static std::string to_json(const std::string &type, const std::string &url) {
    return std::string{"{\""} + k_server_url + "\":\"" + url + "\",\"" +
           k_secret_type + "\":\"" + type + "\"}";
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
    } else {
      set_last_error("Unknown secret type: \"" + type + "\"");
      return false;
    }

    return true;
  }

  bool to_spec(const rapidjson::Value &object, Secret_spec *spec) {
    if (!validate_object(object, {k_secret_type, k_server_url})) {
      return false;
    }

    if (!to_secret_type(object[k_secret_type].GetString(), &spec->type)) {
      return false;
    }

    spec->url = object[k_server_url].GetString();

    return true;
  }

  bool to_secret(const std::string &json, Secret_spec *spec,
                 std::string *secret) {
    rapidjson::Document doc;

    doc.Parse(json.c_str());

    if (doc.HasParseError()) {
      set_last_error("Failed to parse JSON");
      return false;
    }

    if (!to_spec(doc, spec)) {
      return false;
    }

    if (!validate_object(doc, {k_secret})) {
      return false;
    }

    *secret = doc[k_secret].GetString();

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
  static constexpr auto k_server_url = "ServerURL";

  std::unique_ptr<Helper_invoker> m_invoker;

  mutable std::string m_last_error;
};

template <typename T>
class Parametrized_helper_test : public ::testing::TestWithParam<std::string> {
 public:
  static std::set<std::string> list_helpers() {
    SCOPED_TRACE("Parametrized_helper_test::list_helpers()");
    auto helpers = T::list_helpers();
    std::set<std::string> ret{helpers.begin(), helpers.end()};
    EXPECT_TRUE(helpers.size() == ret.size())
        << "List of helpers contains duplicate values";
    return ret;
  }

 protected:
  void SetUp() override {
    tester.select_helper(GetParam());
    clear_store();
  }

  void TearDown() override { clear_store(); }

  bool store(const std::string &url, const std::string &secret) {
    return tester.store(Secret_type::PASSWORD, url, secret);
  }

  bool get(const std::string &url, std::string *secret) {
    return tester.get(Secret_type::PASSWORD, url, secret);
  }

  bool erase(const std::string &url) {
    return tester.erase(Secret_type::PASSWORD, url);
  }

  void expect_list(const std::set<std::string> &urls) {
    SCOPED_TRACE("Parametrized_helper_test::expect_list()");
    std::vector<Secret_spec> specs;
    EXPECT_TRUE(tester.list(&specs));
    expect_no_error();

    std::set<std::string> list_urls;

    for (const auto &s : specs) {
      SCOPED_TRACE(s.url + " -> " + to_string(s.type));
      EXPECT_EQ(Secret_type::PASSWORD, s.type);
      list_urls.emplace(s.url);
    }

    EXPECT_EQ(urls, list_urls);
  }

  void expect_contains(const std::string &url, const std::string &secret) {
    tester.expect_contains(Secret_type::PASSWORD, url, secret);
  }

  void expect_not_contain(const std::string &url) {
    tester.expect_not_contain(Secret_type::PASSWORD, url);
  }

  void expect_error(const std::string &msg) { tester.expect_error(msg); }

  void expect_no_error() { tester.expect_no_error(); }

  void clear_store() { tester.clear_store(); }

  T tester;
};

class Config_editor_invoker {
 public:
  void store(const std::string &name, const std::string &url,
             const std::string &secret) {
    std::vector<std::string> args = {"set", "--skip-warn", "--password"};
    args.emplace_back("--login-path=" + name);

    mysqlsh::Connection_options options{url};

    if (options.has_user()) {
      args.emplace_back("--user=" + options.get_user());
    }
    if (options.has_host()) {
      args.emplace_back("--host=" + options.get_host());
    }
    if (options.has_port()) {
      args.emplace_back("--port=" + std::to_string(options.get_port()));
    }
    if (options.has_socket()) {
      args.emplace_back("--socket=" + options.get_socket());
    }

    invoke(args, true, secret);
  }

  void clear() { invoke({"reset"}); }

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
#else   // ! __APPLE__
  expected.emplace("login-path");
#endif  // ! __APPLE__
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

  verify_available_helpers(T::list_helpers());
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

#define GET_EXPECT_ERROR(url, secret, error) \
  if (tester.has_get()) {                    \
    std::string s;                           \
    MY_EXPECT_ERROR(get(url, &s), error);    \
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

#define STORE_AND_CHECK(url, secret)                  \
  do {                                                \
    SCOPED_TRACE(url + std::string{" -> "} + secret); \
    MY_EXPECT_NO_ERROR(store(url, secret));           \
    expect_contains(url, secret);                     \
  } while (false)

#define STORE_AND_ERASE(url, secret)                  \
  do {                                                \
    SCOPED_TRACE(url + std::string{" -> "} + secret); \
    MY_EXPECT_NO_ERROR(store(url, secret));           \
    expect_contains(url, secret);                     \
    MY_EXPECT_NO_ERROR(erase(url));                   \
    expect_not_contain(url);                          \
  } while (false)

#define STORE_AND_OP_TESTS(op)                                              \
  do {                                                                      \
    op("empty@host", "");                                                   \
    op("user@host", "one");                                                 \
    op("user@host:3306", "two");                                            \
    op("user@/socket", "three");                                            \
    op("user@host:33060", "five");                                          \
    op("user@host:33060", "six");                                           \
    if (GetParam() == "login-path") {                                       \
      op("user@host:55555",                                                 \
         "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"   \
         "abcdefghijklmn");                                                 \
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
    std::set<std::string> urls{"user@host", "user@host:3306",                 \
                               "user@host:33060", "user@/socket"};            \
    for (const auto &url : urls) {                                            \
      STORE_AND_CHECK(url, url);                                              \
    }                                                                         \
    expect_list(urls);                                                        \
    STORE_AND_CHECK(*urls.begin(), "new pass");                               \
    expect_list(urls);                                                        \
  }                                                                           \
                                                                              \
  TEST_P(test_case_name, get_nonexistent_url) {                               \
    constexpr auto error = "Could not find the secret";                       \
    GET_EXPECT_ERROR("user@nonexistent.host", "secret", error);               \
  }                                                                           \
                                                                              \
  TEST_P(test_case_name, erase_nonexistent_url) {                             \
    constexpr auto error = "Could not find the secret";                       \
    MY_EXPECT_ERROR(erase("user@nonexistent.host"), error);                   \
  }                                                                           \
                                                                              \
  NORMALIZATION_TEST(test_case_name, url_normalization) {                     \
    STORE_AND_CHECK("user@/socket", "one");                                   \
    STORE_AND_CHECK("user@(/socket)", "two");                                 \
    expect_contains("user@/socket", "two");                                   \
  }                                                                           \
                                                                              \
  TEST(Helpers, test_case_name##_list_helpers) {                              \
    test_available_helpers<test_case_name>();                                 \
  }                                                                           \
                                                                              \
  TEST_P(test_case_name, login_path_external_entry) {                         \
    if (GetParam() != "login-path") {                                         \
      return;                                                                 \
    }                                                                         \
    Config_editor_invoker invoker;                                            \
    invoker.clear();                                                          \
    std::set<std::string> urls{"user@host", "user@host:3306",                 \
                               "user@host:33060", "user@/socket"};            \
    for (const auto &url : urls) {                                            \
      invoker.store(url, url, url);                                           \
      expect_contains(url, url);                                              \
    }                                                                         \
    expect_list(urls);                                                        \
    invoker.clear();                                                          \
  }                                                                           \
                                                                              \
  TEST_P(test_case_name, login_path_external_and_helper_entries) {            \
    if (GetParam() != "login-path") {                                         \
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
  TEST_P(test_case_name, login_path_external_entry_with_port_and_socket) {    \
    if (GetParam() != "login-path") {                                         \
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
    expect_list({"user@localhost:3306", "user@/socket"});                     \
    STORE_AND_CHECK("user@localhost:3306", "two");                            \
    STORE_AND_CHECK("user@/socket", "three");                                 \
    expect_list({"user@localhost:3306", "user@/socket"});                     \
    erase("user@/socket");                                                    \
    expect_contains("user@/socket", "one");                                   \
    expect_contains("user@localhost:3306", "two");                            \
    expect_list({"user@localhost:3306", "user@/socket"});                     \
    erase("user@localhost:3306");                                             \
    expect_contains("user@/socket", "one");                                   \
    expect_contains("user@localhost:3306", "one");                            \
    expect_list({"user@localhost:3306", "user@/socket"});                     \
    erase("user@/socket");                                                    \
    expect_contains("user@localhost:3306", "one");                            \
    expect_list({"user@localhost:3306"});                                     \
    erase("user@localhost:3306");                                             \
    expect_list({});                                                          \
    invoker.clear();                                                          \
  }                                                                           \
                                                                              \
  TEST_P(test_case_name, login_path_ipv6_stored_by_config_editor) {           \
    if (GetParam() != "login-path") {                                         \
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
  TEST_P(test_case_name, login_path_ipv6_stored_by_helper) {                  \
    if (GetParam() != "login-path") {                                         \
      return;                                                                 \
    }                                                                         \
    Config_editor_invoker invoker;                                            \
    invoker.clear();                                                          \
    STORE_AND_CHECK("user@[::1]", "one");                                     \
    auto output = invoker.invoke({"print", "--all"});                         \
    EXPECT_THAT(output, ::testing::HasSubstr("host = ::1"));                  \
    invoker.clear();                                                          \
    STORE_AND_CHECK("user@[fe80::850a:5a7c:6ab7:aec4]:4321", "two");          \
    output = invoker.invoke({"print", "--all"});                              \
    EXPECT_THAT(output,                                                       \
                ::testing::HasSubstr("host = fe80::850a:5a7c:6ab7:aec4"));    \
    EXPECT_THAT(output, ::testing::HasSubstr("port = 4321"));                 \
    invoker.clear();                                                          \
    STORE_AND_CHECK("user@[fe80::850a:5a7c:6ab7:aec4%25enp0s3]", "three");    \
    output = invoker.invoke({"print", "--all"});                              \
    EXPECT_THAT(output, ::testing::HasSubstr(                                 \
                            "host = fe80::850a:5a7c:6ab7:aec4%enp0s3"));      \
    invoker.clear();                                                          \
  }

#define REGISTER_TESTS(test_case_name)                               \
  namespace {                                                        \
  const auto test_case_name##_list = test_case_name::list_helpers(); \
  }                                                                  \
  INSTANTIATE_TEST_CASE_P(Helpers, test_case_name,                   \
                          ::testing::ValuesIn(test_case_name##_list))

}  // namespace

#define VALIDATION_TEST TEST_P
#define NORMALIZATION_TEST TEST_P

class Mysql_secret_store_api_test
    : public Parametrized_helper_test<Mysql_secret_store_api_tester> {};

ADD_TESTS(Mysql_secret_store_api_test);

#undef VALIDATION_TEST
#undef NORMALIZATION_TEST

TEST_P(Mysql_secret_store_api_test, helper_name) {
  EXPECT_EQ(GetParam(), tester.name());
}

TEST_P(Mysql_secret_store_api_test, get_nullptr) {
  EXPECT_FALSE(get("url", nullptr));
  expect_error("Invalid pointer");
}

TEST_P(Mysql_secret_store_api_test, list_nullptr) {
  EXPECT_FALSE(tester.list(nullptr));
  expect_error("Invalid pointer");
}

TEST(Helpers, Mysql_secret_store_api_test_secret_spec_operators) {
  Secret_spec one{Secret_type::PASSWORD, "first URL"};
  Secret_spec two{Secret_type::PASSWORD, "second URL"};

  EXPECT_FALSE(one == two);
  EXPECT_TRUE(one != two);

  one.url = two.url;

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

REGISTER_TESTS(Mysql_secret_store_api_test);

#define VALIDATION_TEST TEST_P
#define NORMALIZATION_TEST TEST_P

class Shell_api_test : public Parametrized_helper_test<Shell_api_tester> {};

ADD_TESTS(Shell_api_test);

#undef VALIDATION_TEST
#undef NORMALIZATION_TEST

TEST_P(Shell_api_test, delete_all_credentials) {
  std::set<std::string> urls{"user@host", "user@host:3306", "user@host:33060",
                             "user@/socket"};
  for (const auto &url : urls) {
    STORE_AND_CHECK(url, url);
  }
  tester.expect_delete_all_credentials();
  expect_list({});
}

REGISTER_TESTS(Shell_api_test);

#define VALIDATION_TEST IGNORE_TEST
#define NORMALIZATION_TEST IGNORE_TEST

class Helper_executable_test
    : public Parametrized_helper_test<Helper_executable_tester> {};

ADD_TESTS(Helper_executable_test);

#undef VALIDATION_TEST
#undef NORMALIZATION_TEST

TEST_P(Helper_executable_test, uri_should_not_be_validated) {
  std::string output;
  std::string spec = R"("ServerURL":"some+string","SecretType":"password")";
  auto &invoker = tester.get_invoker();

  EXPECT_TRUE(invoker.store("{" + spec + R"(,"Secret":"pass"})", &output));
  EXPECT_EQ("", output);
  output.clear();

  EXPECT_TRUE(invoker.get("{" + spec + "}", &output));
  EXPECT_NE("", output);
  output.clear();

  EXPECT_TRUE(invoker.erase("{" + spec + "}", &output));
  EXPECT_EQ("", output);
  output.clear();
}

TEST_P(Helper_executable_test, secret_type_should_not_be_validated) {
  if (GetParam() == "login-path") {
    // login-path can only store passwords
    return;
  }
  std::string output;
  std::string spec = R"("ServerURL":"user@host","SecretType":"some_type")";
  auto &invoker = tester.get_invoker();

  EXPECT_TRUE(invoker.store("{" + spec + R"(,"Secret":"pass"})", &output));
  EXPECT_EQ("", output);
  output.clear();

  EXPECT_TRUE(invoker.get("{" + spec + "}", &output));
  EXPECT_NE("", output);
  output.clear();

  EXPECT_TRUE(invoker.erase("{" + spec + "}", &output));
  EXPECT_EQ("", output);
  output.clear();
}

TEST_P(Helper_executable_test, invalid_command) {
  std::string output;
  std::string spec = R"({"ServerURL":"user@host","SecretType":"password"})";
  auto &invoker = tester.get_invoker();

  EXPECT_FALSE(invoker.invoke("unknown", spec, &output));
  EXPECT_THAT(output, ::testing::HasSubstr("Unknown command"));
}

TEST_P(Helper_executable_test, missing_command) {
  std::string output;
  std::string spec = R"({"ServerURL":"user@host","SecretType":"password"})";
  auto &invoker = tester.get_invoker();

  EXPECT_FALSE(invoker.invoke(nullptr, spec, &output));
  EXPECT_THAT(output, ::testing::HasSubstr("Missing command"));
}

TEST_P(Helper_executable_test, invalid_json_input_misspelled_server_url) {
  const std::string error_message = R"("ServerURL" is missing)";
  auto &invoker = tester.get_invoker();
  std::string output;

  for (const auto &s : {"ServerURl", "SErverURL", "ServerURLL", "SrverURL"}) {
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
        R"("ServerURL":"user@host",")" + std::string{s} + R"(":"password")";

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

    std::string spec = R"({"ServerURL":"user@host","SecretType":"password",")" +
                       std::string{s} + R"(":"pass"})";

    EXPECT_FALSE(invoker.store(spec, &output));
    EXPECT_THAT(output, ::testing::HasSubstr(error_message));
    output.clear();
  }
}

TEST_P(Helper_executable_test, invalid_json_input_unknown_member) {
  const std::string error_message = "JSON object contains unknown member";
  auto &invoker = tester.get_invoker();
  std::string output;
  std::string spec =
      R"("ServerURL":"user@host","SecretType":"password","InvalidMember":12345)";

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

TEST_P(Helper_executable_test, invalid_json_input_missing_server_url) {
  const std::string error_message = R"("ServerURL" is missing)";
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
  std::string spec = R"("ServerURL":"user@host")";

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
  std::string spec = R"({"ServerURL":"user@host","SecretType":"password"})";

  EXPECT_FALSE(invoker.store(spec, &output));
  EXPECT_THAT(output, ::testing::HasSubstr(error_message));
  output.clear();
}

TEST_P(Helper_executable_test, invalid_json_input_wrong_type_server_url) {
  const std::string error_message = R"("ServerURL" should be a string)";
  auto &invoker = tester.get_invoker();
  std::string output;
  std::string spec = R"("ServerURL":false,"SecretType":"password")";

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
  std::string spec = R"("ServerURL":"user@host","SecretType":false)";

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
      R"({"ServerURL":"user@host","SecretType":"password","Secret":false})";

  EXPECT_FALSE(invoker.store(spec, &output));
  EXPECT_THAT(output, ::testing::HasSubstr(error_message));
  output.clear();
}

REGISTER_TESTS(Helper_executable_test);

}  // namespace tests
