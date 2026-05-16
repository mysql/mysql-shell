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

#ifndef MYSQLSHDK_LIBS_PARSER_DDL_CREATE_LISTENERS_H_
#define MYSQLSHDK_LIBS_PARSER_DDL_CREATE_LISTENERS_H_

#include <string>
#include <vector>
#include "mysqlshdk/include/scripting/types.h"

namespace mysqlshdk {
namespace parser {

struct Identifier {
  std::string schema;
  std::string name;
};

std::string unquote_text_literal(parsers::MySQLParser::TextLiteralContext *ctx,
                                 bool no_backslash_escapes = false);
std::string unquote_text_string_literal(
    parsers::MySQLParser::TextStringLiteralContext *ctx,
    bool no_backslash_escapes = false);

// In order to be able to resolve column references in indices and foreign keys
// we must store column names along with additional info and do the resolution
// after everything has been parsed. This is necessary because index/FK
// definitions and column definitions can appear in random order and also
// referenced tables can appear after an FK definition. Similarly, when
// resolving table names (qualified or not), we can do that also only after all
// parsing is done.
struct Db_object_references {
  enum class Reference_type { Index, Referencing, Referenced, TableRef };

  Db_object_references(shcore::Dictionary_t fk, Reference_type type_)
      : type(type_), foreign_key(std::move(fk)) {}

  Reference_type type;

  // Only one of these is valid (or none), depending on type.
  shcore::Dictionary_t foreign_key;
  shcore::Dictionary_t index;

  // These 2 are only used for target tables.
  Identifier target_identifier;

  // The list of column names for resolution. For indices the column names are
  // stored in the index column entries that make up an index.
  std::vector<std::string> column_names;

  shcore::Dictionary_t table;  // The referencing table.
};

// Object listeners are used to implement the visitor pattern when walking a
// parse tree to get object specific informations out. For easier handling and
// readability we use individual listeners for each schema object type.

// Specialized listener for (qualified) identifiers.
class Identifier_listener : public parsers::MySQLParserBaseListener {
 public:
  explicit Identifier_listener(antlr4::tree::ParseTree *tree);

  void enterIdentifier(parsers::MySQLParser::IdentifierContext *ctx) override;

  std::vector<std::string> parts;  // 1 - 3 identifier parts.
};

class Object_listener : public parsers::MySQLParserBaseListener {
 public:
  explicit Object_listener(shcore::Dictionary_t anObject,
                           bool no_backslash_escapes = false);

  bool ignore_if_exists = false;

 protected:
  shcore::Dictionary_t m_object;
  bool m_no_backslash_escapes = false;
};

class Data_type_listener : public parsers::MySQLParserBaseListener {
 public:
  Data_type_listener(antlr4::tree::ParseTree *tree, shcore::Array_t flags);

  void exitDataType(parsers::MySQLParser::DataTypeContext *ctx) override;
  void exitFieldLength(parsers::MySQLParser::FieldLengthContext *ctx) override;
  void exitPrecision(parsers::MySQLParser::PrecisionContext *ctx) override;
  void exitFieldOptions(
      parsers::MySQLParser::FieldOptionsContext *ctx) override;
  void exitCharsetWithOptBinary(
      parsers::MySQLParser::CharsetWithOptBinaryContext *ctx) override;
  void exitCharsetName(parsers::MySQLParser::CharsetNameContext *ctx) override;
  void exitTypeDatetimePrecision(
      parsers::MySQLParser::TypeDatetimePrecisionContext *ctx) override;
  void exitStringList(parsers::MySQLParser::StringListContext *ctx) override;

  std::string data_type_name;
  std::string data_type_params;
  int scale = -1;
  int precision = -1;
  int length = -1;
  std::string charset_name;
  std::string explicit_params;

 private:
  shcore::Array_t m_flags;
};

class Schema_listener : public Object_listener {
 public:
  Schema_listener(antlr4::tree::ParseTree *tree, shcore::Dictionary_t anObject);

  void exitCreateDatabase(
      parsers::MySQLParser::CreateDatabaseContext *ctx) override;
  void exitCharsetName(parsers::MySQLParser::CharsetNameContext *ctx) override;
  void exitCollationName(
      parsers::MySQLParser::CollationNameContext *ctx) override;
  void exitDefaultEncryption(
      parsers::MySQLParser::DefaultEncryptionContext *ctx) override;
};

class Table_listener : public Object_listener {
 public:
  Table_listener(antlr4::tree::ParseTree *tree, shcore::Dictionary_t table,
                 bool no_backslash_escapes = false);

  void exitTableName(parsers::MySQLParser::TableNameContext *ctx) override;
  void exitCreateTable(parsers::MySQLParser::CreateTableContext *ctx) override;
  void exitPartitionClause(
      parsers::MySQLParser::PartitionClauseContext *ctx) override;
  void exitDuplicateAsQe(
      parsers::MySQLParser::DuplicateAsQeContext *ctx) override;
  void exitCreateTableOptions(
      parsers::MySQLParser::CreateTableOptionsContext *ctx) override;
};

class Create_table_option_listener : public Object_listener {
 public:
  Create_table_option_listener(antlr4::tree::ParseTree *tree,
                               shcore::Dictionary_t table,
                               bool no_backslash_escapes = false);

  void exitCreateTableOption(
      parsers::MySQLParser::CreateTableOptionContext *ctx) override;
};

class Column_definition_listener : public parsers::MySQLParserBaseListener {
 public:
  shcore::Dictionary_t column;

  Column_definition_listener(antlr4::tree::ParseTree *tree,
                             shcore::Dictionary_t table,
                             bool no_backslash_escapes = false);
  void enterColumnDefinition(
      parsers::MySQLParser::ColumnDefinitionContext *ctx) override;
  void exitFieldDefinition(
      parsers::MySQLParser::FieldDefinitionContext *ctx) override;
  void exitColumnAttribute(
      parsers::MySQLParser::ColumnAttributeContext *ctx) override;
  void exitReferences(parsers::MySQLParser::ReferencesContext *ctx) override;

 private:
  shcore::Dictionary_t m_table;
  bool m_no_backslash_escapes = false;
};

class Key_definition_listener : public parsers::MySQLParserBaseListener {
 public:
  Key_definition_listener(antlr4::tree::ParseTree *tree,
                          shcore::Dictionary_t table,
                          bool no_backslash_escapes = false);

  void exitTableConstraintDef(
      parsers::MySQLParser::TableConstraintDefContext *ctx) override;
  void exitIndexType(parsers::MySQLParser::IndexTypeContext *ctx) override;

  void exitCommonIndexOption(
      parsers::MySQLParser::CommonIndexOptionContext *ctx) override;

  void exitFulltextIndexOption(
      parsers::MySQLParser::FulltextIndexOptionContext *ctx) override;

 private:
  shcore::Dictionary_t m_table;
  shcore::Dictionary_t m_current_index;
  bool m_no_backslash_escapes = false;
};

class Logfile_group_listener : public Object_listener {
 public:
  Logfile_group_listener(antlr4::tree::ParseTree *tree,
                         shcore::Dictionary_t anObject,
                         bool no_backslash_escapes = false);

  void exitCreateLogfileGroup(
      parsers::MySQLParser::CreateLogfileGroupContext *ctx) override;
  void exitTsOptionInitialSize(
      parsers::MySQLParser::TsOptionInitialSizeContext *ctx) override;
  void exitTsOptionUndoRedoBufferSize(
      parsers::MySQLParser::TsOptionUndoRedoBufferSizeContext *ctx) override;
  void exitTsOptionNodegroup(
      parsers::MySQLParser::TsOptionNodegroupContext *ctx) override;
  void exitTsOptionEngine(
      parsers::MySQLParser::TsOptionEngineContext *ctx) override;
  void exitTsOptionWait(
      parsers::MySQLParser::TsOptionWaitContext *ctx) override;
  void exitTsOptionComment(
      parsers::MySQLParser::TsOptionCommentContext *ctx) override;
};

class Library_listener : public Object_listener {
 public:
  Library_listener(antlr4::tree::ParseTree *tree,
                   shcore::Dictionary_t anObject);

  void exitCreateLibrary(
      parsers::MySQLParser::CreateLibraryContext *ctx) override;
};

// Used for SF, SP and UDF.
class Routine_listener : public Object_listener {
 public:
  Routine_listener(antlr4::tree::ParseTree *tree, shcore::Dictionary_t routine,
                   bool no_backslash_escapes = false);

  void exitDefinerClause(
      parsers::MySQLParser::DefinerClauseContext *ctx) override;

  void enterCreateProcedure(
      parsers::MySQLParser::CreateProcedureContext *ctx) override;
  void exitCreateProcedure(
      parsers::MySQLParser::CreateProcedureContext *ctx) override;
  void enterCreateFunction(
      parsers::MySQLParser::CreateFunctionContext *ctx) override;
  void exitCreateFunction(
      parsers::MySQLParser::CreateFunctionContext *ctx) override;
  void exitCreateUdf(parsers::MySQLParser::CreateUdfContext *ctx) override;

  void exitProcedureParameter(
      parsers::MySQLParser::ProcedureParameterContext *ctx) override;
  void enterFunctionParameter(
      parsers::MySQLParser::FunctionParameterContext *ctx) override;
  void exitFunctionParameter(
      parsers::MySQLParser::FunctionParameterContext *ctx) override;

  void exitRoutineCreateOption(
      parsers::MySQLParser::RoutineCreateOptionContext *ctx) override;
  void exitRoutineOption(
      parsers::MySQLParser::RoutineOptionContext *ctx) override;

 private:
  void addRoutineLibraries(parsers::MySQLParser::LibraryListContext *ctx);
  void readRoutineName(antlr4::ParserRuleContext *ctx);

  shcore::Dictionary_t m_current_parameter;
};

class Index_listener : public Object_listener {
 public:
  Index_listener(antlr4::tree::ParseTree *tree, shcore::Dictionary_t index,
                 bool no_backslash_escapes = false);

  void exitCreateIndex(parsers::MySQLParser::CreateIndexContext *ctx) override;
  void exitIndexType(parsers::MySQLParser::IndexTypeContext *ctx) override;
  void exitCreateIndexTarget(
      parsers::MySQLParser::CreateIndexTargetContext *ctx) override;
  void exitCommonIndexOption(
      parsers::MySQLParser::CommonIndexOptionContext *ctx) override;
  void exitFulltextIndexOption(
      parsers::MySQLParser::FulltextIndexOptionContext *ctx) override;
  void exitAlterAlgorithmOption(
      parsers::MySQLParser::AlterAlgorithmOptionContext *ctx) override;
  void exitAlterLockOption(
      parsers::MySQLParser::AlterLockOptionContext *ctx) override;
};

class Trigger_listener : public Object_listener {
 public:
  Trigger_listener(antlr4::tree::ParseTree *tree, shcore::Dictionary_t trigger);

  void exitDefinerClause(
      parsers::MySQLParser::DefinerClauseContext *ctx) override;
  void exitCreateTrigger(
      parsers::MySQLParser::CreateTriggerContext *ctx) override;
  void exitTriggerFollowsPrecedesClause(
      parsers::MySQLParser::TriggerFollowsPrecedesClauseContext *ctx) override;
};

class View_listener : public Object_listener {
 public:
  View_listener(antlr4::tree::ParseTree *tree, shcore::Dictionary_t anObject);

  void exitCreateView(parsers::MySQLParser::CreateViewContext *ctx) override;
  void exitViewCheckOption(
      parsers::MySQLParser::ViewCheckOptionContext *ctx) override;
  void exitViewAlgorithm(
      parsers::MySQLParser::ViewAlgorithmContext *ctx) override;
  void exitDefinerClause(
      parsers::MySQLParser::DefinerClauseContext *ctx) override;
  void exitViewQueryBlock(
      parsers::MySQLParser::ViewQueryBlockContext *ctx) override;
  void exitViewSuid(parsers::MySQLParser::ViewSuidContext *ctx) override;
  void exitColumnInternalRefList(
      parsers::MySQLParser::ColumnInternalRefListContext *ctx) override;
};

class Server_listener : public Object_listener {
 public:
  Server_listener(antlr4::tree::ParseTree *tree, shcore::Dictionary_t anObject,
                  bool no_backslash_escapes = false);

  void exitCreateServer(
      parsers::MySQLParser::CreateServerContext *ctx) override;
  void exitServerOption(
      parsers::MySQLParser::ServerOptionContext *ctx) override;
};

class Tablespace_listener : public Object_listener {
 public:
  Tablespace_listener(antlr4::tree::ParseTree *tree,
                      shcore::Dictionary_t anObject,
                      bool no_backslash_escapes = false);

  void exitCreateTablespace(
      parsers::MySQLParser::CreateTablespaceContext *ctx) override;
  void exitLogfileGroupRef(
      parsers::MySQLParser::LogfileGroupRefContext *ctx) override;
  void exitTsDataFile(parsers::MySQLParser::TsDataFileContext *ctx) override;
  void exitTsOptionInitialSize(
      parsers::MySQLParser::TsOptionInitialSizeContext *ctx) override;
  void exitTsOptionAutoextendSize(
      parsers::MySQLParser::TsOptionAutoextendSizeContext *ctx) override;
  void exitTsOptionMaxSize(
      parsers::MySQLParser::TsOptionMaxSizeContext *ctx) override;
  void exitTsOptionExtentSize(
      parsers::MySQLParser::TsOptionExtentSizeContext *ctx) override;
  void exitTsOptionNodegroup(
      parsers::MySQLParser::TsOptionNodegroupContext *ctx) override;
  void exitTsOptionEngine(
      parsers::MySQLParser::TsOptionEngineContext *ctx) override;
  void exitTsOptionWait(
      parsers::MySQLParser::TsOptionWaitContext *ctx) override;
  void exitTsOptionComment(
      parsers::MySQLParser::TsOptionCommentContext *ctx) override;
  void exitTsOptionFileblockSize(
      parsers::MySQLParser::TsOptionFileblockSizeContext *ctx) override;
  void exitTsOptionEncryption(
      parsers::MySQLParser::TsOptionEncryptionContext *ctx) override;
};

class Event_listener : public Object_listener {
 public:
  Event_listener(antlr4::tree::ParseTree *tree, shcore::Dictionary_t anObject,
                 bool no_backslash_escapes = false);

  void exitDefinerClause(
      parsers::MySQLParser::DefinerClauseContext *ctx) override;
  void exitCreateEvent(parsers::MySQLParser::CreateEventContext *ctx) override;
  void exitSchedule(parsers::MySQLParser::ScheduleContext *ctx) override;
};

shcore::Array_t ensure_array(shcore::Dictionary_t dict,
                             const std::string &field);

shcore::Dictionary_t find_named_object_in_list(shcore::Array_t array,
                                               const std::string &name,
                                               bool caseSensitive = true);

namespace internal {

inline std::string read_routine_language(
    parsers::MySQLParser::RoutineOptionContext *ctx) {
  if (ctx->SQL_SYMBOL()) return "SQL";

  return Identifier_listener(ctx->identifier()).parts.back();
}

}  // namespace internal

}  // namespace parser
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_PARSER_DDL_CREATE_LISTENERS_H_
