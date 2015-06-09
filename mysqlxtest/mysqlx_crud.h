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

#ifndef _MYSQLX_CRUD_H_
#define _MYSQLX_CRUD_H_

#include "mysqlx.h"

namespace Mysqlx
{
  namespace Crud
  {
    class Find;
    class Update;
    class Insert;
    class Delete;
  }
}

namespace mysqlx
{
  class Table;
  class Collection;

  typedef boost::shared_ptr<Table> TableRef;
  typedef boost::shared_ptr<Collection> CollectionRef;

  class Schema : public boost::enable_shared_from_this<Schema>
  {
  public:
    Schema(boost::shared_ptr<Session> conn, const std::string &name);

    TableRef getTable(const std::string &name);
    CollectionRef getCollection(const std::string &name);

    boost::shared_ptr<Session> session() const { return m_sess.lock(); }
    const std::string &name() const { return m_name; }
  private:
    std::map<std::string, boost::shared_ptr<Table> > m_tables;
    std::map<std::string, boost::shared_ptr<Collection> > m_collections;
    boost::weak_ptr<Session> m_sess;
    std::string m_name;
  };

  class Table : public boost::enable_shared_from_this<Table>
  {
  public:
    Table(boost::shared_ptr<Schema> schema, const std::string &name);

    boost::shared_ptr<Schema> schema() const { return m_schema.lock(); }
    const std::string& name() const { return m_name; }

  private:
    boost::weak_ptr<Schema> m_schema;
    std::string m_name;
  };

  // -------------------------------------------------------

  class Statement
  {
  public:
    virtual ~Statement();
    virtual Result *execute() = 0;
  };

  // -------------------------------------------------------

  class Collection_Statement : public Statement
  {
  public:
    Collection_Statement(boost::shared_ptr<Collection> coll);
    Collection_Statement &bind(const std::string &name, const DocumentValue &value);

    boost::shared_ptr<Collection> collection() const { return m_coll; }

  protected:
    boost::shared_ptr<Collection> m_coll;
  };


  class Find_Base : public Collection_Statement
  {
  public:
    Find_Base(boost::shared_ptr<Collection> coll);
    Find_Base(const Find_Base &other);
    Find_Base &operator = (const Find_Base &other);

    virtual Result *execute();
  protected:
    boost::shared_ptr<Mysqlx::Crud::Find> m_find;
  };

  class Find_Skip : public Find_Base
  {
  public:
    Find_Skip(boost::shared_ptr<Collection> coll) : Find_Base(coll) {}
    Find_Skip(const Find_Skip &other) : Find_Base(other) {}
    Find_Skip &operator = (const Find_Skip &other) { Find_Base::operator=(other); return *this; }

    Find_Base &skip(uint64_t skip);
  };

  class Find_Limit : public Find_Skip
  {
  public:
    Find_Limit(boost::shared_ptr<Collection> coll) : Find_Skip(coll) {}
    Find_Limit(const Find_Limit &other) : Find_Skip(other) {}
    Find_Limit &operator = (const Find_Limit &other) { Find_Skip::operator=(other); return *this; }

    Find_Skip &limit(uint64_t limit);
  };

  class Find_Sort : public Find_Limit
  {
  public:
    Find_Sort(boost::shared_ptr<Collection> coll) : Find_Limit(coll) {}
    Find_Sort(const Find_Sort &other) : Find_Limit(other) {}
    Find_Sort &operator = (const Find_Sort &other) { Find_Limit::operator=(other); return *this; }

    Find_Limit &sort(const std::string &sortFields);
  };

  class Find_Having : public Find_Sort
  {
  public:
    Find_Having(boost::shared_ptr<Collection> coll) : Find_Sort(coll) {}
    Find_Having(const Find_Having &other) : Find_Sort(other) {}
    Find_Having &operator = (const Find_Having &other) { Find_Sort::operator=(other); return *this; }

    Find_Sort &having(const std::string &searchCondition);
  };

  class Find_GroupBy : public Find_Having
  {
  public:
    Find_GroupBy(boost::shared_ptr<Collection> coll) : Find_Having(coll) {}
    Find_GroupBy(const Find_GroupBy &other) : Find_Having(other) {}
    Find_GroupBy &operator = (const Find_Having &other) { Find_Having::operator=(other); return *this; }

    Find_Having &groupBy(const std::string &searchFields);
  };

  class FindStatement : public Find_GroupBy
  {
  public:
    FindStatement(boost::shared_ptr<Collection> coll, const std::string &searchCondition);
    FindStatement(const FindStatement &other) : Find_GroupBy(other) {}
    FindStatement &operator = (const FindStatement &other) { Find_GroupBy::operator=(other); return *this; }

    Find_GroupBy &fields(const std::string &searchFields);
  };

  // -------------------------------------------------------

  class Add_Base : public Collection_Statement
  {
  public:
    Add_Base(boost::shared_ptr<Collection> coll);
    Add_Base(const Add_Base &other);
    Add_Base &operator = (const Add_Base &other);

    virtual Result *execute();
  protected:
    boost::shared_ptr<Mysqlx::Crud::Insert> m_insert;
  };


  class AddStatement : public Add_Base
  {
  public:
    AddStatement(boost::shared_ptr<Collection> coll, const Document &doc);
    AddStatement(const AddStatement &other) : Add_Base(other) {}
    AddStatement &operator = (const AddStatement &other) { Add_Base::operator=(other); return *this; }

    AddStatement &add(const Document &doc);
  };

  // -------------------------------------------------------

  class Modify_Base : public Collection_Statement
  {
  public:
    Modify_Base(boost::shared_ptr<Collection> coll);
    Modify_Base(const Modify_Base &other);
    Modify_Base &operator = (const Modify_Base &other);

    virtual Result *execute();
  protected:
    boost::shared_ptr<Mysqlx::Crud::Update> m_update;
  };


  class Modify_Operation : public Modify_Base
  {
  public:
    Modify_Operation(boost::shared_ptr<Collection> coll) : Modify_Base(coll) {}
    Modify_Operation(const Modify_Operation &other) : Modify_Base(other) {}
    Modify_Operation &operator = (const Modify_Operation &other) { Modify_Base::operator=(other); return *this; }

    Modify_Operation &remove(const std::string &path);
    Modify_Operation &set(const std::string &path, const DocumentValue &value);
    Modify_Operation &replace(const std::string &path, const DocumentValue &value);
    Modify_Operation &merge(const Document &doc);
    Modify_Operation &arrayInsert(const std::string &path, int index, const DocumentValue &value);
    Modify_Operation &arrayAppend(const std::string &path, const DocumentValue &value);
  };


  class Modify_Limit : public Modify_Operation
  {
  public:
    Modify_Limit(boost::shared_ptr<Collection> coll) : Modify_Operation(coll) {}
    Modify_Limit(const Modify_Limit &other) : Modify_Operation(other) {}
    Modify_Limit &operator = (const Modify_Limit &other) { Modify_Base::operator=(other); return *this; }

    Modify_Operation &limit(uint64_t limit);
  };


  class ModifyStatement : public Modify_Limit
  {
  public:
    ModifyStatement(boost::shared_ptr<Collection> coll, const std::string &searchCondition);
    ModifyStatement(const ModifyStatement &other) : Modify_Limit(other) {}
    ModifyStatement &operator = (const ModifyStatement &other) { Modify_Limit::operator=(other); return *this; }

    Modify_Limit &orderBy(const std::string &sortFields);
  };

  // -------------------------------------------------------

  class Remove_Base : public Collection_Statement
  {
  public:
    Remove_Base(boost::shared_ptr<Collection> coll);
    Remove_Base(const Remove_Base &other);
    Remove_Base &operator = (const Remove_Base &other);

    virtual Result *execute();
  protected:
    boost::shared_ptr<Mysqlx::Crud::Delete> m_delete;
  };

  class Remove_Limit : public Remove_Base
  {
  public:
    Remove_Limit(boost::shared_ptr<Collection> coll) : Remove_Base(coll) {}
    Remove_Limit(const Remove_Limit &other) : Remove_Base(other) {}
    Remove_Limit &operator = (const Remove_Limit &other) { Remove_Base::operator=(other); return *this; }

    Remove_Base &limit(uint64_t limit);
  };

  class RemoveStatement : public Remove_Limit
  {
  public:
    RemoveStatement(boost::shared_ptr<Collection> coll, const std::string &searchCondition);
    RemoveStatement(const RemoveStatement &other) : Remove_Limit(other) {}
    RemoveStatement &operator = (const RemoveStatement &other) { Remove_Limit::operator=(other); return *this; }

    Remove_Limit &orderBy(const std::string &sortFields);
  };

  // -------------------------------------------------------

  class Collection : public boost::enable_shared_from_this<Collection>
  {
  public:
    Collection(boost::shared_ptr<Schema> schema, const std::string &name);

    boost::shared_ptr<Schema> schema() const { return m_schema.lock(); }
    const std::string& name() const { return m_name; }

    FindStatement find(const std::string &searchCondition);
    AddStatement add(const Document &doc);
    ModifyStatement modify(const std::string &searchCondition);
    RemoveStatement remove(const std::string &searchCondition);

  private:
    boost::weak_ptr<Schema> m_schema;
    std::string m_name;
  };

};

#endif
