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

#include "mysqlx_crud.h"
#include "mysqlx_connection.h"

#include "mysqlx_parser.h"

#include "compilerutils.h"

using namespace mysqlx;

Schema::Schema(boost::shared_ptr<Session> conn, const std::string &name_)
: m_sess(conn), m_name(name_)
{
}

boost::shared_ptr<Table> Schema::getTable(const std::string &name_)
{
  std::map<std::string, boost::shared_ptr<Table> >::const_iterator iter = m_tables.find(name_);
  if (iter != m_tables.end())
    return iter->second;
  return m_tables[name_] = boost::shared_ptr<Table>(new Table(shared_from_this(), name_));
}

boost::shared_ptr<Collection> Schema::getCollection(const std::string &name_)
{
  std::map<std::string, boost::shared_ptr<Collection> >::const_iterator iter = m_collections.find(name_);
  if (iter != m_collections.end())
    return iter->second;
  return m_collections[name_] = boost::shared_ptr<Collection>(new Collection(shared_from_this(), name_));
}

Table::Table(boost::shared_ptr<Schema> schema_, const std::string &name_)
: m_schema(schema_), m_name(name_)
{
}

DeleteStatement Table::remove()
{
  DeleteStatement tmp(shared_from_this());
  return tmp;
}

InsertStatement Table::insert()
{
  InsertStatement tmp(shared_from_this());
  return tmp;
}

SelectStatement Table::select(const std::string& fieldList)
{
  SelectStatement tmp(shared_from_this(), fieldList);
  return tmp;
}

Collection::Collection(boost::shared_ptr<Schema> schema_, const std::string &name_)
: m_schema(schema_), m_name(name_)
{
}

FindStatement Collection::find(const std::string &searchCondition)
{
  FindStatement tmp(shared_from_this(), searchCondition);
  return tmp;
}

ModifyStatement Collection::modify(const std::string &searchCondition)
{
  ModifyStatement tmp(shared_from_this(), searchCondition);
  return tmp;
}

AddStatement Collection::add(const Document &doc)
{
  AddStatement tmp(shared_from_this(), doc);
  return tmp;
}

RemoveStatement Collection::remove(const std::string &searchCondition)
{
  RemoveStatement tmp(shared_from_this(), searchCondition);
  return tmp;
}

Statement::~Statement()
{
}

Collection_Statement::Collection_Statement(boost::shared_ptr<Collection> coll)
: m_coll(coll)
{
}

Collection_Statement &Collection_Statement::bind(const std::string &UNUSED(name), const DocumentValue &UNUSED(value))
{
  return *this;
}

// -----

Find_Base::Find_Base(boost::shared_ptr<Collection> coll)
: Collection_Statement(coll), m_find(new Mysqlx::Crud::Find())
{
}

Find_Base::Find_Base(const Find_Base &other)
: Collection_Statement(other.m_coll), m_find(other.m_find)
{
}

Find_Base &Find_Base::operator = (const Find_Base &other)
{
  m_find = other.m_find;
  return *this;
}

Result *Find_Base::execute()
{
  if (!m_find->IsInitialized())
    throw std::logic_error("FindStatement is not completely initialized: " + m_find->InitializationErrorString());

  SessionRef session(m_coll->schema()->session());

  Result *result(session->connection()->execute_find(*m_find));

  // wait for results (at least metadata) to arrive
  result->wait();

  return result;
}

Find_Base &Find_Skip::skip(uint64_t skip_)
{
  m_find->mutable_limit()->set_offset(skip_);
  return *this;
}

Find_Skip &Find_Limit::limit(uint64_t limit_)
{
  m_find->mutable_limit()->set_row_count(limit_);
  return *this;
}

Find_Limit &Find_Sort::sort(const std::string &UNUSED(sortFields))
{
  return *this;
}

Find_Sort &Find_Having::having(const std::string &UNUSED(searchCondition))
{
  //  m_find->set_allocated_having(Expr_parser(searchFields, true).expr().release());
  return *this;
}

Find_Having &Find_GroupBy::groupBy(const std::string &UNUSED(searchFields))
{
  //  m_find->set_allocated_group_by(Proj_parser(searchFields, true).expr().release());
  return *this;
}

FindStatement::FindStatement(boost::shared_ptr<Collection> coll, const std::string &searchCondition)
: Find_GroupBy(coll)
{
  m_find->mutable_collection()->set_schema(coll->schema()->name());
  m_find->mutable_collection()->set_name(coll->name());
  m_find->set_data_model(Mysqlx::Crud::DOCUMENT);

  if (!searchCondition.empty())
    m_find->set_allocated_criteria(parser::parse_collection_filter(searchCondition));
}

Find_GroupBy &FindStatement::fields(const std::string &searchFields)
{
  parser::parse_collection_column_list_with_alias(*m_find->mutable_projection(), searchFields);

  return *this;
}

//----------------------------------

Add_Base::Add_Base(boost::shared_ptr<Collection> coll)
: Collection_Statement(coll), m_insert(new Mysqlx::Crud::Insert())
{
}

Add_Base::Add_Base(const Add_Base &other)
: Collection_Statement(other.m_coll), m_insert(other.m_insert)
{
}

Add_Base &Add_Base::operator = (const Add_Base &other)
{
  m_insert = other.m_insert;
  return *this;
}

Result *Add_Base::execute()
{
  if (!m_insert->IsInitialized())
    throw std::logic_error("AddStatement is not completely initialized: " + m_insert->InitializationErrorString());

  SessionRef session(m_coll->schema()->session());

  Result *result(session->connection()->execute_insert(*m_insert));

  result->wait();

  return result;
}

AddStatement::AddStatement(boost::shared_ptr<Collection> coll, const Document &doc)
: Add_Base(coll)
{
  m_insert->mutable_collection()->set_schema(coll->schema()->name());
  m_insert->mutable_collection()->set_name(coll->name());
  m_insert->set_data_model(Mysqlx::Crud::DOCUMENT);
  add(doc);
}

AddStatement &AddStatement::add(const Document &doc)
{
  Mysqlx::Datatypes::Any *any(m_insert->mutable_row()->Add()->mutable_field()->Add());
  any->set_type(Mysqlx::Datatypes::Any::SCALAR);
  Mysqlx::Datatypes::Scalar *value(any->mutable_scalar());
  value->set_type(Mysqlx::Datatypes::Scalar::V_OCTETS);
  value->set_v_opaque(doc.str());
  return *this;
}

//--------------------------------------------------------------

Remove_Base::Remove_Base(boost::shared_ptr<Collection> coll)
: Collection_Statement(coll), m_delete(new Mysqlx::Crud::Delete())
{
}

Remove_Base::Remove_Base(const Remove_Base &other)
: Collection_Statement(other.m_coll), m_delete(other.m_delete)
{
}

Remove_Base &Remove_Base::operator = (const Remove_Base &other)
{
  m_delete = other.m_delete;
  return *this;
}

Result *Remove_Base::execute()
{
  if (!m_delete->IsInitialized())
    throw std::logic_error("RemoveStatement is not completely initialized: " + m_delete->InitializationErrorString());

  SessionRef session(m_coll->schema()->session());

  Result *result(session->connection()->execute_delete(*m_delete));

  result->wait();

  return result;
}

Remove_Base &Remove_Limit::limit(uint64_t limit_)
{
  m_delete->mutable_limit()->set_row_count(limit_);
  return *this;
}

RemoveStatement::RemoveStatement(boost::shared_ptr<Collection> coll, const std::string &searchCondition)
: Remove_Limit(coll)
{
  m_delete->mutable_collection()->set_schema(coll->schema()->name());
  m_delete->mutable_collection()->set_name(coll->name());
  m_delete->set_data_model(Mysqlx::Crud::DOCUMENT);

  if (!searchCondition.empty())
    m_delete->set_allocated_criteria(parser::parse_collection_filter(searchCondition));
}

Remove_Limit &RemoveStatement::orderBy(const std::string &UNUSED(sortFields))
{
  return *this;
}

//--------------------------------------------------------------

Modify_Base::Modify_Base(boost::shared_ptr<Collection> coll)
: Collection_Statement(coll), m_update(new Mysqlx::Crud::Update())
{
}

Modify_Base::Modify_Base(const Modify_Base &other)
: Collection_Statement(other.m_coll), m_update(other.m_update)
{
}

Modify_Base &Modify_Base::operator = (const Modify_Base &other)
{
  m_coll = other.m_coll;
  m_update = other.m_update;
  return *this;
}

Result *Modify_Base::execute()
{
  if (!m_update->IsInitialized())
    throw std::logic_error("ModifyStatement is not completely initialized: " + m_update->InitializationErrorString());

  SessionRef session(m_coll->schema()->session());

  Result *result(session->connection()->execute_update(*m_update));

  result->wait();

  return result;
}

Modify_Operation &Modify_Operation::remove(const std::string &UNUSED(path))
{
  return *this;
}

Modify_Operation &Modify_Operation::set(const std::string &UNUSED(path), const DocumentValue &UNUSED(value))
{
  return *this;
}

Modify_Operation &Modify_Operation::replace(const std::string &UNUSED(path), const DocumentValue &UNUSED(value))
{
  return *this;
}

Modify_Operation &Modify_Operation::merge(const Document &UNUSED(doc))
{
  return *this;
}

Modify_Operation &Modify_Operation::arrayInsert(const std::string &UNUSED(path), int UNUSED(index), const DocumentValue &UNUSED(value))
{
  return *this;
}

Modify_Operation &Modify_Operation::arrayAppend(const std::string &UNUSED(path), const DocumentValue &UNUSED(value))
{
  return *this;
}

Modify_Operation &Modify_Limit::limit(uint64_t UNUSED(limit_))
{
  return *this;
}

ModifyStatement::ModifyStatement(boost::shared_ptr<Collection> coll, const std::string &UNUSED(searchCondition))
: Modify_Limit(coll)
{
  m_update->mutable_collection()->set_schema(coll->schema()->name());
  m_update->mutable_collection()->set_name(coll->name());
  m_update->set_data_model(Mysqlx::Crud::DOCUMENT);
  //  m_update->set_allocated_criteria(parser::parse_collection_filter(searchCondition).release());
}

Modify_Limit &ModifyStatement::orderBy(const std::string &UNUSED(sortFields))
{
  return *this;
}

//--------------------------------------------------------------

Table_Statement::Table_Statement(boost::shared_ptr<Table> table)
: m_table(table)
{
}

//Collection_Statement &Collection_Statement::bind(const std::string &UNUSED(name), const DocumentValue &UNUSED(value))
//{
//  return *this;
//}

Delete_Base::Delete_Base(boost::shared_ptr<Table> table)
: Table_Statement(table), m_delete(new Mysqlx::Crud::Delete())
{
}

Delete_Base::Delete_Base(const Delete_Base &other)
: Table_Statement(other.m_table), m_delete(other.m_delete)
{
}

Delete_Base &Delete_Base::operator = (const Delete_Base &other)
{
  m_delete = other.m_delete;
  return *this;
}

Result *Delete_Base::execute()
{
  if (!m_delete->IsInitialized())
    throw std::logic_error("DeleteStatement is not completely initialized: " + m_delete->InitializationErrorString());

  SessionRef session(m_table->schema()->session());

  Result *result(session->connection()->execute_delete(*m_delete));

  result->wait();

  return result;
}

Delete_Base &Delete_Limit::limit(uint64_t limit_)
{
  m_delete->mutable_limit()->set_row_count(limit_);
  return *this;
}

Delete_Limit &Delete_OrderBy::orderBy(const std::string &sortFields)
{
  //if (!sortFields.empty())
  //  m_delete->set_allocated_criteria(parser::parse_table_filter(searchCondition));
  return *this;
}

DeleteStatement::DeleteStatement(boost::shared_ptr<Table> table)
: Delete_OrderBy(table)
{
  m_delete->mutable_collection()->set_schema(table->schema()->name());
  m_delete->mutable_collection()->set_name(table->name());
  m_delete->set_data_model(Mysqlx::Crud::TABLE);
}

Delete_OrderBy &DeleteStatement::where(const std::string& searchCondition)
{
  if (!searchCondition.empty())
    m_delete->set_allocated_criteria(parser::parse_table_filter(searchCondition));

  return *this;
}

//--------------------------------------------------------------

Select_Base::Select_Base(boost::shared_ptr<Table> table)
: Table_Statement(table), m_find(new Mysqlx::Crud::Find())
{
}

Select_Base::Select_Base(const Select_Base &other)
: Table_Statement(other.m_table), m_find(other.m_find)
{
}

Select_Base &Select_Base::operator = (const Select_Base &other)
{
  m_find = other.m_find;
  return *this;
}

Result *Select_Base::execute()
{
  if (!m_find->IsInitialized())
    throw std::logic_error("SelectStatement is not completely initialized: " + m_find->InitializationErrorString());

  SessionRef session(m_table->schema()->session());

  Result *result(session->connection()->execute_find(*m_find));

  // wait for results (at least metadata) to arrive
  result->wait();

  return result;
}

Select_Base &Select_Skip::skip(uint64_t skip_)
{
  m_find->mutable_limit()->set_offset(skip_);
  return *this;
}

Select_Skip &Select_Limit::limit(uint64_t limit_)
{
  m_find->mutable_limit()->set_row_count(limit_);
  return *this;
}

Select_Limit &Select_Sort::sort(const std::string &UNUSED(sortFields))
{
  return *this;
}

Select_Sort &Select_Having::having(const std::string &UNUSED(searchCondition))
{
  //  m_find->set_allocated_having(Expr_parser(searchFields, true).expr().release());
  return *this;
}

Select_Having &Select_GroupBy::groupBy(const std::string &UNUSED(searchFields))
{
  //  m_find->set_allocated_group_by(Proj_parser(searchFields, true).expr().release());
  return *this;
}

SelectStatement::SelectStatement(boost::shared_ptr<Table> table, const std::string &fieldList)
: Select_GroupBy(table)
{
  m_find->mutable_collection()->set_schema(table->schema()->name());
  m_find->mutable_collection()->set_name(table->name());
  m_find->set_data_model(Mysqlx::Crud::TABLE);

  if (!fieldList.empty())
    parser::parse_table_column_list_with_alias(*m_find->mutable_projection(), fieldList);
}

Select_GroupBy &SelectStatement::where(const std::string &searchCondition)
{
  if (!searchCondition.empty())
    m_find->set_allocated_criteria(parser::parse_table_filter(searchCondition));

  return *this;
}

//--------------------------------------------------------------

Insert_Base::Insert_Base(boost::shared_ptr<Table> table)
: Table_Statement(table), m_insert(new Mysqlx::Crud::Insert())
{
}

Insert_Base::Insert_Base(const Insert_Base &other)
: Table_Statement(other.m_table), m_insert(other.m_insert)
{
}

Insert_Base &Insert_Base::operator = (const Insert_Base &other)
{
  m_insert = other.m_insert;
  return *this;
}

Result *Insert_Base::execute()
{
  if (!m_insert->IsInitialized())
    throw std::logic_error("InsertStatement is not completely initialized: " + m_insert->InitializationErrorString());

  SessionRef session(m_table->schema()->session());

  Result *result(session->connection()->execute_insert(*m_insert));

  result->wait();

  return result;
}

Insert_Values::Insert_Values(boost::shared_ptr<Table> table)
: Insert_Base(table)
{
}

Insert_Values &Insert_Values::values(const std::vector<TableValue> &row_data)
{
  Mysqlx::Crud::Insert_TypedRow* row = m_insert->mutable_row()->Add();
  std::vector<TableValue>::const_iterator index, end = row_data.end();

  for (index = row_data.begin(); index != end; index++)
  {
    Mysqlx::Datatypes::Any *any(row->mutable_field()->Add());
    any->set_type(Mysqlx::Datatypes::Any::SCALAR);
    Mysqlx::Datatypes::Scalar *value(any->mutable_scalar());

    mysqlx::TableValue column_value(*index);

    switch (index->type())
    {
      case TableValue::TInteger:
        value->set_type(Mysqlx::Datatypes::Scalar::V_SINT);
        value->set_v_signed_int(column_value);
        break;
      case TableValue::TUInteger:
        value->set_type(Mysqlx::Datatypes::Scalar::V_UINT);
        value->set_v_unsigned_int(column_value);
        break;
      case TableValue::TBool:
        value->set_type(Mysqlx::Datatypes::Scalar::V_BOOL);
        value->set_v_bool(column_value);
        break;
      case TableValue::TDouble:
        value->set_type(Mysqlx::Datatypes::Scalar::V_DOUBLE);
        value->set_v_double(column_value);
        break;
      case TableValue::TFloat:
        value->set_type(Mysqlx::Datatypes::Scalar::V_FLOAT);
        value->set_v_float(column_value);
        break;
      case TableValue::TNull:
        value->set_type(Mysqlx::Datatypes::Scalar::V_NULL);
        break;
      case TableValue::TOctets:
        value->set_type(Mysqlx::Datatypes::Scalar::V_OCTETS);
        value->set_v_opaque(column_value);
        break;
      case TableValue::TString:
        value->set_type(Mysqlx::Datatypes::Scalar::V_STRING);
        value->mutable_v_string()->set_value(column_value);
        break;
    }
  }

  return *this;
}

Insert_Values &InsertStatement::insert(const std::vector<std::string> &columns)
{
  for (size_t index = 0; index < columns.size(); index++)
    m_insert->mutable_projection()->Add()->set_name(columns[index]);

  return *this;
}

InsertStatement::InsertStatement(boost::shared_ptr<Table> table)
: Insert_Values(table)
{
  m_insert->mutable_collection()->set_schema(table->schema()->name());
  m_insert->mutable_collection()->set_name(table->name());
  m_insert->set_data_model(Mysqlx::Crud::TABLE);
}

//--------------------------------------------------------------