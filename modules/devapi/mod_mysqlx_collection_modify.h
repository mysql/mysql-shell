/*
 * Copyright (c) 2015, 2024, Oracle and/or its affiliates.
 * reserved.
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

// MySQL DB access module, for use by plugins and others
// For the module that implements interactive DB functionality see mod_db

#ifndef MODULES_DEVAPI_MOD_MYSQLX_COLLECTION_MODIFY_H_
#define MODULES_DEVAPI_MOD_MYSQLX_COLLECTION_MODIFY_H_

#include <memory>
#include <string>

#include "modules/devapi/collection_crud_definition.h"

namespace mysqlsh {
namespace mysqlx {
class Collection;
class Result;
/**
 * \ingroup XDevAPI
 * $(COLLECTIONMODIFY_BRIEF)
 *
 * $(COLLECTIONMODIFY_DETAIL)
 *
 * $(COLLECTIONMODIFY_DETAIL1)
 *
 * \sa Collection
 */
class CollectionModify : public Collection_crud_definition,
                         public std::enable_shared_from_this<CollectionModify> {
 public:
  explicit CollectionModify(std::shared_ptr<Collection> owner);

 public:
#if DOXYGEN_JS
  CollectionModify modify(String searchCondition);
  CollectionModify set(String attribute, Value value);
  CollectionModify unset(String attribute[, String attribute, ...]);
  CollectionModify unset(List attributes);
  CollectionModify merge(Document document);
  CollectionModify patch(Document document);
  CollectionModify arrayAppend(String docPath, Value value);
  CollectionModify arrayInsert(String docPath, Value value);
  CollectionModify arrayDelete(String docPath);
  CollectionModify sort(List sortCriteria);
  CollectionModify sort(String sortCriterion[, String sortCriterion, ...]);
  CollectionModify limit(Integer numberOfRows);
  CollectionModify bind(String name, Value value);
  Result execute();
#elif DOXYGEN_PY
  CollectionModify modify(str searchCondition);
  CollectionModify set(str attribute, Value value);
  CollectionModify unset(str attribute[, str attribute, ...]);
  CollectionModify unset(list attributes);
  CollectionModify merge(Document document);
  CollectionModify patch(Document document);
  CollectionModify array_append(str docPath, Value value);
  CollectionModify array_insert(str docPath, Value value);
  CollectionModify array_delete(str docPath);
  CollectionModify sort(list sortCriteria);
  CollectionModify sort(str sortCriterion[, str sortCriterion, ...]);
  CollectionModify limit(int numberOfRows);
  CollectionModify bind(str name, Value value);
  Result execute();
#endif
  std::string class_name() const override { return "CollectionModify"; }
  static std::shared_ptr<shcore::Object_bridge> create(
      const shcore::Argument_list &args);

 private:
  std::shared_ptr<CollectionModify> modify(const std::string &condition);
  std::shared_ptr<CollectionModify> set(const std::string &attribute,
                                        shcore::Value value);
  shcore::Value unset(const shcore::Argument_list &args);
  shcore::Value merge(const shcore::Argument_list &args);
  shcore::Value patch(const shcore::Argument_list &args);
  std::shared_ptr<CollectionModify> array_insert(const std::string &doc_path,
                                                 shcore::Value value);
  std::shared_ptr<CollectionModify> array_append(const std::string &doc_path,
                                                 shcore::Value value);
  std::shared_ptr<CollectionModify> array_delete(const std::string &doc_path);
  shcore::Value sort(const shcore::Argument_list &args);
  shcore::Value execute(const shcore::Argument_list &args) override;
  void set_prepared_stmt() override;
  void update_limits() override { set_limits_on_message(&message_); }
  shcore::Value this_object() override;
#if !defined DOXYGEN_JS && !defined DOXYGEN_PY
  shcore::Value execute();
#endif
  friend class Collection;
  Mysqlx::Crud::Update message_;
  CollectionModify &set_filter(const std::string &filter);
  void set_operation(int type, const std::string &path,
                     const shcore::Value &value, bool validate_array = false);

  struct F {
    static constexpr Allowed_function_mask operation = 1 << 0;
    static constexpr Allowed_function_mask modify = 1 << 1;
    static constexpr Allowed_function_mask set = 1 << 2;
    static constexpr Allowed_function_mask unset = 1 << 3;
    static constexpr Allowed_function_mask merge = 1 << 4;
    static constexpr Allowed_function_mask patch = 1 << 5;
    static constexpr Allowed_function_mask arrayInsert = 1 << 6;
    static constexpr Allowed_function_mask arrayAppend = 1 << 7;
    static constexpr Allowed_function_mask arrayDelete = 1 << 8;
    static constexpr Allowed_function_mask sort = 1 << 9;
    static constexpr Allowed_function_mask limit = 1 << 10;
    static constexpr Allowed_function_mask bind = 1 << 11;
    static constexpr Allowed_function_mask execute = 1 << 12;
  };

  Allowed_function_mask function_name_to_bitmask(
      std::string_view s) const override {
    if ("operation" == s) {
      return F::operation;
    }
    if ("modify" == s) {
      return F::modify;
    }
    if ("set" == s) {
      return F::set;
    }
    if ("unset" == s) {
      return F::unset;
    }
    if ("merge" == s) {
      return F::merge;
    }
    if ("patch" == s) {
      return F::patch;
    }
    if ("arrayInsert" == s) {
      return F::arrayInsert;
    }
    if ("arrayAppend" == s) {
      return F::arrayAppend;
    }
    if ("arrayDelete" == s) {
      return F::arrayDelete;
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

#endif  // MODULES_DEVAPI_MOD_MYSQLX_COLLECTION_MODIFY_H_
