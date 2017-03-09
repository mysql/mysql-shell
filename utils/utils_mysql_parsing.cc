/*
 * Copyright (c) 2015, 2016, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */
//--------------------------------------------------------------------------------------------------
#include "utils_mysql_parsing.h"
#include <boost/algorithm/string/trim.hpp>

namespace shcore {
namespace mysql {
namespace splitter {

Delimiters::Delimiters(const std::initializer_list<delim_type_t> &delimiters) :
    main_delimiter(*std::begin(delimiters)) {
  if (delimiters.size() > 1) {
    additional_delimiters.reserve(delimiters.size() - 1);
    std::copy(std::begin(delimiters) + 1, std::end(delimiters),
        std::back_inserter(additional_delimiters));
  }
}

std::size_t Delimiters::size() const {
  return additional_delimiters.size() + 1;
}

void Delimiters::set_main_delimiter(delim_type_t delimiter) {
  main_delimiter = std::move(delimiter);
}

const Delimiters::delim_type_t& Delimiters::get_main_delimiter() const {
  return main_delimiter;
}

Delimiters::delim_type_t& Delimiters::operator[](std::size_t pos) {
  assert(pos < size());
  if (pos == 0)
    return main_delimiter;
  else
    return additional_delimiters[pos - 1];
}

const Delimiters::delim_type_t& Delimiters::operator[](std::size_t pos) const {
  return this->operator[](pos);
}

Statement_range::Statement_range(std::size_t begin, std::size_t end,
    Delimiters::delim_type_t delimiter) : m_begin(begin), m_end(end),
    m_delimiter(std::move(delimiter)){

    }

std::size_t Statement_range::offset() const {
  return m_begin;
}

std::size_t Statement_range::length() const {
  return m_end;
}

const Delimiters::delim_type_t& Statement_range::get_delimiter() const {
  return m_delimiter;
}
//--------------------------------------------------------------------------------------------------

const unsigned char* skip_leading_whitespace(const unsigned char *head, const unsigned char *tail) {
  while (head < tail && *head <= ' ')
    head++;
  return head;
}

//--------------------------------------------------------------------------------------------------

bool is_line_break(const unsigned char *head, const unsigned char *line_break) {
  if (*line_break == '\0')
    return false;

  while (*head != '\0' && *line_break != '\0' && *head == *line_break) {
    head++;
    line_break++;
  }
  return *line_break == '\0';
}

//--------------------------------------------------------------------------------------------------

/**
 * A statement splitter to take a list of sql statements and split them into individual statements,
 * return their position and length in the original string (instead the copied strings).
 * Complete statements contain additional delimiter attached, if no delimiter is found
 * command should be treated as incomplete.
 */
std::vector<Statement_range> determineStatementRanges(
    const char *sql, size_t length, Delimiters &delimiters,
    const std::string &line_break, std::stack<std::string> &input_context_stack) {

  const unsigned char keyword[] = "delimiter";

  const unsigned char *head = (unsigned char *)sql;
  const unsigned char *tail = head;
  const unsigned char *end = head + length;
  const unsigned char *new_line = (unsigned char*)line_break.c_str();
  bool have_content = false; // Set when anything else but comments were found for the current statement.

  std::vector<Statement_range> ranges;

  while (tail < end) {
    switch (*tail) {
      case '*': // Comes from a multiline comment and comment is done
        if (*(tail + 1) == '/' && (!input_context_stack.empty() && input_context_stack.top() == "/*")) {
          if (!input_context_stack.empty())
            input_context_stack.pop();

          tail += 2;
          head = tail; // Skip over the comment.
        }
        break;
      case '/': // Possible multi line comment or hidden (conditional) command.
        if (*(tail + 1) == '*') {
          tail += 2;
          bool is_hidden_command = (*tail == '!');
          while (true) {
            while (tail < end && *tail != '*')
              tail++;
            if (tail == end) // Unfinished comment.
            {
              input_context_stack.push("/*");
              break;
            } else {
              if (*(tail + 1) == '/') {
                tail++;
                break;
              }
            }
          }

          if (!is_hidden_command && !have_content)
            head = tail + 1; // Skip over the comment.

          break;
        }

      case '-': // Possible single line comment.
      {
        const unsigned char *end_char = tail + 2;
        if (*(tail + 1) == '-' && (*end_char == ' ' || *end_char == '\t' || is_line_break(end_char, new_line) || length == 2)) {
          // Skip everything until the end of the line.
          tail += 2;
          while (tail < end && !is_line_break(tail, new_line))
            tail++;
          if (!have_content)
            head = tail;
        }
        break;
      }

      case '#': // MySQL single line comment.
        while (tail < end && !is_line_break(tail, new_line))
          tail++;
        if (!have_content)
          head = tail;
        break;

      case '"':
      case '\'':
      case '`':
      {
        have_content = true;
        char quote = *tail++;

        if (input_context_stack.empty() || input_context_stack.top() == "-") {
          // Quoted string/id. Skip this in a local loop if is opening quote.
          while (tail < end ) {
            // Handle consecutive double quotes within a quoted string (for ' and ")
            // Consecutive double quotes for identifiers should not be handled, i. e., in case of `
            // See http://dev.mysql.com/doc/refman/5.7/en/string-literals.html#character-escape-sequences
            if(*tail == quote) {
              if((tail + 1) < end && *(tail + 1) == quote && quote != '`')
                tail++;
              else
                break;
            }
            // Skip any escaped character within a quoted string (for ' and ")
            // Escaped characters for identifiers should not be handled, i. e., in case of `
            // See http://dev.mysql.com/doc/refman/5.7/en/string-literals.html#character-escape-sequences
            if (*tail == '\\' && quote != '`')
              tail++;
            tail++;
          }
          if (*tail == quote)
            tail++; // Skip trailing quote char to if one was there.
          else {
            std::string q;
            q.assign(&quote, 1);
            input_context_stack.push(q); // Sets multiline opening quote to continue processing
          }
        } else // Closing quote, clears the multiline flag
          input_context_stack.pop();

        break;
      }

      case 'd':
      case 'D':
      {
        have_content = true;

        // Possible start of the keyword DELIMITER. Must be at the start of the text or a character,
        // which is not part of a regular MySQL identifier (0-9, A-Z, a-z, _, $, \u0080-\uffff).
        unsigned char previous = tail > (unsigned char *)sql ? *(tail - 1) : 0;
        bool is_identifier_char = previous >= 0x80
        || (previous >= '0' && previous <= '9')
        || ((previous | 0x20) >= 'a' && (previous | 0x20) <= 'z')
        || previous == '$'
        || previous == '_';
        if (tail == (unsigned char *)sql || !is_identifier_char) {
          const unsigned char *run = tail + 1;
          const unsigned char *kw = keyword + 1;
          int count = 9;
          while (count-- > 1 && (*run++ | 0x20) == *kw++)
            ;
          if (count == 0 && *run == ' ') {
            // Delimiter keyword found. Get the new delimiter (everything until the end of the line).
            tail = run++;
            while (run < end && !is_line_break(run, new_line))
              run++;

            std::string delimiter = std::string((char *)tail, run - tail);
            boost::trim(delimiter);
            delimiters.set_main_delimiter(delimiter);

            // Skip over the delimiter statement and any following line breaks.
            while (is_line_break(run, new_line))
              run++;
            tail = run;
            head = tail;
          }
        }
        break;
      }
    }

    for(int i = 0; i < delimiters.size(); i++) {
      auto delimiter = delimiters[i];
      if (*tail == delimiter[0]) {
        // Found possible start of the delimiter. Check if it really is.
        size_t count = delimiter.size();
        if (count == 1) {
          // Most common case. Trim the statement and check if it is not empty before adding the range.
          head = skip_leading_whitespace(head, tail);
          if (head < tail || (!input_context_stack.empty() && input_context_stack.top() == "-")) {

            if (!input_context_stack.empty())
              input_context_stack.pop();

            if (head < tail)
              ranges.emplace_back(head - (unsigned char *)sql, tail - head, delimiter);
            else
              ranges.emplace_back(0, 0, delimiter); // Empty line with just a delimiter
          }
          head = ++tail;
          have_content = false;
        } else {
          const unsigned char *run = tail + 1;
          const char *del = delimiter.c_str() + 1;
          while (count-- > 1 && (*run++ == *del++))
            ;

          if (count == 0) {
            // Multi char delimiter is complete. Tail still points to the start of the delimiter.
            // Run points to the first character after the delimiter.
            head = skip_leading_whitespace(head, tail);
            if (head < tail || (!input_context_stack.empty() && input_context_stack.top() == "-")) {

              if (!input_context_stack.empty())
                input_context_stack.pop();

              if (head < tail)
                ranges.emplace_back(head - (unsigned char *)sql, tail - head, delimiter);
              else
                ranges.emplace_back(0, 0, delimiter); // Empty line with just a delimiter
            }

            tail = run;
            head = run;
            have_content = false;
          }
        }
      }
    }

    // Multiline comments are ignored, everything else is not
    if (*tail > ' ' && (input_context_stack.empty() || input_context_stack.top() != "/*"))
      have_content = true;
    tail++;
  }

  // Add remaining text to the range list but ignores it when it is a multiline comment
  head = skip_leading_whitespace(head, tail);
  if (head < tail && (input_context_stack.empty() || input_context_stack.top() != "/*")) {

    // There is no delimiter yet
    ranges.emplace_back(head - (unsigned char *)sql, tail - head, "");

    // If not a multiline string then sets the flag to multiline statement (not terminated)
    if (input_context_stack.empty())
      input_context_stack.push("-");
  }

  return ranges;
}
}
}
}
