/*
 * Copyright (c) 2020, 2025, Oracle and/or its affiliates.
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

#include "modules/util/dump/compatibility.h"

#include <cassert>
#include <regex>
#include <unordered_map>
#include <utility>

#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_lexing.h"
#include "mysqlshdk/libs/utils/utils_sqlstring.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlsh {
namespace compatibility {

const std::set<std::string> k_mysqlaas_allowed_privileges = {
    // each account has USAGE
    "USAGE",
    // global static privileges
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
    "PROXY",  // this privilege is granted to the initial user account itself
    "REFERENCES",
    "REPLICATION CLIENT",
    "REPLICATION SLAVE",
    "SELECT",
    "SHOW DATABASES",
    "SHOW VIEW",
    "TRIGGER",
    "UPDATE",
    // global dynamic privileges
    "APPLICATION_PASSWORD_ADMIN",
    "AUDIT_ADMIN",
    "BACKUP_ADMIN",
    "CONNECTION_ADMIN",
    "FLUSH_OPTIMIZER_COSTS",
    "FLUSH_PRIVILEGES",
    "FLUSH_STATUS",
    "FLUSH_TABLES",
    "FLUSH_USER_RESOURCES",
    "OPTION_TRACKER_OBSERVER",
    "REPLICATION_APPLIER",
    "ROLE_ADMIN",
    "SHOW_ROUTINE",
    "TRANSACTION_GTID_TAG",
    "XA_RECOVER_ADMIN",
};

const std::unordered_set<std::string> k_mds_users_with_system_user_priv = {
    "mysql.infoschema", "mysql.session", "mysql.sys",
    "ociadmin",         "ocidbm",        "ocirpl",
};

namespace {
using Offset = std::pair<std::size_t, std::size_t>;
using Offsets = std::vector<Offset>;
using SQL_iterator = mysqlshdk::utils::SQL_iterator;

struct Comment_offset {
  Offset offset;
  bool inside_hint;
};

using Coffsets = std::vector<Comment_offset>;

std::string replace_at_offsets(std::string_view s, const Offsets &offsets,
                               const std::string &target) {
  if (offsets.empty()) return std::string{s};

  std::string out;

  out += s.substr(0, offsets[0].first);
  out += target;

  for (std::size_t i = 1; i < offsets.size(); i++) {
    out += s.substr(offsets[i - 1].second,
                    offsets[i].first - offsets[i - 1].second);
    out += target;
  }

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

void skip_columns_definition(SQL_iterator *it) {
  if (!shcore::str_caseeq(it->next_token(), "CREATE") ||
      !shcore::str_caseeq(it->next_token(), "TABLE"))
    throw std::runtime_error("Malformed create table statement");
  while (it->valid() && it->next_token() != "(") {
  }
  while (it->valid() && it->next_token() != ")") {
  }
  if (!it->valid())
    throw std::runtime_error(
        "Malformed create table statement - columns definition not found");
}

bool comment_out_option_with_string(
    const std::string &s, std::string *rewritten,
    std::function<bool(std::string_view *token, SQL_iterator *it)>
        skip_condition) {
  Coffsets offsets;
  SQL_iterator it(s, 12);
  std::size_t prev_pos = 0, end = 0;
  auto token = it.next_token();
  bool prev_added = false;
  while (!token.empty()) {
    auto start = it.position() - token.size();
    bool hint = it.inside_hint();
    if (skip_condition(&token, &it) ||
        (end = eat_string_val(s, it.position())) == std::string_view::npos) {
      prev_pos = it.position() - token.size();
      prev_added = false;
      token = it.next_token();
      continue;
    }
    it.set_position(end);
    token = it.next_token();

    if (token == ",") {
      end = it.position();
      token = it.next_token();
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

inline bool is_quote(char c) { return '\'' == c || '"' == c || '`' == c; }

void unquote_string(std::string *s) {
  if (!s->empty() && is_quote((*s)[0])) {
    *s = shcore::unquote_string(*s, (*s)[0]);
  }
}

std::string get_account(SQL_iterator *it) {
  auto user = it->next_token();  // IF or user

  if (shcore::str_caseeq(user, "IF")) {
    it->next_token();         // NOT
    it->next_token();         // EXISTS
    user = it->next_token();  // user
  }

  assert(!user.empty());
  const size_t pos = it->position();

  // The host part can be omitted for the role name (defaults to '%')
  if (it->next_token() != "@") {
    it->set_position(pos);
    return std::string{user};
  }

  const auto host = it->next_token();
  assert(!host.empty());
  return std::string{user} + "@" + std::string{host};
}

/**
 * Expects iterator to CREATE USER statement, will stop after account is found.
 *
 * @returns account parsed from the CREATE USER statement
 */
std::string create_user_parse_account(SQL_iterator *it) {
  if (!shcore::str_caseeq(it->next_token(), "CREATE") ||
      !shcore::str_caseeq(it->next_token(), "USER")) {
    throw std::runtime_error(
        "This check can be only performed on CREATE USER statements");
  }

  return get_account(it);
}

}  // namespace

std::vector<std::string> check_privileges(
    const std::string &grant, std::string *out_rewritten_grant,
    const std::set<std::string> &privileges) {
  std::vector<std::string> res;
  Offsets offsets;

  SQL_iterator it(grant);
  auto token = it.next_token();
  if (!shcore::str_caseeq(token, "GRANT", "REVOKE"))
    throw std::runtime_error("Malformed grant statement: " + grant);
  const auto first_token_size = token.size();

  int remaining_priv = 0;
  bool last_removed = false;
  while (it.valid()) {
    auto prev_pos = it.position() - 1;
    token = it.next_token();
    auto after_pos = it.position();
    auto next = it.next_token();
    // @ is reported as a token for roles, it is skipped as this function only
    // searches for individual grants
    while (token == "@" && next == ",") {
      token = it.next_token();
      after_pos = it.position();
      next = it.next_token();
    }

    std::string privilege{token};

    while (!next.empty() && next != "," && next != "ON" && next != "TO") {
      privilege += ' ';
      privilege += next;
      after_pos = it.position();
      next = it.next_token();
    }

    if (privilege != "@") {
      auto pit = privileges.find(shcore::str_upper(privilege));
      if (pit != privileges.end()) {
        ++remaining_priv;
        last_removed = false;
      } else {
        res.emplace_back(privilege);
        if (out_rewritten_grant) {
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
    if (next.empty() || shcore::str_caseeq(next, "ON", "TO", "FROM")) break;
  }

  // update output variable only if there are restricted grants
  if (!res.empty() && out_rewritten_grant) {
    if (remaining_priv == 0)
      out_rewritten_grant->clear();
    else
      *out_rewritten_grant = replace_at_offsets(grant, offsets, "");
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
  std::vector<std::pair<std::string_view, size_t>> tokens;
  {
    SQL_iterator it(stmt, 0, false);
    while (it.valid()) {
      tokens.push_back(it.next_token_and_offset());
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

  const auto consume = [&t, t_end, stmt]() -> std::string_view {
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
    std::string priv{consume()};

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
          priv += ' ';
          priv += consume();
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

std::string_view is_grant_on_object_from_mysql_schema(
    const std::string &grant) {
  SQL_iterator it(grant, 0, false);

  if (shcore::str_caseeq(it.next_token(), "GRANT", "REVOKE")) {
    while (!shcore::str_caseeq(it.next_token(), "ON", "")) {
    }

    for (auto token = it.next_token();
         !token.empty() && !shcore::str_caseeq(token, "TO");
         token = it.next_token()) {
      if ((shcore::str_ibeginswith(token, "mysql.") &&
           !shcore::str_caseeq(token, "mysql.*")) ||
          (shcore::str_ibeginswith(token, "`mysql`.") &&
           !shcore::str_caseeq(token, "`mysql`.*")))
        return token;
    }
  }

  return "";
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
      create_table, rewritten, [](std::string_view *token, SQL_iterator *it) {
        if (!shcore::str_caseeq(*token, "DATA", "INDEX")) return true;
        *token = it->next_token();
        return !shcore::str_caseeq(*token, "DIRECTORY");
      });
}

bool check_create_table_for_encryption_option(const std::string &create_table,
                                              std::string *rewritten) {
  // Comment out ENCRYPTION option if set to Y, this also handles CREATE SCHEMA.
  // [DEFAULT] ENCRYPTION [=] {'Y' | 'N'}
  SQL_iterator it(create_table, 0, false);

  std::string_view token;
  std::size_t start;
  bool hint;

  while (it.valid()) {
    token = it.next_token();

    start = it.position() - token.length();
    hint = it.inside_hint();

    if (shcore::str_caseeq(token, "DEFAULT")) {
      token = it.next_token();
    }

    if (shcore::str_caseeq(token, "ENCRYPTION")) {
      token = it.next_token();

      if ("=" == token) {
        token = it.next_token();
      }

      if (shcore::str_caseeq(token, "'Y'", "\"Y\"")) {
        // encryption is enabled
        if (rewritten) {
          *rewritten = comment_out_offsets(
              create_table, {Comment_offset{{start, it.position()}, hint}});
        }

        return true;
      } else {
        // encryption is disabled, no change needed
        break;
      }
    }
  }

  // no changes
  if (rewritten) {
    *rewritten = create_table;
  }

  return false;
}

std::string check_create_table_for_engine_option(
    const std::string &create_table, std::string *rewritten,
    const std::string &target) {
  std::string res;
  Offsets offsets;
  SQL_iterator it(create_table);
  skip_columns_definition(&it);
  std::string token;
  while (!(token = it.next_token()).empty()) {
    if (!shcore::str_caseeq(token, "ENGINE")) continue;
    auto name = it.next_token();
    if (name == "=") name = it.next_token();
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
  while (!(token = it.next_token()).empty()) {
    if (!shcore::str_caseeq(token, "TABLESPACE")) {
      prev_pos = it.position() - token.size();
      continue;
    }
    auto start = it.position() - token.size();
    auto name = it.next_token();
    if (name == "=") name = it.next_token();

    // Leave whitelisted tablespaces
    size_t iw = 0;
    std::string_view n = name;

    if (n[0] == '`') {
      n.remove_prefix(1);
    }

    for (; iw < whitelist.size(); ++iw) {
      if (shcore::str_ibeginswith(n, whitelist[iw])) {
        break;
      }
    }
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
    if (shcore::str_caseeq("STORAGE", it.next_token())) {
      // either DISK or MEMORY
      it.next_token();
      end = it.position();
    }
    offsets.emplace_back(create_table[prev_pos] == ',' ? prev_pos : start, end);
  }

  if (rewritten) *rewritten = replace_at_offsets(create_table, offsets, "");

  return !offsets.empty();
}

bool check_create_table_for_fixed_row_format(const std::string &create_table,
                                             std::string *rewritten) {
  Offsets offsets;
  SQL_iterator it(create_table);

  skip_columns_definition(&it);

  while (it.valid()) {
    auto token = it.next_token();

    if (shcore::str_caseeq(token, "ROW_FORMAT")) {
      Offset offset;

      offset.first = it.position() - token.length();

      token = it.next_token();

      if (shcore::str_caseeq(token, "=")) {
        token = it.next_token();
      }

      if (shcore::str_caseeq(token, "FIXED")) {
        offset.second = it.position();

        // consume comma, if it's there
        if (shcore::str_caseeq(it.next_token(), ",")) {
          offset.second = it.position();
        }

        offsets.emplace_back(std::move(offset));
      }

      // no need to parse the rest
      break;
    }
  }

  // if new statement ends with comma, strip it as well
  if (rewritten)
    *rewritten = shcore::str_strip(
        replace_at_offsets(create_table, offsets, ""), " \r\n\t,");

  return !offsets.empty();
}

std::vector<std::string> check_statement_for_charset_option(
    const std::string &statement, std::string *rewritten,
    const std::vector<std::string> &whitelist) {
  Offsets offsets;
  SQL_iterator it(statement, 0, true);
  it.next_token();

  // CHARSET keyword can be a column name but cannot be a column option
  std::size_t end_of_col_def = 0;
  if (shcore::str_caseeq(it.next_token(), "TABLE")) {
    SQL_iterator itc(statement, 0);
    skip_columns_definition(&itc);
    end_of_col_def = itc.position();
  }

  std::vector<std::string> res;
  std::string token;
  while (!(token = it.next_token()).empty()) {
    std::size_t prev_pos = 0;
    if (shcore::str_caseeq(token, "DEFAULT")) {
      prev_pos = it.position() - token.size();
      token = it.next_token();
    }
    bool collate = false;
    if (shcore::str_caseeq(token, "COLLATE")) {
      collate = true;
    } else if (shcore::str_caseeq(token, "CHARSET")) {
      if (it.position() < end_of_col_def) continue;
      collate = false;
    } else if (shcore::str_caseeq(token, "CHARACTER")) {
      if (prev_pos == 0) prev_pos = it.position() - token.size();
      if (!shcore::str_caseeq(it.next_token(), "SET")) continue;
      collate = false;
    } else {
      continue;
    }

    if (prev_pos == 0) prev_pos = it.position() - token.size();
    bool hint = it.inside_hint();

    while ((token = it.next_token()) == "=") {
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
  SQL_iterator it(statement, 0, false);
  auto token = it.next_token();

  if (!shcore::str_caseeq(token, "CREATE")) return user;

  while (!(token = it.next_token()).empty()) {
    if (shcore::str_caseeq(token, "VIEW", "EVENT", "FUNCTION", "PROCEDURE",
                           "TRIGGER")) {
      break;
    }

    if (!shcore::str_caseeq(token, "DEFINER")) continue;

    const auto start = it.position() - token.size();

    // definer is in the form of DEFINER = user, this could also be SQL SECURITY
    // DEFINER clause in a CREATE VIEW statement
    if (shcore::str_caseeq((token = it.next_token()), "=")) {
      // user
      user = get_account(&it);

      if (rewritten) {
        auto i = it.position();

        while (std::isblank(statement[i])) ++i;

        *rewritten = statement.substr(0, start) + statement.substr(i);
      }

      break;
    }
  }

  return user;
}

bool check_statement_for_sqlsecurity_clause(const std::string &statement,
                                            std::string *rewritten) {
  SQL_iterator it(statement, 0, false);
  auto token = it.next_token();

  if (!shcore::str_caseeq(token, "CREATE")) return false;

  const auto add_sql_security_invoker = [rewritten, &statement, &it, &token]() {
    if (rewritten) {
      const auto pos = it.position() - token.size();
      *rewritten = statement.substr(0, pos) + "SQL SECURITY INVOKER\n" +
                   statement.substr(pos);
    }
  };

  const auto is_sql_security_clause = [&it, &token]() {
    if (shcore::str_caseeq(token, "SQL") &&
        shcore::str_caseeq((token = it.next_token()), "SECURITY")) {
      return true;
    }

    return false;
  };

  const auto set_sql_security_invoker = [rewritten, &statement, &it, &token]() {
    token = it.next_token();

    if (shcore::str_caseeq(token, "INVOKER")) {
      return false;
    } else {
      if (rewritten) {
        *rewritten = statement.substr(0, it.position() - token.size()) +
                     "INVOKER" + statement.substr(it.position());
      }

      return true;
    }
  };

  bool func_or_proc = false;

  while (it.valid()) {
    token = it.next_token();

    if (shcore::str_caseeq(token, "PROCEDURE", "FUNCTION")) {
      func_or_proc = true;
      break;
    }

    if (shcore::str_caseeq(token, "VIEW")) {
      // this is a CREATE VIEW statement, there was no SQL SECURITY clause
      add_sql_security_invoker();
      return true;
    }

    if (is_sql_security_clause()) {
      return set_sql_security_invoker();
    }
  }

  if (func_or_proc) {
    // find starting parenthesis
    while (!shcore::str_caseeq((token = it.next_token()), "("))
      ;

    const auto find_closing_parenthesis = [&it, &token]() {
      assert(shcore::str_caseeq(token, "("));

      std::size_t count = 1;

      while (count) {
        token = it.next_token();

        if (shcore::str_caseeq(token, "(")) {
          ++count;
        } else if (shcore::str_caseeq(token, ")")) {
          --count;
        }
      }
    };

    // move past parameters
    find_closing_parenthesis();

    while (it.valid()) {
      token = it.next_token();

      if (shcore::str_caseeq(token, "RETURNS")) {
        // move past type

        // data type
        token = it.next_token();

        if (shcore::str_caseeq(token, "NATIONAL")) {
          // it was NATIONAL keyword, data type follows
          token = it.next_token();
        } else if (shcore::str_caseeq(token, "NCHAR")) {
          const auto pos = it.position();
          token = it.next_token();

          // NCHAR can be followed by VARCHAR, VARCHARACTER, check if that's the
          // case
          if (!shcore::str_caseeq(token, "VARCHAR", "VARCHARACTER")) {
            // it's not, step back
            it.set_position(pos);
          }
        } else if (shcore::str_caseeq(token, "LONG")) {
          const auto pos = it.position();
          token = it.next_token();

          // LONG can be followed by VARBINARY, VARCHAR, VARCHARACTER
          // or by { CHAR | CHARACTER } VARYING
          if (!shcore::str_caseeq(token, "VARBINARY", "VARCHAR",
                                  "VARCHARACTER") &&
              !(shcore::str_caseeq(token, "CHAR", "CHARACTER") &&
                shcore::str_caseeq((token = it.next_token()), "VARYING"))) {
            // this is not the case, step back
            it.set_position(pos);
          }
        }

        bool parentheses = false;

        while (it.valid()) {
          const auto pos = it.position();
          token = it.next_token();

          if (!parentheses && shcore::str_caseeq(token, "(")) {
            // (M), (M,D), ('value1','value2',...), (fsp)
            find_closing_parenthesis();
            parentheses = true;
          } else if (shcore::str_caseeq(token, "SIGNED", "UNSIGNED", "ZEROFILL",
                                        "BINARY", "ASCII", "UNICODE", "BYTE",
                                        "PRECISION", "VARYING")) {
            // field_options:
            //   [ SIGNED ] [ UNSIGNED ] [ ZEROFILL ]
            // opt_charset_with_opt_binary:
            //   ASCII | BINARY ASCII | ASCII BINARY
            //   UNICODE | UNICODE BINARY | BINARY UNICODE
            //   BYTE
            //   BINARY
            //   character_set charset_name [ BINARY ]
            //   BINARY character_set charset_name
            // real_type:
            //   { DOUBLE | FLOAT8 } PRECISION
            // VARYING - suffix for CHAR, specifies VARCHAR
            // nothing to do
          } else if (shcore::str_caseeq(token, "CHAR", "CHARACTER")) {
            // CHAR SET charset_name
            // CHARACTER SET charset_name
            // consume two subsequent tokens
            it.next_token();
            it.next_token();
          } else if (shcore::str_caseeq(token, "CHARSET", "COLLATE")) {
            // CHARSET charset_name
            // COLLATE collation_name
            // consume subsequent token
            it.next_token();
          } else {
            // we're past the data type, move back
            it.set_position(pos);
            break;
          }
        }
      } else if (shcore::str_caseeq(token, "COMMENT", "LANGUAGE", "NOT",
                                    "CONTAINS", "NO")) {
        // COMMENT 'string'
        // LANGUAGE SQL
        // NOT DETERMINISTIC
        // CONTAINS SQL
        // NO SQL
        // consume subsequent token
        it.next_token();
      } else if (shcore::str_caseeq(token, "READS", "MODIFIES")) {
        // READS SQL DATA
        // MODIFIES SQL DATA
        // consume two subsequent tokens
        it.next_token();
        it.next_token();
      } else if (shcore::str_caseeq(token, "DETERMINISTIC")) {
        // nothing to do
      } else if (is_sql_security_clause()) {
        return set_sql_security_invoker();
      } else {
        // beginning of the routine_body, there was no SQL SECURITY clause
        add_sql_security_invoker();
        return true;
      }
    }
  }

  return false;
}

Deferred_statements check_create_table_for_indexes(
    std::string_view statement, const std::string &table_name,
    bool fulltext_only) {
  Deferred_statements ret;
  Offsets offsets;
  SQL_iterator it(statement, 0, false);

  if (!shcore::str_caseeq(it.next_token(), "CREATE") ||
      !shcore::str_caseeq(it.next_token(), "TABLE"))
    throw std::runtime_error(
        "Index check can be only performed on create table statements");

  while (it.valid() && it.next_token() != "(") {
  }

  int brace_count = 1;
  std::string auto_increment_column;

  while (it.valid() && brace_count > 0) {
    bool index_declaration = false;
    bool constraint = false;
    std::string column_name;
    bool generated_virtual = false;
    auto potential_comma = it.position() - 1;
    auto token = it.next_token();
    auto start = it.position() - token.length();
    auto storage = &ret.index_info.regular;

    if (token[0] == '`') {
      column_name = token;
    } else if (fulltext_only) {
      if (shcore::str_caseeq(token, "FULLTEXT")) {
        index_declaration = true;
        storage = &ret.index_info.fulltext;
      }
    } else {
      if (shcore::str_caseeq(token, "FULLTEXT", "UNIQUE", "KEY", "INDEX",
                             "SPATIAL")) {
        index_declaration = true;

        if (shcore::str_caseeq(token, "FULLTEXT")) {
          storage = &ret.index_info.fulltext;
        } else if (shcore::str_caseeq(token, "SPATIAL")) {
          storage = &ret.index_info.spatial;
        }
      } else {
        // Note: FKs on FULLTEXT indexes are not supported, so if we're doing
        // FULLTEXT only we can ignore FKs
        constraint = shcore::str_caseeq(token, "CONSTRAINT");
      }
    }

    while (it.valid() && brace_count > 0) {
      token = it.next_token();
      if (shcore::str_caseeq(token, "(")) {
        ++brace_count;
      } else if (shcore::str_caseeq(token, ")")) {
        --brace_count;
      } else if (brace_count == 1 && shcore::str_caseeq(token, ",")) {
        break;
      } else if (constraint) {
        if (shcore::str_caseeq(token, "KEY", "INDEX")) {
          index_declaration = true;
        } else if (shcore::str_caseeq(token, "FOREIGN")) {
          storage = &ret.foreign_keys;
        }
      } else if (!column_name.empty()) {
        if (!fulltext_only && shcore::str_caseeq(token, "AUTO_INCREMENT")) {
          auto_increment_column = column_name;
        } else if (shcore::str_caseeq(token, "AS")) {
          // AS indicates this is a generated column, it is virtual by default
          generated_virtual = true;
        } else if (shcore::str_caseeq(token, "STORED")) {
          // STORED occurs after AS, indicates that the column is not virtual
          generated_virtual = false;
        }
      }
    }

    ret.index_info.has_virtual_columns |= generated_virtual;

    if (!it.valid())
      throw std::runtime_error(
          "Incomplete create_definition in create table statement of table " +
          table_name);
    if (!index_declaration) continue;

    // previous token had to be either ',' or ')'
    const auto end = it.position() - token.length();

    const auto index_clause =
        std::string_view{statement}.substr(start, end - start);

    // do not remove indexes specified on AUTO_INCREMENT columns, as this will
    // result in an invalid SQL:
    //    there can be only one auto column and it must be defined as a key
    // note: AUTO_INCREMENT columns cannot be foreign keys
    if (!auto_increment_column.empty() && !constraint &&
        index_clause.find(auto_increment_column) != std::string::npos) {
      continue;
    }

    if (statement[potential_comma] == ',') {
      if (!offsets.empty() && offsets.back().second == potential_comma)
        offsets.back().second = end;
      else
        offsets.emplace_back(potential_comma, end);
    } else if (statement[end] == ',') {
      offsets.emplace_back(start, end + 1);
    } else {
      offsets.emplace_back(start, end);
    }

    storage->emplace_back(shcore::str_strip(index_clause));
  }

  std::string_view engine;

  while (it.valid()) {
    auto token = it.next_token();

    // SECONDARY_ENGINE [=] engine_name
    if (shcore::str_caseeq(token, "SECONDARY_ENGINE")) {
      const auto start = it.position() - token.length();
      auto secondary_engine = it.next_token();

      if (shcore::str_caseeq(secondary_engine, "=")) {
        secondary_engine = it.next_token();
      }

      // NULL disables the secondary engine
      if (!shcore::str_caseeq(secondary_engine, "NULL")) {
        offsets.emplace_back(start, it.position());
        ret.secondary_engine =
            "ALTER TABLE " + table_name + " SECONDARY_ENGINE=";
        ret.secondary_engine += secondary_engine;
        ret.secondary_engine += ';';
      }
    } else if (shcore::str_caseeq(token, "ENGINE")) {
      // ENGINE [=] engine_name
      engine = it.next_token();

      if (shcore::str_caseeq(engine, "=")) {
        engine = it.next_token();
      }
    }
  }

  if (shcore::str_caseeq(engine, "Lakehouse")) {
    // we prevent any modifications of tables which are using the Lakehouse
    // engine, they don't benefit from index deferral
    return {};
  }

  ret.rewritten = replace_at_offsets(statement, offsets, "");

  return ret;
}

std::set<std::string> check_create_user_for_authentication_plugins(
    std::string_view create_user) {
  SQL_iterator it(create_user, 0, false);

  create_user_parse_account(&it);

  std::set<std::string> result;

  while (it) {
    if (it.consume_tokens("IDENTIFIED", "WITH")) {
      std::string auth_plugin{it.next_token()};

      unquote_string(&auth_plugin);

      if (!auth_plugin.empty()) {
        result.emplace(std::move(auth_plugin));
      }
    }
  }

  return result;
}

bool check_create_user_for_empty_password(const std::string &create_user) {
  SQL_iterator it(create_user, 0, false);

  create_user_parse_account(&it);

  // end of statement, or there is no IDENTIFIED clause, there is no password
  if (!it.valid() || !shcore::str_caseeq(it.next_token(), "IDENTIFIED")) {
    return true;
  }

  std::string password;

  if (shcore::str_caseeq(it.next_token(), "BY")) {
    // IDENTIFIED BY
    password = it.next_token();

    if (shcore::str_caseeq(password, "PASSWORD")) {
      password = it.next_token();
    } else if (shcore::str_caseeq(password, "RANDOM")) {
      // random password, not empty
      return false;
    }
  } else {
    // IDENTIFIED WITH, "WITH" already consumed, next is auth_plugin
    it.next_token();

    // end of statement, no password
    if (!it.valid()) {
      return true;
    }

    const auto token = it.next_token();

    if (shcore::str_caseeq(token, "BY")) {
      password = it.next_token();

      if (shcore::str_caseeq(password, "RANDOM")) {
        // random password, not empty
        return false;
      }
    } else if (shcore::str_caseeq(token, "AS")) {
      // AS 'auth_string', password cannot be checked
      return false;
    } else {
      // no password
      return true;
    }
  }

  return password.empty() ||
         shcore::unquote_string(password, password[0]).empty();
}

std::string convert_create_user_to_create_role(const std::string &create_user) {
  SQL_iterator it(create_user, 0, false);

  const auto account = create_user_parse_account(&it);
  const auto begin_account_config = it.position();

  std::string sql = "CREATE ROLE IF NOT EXISTS " + account;

  // find beginning and end of DEFAULT ROLE clause
  auto begin = std::string::npos;
  auto end = begin;

  while (it.valid()) {
    const auto token = it.next_token_and_offset();

    if (shcore::str_caseeq(token.first, "DEFAULT") &&
        shcore::str_caseeq(it.next_token(), "ROLE")) {
      begin = token.second;

      do {
        // skip role
        get_account(&it);
        end = it.position();
      } while (shcore::str_caseeq(it.next_token(), ","));

      break;
    }
  }

  const std::string alter_user = ";\nALTER USER " + account + " ";

  // if there was a DEFAULT ROLE clause, add it to SQL
  if (std::string::npos != begin) {
    sql += alter_user + create_user.substr(begin, end - begin);
  }

  // get the account configuration minus the DEFAULT ROLE clause
  std::string account_config;

  if (std::string::npos != begin) {
    account_config = shcore::str_strip(
        create_user.substr(begin_account_config, begin - begin_account_config));
    account_config += " ";
    account_config += shcore::str_strip(create_user.substr(end));
  } else {
    account_config = create_user.substr(begin_account_config);
  }

  account_config = shcore::str_strip(account_config);

  if (!account_config.empty()) {
    sql += alter_user + account_config;
  }

  return sql;
}

void add_pk_to_create_table(std::string_view statement,
                            std::string *rewritten) {
  assert(rewritten);

  SQL_iterator it(statement, 0, false);

  if (!shcore::str_caseeq(it.next_token(), "CREATE") ||
      !shcore::str_caseeq(it.next_token(), "TABLE")) {
    throw std::runtime_error(
        "Invisible primary key can be only added to CREATE TABLE statements");
  }

  // find opening parenthesis of create_definition
  while (it.valid() && it.next_token() != "(") {
  }

  if (!it.valid()) {
    // no create_definition -> CREATE TABLE ... LIKE or CREATE TABLE ... SELECT
    throw std::runtime_error("Unsupported CREATE TABLE statement");
  }

  // we're going to insert the PK as the first column
  const auto position = it.position();
  int brace_count = 1;
  std::string token;

  while (it.valid() && brace_count > 0) {
    token = it.next_token();

    if (shcore::str_caseeq(token, "(")) {
      ++brace_count;
    } else if (shcore::str_caseeq(token, ")")) {
      --brace_count;
    }
  }

  while (it.valid()) {
    if (shcore::str_caseeq(it.next_token(), "SELECT")) {
      // CREATE TABLE ... SELECT
      throw std::runtime_error("Unsupported CREATE TABLE statement");
    }
  }

  rewritten->clear();

  rewritten->append(statement.substr(0, position));
  rewritten->append(
      "`my_row_id` BIGINT UNSIGNED AUTO_INCREMENT INVISIBLE PRIMARY KEY,");
  rewritten->append(statement.substr(position));
}

bool add_pk_to_create_table_if_missing(const std::string &statement,
                                       std::string *rewritten,
                                       bool ignore_pke) {
  assert(rewritten);

  SQL_iterator it(statement, 0, false);

  if (!shcore::str_caseeq(it.next_token(), "CREATE") ||
      !shcore::str_caseeq(it.next_token(), "TABLE")) {
    throw std::runtime_error(
        "Invisible primary key can be only added to CREATE TABLE statements");
  }

  // find opening parenthesis of create_definition
  while (it.valid() && it.next_token() != "(") {
  }

  if (!it.valid()) {
    // no create_definition -> CREATE TABLE ... LIKE or CREATE TABLE ... SELECT
    throw std::runtime_error("Unsupported CREATE TABLE statement");
  }

  // we're going to insert the PK as the first column
  const auto position = it.position();
  int brace_count = 1;

  const auto skip_create_definition = [&brace_count, &it]() {
    while (it.valid() && brace_count > 0) {
      const auto token = it.next_token();

      if (shcore::str_caseeq(token, "(")) {
        ++brace_count;
      } else if (shcore::str_caseeq(token, ")")) {
        --brace_count;
      } else if (shcore::str_caseeq(token, ",") && 1 == brace_count) {
        // comma directly inside of create_definition
        return;
      }
    }
  };

  const auto next_token = [&it]() {
    if (!it.valid()) {
      throw std::runtime_error("Malformed CREATE TABLE statement");
    }

    return it.next_token();
  };

  std::string token;
  bool first_token = true;
  std::vector<std::vector<std::string>> unique_indexes;
  // column name -> NOT NULL
  std::unordered_map<std::string, bool> columns;

  while (it.valid() && brace_count > 0) {
    token = it.next_token();

    if (shcore::str_caseeq(token, "LIKE") && first_token) {
      // CREATE TABLE ... LIKE
      throw std::runtime_error("Unsupported CREATE TABLE statement");
    }

    first_token = false;

    if (shcore::str_caseeq(token, "INDEX", "KEY", "FULLTEXT", "SPATIAL")) {
      // {INDEX | KEY} [index_name] [index_type] (key_part,...)
      //   [index_option] ...
      // or
      // {FULLTEXT | SPATIAL} [INDEX | KEY] [index_name] (key_part,...)
      //   [index_option] ...
      skip_create_definition();
      continue;
    }

    if (shcore::str_caseeq(token, "CONSTRAINT")) {
      token = next_token();

      if (!shcore::str_caseeq(token, "PRIMARY", "UNIQUE", "FOREIGN", "CHECK")) {
        // symbol name, skip it
        token = next_token();
      }
    }

    if (shcore::str_caseeq(token, "PRIMARY")) {
      token = next_token();

      if (!shcore::str_caseeq(token, "KEY")) {
        throw std::runtime_error("Malformed CREATE TABLE statement");
      }

      // we don't care about the rest
      return false;
    } else if (shcore::str_caseeq(token, "UNIQUE")) {
      // [CONSTRAINT [symbol]] UNIQUE [INDEX | KEY]
      //  [index_name] [index_type] (key_part,...)
      //  [index_option] ...

      // move till first key_part
      do {
        token = next_token();
      } while (!shcore::str_caseeq(token, "("));

      ++brace_count;

      std::vector<std::string> unique_index;
      bool ignore_index = false;

      do {
        // key_part: {col_name [(length)] | (expr)} [ASC | DESC]
        token = next_token();

        if (shcore::str_caseeq(token, "(")) {
          // (expr), we don't allow for such indexes
          ignore_index = true;

          const auto current_brace_count = brace_count;
          ++brace_count;

          while (current_brace_count != brace_count) {
            token = next_token();

            if (shcore::str_caseeq(token, "(")) {
              ++brace_count;
            } else if (shcore::str_caseeq(token, ")")) {
              --brace_count;
            }
          }

          token = next_token();
        } else {
          unique_index.emplace_back(shcore::unquote_identifier(token));

          token = next_token();

          if (shcore::str_caseeq(token, "(")) {
            token = next_token();  // length
            token = next_token();  // )

            if (!shcore::str_caseeq(token, ")")) {
              throw std::runtime_error("Malformed CREATE TABLE statement");
            }

            // move past this part
            token = next_token();
          }
        }

        if (shcore::str_caseeq(token, "ASC", "DESC")) {
          // skip this
          token = next_token();
        }

        // we're now either at comma, or at finishing parenthesis
      } while (!shcore::str_caseeq(token, ")"));

      --brace_count;

      if (!ignore_index) {
        unique_indexes.emplace_back(std::move(unique_index));
      }

      // ignore the rest of create_definition
      skip_create_definition();
      continue;
    } else if (shcore::str_caseeq(token, "FOREIGN", "CHECK")) {
      // [CONSTRAINT [symbol]] FOREIGN KEY
      //   [index_name] (col_name,...)
      //   reference_definition
      // or
      // [CONSTRAINT [symbol]] CHECK (expr) [[NOT] ENFORCED]
      skip_create_definition();
      continue;
    } else if (shcore::str_caseeq(token, ")")) {
      --brace_count;
    } else {
      // col_name column_definition
      auto column_name = shcore::unquote_identifier(token);
      // If neither NULL nor NOT NULL is specified, the column is treated as
      // though NULL had been specified.
      bool not_null = false;

      while (brace_count > 0) {
        token = next_token();

        if (shcore::str_caseeq(token, "NOT")) {
          token = next_token();

          if (!shcore::str_caseeq(token, "NULL")) {
            throw std::runtime_error("Malformed CREATE TABLE statement");
          }

          not_null = true;
        } else if (shcore::str_caseeq(token, "UNIQUE")) {
          unique_indexes.emplace_back(std::vector<std::string>{column_name});
        } else if (shcore::str_caseeq(token, "PRIMARY")) {
          // we don't care about the rest
          return false;
        } else if (shcore::str_caseeq(token, "(")) {
          ++brace_count;
        } else if (shcore::str_caseeq(token, ")")) {
          --brace_count;
        } else if (shcore::str_caseeq(token, ",") && 1 == brace_count) {
          // comma directly inside of create_definition
          break;
        }
      }

      columns.emplace(std::move(column_name), not_null);
    }
  }

  if (!ignore_pke) {
    for (const auto &unique_index : unique_indexes) {
      bool pke = true;

      for (const auto &column : unique_index) {
        pke &= columns[column];
      }

      if (pke) {
        // all columns of an unique index are not null, we have a PK equivalent
        return false;
      }
    }
  }

  while (it.valid()) {
    if (shcore::str_caseeq(it.next_token(), "SELECT")) {
      // CREATE TABLE ... SELECT
      throw std::runtime_error("Unsupported CREATE TABLE statement");
    }
  }

  *rewritten =
      statement.substr(0, position) +
      "`my_row_id` BIGINT UNSIGNED AUTO_INCREMENT INVISIBLE PRIMARY KEY," +
      statement.substr(position);

  return true;
}

std::string convert_grant_to_create_user(
    const std::string &statement, const std::string &authentication_plugin,
    std::string *rewritten) {
  assert(rewritten);

  SQL_iterator it(statement, 0, false);

  if (!shcore::str_caseeq(it.next_token(), "GRANT")) {
    throw std::runtime_error(
        "Only GRANT statement can be converted to CREATE USER statement");
  }

  // move to the account name
  while (it.valid() && !shcore::str_caseeq(it.next_token(), "TO")) {
  }

  auto create_user = "CREATE USER " + get_account(&it);
  auto grant = statement.substr(0, it.position());

  while (it.valid()) {
    auto token = it.next_token();

    if (shcore::str_caseeq(token, "GRANT") &&
        shcore::str_caseeq(it.next_token(), "OPTION")) {
      grant += " WITH GRANT OPTION";
    } else {
      create_user += ' ';
      create_user += token;

      if (shcore::str_caseeq(token, "IDENTIFIED")) {
        const auto pos = it.position();
        token = it.next_token();

        if (shcore::str_caseeq(token, "BY")) {
          // add auth_plugin information
          create_user += " WITH '" + authentication_plugin + "' ";
          token = it.next_token();

          if (shcore::str_caseeq(token, "PASSWORD")) {
            // hashed password
            create_user += "AS";
            // hash follows
            token = it.next_token();
          } else {
            // clear-text password
            throw std::runtime_error(
                "The GRANT statement contains clear-text password which is not "
                "supported");
          }

          create_user += ' ';
          create_user += token;
        } else {
          // it's a WITH token, statement has the auth_plugin information, step
          // back and handle it normally
          it.set_position(pos);
        }
      }
    }
  }

  // if GRANT statement had only WITH GRANT OPTION clause, we're left with
  // dangling WITH, we need to remove it
  const std::string with = " WITH";

  if (shcore::str_endswith(create_user, with)) {
    create_user = create_user.substr(0, create_user.length() - with.length());
  }

  *rewritten = grant;

  return create_user;
}

std::string strip_default_role(const std::string &create_user,
                               std::string *rewritten) {
  std::string ret;
  SQL_iterator it(create_user, 0, false);

  if (it.consume_tokens("CREATE", "USER")) {
    while (it.valid()) {
      auto tok = it.next_token_and_offset();
      if (!shcore::str_caseeq(tok.first, "DEFAULT") ||
          !shcore::str_caseeq(it.next_token(), "ROLE"))
        continue;

      std::string::size_type end = it.position();
      std::string::size_type next_start = end;
      while (true) {
        get_account(&it);
        end = it.position();
        auto next = it.next_token_and_offset();
        next_start = next.second;
        if (next.first != ",") break;
      }

      assert(end != it.position());
      ret = create_user.substr(tok.second, end - tok.second);
      if (rewritten)
        *rewritten =
            create_user.substr(0, tok.second) + create_user.substr(next_start);
      break;
    }
  }

  if (ret.empty() && rewritten != nullptr && rewritten != &create_user)
    *rewritten = create_user;
  return ret;
}

bool contains_sensitive_information(const std::string &statement) {
  // skip quoted strings, we're not interested in these
  SQL_iterator it(statement);

  while (it) {
    const auto token = it.next_token();

    if (shcore::str_caseeq(token, "IDENTIFIED", "PASSWORD")) {
      return true;
    }
  }

  return false;
}

std::string hide_sensitive_information(
    const std::string &statement, std::string_view replacement,
    std::vector<std::string_view> *replaced) {
  assert(replaced);
  replaced->clear();

  const auto starts_with_quote = [](std::string_view s) {
    if (s.empty()) {
      return false;
    }

    return '\'' == s[0] || '"' == s[0];
  };

  Offsets offsets;
  std::string_view token;
  SQL_iterator it(statement, 0, false);

  const auto advance = [&]() { token = it.next_token(); };
  const auto maybe_hide_and_advance = [&]() {
    if (!starts_with_quote(token)) {
      return false;
    }

    offsets.emplace_back(it.position() - token.length(), it.position());
    replaced->emplace_back(token);
    advance();

    return true;
  };
  const auto hide_and_advance = [&]() {
    [[maybe_unused]] const auto result = maybe_hide_and_advance();
    assert(result);
  };

  advance();

  if (shcore::str_caseeq(token, "SET")) {
    advance();

    if (shcore::str_caseeq(token, "PASSWORD")) {
      // replace all quoted strings
      advance();

      if (shcore::str_caseeq(token, "FOR")) {
        // skip the account
        get_account(&it);
        advance();
      }

      do {
        if (!maybe_hide_and_advance()) {
          advance();
        }
      } while (!token.empty());
    }
  }

  do {
    if (shcore::str_ibeginswith(token, "PASSWORD") ||
        shcore::str_iendswith(token, "PASSWORD")) {
      advance();

      if ("=" == token || "(" == token) {
        // assignment or function call
        advance();
        hide_and_advance();
      } else {
        // next item should be the one we hide, don't search further (this could
        // be i.e. PASSWORD EXPIRE)
        maybe_hide_and_advance();
      }
    } else if (shcore::str_caseeq(token, "IDENTIFIED")) {
      advance();

      // IDENTIFIED {VIA|WITH} authentication_rule [OR authentication_rule  ...]
      bool mariadb = false;

      do {
        if ((!mariadb && shcore::str_caseeq(token, "WITH", "VIA")) ||
            (mariadb && shcore::str_caseeq(token, "OR"))) {
          // read authentication plugin
          advance();
          // read keyword
          advance();
        }

        if (!mariadb && shcore::str_caseeq(token, "BY")) {
          advance();

          if (shcore::str_caseeq(token, "PASSWORD")) {
            advance();
          }

          if (shcore::str_caseeq(token, "RANDOM")) {
            advance();

            if (shcore::str_caseeq(token, "PASSWORD")) {
              advance();
            }
          } else {
            hide_and_advance();
          }

          if (shcore::str_caseeq(token, "REPLACE")) {
            advance();
            hide_and_advance();
          }
        } else if (shcore::str_caseeq(token, "AS", "USING")) {
          advance();

          if (shcore::str_caseeq(token, "PASSWORD")) {
            // read (
            advance();
            // read 'password'
            advance();
            // hide and read )
            hide_and_advance();
            // read next token
            advance();
          } else {
            hide_and_advance();
          }
        }

        mariadb = shcore::str_caseeq(token, "OR");
      } while (mariadb);
    } else {
      advance();
    }
  } while (!token.empty());

  if (offsets.empty()) {
    return statement;
  } else {
    const auto target = starts_with_quote(replacement)
                            ? std::string{replacement}
                            : shcore::quote_sql_string(replacement);
    return replace_at_offsets(statement, offsets, target);
  }
}

bool parse_grant_statement(const std::string &statement,
                           Privilege_level_info *info) {
  assert(info);

  mysqlshdk::utils::SQL_iterator it(statement, 0, false);
  const auto type = it.next_token();
  Privilege_level_info result;

  if (shcore::str_caseeq(type, "GRANT")) {
    result.grant = true;
  } else if (shcore::str_caseeq(type, "REVOKE")) {
    result.grant = false;
  } else {
    throw std::runtime_error("Expected GRANT or REVOKE statement");
  }

  Privilege_level_info::Level object_level = Privilege_level_info::Level::TABLE;
  bool is_role = false;
  std::vector<std::string> privileges;

  {
    bool all_privileges_done = false;
    bool privilege_done = false;

    do {
      std::string privilege{it.next_token()};

      do {
        if (!it.valid()) {
          return false;
        }

        auto token = it.next_token();

        if (!is_role && shcore::str_caseeq(token, "@")) {
          is_role = true;
        }

        if (is_role) {
          if (shcore::str_caseeq(token, "@")) {
            privilege += '@';
            privilege += it.next_token();

            token = it.next_token();
          }

          privilege_done = true;
          all_privileges_done =
              shcore::str_caseeq(token, result.grant ? "TO" : "FROM");
        } else {
          if (shcore::str_caseeq(token, "(")) {
            // column-level privilege, move past column_list
            do {
              token = it.next_token();
            } while (!shcore::str_caseeq(token, ")"));

            // read next token (either , or ON)
            token = it.next_token();
          }

          privilege_done = shcore::str_caseeq(token, ",");

          if (shcore::str_caseeq(token, result.grant ? "TO" : "FROM")) {
            // this is GRANT/REVOKE role
            all_privileges_done = is_role = true;
          } else {
            all_privileges_done = shcore::str_caseeq(token, "ON");
          }

          if (!privilege_done && !all_privileges_done) {
            privilege += ' ';
            privilege += token;
          }
        }

        if (privilege_done || all_privileges_done) {
          privileges.emplace_back(std::move(privilege));
        }
      } while (!privilege_done && !all_privileges_done);
    } while (!all_privileges_done);
  }

  if (!it.valid()) {
    return false;
  }

  if (is_role) {
    result.level = Privilege_level_info::Level::ROLE;
  } else {
    for (auto &p : privileges) {
      if (shcore::str_caseeq(p, "PROXY")) {
        return false;
      }

      p = shcore::str_upper(p);
    }

    auto priv_level = it.next_token();

    if (shcore::str_caseeq(priv_level, "TABLE")) {
      object_level = Privilege_level_info::Level::TABLE;
      // priv_level follows
      priv_level = it.next_token();
    } else if (shcore::str_caseeq(priv_level, "FUNCTION", "PROCEDURE")) {
      object_level = Privilege_level_info::Level::ROUTINE;
      // priv_level follows
      priv_level = it.next_token();
    } else if (shcore::str_caseeq(priv_level, "LIBRARY")) {
      object_level = Privilege_level_info::Level::LIBRARY;
      // priv_level follows
      priv_level = it.next_token();
    }

    shcore::split_priv_level(std::string{priv_level}, &result.schema,
                             &result.object);

    if ("*" == result.schema) {
      result.level = Privilege_level_info::Level::GLOBAL;
    } else if ("*" == result.object) {
      result.level = Privilege_level_info::Level::SCHEMA;
    } else {
      result.level = object_level;
    }

    // TO
    it.next_token();
  }

  result.account = get_account(&it);

  if (is_role) {
    result.with_grant = it.consume_tokens("WITH", "ADMIN", "OPTION");
  } else {
    result.with_grant = it.consume_tokens("WITH", "GRANT", "OPTION");
  }

  for (auto &p : privileges) {
    result.privileges.emplace(std::move(p));
  }

  *info = std::move(result);

  return true;
}

std::string to_grant_statement(const Privilege_level_info &info) {
  assert(Privilege_level_info::Level::SCHEMA == info.level);

  std::string result = info.grant ? "GRANT" : "REVOKE";

  result += ' ';
  result += shcore::str_join(info.privileges, ", ");
  result += " ON ";
  result += shcore::quote_identifier(info.schema);
  result += ".* ";
  result += info.grant ? "TO" : "FROM";
  result += ' ';
  result += info.account;

  if (info.with_grant) {
    result += " WITH GRANT OPTION";
  }

  return result;
}

bool supports_set_any_definer_privilege(const mysqlshdk::utils::Version &v) {
  return v.numeric() >= 80200;
}

bool supports_library_ddl(const mysqlshdk::utils::Version &v) {
  return v.numeric() >= 90200;
}

bool supports_vector_store_conversion(const mysqlshdk::utils::Version &v) {
  return v.numeric() >= 90401;
}

bool replace_keyword(std::string_view stmt, std::string_view from,
                     std::string_view to, std::string *result) {
  mysqlshdk::utils::SQL_iterator it(stmt);
  std::pair<std::string_view, size_t> token;

  while (it) {
    token = it.next_token_and_offset();

    if (shcore::str_caseeq(token.first, from)) {
      if (result) {
        // use an additional variable in case of &stmt == result
        std::string output{stmt.substr(0, token.second)};
        output += to;
        output += stmt.substr(token.second + token.first.length());

        *result = std::move(output);
      }

      return true;
    }
  }

  return false;
}

}  // namespace compatibility
}  // namespace mysqlsh
