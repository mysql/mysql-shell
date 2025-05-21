/*
 * Copyright (c) 2023, 2025, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/utils/string_list_matcher.h"

#include <cctype>
#include <optional>

#include "mysqlshdk/libs/utils/utils_lexing.h"

namespace mysqlshdk {

namespace {
size_t count_spaces(const char *data, size_t size, size_t offset = 0) {
  if (size_t count = utils::span_spaces({data, size}, offset);
      count != std::string::npos) {
    return count;
  }
  return size;
}

size_t span_comment(const char *data, const char *end, size_t offset) {
  size_t position = offset;
  std::optional<size_t> comment_end;

  if (*(data + offset) == '/' && (data + offset + 1) < end &&
      *(data + offset + 1) == '*') {
    comment_end =
        mysqlshdk::utils::span_cstyle_comment(std::string_view(data), offset);
  } else if (*(data + offset) == '-' && (data + offset + 2) < end &&
             *(data + offset + 1) == '-' &&
             std::isspace(*(data + offset + 2))) {
    comment_end = mysqlshdk::utils::span_to_eol(std::string_view(data), offset);
  } else if (*(data + offset) == '#') {
    comment_end = mysqlshdk::utils::span_to_eol(std::string_view(data), offset);
  }

  if (comment_end.has_value()) {
    if (*comment_end == std::string_view::npos) {
      position = end - data;
    } else {
      position = *comment_end;
    }
  }

  return position;
}

std::pair<size_t, bool> span_spaces_and_comments(const char *data,
                                                 const char *end) {
  size_t next_position = 0;
  size_t final_position;
  bool space_found = false;
  do {
    final_position = next_position;

    // Spans space
    next_position = count_spaces(data, (end - data), next_position);
    if (next_position > final_position) {
      space_found = true;
    }

    // Spans comment
    next_position = span_comment(data, end, next_position);

  } while (next_position > final_position);

  return {final_position, space_found};
}

}  // namespace

Prefix_overlap_error::Prefix_overlap_error(const std::string &error,
                                           const std::string &tag)
    : std::runtime_error(error), m_tag{tag} {}

String_prefix_matcher::Node *String_prefix_matcher::Node::get(
    unsigned char value) const {
  assert(value < k_alphabet_size);
  return m_next[value] ? m_next[value].get() : nullptr;
}

void String_prefix_matcher::Node::set_final() {
  m_final = true;
  for (int index = 0; index < k_alphabet_size; index++) {
    if (m_next[index]) {
      m_next[index].reset();
    }
  }
}

String_prefix_matcher::Node *String_prefix_matcher::Node::add(
    unsigned char value, const std::string &tag) {
  assert(value < k_alphabet_size);

  if (!m_next[value]) {
    m_next[value] = std::make_unique<Node>(tag);
    m_final = false;
  }

  return m_next[value].get();
}

void String_prefix_matcher::Node::get_strings(
    const std::string &prefix, std::set<std::string> *list) const {
  if (is_final()) {
    list->insert(prefix);
  } else {
    for (int index = 0; index < k_alphabet_size; index++) {
      if (m_next[index]) {
        m_next[index]->get_strings(prefix + static_cast<char>(index), list);
      }
    }
  }
}

String_prefix_matcher::String_prefix_matcher(
    const std::vector<std::string> &str_list, const std::string &tag)
    : m_root(*m_tags.find("__root__")) {
  for (const auto &str : str_list) {
    add_string(str, tag);
  }
}

String_prefix_matcher::Node *String_prefix_matcher::get_max_match(
    const char **index, const char *end) const {
  auto pos_and_spaces = span_spaces_and_comments(*index, end);
  (*index) += pos_and_spaces.first;

  Node *last_match = nullptr;
  Node *next_node = const_cast<Node *>(&m_root);

  while (next_node && *index < end) {
    pos_and_spaces = span_spaces_and_comments(*index, end);
    (*index) += pos_and_spaces.first;

    if (pos_and_spaces.second) {
      next_node = next_node->get(' ');
    } else {
      // Rest of characters are added once
      next_node = next_node->get(std::toupper(**index));
      if (next_node) {
        (*index)++;
      }
    }

    if (next_node) last_match = next_node;
  }

  return last_match;
}

void String_prefix_matcher::add_string(const std::string &str,
                                       const std::string &tag) {
  const auto &this_tag = *m_tags.insert(tag).first;

  const char *index = str.data();
  const char *end = str.data() + str.size();

  Node *next_node = get_max_match(&index, end);

  if (next_node) {
    if (next_node->is_final()) {
      // If the last match is final it means a previous smaller prefix already
      // would match the new prefix.
      // If the tags of the existing a new prefix are different, this is an
      // overlap error, otherwise there's no need to overcomplicate the matcher,
      // we are done.
      if (next_node->tag() != tag) {
        throw Prefix_overlap_error(
            shcore::str_format(
                "The prefix '%s' is shadowed by an existing prefix",
                str.c_str()),
            next_node->tag());
      }
      return;
    } else if (index == end) {
      // If the last match is not final and the new prefix is already consumed,
      // it means the new prefix is shorter than a previous prefix

      // If the existing a new tags are different, this is an overlapping error,
      // otherwise we can simplify the matcher.
      if (next_node->tag() != tag) {
        throw Prefix_overlap_error(
            shcore::str_format("The prefix '%s' will shadow an existing prefix",
                               str.c_str()),
            next_node->tag());
      }
      next_node->set_final();
      return;
    }
  } else {
    next_node = &m_root;
  }

  while (index < end) {
    if (std::isspace(*index)) {
      // On spaces, adds 1 and ignores consecutive ones
      next_node = next_node->add(' ', this_tag);
      index += count_spaces(index, (end - index));
    } else {
      // Rest of characters are added once
      next_node = next_node->add(std::toupper(*index), this_tag);
      index++;
    }
  }

  next_node->set_final();
}

void String_prefix_matcher::check_for_overlaps(const std::string &str) {
  const char *index = str.data();
  const char *end = str.data() + str.size();

  Node *next_node = get_max_match(&index, end);

  if (next_node) {
    if (next_node->is_final()) {
      // If the last match is final it means a previous smaller prefix already
      // would match the new prefix, so it is an overlap.
      throw Prefix_overlap_error(
          shcore::str_format(
              "The prefix '%s' is shadowed by an existing prefix", str.c_str()),
          next_node->tag());
    } else if (index == end) {
      // If the last match is not final and the new prefix is already consumed,
      // it means the new prefix will shadow an existing prefix, so it is an
      // overlap.
      throw Prefix_overlap_error(
          shcore::str_format("The prefix '%s' will shadow an existing prefix",
                             str.c_str()),
          next_node->tag());
    }
  }
}

std::optional<std::string> String_prefix_matcher::matches(
    std::string_view str) const {
  const char *index = str.data();
  const char *end = str.data() + str.size();

  Node *next_node = get_max_match(&index, end);

  if (next_node && next_node->is_final()) {
    return next_node->tag();
  }

  return {};
}

std::set<std::string> String_prefix_matcher::list_strings() const {
  std::set<std::string> strings;
  m_root.get_strings("", &strings);
  return strings;
}

}  // namespace mysqlshdk
