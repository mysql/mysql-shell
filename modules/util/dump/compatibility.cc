/*
 * Copyright (c) 2020, Oracle and/or its affiliates.
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

#include "modules/util/dump/compatibility.h"

#include <regex>
#include <utility>

#include "mysqlshdk/libs/utils/utils_lexing.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlsh {
namespace compatibility {

const std::set<std::string> k_mysqlaas_allowed_privileges = {
    "ALTER",
    "ALTER ROUTINE",
    "CREATE",
    "CREATE ROLE",
    "CREATE ROUTINE",
    "CREATE TEMPORARY TABLES",
    "CREATE USER",
    "CREATE VIEW",
    "DELETE",
    "DROP",
    "DROP ROLE",
    "EVENT",
    "EXECUTE",
    "INDEX",
    "INSERT",
    "LOCK TABLES",
    "PROCESS",
    "REFERENCES",
    "REPLICATION CLIENT",
    "REPLICATION SLAVE",
    "SELECT",
    "SHOW DATABASES",
    "SHOW VIEW",
    "TRIGGER",
    "UPDATE"};

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
  for (const auto &o : offsets) {
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
  auto token = it.get_next_token();
  if (!shcore::str_caseeq_mv(token, "GRANT", "REVOKE"))
    throw std::runtime_error("Malformed grant statement: " + grant);
  const auto first_token_size = token.size();

  int remaining_priv = 0;
  bool last_removed = false;
  while (it.valid()) {
    auto prev_pos = it.position() - 1;
    token = it.get_next_token();
    auto after_pos = it.position();
    auto next = it.get_next_token();
    // @ is reported as a token for roles, it is skipped as this function only
    // searches for individual grants
    while (token == "@" && next == ",") {
      token = it.get_next_token();
      after_pos = it.position();
      next = it.get_next_token();
    }
    while (!next.empty() && next != "," && next != "ON" && next != "TO") {
      token += " " + next;
      after_pos = it.position();
      next = it.get_next_token();
    }
    if (token != "@") {
      auto pit = privileges.find(shcore::str_upper(token));
      if (pit != privileges.end()) {
        ++remaining_priv;
        last_removed = false;
      } else {
        res.emplace_back(token);
        if (rewritten_grant) {
          if (grant[prev_pos] == ',')
            offsets.emplace_back(prev_pos, after_pos);
          else if (next == ",")
            offsets.emplace_back(first_token_size + 1, std::isspace(*it)
                                                           ? (++it).position()
                                                           : it.position());
          else
            break;

          if (last_removed) {
            offsets[offsets.size() - 2].second = offsets.back().second;
            offsets.pop_back();
          }
          last_removed = true;
        }
      }
    }
    if (next.empty() || shcore::str_caseeq_mv(next, "ON", "TO", "FROM")) break;
  }

  if (rewritten_grant) {
    if (remaining_priv == 0)
      rewritten_grant->clear();
    else
      *rewritten_grant = replace_at_offsets(grant, offsets, "");
  }

  return res;
}

std::string filter_grant_or_revoke(
    const std::string &stmt,
    const std::function<bool(bool is_revoke, const std::string &priv_type,
                             const std::string &column_list,
                             const std::string &object_type,
                             const std::string &priv_level)> &filter) {
  std::string filtered_stmt;
  // tokenize the whole thing to simplify
  std::vector<std::pair<std::string, size_t>> tokens;
  {
    SQL_iterator it(stmt, 0, false);
    while (it.valid()) {
      tokens.push_back(it.get_next_token_and_offset());
    }
  }

  auto t = tokens.begin();
  const auto t_end = tokens.end();

  const auto check_if = [&t, t_end](const std::string &token) {
    if (t != t_end && shcore::str_caseeq(t->first, token)) return true;
    return false;
  };

  const auto advance_if = [&t, &check_if](const std::string &token,
                                          size_t *out_offset = nullptr) {
    if (check_if(token)) {
      if (out_offset) *out_offset = t->second;
      ++t;
      return true;
    }
    return false;
  };

  const auto consume = [&t, t_end, stmt]() -> const std::string & {
    if (t != t_end) {
      auto &tmp = *t;
      ++t;
      return tmp.first;
    }
    throw std::runtime_error("Malformed grant/revoke statement: " + stmt);
  };

  if (check_if("REVOKE") || check_if("GRANT")) {
    // iterate list of privileges (and maybe column list) until an ON keyword
    std::vector<std::pair<std::string, std::string>> privs;
    std::string object_type;
    std::string priv_level;

    filtered_stmt = consume();

    bool is_revoke = (shcore::str_caseeq(filtered_stmt, "REVOKE"));

    size_t priv_list_end = 0;
    bool role_grant = true;

    {
      const auto t_save = t;

      // check if there's an ON xxx.yyy clause in the stmt, if there isn't one
      // then this is a role grant/revoke
      while (!check_if("TO") && !check_if("FROM")) {
        if (check_if("ON")) {
          role_grant = false;
          break;
        }
        consume();
      }
      t = t_save;
    }

    // priv_type
    std::string priv = consume();

    if (shcore::str_caseeq(priv, "PROXY")) {
      if (!advance_if("ON", &priv_list_end))
        throw std::runtime_error("Malformed grant/revoke statement: " + stmt);
      consume();  // user
      if (consume() != "@")
        throw std::runtime_error("Malformed grant/revoke statement: " + stmt);
      consume();  // host

      privs.emplace_back(priv, "");
    } else if (role_grant) {
      // this is granting a role
      while (!check_if("TO") && !check_if("FROM")) {
        priv += consume();
      }

      priv_list_end = t->second;
      privs.emplace_back(priv, "");
    } else {
      for (;;) {
        // keep consuming until 'ON' ',' or '('
        while (!check_if("ON") && !check_if("(") && !check_if(",")) {
          priv += " " + consume();
        }

        std::string cols;
        if (advance_if("(")) {
          // optional (column_list)
          cols = "(";
          while (!advance_if(")")) {
            cols += consume();
          }
          cols += ")";
        }

        privs.emplace_back(priv, cols);

        if (advance_if("ON", &priv_list_end)) {
          break;
        }
        if (!advance_if(",")) {
          throw std::runtime_error("Malformed grant/revoke statement: " + stmt);
        }
        priv = consume();
      }

      // parse everything until FROM/TO
      if (!advance_if("FROM") && !advance_if("TO")) {
        if (check_if("TABLE") || check_if("FUNCTION") ||
            check_if("PROCEDURE")) {
          object_type = consume();
        }
        while (!advance_if("FROM") && !advance_if("TO")) {
          priv_level += consume();
        }
      }
    }

    // do the filtering
    for (const auto &p : privs) {
      if (filter(is_revoke, p.first, p.second, object_type, priv_level)) {
        filtered_stmt.append(" ").append(p.first).append(p.second).append(",");
      }
    }
    if (filtered_stmt.back() == ',') {
      filtered_stmt.pop_back();
      filtered_stmt.push_back(' ');
    } else {
      // all privs got stripped
      return "";
    }

    // copy the rest of the stmt
    filtered_stmt += stmt.substr(priv_list_end);
  } else {
    throw std::runtime_error("Malformed grant/revoke statement: " + stmt);
  }

  return filtered_stmt;
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
        if (!shcore::str_caseeq_mv(*token, "DATA", "INDEX")) return true;
        *token = it->get_next_token();
        return !shcore::str_caseeq(*token, "DIRECTORY");
      });
}

bool check_create_table_for_encryption_option(const std::string &create_table,
                                              std::string *rewritten) {
  // Comment out ENCRYPTION option.

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
      auto end = mysqlshdk::utils::span_cstyle_sql_comment(create_table, start);
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
        auto end =
            mysqlshdk::utils::span_cstyle_sql_comment(statement, prev_pos);
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
    if (shcore::str_caseeq_mv(token, "VIEW", "EVENT", "FUNCTION", "PROCEDURE",
                              "TRIGGER"))
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
    if (shcore::str_caseeq_mv(token, "PROCEDURE", "FUNCTION")) {
      func_or_proc = true;
      continue;
    }
    if (func_or_proc &&
        (shcore::str_caseeq_mv(token, "BEGIN", "RETURN", "SELECT", "UPDATE",
                               "DELETE", "INSERT", "REPLACE", "CREATE", "ALTER",
                               "DROP", "RENAME", "TRUNCATE", "CALL", "PREPARE",
                               "EXECUTE", "REPEAT", "DO", "WHILE", "LOOP",
                               "IMPORT", "LOAD", "TABLE", "WITH") ||
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
    const std::string &statement, bool fulltext_only, std::string *rewritten,
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
  std::string auto_increment_column;

  while (it.valid() && brace_count > 0) {
    bool index_declaration = false;
    bool constraint = false;
    std::string column_name;
    auto potential_coma = it.position() - 1;
    auto token = it.get_next_token();
    auto start = it.position() - token.length();

    if (token[0] == '`') {
      column_name = token;
    } else if (fulltext_only) {
      if (shcore::str_caseeq(token, "FULLTEXT")) index_declaration = true;
    } else {
      if (shcore::str_caseeq_mv(token, "FULLTEXT", "UNIQUE", "KEY", "INDEX",
                                "SPATIAL"))
        index_declaration = true;
      else
        // Note: FKs on FULLTEXT indexes are not supported, so if we're doing
        // FULLTEXT only we can ignore FKs
        constraint = shcore::str_caseeq(token, "CONSTRAINT");
    }

    while (it.valid() && brace_count > 0) {
      token = it.get_next_token();
      if (token == "(")
        ++brace_count;
      else if (token == ")")
        --brace_count;
      else if (brace_count == 1 && token == ",")
        break;
      else if (constraint && (shcore::str_caseeq_mv(token, "KEY", "INDEX")))
        index_declaration = true;
      else if (!fulltext_only && !column_name.empty() &&
               shcore::str_caseeq(token, "AUTO_INCREMENT"))
        auto_increment_column = "(" + column_name + ")";
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

    // auto_increment columns cannot be foreign keys
    if (!auto_increment_column.empty() && !constraint &&
        index_definition.find(auto_increment_column) != std::string::npos) {
      offsets.pop_back();
      continue;
    }

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
