/*
 * Copyright (c) 2018, 2021, Oracle and/or its affiliates.
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

#include "modules/adminapi/mod_dba.h"
#include "modules/adminapi/mod_dba_cluster.h"
#include "modules/mod_shell.h"
#include "modules/mod_utils.h"
#include "modules/util/dump/dump_instance_options.h"
#include "modules/util/dump/dump_schemas_options.h"
#include "modules/util/dump/dump_tables_options.h"
#include "modules/util/import_table/import_table_options.h"
#include "modules/util/mod_util.h"
#include "mysqlshdk/include/scripting/type_info/custom.h"
#include "mysqlshdk/include/scripting/type_info/generic.h"
#include "mysqlshdk/include/scripting/types_cpp.h"
#include "mysqlshdk/include/shellcore/shell_options.h"
#include "mysqlshdk/shellcore/shell_cli_operation.h"
#include "unittest/mysqlshdk/scripting/test_option_packs.h"
#include "unittest/test_utils.h"

namespace shcore {
namespace cli {
class CLI_integration_tester : public shcore::Cpp_object_bridge {
 public:
  CLI_integration_tester() {
    expose("testLists", &CLI_integration_tester::test_list_params, "one",
           "?two")
        ->cli();
    expose("testDictionaryWithOptions",
           &CLI_integration_tester::test_dictionary, "one")
        ->cli();
    expose("testDictionaryWithNoOptions",
           &CLI_integration_tester::test_dictionary_no_options, "one")
        ->cli();
    expose("testTwoDictionariesSameOptions",
           &CLI_integration_tester::test_dictionaries, "one", "two")
        ->cli();
    expose("testTypeValidation", &CLI_integration_tester::test_type_validation,
           "myInt", "myString", "myList", "myNamedDouble", "myNamedBool",
           "myNamedValue")
        ->cli();
  }
  void test_list_params(const shcore::Array_t & /*one*/,
                        const shcore::Array_t & /*two*/ = shcore::Array_t()) {}
  void test_dictionary(
      const shcore::Option_pack_ref<tests::Sample_options> & /*one*/) {}
  void test_dictionary_no_options(const shcore::Dictionary_t & /*one*/) {}
  void test_dictionaries(
      const shcore::Option_pack_ref<tests::Sample_options> & /*one*/,
      const shcore::Option_pack_ref<tests::Sample_options> & /*two*/) {}
  void test_type_validation(int /*myInt*/, const std::string & /*myString*/,
                            const std::vector<std::string> & /*myList*/,
                            double /*myNamedDouble*/, bool /*myNamedBool*/,
                            const shcore::Value & /*myNamedValue*/) {}
};

class Shell_cli_operation_test : public Shell_core_test_wrapper,
                                 public Shell_cli_operation {
 public:
  void SetUp() override {
    Shell_core_test_wrapper::SetUp();

    get_provider()->register_provider("util", [this](bool /*for_help*/) {
      return _interactive_shell->shell_context()
          ->get_global("util")
          .as_object<shcore::Cpp_object_bridge>();
    });

    get_provider()->register_provider("dba", [this](bool /*for_help*/) {
      return _interactive_shell->shell_context()
          ->get_global("dba")
          .as_object<shcore::Cpp_object_bridge>();
    });

    get_provider()->register_provider("cluster", [](bool /*for_help*/) {
      return std::make_shared<mysqlsh::dba::Cluster>(nullptr);
    });

    get_provider()->register_provider("rs", [](bool /*for_help*/) {
      return std::make_shared<mysqlsh::dba::ReplicaSet>(nullptr);
    });

    auto shell_provider =
        get_provider()->register_provider("shell", [this](bool /*for_help*/) {
          return _interactive_shell->shell_context()
              ->get_global("shell")
              .as_object<mysqlsh::Shell>();
        });

    shell_provider->register_provider("options", [this](bool /*for_help*/) {
      return _interactive_shell->shell_context()
          ->get_global("shell")
          .as_object<mysqlsh::Shell>()
          ->get_shell_options();
    });

    get_provider()->register_provider("cli", [](bool /*for_help*/) {
      return std::make_shared<CLI_integration_tester>();
    });
  }

  void parse(Options::Cmdline_iterator *iterator) {
    m_object_name.clear();
    m_method_name.clear();
    m_method_name_original.clear();
    m_argument_list.clear();
    m_cli_mapper.clear();

    Shell_cli_operation::parse(iterator);
  }

  void test_list_params(int argc, const char *argv[],
                        const shcore::Array_t &first,
                        const shcore::Array_t &second) {
    Options::Cmdline_iterator it(argc, argv, 0);

    parse(&it);
    prepare();

    // If the second list is empty, it means there were no arguments passed
    // for it and so the CLI mapper will NOT provide this argument to the API
    // call
    EXPECT_EQ(second ? 2 : 1, m_argument_list.size());
    EXPECT_EQ(shcore::Value(first), m_argument_list.at(0));

    // If there were arguments for the second list, they should be mapped
    // properly
    if (second) {
      EXPECT_EQ(shcore::Value(second), m_argument_list.at(1));
    }
  }

  void validate_dictionary(const char *file, int line,
                           const shcore::Value &actual_value,
                           const shcore::Value &expected_value) {
    if (expected_value != actual_value) {
      std::string error = "Mistmatch on parsed arguments:\n --Expected: " +
                          expected_value.json() +
                          "\n --Actual: " + actual_value.json();
      SCOPED_TRACE(error.c_str());
      ADD_FAILURE_AT(file, line);
    }
  }

#define TEST_DICTIONARY(a, b, x) test_dictionary(__FILE__, __LINE__, a, b, x)

  void test_dictionary(const char *file, int line, int argc, const char *argv[],
                       const shcore::Dictionary_t &expected) {
    Options::Cmdline_iterator it(argc, argv, 0);

    parse(&it);
    prepare();

    validate_dictionary(file, line, m_argument_list.at(0),
                        shcore::Value(expected));
  }

#define TEST_DICTIONARIES(a, b, x, y) \
  test_dictionaries(__FILE__, __LINE__, a, b, x, y)

  void test_dictionaries(const char *file, int line, int argc,
                         const char *argv[],
                         const shcore::Dictionary_t &expected1,
                         const shcore::Dictionary_t &expected2) {
    Options::Cmdline_iterator it(argc, argv, 0);

    parse(&it);
    prepare();

    validate_dictionary(file, line, m_argument_list.at(0),
                        shcore::Value(expected1));
    validate_dictionary(file, line, m_argument_list.at(1),
                        shcore::Value(expected2));
  }

#define VALIDATE_ARG_LIST(x, y) validate_arg_list(__FILE__, __LINE__, x, y)

  void validate_arg_list(const char *file, int line,
                         const shcore::Array_t &expected,
                         const shcore::Argument_list &args) {
    auto actual = shcore::make_array();
    for (const auto &arg : args) {
      actual->push_back(arg);
    }

    auto expected_value = shcore::Value(expected);
    auto actual_value = shcore::Value(actual);

    if (expected_value != actual_value) {
      std::string error = "Mistmatch on parsed arguments:\n --Expected: " +
                          expected_value.json() +
                          "\n --Actual: " + actual_value.json();
      SCOPED_TRACE(error.c_str());
      ADD_FAILURE_AT(file, line);
    }
  }
};

TEST_F(Shell_cli_operation_test, ut_list_params) {
  // Tests where NO data is provided for the second list
  auto simple_items = shcore::make_array("my", "sample", "list");
  auto complex_items =
      shcore::make_array("yet another", "list", "using quotes");
  {
    const char *argv[] = {"cli", "test-lists", "my,sample,list"};
    test_list_params(3, argv, simple_items, shcore::Array_t());
  }
  {
    const char *argv[] = {"cli", "test-lists",
                          "'yet another','list','using quotes'"};
    test_list_params(3, argv, complex_items, shcore::Array_t());
  }
  {
    const char *argv[] = {"cli", "test-lists",
                          "\"yet another\",\"list\",\"using quotes\""};
    test_list_params(3, argv, complex_items, shcore::Array_t());
  }
  {
    const char *argv[] = {"cli", "test-lists",
                          "'yet another',list,\"using quotes\""};
    test_list_params(3, argv, complex_items, shcore::Array_t());
  }

  // Parameters after first list become named params so its value can not be
  // specified in a positional way
  {
    const char *argv[] = {"cli", "test-lists",
                          "'yet another',list,\"using quotes\"", "-"};

    test_list_params(
        4, argv, shcore::make_array("yet another", "list", "using quotes", "-"),
        shcore::Array_t());
  }
  {
    const char *argv[] = {"cli", "test-lists", "-", "-"};
    test_list_params(4, argv, shcore::make_array("-", "-"), shcore::Array_t());
  }
  // First list is positional, so it takes the positional args
  {
    const char *argv[] = {"cli", "test-lists", "-",
                          "--two='yet another',list,\"using quotes\""};
    test_list_params(4, argv, shcore::make_array("-"), complex_items);
  }

  // Tests for data in both lists, first list takes any positional args after it
  // starts. Secon list becomes named argument
  {
    const char *argv[] = {"cli",          "test-lists",   "yet another",
                          "list",         "using quotes", "--two=my",
                          "--two=sample", "--two",        "list"};
    test_list_params(9, argv, complex_items, simple_items);
  }
}

TEST_F(Shell_cli_operation_test, ut_dict_param_list_option) {
  // Testing a single list option
  auto expected_list = shcore::make_array("item", "in", "list");
  auto expected_dict = shcore::make_dict("myList", expected_list);
  {
    const char *argv[] = {"cli", "test-dictionary-with-options",
                          "--my-list=item,in,list"};

    TEST_DICTIONARY(3, argv, expected_dict);
  }
  // Lists options can be specified several times, values get aggregated
  expected_list->emplace_back("another set");
  expected_list->emplace_back("of values");
  {
    const char *argv[] = {"cli", "test-dictionary-with-options",
                          "--my-list=item,in,list",
                          "--my-list='another set','of values'"};

    TEST_DICTIONARY(4, argv, expected_dict);
  }
  // It also works with the short name option
  expected_list->emplace_back("a final");
  expected_list->emplace_back("element");
  {
    const char *argv[] = {
        "cli", "test-dictionary-with-options", "--my-list=item,in,list",
        "--my-list='another set','of values'", "-l'a final',element"};

    TEST_DICTIONARY(5, argv, expected_dict);
  }
}

TEST_F(Shell_cli_operation_test, ut_dict_param) {
  auto expected =
      shcore::make_dict("myString", "sample", "myInt", 5, "myBool", true);
  // Tests long named options
  {
    const char *argv[] = {"cli", "test-dictionary-with-options",
                          "--my-string=sample", "--my-int=5", "--my-bool"};
    TEST_DICTIONARY(5, argv, expected);
  }
  // Tests camelCase options
  {
    const char *argv[] = {"cli", "test-dictionary-with-options",
                          "--myString=sample", "--myInt=5", "--myBool"};
    TEST_DICTIONARY(5, argv, expected);
  }
  // Tests short named options
  {
    const char *argv[] = {"cli", "test-dictionary-with-options", "-ssample",
                          "-i5", "-b"};
    TEST_DICTIONARY(5, argv, expected);
  }
  // Tests invalid option
  {
    const char *argv[] = {"cli",
                          "test-dictionary-with-options",
                          "--my-string=sample",
                          "--my-int=5",
                          "--my-bool",
                          "--my-unregistered"};

    EXPECT_THROW_LIKE(TEST_DICTIONARY(6, argv, {}), std::invalid_argument,
                      "The following option is invalid: --my-unregistered");
  }
  // Tests without options defined, everything is mapped as strings except
  // boolean options set as --option-name or -o
  {
    const char *argv[] = {"cli",
                          "test-dictionary-with-no-options",
                          "--my-string=sample",
                          "--my-int=5",
                          "--my-bool",
                          "--true-bool1=1",
                          "--true-bool2=true",
                          "--false-bool1=0",
                          "--false-bool2=false",
                          "-a",
                          "-b1",
                          "-ctrue",
                          "-d0",
                          "-efalse"};

    TEST_DICTIONARY(
        14, argv,
        shcore::make_dict("myString", "sample", "myInt", 5, "myBool", true,
                          "trueBool1", 1, "trueBool2", true, "falseBool1", 0,
                          "falseBool2", false, "a", true, "b", 1, "c", true,
                          "d", 0, "e", false));
  }
}

TEST_F(Shell_cli_operation_test, ut_dict_params_ambiguous_options) {
  {
    const char *argv[] = {"cli", "test-two-dictionaries-same-options",
                          "--my-string=sample"};
    EXPECT_THROW_LIKE(
        TEST_DICTIONARY(3, argv, {}), std::invalid_argument,
        "Option --my-string is ambiguous as it is valid for the following "
        "parameters: one, two. Please use any of the following options to "
        "disambiguate: --one-my-string, --two-my-string");
  }
  {
    const char *argv[] = {"cli", "test-two-dictionaries-same-options",
                          "--one-my-string=parameterOne",
                          "--two-my-string=parameterTwo"};

    TEST_DICTIONARIES(4, argv, shcore::make_dict("myString", "parameterOne"),
                      shcore::make_dict("myString", "parameterTwo"));
  }
}

TEST_F(Shell_cli_operation_test, ut_dict_option) {
  // Tests long named options
  {
    const char *argv[] = {
        "cli",
        "test-dictionary-with-options",
        "--my-dict=sampleAtt=1",
        "--my-dict=integerAtt:int=5",
        "--my-dict=booleanAtt:bool=1",
        "--my-dict=floatAtt:float=5.7",
        "--my-dict=jsonArrayAtt:json=[1,2,3]",
        "--my-dict=jsonDocAtt:json={'jsonAtt1':'val1','jsonAtt2':2}",
        "--my-dict=nullAtt:null=null"};

    auto json_array_att = shcore::make_array();
    json_array_att->push_back(shcore::Value(1));
    json_array_att->push_back(shcore::Value(2));
    json_array_att->push_back(shcore::Value(3));

    auto json_doc_att = shcore::make_dict("jsonAtt1", shcore::Value("val1"),
                                          "jsonAtt2", shcore::Value(2));

    auto dict = shcore::make_dict(
        "myDict",
        shcore::make_dict(
            "sampleAtt", shcore::Value(1), "integerAtt", shcore::Value(5),
            "booleanAtt", shcore::Value::True(), "floatAtt", shcore::Value(5.7),
            "jsonArrayAtt", shcore::Value(json_array_att), "jsonDocAtt",
            shcore::Value(json_doc_att), "nullAtt", shcore::Value::Null()));

    TEST_DICTIONARY(9, argv, dict);
  }
}

TEST_F(Shell_cli_operation_test, test_type_parsing_with_lists) {
  // Function is defined as follows:
  //
  //  void test_type_validation(int /*myInt*/, const std::string & /*myString*/,
  //                            const std::vector<std::string> & /*myList*/,
  //                            double /*myNamedDouble*/, bool /*myNamedBool*/,
  //                            const shcore::Value& /*myNamedValue*/);

  {
    // myList will take everything after it as strings, so the call is
    // missing myNamedDouble, myNamedBool and myNamedValue
    const char *argv[] = {"cli",
                          "test-type-validation",
                          "1",
                          "sample",
                          "one",
                          "two",
                          "5.7",
                          "1",
                          "5",
                          "--my-named-double=1.0",
                          "--my-named-bool",
                          "--my-named-value=1"};

    Options::Cmdline_iterator it(12, argv, 0);
    parse(&it);
    EXPECT_NO_THROW(prepare());

    VALIDATE_ARG_LIST(
        shcore::make_array(1, "sample",
                           shcore::make_array("one", "two", "5.7", "1", "5"), 1,
                           true, 1),
        m_argument_list);
  }
  {
    // myList will take everything until next named arg is specified
    // myNamedValue is taken as string by default
    const char *argv[] = {"cli",
                          "test-type-validation",
                          "1",
                          "sample",
                          "one",
                          "two",
                          "--myNamedDouble=5.7",
                          "--myNamedBool=1",
                          "--myNamedValue=5"};

    Options::Cmdline_iterator it(9, argv, 0);
    parse(&it);
    EXPECT_NO_THROW(prepare());

    VALIDATE_ARG_LIST(
        shcore::make_array(1, "sample", shcore::make_array("one", "two"), 5.7,
                           true, 5),
        m_argument_list);
  }
  {
    // myList will take everything until next named arg is specified
    // myNamedValue can be set to a specific type
    const char *argv[] = {"cli",
                          "test-type-validation",
                          "1",
                          "sample",
                          "one",
                          "two",
                          "--myNamedDouble=5.7",
                          "--myNamedBool=1",
                          "--myNamedValue:int=5"};

    Options::Cmdline_iterator it(9, argv, 0);
    parse(&it);
    EXPECT_NO_THROW(prepare());

    VALIDATE_ARG_LIST(
        shcore::make_array(1, "sample", shcore::make_array("one", "two"), 5.7,
                           true, 5),
        m_argument_list);
  }
}

TEST_F(Shell_cli_operation_test, test_type_validation_rules) {
  // Function is defined as follows:
  //
  //  void test_type_validation(int /*myInt*/, const std::string & /*myString*/,
  //                            const std::vector<std::string> & /*myList*/,
  //                            double /*myNamedDouble*/, bool /*myNamedBool*/,
  //                            const shcore::Value& /*myNamedValue*/);

  {
    // myInt is specified with invalid integer value
    const char *argv[] = {"cli",   "test-type-validation",
                          "error", "sample",
                          "one",   "two",
                          "5.7",   "1",
                          "5"};

    Options::Cmdline_iterator it(9, argv, 0);
    parse(&it);
    EXPECT_THROW_LIKE(prepare(), std::runtime_error,
                      "The following required options are missing: "
                      "--my-named-double, --my-named-bool, --my-named-value");
  }
  {
    // myInt is specified with invalid integer value
    const char *argv[] = {"cli",
                          "test-type-validation",
                          "error",
                          "sample",
                          "one",
                          "two",
                          "5.7",
                          "1",
                          "5",
                          "--my-named-double=1.0",
                          "--my-named-bool",
                          "--my-named-value=1"};

    Options::Cmdline_iterator it(12, argv, 0);
    parse(&it);
    EXPECT_THROW_LIKE(
        prepare(), std::invalid_argument,
        "Argument error at 'error': Integer expected, but value is String");
  }
  {
    // myInt is specified with invalid integer value
    const char *argv[] = {"cli", "test-type-validation",  "5", "sample", "one",
                          "two", "--myNamedDouble:bool=1"};

    Options::Cmdline_iterator it(7, argv, 0);
    parse(&it);
    EXPECT_THROW_LIKE(prepare(), std::invalid_argument,
                      "Argument error at '--myNamedDouble:bool=1': Float "
                      "expected, but value is Bool");
  }
}

TEST_F(Shell_cli_operation_test, parse_commandline) {
  {
    const char *arg0[] = {"--"};
    Options::Cmdline_iterator it(1, arg0, 1);
    parse(&it);
    EXPECT_THROW_LIKE(prepare(), std::invalid_argument,
                      "Empty operations are not allowed");
  }
  {
    const char *arg0[] = {"whatever"};
    Options::Cmdline_iterator it(1, arg0, 0);
    parse(&it);
    EXPECT_THROW_LIKE(prepare(), std::invalid_argument,
                      "There is no object registered under name 'whatever'");
  }
  {
    const char *arg01[] = {"dba"};
    Options::Cmdline_iterator it(1, arg01, 0);
    parse(&it);
    EXPECT_THROW_LIKE(prepare(), std::invalid_argument,
                      "No operation specified for object 'dba'");
  }
  // TODO(rennox): Boolean values are taking next anonymous argument no matter
  // if they are booleans
  /*{
    const char *arg01[] = {"--help", "invalid"};
    Options::Cmdline_iterator it(2, arg01, 0);
    parse(&it);
    EXPECT_THROW_LIKE(
        prepare(), std::invalid_argument,
        "No additional arguments are allowed when requesting CLI help");
  }
  {
    const char *arg01[] = {"dba", "--help", "invalid"};
    Options::Cmdline_iterator it(3, arg01, 0);
    parse(&it);
    EXPECT_THROW_LIKE(
        prepare(), std::invalid_argument,
        "No additional arguments are allowed when requesting CLI help");
  }
  {
    const char *arg01[] = {"dba", "create-cluster", "--help", "invalid"};
    Options::Cmdline_iterator it(4, arg01, 0);
    parse(&it);
    EXPECT_THROW_LIKE(
        prepare(), std::invalid_argument,
        "No additional arguments are allowed when requesting CLI help");
  }*/
  {
    const char *arg1[] = {"util",
                          "check-for-server-upgrade",
                          "root@localhost",
                          "0",
                          "1",
                          "true",
                          "false",
                          "123456789",
                          "12345.12345",
                          "-"};
    Options::Cmdline_iterator it(10, arg1, 0);
    parse(&it);
    EXPECT_THROW_LIKE(prepare(), std::invalid_argument,
                      "The following arguments are invalid: 0, 1, true, false, "
                      "123456789, 12345.12345, -");
    EXPECT_EQ("util", m_object_name);
    EXPECT_EQ("checkForServerUpgrade", m_method_name);
    EXPECT_EQ(1, m_argument_list.size());
    EXPECT_EQ("root@localhost", m_argument_list.string_at(0));
  }
  {
    const char *arg2[] = {"dba", "deploy-sandbox-instance", "3307",
                          "--password=foo", "--sandbox-dir=null"};
    Options::Cmdline_iterator it(5, arg2, 0);
    parse(&it);
    EXPECT_THROW_LIKE(
        prepare(), std::invalid_argument,
        "Argument error at '--sandbox-dir=null': String expected, but "
        "value is Null");
  }
  {
    const char *arg4[] = {"cluster",      "addInstance",      "--port=3307",
                          "--force",      "--bool=false",     "--std-dev=0.7",
                          "--pass-word=", "--sandboxDir=/tmp"};
    Options::Cmdline_iterator it(8, arg4, 0);
    parse(&it);
    prepare();
    EXPECT_EQ("cluster", m_object_name);
    EXPECT_EQ("addInstance", m_method_name);
    EXPECT_EQ(2, m_argument_list.size());

    shcore::Dictionary_t connection_map;

    // Only the port is taken as connectionData
    ASSERT_NO_THROW(connection_map = m_argument_list.map_at(0));
    EXPECT_EQ(1, connection_map->size());
    EXPECT_EQ(3307, connection_map->get_int("port"));

    // The rest is read on the options map, since it does not have defined the
    // valid options accepts everything
    shcore::Dictionary_t option_map;
    ASSERT_NO_THROW(option_map = m_argument_list.map_at(1));
    Argument_map map(*option_map);
    ASSERT_NO_THROW(map.ensure_keys(
        {"force", "bool", "stdDev", "passWord", "sandboxDir"}, {}, "test1"));
    EXPECT_TRUE(map.bool_at("force"));
    EXPECT_FALSE(map.bool_at("bool"));
    EXPECT_EQ(0.7, map.double_at("stdDev"));
    EXPECT_EQ("", map.string_at("passWord"));
    EXPECT_EQ("/tmp", map.string_at("sandboxDir"));

    bool force;
    double std;
    std::string pwd;
    EXPECT_NO_THROW(mysqlsh::Unpack_options(option_map)
                        .required("force", &force)
                        .optional("bool", &force)
                        .optional_ci("stddev", &std)
                        .optional_ci("password", &pwd)
                        .optional_exact("sandboxDir", &pwd)
                        .end());

    mysqlsh::Unpack_options up(option_map);
    // 0.7 will be turned into TRUE
    EXPECT_NO_THROW(up.required("stdDev", &force));
    EXPECT_TRUE(force);
    EXPECT_THROW(up.optional("sandboxDir", &force), Exception);
  }
}

TEST_F(Shell_cli_operation_test, local_dict) {
  {
    const char *arg1[] = {"util", "check-for-server-upgrade",
                          "{",    "--host=localhost",
                          "}",    "123"};
    Options::Cmdline_iterator it(6, arg1, 0);
    parse(&it);

    EXPECT_THROW_LIKE(prepare(), std::invalid_argument,
                      "The following argument is invalid: 123");

    EXPECT_EQ("util", m_object_name);
    EXPECT_EQ("checkForServerUpgrade", m_method_name);
    EXPECT_EQ(1, m_argument_list.size());
    ASSERT_TRUE(m_argument_list[0].type == Value_type::Map);
    EXPECT_EQ("localhost", m_argument_list.map_at(0)->get_string("host"));
  }
  {
    const char *arg2[] = {
        "util", "check-for-server-upgrade", "{", "--host=localhost", "1", "}",
        "123"};
    Options::Cmdline_iterator it(7, arg2, 0);

    parse(&it);
    EXPECT_THROW_LIKE(prepare(), std::invalid_argument,
                      "The following arguments are invalid: 1, 123");
  }
  {
    const char *arg3[] = {"util", "check-for-server-upgrade", "{",
                          "--host=localhost"};
    Options::Cmdline_iterator it(4, arg3, 0);
    parse(&it);
    // This no longer crashes as { and } are simply ignored
    prepare();
    EXPECT_EQ(1, m_argument_list.size());
    ASSERT_TRUE(m_argument_list[0].type == Value_type::Map);
    EXPECT_EQ("localhost", m_argument_list.map_at(0)->get_string("host"));
  }
  {
    const char *arg4[] = {"util",        "check-for-server-upgrade",
                          "--port=3306", "{--host=localhost",
                          "}",           "--lines=123"};
    Options::Cmdline_iterator it(6, arg4, 0);
    parse(&it);

    // Since this function has definition of what options are allowed and
    // "lines" is not one of them, the argument is rejected
    EXPECT_THROW_LIKE(prepare(), std::invalid_argument,
                      "The following option is invalid: --lines");
    EXPECT_EQ("util", m_object_name);
    EXPECT_EQ("checkForServerUpgrade", m_method_name);
  }
}

TEST_F(Shell_cli_operation_test, connection_options) {
  {
    const char *arg1[] = {"util",       "check-for-server-upgrade",
                          "{",          "--sslMode",
                          "required",   "--compression-algorithms",
                          "zstd",       "--user",
                          "root",       "--host",
                          "localhost",  "}",
                          "--password", "samplepwd"};
    Options::Cmdline_iterator it(14, arg1, 0);
    EXPECT_NO_THROW(parse(&it));
    EXPECT_NO_THROW(prepare());
    EXPECT_EQ("util", m_object_name);
    EXPECT_EQ("checkForServerUpgrade", m_method_name);
    EXPECT_EQ(1, m_argument_list.size());
    ASSERT_TRUE(m_argument_list[0].type == Value_type::Map);
    EXPECT_EQ("localhost", m_argument_list.map_at(0)->get_string("host"));
    EXPECT_EQ("required", m_argument_list.map_at(0)->get_string("ssl-mode"));
    EXPECT_EQ("zstd",
              m_argument_list.map_at(0)->get_string("compression-algorithms"));
    EXPECT_EQ("root", m_argument_list.map_at(0)->get_string("user"));
    EXPECT_EQ("samplepwd", m_argument_list.map_at(0)->get_string("password"));
  }
  {
    const char *arg2[] = {"util",
                          "check-for-server-upgrade",
                          "--port=3306",
                          "{",
                          "--sslMode=required",
                          "--config-path=/whatever/path",
                          "}",
                          "--sslMode=required",
                          "--compression-algorithms=zstd",
                          "--user=root"};
    Options::Cmdline_iterator it(10, arg2, 0);
    EXPECT_NO_THROW(parse(&it));
    EXPECT_NO_THROW(prepare());
    EXPECT_EQ("util", m_object_name);
    EXPECT_EQ("checkForServerUpgrade", m_method_name);
    EXPECT_EQ(2, m_argument_list.size());
    ASSERT_TRUE(m_argument_list[0].type == Value_type::Map);
    auto co = m_argument_list.map_at(0);
    // a connection options dictionary
    EXPECT_EQ(3306, co->get_int("port"));
    EXPECT_EQ("required", co->get_string("ssl-mode"));
    EXPECT_EQ("zstd", co->get_string("compression-algorithms"));
    EXPECT_EQ("root", co->get_string("user"));

    // not a connection options dictionary
    ASSERT_TRUE(m_argument_list[1].type == Value_type::Map);
    EXPECT_EQ("/whatever/path",
              m_argument_list.map_at(1)->get_string("configPath"));
  }
}

#define MY_EXPECT_EQ_OR_DUMP(a, b)          \
  {                                         \
    const auto &av = a;                     \
    const auto &bv = b;                     \
    if (av != bv) {                         \
      puts(output_handler.std_out.c_str()); \
      puts(output_handler.std_err.c_str()); \
    }                                       \
    EXPECT_EQ(av, bv);                      \
  }

#define MY_ASSERT_EQ_OR_DUMP(a, b)          \
  {                                         \
    const auto &av = a;                     \
    const auto &bv = b;                     \
    if (av != bv) {                         \
      puts(output_handler.std_out.c_str()); \
      puts(output_handler.std_err.c_str()); \
    }                                       \
    ASSERT_EQ(av, bv);                      \
  }

TEST_F(Shell_cli_operation_test, integration_test) {
  SKIP_UNLESS_DIRECT_MODE();
  testutil->mk_dir("cli-sandboxes");

  std::vector<std::string> env{"MYSQLSH_TERM_COLOR_MODE=nocolor"};
  MY_EXPECT_EQ_OR_DUMP(
      0, testutil->call_mysqlsh_c({"--", "shell", "status"}, "", env));
  MY_EXPECT_STDOUT_CONTAINS("MySQL Shell version");
  output_handler.wipe_all();

  MY_EXPECT_EQ_OR_DUMP(
      0, testutil->call_mysqlsh_c(
             {"--", "shell", "options", "set-persist", "defaultMode", "py"}, "",
             env));
  EXPECT_TRUE(output_handler.std_err.empty() && output_handler.std_out.empty());
  MY_EXPECT_EQ_OR_DUMP(
      0,
      testutil->call_mysqlsh_c(
          {"--", "shell", "options", "unset-persist", "defaultMode"}, "", env));
  EXPECT_TRUE(output_handler.std_err.empty() && output_handler.std_out.empty());

#ifndef _MSC_VER
  MY_ASSERT_EQ_OR_DUMP(
      0,
      testutil->call_mysqlsh_c(
          {"--", "dba", "deploySandboxInstance",
           std::to_string(_mysql_sandbox_ports[0]).c_str(),
           ("--portx=" + std::to_string(_mysql_sandbox_ports[0]) + "0").c_str(),
           "--password=abc", "--allow-root-from=%",
           "--sandbox-dir=cli-sandboxes"},
          "", env));
  MY_EXPECT_STDOUT_CONTAINS("successfully deployed and started");
  output_handler.wipe_all();

  std::string uri =
      "root:abc@localhost:" + std::to_string(_mysql_sandbox_ports[0]);

  EXPECT_NE(10, testutil->call_mysqlsh_c(
                    {"--", "util", "check-for-server-upgrade", uri.c_str()}, "",
                    env));
  MY_EXPECT_STDOUT_CONTAINS("will now be checked for compatibility issues");
  output_handler.wipe_all();

  MY_EXPECT_EQ_OR_DUMP(
      0,
      testutil->call_mysqlsh_c(
          {uri.c_str(), "--", "dba", "create-cluster", "cluster_X"}, "", env));
  MY_EXPECT_STDOUT_CONTAINS("Cluster successfully created");
  output_handler.wipe_all();

  MY_EXPECT_EQ_OR_DUMP(
      0, testutil->call_mysqlsh_c({uri.c_str(), "--", "cluster", "status"}, "",
                                  env));
  MY_EXPECT_STDOUT_CONTAINS("\"clusterName\": \"cluster_X\"");
  output_handler.wipe_all();

  MY_EXPECT_EQ_OR_DUMP(0, testutil->call_mysqlsh_c(
                              {"--", "dba", "stopSandboxInstance",
                               std::to_string(_mysql_sandbox_ports[0]).c_str(),
                               "--password=abc", "--sandbox-dir=cli-sandboxes"},
                              "", env));
  MY_EXPECT_STDOUT_CONTAINS("successfully stopped");
  output_handler.wipe_all();

  MY_EXPECT_EQ_OR_DUMP(0, testutil->call_mysqlsh_c(
                              {"--", "dba", "deleteSandboxInstance",
                               std::to_string(_mysql_sandbox_ports[0]).c_str(),
                               "--sandbox-dir=cli-sandboxes"},
                              "", env));
  MY_EXPECT_STDOUT_CONTAINS("successfully deleted");
  output_handler.wipe_all();
#endif
  testutil->rm_dir("cli-sandboxes", true);
}

TEST_F(Shell_cli_operation_test, test_invalid_cli_operations) {
  std::vector<std::pair<const char *, const char *>> invalid_ops = {
      {"cluster", "fake-operation"},
      {"cluster", "disconnect"},
      {"dba", "fake-operation"},
      {"dba", "get-cluster"},
      {"dba", "get-replica-set"},
      {"shell", "fake-command"},
      {"shell", "add-extension-object-member"},
      {"shell", "connect"},
      {"shell", "connect-to-primary"},
      {"shell", "create-extension-object"},
      {"shell", "disable-pager"},
      {"shell", "dump-rows"},
      {"shell", "enable-pager"},
      {"shell", "get-session"},
      {"shell", "log"},
      {"shell", "open-session"},
      {"shell", "parse-uri"},
      {"shell", "prompt"},
      {"shell", "reconnect"},
      {"shell", "register-report"},
      {"shell", "register-global"},
      {"shell", "set-current-schema"},
      {"shell", "set-session"},
      {"shell", "unparse-uri"},
      {"rs", "fake-operation"},
      {"rs", "disconnect"}};

  for (const auto &pair : invalid_ops) {
    // myInt is specified with invalid integer value
    const char *argv[] = {std::get<0>(pair), std::get<1>(pair)};

    Options::Cmdline_iterator it(2, argv, 0);

    parse(&it);
    EXPECT_THROW_LIKE(prepare(), std::invalid_argument,
                      shcore::str_format("Invalid operation for %s object: %s",
                                         std::get<0>(pair), std::get<1>(pair))
                          .c_str());
  }
}

#define TEST_API_CLI_OPTIONS(OPTIONS, ...) \
  test<OPTIONS>(__FILE__, __LINE__, __VA_ARGS__)

/**
 * This test class is used to ensure that whenever an command line option is
 * used on a specific API, the correct arguments are produced in the CLI
 * Integration layer.
 *
 * It tests option by option using the --camelCaseName version of the option.
 */
class Test_cli_integration_api_options : public Shell_cli_operation_test {
 public:
  std::pair<std::string, shcore::Value> get_value(shcore::Value_type type) {
    switch (type) {
      case shcore::String:
        return {"myString", shcore::Value("myString")};
      case shcore::Integer:
        return {"5", shcore::Value(5)};
      case shcore::UInteger:
        return {"6", shcore::Value(static_cast<uint64_t>(6))};
      case shcore::Float:
        return {"5.7", shcore::Value(5.7)};
      case shcore::Bool:
        return {"true", shcore::Value::True()};
      case shcore::Array:
        return {"one,two,three",
                shcore::Value(shcore::make_array("one", "two", "three"))};
      case shcore::Map:
        return {"key=1", shcore::Value(shcore::make_dict("key", 1))};
      default:
        return {"", shcore::Value()};
    }
  };

  template <typename T, typename... A>
  void test(const char *file, int line, A... mandatory_params) {
    for (const auto &option : T::options().options()) {
      if (option->cmd_line_enabled) {
        auto given_expected = get_value(option->type());

        std::string option_def =
            "--" + option->name + "=" + std::get<0>(given_expected);

        char *arg[] = {const_cast<char *>(mandatory_params)...,
                       const_cast<char *>(option_def.c_str())};

        std::vector<std::string> str_values = {mandatory_params...};
        std::vector<shcore::Value> positional_arguments;

        size_t last_positional = 0;
        for (const auto &str : str_values) {
          // Only anonymous parameters are positional
          if (!shcore::str_beginswith(str, "--")) {
            try {
              positional_arguments.push_back(shcore::Value::parse(str));
            } catch (...) {
              positional_arguments.push_back(shcore::Value(str));
            }
            last_positional++;
          } else {
            auto tokens = shcore::str_split(str, "=");
            auto key = tokens[0].substr(2);
            if (tokens.size() == 1) {
              positional_arguments.push_back(shcore::Value::True());
            } else {
              try {
                positional_arguments.push_back(shcore::Value::parse(tokens[1]));
              } catch (...) {
                positional_arguments.push_back(shcore::Value(tokens[1]));
              }
            }
          }
        }

        Options::Cmdline_iterator it(str_values.size() + 1, arg, 0);

        parse(&it);
        prepare();

        positional_arguments.erase(positional_arguments.begin());
        positional_arguments.erase(positional_arguments.begin());
        last_positional -= 2;

        for (size_t index = 0; index < positional_arguments.size(); index++) {
          auto expected = positional_arguments.at(index);
          auto actual = m_argument_list.at(index);

          // Small exception if the actual argument is an array, in that case
          // the expected should contain all the values until named ones are
          // found
          if (actual.type == shcore::Array) {
            auto array = shcore::make_array();
            while (index < last_positional) {
              array->push_back(positional_arguments.at(index++));
            }
            expected = shcore::Value(array);
          }

          if (expected != actual) {
            std::string error =
                "Mistmatch on positional arguments:\n --Expected: " +
                expected.json() + "\n --Actual: " + actual.json();

            SCOPED_TRACE(error.c_str());
            ADD_FAILURE_AT(file, line);
          }
        }

        shcore::Dictionary_t options = shcore::make_dict();
        options->set(option->name, std::get<1>(given_expected));

        auto expected = shcore::Value(options);
        auto actual = m_argument_list.at(m_argument_list.size() - 1);

        if (expected != actual) {
          std::string error =
              "Mistmatch on named arguments:\n --Expected: " + expected.json() +
              "\n --Actual: " + actual.json();

          SCOPED_TRACE(error.c_str());

          ADD_FAILURE_AT(file, line);
        }
      }
    }
  }
};

TEST_F(Test_cli_integration_api_options, all) {
  /** The UTILS Object **/
  TEST_API_CLI_OPTIONS(mysqlsh::Upgrade_check_options, "util",
                       "check-for-server-upgrade", "dummy_uri@localhost");

  TEST_API_CLI_OPTIONS(mysqlsh::import_table::Import_table_option_pack, "util",
                       "import-table", "some/file/path");

  TEST_API_CLI_OPTIONS(mysqlsh::dump::Dump_schemas_options, "util",
                       "dump-schemas", "mySchema",
                       "--outputUrl=some/folder/path");

  TEST_API_CLI_OPTIONS(mysqlsh::dump::Dump_tables_options, "util",
                       "dump-tables", "mySchema", "myTable",
                       "--outputUrl=some/folder/path");

  TEST_API_CLI_OPTIONS(mysqlsh::dump::Dump_instance_options, "util",
                       "dump-instance", "myOutputURL");

  TEST_API_CLI_OPTIONS(mysqlsh::dump::Export_table_options, "util",
                       "export-table", "myTable", "myOutputURL");

  TEST_API_CLI_OPTIONS(mysqlsh::Load_dump_options, "util", "load-dump",
                       "myUrl");

  /** The DBA Object **/
  TEST_API_CLI_OPTIONS(mysqlsh::dba::Deploy_sandbox_options, "dba",
                       "deploy-sandbox-instance", "3310");
}
}  // namespace cli
}  // namespace shcore
