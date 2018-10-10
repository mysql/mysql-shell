/*
 * Copyright (c) 2015, 2018, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MODULES_DEVAPI_MOD_MYSQLX_COLLECTION_FIND_H_
#define MODULES_DEVAPI_MOD_MYSQLX_COLLECTION_FIND_H_

#include <algorithm>
#include <memory>
#include <string>
#include "modules/devapi/collection_crud_definition.h"

namespace mysqlsh {
namespace mysqlx {
class Collection;
class DocResult;

/**
 * \ingroup XDevAPI
 * Handler for document selection on a Collection.
 *
 * This object provides the necessary functions to allow selecting document data
 * from a collection.
 *
 * This object should only be created by calling the find function on the
 * collection object from which the documents will be retrieved.
 *
 * \sa Collection
 */
class CollectionFind : public Collection_crud_definition,
                       public std::enable_shared_from_this<CollectionFind> {
 public:
  explicit CollectionFind(std::shared_ptr<Collection> owner);

 public:
  std::string class_name() const override { return "CollectionFind"; }
#if DOXYGEN_JS
  CollectionFind find(String searchCondition);
  CollectionFind fields(String fieldDefinition[, String fieldDefinition, ...]);
  CollectionFind fields(List fieldDefinition);
  CollectionFind fields(DocExpression fieldDefinition);
  CollectionFind groupBy(List groupCriteria);
  CollectionFind groupBy(String groupCriteria[, String groupCriteria, ...]);
  CollectionFind having(String condition);
  CollectionFind sort(List sortCriteria);
  CollectionFind sort(String sortCriteria[, String sortCriteria, ...]);
  CollectionFind limit(Integer numberOfDocs);
  CollectionFind skip(Integer numberOfDocs);
  CollectionFind offset(Integer numberOfDocs);
  CollectionFind lockShared(String lockContention);
  CollectionFind lockExclusive(String lockContention);
  CollectionFind bind(String name, Value value);
  DocResult execute();
#elif DOXYGEN_PY
  CollectionFind find(str searchCondition);
  CollectionFind fields(str fieldDefinition[, str fieldDefinition, ...]);
  CollectionFind fields(list fieldDefinition);
  CollectionFind fields(DocExpression fieldDefinition);
  CollectionFind group_by(list groupCriteria);
  CollectionFind group_by(str groupCriteria[, str groupCriteria, ...]);
  CollectionFind having(str condition);
  CollectionFind sort(list sortCriteria);
  CollectionFind sort(str sortCriteria[, str sortCriteria, ...]);
  CollectionFind limit(int numberOfDocs);
  CollectionFind skip(int numberOfDocs);
  CollectionFind offset(int numberOfDocs);
  CollectionFind lock_shared(str lockContention);
  CollectionFind lock_exclusive(str lockContention);
  CollectionFind bind(str name, Value value);
  DocResult execute();
#endif
  shcore::Value find(const shcore::Argument_list &args);
  shcore::Value fields(const shcore::Argument_list &args);
  shcore::Value group_by(const shcore::Argument_list &args);
  shcore::Value having(const shcore::Argument_list &args);
  shcore::Value sort(const shcore::Argument_list &args);
  shcore::Value lock_shared(const shcore::Argument_list &args);
  shcore::Value lock_exclusive(const shcore::Argument_list &args);

  shcore::Value execute(const shcore::Argument_list &args) override;
  CollectionFind &set_filter(const std::string &filter);
  std::unique_ptr<DocResult> execute();

 private:
  Mysqlx::Crud::Find message_;
  void set_lock_contention(const shcore::Argument_list &args);
  void set_prepared_stmt() override;
  void update_limits() override { set_limits_on_message(&message_); }
  shcore::Value this_object() override;

  struct F {
    static constexpr Allowed_function_mask __shell_hook__ = 1 << 0;
    static constexpr Allowed_function_mask find = 1 << 1;
    static constexpr Allowed_function_mask fields = 1 << 2;
    static constexpr Allowed_function_mask groupBy = 1 << 3;
    static constexpr Allowed_function_mask having = 1 << 4;
    static constexpr Allowed_function_mask sort = 1 << 5;
    static constexpr Allowed_function_mask limit = 1 << 6;
    static constexpr Allowed_function_mask skip = 1 << 7;
    static constexpr Allowed_function_mask offset = 1 << 8;
    static constexpr Allowed_function_mask lockShared = 1 << 9;
    static constexpr Allowed_function_mask lockExclusive = 1 << 10;
    static constexpr Allowed_function_mask bind = 1 << 11;
    static constexpr Allowed_function_mask execute = 1 << 12;
  };

  Allowed_function_mask function_name_to_bitmask(
      const std::string &s) const override {
    if ("__shell_hook__" == s) {
      return F::__shell_hook__;
    }
    if ("find" == s) {
      return F::find;
    }
    if ("fields" == s) {
      return F::fields;
    }
    if ("groupBy" == s) {
      return F::groupBy;
    }
    if ("having" == s) {
      return F::having;
    }
    if ("sort" == s) {
      return F::sort;
    }
    if ("limit" == s) {
      return F::limit;
    }
    if ("skip" == s) {
      return F::skip;
    }
    if ("offset" == s) {
      return F::offset;
    }
    if ("lockShared" == s) {
      return F::lockShared;
    }
    if ("lockExclusive" == s) {
      return F::lockExclusive;
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

#endif  // MODULES_DEVAPI_MOD_MYSQLX_COLLECTION_FIND_H_
