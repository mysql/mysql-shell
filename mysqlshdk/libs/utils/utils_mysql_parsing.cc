/*
 * Copyright (c) 2015, 2023, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/utils/utils_mysql_parsing.h"
#include <algorithm>
#include <iostream>
#include <iterator>
#include <sstream>
#include <tuple>
#include <utility>
#include "mysqlshdk/libs/utils/utils_lexing.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlshdk {
namespace utils {

namespace {

template <const char skip_tables[2][256], const char quote>
inline char *span_string(char *p, const char *end, bool no_backslash_escapes) {
  const char *target_table = skip_tables[no_backslash_escapes ? 1 : 0];

  // p must be inside the single quote string (after the ')
  int last_ch = 0;
  for (;;) {
    // Tight-loop to span until end or closing quote
    while (p < end &&
           target_table[last_ch = static_cast<unsigned char>(*p)] > 0) {
      p += target_table[last_ch];
    }

    // Now check why we exited the loop
    if (last_ch == '\0' || p >= end) {
      // there was a '\0', but the string is not over, continue spanning
      if (p < end) {
        ++p;
        continue;
      }
      // string is over and we didn't see a ', so it's an unterminated string
      // if the last char we saw was an escape we need to remember escaping
      // the first char of the next block
      return nullptr;
    }
    // the only other possible way to exit the loop must be the end quote
    assert(last_ch == quote);
    // continue if there's another quote following the quote
    if (*(p + 1) == quote) {
      p += 2;
      continue;
    }
    return p + 1;
  }
}

template <const char quote>
inline char *span_quoted_identifier(char *p, char *end) {
  // unlike strings, identifiers don't allow escaping with the \ char
  while (char *q = static_cast<char *>(memchr(p, quote, end - p))) {
    if (*(q + 1) == quote) {
      p = q + 2;
    } else {
      return q + 1;
    }
  }
  return nullptr;
}

inline char *span_dollar_quote(char *p, char *end) {
  // skips identifier chars, except for $
  while (p < end) {
    if (!internal::k_dollar_quoted_string_span_chars[static_cast<unsigned char>(
            *p)]) {
      return p;
    }
    ++p;
  }
  return nullptr;
}

inline char *span_dollar_quoted_string(char *p, char *end, const char *tag) {
  size_t q = std::string_view(p, end - p).find(tag);
  if (q != std::string_view::npos) {
    return p + q + strlen(tag);
  }
  return nullptr;
}

inline char *span_comment(char *p, char *end) {
  while ((p + 1 < end) && (p = static_cast<char *>(memchr(p, '*', end - p))) &&
         p[1] != '/') {
    p++;
  }
  return (p && (p + 1 < end) && p[1] == '/') ? p : nullptr;
}

inline char *skip_blanks(char *p, const char *end) {
  while (p < end && (*p == ' ' || *p == '\t' || *p == '\r')) ++p;
  return p;
}

inline bool is_any_blank(const char c) {
  return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

inline char *skip_not_blanks(char *p, const char *end) {
  while (p < end && *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n') ++p;
  return p;
}

constexpr std::string_view k_delimiter{"delimiter"};

}  // namespace

Sql_splitter::Sql_splitter(Command_callback cmd_callback,
                           Error_callback err_callback,
                           std::initializer_list<std::string_view> commands)
    : m_cmd_callback(std::move(cmd_callback)),
      m_err_callback(std::move(err_callback)) {
  for (auto cmd : commands) {
    size_t off = tolower(cmd[0]) - 'a';
    assert(off < m_commands_table.size());
    m_commands_table[off].emplace_back(cmd);
  }

  for (auto &cmd : m_commands_table) std::reverse(cmd.begin(), cmd.end());

  reset();
}

void Sql_splitter::reset() {
  m_begin = nullptr;
  m_end = nullptr;
  m_ptr = nullptr;

  m_delimiter = ";";
  m_context.clear();

  m_shrinked_bytes = 0;
  m_current_line = 1;
  m_total_offset = 0;
  m_ansi_quotes = false;
  m_no_backslash_escapes = false;
  m_last_chunk = false;
  m_eof = false;
}

/** Pack input buffer, moving the last unfinished statement to its beginning
 */
void Sql_splitter::pack_buffer(std::string *buffer, Range last_range) {
  assert(buffer);
  if (last_range.offset > 0) {
    memmove(&(*buffer)[0], &(*buffer)[last_range.offset],
            buffer->size() - last_range.offset - m_shrinked_bytes);
    buffer->resize(buffer->size() - last_range.offset - m_shrinked_bytes);
  } else if (m_shrinked_bytes > 0) {
    buffer->resize(buffer->size() - m_shrinked_bytes);
  }
}

void Sql_splitter::feed(char *buffer, size_t size) {
  feed_chunk(buffer, size);
  m_last_chunk = true;
}

void Sql_splitter::feed_chunk(char *buffer, size_t size) {
  // printf("feed --> [[%s]] %zi\n", std::string(buffer, size).c_str(), size);
  if (m_begin) m_total_offset += (m_end - m_begin);

  m_shrinked_bytes = 0;

  m_begin = buffer;
  m_end = buffer + size;

  m_ptr = m_begin;

  m_context.clear();
}

bool Sql_splitter::set_delimiter(std::string delim) {
  if (delim.empty()) {
    m_err_callback(
        "DELIMITER must be followed by a 'delimiter' character or string");
    return false;
  }
  if (!m_no_backslash_escapes && delim.find('\\') != std::string::npos) {
    m_err_callback("DELIMITER cannot contain a backslash character");
    return false;
  }
  m_delimiter = std::move(delim);
  return true;
}

/** Get range of next statement in buffer.
 *
 * @param[out] out_range, the range of the statement
 * @returns true if a full statement was parsed, false otherwise
 *
 * This function is implemented a little unconventionally to make it possible
 * to have similar behaviour as the classic cli.
 *
 * The trickiest is that escape commands (\x) can appear anywhere in the input,
 * including within a SQL statement. These commands are executed as they are
 * seen in the input. That means:
 *   * SQL statements are only executed when the delimiter is seen.
 *      e.g, if input is:
 *          SELECT \w 1;
 *      then \w is executed first, and then SELECT 1;
 *   * \commands that appear inside a SQL statement are stripped away.
 *
 * Note escape commands in the classic cli are all single letter, while
 * mysqlsh allows longer ones. Single letter commands can appear anywhere
 * as in the classic cli, but long commands are only allowed if they appear
 * alone in a line (except inside comments or strings).
 */
bool Sql_splitter::next_range(Sql_splitter::Range *out_range,
                              std::string *out_delim) {
  auto unfinished_stmt = [this](char *bos, Range *result_range) {
    Range range{static_cast<size_t>(bos - m_begin),
                static_cast<size_t>(m_end - bos), m_current_line};
    m_ptr = bos;
    *result_range = range;
    return false;
  };

  char *bos;  // beginning of statement

  bos = m_ptr;
  for (;;) {
    bos = skip_blanks(bos, m_end);
    if (*bos == '\n') {
      m_current_line++;
      ++bos;
    } else {
      break;
    }
  }

  size_t line_count = 0;
  char *next_bol = bos;  // beginning of next line
  while (next_bol < m_end) {
    char *bol = next_bol;
    bool has_complete_line;
    bool has_newline;
    bool command = false;
    bool last_line_was_delimiter = false;
    // process input per line so we can count line numbers
    char *eol = static_cast<char *>(memchr(bol, '\n', m_end - bol));
    if (!eol) {
      eol = m_end;
      has_complete_line = m_last_chunk;
      has_newline = false;
    } else {
      ++eol;  // skip the newline
      has_complete_line = true;
      has_newline = true;
    }
    next_bol = eol;

    // find the end of the statement
    char *p = bol;

    const auto run_command = [&](bool delimited) {
      command = false;
      auto [skip, d] =
          m_cmd_callback(std::string_view{bos, static_cast<size_t>(p - bos)},
                         true, m_current_line);
      if (skip == 0) return false;
      if (delimited) skip += m_delimiter.size();
      p = bos + skip;
      memmove(bos, p, m_end - p + 1);
      m_shrinked_bytes += skip;
      p = bos;
      eol -= skip;
      next_bol -= skip;
      m_end -= skip;
      pop();
      return true;
    };

    while (p && p < eol) {
      auto ctx = context();
      if (ctx == Context::kNone || ctx == Context::kStatement ||
          ctx == Context::kIdentifier || ctx == Context::kCommentConditional) {
        if (ctx == Context::kCommentConditional) {
          if ((eol - p) > 2 && *p == '*' && *(p + 1) == '/') {
            pop();
            p += 2;
            continue;
          }
        } else {
          // check for the current delimiter
          if (*p == m_delimiter[0] &&
              (p + m_delimiter.size() <= eol &&
               m_delimiter.compare(0, m_delimiter.size(), p, 0,
                                   m_delimiter.size()) == 0)) {
            if (command && run_command(true)) continue;
            pop();
            Range range{static_cast<size_t>(bos - m_begin),
                        static_cast<size_t>(p - bos), m_current_line};
            m_ptr = p + m_delimiter.size();
            *out_range = range;
            *out_delim = m_delimiter;
            m_current_line += line_count;
            return true;
          }
        }

        switch (*p) {
          case '\\':  // \x
          {
            if (m_no_backslash_escapes) goto other;

            // commands within commands are not supported
            if (command) goto other;

            if (eol - p == 1) return unfinished_stmt(bos, out_range);

            // Callback returns number of chars to skip and whether the
            // statement should be terminated.
            auto [skip, delim] = m_cmd_callback(
                std::string_view{p, static_cast<size_t>(eol - p)}, p == bos,
                m_current_line);
            if (skip == 0 && (p == bos && !has_complete_line)) {
              // if there's a \command at the beginning of the line and
              // skip is 0, then it means we need the full line because it's a
              // standalone shell \command
              return unfinished_stmt(bos, out_range);
            }
            if (delim) {
              // if this cmd acts as a delimiter (like \g or \G, return the
              // statement)
              *out_delim = std::string(p, skip);
              m_ptr = p + skip;
              *out_range = Range{static_cast<size_t>(bos - m_begin),
                                 static_cast<size_t>(p - bos), m_current_line};
              pop();
              m_current_line += line_count;
              return true;
            } else {
              // pack together the rest of the buffer removing the command
              memmove(p, p + skip, (m_end - p) - skip);
              m_shrinked_bytes += skip;
              eol -= skip;
              m_end -= skip;
            }
            break;
          }

          case '\'':  // 'str'
            ++p;      // skip the opening '
            push(Context::kSQuoteString);
            break;

          case '"':  // "str"
            ++p;     // skip the opening "
            if (m_ansi_quotes) {
              push(Context::kDQuoteIdentifier);
            } else {
              push(Context::kDQuoteString);
            }
            break;

          case '`':  // `ident`
            ++p;
            push(Context::kBQuoteIdentifier);
            break;

          case '$':  // $tag$str$tag$ or $identifier
            if (m_dollar_quoted_strings) {
              // there are 3 possibilities here:
              // - $<identifier>$<not-identifier-or-eof> - $ string
              // - $<identifier><not-identifier-or-eof> - $identifier
              // - $<identifier>... - incomplete/inconclusive

              char *pp = span_dollar_quote(p + 1, eol);
              if (pp) {
                if (*pp == '$') {
                  m_dollar_quote.assign(p, pp - p + 1);
                  p = pp + 1;
                  push(Context::kDollarQuotedString);
                } else {
                  // was an identifier, which we already skipped
                  p = pp;
                }
              } else {
                return unfinished_stmt(bos, out_range);
              }
            } else {
              // definitely not a dollar quoted str
              ++p;
              push(Context::kIdentifier);
            }
            break;

          case '/':  // /* ... */
            if ((m_end - p) >= 3) {
              if (*(p + 1) == '*') {
                if (*(p + 2) == '+') {
                  push(Context::kCommentHint);
                  p += 3;
                } else if (*(p + 2) == '!') {
                  push(Context::kCommentConditional);
                  p += 3;
                } else {
                  push(Context::kComment);
                  p += 2;
                }
                break;
              }
            } else {
              if (!has_complete_line) return unfinished_stmt(bos, out_range);
            }
            goto other;

          case 'd':  // delimiter
          case 'D':
            // Possible start of the keyword DELIMITER. Must be the 1st keyword
            // of a statement.
            if (context() == Context::kNone &&
                shcore::str_ibeginswith(
                    {p, static_cast<std::size_t>(m_end - p)}, k_delimiter)) {
              if (has_complete_line) {
                // handle delimiter change directly here
                auto np = skip_blanks(p + k_delimiter.size(), eol);
                if (np == eol || np == p + k_delimiter.size()) {
                  set_delimiter("");
                } else {
                  p = np;
                  char *end = skip_not_blanks(p, eol);
                  set_delimiter(std::string(p, end - p));
                }
                bos = p = next_bol;
                pop();
                last_line_was_delimiter = true;
                // delimiter is like a full statement, so increment line
                // and start next stmt from scratch
                m_current_line++;
                break;
              } else {
                // newline is missing
                return unfinished_stmt(bos, out_range);
              }
            }
            goto other;

          case '#':  // # ...
            if (has_complete_line) {
              // if the whole line is a comment return it
              if (context() == Context::kNone) {
                int nl = has_newline ? 1 : 0;
                if (*(eol - 1) == '\r') nl++;
                Range range{static_cast<size_t>(bos - m_begin),
                            static_cast<size_t>(eol - bos - nl),
                            m_current_line};
                m_ptr = next_bol;
                *out_range = range;
                *out_delim = "";
                line_count++;
                m_current_line += line_count;
                return true;
              }
              p = next_bol;
            } else {
              return unfinished_stmt(bos, out_range);
            }
            break;

          case '-':  // -- ...
            if ((m_end - p) > 2 && *(p + 1) == '-' && is_any_blank(*(p + 2))) {
              if (has_complete_line) {
                // if the whole line is a comment return it
                if (context() == Context::kNone) {
                  int nl = has_newline;
                  if (*(eol - 1) == '\r') nl++;
                  Range range{static_cast<size_t>(bos - m_begin),
                              static_cast<size_t>(eol - bos - nl),
                              m_current_line};
                  m_ptr = next_bol;
                  *out_range = range;
                  *out_delim = "";
                  line_count++;
                  m_current_line += line_count;
                  return true;
                }
                p = next_bol;
              } else {
                return unfinished_stmt(bos, out_range);
              }
            } else {
              goto other;
            }
            break;

          default:
          other:
            if (context() == Context::kNone) {
              size_t off = tolower(*p) - 'a';
              if (off < m_commands_table.size() &&
                  !m_commands_table[off].empty()) {
                for (const auto &kwd : m_commands_table[off])
                  if (shcore::str_ibeginswith(
                          {p, static_cast<std::size_t>(m_end - p)}, kwd) &&
                      is_any_blank(*(p + kwd.length()))) {
                    command = true;
                    p += kwd.length() + 1;
                    push(Context::kStatement);
                    break;
                  }
                if (command) continue;
              }
            } else {
              if (context() == Context::kDollarQuotedString) {
                m_dollar_quote.push_back(*p);
                ++p;
                break;
              }
            }

            if (!is_any_blank(*p)) {
              if (context() == Context::kNone) push(Context::kStatement);
            } else {
              // end of identifier
              if (context() == Context::kIdentifier) pop();

              if (p == bos) {
                bos++;
              }
            }
            ++p;
            break;
        }
      }

      switch (context()) {
        case Context::kNone:
        case Context::kStatement:
        case Context::kIdentifier:
        case Context::kCommentConditional:
          break;

        case Context::kSQuoteString:
          p = span_string<internal::k_quoted_string_span_skips_sq, '\''>(
              p, eol, m_no_backslash_escapes);
          if (!p) {  // closing quote missing
            if (has_complete_line) {
              p = eol;
            } else {
              return unfinished_stmt(bos, out_range);
            }
          } else {
            pop();
          }
          break;

        case Context::kDQuoteString:
          p = span_string<internal::k_quoted_string_span_skips_dq, '"'>(
              p, eol, m_no_backslash_escapes);
          if (!p) {  // closing quote missing
            if (has_complete_line) {
              p = eol;
            } else {
              return unfinished_stmt(bos, out_range);
            }
          } else {
            pop();
          }
          break;

        case Context::kComment:
        case Context::kCommentHint:
          p = span_comment(p, eol);
          if (!p) {  // comment end missing
            if (has_complete_line) {
              p = eol;
            } else {
              return unfinished_stmt(bos, out_range);
            }
          } else {
            p += 2;
            pop();
          }
          break;

        case Context::kDollarQuotedString:
          p = span_dollar_quoted_string(p, eol, m_dollar_quote.c_str());
          if (!p) {  // comment end missing
            if (has_complete_line) {
              p = eol;
            } else {
              return unfinished_stmt(bos, out_range);
            }
          } else {
            pop();
          }
          break;

        case Context::kBQuoteIdentifier:
          p = span_quoted_identifier<'`'>(p, eol);
          if (!p) {  // closing quote missing
            if (has_complete_line) {
              p = eol;
            } else {
              return unfinished_stmt(bos, out_range);
            }
          } else {
            pop();
          }
          break;

        case Context::kDQuoteIdentifier:
          p = span_quoted_identifier<'"'>(p, eol);
          if (!p) {  // closing quote missing
            if (has_complete_line) {
              p = eol;
            } else {
              return unfinished_stmt(bos, out_range);
            }
          } else {
            pop();
          }
          break;
      }
    }

    if (p == eol) {
      if (command)
        run_command(false);
      else if (!last_line_was_delimiter)
        line_count++;
    }
  }
  if (m_last_chunk && bos < m_end) {
    Range range{static_cast<size_t>(bos - m_begin),
                static_cast<size_t>(m_end - bos), m_current_line};
    m_ptr = m_end;
    *out_range = range;
    *out_delim = "";
    m_current_line += line_count;
    return true;
  }
  if (m_last_chunk) m_eof = true;
  *out_range = Range{static_cast<size_t>(bos - m_begin),
                     static_cast<size_t>(m_end - bos), m_current_line};
  return false;
}

/** Return list of all statements read from a stream object.
 *
 * @param stream stream object to read from
 * @param chunk_size number of bytes to read at a time
 * @param err_callback function to be called when a parse error occurs
 * @param ansi_quotes if true, " quotes are handled as identifiers instead of
 *        strings
 * @param dollar_quoted_strings if true, $$tag$$ style string quotes are
 *        recognized
 * @param delimiter statement delimiter. If the script changes the delimiter,
 *        it will be assigned to this argument.
 * @returns list of (statement, delimiter, line_number) tuples
 */
std::vector<std::tuple<std::string, std::string, size_t>> split_sql_stream(
    std::istream *stream, size_t chunk_size,
    const Sql_splitter::Error_callback &err_callback, bool ansi_quotes,
    bool no_backslash_escapes, bool dollar_quoted_strings,
    std::string *delimiter) {
  std::vector<std::tuple<std::string, std::string, size_t>> results;
  iterate_sql_stream(
      stream, chunk_size,
      [&results](std::string_view s, std::string_view delim, size_t lnum,
                 size_t) {
        results.emplace_back(std::string(s), std::string(delim), lnum);
        return true;
      },
      err_callback, ansi_quotes, no_backslash_escapes, dollar_quoted_strings,
      delimiter);
  return results;
}

std::vector<std::string> split_sql(const std::string &str, bool ansi_quotes,
                                   bool no_backslash_escapes,
                                   bool dollar_quoted_strings) {
  std::istringstream s(str);
  auto parts = split_sql_stream(
      &s, str.size(),
      [](std::string_view err) {
        throw std::runtime_error(
            shcore::str_format("Error splitting SQL script: %.*s",
                               static_cast<int>(err.size()), err.data()));
      },
      ansi_quotes, no_backslash_escapes, dollar_quoted_strings);

  std::vector<std::string> stmts;
  stmts.reserve(parts.size());
  for (const auto &p : parts) {
    stmts.emplace_back(std::get<0>(p));
  }
  return stmts;
}

/** Apply a callback on each statement read from a stream object.
 *
 * @param stream stream object to read from
 * @param chunk_size number of bytes to read at a time
 * @param callback function to call on each statement
 * @param err_callback function to be called when a parse error occurs
 * @param ansi_quotes if true, " quotes are handled as identifiers instead of
 *        strings
 * @param dollar_quoted_strings if true, $$tag$$ strings are recognized
 * @param delimiter statement delimiter. If the script changes the delimiter,
 *        it will be assigned to this argument.
 * @param splitter_ptr if provided function will assign here Sql_splitter used
 *        for stream processing.
 * @returns list of (statement, delimiter, line_number) tuples
 */
bool iterate_sql_stream(
    std::istream *stream, size_t chunk_size,
    const std::function<
        bool(std::string_view /* string */, std::string_view /* delimiter */,
             size_t /* line_num */, size_t /* offset */)> &callback,
    const Sql_splitter::Error_callback &err_callback, bool ansi_quotes,
    bool no_backslash_escapes, bool dollar_quoted_strings,
    std::string *delimiter, Sql_splitter **splitter_ptr) {
  assert(chunk_size > 0);

  bool stop = false;
  std::string buffer;

  Sql_splitter splitter(
      [&callback, &stop, &buffer](std::string_view s, bool bol,
                                  size_t lnum) -> std::pair<size_t, bool> {
        assert((s.data() >= buffer.data()) &&
               (s.data() < (buffer.data() + buffer.size())));

        auto sdata = s.data();
        if (!bol) s = s.substr(0, 2);
        assert(s.size() >= 2);

        if (s[1] != 'g' && s[1] != 'G') {
          if (!callback(s, {}, lnum, sdata - &buffer[0])) stop = true;
          return std::make_pair(s.size(), false);
        }
        return std::make_pair(2U, true);
      },
      err_callback, {"source"});
  splitter.set_ansi_quotes(ansi_quotes);
  splitter.set_no_backslash_escapes(no_backslash_escapes);
  splitter.set_dollar_quoted_strings(dollar_quoted_strings);

  if (delimiter) splitter.set_delimiter(*delimiter);
  if (splitter_ptr) *splitter_ptr = &splitter;

  buffer.resize(chunk_size);

  stream->read(&buffer[0], chunk_size);
  buffer.resize(stream->gcount());
  splitter.feed_chunk(&buffer[0], buffer.size());
  size_t old_chunk_size = splitter.chunk_size();
  size_t offset = 0;

  while (!splitter.eof() && !stop) {
    Sql_splitter::Range range;
    std::string delim;

    if (splitter.next_range(&range, &delim)) {
      if (!callback({&buffer[range.offset], range.length}, delim,
                    range.line_num, offset + range.offset))
        stop = true;
    } else {
      // flush if this is the last chunk, even if statement is unfinished
      if (splitter.is_last_chunk() && range.length > 0) {
        if (!callback({&buffer[range.offset], range.length}, delim,
                      range.line_num, offset + range.offset))
          stop = true;
        break;
      }

      size_t shrinkage = old_chunk_size - splitter.chunk_size();
      if (range.offset > 0) {
        offset += range.offset;
        memmove(&buffer[0], &buffer[range.offset],
                buffer.size() - range.offset - shrinkage);
        buffer.resize(buffer.size() - range.offset - shrinkage);
      } else if (shrinkage > 0) {
        buffer.resize(buffer.size() - shrinkage);
      }

      size_t osize = buffer.size();
      buffer.resize(osize + chunk_size);
      stream->read(&buffer[osize], chunk_size);
      buffer.resize(osize + stream->gcount());

      if (static_cast<size_t>(stream->gcount()) < chunk_size) {
        splitter.feed(&buffer[0], buffer.size());
      } else {
        splitter.feed_chunk(&buffer[0], buffer.size());
      }
      old_chunk_size = splitter.chunk_size();
    }
  }

  if (delimiter) *delimiter = splitter.delimiter();

  return !stop;
}

std::string to_string(Sql_splitter::Context context) {
  switch (context) {
    case Sql_splitter::Context::kNone:
      return "";
    case Sql_splitter::Context::kStatement:
    case Sql_splitter::Context::kIdentifier:
    case Sql_splitter::Context::kCommentHint:
    case Sql_splitter::Context::kCommentConditional:
      return "-";
    case Sql_splitter::Context::kComment:
      return "/*";
    case Sql_splitter::Context::kSQuoteString:
      return "'";
    case Sql_splitter::Context::kDQuoteString:
      return "\"";
    case Sql_splitter::Context::kDollarQuotedString:
      return "$";
    case Sql_splitter::Context::kBQuoteIdentifier:
      return "`";
    case Sql_splitter::Context::kDQuoteIdentifier:
      return "\"";
  }
  return "";
}

}  // namespace utils
}  // namespace mysqlshdk
