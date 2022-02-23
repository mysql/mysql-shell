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

#include "mysqlshdk/libs/utils/utils_sqlstring.h"
#include "mysqlshdk/libs/utils/dtoa.h"
#include "mysqlshdk/libs/utils/utils_lexing.h"
#include "mysqlshdk/libs/utils/utils_string.h"

// updated as of 8.0, 2022-02-11
static const char *reserved_keywords[] = {
    "ACCESSIBLE",
    "ADD",
    "ADMIN",  // became nonreserved in 8.0.12
    "ALL",
    "ALTER",
    "ANALYZE",
    "AND",
    "ARRAY",  // added in 8.0.17 (reserved); became nonreserved in 8.0.19
    "AS",
    "ASC",
    "ASENSITIVE",
    "BEFORE",
    "BETWEEN",
    "BIGINT",
    "BINARY",
    "BLOB",
    "BOTH",
    "BY",
    "CALL",
    "CASCADE",
    "CASE",
    "CHANGE",
    "CHAR",
    "CHARACTER",
    "CHECK",
    "COLLATE",
    "COLUMN",
    "CONDITION",
    "CONSTRAINT",
    "CONTINUE",
    "CONVERT",
    "CREATE",
    "CROSS",
    "CUBE",       // became reserved in 8.0.1
    "CUME_DIST",  // added in 8.0.2 (reserved)
    "CURRENT_DATE",
    "CURRENT_TIME",
    "CURRENT_TIMESTAMP",
    "CURRENT_USER",
    "CURSOR",
    "DATABASE",
    "DATABASES",
    "DAY_HOUR",
    "DAY_MICROSECOND",
    "DAY_MINUTE",
    "DAY_SECOND",
    "DEC",
    "DECIMAL",
    "DECLARE",
    "DEFAULT",
    "DELAYED",
    "DELETE",
    "DENSE_RANK",  // added in 8.0.2 (reserved)
    "DESC",
    "DESCRIBE",
    "DETERMINISTIC",
    "DISTINCT",
    "DISTINCTROW",
    "DIV",
    "DOUBLE",
    "DROP",
    "DUAL",
    "EACH",
    "ELSE",
    "ELSEIF",
    "EMPTY",  // added in 8.0.4 (reserved)
    "ENCLOSED",
    "ESCAPED",
    "EXCEPT",
    "EXISTS",
    "EXIT",
    "EXPLAIN",
    "FALSE",
    "FETCH",
    "FIRST_VALUE",  // added in 8.0.2 (reserved)
    "FLOAT",
    "FLOAT4",
    "FLOAT8",
    "FOR",
    "FORCE",
    "FOREIGN",
    "FROM",
    "FULLTEXT",
    "FUNCTION",  // became reserved in 8.0.1
    "GENERATED",
    "GET",
    "GET_MASTER_PUBLIC_KEY",  // added in 8.0.4; became nonreserved in 8.0.11
    "GRANT",
    "GROUP",
    "GROUPING",  // added in 8.0.1 (reserved)
    "GROUPS",    // added in 8.0.2 (reserved)
    "HAVING",
    "HIGH_PRIORITY",
    "HOUR_MICROSECOND",
    "HOUR_MINUTE",
    "HOUR_SECOND",
    "IF",
    "IGNORE",
    "IN",
    "INDEX",
    "INFILE",
    "INNER",
    "INOUT",
    "INSENSITIVE",
    "INSERT",
    "INT",
    "INT1",
    "INT2",
    "INT3",
    "INT4",
    "INT8",
    "INTEGER",
    "INTERVAL",
    "INTO",
    "IO_AFTER_GTIDS",
    "IO_BEFORE_GTIDS",
    "IS",
    "ITERATE",
    "JOIN",
    "JSON_TABLE",  // added in 8.0.4 (reserved)
    "KEY",
    "KEYS",
    "KILL",
    "LAG",         // added in 8.0.2 (reserved)
    "LAST_VALUE",  // added in 8.0.2 (reserved)
    "LATERAL",     // added in 8.0.14 (reserved)
    "LEAD",        // added in 8.0.2 (reserved)
    "LEADING",
    "LEAVE",
    "LEFT",
    "LIKE",
    "LIMIT",
    "LINEAR",
    "LINES",
    "LOAD",
    "LOCALTIME",
    "LOCALTIMESTAMP",
    "LOCK",
    "LONG",
    "LONGBLOB",
    "LONGTEXT",
    "LOOP",
    "LOW_PRIORITY",
    "MASTER_BIND",
    "MASTER_SSL_VERIFY_SERVER_CERT",
    "MATCH",
    "MAXVALUE",
    "MEDIUMBLOB",
    "MEDIUMINT",
    "MEDIUMTEXT",
    "MEMBER",  // added in 8.0.17 (reserved); became nonreserved in 8.0.19
    "MIDDLEINT",
    "MINUTE_MICROSECOND",
    "MINUTE_SECOND",
    "MOD",
    "MODIFIES",
    "NATURAL",
    "NONBLOCKING",  // removed in 5.7.6
    "NOT",
    "NO_WRITE_TO_BINLOG",
    "NTH_VALUE",  // added in 8.0.2 (reserved)
    "NTILE",      // added in 8.0.2 (reserved)
    "NULL",
    "NUMERIC",
    "OF",  // added in 8.0.1 (reserved)
    "ON",
    "OPTIMIZE",
    "OPTIMIZER_COSTS",
    "OPTION",
    "OPTIONALLY",
    "OR",
    "ORDER",
    "OUT",
    "OUTER",
    "OUTFILE",
    "OVER",  // added in 8.0.2 (reserved)
    "PARTITION",
    "PERCENT_RANK",  // added in 8.0.2 (reserved)
    "PERSIST",       // became nonreserved in 8.0.16
    "PERSIST_ONLY",  // added in 8.0.2 (reserved); became nonreserved in 8.0.16
    "PRECISION",
    "PRIMARY",
    "PROCEDURE",
    "PURGE",
    "RANGE",
    "RANK",  // added in 8.0.2 (reserved)
    "READ",
    "READS",
    "READ_WRITE",
    "REAL",
    "RECURSIVE",  // added in 8.0.1 (reserved)
    "REFERENCES",
    "REGEXP",
    "RELEASE",
    "RENAME",
    "REPEAT",
    "REPLACE",
    "REQUIRE",
    "RESIGNAL",
    "RESTRICT",
    "RETURN",
    "REVOKE",
    "RIGHT",
    "RLIKE",
    "ROLE",        // became nonreserved in 8.0.1
    "ROW",         // became reserved in 8.0.2
    "ROWS",        // became reserved in 8.0.2
    "ROW_NUMBER",  // added in 8.0.2 (reserved)
    "SCHEMA",
    "SCHEMAS",
    "SECOND_MICROSECOND",
    "SELECT",
    "SENSITIVE",
    "SEPARATOR",
    "SET",
    "SHOW",
    "SIGNAL",
    "SMALLINT",
    "SPATIAL",
    "SPECIFIC",
    "SQL",
    "SQLEXCEPTION",
    "SQLSTATE",
    "SQLWARNING",
    "SQL_BIG_RESULT",
    "SQL_CALC_FOUND_ROWS",
    "SQL_SMALL_RESULT",
    "SSL",
    "STARTING",
    "STORED",
    "STRAIGHT_JOIN",
    "SYSTEM",  // added in 8.0.3 (reserved)
    "TABLE",
    "TERMINATED",
    "THEN",
    "TINYBLOB",
    "TINYINT",
    "TINYTEXT",
    "TO",
    "TRAILING",
    "TRIGGER",
    "TRUE",
    "UNDO",
    "UNION",
    "UNIQUE",
    "UNLOCK",
    "UNSIGNED",
    "UPDATE",
    "USAGE",
    "USE",
    "USING",
    "UTC_DATE",
    "UTC_TIME",
    "UTC_TIMESTAMP",
    "VALUES",
    "VARBINARY",
    "VARCHAR",
    "VARCHARACTER",
    "VARYING",
    "VIRTUAL",
    "WHEN",
    "WHERE",
    "WHILE",
    "WINDOW",  // added in 8.0.2 (reserved)
    "WITH",
    "WRITE",
    "XOR",
    "YEAR_MONTH",
    "ZEROFILL",
    nullptr,
};

namespace shcore {
//--------------------------------------------------------------------------------------------------

/**
 * Escape a string to be used in a SQL query
 * Same code as used by mysql. Handles null bytes in the middle of the string.
 * If wildcards is true then _ and % are masked as well.
 */
std::string escape_sql_string(const std::string &s, bool wildcards) {
  std::string result;
  result.reserve(s.size());

  for (std::string::const_iterator ch = s.begin(); ch != s.end(); ++ch) {
    char escape = 0;

    switch (*ch) {
      case 0: /* Must be escaped for 'mysql' */
        escape = '0';
        break;
      case '\n': /* Must be escaped for logs */
        escape = 'n';
        break;
      case '\r':
        escape = 'r';
        break;
      case '\\':
        escape = '\\';
        break;
      case '\'':
        escape = '\'';
        break;
      case '"': /* Better safe than sorry */
        escape = '"';
        break;
      case '\032': /* This gives problems on Win32 */
        escape = 'Z';
        break;
      case '_':
        if (wildcards) escape = '_';
        break;
      case '%':
        if (wildcards) escape = '%';
        break;
    }
    if (escape) {
      result.push_back('\\');
      result.push_back(escape);
    } else {
      result.push_back(*ch);
    }
  }
  return result;
}

//--------------------------------------------------------------------------------------------------

// NOTE: This is not the same as escape_sql_string, as embedded ` must be
// escaped as ``, not \` and \ ' and " must not be escaped
std::string escape_backticks(const std::string &s) {
  std::string result;
  result.reserve(s.size());

  for (std::string::const_iterator ch = s.begin(); ch != s.end(); ++ch) {
    char escape = 0;

    switch (*ch) {
      case 0: /* Must be escaped for 'mysql' */
        escape = '0';
        break;
      case '\n': /* Must be escaped for logs */
        escape = 'n';
        break;
      case '\r':
        escape = 'r';
        break;
      case '\032': /* This gives problems on Win32 */
        escape = 'Z';
        break;
      case '`':
        // special case
        result.push_back('`');
        break;
    }
    if (escape) {
      result.push_back('\\');
      result.push_back(escape);
    } else {
      result.push_back(*ch);
    }
  }
  return result;
}

std::string escape_wildcards(const std::string &s) {
  std::string result;
  result.reserve(s.size());

  for (const auto &ch : s) {
    bool escape = (ch == '%' || ch == '_');

    if (escape) result.push_back('\\');

    result.push_back(ch);
  }

  return result;
}

//--------------------------------------------------------------------------------------------------

bool is_reserved_word(const std::string &word) {
  std::string upper = str_upper(word);
  for (const char **kw = reserved_keywords; *kw != NULL; ++kw) {
    if (upper.compare(*kw) == 0) return true;
  }
  return false;
}

//--------------------------------------------------------------------------------------------------

std::string quote_sql_string(const std::string &s) {
  return "'" + escape_sql_string(s) + "'";
}

//--------------------------------------------------------------------------------------------------

std::string quote_identifier(const std::string &identifier) {
  return "`" + str_replace(identifier, "`", "``") + "`";
}

//--------------------------------------------------------------------------------------------------

/**
 * Quotes the given identifier, but only if it needs to be quoted.
 * http://dev.mysql.com/doc/refman/5.1/en/identifiers.html specifies what is
 * allowed in unquoted identifiers. Leading numbers are not strictly forbidden
 * but discouraged as they may lead to ambiguous behavior.
 */
std::string quote_identifier_if_needed(const std::string &ident) {
  bool needs_quotation =
      is_reserved_word(ident);  // check whether it's a reserved keyword
  size_t digits = 0;

  if (!needs_quotation) {
    for (std::string::const_iterator i = ident.begin(); i != ident.end(); ++i) {
      if ((*i >= 'a' && *i <= 'z') || (*i >= 'A' && *i <= 'Z') ||
          (*i >= '0' && *i <= '9') || (*i == '_') || (*i == '$') ||
          ((unsigned char)(*i) > 0x7F)) {
        if (*i >= '0' && *i <= '9') digits++;

        continue;
      }
      needs_quotation = true;
      break;
    }
  }

  if (needs_quotation || digits == ident.length())
    return quote_identifier(ident);
  else
    return ident;
}

const sqlstring sqlstring::null(sqlstring("NULL", 0));

sqlstring::sqlstring(const char *format_string, const sqlstringformat format)
    : _format_string_left(format_string), _format(format) {
  append(consume_until_next_escape());
}

sqlstring::sqlstring(const std::string &format_string,
                     const sqlstringformat format)
    : _format_string_left(format_string), _format(format) {
  append(consume_until_next_escape());
}

sqlstring::sqlstring(const sqlstring &copy)
    : _formatted(copy._formatted),
      _format_string_left(copy._format_string_left),
      _format(copy._format) {}

sqlstring::sqlstring() : _format(0) {}

std::string sqlstring::consume_until_next_escape() {
  mysqlshdk::utils::SQL_iterator it(_format_string_left);
  for (; it.valid(); ++it)
    if (*it == '?' || *it == '!') break;

  if (it.position() > 0) {
    std::string s = _format_string_left.substr(0, it.position());
    if (it.position() < _format_string_left.length())
      _format_string_left = _format_string_left.substr(it.position());
    else
      _format_string_left.clear();
    return s;
  }
  return "";
}

int sqlstring::next_escape() {
  if (_format_string_left.empty())
    throw std::invalid_argument(
        "Error formatting SQL query: more arguments than escapes");
  int c = _format_string_left[0];
  _format_string_left = _format_string_left.substr(1);
  return c;
}

sqlstring &sqlstring::append(const std::string &s) {
  _formatted.append(s);
  return *this;
}

sqlstring::operator std::string() const {
  return _formatted + _format_string_left;
}

std::string sqlstring::str() const {
  done();
  return _formatted + _format_string_left;
}

std::size_t sqlstring::size() const {
  return _formatted.size() + _format_string_left.size();
}

void sqlstring::done() const {
  if (!_format_string_left.empty() ||
      !(_format_string_left[0] != '!' && _format_string_left[0] != '?')) {
    throw std::logic_error("Unbound placeholders left in query");
  }
}

sqlstring &sqlstring::operator<<(const double v) {
  int esc = next_escape();
  if (esc != '?')
    throw std::invalid_argument(
        "Error formatting SQL query: invalid escape for numeric argument");

  append(shcore::dtoa(v));
  append(consume_until_next_escape());

  return *this;
}

sqlstring &sqlstring::operator<<(const sqlstringformat format) {
  _format = format;
  return *this;
}

sqlstring &sqlstring::operator<<(const std::string &v) {
  int esc = next_escape();
  if (esc == '!') {
    if ((_format._flags & QuoteOnlyIfNeeded) != 0)
      append(quote_identifier_if_needed(v));
    else
      append(quote_identifier(v));
  } else if (esc == '?') {
    append("'").append(escape_sql_string(v)).append("'");
  } else {  // shouldn't happen
    throw std::invalid_argument(
        "Error formatting SQL query: internal error, expected ? or ! escape "
        "got something else");
  }
  append(consume_until_next_escape());

  return *this;
}

sqlstring &sqlstring::operator<<(const sqlstring &v) {
  next_escape();

  append(v);
  append(consume_until_next_escape());

  return *this;
}

sqlstring &sqlstring::operator<<(const char *v) {
  int esc = next_escape();

  if (esc == '!') {
    if (!v)
      throw std::invalid_argument(
          "Error formatting SQL query: NULL value found for identifier");
    if (_format._flags & QuoteOnlyIfNeeded)
      append(quote_identifier_if_needed(v));
    else
      append(quote_identifier(v));
  } else if (esc == '?') {
    if (v) {
      append(quote_sql_string(v));
    } else {
      append("NULL");
    }
  } else {  // shouldn't happen
    throw std::invalid_argument(
        "Error formatting SQL query: internal error, expected ? or ! escape "
        "got something else");
  }
  append(consume_until_next_escape());

  return *this;
}
}  // namespace shcore
