/* Copyright (c) 2015, 2024, Oracle and/or its affiliates.

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

#include "unittest/test_utils/shell_base_test.h"

#include <fstream>

#include "mysqlshdk/include/scripting/types.h"
#include "utils/utils_file.h"
#include "utils/utils_general.h"
#include "utils/utils_string.h"

namespace tests {

Shell_base_test::Shell_base_test() {}

void Shell_base_test::TearDown() { Shell_test_env::TearDown(); }

void Shell_base_test::create_file(const std::string &name,
                                  const std::string &content) {
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
void Shell_base_test::check_string_expectation(const char *file, int line,
                                               const std::string &expected_str,
                                               const std::string &actual,
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
    const char *file, int line, const std::vector<std::string> &expected_strs,
    const std::string &actual, bool expected) {
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
 * Matches a line containing wildcard tokens, which will match any sequence of
 * characters.
 *
 * @param expected: The string with the expected data.
 * @param actual: The string where the expected string should be found.
 * @param start: the position in actual where the search starts.
 * @param out_start: Will hold the position where the match started.
 * @param out_end: Will hold the position where the match ended.
 *
 * \code
 * Line executed in [[*]] seconds.
 * \endcode
 *
 * Supports presense of multiple tokens as long as they have fixed text in
 * between.
 *
 * If the actual contains the expected string:
 * - If start is provided, the position where the match started will be set.
 * - If end is provided, the position of the last character of the match will be
 * set.
 */
bool Shell_base_test::check_wildcard_match(std::string_view expected,
                                           std::string_view actual,
                                           bool full_match, size_t start,
                                           size_t *out_start, size_t *out_end) {
  size_t all_start = std::string_view::npos;
  size_t all_end = std::string_view::npos;

  if (expected.empty()) {
    if (full_match && expected != actual) {
      return false;
    }
    all_start = start;
    all_end = start;

  } else {
    static constexpr auto k_wildcard = "[[*]]";

    std::vector<std::string_view> strings;

    // Split the string by the wildcard construct until no split is done (second
    // is empty)
    auto tokens = std::make_pair(std::string_view(), expected);
    bool ends_in_wildcard = false;
    do {
      tokens = shcore::str_partition<std::string_view>(
          tokens.second, k_wildcard, &ends_in_wildcard);
      if (tokens.first.find(k_wildcard) == 0) {
        throw std::runtime_error(
            "Consecutive wildcard match tokens are not allowed: [[*]][[*]]");
      } else {
        strings.push_back(tokens.first);
      }
    } while (!tokens.second.empty());

    all_start = actual.find(strings[0], start);

    if (all_start == std::string_view::npos) return false;

    all_end = all_start + strings[0].length();

    strings.erase(strings.begin());

    for (const auto str : strings) {
      size_t tmp = actual.find(str, all_end);
      if (tmp == std::string_view::npos) return false;
      all_end = tmp + str.length();
    }

    if (ends_in_wildcard) all_end = actual.length();
  }

  if (full_match && (all_start != start || all_end != actual.length()))
    return false;

  if (out_start) *out_start = all_start;
  if (out_end) *out_end = all_end;

  return true;
}

/**
 * Multiple value validation
 * To be used on a single line
 *
 * @param expected: The string with the expected data.
 * @param actual: The string where the expected string should be found.
 * @param start: the position in actual where the search starts.
 * @param out_start: Will hold the position where the match started.
 * @param out_end: Will hold the position where the match ended.
 *
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
 *
 * If the actual contains the expected string:
 * - If start is provided, the position where the match started will be set.
 * - If end is provided, the position of the last character of the match will
 * be set.
 */
bool Shell_base_test::multi_value_compare(std::string_view expected,
                                          std::string_view actual,
                                          bool full_match, size_t start,
                                          size_t *out_start, size_t *out_end) {
  // ignoring spaces
  const auto local_start = expected.find("{{");

  if (local_start == std::string_view::npos)
    return check_wildcard_match(expected, actual, full_match, start, out_start,
                                out_end);

  const auto local_end = expected.find("}}");

  auto pre = expected.substr(0, local_start);
  auto post = expected.substr(local_end + 2);
  auto opts = expected.substr(local_start + 2, local_end - (local_start + 2));

  bool ret_val = false;
  shcore::split_string(opts, "|", false, [&](auto token) {
    auto exp = shcore::str_format("%.*s%.*s%.*s",  //
                                  static_cast<int>(pre.size()), pre.data(),
                                  static_cast<int>(token.size()), token.data(),
                                  static_cast<int>(post.size()), post.data());

    if (!check_wildcard_match(exp, actual, full_match, start, out_start,
                              out_end))
      return true;  // next token

    ret_val = true;
    return false;  // stop splitting
  });

  return ret_val;
}

size_t Shell_base_test::find_line_matching(
    std::string_view expected, const std::vector<std::string> &actual_lines,
    size_t start_at) {
  size_t actual_index = start_at;
  for (; actual_index < actual_lines.size(); actual_index++) {
    // Ignore whitespace at the end of the actual and expected lines
    auto r_trimmed_actual = shcore::str_rstrip_view(actual_lines[actual_index]);
    if (multi_value_compare(expected, r_trimmed_actual)) break;
  }

  return actual_index;
}

/**
 * Validates a string expectation that spans over multiple lines.
 * @param context Context information that identifies where the verification
 * is done.
 * @param stream The stream to which the failure will be reported.
 * @param expected A multiline string expectation.
 * @param actual The actual string to be used on the verification.
 *
 * This function will split the expected and actual strings in lines and then
 * for each line on the expected stringit will do the string resolution and
 * then will verify against the actual string.
 *
 * The process starts by first identifying the first line on the expected
 * string on the actual string, once found it will verify every single line on
 * the expected string comes on the actual string in the exact same order and
 * with the exact same content.
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
bool Shell_base_test::check_multiline_expect(const std::string &context,
                                             const std::string &stream,
                                             const std::string &expected,
                                             const std::string &actual,
                                             int /* srcline */, int valline) {
  bool ret_val = true;
  auto expected_lines = shcore::split_string(expected, "\n");
  auto actual_lines = shcore::split_string(actual, "\n");

  // Removes empty lines at the beggining of the expectation
  // Multiline comparison should start from first non empty line
  // ${*} at the beggining is ignored as well
  while (!expected_lines.empty() && (expected_lines.begin()->empty() ||
                                     (*expected_lines.begin() == "${*}")))
    expected_lines.erase(expected_lines.begin());

  if (expected_lines.empty()) return true;

  std::string r_trimmed_expected;
  // Does expected line resolution using the pre-defined tokens
  for (decltype(expected_lines)::size_type index = 0;
       index < expected_lines.size(); index++)
    expected_lines[index] = resolve_string(expected_lines[index]);

  // Identifies the index of the actual line containing the first expected
  // line
  r_trimmed_expected = shcore::str_rstrip(expected_lines.at(0));
  size_t actual_index = find_line_matching(r_trimmed_expected, actual_lines);

  if (actual_index < actual_lines.size()) {
    size_t expected_index;
    for (expected_index = 0; expected_index < expected_lines.size();
         expected_index++) {
      if (expected_lines[expected_index] == "${*}") {
        if (expected_index == expected_lines.size() - 1) {
          continue;  // Ignores ${*} if at the end of the expectation
        } else {
          expected_index++;
          r_trimmed_expected =
              shcore::str_rstrip(expected_lines.at(expected_index));
          actual_index = find_line_matching(r_trimmed_expected, actual_lines,
                                            actual_index + expected_index - 1);
          if (actual_index < actual_lines.size()) {
            actual_index -= expected_index;
          } else {
            SCOPED_TRACE(makeyellow(stream + " actual: ") + actual);

            expected_lines[expected_index] +=
                makeyellow("<------ INCONSISTENCY");

            SCOPED_TRACE(makeyellow(stream + " expected: ") +
                         shcore::str_join(expected_lines, "\n"));
            SCOPED_TRACE(makeblue("Executing: " + context) +
                         ", validation at line " + std::to_string(valline));
            ADD_FAILURE();
            ret_val = false;
            break;
          }
        }
      }

      // if there are less actual lines than the ones expected
      if ((actual_index + expected_index) >= actual_lines.size()) {
        SCOPED_TRACE(makeyellow(stream + " actual: ") + actual);
        expected_lines[expected_index] += makeyellow("<------ MISSING");
        SCOPED_TRACE(makeyellow(stream + " expected lines missing: ") +
                     shcore::str_join(expected_lines, "\n"));
        ADD_FAILURE();
        ret_val = false;
        break;
      }

      auto act_str =
          shcore::str_rstrip(actual_lines[actual_index + expected_index]);

      auto exp_str = shcore::str_rstrip(expected_lines[expected_index]);
      if (!multi_value_compare(exp_str, act_str)) {
        SCOPED_TRACE(makeyellow(stream + " actual: ") + actual);

        expected_lines[expected_index] += makeyellow("<------ INCONSISTENCY");

        SCOPED_TRACE(makeyellow(stream + " expected: ") +
                     shcore::str_join(expected_lines, "\n"));
        SCOPED_TRACE(makeblue("Executing: " + context) +
                     ", validation at line " + std::to_string(valline));
        ADD_FAILURE();
        ret_val = false;
        break;
      }
    }
  } else {
    SCOPED_TRACE(makeyellow(stream + " actual: ") + actual);

    expected_lines[0] += makeyellow("<------ INCONSISTENCY");

    SCOPED_TRACE(makeyellow(stream + " expected: ") +
                 shcore::str_join(expected_lines, "\n"));
    SCOPED_TRACE(makeblue("Executing: " + context) + ", validation at line " +
                 std::to_string(valline));
    ADD_FAILURE();
    ret_val = false;
  }

  return ret_val;
}

/**
 * Joins all the lines on the vector using the new line character
 */
std::string Shell_base_test::multiline(const std::vector<std::string> &input) {
  return shcore::str_join(input, "\n");
}

}  // namespace tests
