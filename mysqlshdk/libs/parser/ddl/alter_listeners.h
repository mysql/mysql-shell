/*
 * Copyright (c) 2016, 2026, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation. The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef MYSQLSHDK_LIBS_PARSER_DDL_ALTER_LISTENERS_H_
#define MYSQLSHDK_LIBS_PARSER_DDL_ALTER_LISTENERS_H_

#include <string>
#include <vector>
#include "mysqlshdk/include/scripting/types.h"

#include "mysqlshdk/libs/parser/mysql/MySQLParserBaseListener.h"

namespace mysqlshdk {
namespace parser {

class Alter_listener : public parsers::MySQLParserBaseListener {
 public:
  Alter_listener(const std::string &current_schema,
                 shcore::Array_t *change_list)
      : Alter_listener(current_schema, change_list, false) {}

  Alter_listener(const std::string &current_schema,
                 shcore::Array_t *change_list, bool no_backslash_escapes)
      : m_schema(current_schema),
        m_change_list(change_list),
        m_no_backslash_escapes(no_backslash_escapes) {}

 protected:
  void add_update(const std::string &type, const std::string &name,
                  const shcore::Dictionary_t &updates);
  void add_update(const std::string &type, const std::string &name,
                  const std::string &table_name,
                  const shcore::Dictionary_t &updates);
  void add_replace(const std::string &type, const std::string &name,
                   const std::string &table_name,
                   const shcore::Dictionary_t &updates);
  void add_drop(const std::string &type, const std::string &name,
                const std::string &parent_name = "");
  void add_create(const std::string &type, const std::string &table_name,
                  const shcore::Dictionary_t &change);

 private:
  void add_change(const std::string &op, const shcore::Dictionary_t &change);

 protected:
  std::string m_schema;
  shcore::Array_t *m_change_list;
  bool m_no_backslash_escapes = false;
};

class Schema_alter_listener : public Alter_listener {
 public:
  Schema_alter_listener(antlr4::tree::ParseTree *tree,
                        shcore::Array_t *change_list);

  void exitAlterDatabase(
      parsers::MySQLParser::AlterDatabaseContext *ctx) override;
  void exitAlterDatabaseOption(
      parsers::MySQLParser::AlterDatabaseOptionContext *ctx) override;

  void exitCharsetName(parsers::MySQLParser::CharsetNameContext *ctx) override;
  void exitCollationName(
      parsers::MySQLParser::CollationNameContext *ctx) override;
  void exitDefaultEncryption(
      parsers::MySQLParser::DefaultEncryptionContext *ctx) override;

 private:
  shcore::Dictionary_t m_changes;
};

class Table_alter_listener : public Alter_listener {
 public:
  Table_alter_listener(antlr4::tree::ParseTree *tree,
                       const std::string &current_schema,
                       shcore::Array_t *change_list,
                       bool no_backslash_escapes = false);

  void enterAlterTable(parsers::MySQLParser::AlterTableContext *ctx) override;

  void exitAlterListItem(
      parsers::MySQLParser::AlterListItemContext *ctx) override;
  void exitTableConstraintDef(
      parsers::MySQLParser::TableConstraintDefContext *ctx) override;
  void exitTableElement(
      parsers::MySQLParser::TableElementContext *ctx) override;
  void exitCreateTableOptionsSpaceSeparated(
      parsers::MySQLParser::CreateTableOptionsSpaceSeparatedContext *ctx)
      override;
  void exitPartitionClause(
      parsers::MySQLParser::PartitionClauseContext *ctx) override;
  void exitRemovePartitioning(
      parsers::MySQLParser::RemovePartitioningContext *ctx) override;
  void exitAlterPartition(
      parsers::MySQLParser::AlterPartitionContext *ctx) override;

 private:
  std::string m_table_name;
};

class View_alter_listener : public Alter_listener {
 public:
  View_alter_listener(antlr4::tree::ParseTree *tree,
                      const std::string &current_schema,
                      shcore::Array_t *change_list);

  void exitAlterView(parsers::MySQLParser::AlterViewContext *ctx) override;
  void exitViewCheckOption(
      parsers::MySQLParser::ViewCheckOptionContext *) override;
  void exitViewAlgorithm(
      parsers::MySQLParser::ViewAlgorithmContext *ctx) override;
  void exitDefinerClause(
      parsers::MySQLParser::DefinerClauseContext *ctx) override;
  void exitViewTail(parsers::MySQLParser::ViewTailContext *ctx) override;
  void exitViewSuid(parsers::MySQLParser::ViewSuidContext *ctx) override;
  void exitColumnInternalRefList(
      parsers::MySQLParser::ColumnInternalRefListContext *ctx) override;

 private:
  shcore::Dictionary_t m_changes;
};

class Routine_alter_listener : public Alter_listener {
 public:
  Routine_alter_listener(antlr4::tree::ParseTree *tree,
                         const std::string &current_schema,
                         shcore::Array_t *change_list,
                         bool no_backslash_escapes = false);

  void exitAlterStatement(
      parsers::MySQLParser::AlterStatementContext *ctx) override;
  void exitRoutineCreateOption(
      parsers::MySQLParser::RoutineCreateOptionContext *ctx) override;
  void exitRoutineOption(
      parsers::MySQLParser::RoutineOptionContext *ctx) override;

 private:
  shcore::Dictionary_t m_changes;
};

class Event_alter_listener : public Alter_listener {
 public:
  Event_alter_listener(antlr4::tree::ParseTree *tree,
                       const std::string &current_schema,
                       shcore::Array_t *change_list,
                       bool no_backslash_escapes = false);

  void exitDefinerClause(
      parsers::MySQLParser::DefinerClauseContext *ctx) override;
  void exitAlterEvent(parsers::MySQLParser::AlterEventContext *ctx) override;
  void exitSchedule(parsers::MySQLParser::ScheduleContext *ctx) override;

 private:
  shcore::Dictionary_t m_changes;
};

}  // namespace parser
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_PARSER_DDL_ALTER_LISTENERS_H_
