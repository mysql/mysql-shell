/* Copyright (c) 2015, 2017, Oracle and/or its affiliates. All rights reserved.

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

#include <fstream>
#include "unittest/test_utils/shell_base_test.h"
#include "mysqlshdk/include/scripting/types.h"
#include "utils/utils_general.h"
#include "utils/utils_json.h"
#include "utils/utils_file.h"
#include "utils/utils_string.h"

namespace tests {

Shell_base_test::Shell_base_test() {
}

void Shell_base_test::TearDown() {
  Shell_test_env::TearDown();
}

void Shell_base_test::create_file(const std::string& name,
                                  const std::string& content) {
  if (!shcore::create_file(name, content)) {
    SCOPED_TRACE("Error Creating File: " + name);
    ADD_FAILURE();
  }
}

/**
 * Validates a string expectation.
 * @param file The name of the test file using this verification.
 * @param line The line where the verification is called.
 * @param expected_str A string expectation.
 * @param actual The actual string to be used on the verification.
 * @param expected Determines whether the string is expected or not.
 *
 * This function will do the string resolution on the expected string and then
 * will verify against the actual string.
 *
 * If expected is true and the expected string is not found on the actual string
 * a failure will be added to the test.
 *
 * If expected is false and the expected string is found on the actual string
 * a failure will be added to the test.
 *
 * In any other case no failures will be added.
 */
void Shell_base_test::check_string_expectation(const char* file, int line,
                                               const std::string& expected_str,
                                               const std::string& actual,
                                               bool expected) {
  std::string resolved_str = resolve_string(expected_str);

  bool found = actual.find(resolved_str) != std::string::npos;

  if (found != expected) {
    std::string error = expected ? "Missing Output: " : "Unexpected Output: ";
    error = makeyellow(error) + resolved_str;
    SCOPED_TRACE(makeyellow("Actual: ") + actual);
    SCOPED_TRACE(error);
    ADD_FAILURE_AT(file, line);
  }
}

/**
 * Validates multiple string expectations.
 * @param file The name of the test file using this verification.
 * @param line The line where the verification is called.
 * @param expected_strs A vector with string expectations.
 * @param actual The actual string to be used on the verification.
 * @param expected Determines whether the expected strings are expected or not.
 *
 * This function will do the string resolution on each expected string and then
 * will verify against the actual string.
 *
 * If expected is true and any of expected strings is not found on the actual
 * string a failure will be added to the test.
 *
 * If expected is false and any of the expected string is found on the actual
 * string a failure will be added to the test.
 *
 * In any other case no failures will be added.
 */
void Shell_base_test::check_string_list_expectation(
    const char *file, int line,
    const std::vector<std::string>& expected_strs, const std::string& actual,
    bool expected) {
  bool found = false;
  for (const auto &expected_str : expected_strs) {
    std::string resolved_str = resolve_string(expected_str);
    if (actual.find(resolved_str) != std::string::npos) {
      found = true;
      break;
    }
  }

  if (!found) {
    std::string error = expected ? "Missing Output: " : "Unexpected Output: ";
    error = makeyellow(error) + shcore::str_join(expected_strs, "\n\t");
    SCOPED_TRACE(makeyellow("Actual: ") + actual);
    SCOPED_TRACE(error);
    ADD_FAILURE_AT(file, line);
  }
}

/**
 * Multiple value validation
 * To be used on a single line
 * Line may have an entry that may have one of several values, i.e. on an
 * expected line like:
 *
 * \code
 * My text line is {{empty|full}}
 * \endcode
 *
 * Ths function would return true whenever the actual line is any of
 *
 * \code
 * My text line is empty
 * My text line is full
 * \endcode
 */
bool Shell_base_test::multi_value_compare(const std::string& expected,
                                          const std::string &actual) {
  bool ret_val = false;

  size_t start;
  size_t end;

  // ignoring spaces
  start = expected.find("{{");

  if (start != std::string::npos) {
    end = expected.find("}}");

    std::string pre = expected.substr(0, start);
    std::string post = expected.substr(end + 2);
    std::string opts = expected.substr(start + 2, end - (start + 2));
    auto options = shcore::split_string(opts, "|");

    for (auto item : options) {
      std::string exp = pre + item + post;
      if ((ret_val = (exp == actual)))
        break;
    }
  } else {
    ret_val = (expected == actual);
  }

  return ret_val;
}

/**
 * Validates a string expectation that spans over multiple lines.
 * @param context Context information that identifies where the verification is
 * done.
 * @param stream The stream to which the failure will be reported.
 * @param expected A multiline string expectation.
 * @param actual The actual string to be used on the verification.
 *
 * This function will split the expected and actual strings in lines and then
 * for each line on the expected stringit will do the string resolution and then
 * will verify against the actual string.
 *
 * The process starts by first identifying the first line on the expected string
 * on the actual string, once found it will verify every single line on the
 * expected string comes on the actual string in the exact same order and with
 * the exact same content.
 *
 * If an inconsistency is found, a failure will be added including the failed
 * expected string and the line that failed the verification will include the
 * next suffix:
 *
 * \code
 * <------ INCONSISTENCY
 * \endcode
 *
 * If the first expected line is not found, a failure will be added including
 * the next suffix on the first expected line:
 *
 * \code
 * <------ MISSING
 * \endcode
 *
 * In any other case no failures will be added.
 */
bool Shell_base_test::check_multiline_expect(const std::string& context,
                                             const std::string &stream,
                                             const std::string& expected,
                                             const std::string &actual) {
  bool ret_val = true;
  auto expected_lines = shcore::split_string(expected, "\n");
  auto actual_lines = shcore::split_string(actual, "\n");
  std::string r_trimmed_actual, r_trimmed_expected;
  // Does expected line resolution using the pre-defined tokens
  for (decltype(expected_lines)::size_type index = 0;
       index < expected_lines.size(); index++)
    expected_lines[index] = resolve_string(expected_lines[index]);

  // Identifies the index of the actual line containing the first expected line
  size_t actual_index = 0;
  r_trimmed_expected = shcore::str_rstrip(expected_lines.at(0));
  while (actual_index < actual_lines.size()) {
    // Ignore whitespace at the end of the actual and expected lines
    r_trimmed_actual = shcore::str_rstrip(actual_lines[actual_index]);
    if (multi_value_compare(r_trimmed_expected, r_trimmed_actual))
      break;
    else
      actual_index++;
  }

  if (actual_index < actual_lines.size()) {
    size_t expected_index;
    for (expected_index = 0; expected_index < expected_lines.size();
         expected_index++) {
      // if there are less actual lines than the ones expected
      if ((actual_index + expected_index) >= actual_lines.size()) {
        SCOPED_TRACE(makeyellow(stream + " actual: ") + actual);
        expected_lines[expected_index] += "<------ MISSING";
        SCOPED_TRACE(makeyellow(stream + " expected lines missing: ") +
            shcore::str_join(expected_lines, "\n"));
        ADD_FAILURE();
        ret_val = false;
        break;
      }

      auto act_str = shcore::str_rstrip(actual_lines[actual_index +
          expected_index]);

      auto exp_str = shcore::str_rstrip(expected_lines[expected_index]);
      if (!multi_value_compare(exp_str, act_str)) {
        SCOPED_TRACE("Executing: " + context);
        SCOPED_TRACE(makeyellow(stream + " actual: ") + actual);

        expected_lines[expected_index] += "<------ INCONSISTENCY";

        SCOPED_TRACE(makeyellow(stream + " inconsistent: ") +
            shcore::str_join(expected_lines, "\n"));
        ADD_FAILURE();
        ret_val = false;
        break;
      }
    }
  } else {
      SCOPED_TRACE(makeyellow("Executing: ") + context);
      SCOPED_TRACE(makeyellow(stream + " actual: ") + actual);

      expected_lines[0] += "<------ INCONSISTENCY";

      SCOPED_TRACE(makeyellow(stream + " inconsistent: ") +
      shcore::str_join(expected_lines, "\n"));
      ADD_FAILURE();
      ret_val = false;
  }

  return ret_val;
}

/**
 * Joins all the lines on the vector using the new line character
 */
std::string Shell_base_test::multiline(const std::vector<std::string> input) {
  return shcore::str_join(input, "\n");
}

}  // namespace tests
