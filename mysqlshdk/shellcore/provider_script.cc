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

#include "mysqlshdk/shellcore/provider_script.h"
#include <algorithm>
#include <cctype>
#include <iostream>
#include <iterator>
#include <set>

#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace shcore {
namespace completer {

/** A virtual object that represents an object class that supports completion of
 * its members through a static list */
class Placeholder : public Object {
 public:
  Placeholder(const Object_registry *registry, const std::string &type,
              const std::vector<Object_registry::Member> &members) {
    type_ = type;
    registry_ = registry;
    for (auto &m : members) {
      members_[m.name] = {m.return_type, std::weak_ptr<Object>(),
                          m.is_callable};
    }
  }

  std::string get_type() const override { return type_; }

  bool is_member_callable(const std::string &name) const override {
    auto iter = members_.find(name);
    if (iter != members_.end()) {
      return iter->second.is_callable;
    }
    return false;
  }

  std::shared_ptr<Object> get_member(const std::string &name) const override {
    auto iter = members_.find(name);
    if (iter != members_.end()) {
      if (!iter->second.return_object.expired()) {
        return iter->second.return_object.lock();
      } else {
        std::shared_ptr<Object> tmp(
            registry_->lookup(iter->second.return_type));
        iter->second.return_object = tmp;
        return tmp;
      }
    }
    return std::shared_ptr<Object>();
  }

  size_t add_completions(const std::string &prefix,
                         Completion_list *list) const override {
    size_t c = 0;
    for (auto &iter : members_) {
      if (shcore::str_beginswith(iter.first, prefix)) {
        list->push_back(iter.first + (iter.second.is_callable ? "()" : ""));
        ++c;
      }
    }
    return c;
  }

 private:
  const Object_registry *registry_;
  std::string type_;

  struct Member_info {
    std::string return_type;
    mutable std::weak_ptr<Object> return_object;
    bool is_callable;
  };
  // member name -> Member_info
  std::map<std::string, Member_info> members_;
};

void Object_registry::add_completable_type(
    const std::string &type_name,
    const std::vector<Object_registry::Member> &members) {
  placeholders_[type_name].reset(
      new Placeholder(this, type_name, members));
}

std::shared_ptr<Object> Object_registry::lookup(
    const std::string &class_name) const {
  auto iter = placeholders_.find(class_name);
  if (iter != placeholders_.end()) {
    return iter->second;
  }
  log_debug2(("Object type '%s' has no known completion rules registered\n",
              class_name.c_str()));
  return std::shared_ptr<Object>(nullptr);
}

Provider_script::Provider_script(std::shared_ptr<Object_registry> registry)
    : registry_(registry) {}

Completion_list Provider_script::complete(const std::string &text,
                                          size_t *compl_offset) {
  Chain chain = process_input(text, compl_offset);
  if (!chain.empty()) {
    *compl_offset = text.length() - chain[chain.size() - 1].second.length();
  }
  return complete_chain(chain);
}

Provider_script::Chain Provider_script::process_input(const std::string &s,
                                                      size_t *compl_offset) {
  size_t p = 0;
  size_t offset = 0;
  Chain chain = parse_until(s, &p, 0, &offset);
  if (compl_offset)
    *compl_offset = offset;
  return chain;
}

Completion_list Provider_script::complete_chain(const Chain &chain_a) {
  Completion_list list;

  if (chain_a.size() > 1 && !chain_a.invalid()) {
    Chain chain(chain_a);

    std::shared_ptr<Object> root;
    if (chain.peek_type() == Chain::Variable) {
      root = lookup_global_object(chain.next().second);
    }
    if (root) {
      while (!chain.empty()) {
        if (chain.size() == 1) {
          // the last element of the chain is the one being completed
          root->add_completions(chain.next().second, &list);
          break;
        }
        Chain::Type type;
        std::string member;
        std::tie(type, member) = chain.next();
        std::shared_ptr<Object> object = root->get_member(member);
        if (!object) {
          log_debug2(("No completable object known for member '%s' of '%s'",
                      member.c_str(), root->get_type().c_str()));
          break;
        }
        // obj.method.bla or obj.not_method().bla
        if ((type == Chain::Function) != root->is_member_callable(member)) {
          break;
        }
        root = object;
      }
    }
  }
  return list;
}

}  // namespace completer
}  // namespace shcore
