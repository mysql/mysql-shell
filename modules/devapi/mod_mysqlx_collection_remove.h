/*
 * Copyright (c) 2015, 2019, Oracle and/or its affiliates. All rights reserved.
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

// MySQL DB access module, for use by plugins and others
// For the module that implements interactive DB functionality see mod_db

#ifndef MODULES_DEVAPI_MOD_MYSQLX_COLLECTION_REMOVE_H_
#define MODULES_DEVAPI_MOD_MYSQLX_COLLECTION_REMOVE_H_

#include <algorithm>
#include <memory>
#include <string>
#include "modules/devapi/collection_crud_definition.h"

namespace mysqlsh {
namespace mysqlx {
class Collection;

/**
 * \ingroup XDevAPI
 * $(COLLECTIONREMOVE_BRIEF)
 *
 * $(COLLECTIONREMOVE_DETAIL)
 *
 * $(COLLECTIONREMOVE_DETAIL1)
 *
 * \sa Collection
 */
class CollectionRemove : public Collection_crud_definition,
                         public std::enable_shared_from_this<CollectionRemove> {
 public:
  explicit CollectionRemove(std::shared_ptr<Collection> owner);

 public:
#if DOXYGEN_JS
  CollectionRemove remove(String searchCondition);
  CollectionRemove sort(List sortCriteria);
  CollectionRemove sort(String sortCriterion[, String sortCriterion, ...]);
  CollectionRemove limit(Integer numberOfRows);
  CollectionRemove bind(String name, Value value);
  Result execute();
#elif DOXYGEN_PY
  CollectionRemove remove(str searchCondition);
  CollectionRemove sort(list sortCriteria);
  CollectionRemove sort(str sortCriterion[, str sortCriterion, ...]);
  CollectionRemove limit(int numberOfRows);
  CollectionRemove bind(str name, Value value);
  Result execute();
#endif
  std::string class_name() const override { return "CollectionRemove"; }
  static std::shared_ptr<shcore::Object_bridge> create(
      const shcore::Argument_list &args);

 private:
  shcore::Value remove(const shcore::Argument_list &args);
  shcore::Value sort(const shcore::Argument_list &args);

  shcore::Value execute(const shcore::Argument_list &args) override;
  void set_prepared_stmt() override;
  void update_limits() override { set_limits_on_message(&message_); }
  shcore::Value this_object() override;
#if !defined DOXYGEN_JS && !defined DOXYGEN_PY
  shcore::Value execute();
#endif

  friend class Collection;
  CollectionRemove &set_filter(const std::string &filter);
  Mysqlx::Crud::Delete message_;

  struct F {
    static constexpr Allowed_function_mask __shell_hook__ = 1 << 0;
    static constexpr Allowed_function_mask remove = 1 << 1;
    static constexpr Allowed_function_mask sort = 1 << 2;
    static constexpr Allowed_function_mask limit = 1 << 3;
    static constexpr Allowed_function_mask bind = 1 << 4;
    static constexpr Allowed_function_mask execute = 1 << 5;
  };

  Allowed_function_mask function_name_to_bitmask(
      const std::string &s) const override {
    if ("__shell_hook__" == s) {
      return F::__shell_hook__;
    }
    if ("remove" == s) {
      return F::remove;
    }
    if ("sort" == s) {
      return F::sort;
    }
    if ("limit" == s) {
      return F::limit;
    }
    if ("bind" == s) {
      return F::bind;
    }
    if ("execute" == s) {
      return F::execute;
    }
    if ("help" == s) {
      return enabled_functions_;
    }
    return 0;
  }
};
}  // namespace mysqlx
}  // namespace mysqlsh

#endif  // MODULES_DEVAPI_MOD_MYSQLX_COLLECTION_REMOVE_H_
