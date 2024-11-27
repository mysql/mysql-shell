/*
 * Copyright (c) 2017, 2024, Oracle and/or its affiliates.
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

#include <gtest/gtest_prod.h>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include "mysqlsh/cmdline_shell.h"
#include "mysqlshdk/libs/utils/debug.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "unittest/gtest_clean.h"
#include "unittest/test_utils.h"
#include "unittest/test_utils/mocks/gmock_clean.h"

#include "modules/devapi/mod_mysqlx_schema.h"
#include "modules/devapi/mod_mysqlx_session.h"

namespace mysqlsh {

class Completer_frontend : public Shell_core_test_wrapper {
  struct linenoiseCompletions {
    std::vector<std::string> completionStrings;
  };

  // mostly matches cmdline_shell.cc:auto_complete()
  void completionCallback(const std::string &text, int *start_index,
                          linenoiseCompletions *completions) {
    size_t completion_offset = *start_index;
    std::vector<std::string> options(_interactive_shell->completer()->complete(
        _interactive_shell->shell_context()->interactive_mode(), {}, text,
        &completion_offset));

    std::sort(options.begin(), options.end(),
              shcore::Case_insensitive_comparator{});
    auto last = std::unique(options.begin(), options.end());
    options.erase(last, options.end());

    *start_index = completion_offset;
    // std::cout << "Complete '" << text << "'\n";
    for (auto &i : options) {
      // std::cout << i << "\n";
      completions->completionStrings.push_back(i);
    }
  }

  // mostly matches linenoise.cpp:completeLine()
  const char *breakChars = " =+-/\\*?\"'`&<>;|@{([])}";

  // just stubs to let completeLine() compile
  void beep() {}
  void freeCompletions(linenoiseCompletions *) {}
  using Utf8String = std::string;
  int buflen = 1024;

  void copyString32(std::string &buf32,  // NOLINT
                    const std::string &displayText, size_t len) {
    buf32 = displayText.substr(0, len);
  }

  int completeLine(std::string &buf32, int &pos,  // NOLINT
                   linenoiseCompletions *out_lc) {
    linenoiseCompletions lc;
    int len = static_cast<int>(buf32.size());

    // completionCallback() expects a parsable entity, so find the previous
    // break character and extract a copy to parse.  we also handle the case
    // where tab is hit while not at end-of-line.
    int startIndex = pos;
    while (--startIndex >= 0) {
      if (strchr(breakChars, buf32[startIndex])) {
        break;
      }
    }
    ++startIndex;
    int itemLength = pos - startIndex;
    Utf8String bufferContents(buf32, 0, pos);

    // get a list of completions and the position to start completing from
    completionCallback(bufferContents, &startIndex, &lc);
    itemLength = pos - startIndex;
    // if no completions, we are done
    if (lc.completionStrings.size() == 0) {
      beep();
      freeCompletions(&lc);
      return 0;
    }

    // at least one completion
    int longestCommonPrefix = 0;
    int displayLength = 0;
    if (lc.completionStrings.size() == 1) {
      longestCommonPrefix = static_cast<int>(lc.completionStrings[0].length());
    } else {
      bool keepGoing = true;
      while (keepGoing) {
        for (size_t j = 0; j < lc.completionStrings.size() - 1; ++j) {
          char c1 = lc.completionStrings[j][longestCommonPrefix];
          char c2 = lc.completionStrings[j + 1][longestCommonPrefix];
          if ((0 == c1) || (0 == c2) || (c1 != c2)) {
            keepGoing = false;
            break;
          }
        }
        if (keepGoing) {
          ++longestCommonPrefix;
        }
      }
    }

    // if we can extend the item, extend it and return to main loop
    if (longestCommonPrefix > itemLength) {
      displayLength = len + longestCommonPrefix - itemLength;
      if (displayLength > buflen) {
        longestCommonPrefix -= displayLength - buflen;  // don't overflow buffer
        displayLength = buflen;  // truncate the insertion
        beep();                  // and make a noise
      }
      Utf8String displayText;
      displayText.resize(displayLength + 1);
      memcpy(&displayText[0], &buf32[0], startIndex);
      memcpy(&displayText[startIndex], &lc.completionStrings[0][0],
             longestCommonPrefix);
      int tailIndex = startIndex + longestCommonPrefix;
      memcpy(&displayText[tailIndex], &buf32[pos],
             (displayLength - tailIndex + 1));

      copyString32(buf32, displayText, displayLength);
      pos = startIndex + longestCommonPrefix;
      len = displayLength;
      *out_lc = lc;
      return 0;
    }
    if (lc.completionStrings.size() != 1) {  // beep if ambiguous
      beep();
      *out_lc = lc;
      return 1;
    }
    return 0;
  }

 public:
  void SetUp() override {
    Shell_core_test_wrapper::SetUp();
    output_handler.set_errors_to_stderr(true);
  }

  void SetUpOnce() override {
    run_script_classic(
        {"create schema if not exists zzz;",
         "create schema if not exists zombie;",
         "create schema if not exists zoo;", "drop schema if exists actest;",
         "create schema actest;", "use actest;",
         "create table productTable (id int, name varchar(20));",
         "create table creature (a int);", "create table croissant (a int);",
         "create table tab_le(col_umn int);",
         "create view vi_ew as select * from tab_le;"});

    // Explicit create collections for 5.7
    {
      auto x = mysqlsh::mysqlx::Session();
      x.connect(mysqlshdk::db::Connection_options(shell_test_server_uri('x')));

      shcore::Dictionary_t options = shcore::make_dict();
      options->set("schema", shcore::Value("actest"));
      options->set("name", shcore::Value("people"));
      x.execute_mysqlx_stmt("create_collection", options);

      options->set("name", shcore::Value("person"));
      x.execute_mysqlx_stmt("create_collection", options);
    }
  }

  static void TearDownTestCase() {
    run_script_classic({"drop schema actest", "drop schema zzz;",
                        "drop schema zombie;", "drop schema zoo;"});
  }

  void reset_shell() override {
    get_options();
    _options->interactive = true;
    _options->db_name_cache = true;

    replace_shell(get_options(),
                  std::unique_ptr<shcore::Interpreter_delegate>(
                      new shcore::Interpreter_delegate(output_handler.deleg)));

    execute("\\py");
    execute("import sys");
    // delete some Python global vars that are leftover from other tests
    execute("del globals()['db1']");
    execute("del globals()['db2']");
    wipe_all();
    execute("sys.version.split(' ')[0]");
    m_python_version = output_handler.std_out;
  }
  std::string m_python_version;

 public:
  std::pair<std::string, std::vector<std::string>> complete(
      const std::string &line) {
    // refresh the prompt, which triggers auto-complete refresh
    _interactive_shell->prompt();

    linenoiseCompletions lc;

    std::string completed;
    int position = line.length();
    auto tab = line.find('\t');
    if (tab != std::string::npos) {
      position = tab;
      completed = line.substr(0, tab) + line.substr(tab + 1);
    } else {
      completed = line;
    }

    completeLine(completed, position, &lc);

    return {completed, lc.completionStrings};
  }

  bool is_function(const std::string &member) {
    wipe_all();
    if (_interactive_shell->shell_context()->interactive_mode() ==
        shcore::IShell_core::Mode::Python) {
      execute_noerr("callable(checkvar." + member + ")");
      if (output_handler.std_out == "True\n") return true;
      assert(output_handler.std_out == "False\n");
      return false;
    } else {
      execute_noerr(
          "['Function', 'm.Function']."
          "indexOf(type(checkvar." +
          member + ")) >= 0");
      if (output_handler.std_out == "true\n") return true;
      assert(output_handler.std_out == "false\n");
      return false;
    }
  }

  void check_object_member_completions(
      const std::string &expr,
      std::vector<std::pair<std::string, std::string>> method_args) {
    {
      SCOPED_TRACE("Autocompletion table completeness for result of " + expr);
      check_object_completions(expr);
    }
    // method_args is a map of method_name -> method_arg_string
    // so that evaluating method_name + method_arg_string will return a valid
    // object to be tested

    wipe_all();
    // first evaluate the expression and assign to a temporary variable
    execute_noerr("checkvar = " + expr + ";");

    // first evaluate and list members of the object
    std::vector<std::string> completions(complete("checkvar.").second);

    // remove methods unsupported by 5.7
    if (_target_server_version < mysqlshdk::utils::Version("8.0")) {
      method_args.erase(
          remove_if(method_args.begin(), method_args.end(),
                    [](const auto &other) {
                      return other.first.compare("replaceOne") == 0 ||
                             other.first.compare("createIndex") == 0;
                    }),
          method_args.end());
      completions.erase(remove_if(completions.begin(), completions.end(),
                                  [](const auto &other) {
                                    return other.compare("replaceOne()") == 0 ||
                                           other.compare("createIndex()") == 0;
                                  }),
                        completions.end());
    }

    if (_target_server_version < mysqlshdk::utils::Version("8.0.19")) {
      method_args.erase(remove_if(method_args.begin(), method_args.end(),
                                  [](const auto &other) {
                                    return other.first == "modifyCollection";
                                  }),
                        method_args.end());
      completions.erase(remove_if(completions.begin(), completions.end(),
                                  [](const auto &other) {
                                    return other == "modifyCollection()";
                                  }),
                        completions.end());
    }

    // first execute the methods with special handling given by the test
    for (const auto &method : method_args) {
      if (!method.second.empty()) {
        std::string member_call = method.first + method.second;
        SCOPED_TRACE("Autocompletion table completeness for result of " + expr +
                     "." + member_call);
        check_object_completions("checkvar." + member_call);
      }
      size_t osize = completions.size();
      completions.erase(std::remove_if(completions.begin(), completions.end(),
                                       [&method](const std::string &m) {
                                         return (m == method.first + "()" ||
                                                 m == method.first);
                                       }),
                        completions.end());
      if (osize == completions.size()) {
        FAIL() << "Expected member " << method.first
               << " not found in completion list for " << expr << "\n"
               << "\t" << shcore::str_join(completions, "\n\t");
      }
    }

    // then for each member, check their completions with the provided args
    for (const auto &member : completions) {
      size_t p;
      if ((p = member.find('(')) != std::string::npos) {
        // completion thinks this is a method, confirm
        EXPECT_TRUE(is_function(member.substr(0, p)));
      } else {
        // completion thinks this is not a method, confirm
        EXPECT_FALSE(is_function(member));
      }
      {
        SCOPED_TRACE("Autocompletion table completeness for result of " + expr +
                     "." + member);
        check_object_completions("checkvar." + member);
      }
    }
  }

  void check_object_completions(const std::string &expr) {
    // list members
    std::vector<std::string> completions(complete(expr + ".").second);
    for (auto &s : completions) {
      // strip ()
      auto pos = s.find('(');
      if (pos != std::string::npos) s = s.substr(0, pos);
    }

    wipe_all();
    // evaluate the expression and compare with the auto-completion list
    if (_interactive_shell->shell_context()->interactive_mode() ==
        shcore::IShell_core::Mode::Python) {
      execute_noerr("tmpvar = " + expr);
      wipe_all();
      execute_noerr("'shell.Object' in str(type(tmpvar))");
      shcore::Value result = shcore::Value::parse(output_handler.std_out);
      if (result.descr() == "False") return;
    } else {
      ASSERT_EQ("", output_handler.std_err);
      execute("var tmpvar = " + expr);
      // Only deprecation errors are bypassed on the function calls
      if (!output_handler.std_err.empty()) {
        if (output_handler.std_err.find("is deprecated") == std::string::npos) {
          FAIL() << "Only deprecation errors are allowed on autocompletion "
                    "tests, found: "
                 << output_handler.std_err.c_str() << "\n";
        }
      }
      wipe_all();

      execute_noerr(
          "['m.Array', 'm.Map', 'String', 'Integer', "
          "'Boolean'].indexOf(type(tmpvar)) >= 0");
      shcore::Value result = shcore::Value::parse(output_handler.std_out);
      // don't need to check non-object types
      if (result.as_bool()) return;
    }

    wipe_all();
    execute("dir(tmpvar)");
    ASSERT_EQ("", output_handler.std_err);

    shcore::Value result;
    try {
      result = shcore::Value::parse(output_handler.std_out);
    } catch (std::exception &e) {
      if (output_handler.std_out.empty())
        ADD_FAILURE() << "Evaluation of " << expr << " produced error:\n "
                      << output_handler.std_err << "\n";
      else
        ADD_FAILURE() << "Evaluation of " << expr << " produced "
                      << output_handler.std_out << ": " << e.what() << "\n"
                      << output_handler.std_err << "\n";
      return;
    }

    std::vector<std::string> expected;
    for (const shcore::Value &value : *result.as_array()) {
      // skip __methods from python
      if (shcore::str_beginswith(value.get_string(), "__")) continue;
      expected.push_back(value.get_string());
    }
    std::sort(expected.begin(), expected.end());
    std::sort(completions.begin(), completions.end());

    if (expected != completions) {
      ADD_FAILURE() << "Completion list mismatch for '" << expr << "':\n"
                    << "Actual (completer): \n\t"
                    << shcore::str_join(completions, "\n\t") << "\n"
                    << "Should be (dir(obj)): \n\t"
                    << shcore::str_join(expected, "\n\t") << "\n";
    }
  }
};

#ifdef _WIN32
#define PRODUCTTABLE "producttable"
#define DB_PRODUCTTABLE "db.producttable"
#else
#define PRODUCTTABLE "productTable"
#define DB_PRODUCTTABLE "db.productTable"
#endif

using strv = std::vector<std::string>;

// TS_FR2_C01, TS_FR2_C02, TS_FR2_C03, TS_FR2_X01, TS_FR2_X02, TS_FR2_X03
// Check auto-completion on 1 tab
#define EXPECT_AFTER_TAB(text, expected_text)                         \
  do {                                                                \
    auto r = complete(text);                                          \
    EXPECT_EQ(shcore::str_replace(expected_text, "\t", ""), r.first); \
  } while (0)

// TS_FR3_C01, TS_FR3_C02, TS_FR3_X01, TS_FR3_X02
// Check completion alternatives on 2 tabs
#define EXPECT_AFTER_TAB_TAB(text, expected_options)         \
  do {                                                       \
    auto r = complete(text);                                 \
    SCOPED_TRACE(text);                                      \
    EXPECT_EQ(shcore::str_replace(text, "\t", ""), r.first); \
    EXPECT_EQ(shcore::str_join(expected_options, "\n"),      \
              shcore::str_join(r.second, "\n"));             \
  } while (0)

#define EXPECT_TAB_TAB_CONTAINS(text, expected_option)           \
  do {                                                           \
    auto r = complete(text);                                     \
    SCOPED_TRACE(text);                                          \
    EXPECT_THAT(r.second, ::testing::Contains(expected_option)); \
  } while (0)

#define EXPECT_TAB_TAB_NOT_CONTAINS(text, unexpected_option)             \
  do {                                                                   \
    auto r = complete(text);                                             \
    SCOPED_TRACE(text);                                                  \
    EXPECT_THAT(r.second,                                                \
                ::testing::Not(::testing::Contains(unexpected_option))); \
  } while (0)

#define EXPECT_TAB_DOES_NOTHING(text)                            \
  do {                                                           \
    EXPECT_AFTER_TAB(shcore::str_replace(text, "\t", ""), text); \
    EXPECT_AFTER_TAB_TAB(text, strv());                          \
  } while (0)

#define CHECK_OBJECT_COMPLETIONS(object) \
  do {                                   \
    SCOPED_TRACE(object);                \
    check_object_completions(object);    \
  } while (0)

#define CHECK_OBJECT_MEMBER_COMPLETIONS(object, method_args) \
  do {                                                       \
    SCOPED_TRACE(object);                                    \
    check_object_member_completions(object, method_args);    \
  } while (0)

// TS_FR6.1_C01
TEST_F(Completer_frontend, builtin_use_c) {
  connect_classic();
  // TS_FR7_C01
  EXPECT_TAB_DOES_NOTHING("");
  // TS_FR7_C02
  EXPECT_TAB_DOES_NOTHING(" ");

  EXPECT_AFTER_TAB("\\u", "\\use");
  EXPECT_AFTER_TAB("\\us", "\\use");

  EXPECT_AFTER_TAB("\\use i", "\\use information_schema");
  EXPECT_AFTER_TAB("\\use perf", "\\use performance_schema");

  EXPECT_AFTER_TAB("\\use bla", "\\use bla");
}

// TS_FR6_X01, TS_FR6.1_X01
TEST_F(Completer_frontend, builtin_use_x) {
  connect_x();
  // TS_FR7_X01
  EXPECT_TAB_DOES_NOTHING("");
  // TS_FR7_X02
  EXPECT_TAB_DOES_NOTHING(" ");

  EXPECT_AFTER_TAB("\\u", "\\use");
  EXPECT_AFTER_TAB("\\us", "\\use");

  EXPECT_AFTER_TAB("\\use i", "\\use information_schema");
  EXPECT_AFTER_TAB("\\use perf", "\\use performance_schema");

  EXPECT_AFTER_TAB("\\use bla", "\\use bla");
}

TEST_F(Completer_frontend, builtin_connect) {
  EXPECT_AFTER_TAB("\\co", "\\connect");
  // EXPECT_AFTER_TAB("\\connect root@l", "\\connect root@localhost");
}

// TS_FR6_C01,
TEST_F(Completer_frontend, builtin_others) {
  EXPECT_AFTER_TAB("\\h", "\\h");
  EXPECT_AFTER_TAB_TAB("\\h", strv({"\\help", "\\history"}));

  EXPECT_AFTER_TAB("\\hi", "\\history");
  EXPECT_AFTER_TAB("\\he", "\\help");

  auto expect = strv({"\\",        "\\connect",    "\\disconnect", "\\edit",
                      "\\exit",    "\\help",       "\\history",    "\\js",
                      "\\nopager", "\\nowarnings", "\\option",     "\\pager",
                      "\\py",      "\\quit",       "\\reconnect",  "\\rehash",
                      "\\show",    "\\source",     "\\sql",        "\\status",
                      "\\system",  "\\use",        "\\warnings",   "\\watch"});

#ifndef HAVE_JS
  expect.erase(std::find(expect.begin(), expect.end(), "\\js"));
#endif

  EXPECT_AFTER_TAB_TAB("\\", expect);
}

// FR4
TEST_F(Completer_frontend, sql_keywords) {
  execute("\\sql");

  EXPECT_AFTER_TAB("sel", "SELECT");
  EXPECT_AFTER_TAB("select * fr", "select * FROM");
  EXPECT_TAB_DOES_NOTHING("select * FROM");
}

TEST_F(Completer_frontend, sql_schema) {
  connect_classic();
  execute("\\sql");

  EXPECT_AFTER_TAB("show tables from info",
                   "show tables from information_schema");
}

TEST_F(Completer_frontend, sql_table) {
  connect_classic();
  execute("\\use mysql");
  execute("\\sql");

  // table name from default schema
  EXPECT_AFTER_TAB("select * from plu", "select * from plugin");

  execute("\\use actest");

  EXPECT_TAB_DOES_NOTHING("select * from plu");

  EXPECT_AFTER_TAB("select * from peo", "select * from people");
  EXPECT_AFTER_TAB_TAB("select * from `z",
                       strv({"`zombie`", "`zoo`", "`zzz`"}));

  // identifier in `` should skip keywords
  EXPECT_AFTER_TAB("cr", "CREATE");
  EXPECT_AFTER_TAB("`cr", "`cr");
  EXPECT_TAB_DOES_NOTHING("`cr");
}

// try different orders of mode switching
TEST_F(Completer_frontend, sql_table_o1) {
  connect_classic();
  execute("\\sql");
  execute("\\use performance_schema");

  // table name from default schema
  EXPECT_AFTER_TAB("select * from rw", "select * from rwlock_instances");

  execute("\\use mysql");
  EXPECT_AFTER_TAB("describe `pl", "describe `plugin`");
}

TEST_F(Completer_frontend, sql_table_o2) {
  connect_classic();
  execute("\\use performance_schema");
  execute("\\sql");

  // table name from default schema
  EXPECT_AFTER_TAB("select * from rw", "select * from rwlock_instances");

  execute("\\use mysql");
  EXPECT_AFTER_TAB("describe `pl", "describe `plugin`");
}

TEST_F(Completer_frontend, sql_table_o3) {
  execute("\\sql");
  connect_classic();
  execute("\\use performance_schema");

  // table name from default schema
  EXPECT_AFTER_TAB("select * from rw", "select * from rwlock_instances");

  execute("\\use mysql");
  EXPECT_AFTER_TAB("describe `pl", "describe `plugin`");
}

#ifdef HAVE_JS
TEST_F(Completer_frontend, js_keywords) {
  execute("\\js");

  EXPECT_AFTER_TAB("pri", "print");
  EXPECT_AFTER_TAB("x=pri", "x=print");
  EXPECT_AFTER_TAB("var x =pri", "var x =print");
  EXPECT_AFTER_TAB("println(sh", "println(shell");
  EXPECT_AFTER_TAB("println(sh\t)", "println(shell)");
  EXPECT_AFTER_TAB("wh", "while");

  EXPECT_AFTER_TAB("while (sh", "while (shell");
  EXPECT_AFTER_TAB("while (sh\t)", "while (shell)");
  EXPECT_AFTER_TAB("while 1: sh", "while 1: shell");
  EXPECT_AFTER_TAB("while 1: if (sh", "while 1: if (shell");
  EXPECT_AFTER_TAB("while 1: /* bla '*/ if (sh",
                   "while 1: /* bla '*/ if (shell");
  EXPECT_AFTER_TAB("while 1: /* bla \"*/ if (sh",
                   "while 1: /* bla \"*/ if (shell");
  EXPECT_AFTER_TAB("while 1: /* bla /**/ if (sh",
                   "while 1: /* bla /**/ if (shell");
  EXPECT_AFTER_TAB("while 1: /* bla (*/ if (sh",
                   "while 1: /* bla (*/ if (shell");
  EXPECT_AFTER_TAB("while 1: /* bla {*/ if (sh",
                   "while 1: /* bla {*/ if (shell");
  EXPECT_AFTER_TAB("while 1: /* bla }*/ if (sh",
                   "while 1: /* bla }*/ if (shell");
  EXPECT_TAB_DOES_NOTHING("print('");
  EXPECT_TAB_DOES_NOTHING("print(\"");
  EXPECT_TAB_DOES_NOTHING("print('pri");
  EXPECT_TAB_DOES_NOTHING("print(\"pri");
  EXPECT_TAB_DOES_NOTHING("var x=px");
  EXPECT_TAB_DOES_NOTHING("x = px");
  EXPECT_TAB_DOES_NOTHING(".");
  EXPECT_TAB_DOES_NOTHING("..");
  EXPECT_TAB_DOES_NOTHING(".s");
  EXPECT_TAB_DOES_NOTHING("(.s");
  EXPECT_TAB_DOES_NOTHING("(.re");
  EXPECT_TAB_DOES_NOTHING("shell..");
  EXPECT_TAB_DOES_NOTHING("*");

  // Session stuff while not connected
  EXPECT_AFTER_TAB_TAB("session.", strv({}));
  EXPECT_TAB_DOES_NOTHING("db.");
}

TEST_F(Completer_frontend, js_negative) {
  execute("\\js");
  EXPECT_AFTER_TAB("bogus", "bogus");
  EXPECT_AFTER_TAB_TAB("bogus", strv({}));
  EXPECT_AFTER_TAB("bogus.", "bogus.");
  EXPECT_AFTER_TAB_TAB("bogus.", strv({}));
  EXPECT_AFTER_TAB("print(\t)", "print()");
  EXPECT_AFTER_TAB("print('pr\t", "print('pr");
  EXPECT_AFTER_TAB("print('pr\t')", "print('pr')");
}

TEST_F(Completer_frontend, js_shell) {
  execute("\\js");

  CHECK_OBJECT_COMPLETIONS("shell");

  EXPECT_AFTER_TAB("sh", "shell");
  EXPECT_AFTER_TAB("shell.rec", "shell.reconnect()");
  EXPECT_AFTER_TAB_TAB("shell.", strv({"addExtensionObjectMember()",
                                       "autoCompleteSql()",
                                       "connect()",
                                       "connectToPrimary()",
                                       "createContext()",
                                       "createExtensionObject()",
                                       "deleteAllCredentials()",
                                       "deleteCredential()",
                                       "disablePager()",
                                       "disconnect()",
                                       "dumpRows()",
                                       "enablePager()",
                                       "getSession()",
                                       "help()",
                                       "listCredentialHelpers()",
                                       "listCredentials()",
                                       "listSshConnections()",
                                       "log()",
                                       "openSession()",
                                       "options",
                                       "parseUri()",
                                       "prompt()",
                                       "reconnect()",
                                       "registerGlobal()",
                                       "registerReport()",
                                       "reports",
                                       "setCurrentSchema()",
                                       "setSession()",
                                       "status()",
                                       "storeCredential()",
                                       "unparseUri()",
                                       "version"}));

  EXPECT_TAB_DOES_NOTHING("shell.conect()");

  EXPECT_AFTER_TAB_TAB("mysql", strv({"mysql", "mysqlx"}));
  EXPECT_AFTER_TAB("mysql.getC", "mysql.getClassicSession()");
  EXPECT_AFTER_TAB("mysqlx.get", "mysqlx.getSession()");

  // a dynamically created global var
  execute("var testobj = {'key_one':1, 'another_key': 2}");
  EXPECT_AFTER_TAB("testo", "testobj");
  EXPECT_AFTER_TAB_TAB("testobj.", strv({"another_key", "key_one"}));
  EXPECT_AFTER_TAB("testobj.k", "testobj.key_one");
  EXPECT_TAB_DOES_NOTHING("testobj.key_one");

  // TS_FR5.3_C01
  CHECK_OBJECT_COMPLETIONS("shell.options");
}

TEST_F(Completer_frontend, js_adminapi) {
  execute("\\js");

  CHECK_OBJECT_COMPLETIONS("dba");

  // TS_FR5.2_C02, TS_FR5.2_X02
  EXPECT_AFTER_TAB_TAB(
      "dba.",
      strv({"checkInstanceConfiguration()", "configureInstance()",
            "configureReplicaSetInstance()", "createCluster()",
            "createReplicaSet()", "deleteSandboxInstance()",
            "deploySandboxInstance()", "dropMetadataSchema()", "getCluster()",
            "getClusterSet()", "getReplicaSet()", "help()",
            "killSandboxInstance()", "rebootClusterFromCompleteOutage()",
            "session", "startSandboxInstance()", "stopSandboxInstance()",
            "upgradeMetadata()", "verbose"}));
  EXPECT_AFTER_TAB("dba.depl", "dba.deploySandboxInstance()");
}

// TS_FR8_X01
TEST_F(Completer_frontend, js_devapi) {
  connect_x();
  execute("\\js");

  CHECK_OBJECT_COMPLETIONS("mysqlx");
  CHECK_OBJECT_COMPLETIONS("session");

  EXPECT_AFTER_TAB("session.createS", "session.createSchema()");
  EXPECT_AFTER_TAB("session.st", "session.startTransaction()");
  EXPECT_AFTER_TAB_TAB(
      "session.get",
      strv({"getCurrentSchema()", "getDefaultSchema()", "getSchema()",
            "getSchemas()", "getSshUri()", "getUri()"}));

  EXPECT_AFTER_TAB("session.sql", "session.sql()");
  EXPECT_AFTER_TAB("session.sql('sele\t", "session.sql('sele");
  EXPECT_AFTER_TAB_TAB("session.sql('sele\t", strv({}));
  EXPECT_AFTER_TAB("session.sql('select 1')\t", "session.sql('select 1')");
  EXPECT_AFTER_TAB_TAB("session.sql('select 1')\t", strv({}));

  EXPECT_AFTER_TAB_TAB("session.sql('select 1').",
                       strv({"bind()", "execute()", "help()"}));
  EXPECT_AFTER_TAB("session.sql('select 1').b",
                   "session.sql('select 1').bind()");
  EXPECT_AFTER_TAB("session.sql('select 1').e",
                   "session.sql('select 1').execute()");

  EXPECT_AFTER_TAB_TAB("session.sql(\"select 1\").",
                       strv({"bind()", "execute()", "help()"}));
  EXPECT_AFTER_TAB("session.sql(\"select 1\").b",
                   "session.sql(\"select 1\").bind()");
  EXPECT_AFTER_TAB("session.sql(mkquery()).e",
                   "session.sql(mkquery()).execute()");
  EXPECT_AFTER_TAB("session.sql(mkquery(\"\")).e",
                   "session.sql(mkquery(\"\")).execute()");

  EXPECT_AFTER_TAB_TAB(
      "session.get",
      strv({"getCurrentSchema()", "getDefaultSchema()", "getSchema()",
            "getSchemas()", "getSshUri()", "getUri()"}));
  EXPECT_AFTER_TAB("session.getC", "session.getCurrentSchema()");

  // TS_FR5.1_X01
  EXPECT_AFTER_TAB_TAB("db", strv({"db", "dba"}));
  // TS_FR5.1_X02
  EXPECT_AFTER_TAB_TAB("mysql", strv({"mysql", "mysqlx"}));
  // TS_FR5.1_X03
  EXPECT_AFTER_TAB_TAB(
      "s", strv({"session", "shell", "source()", "super", "switch", "sys"}));
  // TS_FR5.1_X04
  EXPECT_AFTER_TAB("ses", "session");

  // no schema active
  EXPECT_TAB_DOES_NOTHING("db.p");

  execute("\\use actest");
  EXPECT_AFTER_TAB("db.p", "db.p");
  EXPECT_AFTER_TAB_TAB("db.p", strv({"people", "person", PRODUCTTABLE}));
  EXPECT_AFTER_TAB("db.peo", "db.people");
  EXPECT_AFTER_TAB("db.pr", DB_PRODUCTTABLE);
  EXPECT_AFTER_TAB("var p = db.peo", "var p = db.people");

  EXPECT_AFTER_TAB("db.getC", "db.getCollection");
  EXPECT_AFTER_TAB("db.getCollectionA", "db.getCollectionAsTable()");
  EXPECT_AFTER_TAB("db.getT", "db.getTable");

  // TS_FR5.2_X01
  CHECK_OBJECT_COMPLETIONS("db");

  execute("var actest = session.getSchema('actest')");
  EXPECT_AFTER_TAB("actest.createC", "actest.createCollection()");
}

// TS_FR5.4_X10, TS_FR5.4_X11, TS_FR5.4_X12, TS_FR5.4_X13, TS_FR5.4_X14
// TS_FR5.4_X15, TS_FR5.4_X16, TS_FR5.4_X17, TS_FR5.4_X18
TEST_F(Completer_frontend, js_devapi_collection) {
  connect_x();
  execute("\\js");

  execute("\\use actest");
  execute("session.startTransaction()");

  EXPECT_AFTER_TAB("db.peo", "db.people");
  EXPECT_AFTER_TAB("db.people.", "db.people.");
  EXPECT_AFTER_TAB_TAB(
      "db.people.",
      strv({"add()", "addOrReplaceOne()", "count()", "createIndex()",
            "dropIndex()", "existsInDatabase()", "find()", "getName()",
            "getOne()", "getSchema()", "getSession()", "help()", "modify()",
            "name", "remove()", "removeOne()", "replaceOne()", "schema",
            "session"}));

  EXPECT_TAB_DOES_NOTHING("db.people.x");
  EXPECT_TAB_DOES_NOTHING("db.people..");
  EXPECT_TAB_DOES_NOTHING("db.people.?");
  EXPECT_TAB_DOES_NOTHING("db.people.#");

  EXPECT_AFTER_TAB("db.people.f", "db.people.find()");
  EXPECT_AFTER_TAB("db.people.find().s", "db.people.find().sort()");
  EXPECT_AFTER_TAB("db.people.find('foo()').b",
                   "db.people.find('foo()').bind()");
  EXPECT_AFTER_TAB("db.people.find().bind().b",
                   "db.people.find().bind().bind()");
  EXPECT_AFTER_TAB("db.people.find().bind().e",
                   "db.people.find().bind().execute()");
  EXPECT_AFTER_TAB("db.people.find().gr", "db.people.find().groupBy()");
  EXPECT_AFTER_TAB("db.people.find().groupBy().ha",
                   "db.people.find().groupBy().having()");
  EXPECT_AFTER_TAB("db.people.find().execute().fe",
                   "db.people.find().execute().fetch");
  EXPECT_AFTER_TAB_TAB("db.people.find().execute().fetch",
                       strv({"fetchAll()", "fetchOne()"}));

  EXPECT_TAB_DOES_NOTHING("db.people.find().bind().s");
  EXPECT_TAB_DOES_NOTHING("db.people.find().bind(.s");
  EXPECT_TAB_DOES_NOTHING("db.people.find.ex");
  EXPECT_TAB_DOES_NOTHING("db.people.find(.ex");
  EXPECT_TAB_DOES_NOTHING("db.people.find).ex");
  EXPECT_TAB_DOES_NOTHING("db.people.find()");

  execute("var people = db.people");
  EXPECT_AFTER_TAB("people.", "people.");
  EXPECT_AFTER_TAB_TAB(
      "people.", strv({"add()", "addOrReplaceOne()", "count()", "createIndex()",
                       "dropIndex()", "existsInDatabase()", "find()",
                       "getName()", "getOne()", "getSchema()", "getSession()",
                       "help()", "modify()", "name", "remove()", "removeOne()",
                       "replaceOne()", "schema", "session"}));
  EXPECT_AFTER_TAB("people.f", "people.find()");
  EXPECT_AFTER_TAB("people.find().e", "people.find().execute()");
  EXPECT_AFTER_TAB("var f = people.find().b", "var f = people.find().bind()");
  execute("var findOp = db.people.find()");
  EXPECT_TAB_DOES_NOTHING("findOp.fin");
  EXPECT_AFTER_TAB("findOp.exe", "findOp.execute()");

  CHECK_OBJECT_COMPLETIONS("people");

  CHECK_OBJECT_COMPLETIONS("people.find()");
  CHECK_OBJECT_COMPLETIONS("people.find().fields(['x'])");
  CHECK_OBJECT_COMPLETIONS("people.find().sort(['x'])");
  CHECK_OBJECT_COMPLETIONS("people.find().groupBy(['x'])");
  CHECK_OBJECT_COMPLETIONS("people.find().groupBy(['x']).having('x')");
  CHECK_OBJECT_COMPLETIONS("people.find().limit(2)");
  CHECK_OBJECT_COMPLETIONS("people.find().lockShared()");
  CHECK_OBJECT_COMPLETIONS("people.find().lockExclusive()");
  CHECK_OBJECT_COMPLETIONS("people.find(':x').bind('x',1)");
  CHECK_OBJECT_COMPLETIONS("people.find().execute()");

  CHECK_OBJECT_COMPLETIONS("people.modify('1')");
  CHECK_OBJECT_COMPLETIONS("people.modify('1').set('x',1)");
  CHECK_OBJECT_COMPLETIONS("people.modify('1').unset('x')");
  CHECK_OBJECT_COMPLETIONS("people.modify(':x').unset('x').bind('x',1)");
  CHECK_OBJECT_COMPLETIONS("people.modify('1').unset('x').execute()");

  CHECK_OBJECT_COMPLETIONS("people.remove('1')");
  CHECK_OBJECT_COMPLETIONS("people.remove('1').sort(['x'])");
  CHECK_OBJECT_COMPLETIONS("people.remove(':x').bind('x',1)");
  CHECK_OBJECT_COMPLETIONS("people.remove('1').execute()");

  CHECK_OBJECT_COMPLETIONS("people.add({_id: '0001'})");
  CHECK_OBJECT_COMPLETIONS("people.add({_id: '0002'}).execute()");

  execute("session.rollback()");
}

// TS_FR5.4_X01, TS_FR5.4_X02, TS_FR5.4_X03, TS_FR5.4_X04, TS_FR5.4_X05,
// TS_FR5.4_X06, TS_FR5.4_X07, TS_FR5.4_X08, TS_FR5.4_X09
TEST_F(Completer_frontend, js_devapi_table) {
  connect_x();
  execute("\\js");
  execute("\\use actest");

  execute("session.startTransaction()");

  EXPECT_AFTER_TAB("db.pro", DB_PRODUCTTABLE);
  EXPECT_AFTER_TAB(DB_PRODUCTTABLE ".", DB_PRODUCTTABLE ".");
  EXPECT_AFTER_TAB_TAB(
      DB_PRODUCTTABLE ".",
      strv({"count()", "delete()", "existsInDatabase()", "getName()",
            "getSchema()", "getSession()", "help()", "insert()", "isView()",
            "name", "schema", "select()", "session", "update()"}));

  EXPECT_TAB_DOES_NOTHING(DB_PRODUCTTABLE ".x");
  EXPECT_TAB_DOES_NOTHING(DB_PRODUCTTABLE "..");
  EXPECT_TAB_DOES_NOTHING(DB_PRODUCTTABLE ".?");
  EXPECT_TAB_DOES_NOTHING(DB_PRODUCTTABLE ".#");

  EXPECT_AFTER_TAB(DB_PRODUCTTABLE ".sel", DB_PRODUCTTABLE ".select()");
  EXPECT_AFTER_TAB(DB_PRODUCTTABLE ".select().w",
                   DB_PRODUCTTABLE ".select().where()");
  EXPECT_AFTER_TAB(DB_PRODUCTTABLE ".select().o",
                   DB_PRODUCTTABLE ".select().orderBy()");
  EXPECT_AFTER_TAB(DB_PRODUCTTABLE ".select('foo()').b",
                   DB_PRODUCTTABLE ".select('foo()').bind()");
  EXPECT_AFTER_TAB(DB_PRODUCTTABLE ".select().bind().b",
                   DB_PRODUCTTABLE ".select().bind().bind()");
  EXPECT_AFTER_TAB(DB_PRODUCTTABLE ".select().bind().e",
                   DB_PRODUCTTABLE ".select().bind().execute()");
  EXPECT_AFTER_TAB(DB_PRODUCTTABLE ".select().gr",
                   DB_PRODUCTTABLE ".select().groupBy()");
  EXPECT_AFTER_TAB(DB_PRODUCTTABLE ".select().groupBy().ha",
                   DB_PRODUCTTABLE ".select().groupBy().having()");
  EXPECT_AFTER_TAB(DB_PRODUCTTABLE ".select().execute().fe",
                   DB_PRODUCTTABLE ".select().execute().fetch");
  EXPECT_AFTER_TAB_TAB(DB_PRODUCTTABLE ".select().execute().fetch",
                       strv({"fetchAll()", "fetchOne()", "fetchOneObject()"}));

  EXPECT_TAB_DOES_NOTHING(DB_PRODUCTTABLE ".select().bind().s");
  EXPECT_TAB_DOES_NOTHING(DB_PRODUCTTABLE ".select().bind(.s");
  EXPECT_TAB_DOES_NOTHING(DB_PRODUCTTABLE ".select.ex");
  EXPECT_TAB_DOES_NOTHING(DB_PRODUCTTABLE ".select(.ex");
  EXPECT_TAB_DOES_NOTHING(DB_PRODUCTTABLE ".select).ex");
  EXPECT_TAB_DOES_NOTHING(DB_PRODUCTTABLE ".select()");
  EXPECT_TAB_DOES_NOTHING(DB_PRODUCTTABLE ".select().hav");

  EXPECT_AFTER_TAB(DB_PRODUCTTABLE ".insert([]).v",
                   DB_PRODUCTTABLE ".insert([]).values()");

  execute("var " PRODUCTTABLE " = " DB_PRODUCTTABLE);
  EXPECT_AFTER_TAB(PRODUCTTABLE ".", PRODUCTTABLE ".");
  EXPECT_AFTER_TAB_TAB(
      PRODUCTTABLE ".",
      strv({"count()", "delete()", "existsInDatabase()", "getName()",
            "getSchema()", "getSession()", "help()", "insert()", "isView()",
            "name", "schema", "select()", "session", "update()"}));
  EXPECT_AFTER_TAB(PRODUCTTABLE ".sel", PRODUCTTABLE ".select()");
  EXPECT_AFTER_TAB(PRODUCTTABLE ".select([]).e",
                   PRODUCTTABLE ".select([]).execute()");
  EXPECT_AFTER_TAB("var f = " PRODUCTTABLE ".select([]).b",
                   "var f = " PRODUCTTABLE ".select([]).bind()");
  execute("var findOp = " DB_PRODUCTTABLE ".select();");

  EXPECT_TAB_DOES_NOTHING("findOp.sel");
  EXPECT_AFTER_TAB("findOp.exe", "findOp.execute()");

  CHECK_OBJECT_COMPLETIONS(PRODUCTTABLE);

  CHECK_OBJECT_COMPLETIONS(PRODUCTTABLE ".select(['x'])");
  CHECK_OBJECT_COMPLETIONS(PRODUCTTABLE ".select(['x']).where('x')");
  CHECK_OBJECT_COMPLETIONS(PRODUCTTABLE ".select(['x']).orderBy(['x'])");
  CHECK_OBJECT_COMPLETIONS(PRODUCTTABLE ".select(['x']).groupBy(['x'])");
  CHECK_OBJECT_COMPLETIONS(PRODUCTTABLE
                           ".select(['x']).groupBy(['x']).having('x')");
  CHECK_OBJECT_COMPLETIONS(PRODUCTTABLE ".select(['x']).limit(2)");
  CHECK_OBJECT_COMPLETIONS(PRODUCTTABLE ".select(['x']).limit(2).offset(2)");
  CHECK_OBJECT_COMPLETIONS(PRODUCTTABLE
                           ".select(['x']).where(':x').bind('x',1)");
  CHECK_OBJECT_COMPLETIONS(PRODUCTTABLE ".select().execute()");

  CHECK_OBJECT_COMPLETIONS(PRODUCTTABLE ".update()");
  CHECK_OBJECT_COMPLETIONS(PRODUCTTABLE ".update().set('x',1)");
  CHECK_OBJECT_COMPLETIONS(PRODUCTTABLE
                           ".update().set('x',1).where(':x').bind('x',1)");
  CHECK_OBJECT_COMPLETIONS(PRODUCTTABLE ".update().set('id',1).orderBy(['x'])");
  CHECK_OBJECT_COMPLETIONS(PRODUCTTABLE ".update().set('id',1).execute()");

  CHECK_OBJECT_COMPLETIONS(PRODUCTTABLE ".delete()");
  CHECK_OBJECT_COMPLETIONS(PRODUCTTABLE ".delete().where('0')");
  CHECK_OBJECT_COMPLETIONS(PRODUCTTABLE ".delete().orderBy(['x'])");
  CHECK_OBJECT_COMPLETIONS(PRODUCTTABLE ".delete().where(':x').bind('x',1)");
  CHECK_OBJECT_COMPLETIONS(PRODUCTTABLE ".delete().execute()");

  CHECK_OBJECT_COMPLETIONS(PRODUCTTABLE ".insert([])");
  CHECK_OBJECT_COMPLETIONS(PRODUCTTABLE ".insert([]).values([])");

  execute("session.rollback()");
}

TEST_F(Completer_frontend, js_classic) {
  connect_classic();
  execute("\\js");

  EXPECT_AFTER_TAB("session.", "session.");
  // TS_FR5.2_C05
  CHECK_OBJECT_COMPLETIONS("session");
  // TS_FR5.2_C06
  CHECK_OBJECT_COMPLETIONS("shell");

  EXPECT_AFTER_TAB("session.ru", "session.runSql()");

  // TS_FR5.2_C03
  CHECK_OBJECT_COMPLETIONS("mysql");
}

TEST_F(Completer_frontend, js_devapi_members_x) {
  _options->devapi_schema_object_handles = false;
  reset_shell();
  connect_x();
  execute("\\sql");
  execute("drop schema if exists dropme;");
  execute("drop schema if exists dropme2;");
  execute("create table actest.testtab (a int);");
  execute("create view actest.veee as select 1;");

  execute("create table actest.droptab (a int);");
  execute("create view actest.dropview as select 1;");

  execute("\\js");
  execute("\\use actest");

  execute("db.createCollection('testcol')");
  execute("db.createCollection('dropcol')");
  wipe_all();

  std::vector<std::pair<std::string, std::string>> mysqlx_calls{
      {"dateValue", ""},
      {"expr", ""},
      {"getSession", "('mysqlx://" + _uri + "')"},
      {"help", ""}};
  CHECK_OBJECT_MEMBER_COMPLETIONS("mysqlx", mysqlx_calls);

  std::vector<std::pair<std::string, std::string>> session_calls{
      {"createSchema", "('dropme2')"},
      {"dropSchema", "('dropme2')"},
      {"setCurrentSchema", "('actest')"},
      {"getSchema", "('actest')"},
      {"setFetchWarnings", "(true)"},
      {"sql", "('select 1')"},
      {"getDefaultSchema", ""},
      {"getUri", ""},
      {"close", ""},
      {"help", ""},
      {"quoteName", ""},
      {"startTransaction", "()"},
      {"setSavepoint", "('name')"},
      {"rollbackTo", "('name')"},
      {"releaseSavepoint", "('name')"},
      {"rollback", "()"},
      {"runSql", "('select 1')"},
      {"_enableNotices", ""},
      {"_fetchNotice", ""},
      {"_getSocketFd", ""}};
  CHECK_OBJECT_MEMBER_COMPLETIONS("session", session_calls);

  std::vector<std::pair<std::string, std::string>> db_calls{
      {"createCollection", "('dropme')"},
      {"modifyCollection", "('dropme', {'validation': {'level':'off'}})"},
      {"dropCollection", "('dropme')"},
      {"getCollection", "('testcol')"},
      {"getTable", "('testtab')"},
      {"getCollectionAsTable", "('testcol')"},
      {"getSchema", ""},  // bogus getSchema() in Schema that returns null
      {"help", ""}};
  CHECK_OBJECT_MEMBER_COMPLETIONS("db", db_calls);

  std::vector<std::pair<std::string, std::string>> collection_calls{
      {"add", "({})"},
      {"addOrReplaceOne", "('0', {})"},
      {"modify", "('0')"},
      {"remove", "('0')"},
      {"removeOne", "('0')"},
      {"replaceOne", "('0', {})"},
      {"getOne", "('0')"},
      {"find", "('0')"},
      {"createIndex", "('x', {fields:[{field:'$.field'}]})"},
      {"dropIndex", "('x')"},
      {"help", ""}};
  CHECK_OBJECT_MEMBER_COMPLETIONS("db.getCollection('people')",
                                  collection_calls);  // collection

  std::vector<std::pair<std::string, std::string>> table_calls{{"help", ""}};
  CHECK_OBJECT_MEMBER_COMPLETIONS("db.getTable('productTable')",
                                  table_calls);  // table

  execute("\\sql");
  execute("drop schema if exists dropme;");
}

TEST_F(Completer_frontend, js_devapi_members_classic) {
  connect_classic();
  execute("\\js");
  execute("\\use actest");

  std::vector<std::pair<std::string, std::string>> mysql_calls{
      {"quoteIdentifier", "('a')"},
      {"unquoteIdentifier", "(`a`)"},
      {"splitScript", "('a;b;')"},
      {"parseStatementAst", "('select 1')"},
      {"help", ""},
      {"getClassicSession", "('" + _mysql_uri + "')"},
      {"getSession", "('" + _mysql_uri + "')"}};

  CHECK_OBJECT_MEMBER_COMPLETIONS("mysql", mysql_calls);

  std::vector<std::pair<std::string, std::string>> session_calls{
      {"help", ""},
      {"close", ""},
      {"runSql", "('select 1')"},
      {"setQueryAttributes", "({})"},
      {"_getSocketFd", ""}};
  CHECK_OBJECT_MEMBER_COMPLETIONS("session", session_calls);
}
#endif
// -----

TEST_F(Completer_frontend, py_keywords) {
  execute("\\py");

  EXPECT_AFTER_TAB("pri", "print");
  EXPECT_AFTER_TAB("x=pri", "x=print");
  EXPECT_AFTER_TAB("var x =pri", "var x =print");
  EXPECT_AFTER_TAB("print sh", "print shell");
  EXPECT_AFTER_TAB("print (sh\t)", "print (shell)");
  EXPECT_AFTER_TAB("wh", "while");

  EXPECT_AFTER_TAB("while sh", "while shell");
  EXPECT_AFTER_TAB("while (sh\t)", "while (shell)");
  EXPECT_AFTER_TAB("while 1: sh", "while 1: shell");
  EXPECT_AFTER_TAB("while 1: if sh", "while 1: if shell");
  EXPECT_TAB_DOES_NOTHING("print '");
  EXPECT_TAB_DOES_NOTHING("print \"");
  EXPECT_TAB_DOES_NOTHING("print 'pri");
  EXPECT_TAB_DOES_NOTHING("print \"pri");
  EXPECT_TAB_DOES_NOTHING("x = px");
  EXPECT_TAB_DOES_NOTHING(".");
  EXPECT_TAB_DOES_NOTHING("..");
  EXPECT_TAB_DOES_NOTHING(".s");
  EXPECT_TAB_DOES_NOTHING("(.s");
  EXPECT_TAB_DOES_NOTHING("(.re");
  EXPECT_TAB_DOES_NOTHING("shell..");

  // Session stuff while not connected
  EXPECT_AFTER_TAB_TAB("session.", strv());
  EXPECT_TAB_DOES_NOTHING("db.");
}

TEST_F(Completer_frontend, py_negative) {
  execute("\\py");
  EXPECT_AFTER_TAB("bogus", "bogus");
  EXPECT_AFTER_TAB_TAB("bogus", strv({}));
  EXPECT_AFTER_TAB("bogus.", "bogus.");
  EXPECT_AFTER_TAB_TAB("bogus.", strv({}));
  EXPECT_AFTER_TAB("print(\t)", "print()");
  EXPECT_AFTER_TAB("print('pr\t", "print('pr");
  EXPECT_AFTER_TAB("print('pr\t')", "print('pr')");
}

TEST_F(Completer_frontend, py_shell) {
  execute("\\py");

  CHECK_OBJECT_COMPLETIONS("shell");

  EXPECT_AFTER_TAB("sh", "shell");
  EXPECT_AFTER_TAB("shell.rec", "shell.reconnect()");
  EXPECT_AFTER_TAB_TAB("shell.", strv({"add_extension_object_member()",
                                       "auto_complete_sql()",
                                       "connect()",
                                       "connect_to_primary()",
                                       "create_context()",
                                       "create_extension_object()",
                                       "delete_all_credentials()",
                                       "delete_credential()",
                                       "disable_pager()",
                                       "disconnect()",
                                       "dump_rows()",
                                       "enable_pager()",
                                       "get_session()",
                                       "help()",
                                       "list_credential_helpers()",
                                       "list_credentials()",
                                       "list_ssh_connections()",
                                       "log()",
                                       "open_session()",
                                       "options",
                                       "parse_uri()",
                                       "prompt()",
                                       "reconnect()",
                                       "register_global()",
                                       "register_report()",
                                       "reports",
                                       "set_current_schema()",
                                       "set_session()",
                                       "status()",
                                       "store_credential()",
                                       "unparse_uri()",
                                       "version"}));

  EXPECT_TAB_DOES_NOTHING("shell.conect()");

  EXPECT_AFTER_TAB_TAB("mysql", strv({"mysql", "mysqlx"}));
  EXPECT_AFTER_TAB("mysql.get_c", "mysql.get_classic_session()");
  EXPECT_AFTER_TAB("mysqlx.get", "mysqlx.get_session()");

  // a dynamically created global var
  execute("testobj = {'key_one':1, 'another_key': 2}");
  EXPECT_AFTER_TAB("testo", "testobj");
  std::set<std::string> dict_options = {
      "clear()", "copy()",    "fromkeys()",   "get()",    "items()", "keys()",
      "pop()",   "popitem()", "setdefault()", "update()", "values()"};
  if (shcore::str_beginswith(m_python_version, "2.")) {
    dict_options.emplace("has_key()");
    dict_options.emplace("iteritems()");
    dict_options.emplace("iterkeys()");
    dict_options.emplace("itervalues()");
  }
  if (shcore::str_beginswith(m_python_version, "2.7")) {
    dict_options.emplace("viewitems()");
    dict_options.emplace("viewkeys()");
    dict_options.emplace("viewvalues()");
  }
  EXPECT_AFTER_TAB_TAB("testobj.", dict_options);
  EXPECT_AFTER_TAB("testobj.k", "testobj.keys()");
  EXPECT_TAB_DOES_NOTHING("testobj.key_one");

  // TS_FR5.3_C01
  CHECK_OBJECT_COMPLETIONS("shell.options");
}

TEST_F(Completer_frontend, py_adminapi) {
  execute("\\py");

  CHECK_OBJECT_COMPLETIONS("dba");

  // TS_FR5.2_C02, TS_FR5.2_X02
  EXPECT_AFTER_TAB_TAB(
      "dba.",
      strv({"check_instance_configuration()", "configure_instance()",
            "configure_replica_set_instance()", "create_cluster()",
            "create_replica_set()", "delete_sandbox_instance()",
            "deploy_sandbox_instance()", "drop_metadata_schema()",
            "get_cluster()", "get_cluster_set()", "get_replica_set()", "help()",
            "kill_sandbox_instance()", "reboot_cluster_from_complete_outage()",
            "session", "start_sandbox_instance()", "stop_sandbox_instance()",
            "upgrade_metadata()", "verbose"}));
  EXPECT_AFTER_TAB("dba.depl", "dba.deploy_sandbox_instance()");
}

TEST_F(Completer_frontend, py_devapi) {
  connect_x();
  execute("\\py");

  CHECK_OBJECT_COMPLETIONS("session");

  EXPECT_AFTER_TAB("session.create_s", "session.create_schema()");
  EXPECT_AFTER_TAB("session.st", "session.start_transaction()");
  EXPECT_AFTER_TAB_TAB(
      "session.get_",
      strv({"get_current_schema()", "get_default_schema()", "get_schema()",
            "get_schemas()", "get_ssh_uri()", "get_uri()"}));

  EXPECT_AFTER_TAB("session.sql", "session.sql()");
  EXPECT_AFTER_TAB("session.sql('sele\t", "session.sql('sele");
  EXPECT_AFTER_TAB_TAB("session.sql('sele\t", strv({}));
  EXPECT_AFTER_TAB("session.sql('select 1')\t", "session.sql('select 1')");
  EXPECT_AFTER_TAB_TAB("session.sql('select 1')\t", strv({}));

  EXPECT_AFTER_TAB_TAB("session.sql('select 1').",
                       strv({"bind()", "execute()", "help()"}));
  EXPECT_AFTER_TAB("session.sql('select 1').b",
                   "session.sql('select 1').bind()");
  EXPECT_AFTER_TAB("session.sql('select 1').e",
                   "session.sql('select 1').execute()");

  EXPECT_AFTER_TAB_TAB("session.sql(\"select 1\").",
                       strv({"bind()", "execute()", "help()"}));
  EXPECT_AFTER_TAB("session.sql(\"select 1\").b",
                   "session.sql(\"select 1\").bind()");
  EXPECT_AFTER_TAB("session.sql(mkquery()).e",
                   "session.sql(mkquery()).execute()");
  EXPECT_AFTER_TAB("session.sql(mkquery(\"\")).e",
                   "session.sql(mkquery(\"\")).execute()");

  EXPECT_AFTER_TAB_TAB(
      "session.get_",
      strv({"get_current_schema()", "get_default_schema()", "get_schema()",
            "get_schemas()", "get_ssh_uri()", "get_uri()"}));
  EXPECT_AFTER_TAB("session.get_c", "session.get_current_schema()");

  // TS_FR5.1_X01
  EXPECT_AFTER_TAB_TAB("db", strv({"db", "dba"}));
  // TS_FR5.1_X02
  EXPECT_AFTER_TAB_TAB("mysql", strv({"mysql", "mysqlx"}));
  // TS_FR5.1_X03
  // invalid for python
  // TS_FR5.1_X04
  EXPECT_AFTER_TAB("ses", "session");

  // no schema active
  EXPECT_TAB_DOES_NOTHING("db.p");

  execute("\\use actest");
  EXPECT_AFTER_TAB("db.p", "db.p");
  EXPECT_AFTER_TAB_TAB("db.p", strv({"people", "person", PRODUCTTABLE}));
  EXPECT_AFTER_TAB("db.peo", "db.people");
  EXPECT_AFTER_TAB("db.pr", DB_PRODUCTTABLE);
  EXPECT_AFTER_TAB("p = db.peo", "p = db.people");

  EXPECT_AFTER_TAB("db.get_c", "db.get_collection");
  EXPECT_AFTER_TAB("db.get_collection_a", "db.get_collection_as_table()");
  EXPECT_AFTER_TAB("db.get_t", "db.get_table");

  // TS_FR5.2_X01
  CHECK_OBJECT_COMPLETIONS("db");

  execute("actest = session.get_schema('actest')");
  EXPECT_AFTER_TAB("actest.create_c", "actest.create_collection()");
}

TEST_F(Completer_frontend, py_devapi_collection) {
  connect_x();
  execute("\\py");
  execute("\\use actest");
  execute("session.start_transaction()");

  CHECK_OBJECT_COMPLETIONS("db");

  EXPECT_AFTER_TAB("db.peo", "db.people");
  EXPECT_AFTER_TAB("db.people.", "db.people.");
  EXPECT_AFTER_TAB_TAB(
      "db.people.",
      strv({"add()", "add_or_replace_one()", "count()", "create_index()",
            "drop_index()", "exists_in_database()", "find()", "get_name()",
            "get_one()", "get_schema()", "get_session()", "help()", "modify()",
            "name", "remove()", "remove_one()", "replace_one()", "schema",
            "session"}));

  EXPECT_TAB_DOES_NOTHING("db.people.x");
  EXPECT_TAB_DOES_NOTHING("db.people..");
  EXPECT_TAB_DOES_NOTHING("db.people.?");
  EXPECT_TAB_DOES_NOTHING("db.people.#");

  EXPECT_AFTER_TAB("db.people.f", "db.people.find()");
  EXPECT_AFTER_TAB("db.people.find().s", "db.people.find().sort()");
  EXPECT_AFTER_TAB("db.people.find('foo()').b",
                   "db.people.find('foo()').bind()");
  EXPECT_AFTER_TAB("db.people.find().bind().b",
                   "db.people.find().bind().bind()");
  EXPECT_AFTER_TAB("db.people.find().bind().e",
                   "db.people.find().bind().execute()");
  EXPECT_AFTER_TAB("db.people.find().gr", "db.people.find().group_by()");
  EXPECT_AFTER_TAB("db.people.find().group_by().ha",
                   "db.people.find().group_by().having()");
  EXPECT_AFTER_TAB("db.people.find().execute().fe",
                   "db.people.find().execute().fetch_");
  EXPECT_AFTER_TAB_TAB("db.people.find().execute().fetch_",
                       strv({"fetch_all()", "fetch_one()"}));

  EXPECT_TAB_DOES_NOTHING("db.people.find().bind().s");
  EXPECT_TAB_DOES_NOTHING("db.people.find().bind(.s");
  EXPECT_TAB_DOES_NOTHING("db.people.find.ex");
  EXPECT_TAB_DOES_NOTHING("db.people.find(.ex");
  EXPECT_TAB_DOES_NOTHING("db.people.find).ex");
  EXPECT_TAB_DOES_NOTHING("db.people.find()");

  execute("people = db.people");
  EXPECT_AFTER_TAB("people.", "people.");
  EXPECT_AFTER_TAB_TAB(
      "people.",
      strv({"add()", "add_or_replace_one()", "count()", "create_index()",
            "drop_index()", "exists_in_database()", "find()", "get_name()",
            "get_one()", "get_schema()", "get_session()", "help()", "modify()",
            "name", "remove()", "remove_one()", "replace_one()", "schema",
            "session"}));
  EXPECT_AFTER_TAB("people.f", "people.find()");
  EXPECT_AFTER_TAB("people.find().e", "people.find().execute()");
  EXPECT_AFTER_TAB("f = people.find().b", "f = people.find().bind()");
  execute("findOp = db.people.find()");
  EXPECT_TAB_DOES_NOTHING("findOp.fin");
  EXPECT_AFTER_TAB("findOp.exe", "findOp.execute()");

  CHECK_OBJECT_COMPLETIONS("people");

  CHECK_OBJECT_COMPLETIONS("people.find()");
  CHECK_OBJECT_COMPLETIONS("people.find().fields(['x'])");
  CHECK_OBJECT_COMPLETIONS("people.find().sort(['x'])");
  CHECK_OBJECT_COMPLETIONS("people.find().group_by(['x'])");
  CHECK_OBJECT_COMPLETIONS("people.find().group_by(['x']).having('x')");
  CHECK_OBJECT_COMPLETIONS("people.find().limit(2)");
  CHECK_OBJECT_COMPLETIONS("people.find(':x').bind('x',1)");
  CHECK_OBJECT_COMPLETIONS("people.find().execute()");

  CHECK_OBJECT_COMPLETIONS("people.modify('1')");
  CHECK_OBJECT_COMPLETIONS("people.modify('1').set('x',1)");
  CHECK_OBJECT_COMPLETIONS("people.modify('1').unset('x')");
  CHECK_OBJECT_COMPLETIONS("people.modify(':x').unset('x').bind('x',1)");
  CHECK_OBJECT_COMPLETIONS("people.modify('1').unset('x').execute()");

  CHECK_OBJECT_COMPLETIONS("people.remove('1')");
  CHECK_OBJECT_COMPLETIONS("people.remove('1').sort(['x'])");
  CHECK_OBJECT_COMPLETIONS("people.remove(':x').bind('x',1)");
  CHECK_OBJECT_COMPLETIONS("people.remove('1').execute()");

  CHECK_OBJECT_COMPLETIONS("people.add({'_id': '0001'})");
  CHECK_OBJECT_COMPLETIONS("people.add({'_id': '0002'}).execute()");

  execute("session.rollback()");
}

TEST_F(Completer_frontend, py_devapi_table) {
  connect_x();
  execute("\\py");
  execute("\\use actest");

  execute("session.startTransaction()");

  EXPECT_AFTER_TAB("db.pro", DB_PRODUCTTABLE);
  EXPECT_AFTER_TAB(DB_PRODUCTTABLE ".", DB_PRODUCTTABLE ".");
  EXPECT_AFTER_TAB_TAB(
      DB_PRODUCTTABLE ".",
      strv({"count()", "delete()", "exists_in_database()", "get_name()",
            "get_schema()", "get_session()", "help()", "insert()", "is_view()",
            "name", "schema", "select()", "session", "update()"}));

  EXPECT_TAB_DOES_NOTHING(DB_PRODUCTTABLE ".x");
  EXPECT_TAB_DOES_NOTHING(DB_PRODUCTTABLE "..");
  EXPECT_TAB_DOES_NOTHING(DB_PRODUCTTABLE ".?");
  EXPECT_TAB_DOES_NOTHING(DB_PRODUCTTABLE ".#");

  EXPECT_AFTER_TAB(DB_PRODUCTTABLE ".sel", DB_PRODUCTTABLE ".select()");
  EXPECT_AFTER_TAB(DB_PRODUCTTABLE ".select().w",
                   DB_PRODUCTTABLE ".select().where()");
  EXPECT_AFTER_TAB(DB_PRODUCTTABLE ".select().o",
                   DB_PRODUCTTABLE ".select().order_by()");
  EXPECT_AFTER_TAB(DB_PRODUCTTABLE ".select('foo()').b",
                   DB_PRODUCTTABLE ".select('foo()').bind()");
  EXPECT_AFTER_TAB(DB_PRODUCTTABLE ".select().bind().b",
                   DB_PRODUCTTABLE ".select().bind().bind()");
  EXPECT_AFTER_TAB(DB_PRODUCTTABLE ".select().bind().e",
                   DB_PRODUCTTABLE ".select().bind().execute()");
  EXPECT_AFTER_TAB(DB_PRODUCTTABLE ".select().gr",
                   DB_PRODUCTTABLE ".select().group_by()");
  EXPECT_AFTER_TAB(DB_PRODUCTTABLE ".select().group_by().ha",
                   DB_PRODUCTTABLE ".select().group_by().having()");
  EXPECT_AFTER_TAB(DB_PRODUCTTABLE ".select().execute().fe",
                   DB_PRODUCTTABLE ".select().execute().fetch_");
  EXPECT_AFTER_TAB_TAB(
      DB_PRODUCTTABLE ".select().execute().fetch_",
      strv({"fetch_all()", "fetch_one()", "fetch_one_object()"}));

  EXPECT_TAB_DOES_NOTHING(DB_PRODUCTTABLE ".select().bind().s");
  EXPECT_TAB_DOES_NOTHING(DB_PRODUCTTABLE ".select().bind(.s");
  EXPECT_TAB_DOES_NOTHING(DB_PRODUCTTABLE ".select.ex");
  EXPECT_TAB_DOES_NOTHING(DB_PRODUCTTABLE ".select(.ex");
  EXPECT_TAB_DOES_NOTHING(DB_PRODUCTTABLE ".select).ex");
  EXPECT_TAB_DOES_NOTHING(DB_PRODUCTTABLE ".select()");
  EXPECT_TAB_DOES_NOTHING(DB_PRODUCTTABLE ".select().hav");

  EXPECT_AFTER_TAB(DB_PRODUCTTABLE ".insert([]).v",
                   DB_PRODUCTTABLE ".insert([]).values()");

  execute("productTable = " DB_PRODUCTTABLE);
  EXPECT_AFTER_TAB("productTable.", "productTable.");
  EXPECT_AFTER_TAB_TAB(
      "productTable.",
      strv({"count()", "delete()", "exists_in_database()", "get_name()",
            "get_schema()", "get_session()", "help()", "insert()", "is_view()",
            "name", "schema", "select()", "session", "update()"}));
  EXPECT_AFTER_TAB("productTable.sel", "productTable.select()");
  EXPECT_AFTER_TAB("productTable.select([]).e",
                   "productTable.select([]).execute()");
  EXPECT_AFTER_TAB("f = productTable.select([]).b",
                   "f = productTable.select([]).bind()");
  execute("findOp = " DB_PRODUCTTABLE ".select();");

  EXPECT_TAB_DOES_NOTHING("findOp.sel");
  EXPECT_AFTER_TAB("findOp.exe", "findOp.execute()");

  CHECK_OBJECT_COMPLETIONS("productTable");

  CHECK_OBJECT_COMPLETIONS("productTable.select(['x'])");
  CHECK_OBJECT_COMPLETIONS("productTable.select(['x']).where('x')");
  CHECK_OBJECT_COMPLETIONS("productTable.select(['x']).order_by(['x'])");
  CHECK_OBJECT_COMPLETIONS("productTable.select(['x']).group_by(['x'])");
  CHECK_OBJECT_COMPLETIONS(
      "productTable.select(['x']).group_by(['x']).having('x')");
  CHECK_OBJECT_COMPLETIONS("productTable.select(['x']).limit(2)");
  CHECK_OBJECT_COMPLETIONS("productTable.select(['x']).limit(2).offset(2)");
  CHECK_OBJECT_COMPLETIONS(
      "productTable.select(['x']).where(':x').bind('x',1)");
  CHECK_OBJECT_COMPLETIONS("productTable.select().execute()");

  CHECK_OBJECT_COMPLETIONS("productTable.update()");
  CHECK_OBJECT_COMPLETIONS("productTable.update().set('x',1)");
  CHECK_OBJECT_COMPLETIONS(
      "productTable.update().set('x',1).where(':x').bind('x',1)");
  CHECK_OBJECT_COMPLETIONS("productTable.update().set('id',1).order_by(['x'])");
  CHECK_OBJECT_COMPLETIONS("productTable.update().set('id',1).execute()");

  CHECK_OBJECT_COMPLETIONS("productTable.delete()");
  CHECK_OBJECT_COMPLETIONS("productTable.delete().where('0')");
  CHECK_OBJECT_COMPLETIONS("productTable.delete().order_by(['x'])");
  CHECK_OBJECT_COMPLETIONS("productTable.delete().where(':x').bind('x',1)");
  CHECK_OBJECT_COMPLETIONS("productTable.delete().execute()");

  CHECK_OBJECT_COMPLETIONS("productTable.insert([])");
  CHECK_OBJECT_COMPLETIONS("productTable.insert([]).values([])");

  execute("session.rollback()");
}

TEST_F(Completer_frontend, py_classic) {
  connect_classic();
  execute("\\py");

  EXPECT_AFTER_TAB("session.", "session.");
  // TS_FR5.2_C05
  CHECK_OBJECT_COMPLETIONS("session");
  // TS_FR5.2_C06
  CHECK_OBJECT_COMPLETIONS("shell");

  EXPECT_AFTER_TAB("session.ru", "session.run_sql()");

  // TS_FR5.2_C03
  CHECK_OBJECT_COMPLETIONS("mysql");
}

TEST_F(Completer_frontend, WL13397_TSFR_1_1) {
  connect_classic();
  execute("\\sql");

  EXPECT_AFTER_TAB("SELECT 1 FROM du", "SELECT 1 FROM DUAL");
}

TEST_F(Completer_frontend, WL13397_TSFR_1_2) {
  connect_classic();
  execute("\\sql");

  EXPECT_TAB_DOES_NOTHING("DELIMITER //");
}

TEST_F(Completer_frontend, WL13397_TSFR_1_1_1) {
  connect_classic();
  execute("\\sql");

  // syntax available in 5.7, but not in 8.0
  if (_target_server_version < mysqlshdk::utils::Version(8, 0, 0)) {
    EXPECT_AFTER_TAB("SELECT pass", "SELECT PASSWORD()");
    EXPECT_AFTER_TAB("FLUSH qu", "FLUSH QUERY CACHE");
  } else {
    EXPECT_TAB_DOES_NOTHING("SELECT pass");
    EXPECT_TAB_DOES_NOTHING("FLUSH qu");
  }
}

TEST_F(Completer_frontend, WL13397_TSFR_1_1_1_1) {
#if defined(NDEBUG)
  SKIP_TEST("This test requires a debug build.");
#else
  DBUG_SET("+d,sql_auto_completion_unsupported_version_lower");

  const auto log_level = output_handler.get_log_level();
  output_handler.set_log_level(shcore::Logger::LOG_LEVEL::LOG_WARNING);

  connect_classic();
  execute("\\sql");

  // wrong version (lower), auto-completion falls-back to 5.7 server
  EXPECT_AFTER_TAB("FLUSH qu", "FLUSH QUERY CACHE");
  EXPECT_TAB_DOES_NOTHING("CREATE ro");

  MY_EXPECT_LOG_CONTAINS(
      "Connected to an unsupported server version 1.2.3, SQL auto-completion "
      "is falling back to: 5.7.0");

  output_handler.set_log_level(log_level);
  DBUG_SET("");
#endif
}

TEST_F(Completer_frontend, WL13397_TSFR_1_1_1_2) {
#if defined(NDEBUG)
  SKIP_TEST("This test requires a debug build.");
#else
  DBUG_SET("+d,sql_auto_completion_unsupported_version_higher");

  const auto log_level = output_handler.get_log_level();
  output_handler.set_log_level(shcore::Logger::LOG_LEVEL::LOG_WARNING);

  connect_classic();
  execute("\\sql");

  // wrong version (higher), auto-completion falls-back to latest 8.0 server
  EXPECT_TAB_DOES_NOTHING("FLUSH qu");
  EXPECT_AFTER_TAB("CREATE ro", "CREATE ROLE");

  MY_EXPECT_LOG_CONTAINS(
      "Connected to an unsupported server version 10.9.8, SQL auto-completion "
      "is falling back to: " MYSH_VERSION);

  output_handler.set_log_level(log_level);
  DBUG_SET("");
#endif
}

TEST_F(Completer_frontend, WL13397_TSFR_1_1_2_1) {
  execute("\\sql");

  // no connection, auto-completion falls-back to latest 8.0 server
  EXPECT_TAB_DOES_NOTHING("SELECT pass");
  EXPECT_AFTER_TAB("CREATE ro", "CREATE ROLE");
}

TEST_F(Completer_frontend, WL13397_TSFR_1_2_1) {
  connect_classic();

  execute("\\sql");

  execute("SET @saved_sql_mode = @@global.sql_mode;");
  execute("SET @@global.sql_mode = '';");
  execute("SET @@sql_mode = 'ANSI_QUOTES';");

  execute("\\use mysql");

  // ANSI_QUOTES is active, "mysql" should be treated as an identifier and
  // plugin should be suggested
  EXPECT_AFTER_TAB("SELECT \"mysql\".plu", "SELECT \"mysql\".plugin");

  execute("SET @@global.sql_mode = @saved_sql_mode;");
}

TEST_F(Completer_frontend, WL13397_TSFR_1_2_2) {
  connect_classic();

  execute("\\sql");

  execute("SET @saved_sql_mode = @@global.sql_mode;");
  execute("SET @@global.sql_mode = 'ANSI_QUOTES';");
  execute("SET @@sql_mode = '';");

  execute("\\use mysql");

  // ANSI_QUOTES is not active, "mysql" should be treated as a string and
  // nothing is suggested
  EXPECT_TAB_DOES_NOTHING("SELECT \"mysql\".plu");

  execute("SET @@global.sql_mode = @saved_sql_mode;");
}

TEST_F(Completer_frontend, WL13397_TSFR_1_3_1) {
  connect_classic();
  execute("\\sql");

  EXPECT_TAB_TAB_CONTAINS("SELECT ", "mysql");
  EXPECT_TAB_TAB_CONTAINS("SELECT * FROM ", "mysql");
}

TEST_F(Completer_frontend, WL13397_TSFR_1_3_2) {
  connect_classic("mysql");
  execute("\\sql");

  EXPECT_TAB_TAB_CONTAINS("SELECT * FROM ", "plugin");
}

TEST_F(Completer_frontend, WL13397_TSFR_1_3_3) {
  connect_classic();
  execute("\\sql");
  execute("\\use information_schema");

  EXPECT_TAB_TAB_NOT_CONTAINS("SELECT * FROM ", "plugin");
  EXPECT_TAB_TAB_CONTAINS("SELECT * FROM ", "SCHEMATA");

  execute("\\use mysql");

  EXPECT_TAB_TAB_CONTAINS("SELECT * FROM ", "plugin");
  EXPECT_TAB_TAB_NOT_CONTAINS("SELECT * FROM ", "SCHEMATA");
}

TEST_F(Completer_frontend, WL13397_TSFR_1_3_1_1) {
  connect_classic("mysql");
  execute("\\sql");
  execute("\\disconnect");

  EXPECT_TAB_TAB_CONTAINS("SELECT * FROM ", "DUAL");
  EXPECT_TAB_TAB_NOT_CONTAINS("SELECT * FROM ", "plugin");
}

TEST_F(Completer_frontend, WL13397_TSFR_1_4_1) {
  connect_classic();
  execute("\\sql");

  EXPECT_TAB_TAB_CONTAINS("CREATE ", "TABLE");
  EXPECT_TAB_TAB_CONTAINS("CREATE ", "DATABASE");
  EXPECT_TAB_TAB_CONTAINS("CREATE ", "FUNCTION");
}

// tests for BUG#34371461 (quote_*)

TEST_F(Completer_frontend, quote_schema) {
  connect_classic();
  execute("\\sql");

  EXPECT_AFTER_TAB("SELECT acte", "SELECT actest");
  EXPECT_AFTER_TAB("SELECT `acte", "SELECT `actest`");
  EXPECT_TAB_DOES_NOTHING("SELECT \"acte");

  execute("SET @@sql_mode = 'ANSI_QUOTES';");
  execute("\\rehash");

  EXPECT_AFTER_TAB("SELECT acte", "SELECT actest");
  EXPECT_AFTER_TAB("SELECT `acte", "SELECT `actest`");
  EXPECT_AFTER_TAB("SELECT \"acte", "SELECT \"actest\"");
}

TEST_F(Completer_frontend, quote_table) {
  connect_classic();
  execute("\\sql");
  execute("\\use actest");

  EXPECT_AFTER_TAB("SELECT tab", "SELECT tab_le");
  EXPECT_AFTER_TAB("SELECT `actest`.tab", "SELECT `actest`.tab_le");

  EXPECT_AFTER_TAB("SELECT `tab", "SELECT `tab_le`");
  EXPECT_AFTER_TAB("SELECT actest.`tab", "SELECT actest.`tab_le`");

  EXPECT_TAB_DOES_NOTHING("SELECT \"tab");
  EXPECT_TAB_DOES_NOTHING("SELECT actest.\"tab");

  execute("SET @@sql_mode = 'ANSI_QUOTES';");
  execute("\\rehash");

  EXPECT_AFTER_TAB("SELECT tab", "SELECT tab_le");
  EXPECT_AFTER_TAB("SELECT \"actest\".tab", "SELECT \"actest\".tab_le");

  EXPECT_AFTER_TAB("SELECT `tab", "SELECT `tab_le`");
  EXPECT_AFTER_TAB("SELECT actest.`tab", "SELECT actest.`tab_le`");

  EXPECT_AFTER_TAB("SELECT \"tab", "SELECT \"tab_le\"");
  EXPECT_AFTER_TAB("SELECT `actest`.\"tab", "SELECT `actest`.\"tab_le\"");
}

TEST_F(Completer_frontend, quote_view) {
  connect_classic();
  execute("\\sql");
  execute("\\use actest");

  EXPECT_AFTER_TAB("SELECT vi", "SELECT vi_ew");
  EXPECT_AFTER_TAB("SELECT `actest`.vi", "SELECT `actest`.vi_ew");

  EXPECT_AFTER_TAB("SELECT `vi", "SELECT `vi_ew`");
  EXPECT_AFTER_TAB("SELECT actest.`vi", "SELECT actest.`vi_ew`");

  EXPECT_TAB_DOES_NOTHING("SELECT \"vi");
  EXPECT_TAB_DOES_NOTHING("SELECT actest.\"vi");

  execute("SET @@sql_mode = 'ANSI_QUOTES';");
  execute("\\rehash");

  EXPECT_AFTER_TAB("SELECT vi", "SELECT vi_ew");
  EXPECT_AFTER_TAB("SELECT \"actest\".vi", "SELECT \"actest\".vi_ew");

  EXPECT_AFTER_TAB("SELECT `vi", "SELECT `vi_ew`");
  EXPECT_AFTER_TAB("SELECT actest.`vi", "SELECT actest.`vi_ew`");

  EXPECT_AFTER_TAB("SELECT \"vi", "SELECT \"vi_ew\"");
  EXPECT_AFTER_TAB("SELECT `actest`.\"vi", "SELECT `actest`.\"vi_ew\"");
}

TEST_F(Completer_frontend, quote_column) {
  connect_classic();
  execute("\\sql");
  execute("\\use actest");

  EXPECT_AFTER_TAB("SELECT tab_le.col", "SELECT tab_le.col_umn");
  EXPECT_AFTER_TAB("SELECT `actest`.tab_le.col",
                   "SELECT `actest`.tab_le.col_umn");

  EXPECT_AFTER_TAB("SELECT `tab_le`.`col", "SELECT `tab_le`.`col_umn`");
  EXPECT_AFTER_TAB("SELECT actest.`tab_le`.`col",
                   "SELECT actest.`tab_le`.`col_umn`");

  EXPECT_TAB_DOES_NOTHING("SELECT tab_le.\"col");
  EXPECT_TAB_DOES_NOTHING("SELECT actest.tab_le.\"col");

  EXPECT_AFTER_TAB("SELECT vi_ew.col", "SELECT vi_ew.col_umn");
  EXPECT_AFTER_TAB("SELECT `actest`.vi_ew.col",
                   "SELECT `actest`.vi_ew.col_umn");

  EXPECT_AFTER_TAB("SELECT `vi_ew`.`col", "SELECT `vi_ew`.`col_umn`");
  EXPECT_AFTER_TAB("SELECT actest.`vi_ew`.`col",
                   "SELECT actest.`vi_ew`.`col_umn`");

  EXPECT_TAB_DOES_NOTHING("SELECT vi_ew.\"col");
  EXPECT_TAB_DOES_NOTHING("SELECT actest.vi_ew.\"col");

  execute("SET @@sql_mode = 'ANSI_QUOTES';");
  execute("\\rehash");

  EXPECT_AFTER_TAB("SELECT tab_le.col", "SELECT tab_le.col_umn");
  EXPECT_AFTER_TAB("SELECT \"actest\".tab_le.col",
                   "SELECT \"actest\".tab_le.col_umn");

  EXPECT_AFTER_TAB("SELECT `tab_le`.`col", "SELECT `tab_le`.`col_umn`");
  EXPECT_AFTER_TAB("SELECT actest.`tab_le`.`col",
                   "SELECT actest.`tab_le`.`col_umn`");

  EXPECT_AFTER_TAB("SELECT \"tab_le\".\"col", "SELECT \"tab_le\".\"col_umn\"");
  EXPECT_AFTER_TAB("SELECT `actest`.\"tab_le\".\"col",
                   "SELECT `actest`.\"tab_le\".\"col_umn\"");

  EXPECT_AFTER_TAB("SELECT vi_ew.col", "SELECT vi_ew.col_umn");
  EXPECT_AFTER_TAB("SELECT \"actest\".vi_ew.col",
                   "SELECT \"actest\".vi_ew.col_umn");

  EXPECT_AFTER_TAB("SELECT `vi_ew`.`col", "SELECT `vi_ew`.`col_umn`");
  EXPECT_AFTER_TAB("SELECT actest.`vi_ew`.`col",
                   "SELECT actest.`vi_ew`.`col_umn`");

  EXPECT_AFTER_TAB("SELECT \"vi_ew\".\"col", "SELECT \"vi_ew\".\"col_umn\"");
  EXPECT_AFTER_TAB("SELECT `actest`.\"vi_ew\".\"col",
                   "SELECT `actest`.\"vi_ew\".\"col_umn\"");
}

TEST_F(Completer_frontend, quote_internal_column) {
  connect_classic();
  execute("\\sql");
  execute("\\use actest");

  EXPECT_AFTER_TAB("ALTER TABLE tab_le ALTER COLUMN col",
                   "ALTER TABLE tab_le ALTER COLUMN col_umn");
  EXPECT_AFTER_TAB("ALTER TABLE actest.tab_le ALTER COLUMN `col",
                   "ALTER TABLE actest.tab_le ALTER COLUMN `col_umn`");
  EXPECT_TAB_DOES_NOTHING("ALTER TABLE tab_le ALTER COLUMN \"col");

  execute("SET @@sql_mode = 'ANSI_QUOTES';");
  execute("\\rehash");

  EXPECT_AFTER_TAB("ALTER TABLE actest.tab_le ALTER COLUMN col",
                   "ALTER TABLE actest.tab_le ALTER COLUMN col_umn");
  EXPECT_AFTER_TAB("ALTER TABLE tab_le ALTER COLUMN `col",
                   "ALTER TABLE tab_le ALTER COLUMN `col_umn`");
  EXPECT_AFTER_TAB("ALTER TABLE actest.tab_le ALTER COLUMN \"col",
                   "ALTER TABLE actest.tab_le ALTER COLUMN \"col_umn\"");
}

TEST_F(Completer_frontend, quote_procedure) {
  connect_classic();
  execute("\\sql");
  execute("CREATE PROCEDURE actest.pro_cedure() BEGIN END;");
  execute("\\use actest");

  EXPECT_AFTER_TAB("CALL pro", "CALL pro_cedure()");
  EXPECT_AFTER_TAB("CALL `actest`.pro", "CALL `actest`.pro_cedure()");

  EXPECT_AFTER_TAB("CALL `pro", "CALL `pro_cedure`()");
  EXPECT_AFTER_TAB("CALL actest.`pro", "CALL actest.`pro_cedure`()");

  EXPECT_TAB_DOES_NOTHING("CALL \"pro");
  EXPECT_TAB_DOES_NOTHING("CALL actest.\"pro");

  execute("SET @@sql_mode = 'ANSI_QUOTES';");
  execute("\\rehash");

  EXPECT_AFTER_TAB("CALL pro", "CALL pro_cedure()");
  EXPECT_AFTER_TAB("CALL \"actest\".pro", "CALL \"actest\".pro_cedure()");

  EXPECT_AFTER_TAB("CALL `pro", "CALL `pro_cedure`()");
  EXPECT_AFTER_TAB("CALL actest.`pro", "CALL actest.`pro_cedure`()");

  EXPECT_AFTER_TAB("CALL \"pro", "CALL \"pro_cedure\"()");
  EXPECT_AFTER_TAB("CALL `actest`.\"pro", "CALL `actest`.\"pro_cedure\"()");
}

TEST_F(Completer_frontend, quote_function) {
  // BUG#34342966
  connect_classic();
  execute("\\sql");
  execute(
      "CREATE FUNCTION actest.fun_ction() RETURNS int DETERMINISTIC return 7;");
  execute("\\use actest");

  EXPECT_AFTER_TAB("SELECT fun", "SELECT fun_ction()");
  EXPECT_AFTER_TAB("SELECT `actest`.fun", "SELECT `actest`.fun_ction()");

  EXPECT_AFTER_TAB("SELECT `fun", "SELECT `fun_ction`()");
  EXPECT_AFTER_TAB("SELECT actest.`fun", "SELECT actest.`fun_ction`()");

  EXPECT_TAB_DOES_NOTHING("SELECT \"fun");
  EXPECT_TAB_DOES_NOTHING("SELECT actest.\"fun");

  execute("SET @@sql_mode = 'ANSI_QUOTES';");
  execute("\\rehash");

  EXPECT_AFTER_TAB("SELECT fun", "SELECT fun_ction()");
  EXPECT_AFTER_TAB("SELECT \"actest\".fun", "SELECT \"actest\".fun_ction()");

  EXPECT_AFTER_TAB("SELECT `fun", "SELECT `fun_ction`()");
  EXPECT_AFTER_TAB("SELECT actest.`fun", "SELECT actest.`fun_ction`()");

  EXPECT_AFTER_TAB("SELECT \"fun", "SELECT \"fun_ction\"()");
  EXPECT_AFTER_TAB("SELECT `actest`.\"fun", "SELECT `actest`.\"fun_ction\"()");
}

TEST_F(Completer_frontend, quote_trigger) {
  connect_classic();
  execute("\\sql");
  execute(
      "CREATE TRIGGER actest.trig_ger BEFORE INSERT ON tab_le FOR EACH ROW "
      "BEGIN END;");
  execute("\\use actest");

  EXPECT_AFTER_TAB("DROP TRIGGER trig", "DROP TRIGGER trig_ger");
  EXPECT_AFTER_TAB("DROP TRIGGER `actest`.trig",
                   "DROP TRIGGER `actest`.trig_ger");

  EXPECT_AFTER_TAB("DROP TRIGGER `trig", "DROP TRIGGER `trig_ger`");
  EXPECT_AFTER_TAB("DROP TRIGGER actest.`trig",
                   "DROP TRIGGER actest.`trig_ger`");

  EXPECT_TAB_DOES_NOTHING("DROP TRIGGER \"trig");
  EXPECT_TAB_DOES_NOTHING("DROP TRIGGER actest.\"trig");

  execute("SET @@sql_mode = 'ANSI_QUOTES';");
  execute("\\rehash");

  EXPECT_AFTER_TAB("DROP TRIGGER trig", "DROP TRIGGER trig_ger");
  EXPECT_AFTER_TAB("DROP TRIGGER \"actest\".trig",
                   "DROP TRIGGER \"actest\".trig_ger");

  EXPECT_AFTER_TAB("DROP TRIGGER `trig", "DROP TRIGGER `trig_ger`");
  EXPECT_AFTER_TAB("DROP TRIGGER actest.`trig",
                   "DROP TRIGGER actest.`trig_ger`");

  EXPECT_AFTER_TAB("DROP TRIGGER \"trig", "DROP TRIGGER \"trig_ger\"");
  EXPECT_AFTER_TAB("DROP TRIGGER `actest`.\"trig",
                   "DROP TRIGGER `actest`.\"trig_ger\"");
}

TEST_F(Completer_frontend, quote_event) {
  connect_classic();
  execute("\\sql");
  execute("CREATE EVENT actest.ev_ent ON SCHEDULE EVERY 1 YEAR DO BEGIN END;");
  execute("\\use actest");

  EXPECT_AFTER_TAB("DROP EVENT ev", "DROP EVENT ev_ent");
  EXPECT_AFTER_TAB("DROP EVENT `actest`.ev", "DROP EVENT `actest`.ev_ent");

  EXPECT_AFTER_TAB("DROP EVENT `ev", "DROP EVENT `ev_ent`");
  EXPECT_AFTER_TAB("DROP EVENT actest.`ev", "DROP EVENT actest.`ev_ent`");

  EXPECT_TAB_DOES_NOTHING("DROP EVENT \"ev");
  EXPECT_TAB_DOES_NOTHING("DROP EVENT actest.\"ev");

  execute("SET @@sql_mode = 'ANSI_QUOTES';");
  execute("\\rehash");

  EXPECT_AFTER_TAB("DROP EVENT ev", "DROP EVENT ev_ent");
  EXPECT_AFTER_TAB("DROP EVENT \"actest\".ev", "DROP EVENT \"actest\".ev_ent");

  EXPECT_AFTER_TAB("DROP EVENT `ev", "DROP EVENT `ev_ent`");
  EXPECT_AFTER_TAB("DROP EVENT actest.`ev", "DROP EVENT actest.`ev_ent`");

  EXPECT_AFTER_TAB("DROP EVENT \"ev", "DROP EVENT \"ev_ent\"");
  EXPECT_AFTER_TAB("DROP EVENT `actest`.\"ev",
                   "DROP EVENT `actest`.\"ev_ent\"");
}

TEST_F(Completer_frontend, quote_engine) {
  connect_classic();
  execute("\\sql");

  EXPECT_AFTER_TAB("CREATE TABLE t(id INT) ENGINE=Inno",
                   "CREATE TABLE t(id INT) ENGINE=InnoDB");
  EXPECT_AFTER_TAB("CREATE TABLE t(id INT) ENGINE=`Inno",
                   "CREATE TABLE t(id INT) ENGINE=`InnoDB`");
  EXPECT_TAB_DOES_NOTHING("CREATE TABLE t(id INT) ENGINE=\"Inno");

  execute("SET @@sql_mode = 'ANSI_QUOTES';");
  execute("\\rehash");

  EXPECT_AFTER_TAB("CREATE TABLE t(id INT) ENGINE=Inno",
                   "CREATE TABLE t(id INT) ENGINE=InnoDB");
  EXPECT_AFTER_TAB("CREATE TABLE t(id INT) ENGINE=`Inno",
                   "CREATE TABLE t(id INT) ENGINE=`InnoDB`");
  EXPECT_AFTER_TAB("CREATE TABLE t(id INT) ENGINE=\"Inno",
                   "CREATE TABLE t(id INT) ENGINE=\"InnoDB\"");
}

TEST_F(Completer_frontend, quote_udf) {
  if (this->_target_server_version < mysqlshdk::utils::Version(8, 0, 0)) {
    SKIP_TEST("This test requires 8.0+ server");
  }

  connect_classic();
  execute("\\sql");

  EXPECT_AFTER_TAB("SELECT mysqlx_e", "SELECT mysqlx_error()");
  EXPECT_AFTER_TAB("SELECT `mysqlx_e", "SELECT `mysqlx_error`()");
  EXPECT_TAB_DOES_NOTHING("SELECT \"mysqlx_e");

  execute("SET @@sql_mode = 'ANSI_QUOTES';");
  execute("\\rehash");

  EXPECT_AFTER_TAB("SELECT mysqlx_e", "SELECT mysqlx_error()");
  EXPECT_AFTER_TAB("SELECT `mysqlx_e", "SELECT `mysqlx_error`()");
  EXPECT_AFTER_TAB("SELECT \"mysqlx_e", "SELECT \"mysqlx_error\"()");
}

TEST_F(Completer_frontend, quote_runtime_function) {
  connect_classic();
  execute("\\sql");

  for (const std::string sql_mode : {"", "ANSI_QUOTES"}) {
    execute("SET @@sql_mode = '" + sql_mode + "';");
    execute("\\rehash");

    EXPECT_AFTER_TAB("SELECT current_u", "SELECT CURRENT_USER()");
    EXPECT_TAB_DOES_NOTHING("SELECT `current_u");
    EXPECT_TAB_DOES_NOTHING("SELECT \"current_u");
  }
}

TEST_F(Completer_frontend, logfile_group) {
#if defined(NDEBUG)
  SKIP_TEST("This test requires a debug build.");
#else
  DBUG_SET("+d,sql_auto_completion_logfile_group");

  connect_classic();
  execute("\\sql");

  EXPECT_AFTER_TAB("DROP LOGFILE GROUP log_",
                   "DROP LOGFILE GROUP log_file_group");
  EXPECT_AFTER_TAB("DROP LOGFILE GROUP `log_",
                   "DROP LOGFILE GROUP `log_file_group`");
  EXPECT_TAB_DOES_NOTHING("DROP LOGFILE GROUP \"log_");

  execute("SET @@sql_mode = 'ANSI_QUOTES';");
  execute("\\rehash");

  EXPECT_AFTER_TAB("DROP LOGFILE GROUP log_",
                   "DROP LOGFILE GROUP log_file_group");
  EXPECT_AFTER_TAB("DROP LOGFILE GROUP `log_",
                   "DROP LOGFILE GROUP `log_file_group`");
  EXPECT_AFTER_TAB("DROP LOGFILE GROUP \"log_",
                   "DROP LOGFILE GROUP \"log_file_group\"");

  DBUG_SET("");
#endif
}

TEST_F(Completer_frontend, quote_user_var) {
  connect_classic();
  execute("\\sql");
  execute("set @ab_cd = 1234;");
  execute("\\rehash");

  for (const std::string sql_mode : {"", "ANSI_QUOTES"}) {
    execute("SET @@sql_mode = '" + sql_mode + "';");
    execute("\\rehash");

    EXPECT_AFTER_TAB("SELECT @ab", "SELECT @ab_cd");
    EXPECT_AFTER_TAB("SELECT @'ab", "SELECT @'ab_cd'");
    EXPECT_AFTER_TAB("SELECT @\"ab", "SELECT @\"ab_cd\"");
    EXPECT_AFTER_TAB("SELECT @`ab", "SELECT @`ab_cd`");

    EXPECT_AFTER_TAB("SET @ab", "SET @ab_cd");
    EXPECT_AFTER_TAB("SET @'ab", "SET @'ab_cd'");
    EXPECT_AFTER_TAB("SET @\"ab", "SET @\"ab_cd\"");
    EXPECT_AFTER_TAB("SET @`ab", "SET @`ab_cd`");
  }
}

TEST_F(Completer_frontend, quote_sys_var) {
  // BUG#34343266
  connect_classic();
  execute("\\sql");

  for (const std::string sql_mode : {"", "ANSI_QUOTES"}) {
    execute("SET @@sql_mode = '" + sql_mode + "';");
    execute("\\rehash");

    EXPECT_AFTER_TAB("SELECT @@sql_log_b", "SELECT @@sql_log_bin");
    EXPECT_AFTER_TAB("SELECT @@`sql_log_b", "SELECT @@`sql_log_bin`");
    EXPECT_TAB_DOES_NOTHING("SELECT @@'sql_log_b");
    EXPECT_TAB_DOES_NOTHING("SELECT @@\"sql_log_b");

    EXPECT_AFTER_TAB("SELECT @@session.sql_log_b",
                     "SELECT @@session.sql_log_bin");
    EXPECT_AFTER_TAB("SELECT @@session.`sql_log_b",
                     "SELECT @@session.`sql_log_bin`");
    EXPECT_TAB_DOES_NOTHING("SELECT @@session.'sql_log_b");
    EXPECT_TAB_DOES_NOTHING("SELECT @@session.\"sql_log_b");

    EXPECT_AFTER_TAB("SET sql_log_b", "SET sql_log_bin");
    EXPECT_AFTER_TAB("SET `sql_log_b", "SET `sql_log_bin`");
    EXPECT_TAB_DOES_NOTHING("SET 'sql_log_b");
    EXPECT_TAB_DOES_NOTHING("SET \"sql_log_b");

    EXPECT_AFTER_TAB("SET PERSIST sql_log_b", "SET PERSIST sql_log_bin");
    EXPECT_AFTER_TAB("SET PERSIST `sql_log_b", "SET PERSIST `sql_log_bin`");
    EXPECT_TAB_DOES_NOTHING("SET PERSIST 'sql_log_b");
    EXPECT_TAB_DOES_NOTHING("SET PERSIST \"sql_log_b");

    EXPECT_AFTER_TAB("SET @@sql_log_b", "SET @@sql_log_bin");
    EXPECT_AFTER_TAB("SET @@`sql_log_b", "SET @@`sql_log_bin`");
    EXPECT_TAB_DOES_NOTHING("SET @@'sql_log_b");
    EXPECT_TAB_DOES_NOTHING("SET @@\"sql_log_b");

    EXPECT_AFTER_TAB("SET @@session.sql_log_b", "SET @@session.sql_log_bin");
    EXPECT_AFTER_TAB("SET @@session.`sql_log_b", "SET @@session.`sql_log_bin`");
    EXPECT_TAB_DOES_NOTHING("SET @@session.'sql_log_b");
    EXPECT_TAB_DOES_NOTHING("SET @@session.\"sql_log_b");
  }
}

TEST_F(Completer_frontend, quote_tablespace) {
  connect_classic();
  execute("\\sql");
  execute("CREATE TABLESPACE table_space ADD DATAFILE 'table_space.ibd';");
  execute("\\rehash");

  EXPECT_AFTER_TAB("DROP TABLESPACE table_", "DROP TABLESPACE table_space");
  EXPECT_AFTER_TAB("DROP TABLESPACE `table_", "DROP TABLESPACE `table_space`");
  EXPECT_TAB_DOES_NOTHING("DROP TABLESPACE \"table_");

  execute("SET @@sql_mode = 'ANSI_QUOTES';");
  execute("\\rehash");

  EXPECT_AFTER_TAB("DROP TABLESPACE table_", "DROP TABLESPACE table_space");
  EXPECT_AFTER_TAB("DROP TABLESPACE `table_", "DROP TABLESPACE `table_space`");
  EXPECT_AFTER_TAB("DROP TABLESPACE \"table_",
                   "DROP TABLESPACE \"table_space\"");

  execute("DROP TABLESPACE table_space;");
}

TEST_F(Completer_frontend, quote_user) {
  // BUG#34360367
  connect_classic();
  execute("\\sql");

  execute("CREATE USER us_er@ho_st.com;");

  for (const std::string sql_mode : {"", "ANSI_QUOTES"}) {
    execute("SET @@sql_mode = '" + sql_mode + "';");
    execute("\\rehash");

    EXPECT_TAB_TAB_CONTAINS("ALTER USER ", "'us_er'@'ho_st.com'");
    EXPECT_TAB_TAB_CONTAINS("ALTER USER u", "us_er@ho_st.com");
    EXPECT_TAB_TAB_CONTAINS("ALTER USER us", "us_er@ho_st.com");
    EXPECT_TAB_TAB_CONTAINS("ALTER USER us_", "us_er@ho_st.com");
    EXPECT_TAB_TAB_CONTAINS("ALTER USER us_e", "us_er@ho_st.com");
    EXPECT_TAB_TAB_CONTAINS("ALTER USER us_er", "us_er@ho_st.com");
    EXPECT_TAB_TAB_CONTAINS("ALTER USER us_er@", "us_er@ho_st.com");
    EXPECT_TAB_TAB_CONTAINS("ALTER USER us_er@h", "us_er@ho_st.com");
    EXPECT_TAB_TAB_CONTAINS("ALTER USER us_er@ho", "us_er@ho_st.com");
    EXPECT_TAB_TAB_CONTAINS("ALTER USER us_er@ho_", "us_er@ho_st.com");
    EXPECT_TAB_TAB_CONTAINS("ALTER USER us_er@ho_s", "us_er@ho_st.com");
    EXPECT_TAB_TAB_CONTAINS("ALTER USER us_er@ho_st", "us_er@ho_st.com");
    EXPECT_TAB_TAB_CONTAINS("ALTER USER us_er@ho_st.c", "us_er@ho_st.com");
    EXPECT_TAB_TAB_CONTAINS("ALTER USER us_er@ho_st.co", "us_er@ho_st.com");

    EXPECT_AFTER_TAB("ALTER USER us_", "ALTER USER us_er@ho_st.com");
    EXPECT_AFTER_TAB("ALTER USER `us_", "ALTER USER `us_er`@`ho_st.com`");
    EXPECT_AFTER_TAB("ALTER USER 'us_", "ALTER USER 'us_er'@'ho_st.com'");
    EXPECT_AFTER_TAB("ALTER USER \"us_", "ALTER USER \"us_er\"@\"ho_st.com\"");

    EXPECT_AFTER_TAB("ALTER USER us_er@ho_", "ALTER USER us_er@ho_st.com");
    EXPECT_AFTER_TAB("ALTER USER `us_er`@'ho_",
                     "ALTER USER `us_er`@'ho_st.com'");
    EXPECT_AFTER_TAB("ALTER USER us_er@`ho_", "ALTER USER us_er@`ho_st.com`");
    EXPECT_AFTER_TAB("ALTER USER \"us_er\"@\"ho_",
                     "ALTER USER \"us_er\"@\"ho_st.com\"");
  }

  execute("DROP USER us_er@ho_st.com;");
}

TEST_F(Completer_frontend, quote_charset) {
  connect_classic();
  execute("\\sql");

  for (const std::string sql_mode : {"", "ANSI_QUOTES"}) {
    execute("SET @@sql_mode = '" + sql_mode + "';");
    execute("\\rehash");

    EXPECT_AFTER_TAB("LOAD DATA INFILE 'f' INTO TABLE t CHARACTER SET asc",
                     "LOAD DATA INFILE 'f' INTO TABLE t CHARACTER SET ascii");
    EXPECT_AFTER_TAB("LOAD DATA INFILE 'f' INTO TABLE t CHARACTER SET 'asc",
                     "LOAD DATA INFILE 'f' INTO TABLE t CHARACTER SET 'ascii'");
    EXPECT_AFTER_TAB(
        "LOAD DATA INFILE 'f' INTO TABLE t CHARACTER SET \"asc",
        "LOAD DATA INFILE 'f' INTO TABLE t CHARACTER SET \"ascii\"");
    EXPECT_AFTER_TAB("LOAD DATA INFILE 'f' INTO TABLE t CHARACTER SET `asc",
                     "LOAD DATA INFILE 'f' INTO TABLE t CHARACTER SET `ascii`");
  }
}

TEST_F(Completer_frontend, quote_collation) {
  connect_classic();
  execute("\\sql");

  for (const std::string sql_mode : {"", "ANSI_QUOTES"}) {
    execute("SET @@sql_mode = '" + sql_mode + "';");
    execute("\\rehash");

    EXPECT_AFTER_TAB("CREATE TABLE t(c CHAR(32) COLLATE ascii_b",
                     "CREATE TABLE t(c CHAR(32) COLLATE ascii_bin");
    EXPECT_AFTER_TAB("CREATE TABLE t(c CHAR(32) COLLATE 'ascii_b",
                     "CREATE TABLE t(c CHAR(32) COLLATE 'ascii_bin'");
    EXPECT_AFTER_TAB("CREATE TABLE t(c CHAR(32) COLLATE \"ascii_b",
                     "CREATE TABLE t(c CHAR(32) COLLATE \"ascii_bin\"");
    EXPECT_AFTER_TAB("CREATE TABLE t(c CHAR(32) COLLATE `ascii_b",
                     "CREATE TABLE t(c CHAR(32) COLLATE `ascii_bin`");
  }
}

TEST_F(Completer_frontend, quote_plugin) {
  connect_classic();
  execute("\\sql");

  EXPECT_AFTER_TAB("UNINSTALL PLUGIN mysql_n",
                   "UNINSTALL PLUGIN mysql_native_password");
  EXPECT_AFTER_TAB("UNINSTALL PLUGIN `mysql_n",
                   "UNINSTALL PLUGIN `mysql_native_password`");
  EXPECT_TAB_DOES_NOTHING("UNINSTALL PLUGIN \"mysql_n");

  execute("SET @@sql_mode = 'ANSI_QUOTES';");
  execute("\\rehash");

  EXPECT_AFTER_TAB("UNINSTALL PLUGIN mysql_n",
                   "UNINSTALL PLUGIN mysql_native_password");
  EXPECT_AFTER_TAB("UNINSTALL PLUGIN `mysql_n",
                   "UNINSTALL PLUGIN `mysql_native_password`");
  EXPECT_AFTER_TAB("UNINSTALL PLUGIN \"mysql_n",
                   "UNINSTALL PLUGIN \"mysql_native_password\"");
}

TEST_F(Completer_frontend, quote_labels) {
  connect_classic();
  execute("\\sql");

  EXPECT_AFTER_TAB(
      "CREATE PROCEDURE p(i INT) BEGIN label1: LOOP SET i = i + 1; IF i < 10 "
      "THEN ITERATE lab",
      "CREATE PROCEDURE p(i INT) BEGIN label1: LOOP SET i = i + 1; IF i < 10 "
      "THEN ITERATE label1");
  EXPECT_AFTER_TAB(
      "CREATE PROCEDURE p(i INT) BEGIN label1: LOOP SET i = i + 1; IF i < 10 "
      "THEN ITERATE `lab",
      "CREATE PROCEDURE p(i INT) BEGIN label1: LOOP SET i = i + 1; IF i < 10 "
      "THEN ITERATE `label1`");
  EXPECT_TAB_DOES_NOTHING(
      "CREATE PROCEDURE p(i INT) BEGIN label1: LOOP SET i = i + 1; IF i < 10 "
      "THEN ITERATE \"lab");

  execute("SET @@sql_mode = 'ANSI_QUOTES';");
  execute("\\rehash");

  EXPECT_AFTER_TAB(
      "CREATE PROCEDURE p(i INT) BEGIN label1: LOOP SET i = i + 1; IF i < 10 "
      "THEN ITERATE lab",
      "CREATE PROCEDURE p(i INT) BEGIN label1: LOOP SET i = i + 1; IF i < 10 "
      "THEN ITERATE label1");
  EXPECT_AFTER_TAB(
      "CREATE PROCEDURE p(i INT) BEGIN label1: LOOP SET i = i + 1; IF i < 10 "
      "THEN ITERATE `lab",
      "CREATE PROCEDURE p(i INT) BEGIN label1: LOOP SET i = i + 1; IF i < 10 "
      "THEN ITERATE `label1`");
  EXPECT_AFTER_TAB(
      "CREATE PROCEDURE p(i INT) BEGIN label1: LOOP SET i = i + 1; IF i < 10 "
      "THEN ITERATE \"lab",
      "CREATE PROCEDURE p(i INT) BEGIN label1: LOOP SET i = i + 1; IF i < 10 "
      "THEN ITERATE \"label1\"");
}

TEST_F(Completer_frontend, bug_34372040) {
  connect_classic();
  execute("\\sql");

  EXPECT_AFTER_TAB("SHOW SL", "SHOW SLAVE");
  EXPECT_AFTER_TAB_TAB("SHOW SLAVE ", strv({"HOSTS", "STATUS"}));

  if (_target_server_version >= mysqlshdk::utils::Version(8, 0, 0)) {
    EXPECT_AFTER_TAB_TAB("SHOW REPLICA", strv({"REPLICA", "REPLICAS"}));
    EXPECT_AFTER_TAB("SHOW REPLICA ", "SHOW REPLICA STATUS");
    EXPECT_TAB_DOES_NOTHING("SHOW REPLICAS ");
    EXPECT_TAB_DOES_NOTHING("SHOW REPLICA H");
  }
}

TEST_F(Completer_frontend, bug_34370621) {
  if (this->_target_server_version < mysqlshdk::utils::Version(8, 0, 0)) {
    SKIP_TEST("This test requires 8.0+ server");
  }

  connect_classic();
  execute("\\sql");

  EXPECT_AFTER_TAB("RESET PERSIST sql_log_b", "RESET PERSIST sql_log_bin");
  EXPECT_AFTER_TAB("RESET PERSIST IF EXISTS sql_log_b",
                   "RESET PERSIST IF EXISTS sql_log_bin");
}

TEST_F(Completer_frontend, bug_34343279) {
  if (this->_target_server_version < mysqlshdk::utils::Version(8, 0, 0)) {
    SKIP_TEST("This test requires 8.0+ server");
  }

  connect_classic();
  execute("\\sql");

  EXPECT_AFTER_TAB_TAB("ALTER INSTANCE ",
                       strv({"DISABLE", "ENABLE", "RELOAD", "ROTATE"}));
  EXPECT_AFTER_TAB_TAB("ALTER INSTANCE RELOAD ", strv({"KEYRING", "TLS"}));
}

TEST_F(Completer_frontend, bug_34353732) {
  connect_classic();
  execute("\\sql");

  EXPECT_AFTER_TAB_TAB("SELECT JSON_ARRAY",
                       strv({"JSON_ARRAY()", "JSON_ARRAY_APPEND()",
                             "JSON_ARRAY_INSERT()", "JSON_ARRAYAGG()"}));
  EXPECT_AFTER_TAB_TAB("SELECT JSON_ARRAY_",
                       strv({"JSON_ARRAY_APPEND()", "JSON_ARRAY_INSERT()"}));
}

TEST_F(Completer_frontend, bug_34342946) {
  connect_classic("mysql");
  execute("\\sql");

  // BUG#34370211
  EXPECT_TAB_DOES_NOTHING("USE mysql ");

  EXPECT_TAB_TAB_NOT_CONTAINS("SELECT mysql.plugin.name ", "dl");
  EXPECT_TAB_TAB_NOT_CONTAINS("SELECT mysql.plugin.name ", "name");

  EXPECT_TAB_TAB_NOT_CONTAINS("CREATE DEFINER='root'@'localhost' ",
                              "'root'@'localhost'");
}

TEST_F(Completer_frontend, bug_34365581) {
  connect_classic();
  execute("\\sql");

  execute("SELECT @@GLOBAL.lower_case_table_names\\G");

  if (std::string::npos ==
      output_handler.std_out.find("@@GLOBAL.lower_case_table_names: 0")) {
    SKIP_TEST("This test requires case-sensitive MySQL server");
  }

  execute("CREATE SCHEMA IF NOT EXISTS ogórek;");
  execute("CREATE SCHEMA IF NOT EXISTS ogÓrek;");
  execute("\\rehash");

  EXPECT_AFTER_TAB_TAB("SELECT ogó", strv({"ogÓrek", "ogórek"}));

  execute("DROP SCHEMA ogórek;");
  execute("DROP SCHEMA ogÓrek;");
}

}  // namespace mysqlsh
