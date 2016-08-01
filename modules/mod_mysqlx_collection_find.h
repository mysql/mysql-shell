/*
 * Copyright (c) 2015, Oracle and/or its affiliates. All rights reserved.
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

// MySQL DB access module, for use by plugins and others
// For the module that implements interactive DB functionality see mod_db

#ifndef _MOD_CRUD_COLLECTION_FIND_H_
#define _MOD_CRUD_COLLECTION_FIND_H_

#include "collection_crud_definition.h"

namespace mysh
{
  namespace mysqlx
  {
    class Collection;

    /**
    * Handler for document selection on a Collection.
    *
    * This object provides the necessary functions to allow selecting document data from a collection.
    *
    * This object should only be created by calling the find function on the collection object from which the documents will be retrieved.
    *
    * \sa Collection
    */
    class CollectionFind : public Collection_crud_definition, public std::enable_shared_from_this<CollectionFind>
    {
    public:
      CollectionFind(std::shared_ptr<Collection> owner);
    public:
      virtual std::string class_name() const { return "CollectionFind"; }
#if DOXYGEN_JS
      CollectionFind find(String searchCondition);
      CollectionFind fields(List projectedSearchExprStr);
      CollectionFind fields(DocExpression projection);
      CollectionFind groupBy(List searchExprStr);
      CollectionFind having(String searchCondition);
      CollectionFind sort(List sortExprStr);
      CollectionFind limit(Integer numberOfRows);
      CollectionFind skip(Integer limitOffset);
      CollectionFind bind(String name, Value value);
      DocResult execute();
#elif DOXYGEN_PY
      CollectionFind find(str searchCondition);
      CollectionFind fields(list projectedSearchExprStr);
      CollectionFind fields(DocExpression projection);
      CollectionFind group_by(list searchExprStr);
      CollectionFind having(str searchCondition);
      CollectionFind sort(list sortExprStr);
      CollectionFind limit(int numberOfRows);
      CollectionFind skip(int limitOffset);
      CollectionFind bind(str name, Value value);
      DocResult execute();
#endif
      shcore::Value find(const shcore::Argument_list &args);
      shcore::Value fields(const shcore::Argument_list &args);
      shcore::Value group_by(const shcore::Argument_list &args);
      shcore::Value having(const shcore::Argument_list &args);
      shcore::Value sort(const shcore::Argument_list &args);
      shcore::Value limit(const shcore::Argument_list &args);
      shcore::Value skip(const shcore::Argument_list &args);
      shcore::Value bind(const shcore::Argument_list &args);

      virtual shcore::Value execute(const shcore::Argument_list &args);


    private:
      std::unique_ptr< ::mysqlx::FindStatement> _find_statement;
    };
  };
};

#endif
