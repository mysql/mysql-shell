/*
   Copyright (c) 2020, Oracle and/or its affiliates.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License, version 2.0,
   as published by the Free Software Foundation.

   This program is also distributed with certain software (including
   but not limited to OpenSSL) that is licensed under separate terms,
   as designated in a particular file or component or in included license
   documentation.  The authors of MySQL hereby grant you an additional
   permission to link the program and your derivative works with the
   separately licensed software that they have included with MySQL.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License, version 2.0, for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA
*/

#include "modules/util/dump/compatibility.h"

#include <regex>
#include <utility>

#include "mysqlshdk/libs/utils/utils_lexing.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlsh {
namespace compatibility {

namespace {
using Offset = std::pair<std::size_t, std::size_t>;
using Offsets = std::vector<Offset>;
using SQL_iterator = mysqlshdk::utils::SQL_iterator;

struct Comment_offset {
  Offset offset;
  bool inside_hint;
};

using Coffsets = std::vector<Comment_offset>;

std::string replace_at_offsets(const std::string &s, const Offsets &offsets,
                               const std::string &target) {
  if (offsets.empty()) return s;

  std::string out = s.substr(0, offsets[0].first) + target;
  for (std::size_t i = 1; i < offsets.size(); i++)
    out += s.substr(offsets[i - 1].second,
                    offsets[i].first - offsets[i - 1].second) +
           target;
  out += s.substr(offsets.back().second);
  return out;
}

std::string comment_out_offsets(const std::string &s, const Coffsets &offsets) {
  if (offsets.empty()) return s;

  std::size_t pos = 0;
  std::string out;
  for (const auto o : offsets) {
    out += s.substr(pos, o.offset.first - pos);
    if (o.inside_hint) {
      auto ol = shcore::str_rstrip(shcore::str_replace(
          s.substr(o.offset.first, o.offset.second - o.offset.first), "\n",
          " "));
      out += "-- " + ol + "\n";
    } else {
      out += "/* " +
             s.substr(o.offset.first, o.offset.second - o.offset.first) + "*/ ";
    }
    pos = o.offset.second;
  }
  out += s.substr(pos);
  return out;
}

std::size_t eat_string_val(const std::string &s, std::size_t i) {
  while (std::isspace(s[i]) || s[i] == '=') ++i;
  if (s[i] == '\'') return mysqlshdk::utils::span_quoted_string_sq(s, i);
  if (s[i] == '"') return mysqlshdk::utils::span_quoted_string_dq(s, i);
  return std::string::npos;
}

std::size_t eat_quoted_string(const std::string &s, std::size_t i) {
  if (s[i] == '\'') return mysqlshdk::utils::span_quoted_string_sq(s, i);
  if (s[i] == '"') return mysqlshdk::utils::span_quoted_string_dq(s, i);
  if (s[i] == '`') return mysqlshdk::utils::span_quoted_sql_identifier_bt(s, i);
  return std::string::npos;
}

void skip_columns_definition(SQL_iterator *it) {
  if (!shcore::str_caseeq(it->get_next_token(), "CREATE") ||
      !shcore::str_caseeq(it->get_next_token(), "TABLE"))
    throw std::runtime_error("Malformed create table statement");
  while (it->valid() && it->get_next_token() != "(") {
  }
  while (it->valid() && it->get_next_token() != ")") {
  }
  if (!it->valid())
    throw std::runtime_error(
        "Malformed create table statement - columns definition not found");
}

bool comment_out_option_with_string(
    const std::string &s, std::string *rewritten,
    std::function<bool(std::string *token, SQL_iterator *it)> skip_condition) {
  Coffsets offsets;
  SQL_iterator it(s, 12);
  std::size_t prev_pos = 0, end = 0;
  std::string token = it.get_next_token();
  bool prev_added = false;
  while (!token.empty()) {
    auto start = it.position() - token.size();
    bool hint = it.inside_hint();
    if (skip_condition(&token, &it) ||
        (end = eat_string_val(s, it.position())) == std::string::npos) {
      prev_pos = it.position() - token.size();
      prev_added = false;
      token = it.get_next_token();
      continue;
    }
    it.set_position(end);
    token = it.get_next_token();

    if (token == ",") {
      end = it.position();
      token = it.get_next_token();
    } else if (s[prev_pos] == ',') {
      start = prev_pos;
    }

    if (prev_added) {
      offsets.back().offset.second = end;
      offsets.back().inside_hint = offsets.back().inside_hint || hint;
    } else {
      offsets.emplace_back(Comment_offset{{start, end}, hint});
    }
    prev_added = true;
  }

  if (rewritten) *rewritten = comment_out_offsets(s, offsets);
  return !offsets.empty();
}

}  // namespace

std::vector<std::string> check_privileges(
    const std::string grant, std::string *rewritten_grant,
    const std::set<std::string> &privileges) {
  std::vector<std::string> res;
  Offsets offsets;

  SQL_iterator it(grant);
  if (!shcore::str_caseeq(it.get_next_token(), "GRANT"))
    throw std::runtime_error("Malformed grant statement: " + grant);

  int remaining_priv = 0;
  bool last_removed = false;
  while (it.valid()) {
    auto prev_pos = it.position() - 1;
    auto token = it.get_next_token();
    auto after_pos = it.position();
    auto next = it.get_next_token();
    while (!next.empty() && next != "," && next != "ON" && next != "TO") {
      token += " " + next;
      after_pos = it.position();
      next = it.get_next_token();
    }
    auto pit = privileges.find(shcore::str_upper(token));
    if (pit == privileges.end()) {
      ++remaining_priv;
      last_removed = false;
    } else {
      res.emplace_back(*pit);
      if (rewritten_grant) {
        if (grant[prev_pos] == ',')
          offsets.emplace_back(prev_pos, after_pos);
        else if (next == ",")
          offsets.emplace_back(
              6, std::isspace(*it) ? (++it).position() : it.position());
        else
          break;

        if (last_removed) {
          offsets[offsets.size() - 2].second = offsets.back().second;
          offsets.pop_back();
        }
        last_removed = true;
      }
    }
    if (next.empty() || next == "ON" || next == "TO") break;
  }

  if (rewritten_grant) {
    if (remaining_priv == 0)
      rewritten_grant->clear();
    else
      *rewritten_grant = replace_at_offsets(grant, offsets, "");
  }

  return res;
}

bool check_create_table_for_data_index_dir_option(
    const std::string &create_table, std::string *rewritten) {
  /*
        First, comment out DATA|INDEX DIRECTORY = '...', but note:
         - Optional preceding and trailing commas.
         - Optional assignment operator.
         - Path enclosed in single quotes.
         - Use of ungreedy patterns to get correct grouping.
     */
  //  std::regex directory_option(
  //      "\\s*,?\\s*(DATA|INDEX)\\s+DIRECTORY\\s*?=?\\s*?[\"'][^\"']+?[\"']\\s*,"
  //      "?"
  //      "\\s*",
  //      std::regex::icase | std::regex::nosubs);
  //  auto q = std::regex_replace(create_table, directory_option, " /*$&*/ ");
  //  bool res = q != create_table;
  //  if (rewritten) *rewritten = q;
  //  return res;

  return comment_out_option_with_string(
      create_table, rewritten, [](std::string *token, SQL_iterator *it) {
        if (!shcore::str_caseeq(*token, "DATA") &&
            !shcore::str_caseeq(*token, "INDEX"))
          return true;
        *token = it->get_next_token();
        return !shcore::str_caseeq(*token, "DIRECTORY");
      });
}

bool check_create_table_for_encryption_option(const std::string &create_table,
                                              std::string *rewritten) {
  // Comment out ENCRYPTION option.
  //  std::regex encryption_option(
  //      "\\s*,?\\s*ENCRYPTION\\s*?=?\\s*?[\"'][NY]?[\"']\\s*,?\\s*",
  //      std::regex::icase | std::regex::nosubs);
  //  auto q = std::regex_replace(create_table, encryption_option, " /*$&*/ ");
  //  bool res = q != create_table;
  //  if (rewritten) *rewritten = q;
  //  return res;

  return comment_out_option_with_string(
      create_table, rewritten, [](std::string *token, SQL_iterator *it) {
        if (shcore::str_caseeq(*token, "DEFAULT"))
          *token = it->get_next_token();
        return !shcore::str_caseeq(*token, "ENCRYPTION");
      });
}

std::string check_create_table_for_engine_option(
    const std::string &create_table, std::string *rewritten,
    const std::string &target) {
  std::string res;
  Offsets offsets;
  SQL_iterator it(create_table);
  skip_columns_definition(&it);
  std::string token;
  while (!(token = it.get_next_token()).empty()) {
    if (!shcore::str_caseeq(token, "ENGINE")) continue;
    auto name = it.get_next_token();
    if (name == "=") name = it.get_next_token();
    if (shcore::str_caseeq(name, target)) continue;
    offsets.emplace_back(it.position() - name.length(), it.position());
    res = name;
  }

  if (rewritten) *rewritten = replace_at_offsets(create_table, offsets, target);
  return res;
}

bool check_create_table_for_tablespace_option(
    const std::string &create_table, std::string *rewritten,
    const std::vector<std::string> &whitelist) {
  Offsets offsets;
  SQL_iterator it(create_table, 0, false);
  skip_columns_definition(&it);
  std::size_t prev_pos = 0;
  std::string token;
  while (!(token = it.get_next_token()).empty()) {
    if (!shcore::str_caseeq(token, "TABLESPACE")) {
      prev_pos = it.position() - token.size();
      continue;
    }
    auto start = it.position() - token.size();
    auto name = it.get_next_token();
    if (name == "=") name = it.get_next_token();

    // Leave whitelisted tablespaces
    size_t iw = 0;
    for (; iw < whitelist.size(); ++iw)
      if (shcore::str_ibeginswith(
              name[0] == '`' ? name.c_str() + 1 : name.c_str(), whitelist[iw]))
        break;
    if (iw < whitelist.size()) continue;

    // Find if option encompassed by comment hint '/*!50100 '
    if (it.inside_hint() && create_table.compare(start - 9, 3, "/*!") == 0) {
      start -= 9;
      auto end = mysqlshdk::utils::span_cstyle_comment(create_table, start);
      offsets.emplace_back(create_table[prev_pos] == ',' ? prev_pos : start,
                           end);
      it.set_position(end);
      continue;
    }

    auto end = it.position();
    if (shcore::str_caseeq("STORAGE", it.get_next_token())) {
      // either DISK or MEMORY
      it.get_next_token();
      end = it.position();
    }
    offsets.emplace_back(create_table[prev_pos] == ',' ? prev_pos : start, end);
  }

  if (rewritten) *rewritten = replace_at_offsets(create_table, offsets, "");

  return !offsets.empty();
}

std::vector<std::string> check_statement_for_charset_option(
    const std::string &statement, std::string *rewritten,
    const std::vector<std::string> &whitelist) {
  Offsets offsets;
  SQL_iterator it(statement, 0, true);
  it.get_next_token();

  // CHARSET keyword can be a column name but cannot be a column option
  std::size_t end_of_col_def = 0;
  if (shcore::str_caseeq(it.get_next_token(), "TABLE")) {
    SQL_iterator itc(statement, 0);
    skip_columns_definition(&itc);
    end_of_col_def = itc.position();
  }

  std::vector<std::string> res;
  std::string token;
  while (!(token = it.get_next_token()).empty()) {
    std::size_t prev_pos = 0;
    if (shcore::str_caseeq(token.c_str(), "DEFAULT")) {
      prev_pos = it.position() - token.size();
      token = it.get_next_token();
    }
    bool collate = false;
    if (shcore::str_caseeq(token, "COLLATE")) {
      collate = true;
    } else if (shcore::str_caseeq(token, "CHARSET")) {
      if (it.position() < end_of_col_def) continue;
      collate = false;
    } else if (shcore::str_caseeq(token, "CHARACTER")) {
      if (prev_pos == 0) prev_pos = it.position() - token.size();
      if (!shcore::str_caseeq(it.get_next_token(), "SET")) continue;
      collate = false;
    } else {
      continue;
    }

    if (prev_pos == 0) prev_pos = it.position() - token.size();
    bool hint = it.inside_hint();

    while ((token = it.get_next_token()) == "=") {
    }
    bool supported = false;
    if (collate) token = token.substr(0, token.find('_'));
    for (const auto &c : whitelist)
      if (shcore::str_caseeq(token, c)) {
        supported = true;
        break;
      }

    if (!supported) {
      if (std::find(res.begin(), res.end(), token) == res.end())
        res.emplace_back(token);
      if (hint && statement.compare(prev_pos - 9, 3, "/*!") == 0) {
        prev_pos -= 9;
        auto end = mysqlshdk::utils::span_cstyle_comment(statement, prev_pos);
        offsets.emplace_back(prev_pos, end);
        it.set_position(end);
      } else {
        offsets.emplace_back(prev_pos, it.position());
      }
    }
  }
  if (rewritten) *rewritten = replace_at_offsets(statement, offsets, "");
  return res;
}

std::string check_statement_for_definer_clause(const std::string &statement,
                                               std::string *rewritten) {
  std::string user;
  SQL_iterator it(statement, 0);
  std::string token = it.get_next_token();
  if (!shcore::str_caseeq(token, "CREATE")) return user;

  while (!(token = it.get_next_token()).empty()) {
    if (shcore::str_caseeq(token, "VIEW") ||
        shcore::str_caseeq(token, "EVENT") ||
        shcore::str_caseeq(token, "FUNCTION") ||
        shcore::str_caseeq(token, "PROCEDURE") ||
        shcore::str_caseeq(token, "TRIGGER"))
      break;
    if (!shcore::str_caseeq(token, "DEFINER")) continue;
    // definer is in the form of DEFINER = user@host
    std::size_t i = it.position();
    std::size_t start = i - token.size();
    while (std::isspace(statement[i]) || statement[i] == '=') ++i;

    // user part
    std::size_t us = i;
    i = eat_quoted_string(statement, i);
    if (i == std::string::npos) continue;

    // now we need @
    while (std::isspace(statement[i])) ++i;
    if (statement[i++] != '@') continue;
    while (std::isspace(statement[i])) ++i;

    // host part
    i = eat_quoted_string(statement, i);
    if (i == std::string::npos) continue;

    user = shcore::str_replace(statement.substr(us, i - us), " ", "");
    if (rewritten) {
      while (std::isblank(statement[i])) ++i;
      *rewritten = statement.substr(0, start) + statement.substr(i);
    }
    break;
  }
  return user;
}

bool check_statement_for_sqlsecurity_clause(const std::string &statement,
                                            std::string *rewritten) {
  SQL_iterator it(statement, 0);
  std::string token = it.get_next_token();
  if (!shcore::str_caseeq(token, "CREATE")) return false;
  bool func_or_proc = false;

  while (!(token = it.get_next_token()).empty()) {
  loop_start:
    if (shcore::str_caseeq(token, "PROCEDURE") ||
        shcore::str_caseeq(token, "FUNCTION")) {
      func_or_proc = true;
      continue;
    }
    if (func_or_proc && (shcore::str_caseeq(token, "BEGIN") ||
                         shcore::str_caseeq(token, "RETURN") ||
                         shcore::str_caseeq(token, "SELECT") ||
                         shcore::str_caseeq(token, "UPDATE") ||
                         shcore::str_caseeq(token, "DELETE") ||
                         shcore::str_caseeq(token, "INSERT") ||
                         shcore::str_caseeq(token, "REPLACE") ||
                         shcore::str_caseeq(token, "CREATE") ||
                         shcore::str_caseeq(token, "ALTER") ||
                         shcore::str_caseeq(token, "DROP") ||
                         shcore::str_caseeq(token, "RENAME") ||
                         shcore::str_caseeq(token, "TRUNCATE") ||
                         shcore::str_caseeq(token, "CALL") ||
                         shcore::str_caseeq(token, "PREPARE") ||
                         shcore::str_caseeq(token, "EXECUTE") ||
                         shcore::str_caseeq(token, "REPEAT") ||
                         shcore::str_caseeq(token, "DO") ||
                         shcore::str_caseeq(token, "WHILE") ||
                         shcore::str_caseeq(token, "LOOP") ||
                         shcore::str_caseeq(token, "IMPORT") ||
                         shcore::str_caseeq(token, "LOAD") ||
                         shcore::str_caseeq(token, "TABLE") ||
                         shcore::str_caseeq(token, "WITH") ||
                         shcore::str_iendswith(token, ":"))) {
      if (rewritten) {
        const auto pos = it.position() - token.size();
        *rewritten = statement.substr(0, pos) + "SQL SECURITY INVOKER\n" +
                     statement.substr(pos);
      }
      return true;
    }
    if (!shcore::str_caseeq(token, "SQL")) continue;
    if (!shcore::str_caseeq((token = it.get_next_token()), "SECURITY"))
      goto loop_start;
    token = it.get_next_token();
    if (shcore::str_caseeq(token, "INVOKER")) return false;
    if (!shcore::str_caseeq(token, "DEFINER")) continue;
    if (rewritten)
      *rewritten = statement.substr(0, it.position() - token.size()) +
                   "INVOKER" + statement.substr(it.position());
    return true;
  }
  return false;
}

std::vector<std::string> check_create_table_for_indexes(
    const std::string &statement, std::string *rewritten,
    bool return_alter_table) {
  std::vector<std::string> ret;
  Offsets offsets;
  SQL_iterator it(statement, 0, false);

  if (!shcore::str_caseeq(it.get_next_token(), "CREATE") ||
      !shcore::str_caseeq(it.get_next_token(), "TABLE"))
    throw std::runtime_error(
        "Index check can be only performed on create table statements");

  auto table_name = it.get_next_token();
  if (shcore::str_caseeq(table_name, "IF")) {
    while (it.valid())
      if (shcore::str_caseeq(it.get_next_token(), "EXISTS")) break;
    table_name = it.get_next_token();
  }

  while (it.valid() && it.get_next_token() != "(") {
  }

  int brace_count = 1;
  while (it.valid() && brace_count > 0) {
    bool index_declaration = false;
    auto potential_coma = it.position() - 1;
    auto token = it.get_next_token();
    auto start = it.position() - token.length();

    if (shcore::str_caseeq(token, "FULLTEXT") ||
        shcore::str_caseeq(token, "UNIQUE") ||
        shcore::str_caseeq(token, "KEY") ||
        shcore::str_caseeq(token, "INDEX") ||
        shcore::str_caseeq(token, "SPATIAL"))
      index_declaration = true;

    bool constraint = shcore::str_caseeq(token, "CONSTRAINT");
    while (it.valid() && brace_count > 0) {
      token = it.get_next_token();
      if (token == "(")
        ++brace_count;
      else if (token == ")")
        --brace_count;
      else if (brace_count == 1 && token == ",")
        break;
      else if (constraint && (shcore::str_caseeq(token, "KEY") ||
                              shcore::str_caseeq(token, "INDEX")))
        index_declaration = true;
    }

    if (!it.valid())
      throw std::runtime_error(
          "Incomplete create_definition in create table statement of table " +
          table_name);
    if (!index_declaration) continue;

    // previous token had to be either ',' or ')'
    auto end = it.position() - token.length();
    if (statement[potential_coma] == ',') {
      if (!offsets.empty() && offsets.back().second == potential_coma)
        offsets.back().second = end;
      else
        offsets.emplace_back(potential_coma, end);
    } else if (statement[end] == ',') {
      offsets.emplace_back(start, end + 1);
    } else {
      offsets.emplace_back(start, end);
    }
    const auto index_definition =
        shcore::str_rstrip(statement.substr(start, end - start));
    if (return_alter_table)
      ret.emplace_back("ALTER TABLE " + table_name + " ADD " +
                       index_definition + ";");
    else
      ret.emplace_back(index_definition);
  }

  if (rewritten) *rewritten = replace_at_offsets(statement, offsets, "");
  return ret;
}

}  // namespace compatibility
}  // namespace mysqlsh
