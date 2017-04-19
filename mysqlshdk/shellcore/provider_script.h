/*
 * Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MYSQLSHDK_SHELLCORE_PROVIDER_SCRIPT_H_
#define MYSQLSHDK_SHELLCORE_PROVIDER_SCRIPT_H_

#include <deque>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "shellcore/completer.h"

namespace shcore {
namespace completer {

/** Interface for objects that have a member that can be completed */
class Object {
 public:
  virtual std::string get_type() const = 0;
  virtual bool is_member_callable(const std::string &name) const = 0;
  virtual std::shared_ptr<Object> get_member(const std::string &name) const = 0;
  virtual size_t add_completions(const std::string &prefix,
                                 Completion_list *list) const = 0;
};

class Placeholder;

class Object_registry {
 public:
  struct Member {
    std::string name;
    std::string return_type;
    bool is_callable;
  };

  void add_completable_type(const std::string &type_name,
                            const std::vector<Member> &members);

  std::shared_ptr<Object> lookup(const std::string &class_name) const;

 private:
  std::map<std::string, std::shared_ptr<Object>> placeholders_;
};

class Provider_script : public Provider {
 public:
  explicit Provider_script(std::shared_ptr<Object_registry> registry);

  Completion_list complete(const std::string &text,
                           size_t *compl_offset) override;

  std::shared_ptr<Object_registry> object_registry() const {
    return registry_;
  }

 public:
  /** A chain of properties/method calls/objects that can be completed */
  class Chain {
   public:
    enum Type { Variable, Function, Literal };

    Chain() {
    }
    Chain(const Chain &c)
        : parts_(c.parts_), has_dot_(c.has_dot_), invalid_(c.invalid_) {
    }

    void clear() {
      parts_.clear();
      has_dot_ = false;
    }

    void add_dot() {
      has_dot_ = true;
    }

    bool add_variable(const std::string &s) {
      if ((!empty() && peek_type() == Literal) || !has_dot_)
        clear();
      has_dot_ = false;
      if (!s.empty() || !parts_.empty()) {
        parts_.push_back({Variable, s});
        // true if this is the 1st element of the chain
        return parts_.size() == 1;
      }
      return false;
    }

    bool add_method(const std::string &s) {
      if ((!empty() && peek_type() == Literal) || !has_dot_)
        clear();
      has_dot_ = false;
      parts_.push_back({Function, s});
      // true if this is the 1st element of the chain
      return parts_.size() == 1;
    }

    bool set_literal(const std::string &s) {
      has_dot_ = false;
      parts_.clear();
      parts_.push_back({Literal, s});
      return true;
    }

    void operator=(const Chain &o) {
      parts_ = o.parts_;
    }

    Type peek_type() const {
      return parts_.front().first;
    }

    std::pair<Type, std::string> next() {
      auto tmp(parts_.front());
      parts_.pop_front();
      return tmp;
    }

    bool empty() const {
      return parts_.empty();
    }
    size_t size() const {
      return parts_.size();
    }

    bool invalid() const {
      return invalid_;
    }

    void invalidate() {
      invalid_ = true;
    }

    const std::pair<Type, std::string> &operator[](size_t i) const {
      return parts_[i];
    }

   private:
    std::deque<std::pair<Type, std::string>> parts_;
    bool has_dot_ = false;
    bool invalid_ = false;
  };

 protected:
  std::shared_ptr<Object_registry> registry_;

  virtual Chain process_input(const std::string &input, size_t *compl_offset);

  virtual Chain parse_until(const std::string &s, size_t *pos, int close_char,
                            size_t *chain_start_pos) = 0;

  virtual Completion_list complete_chain(const Chain &chain);

  virtual std::shared_ptr<Object> lookup_global_object(
      const std::string &name) = 0;
};

}  // namespace completer
}  // namespace shcore

#endif  // MYSQLSHDK_SHELLCORE_PROVIDER_SCRIPT_H_
