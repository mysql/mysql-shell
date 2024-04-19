/*
 * Copyright (c) 2023, 2024, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_LIBS_UTILS_STRING_LIST_MATCHER_H_
#define MYSQLSHDK_LIBS_UTILS_STRING_LIST_MATCHER_H_

#include <limits>
#include <memory>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace mysqlshdk {

class Prefix_overlap_error : public std::runtime_error {
 public:
  Prefix_overlap_error(const std::string &error, const std::string &tag);

  const std::string &tag() const { return m_tag; }

 private:
  std::string m_tag;
};

class String_prefix_matcher final {
 public:
  explicit String_prefix_matcher(const std::vector<std::string> &str_list = {},
                                 const std::string &tag = {});
  ~String_prefix_matcher() = default;

  void add_string(const std::string &str, const std::string &tag = {});
  std::optional<std::string> matches(std::string_view sql) const;
  std::set<std::string> list_strings() const;
  void check_for_overlaps(const std::string &str);

 private:
  /**
   * A node to be used in a string validation.
   *
   * Represents a valid element in a sequence and holds the information about
   * next valid elements.
   */
  class Node {
   public:
    Node(const std::string &tag) : m_tag(tag) {}
    bool is_final() const { return m_final; }
    void set_final();

    /**
     * Identifies if value is a registered as a valid next element.
     *
     * Returns the reference to the next Node is valid, otherwise nullprt
     */
    Node *get(unsigned char value) const;

    /**
     * Registers value as a valid next element (if did not exist already).
     *
     * Returns a reference to the associated next Node
     */
    Node *add(unsigned char value, const std::string &tag);

    /**
     * Used to create a list of text patterns with the data on this Node
     *
     * Prefix contains accumulated data from parent nodes, if this is a final
     * node the prefix is added to the list, otherwise this Node's data is
     * appended to the prefix and this function is called on the child Nodes
     */
    void get_strings(const std::string &prefix,
                     std::set<std::string> *list) const;

    /**
     * Returns the tag associated to the node.
     */
    const std::string &tag() const { return m_tag; }

   private:
    const std::string &m_tag;

    static constexpr auto k_alphabet_size =
        static_cast<int>(std::numeric_limits<char>::max()) + 1;

    std::unique_ptr<Node> m_next[k_alphabet_size] = {};
    bool m_final = true;
  };

  std::set<std::string> m_tags = {"__root__"};
  Node m_root;
  Node *get_max_match(const char **index, const char *end) const;
};
}  // namespace mysqlshdk
#endif  // MYSQLSHDK_LIBS_UTILS_STRING_LIST_MATCHER_H_