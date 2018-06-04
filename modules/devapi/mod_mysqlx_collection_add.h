/*
 * Copyright (c) 2015, 2017, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MODULES_DEVAPI_MOD_MYSQLX_COLLECTION_ADD_H_
#define MODULES_DEVAPI_MOD_MYSQLX_COLLECTION_ADD_H_

#include <memory>
#include <string>
#include <vector>
#include "modules/devapi/collection_crud_definition.h"

namespace mysqlsh {
namespace mysqlx {
class Collection;

/**
 * \ingroup XDevAPI
 * Handler for document addition on a Collection.
 *
 * This object provides the necessary functions to allow adding documents into a
 * collection.
 *
 * This object should only be created by calling any of the add functions on the
 * collection object where the documents will be added.
 *
 * \sa Collection
 */
class CollectionAdd : public Collection_crud_definition,
                      public std::enable_shared_from_this<CollectionAdd> {
 public:
  explicit CollectionAdd(std::shared_ptr<Collection> owner);

  std::string class_name() const override { return "CollectionAdd"; }

  shcore::Value add(const shcore::Argument_list &args);
  shcore::Value execute(const shcore::Argument_list &args) override;
  shcore::Value execute(bool upsert);

#if DOXYGEN_JS
  CollectionAdd add(DocDefinition document[, DocDefinition document, ...]);
  CollectionAdd add(List documents);
  Result execute();
#elif DOXYGEN_PY
  CollectionAdd add(DocDefinition document[, DocDefinition document, ...]);
  CollectionAdd add(list documents);
  Result execute();
#endif

 private:
  friend class Collection;
  void add_one_document(shcore::Value doc, const std::string &error_context);

  std::vector<std::string> last_document_ids_;
  Mysqlx::Crud::Insert message_;

  struct F {
    static constexpr Allowed_function_mask __shell_hook__ = 1 << 0;
    static constexpr Allowed_function_mask _empty = 1 << 1;
    static constexpr Allowed_function_mask add = 1 << 2;
    static constexpr Allowed_function_mask execute = 1 << 3;
  };

  Allowed_function_mask function_name_to_bitmask(
      const std::string &s) const override {
    if ("__shell_hook__" == s) {
      return F::__shell_hook__;
    }
    if ("" == s) {
      return F::_empty;
    }
    if ("add" == s) {
      return F::add;
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

#endif  // MODULES_DEVAPI_MOD_MYSQLX_COLLECTION_ADD_H_
