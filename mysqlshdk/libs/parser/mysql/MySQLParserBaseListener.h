// clang-format off
/*
 * Copyright (c) 2020, 2023, Oracle and/or its affiliates.
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




// Generated from /home/paandrus/dev/ngshell/mysqlshdk/libs/parser/grammars/MySQLParser.g4 by ANTLR 4.10.1

#pragma once


#include "antlr4-runtime.h"
#include "MySQLParserListener.h"


namespace parsers {

/**
 * This class provides an empty implementation of MySQLParserListener,
 * which can be extended to create a listener which only needs to handle a subset
 * of the available methods.
 */
class  MySQLParserBaseListener : public MySQLParserListener {
public:

  virtual void enterQuery(MySQLParser::QueryContext * /*ctx*/) override { }
  virtual void exitQuery(MySQLParser::QueryContext * /*ctx*/) override { }

  virtual void enterSimpleStatement(MySQLParser::SimpleStatementContext * /*ctx*/) override { }
  virtual void exitSimpleStatement(MySQLParser::SimpleStatementContext * /*ctx*/) override { }

  virtual void enterAlterStatement(MySQLParser::AlterStatementContext * /*ctx*/) override { }
  virtual void exitAlterStatement(MySQLParser::AlterStatementContext * /*ctx*/) override { }

  virtual void enterAlterDatabase(MySQLParser::AlterDatabaseContext * /*ctx*/) override { }
  virtual void exitAlterDatabase(MySQLParser::AlterDatabaseContext * /*ctx*/) override { }

  virtual void enterAlterDatabaseOption(MySQLParser::AlterDatabaseOptionContext * /*ctx*/) override { }
  virtual void exitAlterDatabaseOption(MySQLParser::AlterDatabaseOptionContext * /*ctx*/) override { }

  virtual void enterAlterEvent(MySQLParser::AlterEventContext * /*ctx*/) override { }
  virtual void exitAlterEvent(MySQLParser::AlterEventContext * /*ctx*/) override { }

  virtual void enterAlterLogfileGroup(MySQLParser::AlterLogfileGroupContext * /*ctx*/) override { }
  virtual void exitAlterLogfileGroup(MySQLParser::AlterLogfileGroupContext * /*ctx*/) override { }

  virtual void enterAlterLogfileGroupOptions(MySQLParser::AlterLogfileGroupOptionsContext * /*ctx*/) override { }
  virtual void exitAlterLogfileGroupOptions(MySQLParser::AlterLogfileGroupOptionsContext * /*ctx*/) override { }

  virtual void enterAlterLogfileGroupOption(MySQLParser::AlterLogfileGroupOptionContext * /*ctx*/) override { }
  virtual void exitAlterLogfileGroupOption(MySQLParser::AlterLogfileGroupOptionContext * /*ctx*/) override { }

  virtual void enterAlterServer(MySQLParser::AlterServerContext * /*ctx*/) override { }
  virtual void exitAlterServer(MySQLParser::AlterServerContext * /*ctx*/) override { }

  virtual void enterAlterTable(MySQLParser::AlterTableContext * /*ctx*/) override { }
  virtual void exitAlterTable(MySQLParser::AlterTableContext * /*ctx*/) override { }

  virtual void enterAlterTableActions(MySQLParser::AlterTableActionsContext * /*ctx*/) override { }
  virtual void exitAlterTableActions(MySQLParser::AlterTableActionsContext * /*ctx*/) override { }

  virtual void enterAlterCommandList(MySQLParser::AlterCommandListContext * /*ctx*/) override { }
  virtual void exitAlterCommandList(MySQLParser::AlterCommandListContext * /*ctx*/) override { }

  virtual void enterAlterCommandsModifierList(MySQLParser::AlterCommandsModifierListContext * /*ctx*/) override { }
  virtual void exitAlterCommandsModifierList(MySQLParser::AlterCommandsModifierListContext * /*ctx*/) override { }

  virtual void enterStandaloneAlterCommands(MySQLParser::StandaloneAlterCommandsContext * /*ctx*/) override { }
  virtual void exitStandaloneAlterCommands(MySQLParser::StandaloneAlterCommandsContext * /*ctx*/) override { }

  virtual void enterAlterPartition(MySQLParser::AlterPartitionContext * /*ctx*/) override { }
  virtual void exitAlterPartition(MySQLParser::AlterPartitionContext * /*ctx*/) override { }

  virtual void enterAlterList(MySQLParser::AlterListContext * /*ctx*/) override { }
  virtual void exitAlterList(MySQLParser::AlterListContext * /*ctx*/) override { }

  virtual void enterAlterCommandsModifier(MySQLParser::AlterCommandsModifierContext * /*ctx*/) override { }
  virtual void exitAlterCommandsModifier(MySQLParser::AlterCommandsModifierContext * /*ctx*/) override { }

  virtual void enterAlterListItem(MySQLParser::AlterListItemContext * /*ctx*/) override { }
  virtual void exitAlterListItem(MySQLParser::AlterListItemContext * /*ctx*/) override { }

  virtual void enterPlace(MySQLParser::PlaceContext * /*ctx*/) override { }
  virtual void exitPlace(MySQLParser::PlaceContext * /*ctx*/) override { }

  virtual void enterRestrict(MySQLParser::RestrictContext * /*ctx*/) override { }
  virtual void exitRestrict(MySQLParser::RestrictContext * /*ctx*/) override { }

  virtual void enterAlterOrderList(MySQLParser::AlterOrderListContext * /*ctx*/) override { }
  virtual void exitAlterOrderList(MySQLParser::AlterOrderListContext * /*ctx*/) override { }

  virtual void enterAlterAlgorithmOption(MySQLParser::AlterAlgorithmOptionContext * /*ctx*/) override { }
  virtual void exitAlterAlgorithmOption(MySQLParser::AlterAlgorithmOptionContext * /*ctx*/) override { }

  virtual void enterAlterLockOption(MySQLParser::AlterLockOptionContext * /*ctx*/) override { }
  virtual void exitAlterLockOption(MySQLParser::AlterLockOptionContext * /*ctx*/) override { }

  virtual void enterIndexLockAndAlgorithm(MySQLParser::IndexLockAndAlgorithmContext * /*ctx*/) override { }
  virtual void exitIndexLockAndAlgorithm(MySQLParser::IndexLockAndAlgorithmContext * /*ctx*/) override { }

  virtual void enterWithValidation(MySQLParser::WithValidationContext * /*ctx*/) override { }
  virtual void exitWithValidation(MySQLParser::WithValidationContext * /*ctx*/) override { }

  virtual void enterRemovePartitioning(MySQLParser::RemovePartitioningContext * /*ctx*/) override { }
  virtual void exitRemovePartitioning(MySQLParser::RemovePartitioningContext * /*ctx*/) override { }

  virtual void enterAllOrPartitionNameList(MySQLParser::AllOrPartitionNameListContext * /*ctx*/) override { }
  virtual void exitAllOrPartitionNameList(MySQLParser::AllOrPartitionNameListContext * /*ctx*/) override { }

  virtual void enterAlterTablespace(MySQLParser::AlterTablespaceContext * /*ctx*/) override { }
  virtual void exitAlterTablespace(MySQLParser::AlterTablespaceContext * /*ctx*/) override { }

  virtual void enterAlterUndoTablespace(MySQLParser::AlterUndoTablespaceContext * /*ctx*/) override { }
  virtual void exitAlterUndoTablespace(MySQLParser::AlterUndoTablespaceContext * /*ctx*/) override { }

  virtual void enterUndoTableSpaceOptions(MySQLParser::UndoTableSpaceOptionsContext * /*ctx*/) override { }
  virtual void exitUndoTableSpaceOptions(MySQLParser::UndoTableSpaceOptionsContext * /*ctx*/) override { }

  virtual void enterUndoTableSpaceOption(MySQLParser::UndoTableSpaceOptionContext * /*ctx*/) override { }
  virtual void exitUndoTableSpaceOption(MySQLParser::UndoTableSpaceOptionContext * /*ctx*/) override { }

  virtual void enterAlterTablespaceOptions(MySQLParser::AlterTablespaceOptionsContext * /*ctx*/) override { }
  virtual void exitAlterTablespaceOptions(MySQLParser::AlterTablespaceOptionsContext * /*ctx*/) override { }

  virtual void enterAlterTablespaceOption(MySQLParser::AlterTablespaceOptionContext * /*ctx*/) override { }
  virtual void exitAlterTablespaceOption(MySQLParser::AlterTablespaceOptionContext * /*ctx*/) override { }

  virtual void enterChangeTablespaceOption(MySQLParser::ChangeTablespaceOptionContext * /*ctx*/) override { }
  virtual void exitChangeTablespaceOption(MySQLParser::ChangeTablespaceOptionContext * /*ctx*/) override { }

  virtual void enterAlterView(MySQLParser::AlterViewContext * /*ctx*/) override { }
  virtual void exitAlterView(MySQLParser::AlterViewContext * /*ctx*/) override { }

  virtual void enterViewTail(MySQLParser::ViewTailContext * /*ctx*/) override { }
  virtual void exitViewTail(MySQLParser::ViewTailContext * /*ctx*/) override { }

  virtual void enterViewQueryBlock(MySQLParser::ViewQueryBlockContext * /*ctx*/) override { }
  virtual void exitViewQueryBlock(MySQLParser::ViewQueryBlockContext * /*ctx*/) override { }

  virtual void enterViewCheckOption(MySQLParser::ViewCheckOptionContext * /*ctx*/) override { }
  virtual void exitViewCheckOption(MySQLParser::ViewCheckOptionContext * /*ctx*/) override { }

  virtual void enterAlterInstanceStatement(MySQLParser::AlterInstanceStatementContext * /*ctx*/) override { }
  virtual void exitAlterInstanceStatement(MySQLParser::AlterInstanceStatementContext * /*ctx*/) override { }

  virtual void enterCreateStatement(MySQLParser::CreateStatementContext * /*ctx*/) override { }
  virtual void exitCreateStatement(MySQLParser::CreateStatementContext * /*ctx*/) override { }

  virtual void enterCreateDatabase(MySQLParser::CreateDatabaseContext * /*ctx*/) override { }
  virtual void exitCreateDatabase(MySQLParser::CreateDatabaseContext * /*ctx*/) override { }

  virtual void enterCreateDatabaseOption(MySQLParser::CreateDatabaseOptionContext * /*ctx*/) override { }
  virtual void exitCreateDatabaseOption(MySQLParser::CreateDatabaseOptionContext * /*ctx*/) override { }

  virtual void enterCreateTable(MySQLParser::CreateTableContext * /*ctx*/) override { }
  virtual void exitCreateTable(MySQLParser::CreateTableContext * /*ctx*/) override { }

  virtual void enterTableElementList(MySQLParser::TableElementListContext * /*ctx*/) override { }
  virtual void exitTableElementList(MySQLParser::TableElementListContext * /*ctx*/) override { }

  virtual void enterTableElement(MySQLParser::TableElementContext * /*ctx*/) override { }
  virtual void exitTableElement(MySQLParser::TableElementContext * /*ctx*/) override { }

  virtual void enterDuplicateAsQe(MySQLParser::DuplicateAsQeContext * /*ctx*/) override { }
  virtual void exitDuplicateAsQe(MySQLParser::DuplicateAsQeContext * /*ctx*/) override { }

  virtual void enterAsCreateQueryExpression(MySQLParser::AsCreateQueryExpressionContext * /*ctx*/) override { }
  virtual void exitAsCreateQueryExpression(MySQLParser::AsCreateQueryExpressionContext * /*ctx*/) override { }

  virtual void enterQueryExpressionOrParens(MySQLParser::QueryExpressionOrParensContext * /*ctx*/) override { }
  virtual void exitQueryExpressionOrParens(MySQLParser::QueryExpressionOrParensContext * /*ctx*/) override { }

  virtual void enterQueryExpressionWithOptLockingClauses(MySQLParser::QueryExpressionWithOptLockingClausesContext * /*ctx*/) override { }
  virtual void exitQueryExpressionWithOptLockingClauses(MySQLParser::QueryExpressionWithOptLockingClausesContext * /*ctx*/) override { }

  virtual void enterCreateRoutine(MySQLParser::CreateRoutineContext * /*ctx*/) override { }
  virtual void exitCreateRoutine(MySQLParser::CreateRoutineContext * /*ctx*/) override { }

  virtual void enterCreateProcedure(MySQLParser::CreateProcedureContext * /*ctx*/) override { }
  virtual void exitCreateProcedure(MySQLParser::CreateProcedureContext * /*ctx*/) override { }

  virtual void enterRoutineString(MySQLParser::RoutineStringContext * /*ctx*/) override { }
  virtual void exitRoutineString(MySQLParser::RoutineStringContext * /*ctx*/) override { }

  virtual void enterStoredRoutineBody(MySQLParser::StoredRoutineBodyContext * /*ctx*/) override { }
  virtual void exitStoredRoutineBody(MySQLParser::StoredRoutineBodyContext * /*ctx*/) override { }

  virtual void enterCreateFunction(MySQLParser::CreateFunctionContext * /*ctx*/) override { }
  virtual void exitCreateFunction(MySQLParser::CreateFunctionContext * /*ctx*/) override { }

  virtual void enterCreateUdf(MySQLParser::CreateUdfContext * /*ctx*/) override { }
  virtual void exitCreateUdf(MySQLParser::CreateUdfContext * /*ctx*/) override { }

  virtual void enterRoutineCreateOption(MySQLParser::RoutineCreateOptionContext * /*ctx*/) override { }
  virtual void exitRoutineCreateOption(MySQLParser::RoutineCreateOptionContext * /*ctx*/) override { }

  virtual void enterRoutineAlterOptions(MySQLParser::RoutineAlterOptionsContext * /*ctx*/) override { }
  virtual void exitRoutineAlterOptions(MySQLParser::RoutineAlterOptionsContext * /*ctx*/) override { }

  virtual void enterRoutineOption(MySQLParser::RoutineOptionContext * /*ctx*/) override { }
  virtual void exitRoutineOption(MySQLParser::RoutineOptionContext * /*ctx*/) override { }

  virtual void enterCreateIndex(MySQLParser::CreateIndexContext * /*ctx*/) override { }
  virtual void exitCreateIndex(MySQLParser::CreateIndexContext * /*ctx*/) override { }

  virtual void enterIndexNameAndType(MySQLParser::IndexNameAndTypeContext * /*ctx*/) override { }
  virtual void exitIndexNameAndType(MySQLParser::IndexNameAndTypeContext * /*ctx*/) override { }

  virtual void enterCreateIndexTarget(MySQLParser::CreateIndexTargetContext * /*ctx*/) override { }
  virtual void exitCreateIndexTarget(MySQLParser::CreateIndexTargetContext * /*ctx*/) override { }

  virtual void enterCreateLogfileGroup(MySQLParser::CreateLogfileGroupContext * /*ctx*/) override { }
  virtual void exitCreateLogfileGroup(MySQLParser::CreateLogfileGroupContext * /*ctx*/) override { }

  virtual void enterLogfileGroupOptions(MySQLParser::LogfileGroupOptionsContext * /*ctx*/) override { }
  virtual void exitLogfileGroupOptions(MySQLParser::LogfileGroupOptionsContext * /*ctx*/) override { }

  virtual void enterLogfileGroupOption(MySQLParser::LogfileGroupOptionContext * /*ctx*/) override { }
  virtual void exitLogfileGroupOption(MySQLParser::LogfileGroupOptionContext * /*ctx*/) override { }

  virtual void enterCreateServer(MySQLParser::CreateServerContext * /*ctx*/) override { }
  virtual void exitCreateServer(MySQLParser::CreateServerContext * /*ctx*/) override { }

  virtual void enterServerOptions(MySQLParser::ServerOptionsContext * /*ctx*/) override { }
  virtual void exitServerOptions(MySQLParser::ServerOptionsContext * /*ctx*/) override { }

  virtual void enterServerOption(MySQLParser::ServerOptionContext * /*ctx*/) override { }
  virtual void exitServerOption(MySQLParser::ServerOptionContext * /*ctx*/) override { }

  virtual void enterCreateTablespace(MySQLParser::CreateTablespaceContext * /*ctx*/) override { }
  virtual void exitCreateTablespace(MySQLParser::CreateTablespaceContext * /*ctx*/) override { }

  virtual void enterCreateUndoTablespace(MySQLParser::CreateUndoTablespaceContext * /*ctx*/) override { }
  virtual void exitCreateUndoTablespace(MySQLParser::CreateUndoTablespaceContext * /*ctx*/) override { }

  virtual void enterTsDataFileName(MySQLParser::TsDataFileNameContext * /*ctx*/) override { }
  virtual void exitTsDataFileName(MySQLParser::TsDataFileNameContext * /*ctx*/) override { }

  virtual void enterTsDataFile(MySQLParser::TsDataFileContext * /*ctx*/) override { }
  virtual void exitTsDataFile(MySQLParser::TsDataFileContext * /*ctx*/) override { }

  virtual void enterTablespaceOptions(MySQLParser::TablespaceOptionsContext * /*ctx*/) override { }
  virtual void exitTablespaceOptions(MySQLParser::TablespaceOptionsContext * /*ctx*/) override { }

  virtual void enterTablespaceOption(MySQLParser::TablespaceOptionContext * /*ctx*/) override { }
  virtual void exitTablespaceOption(MySQLParser::TablespaceOptionContext * /*ctx*/) override { }

  virtual void enterTsOptionInitialSize(MySQLParser::TsOptionInitialSizeContext * /*ctx*/) override { }
  virtual void exitTsOptionInitialSize(MySQLParser::TsOptionInitialSizeContext * /*ctx*/) override { }

  virtual void enterTsOptionUndoRedoBufferSize(MySQLParser::TsOptionUndoRedoBufferSizeContext * /*ctx*/) override { }
  virtual void exitTsOptionUndoRedoBufferSize(MySQLParser::TsOptionUndoRedoBufferSizeContext * /*ctx*/) override { }

  virtual void enterTsOptionAutoextendSize(MySQLParser::TsOptionAutoextendSizeContext * /*ctx*/) override { }
  virtual void exitTsOptionAutoextendSize(MySQLParser::TsOptionAutoextendSizeContext * /*ctx*/) override { }

  virtual void enterTsOptionMaxSize(MySQLParser::TsOptionMaxSizeContext * /*ctx*/) override { }
  virtual void exitTsOptionMaxSize(MySQLParser::TsOptionMaxSizeContext * /*ctx*/) override { }

  virtual void enterTsOptionExtentSize(MySQLParser::TsOptionExtentSizeContext * /*ctx*/) override { }
  virtual void exitTsOptionExtentSize(MySQLParser::TsOptionExtentSizeContext * /*ctx*/) override { }

  virtual void enterTsOptionNodegroup(MySQLParser::TsOptionNodegroupContext * /*ctx*/) override { }
  virtual void exitTsOptionNodegroup(MySQLParser::TsOptionNodegroupContext * /*ctx*/) override { }

  virtual void enterTsOptionEngine(MySQLParser::TsOptionEngineContext * /*ctx*/) override { }
  virtual void exitTsOptionEngine(MySQLParser::TsOptionEngineContext * /*ctx*/) override { }

  virtual void enterTsOptionWait(MySQLParser::TsOptionWaitContext * /*ctx*/) override { }
  virtual void exitTsOptionWait(MySQLParser::TsOptionWaitContext * /*ctx*/) override { }

  virtual void enterTsOptionComment(MySQLParser::TsOptionCommentContext * /*ctx*/) override { }
  virtual void exitTsOptionComment(MySQLParser::TsOptionCommentContext * /*ctx*/) override { }

  virtual void enterTsOptionFileblockSize(MySQLParser::TsOptionFileblockSizeContext * /*ctx*/) override { }
  virtual void exitTsOptionFileblockSize(MySQLParser::TsOptionFileblockSizeContext * /*ctx*/) override { }

  virtual void enterTsOptionEncryption(MySQLParser::TsOptionEncryptionContext * /*ctx*/) override { }
  virtual void exitTsOptionEncryption(MySQLParser::TsOptionEncryptionContext * /*ctx*/) override { }

  virtual void enterTsOptionEngineAttribute(MySQLParser::TsOptionEngineAttributeContext * /*ctx*/) override { }
  virtual void exitTsOptionEngineAttribute(MySQLParser::TsOptionEngineAttributeContext * /*ctx*/) override { }

  virtual void enterCreateView(MySQLParser::CreateViewContext * /*ctx*/) override { }
  virtual void exitCreateView(MySQLParser::CreateViewContext * /*ctx*/) override { }

  virtual void enterViewReplaceOrAlgorithm(MySQLParser::ViewReplaceOrAlgorithmContext * /*ctx*/) override { }
  virtual void exitViewReplaceOrAlgorithm(MySQLParser::ViewReplaceOrAlgorithmContext * /*ctx*/) override { }

  virtual void enterViewAlgorithm(MySQLParser::ViewAlgorithmContext * /*ctx*/) override { }
  virtual void exitViewAlgorithm(MySQLParser::ViewAlgorithmContext * /*ctx*/) override { }

  virtual void enterViewSuid(MySQLParser::ViewSuidContext * /*ctx*/) override { }
  virtual void exitViewSuid(MySQLParser::ViewSuidContext * /*ctx*/) override { }

  virtual void enterCreateTrigger(MySQLParser::CreateTriggerContext * /*ctx*/) override { }
  virtual void exitCreateTrigger(MySQLParser::CreateTriggerContext * /*ctx*/) override { }

  virtual void enterTriggerFollowsPrecedesClause(MySQLParser::TriggerFollowsPrecedesClauseContext * /*ctx*/) override { }
  virtual void exitTriggerFollowsPrecedesClause(MySQLParser::TriggerFollowsPrecedesClauseContext * /*ctx*/) override { }

  virtual void enterCreateEvent(MySQLParser::CreateEventContext * /*ctx*/) override { }
  virtual void exitCreateEvent(MySQLParser::CreateEventContext * /*ctx*/) override { }

  virtual void enterCreateRole(MySQLParser::CreateRoleContext * /*ctx*/) override { }
  virtual void exitCreateRole(MySQLParser::CreateRoleContext * /*ctx*/) override { }

  virtual void enterCreateSpatialReference(MySQLParser::CreateSpatialReferenceContext * /*ctx*/) override { }
  virtual void exitCreateSpatialReference(MySQLParser::CreateSpatialReferenceContext * /*ctx*/) override { }

  virtual void enterSrsAttribute(MySQLParser::SrsAttributeContext * /*ctx*/) override { }
  virtual void exitSrsAttribute(MySQLParser::SrsAttributeContext * /*ctx*/) override { }

  virtual void enterDropStatement(MySQLParser::DropStatementContext * /*ctx*/) override { }
  virtual void exitDropStatement(MySQLParser::DropStatementContext * /*ctx*/) override { }

  virtual void enterDropDatabase(MySQLParser::DropDatabaseContext * /*ctx*/) override { }
  virtual void exitDropDatabase(MySQLParser::DropDatabaseContext * /*ctx*/) override { }

  virtual void enterDropEvent(MySQLParser::DropEventContext * /*ctx*/) override { }
  virtual void exitDropEvent(MySQLParser::DropEventContext * /*ctx*/) override { }

  virtual void enterDropFunction(MySQLParser::DropFunctionContext * /*ctx*/) override { }
  virtual void exitDropFunction(MySQLParser::DropFunctionContext * /*ctx*/) override { }

  virtual void enterDropProcedure(MySQLParser::DropProcedureContext * /*ctx*/) override { }
  virtual void exitDropProcedure(MySQLParser::DropProcedureContext * /*ctx*/) override { }

  virtual void enterDropIndex(MySQLParser::DropIndexContext * /*ctx*/) override { }
  virtual void exitDropIndex(MySQLParser::DropIndexContext * /*ctx*/) override { }

  virtual void enterDropLogfileGroup(MySQLParser::DropLogfileGroupContext * /*ctx*/) override { }
  virtual void exitDropLogfileGroup(MySQLParser::DropLogfileGroupContext * /*ctx*/) override { }

  virtual void enterDropLogfileGroupOption(MySQLParser::DropLogfileGroupOptionContext * /*ctx*/) override { }
  virtual void exitDropLogfileGroupOption(MySQLParser::DropLogfileGroupOptionContext * /*ctx*/) override { }

  virtual void enterDropServer(MySQLParser::DropServerContext * /*ctx*/) override { }
  virtual void exitDropServer(MySQLParser::DropServerContext * /*ctx*/) override { }

  virtual void enterDropTable(MySQLParser::DropTableContext * /*ctx*/) override { }
  virtual void exitDropTable(MySQLParser::DropTableContext * /*ctx*/) override { }

  virtual void enterDropTableSpace(MySQLParser::DropTableSpaceContext * /*ctx*/) override { }
  virtual void exitDropTableSpace(MySQLParser::DropTableSpaceContext * /*ctx*/) override { }

  virtual void enterDropTrigger(MySQLParser::DropTriggerContext * /*ctx*/) override { }
  virtual void exitDropTrigger(MySQLParser::DropTriggerContext * /*ctx*/) override { }

  virtual void enterDropView(MySQLParser::DropViewContext * /*ctx*/) override { }
  virtual void exitDropView(MySQLParser::DropViewContext * /*ctx*/) override { }

  virtual void enterDropRole(MySQLParser::DropRoleContext * /*ctx*/) override { }
  virtual void exitDropRole(MySQLParser::DropRoleContext * /*ctx*/) override { }

  virtual void enterDropSpatialReference(MySQLParser::DropSpatialReferenceContext * /*ctx*/) override { }
  virtual void exitDropSpatialReference(MySQLParser::DropSpatialReferenceContext * /*ctx*/) override { }

  virtual void enterDropUndoTablespace(MySQLParser::DropUndoTablespaceContext * /*ctx*/) override { }
  virtual void exitDropUndoTablespace(MySQLParser::DropUndoTablespaceContext * /*ctx*/) override { }

  virtual void enterRenameTableStatement(MySQLParser::RenameTableStatementContext * /*ctx*/) override { }
  virtual void exitRenameTableStatement(MySQLParser::RenameTableStatementContext * /*ctx*/) override { }

  virtual void enterRenamePair(MySQLParser::RenamePairContext * /*ctx*/) override { }
  virtual void exitRenamePair(MySQLParser::RenamePairContext * /*ctx*/) override { }

  virtual void enterTruncateTableStatement(MySQLParser::TruncateTableStatementContext * /*ctx*/) override { }
  virtual void exitTruncateTableStatement(MySQLParser::TruncateTableStatementContext * /*ctx*/) override { }

  virtual void enterImportStatement(MySQLParser::ImportStatementContext * /*ctx*/) override { }
  virtual void exitImportStatement(MySQLParser::ImportStatementContext * /*ctx*/) override { }

  virtual void enterCallStatement(MySQLParser::CallStatementContext * /*ctx*/) override { }
  virtual void exitCallStatement(MySQLParser::CallStatementContext * /*ctx*/) override { }

  virtual void enterDeleteStatement(MySQLParser::DeleteStatementContext * /*ctx*/) override { }
  virtual void exitDeleteStatement(MySQLParser::DeleteStatementContext * /*ctx*/) override { }

  virtual void enterPartitionDelete(MySQLParser::PartitionDeleteContext * /*ctx*/) override { }
  virtual void exitPartitionDelete(MySQLParser::PartitionDeleteContext * /*ctx*/) override { }

  virtual void enterDeleteStatementOption(MySQLParser::DeleteStatementOptionContext * /*ctx*/) override { }
  virtual void exitDeleteStatementOption(MySQLParser::DeleteStatementOptionContext * /*ctx*/) override { }

  virtual void enterDoStatement(MySQLParser::DoStatementContext * /*ctx*/) override { }
  virtual void exitDoStatement(MySQLParser::DoStatementContext * /*ctx*/) override { }

  virtual void enterHandlerStatement(MySQLParser::HandlerStatementContext * /*ctx*/) override { }
  virtual void exitHandlerStatement(MySQLParser::HandlerStatementContext * /*ctx*/) override { }

  virtual void enterHandlerReadOrScan(MySQLParser::HandlerReadOrScanContext * /*ctx*/) override { }
  virtual void exitHandlerReadOrScan(MySQLParser::HandlerReadOrScanContext * /*ctx*/) override { }

  virtual void enterInsertStatement(MySQLParser::InsertStatementContext * /*ctx*/) override { }
  virtual void exitInsertStatement(MySQLParser::InsertStatementContext * /*ctx*/) override { }

  virtual void enterInsertLockOption(MySQLParser::InsertLockOptionContext * /*ctx*/) override { }
  virtual void exitInsertLockOption(MySQLParser::InsertLockOptionContext * /*ctx*/) override { }

  virtual void enterInsertFromConstructor(MySQLParser::InsertFromConstructorContext * /*ctx*/) override { }
  virtual void exitInsertFromConstructor(MySQLParser::InsertFromConstructorContext * /*ctx*/) override { }

  virtual void enterFields(MySQLParser::FieldsContext * /*ctx*/) override { }
  virtual void exitFields(MySQLParser::FieldsContext * /*ctx*/) override { }

  virtual void enterInsertValues(MySQLParser::InsertValuesContext * /*ctx*/) override { }
  virtual void exitInsertValues(MySQLParser::InsertValuesContext * /*ctx*/) override { }

  virtual void enterInsertQueryExpression(MySQLParser::InsertQueryExpressionContext * /*ctx*/) override { }
  virtual void exitInsertQueryExpression(MySQLParser::InsertQueryExpressionContext * /*ctx*/) override { }

  virtual void enterValueList(MySQLParser::ValueListContext * /*ctx*/) override { }
  virtual void exitValueList(MySQLParser::ValueListContext * /*ctx*/) override { }

  virtual void enterValues(MySQLParser::ValuesContext * /*ctx*/) override { }
  virtual void exitValues(MySQLParser::ValuesContext * /*ctx*/) override { }

  virtual void enterValuesReference(MySQLParser::ValuesReferenceContext * /*ctx*/) override { }
  virtual void exitValuesReference(MySQLParser::ValuesReferenceContext * /*ctx*/) override { }

  virtual void enterInsertUpdateList(MySQLParser::InsertUpdateListContext * /*ctx*/) override { }
  virtual void exitInsertUpdateList(MySQLParser::InsertUpdateListContext * /*ctx*/) override { }

  virtual void enterLoadStatement(MySQLParser::LoadStatementContext * /*ctx*/) override { }
  virtual void exitLoadStatement(MySQLParser::LoadStatementContext * /*ctx*/) override { }

  virtual void enterDataOrXml(MySQLParser::DataOrXmlContext * /*ctx*/) override { }
  virtual void exitDataOrXml(MySQLParser::DataOrXmlContext * /*ctx*/) override { }

  virtual void enterLoadDataLock(MySQLParser::LoadDataLockContext * /*ctx*/) override { }
  virtual void exitLoadDataLock(MySQLParser::LoadDataLockContext * /*ctx*/) override { }

  virtual void enterLoadFrom(MySQLParser::LoadFromContext * /*ctx*/) override { }
  virtual void exitLoadFrom(MySQLParser::LoadFromContext * /*ctx*/) override { }

  virtual void enterLoadSourceType(MySQLParser::LoadSourceTypeContext * /*ctx*/) override { }
  virtual void exitLoadSourceType(MySQLParser::LoadSourceTypeContext * /*ctx*/) override { }

  virtual void enterSourceCount(MySQLParser::SourceCountContext * /*ctx*/) override { }
  virtual void exitSourceCount(MySQLParser::SourceCountContext * /*ctx*/) override { }

  virtual void enterSourceOrder(MySQLParser::SourceOrderContext * /*ctx*/) override { }
  virtual void exitSourceOrder(MySQLParser::SourceOrderContext * /*ctx*/) override { }

  virtual void enterXmlRowsIdentifiedBy(MySQLParser::XmlRowsIdentifiedByContext * /*ctx*/) override { }
  virtual void exitXmlRowsIdentifiedBy(MySQLParser::XmlRowsIdentifiedByContext * /*ctx*/) override { }

  virtual void enterLoadDataFileTail(MySQLParser::LoadDataFileTailContext * /*ctx*/) override { }
  virtual void exitLoadDataFileTail(MySQLParser::LoadDataFileTailContext * /*ctx*/) override { }

  virtual void enterLoadDataFileTargetList(MySQLParser::LoadDataFileTargetListContext * /*ctx*/) override { }
  virtual void exitLoadDataFileTargetList(MySQLParser::LoadDataFileTargetListContext * /*ctx*/) override { }

  virtual void enterFieldOrVariableList(MySQLParser::FieldOrVariableListContext * /*ctx*/) override { }
  virtual void exitFieldOrVariableList(MySQLParser::FieldOrVariableListContext * /*ctx*/) override { }

  virtual void enterLoadAlgorithm(MySQLParser::LoadAlgorithmContext * /*ctx*/) override { }
  virtual void exitLoadAlgorithm(MySQLParser::LoadAlgorithmContext * /*ctx*/) override { }

  virtual void enterLoadParallel(MySQLParser::LoadParallelContext * /*ctx*/) override { }
  virtual void exitLoadParallel(MySQLParser::LoadParallelContext * /*ctx*/) override { }

  virtual void enterLoadMemory(MySQLParser::LoadMemoryContext * /*ctx*/) override { }
  virtual void exitLoadMemory(MySQLParser::LoadMemoryContext * /*ctx*/) override { }

  virtual void enterReplaceStatement(MySQLParser::ReplaceStatementContext * /*ctx*/) override { }
  virtual void exitReplaceStatement(MySQLParser::ReplaceStatementContext * /*ctx*/) override { }

  virtual void enterSelectStatement(MySQLParser::SelectStatementContext * /*ctx*/) override { }
  virtual void exitSelectStatement(MySQLParser::SelectStatementContext * /*ctx*/) override { }

  virtual void enterSelectStatementWithInto(MySQLParser::SelectStatementWithIntoContext * /*ctx*/) override { }
  virtual void exitSelectStatementWithInto(MySQLParser::SelectStatementWithIntoContext * /*ctx*/) override { }

  virtual void enterQueryExpression(MySQLParser::QueryExpressionContext * /*ctx*/) override { }
  virtual void exitQueryExpression(MySQLParser::QueryExpressionContext * /*ctx*/) override { }

  virtual void enterQueryExpressionBody(MySQLParser::QueryExpressionBodyContext * /*ctx*/) override { }
  virtual void exitQueryExpressionBody(MySQLParser::QueryExpressionBodyContext * /*ctx*/) override { }

  virtual void enterQueryExpressionParens(MySQLParser::QueryExpressionParensContext * /*ctx*/) override { }
  virtual void exitQueryExpressionParens(MySQLParser::QueryExpressionParensContext * /*ctx*/) override { }

  virtual void enterQueryPrimary(MySQLParser::QueryPrimaryContext * /*ctx*/) override { }
  virtual void exitQueryPrimary(MySQLParser::QueryPrimaryContext * /*ctx*/) override { }

  virtual void enterQuerySpecification(MySQLParser::QuerySpecificationContext * /*ctx*/) override { }
  virtual void exitQuerySpecification(MySQLParser::QuerySpecificationContext * /*ctx*/) override { }

  virtual void enterSubquery(MySQLParser::SubqueryContext * /*ctx*/) override { }
  virtual void exitSubquery(MySQLParser::SubqueryContext * /*ctx*/) override { }

  virtual void enterQuerySpecOption(MySQLParser::QuerySpecOptionContext * /*ctx*/) override { }
  virtual void exitQuerySpecOption(MySQLParser::QuerySpecOptionContext * /*ctx*/) override { }

  virtual void enterLimitClause(MySQLParser::LimitClauseContext * /*ctx*/) override { }
  virtual void exitLimitClause(MySQLParser::LimitClauseContext * /*ctx*/) override { }

  virtual void enterSimpleLimitClause(MySQLParser::SimpleLimitClauseContext * /*ctx*/) override { }
  virtual void exitSimpleLimitClause(MySQLParser::SimpleLimitClauseContext * /*ctx*/) override { }

  virtual void enterLimitOptions(MySQLParser::LimitOptionsContext * /*ctx*/) override { }
  virtual void exitLimitOptions(MySQLParser::LimitOptionsContext * /*ctx*/) override { }

  virtual void enterLimitOption(MySQLParser::LimitOptionContext * /*ctx*/) override { }
  virtual void exitLimitOption(MySQLParser::LimitOptionContext * /*ctx*/) override { }

  virtual void enterIntoClause(MySQLParser::IntoClauseContext * /*ctx*/) override { }
  virtual void exitIntoClause(MySQLParser::IntoClauseContext * /*ctx*/) override { }

  virtual void enterProcedureAnalyseClause(MySQLParser::ProcedureAnalyseClauseContext * /*ctx*/) override { }
  virtual void exitProcedureAnalyseClause(MySQLParser::ProcedureAnalyseClauseContext * /*ctx*/) override { }

  virtual void enterHavingClause(MySQLParser::HavingClauseContext * /*ctx*/) override { }
  virtual void exitHavingClause(MySQLParser::HavingClauseContext * /*ctx*/) override { }

  virtual void enterWindowClause(MySQLParser::WindowClauseContext * /*ctx*/) override { }
  virtual void exitWindowClause(MySQLParser::WindowClauseContext * /*ctx*/) override { }

  virtual void enterWindowDefinition(MySQLParser::WindowDefinitionContext * /*ctx*/) override { }
  virtual void exitWindowDefinition(MySQLParser::WindowDefinitionContext * /*ctx*/) override { }

  virtual void enterWindowSpec(MySQLParser::WindowSpecContext * /*ctx*/) override { }
  virtual void exitWindowSpec(MySQLParser::WindowSpecContext * /*ctx*/) override { }

  virtual void enterWindowSpecDetails(MySQLParser::WindowSpecDetailsContext * /*ctx*/) override { }
  virtual void exitWindowSpecDetails(MySQLParser::WindowSpecDetailsContext * /*ctx*/) override { }

  virtual void enterWindowFrameClause(MySQLParser::WindowFrameClauseContext * /*ctx*/) override { }
  virtual void exitWindowFrameClause(MySQLParser::WindowFrameClauseContext * /*ctx*/) override { }

  virtual void enterWindowFrameUnits(MySQLParser::WindowFrameUnitsContext * /*ctx*/) override { }
  virtual void exitWindowFrameUnits(MySQLParser::WindowFrameUnitsContext * /*ctx*/) override { }

  virtual void enterWindowFrameExtent(MySQLParser::WindowFrameExtentContext * /*ctx*/) override { }
  virtual void exitWindowFrameExtent(MySQLParser::WindowFrameExtentContext * /*ctx*/) override { }

  virtual void enterWindowFrameStart(MySQLParser::WindowFrameStartContext * /*ctx*/) override { }
  virtual void exitWindowFrameStart(MySQLParser::WindowFrameStartContext * /*ctx*/) override { }

  virtual void enterWindowFrameBetween(MySQLParser::WindowFrameBetweenContext * /*ctx*/) override { }
  virtual void exitWindowFrameBetween(MySQLParser::WindowFrameBetweenContext * /*ctx*/) override { }

  virtual void enterWindowFrameBound(MySQLParser::WindowFrameBoundContext * /*ctx*/) override { }
  virtual void exitWindowFrameBound(MySQLParser::WindowFrameBoundContext * /*ctx*/) override { }

  virtual void enterWindowFrameExclusion(MySQLParser::WindowFrameExclusionContext * /*ctx*/) override { }
  virtual void exitWindowFrameExclusion(MySQLParser::WindowFrameExclusionContext * /*ctx*/) override { }

  virtual void enterWithClause(MySQLParser::WithClauseContext * /*ctx*/) override { }
  virtual void exitWithClause(MySQLParser::WithClauseContext * /*ctx*/) override { }

  virtual void enterCommonTableExpression(MySQLParser::CommonTableExpressionContext * /*ctx*/) override { }
  virtual void exitCommonTableExpression(MySQLParser::CommonTableExpressionContext * /*ctx*/) override { }

  virtual void enterGroupByClause(MySQLParser::GroupByClauseContext * /*ctx*/) override { }
  virtual void exitGroupByClause(MySQLParser::GroupByClauseContext * /*ctx*/) override { }

  virtual void enterOlapOption(MySQLParser::OlapOptionContext * /*ctx*/) override { }
  virtual void exitOlapOption(MySQLParser::OlapOptionContext * /*ctx*/) override { }

  virtual void enterOrderClause(MySQLParser::OrderClauseContext * /*ctx*/) override { }
  virtual void exitOrderClause(MySQLParser::OrderClauseContext * /*ctx*/) override { }

  virtual void enterDirection(MySQLParser::DirectionContext * /*ctx*/) override { }
  virtual void exitDirection(MySQLParser::DirectionContext * /*ctx*/) override { }

  virtual void enterFromClause(MySQLParser::FromClauseContext * /*ctx*/) override { }
  virtual void exitFromClause(MySQLParser::FromClauseContext * /*ctx*/) override { }

  virtual void enterTableReferenceList(MySQLParser::TableReferenceListContext * /*ctx*/) override { }
  virtual void exitTableReferenceList(MySQLParser::TableReferenceListContext * /*ctx*/) override { }

  virtual void enterTableValueConstructor(MySQLParser::TableValueConstructorContext * /*ctx*/) override { }
  virtual void exitTableValueConstructor(MySQLParser::TableValueConstructorContext * /*ctx*/) override { }

  virtual void enterExplicitTable(MySQLParser::ExplicitTableContext * /*ctx*/) override { }
  virtual void exitExplicitTable(MySQLParser::ExplicitTableContext * /*ctx*/) override { }

  virtual void enterRowValueExplicit(MySQLParser::RowValueExplicitContext * /*ctx*/) override { }
  virtual void exitRowValueExplicit(MySQLParser::RowValueExplicitContext * /*ctx*/) override { }

  virtual void enterSelectOption(MySQLParser::SelectOptionContext * /*ctx*/) override { }
  virtual void exitSelectOption(MySQLParser::SelectOptionContext * /*ctx*/) override { }

  virtual void enterLockingClauseList(MySQLParser::LockingClauseListContext * /*ctx*/) override { }
  virtual void exitLockingClauseList(MySQLParser::LockingClauseListContext * /*ctx*/) override { }

  virtual void enterLockingClause(MySQLParser::LockingClauseContext * /*ctx*/) override { }
  virtual void exitLockingClause(MySQLParser::LockingClauseContext * /*ctx*/) override { }

  virtual void enterLockStrengh(MySQLParser::LockStrenghContext * /*ctx*/) override { }
  virtual void exitLockStrengh(MySQLParser::LockStrenghContext * /*ctx*/) override { }

  virtual void enterLockedRowAction(MySQLParser::LockedRowActionContext * /*ctx*/) override { }
  virtual void exitLockedRowAction(MySQLParser::LockedRowActionContext * /*ctx*/) override { }

  virtual void enterSelectItemList(MySQLParser::SelectItemListContext * /*ctx*/) override { }
  virtual void exitSelectItemList(MySQLParser::SelectItemListContext * /*ctx*/) override { }

  virtual void enterSelectItem(MySQLParser::SelectItemContext * /*ctx*/) override { }
  virtual void exitSelectItem(MySQLParser::SelectItemContext * /*ctx*/) override { }

  virtual void enterSelectAlias(MySQLParser::SelectAliasContext * /*ctx*/) override { }
  virtual void exitSelectAlias(MySQLParser::SelectAliasContext * /*ctx*/) override { }

  virtual void enterWhereClause(MySQLParser::WhereClauseContext * /*ctx*/) override { }
  virtual void exitWhereClause(MySQLParser::WhereClauseContext * /*ctx*/) override { }

  virtual void enterTableReference(MySQLParser::TableReferenceContext * /*ctx*/) override { }
  virtual void exitTableReference(MySQLParser::TableReferenceContext * /*ctx*/) override { }

  virtual void enterEscapedTableReference(MySQLParser::EscapedTableReferenceContext * /*ctx*/) override { }
  virtual void exitEscapedTableReference(MySQLParser::EscapedTableReferenceContext * /*ctx*/) override { }

  virtual void enterJoinedTable(MySQLParser::JoinedTableContext * /*ctx*/) override { }
  virtual void exitJoinedTable(MySQLParser::JoinedTableContext * /*ctx*/) override { }

  virtual void enterNaturalJoinType(MySQLParser::NaturalJoinTypeContext * /*ctx*/) override { }
  virtual void exitNaturalJoinType(MySQLParser::NaturalJoinTypeContext * /*ctx*/) override { }

  virtual void enterInnerJoinType(MySQLParser::InnerJoinTypeContext * /*ctx*/) override { }
  virtual void exitInnerJoinType(MySQLParser::InnerJoinTypeContext * /*ctx*/) override { }

  virtual void enterOuterJoinType(MySQLParser::OuterJoinTypeContext * /*ctx*/) override { }
  virtual void exitOuterJoinType(MySQLParser::OuterJoinTypeContext * /*ctx*/) override { }

  virtual void enterTableFactor(MySQLParser::TableFactorContext * /*ctx*/) override { }
  virtual void exitTableFactor(MySQLParser::TableFactorContext * /*ctx*/) override { }

  virtual void enterSingleTable(MySQLParser::SingleTableContext * /*ctx*/) override { }
  virtual void exitSingleTable(MySQLParser::SingleTableContext * /*ctx*/) override { }

  virtual void enterSingleTableParens(MySQLParser::SingleTableParensContext * /*ctx*/) override { }
  virtual void exitSingleTableParens(MySQLParser::SingleTableParensContext * /*ctx*/) override { }

  virtual void enterDerivedTable(MySQLParser::DerivedTableContext * /*ctx*/) override { }
  virtual void exitDerivedTable(MySQLParser::DerivedTableContext * /*ctx*/) override { }

  virtual void enterTableReferenceListParens(MySQLParser::TableReferenceListParensContext * /*ctx*/) override { }
  virtual void exitTableReferenceListParens(MySQLParser::TableReferenceListParensContext * /*ctx*/) override { }

  virtual void enterTableFunction(MySQLParser::TableFunctionContext * /*ctx*/) override { }
  virtual void exitTableFunction(MySQLParser::TableFunctionContext * /*ctx*/) override { }

  virtual void enterColumnsClause(MySQLParser::ColumnsClauseContext * /*ctx*/) override { }
  virtual void exitColumnsClause(MySQLParser::ColumnsClauseContext * /*ctx*/) override { }

  virtual void enterJtColumn(MySQLParser::JtColumnContext * /*ctx*/) override { }
  virtual void exitJtColumn(MySQLParser::JtColumnContext * /*ctx*/) override { }

  virtual void enterOnEmptyOrError(MySQLParser::OnEmptyOrErrorContext * /*ctx*/) override { }
  virtual void exitOnEmptyOrError(MySQLParser::OnEmptyOrErrorContext * /*ctx*/) override { }

  virtual void enterOnEmptyOrErrorJsonTable(MySQLParser::OnEmptyOrErrorJsonTableContext * /*ctx*/) override { }
  virtual void exitOnEmptyOrErrorJsonTable(MySQLParser::OnEmptyOrErrorJsonTableContext * /*ctx*/) override { }

  virtual void enterOnEmpty(MySQLParser::OnEmptyContext * /*ctx*/) override { }
  virtual void exitOnEmpty(MySQLParser::OnEmptyContext * /*ctx*/) override { }

  virtual void enterOnError(MySQLParser::OnErrorContext * /*ctx*/) override { }
  virtual void exitOnError(MySQLParser::OnErrorContext * /*ctx*/) override { }

  virtual void enterJsonOnResponse(MySQLParser::JsonOnResponseContext * /*ctx*/) override { }
  virtual void exitJsonOnResponse(MySQLParser::JsonOnResponseContext * /*ctx*/) override { }

  virtual void enterUnionOption(MySQLParser::UnionOptionContext * /*ctx*/) override { }
  virtual void exitUnionOption(MySQLParser::UnionOptionContext * /*ctx*/) override { }

  virtual void enterTableAlias(MySQLParser::TableAliasContext * /*ctx*/) override { }
  virtual void exitTableAlias(MySQLParser::TableAliasContext * /*ctx*/) override { }

  virtual void enterIndexHintList(MySQLParser::IndexHintListContext * /*ctx*/) override { }
  virtual void exitIndexHintList(MySQLParser::IndexHintListContext * /*ctx*/) override { }

  virtual void enterIndexHint(MySQLParser::IndexHintContext * /*ctx*/) override { }
  virtual void exitIndexHint(MySQLParser::IndexHintContext * /*ctx*/) override { }

  virtual void enterIndexHintType(MySQLParser::IndexHintTypeContext * /*ctx*/) override { }
  virtual void exitIndexHintType(MySQLParser::IndexHintTypeContext * /*ctx*/) override { }

  virtual void enterKeyOrIndex(MySQLParser::KeyOrIndexContext * /*ctx*/) override { }
  virtual void exitKeyOrIndex(MySQLParser::KeyOrIndexContext * /*ctx*/) override { }

  virtual void enterConstraintKeyType(MySQLParser::ConstraintKeyTypeContext * /*ctx*/) override { }
  virtual void exitConstraintKeyType(MySQLParser::ConstraintKeyTypeContext * /*ctx*/) override { }

  virtual void enterIndexHintClause(MySQLParser::IndexHintClauseContext * /*ctx*/) override { }
  virtual void exitIndexHintClause(MySQLParser::IndexHintClauseContext * /*ctx*/) override { }

  virtual void enterIndexList(MySQLParser::IndexListContext * /*ctx*/) override { }
  virtual void exitIndexList(MySQLParser::IndexListContext * /*ctx*/) override { }

  virtual void enterIndexListElement(MySQLParser::IndexListElementContext * /*ctx*/) override { }
  virtual void exitIndexListElement(MySQLParser::IndexListElementContext * /*ctx*/) override { }

  virtual void enterUpdateStatement(MySQLParser::UpdateStatementContext * /*ctx*/) override { }
  virtual void exitUpdateStatement(MySQLParser::UpdateStatementContext * /*ctx*/) override { }

  virtual void enterTransactionOrLockingStatement(MySQLParser::TransactionOrLockingStatementContext * /*ctx*/) override { }
  virtual void exitTransactionOrLockingStatement(MySQLParser::TransactionOrLockingStatementContext * /*ctx*/) override { }

  virtual void enterTransactionStatement(MySQLParser::TransactionStatementContext * /*ctx*/) override { }
  virtual void exitTransactionStatement(MySQLParser::TransactionStatementContext * /*ctx*/) override { }

  virtual void enterBeginWork(MySQLParser::BeginWorkContext * /*ctx*/) override { }
  virtual void exitBeginWork(MySQLParser::BeginWorkContext * /*ctx*/) override { }

  virtual void enterStartTransactionOptionList(MySQLParser::StartTransactionOptionListContext * /*ctx*/) override { }
  virtual void exitStartTransactionOptionList(MySQLParser::StartTransactionOptionListContext * /*ctx*/) override { }

  virtual void enterSavepointStatement(MySQLParser::SavepointStatementContext * /*ctx*/) override { }
  virtual void exitSavepointStatement(MySQLParser::SavepointStatementContext * /*ctx*/) override { }

  virtual void enterLockStatement(MySQLParser::LockStatementContext * /*ctx*/) override { }
  virtual void exitLockStatement(MySQLParser::LockStatementContext * /*ctx*/) override { }

  virtual void enterLockItem(MySQLParser::LockItemContext * /*ctx*/) override { }
  virtual void exitLockItem(MySQLParser::LockItemContext * /*ctx*/) override { }

  virtual void enterLockOption(MySQLParser::LockOptionContext * /*ctx*/) override { }
  virtual void exitLockOption(MySQLParser::LockOptionContext * /*ctx*/) override { }

  virtual void enterXaStatement(MySQLParser::XaStatementContext * /*ctx*/) override { }
  virtual void exitXaStatement(MySQLParser::XaStatementContext * /*ctx*/) override { }

  virtual void enterXaConvert(MySQLParser::XaConvertContext * /*ctx*/) override { }
  virtual void exitXaConvert(MySQLParser::XaConvertContext * /*ctx*/) override { }

  virtual void enterXid(MySQLParser::XidContext * /*ctx*/) override { }
  virtual void exitXid(MySQLParser::XidContext * /*ctx*/) override { }

  virtual void enterReplicationStatement(MySQLParser::ReplicationStatementContext * /*ctx*/) override { }
  virtual void exitReplicationStatement(MySQLParser::ReplicationStatementContext * /*ctx*/) override { }

  virtual void enterPurgeOptions(MySQLParser::PurgeOptionsContext * /*ctx*/) override { }
  virtual void exitPurgeOptions(MySQLParser::PurgeOptionsContext * /*ctx*/) override { }

  virtual void enterResetOption(MySQLParser::ResetOptionContext * /*ctx*/) override { }
  virtual void exitResetOption(MySQLParser::ResetOptionContext * /*ctx*/) override { }

  virtual void enterMasterOrBinaryLogsAndGtids(MySQLParser::MasterOrBinaryLogsAndGtidsContext * /*ctx*/) override { }
  virtual void exitMasterOrBinaryLogsAndGtids(MySQLParser::MasterOrBinaryLogsAndGtidsContext * /*ctx*/) override { }

  virtual void enterSourceResetOptions(MySQLParser::SourceResetOptionsContext * /*ctx*/) override { }
  virtual void exitSourceResetOptions(MySQLParser::SourceResetOptionsContext * /*ctx*/) override { }

  virtual void enterReplicationLoad(MySQLParser::ReplicationLoadContext * /*ctx*/) override { }
  virtual void exitReplicationLoad(MySQLParser::ReplicationLoadContext * /*ctx*/) override { }

  virtual void enterChangeReplicationSource(MySQLParser::ChangeReplicationSourceContext * /*ctx*/) override { }
  virtual void exitChangeReplicationSource(MySQLParser::ChangeReplicationSourceContext * /*ctx*/) override { }

  virtual void enterChangeSource(MySQLParser::ChangeSourceContext * /*ctx*/) override { }
  virtual void exitChangeSource(MySQLParser::ChangeSourceContext * /*ctx*/) override { }

  virtual void enterSourceDefinitions(MySQLParser::SourceDefinitionsContext * /*ctx*/) override { }
  virtual void exitSourceDefinitions(MySQLParser::SourceDefinitionsContext * /*ctx*/) override { }

  virtual void enterSourceDefinition(MySQLParser::SourceDefinitionContext * /*ctx*/) override { }
  virtual void exitSourceDefinition(MySQLParser::SourceDefinitionContext * /*ctx*/) override { }

  virtual void enterChangeReplicationSourceAutoPosition(MySQLParser::ChangeReplicationSourceAutoPositionContext * /*ctx*/) override { }
  virtual void exitChangeReplicationSourceAutoPosition(MySQLParser::ChangeReplicationSourceAutoPositionContext * /*ctx*/) override { }

  virtual void enterChangeReplicationSourceHost(MySQLParser::ChangeReplicationSourceHostContext * /*ctx*/) override { }
  virtual void exitChangeReplicationSourceHost(MySQLParser::ChangeReplicationSourceHostContext * /*ctx*/) override { }

  virtual void enterChangeReplicationSourceBind(MySQLParser::ChangeReplicationSourceBindContext * /*ctx*/) override { }
  virtual void exitChangeReplicationSourceBind(MySQLParser::ChangeReplicationSourceBindContext * /*ctx*/) override { }

  virtual void enterChangeReplicationSourceUser(MySQLParser::ChangeReplicationSourceUserContext * /*ctx*/) override { }
  virtual void exitChangeReplicationSourceUser(MySQLParser::ChangeReplicationSourceUserContext * /*ctx*/) override { }

  virtual void enterChangeReplicationSourcePassword(MySQLParser::ChangeReplicationSourcePasswordContext * /*ctx*/) override { }
  virtual void exitChangeReplicationSourcePassword(MySQLParser::ChangeReplicationSourcePasswordContext * /*ctx*/) override { }

  virtual void enterChangeReplicationSourcePort(MySQLParser::ChangeReplicationSourcePortContext * /*ctx*/) override { }
  virtual void exitChangeReplicationSourcePort(MySQLParser::ChangeReplicationSourcePortContext * /*ctx*/) override { }

  virtual void enterChangeReplicationSourceConnectRetry(MySQLParser::ChangeReplicationSourceConnectRetryContext * /*ctx*/) override { }
  virtual void exitChangeReplicationSourceConnectRetry(MySQLParser::ChangeReplicationSourceConnectRetryContext * /*ctx*/) override { }

  virtual void enterChangeReplicationSourceRetryCount(MySQLParser::ChangeReplicationSourceRetryCountContext * /*ctx*/) override { }
  virtual void exitChangeReplicationSourceRetryCount(MySQLParser::ChangeReplicationSourceRetryCountContext * /*ctx*/) override { }

  virtual void enterChangeReplicationSourceDelay(MySQLParser::ChangeReplicationSourceDelayContext * /*ctx*/) override { }
  virtual void exitChangeReplicationSourceDelay(MySQLParser::ChangeReplicationSourceDelayContext * /*ctx*/) override { }

  virtual void enterChangeReplicationSourceSSL(MySQLParser::ChangeReplicationSourceSSLContext * /*ctx*/) override { }
  virtual void exitChangeReplicationSourceSSL(MySQLParser::ChangeReplicationSourceSSLContext * /*ctx*/) override { }

  virtual void enterChangeReplicationSourceSSLCA(MySQLParser::ChangeReplicationSourceSSLCAContext * /*ctx*/) override { }
  virtual void exitChangeReplicationSourceSSLCA(MySQLParser::ChangeReplicationSourceSSLCAContext * /*ctx*/) override { }

  virtual void enterChangeReplicationSourceSSLCApath(MySQLParser::ChangeReplicationSourceSSLCApathContext * /*ctx*/) override { }
  virtual void exitChangeReplicationSourceSSLCApath(MySQLParser::ChangeReplicationSourceSSLCApathContext * /*ctx*/) override { }

  virtual void enterChangeReplicationSourceSSLCipher(MySQLParser::ChangeReplicationSourceSSLCipherContext * /*ctx*/) override { }
  virtual void exitChangeReplicationSourceSSLCipher(MySQLParser::ChangeReplicationSourceSSLCipherContext * /*ctx*/) override { }

  virtual void enterChangeReplicationSourceSSLCLR(MySQLParser::ChangeReplicationSourceSSLCLRContext * /*ctx*/) override { }
  virtual void exitChangeReplicationSourceSSLCLR(MySQLParser::ChangeReplicationSourceSSLCLRContext * /*ctx*/) override { }

  virtual void enterChangeReplicationSourceSSLCLRpath(MySQLParser::ChangeReplicationSourceSSLCLRpathContext * /*ctx*/) override { }
  virtual void exitChangeReplicationSourceSSLCLRpath(MySQLParser::ChangeReplicationSourceSSLCLRpathContext * /*ctx*/) override { }

  virtual void enterChangeReplicationSourceSSLKey(MySQLParser::ChangeReplicationSourceSSLKeyContext * /*ctx*/) override { }
  virtual void exitChangeReplicationSourceSSLKey(MySQLParser::ChangeReplicationSourceSSLKeyContext * /*ctx*/) override { }

  virtual void enterChangeReplicationSourceSSLVerifyServerCert(MySQLParser::ChangeReplicationSourceSSLVerifyServerCertContext * /*ctx*/) override { }
  virtual void exitChangeReplicationSourceSSLVerifyServerCert(MySQLParser::ChangeReplicationSourceSSLVerifyServerCertContext * /*ctx*/) override { }

  virtual void enterChangeReplicationSourceTLSVersion(MySQLParser::ChangeReplicationSourceTLSVersionContext * /*ctx*/) override { }
  virtual void exitChangeReplicationSourceTLSVersion(MySQLParser::ChangeReplicationSourceTLSVersionContext * /*ctx*/) override { }

  virtual void enterChangeReplicationSourceTLSCiphersuites(MySQLParser::ChangeReplicationSourceTLSCiphersuitesContext * /*ctx*/) override { }
  virtual void exitChangeReplicationSourceTLSCiphersuites(MySQLParser::ChangeReplicationSourceTLSCiphersuitesContext * /*ctx*/) override { }

  virtual void enterChangeReplicationSourceSSLCert(MySQLParser::ChangeReplicationSourceSSLCertContext * /*ctx*/) override { }
  virtual void exitChangeReplicationSourceSSLCert(MySQLParser::ChangeReplicationSourceSSLCertContext * /*ctx*/) override { }

  virtual void enterChangeReplicationSourcePublicKey(MySQLParser::ChangeReplicationSourcePublicKeyContext * /*ctx*/) override { }
  virtual void exitChangeReplicationSourcePublicKey(MySQLParser::ChangeReplicationSourcePublicKeyContext * /*ctx*/) override { }

  virtual void enterChangeReplicationSourceGetSourcePublicKey(MySQLParser::ChangeReplicationSourceGetSourcePublicKeyContext * /*ctx*/) override { }
  virtual void exitChangeReplicationSourceGetSourcePublicKey(MySQLParser::ChangeReplicationSourceGetSourcePublicKeyContext * /*ctx*/) override { }

  virtual void enterChangeReplicationSourceHeartbeatPeriod(MySQLParser::ChangeReplicationSourceHeartbeatPeriodContext * /*ctx*/) override { }
  virtual void exitChangeReplicationSourceHeartbeatPeriod(MySQLParser::ChangeReplicationSourceHeartbeatPeriodContext * /*ctx*/) override { }

  virtual void enterChangeReplicationSourceCompressionAlgorithm(MySQLParser::ChangeReplicationSourceCompressionAlgorithmContext * /*ctx*/) override { }
  virtual void exitChangeReplicationSourceCompressionAlgorithm(MySQLParser::ChangeReplicationSourceCompressionAlgorithmContext * /*ctx*/) override { }

  virtual void enterChangeReplicationSourceZstdCompressionLevel(MySQLParser::ChangeReplicationSourceZstdCompressionLevelContext * /*ctx*/) override { }
  virtual void exitChangeReplicationSourceZstdCompressionLevel(MySQLParser::ChangeReplicationSourceZstdCompressionLevelContext * /*ctx*/) override { }

  virtual void enterPrivilegeCheckDef(MySQLParser::PrivilegeCheckDefContext * /*ctx*/) override { }
  virtual void exitPrivilegeCheckDef(MySQLParser::PrivilegeCheckDefContext * /*ctx*/) override { }

  virtual void enterTablePrimaryKeyCheckDef(MySQLParser::TablePrimaryKeyCheckDefContext * /*ctx*/) override { }
  virtual void exitTablePrimaryKeyCheckDef(MySQLParser::TablePrimaryKeyCheckDefContext * /*ctx*/) override { }

  virtual void enterAssignGtidsToAnonymousTransactionsDefinition(MySQLParser::AssignGtidsToAnonymousTransactionsDefinitionContext * /*ctx*/) override { }
  virtual void exitAssignGtidsToAnonymousTransactionsDefinition(MySQLParser::AssignGtidsToAnonymousTransactionsDefinitionContext * /*ctx*/) override { }

  virtual void enterSourceTlsCiphersuitesDef(MySQLParser::SourceTlsCiphersuitesDefContext * /*ctx*/) override { }
  virtual void exitSourceTlsCiphersuitesDef(MySQLParser::SourceTlsCiphersuitesDefContext * /*ctx*/) override { }

  virtual void enterSourceFileDef(MySQLParser::SourceFileDefContext * /*ctx*/) override { }
  virtual void exitSourceFileDef(MySQLParser::SourceFileDefContext * /*ctx*/) override { }

  virtual void enterSourceLogFile(MySQLParser::SourceLogFileContext * /*ctx*/) override { }
  virtual void exitSourceLogFile(MySQLParser::SourceLogFileContext * /*ctx*/) override { }

  virtual void enterSourceLogPos(MySQLParser::SourceLogPosContext * /*ctx*/) override { }
  virtual void exitSourceLogPos(MySQLParser::SourceLogPosContext * /*ctx*/) override { }

  virtual void enterServerIdList(MySQLParser::ServerIdListContext * /*ctx*/) override { }
  virtual void exitServerIdList(MySQLParser::ServerIdListContext * /*ctx*/) override { }

  virtual void enterChangeReplication(MySQLParser::ChangeReplicationContext * /*ctx*/) override { }
  virtual void exitChangeReplication(MySQLParser::ChangeReplicationContext * /*ctx*/) override { }

  virtual void enterFilterDefinition(MySQLParser::FilterDefinitionContext * /*ctx*/) override { }
  virtual void exitFilterDefinition(MySQLParser::FilterDefinitionContext * /*ctx*/) override { }

  virtual void enterFilterDbList(MySQLParser::FilterDbListContext * /*ctx*/) override { }
  virtual void exitFilterDbList(MySQLParser::FilterDbListContext * /*ctx*/) override { }

  virtual void enterFilterTableList(MySQLParser::FilterTableListContext * /*ctx*/) override { }
  virtual void exitFilterTableList(MySQLParser::FilterTableListContext * /*ctx*/) override { }

  virtual void enterFilterStringList(MySQLParser::FilterStringListContext * /*ctx*/) override { }
  virtual void exitFilterStringList(MySQLParser::FilterStringListContext * /*ctx*/) override { }

  virtual void enterFilterWildDbTableString(MySQLParser::FilterWildDbTableStringContext * /*ctx*/) override { }
  virtual void exitFilterWildDbTableString(MySQLParser::FilterWildDbTableStringContext * /*ctx*/) override { }

  virtual void enterFilterDbPairList(MySQLParser::FilterDbPairListContext * /*ctx*/) override { }
  virtual void exitFilterDbPairList(MySQLParser::FilterDbPairListContext * /*ctx*/) override { }

  virtual void enterStartReplicaStatement(MySQLParser::StartReplicaStatementContext * /*ctx*/) override { }
  virtual void exitStartReplicaStatement(MySQLParser::StartReplicaStatementContext * /*ctx*/) override { }

  virtual void enterStopReplicaStatement(MySQLParser::StopReplicaStatementContext * /*ctx*/) override { }
  virtual void exitStopReplicaStatement(MySQLParser::StopReplicaStatementContext * /*ctx*/) override { }

  virtual void enterReplicaUntil(MySQLParser::ReplicaUntilContext * /*ctx*/) override { }
  virtual void exitReplicaUntil(MySQLParser::ReplicaUntilContext * /*ctx*/) override { }

  virtual void enterUserOption(MySQLParser::UserOptionContext * /*ctx*/) override { }
  virtual void exitUserOption(MySQLParser::UserOptionContext * /*ctx*/) override { }

  virtual void enterPasswordOption(MySQLParser::PasswordOptionContext * /*ctx*/) override { }
  virtual void exitPasswordOption(MySQLParser::PasswordOptionContext * /*ctx*/) override { }

  virtual void enterDefaultAuthOption(MySQLParser::DefaultAuthOptionContext * /*ctx*/) override { }
  virtual void exitDefaultAuthOption(MySQLParser::DefaultAuthOptionContext * /*ctx*/) override { }

  virtual void enterPluginDirOption(MySQLParser::PluginDirOptionContext * /*ctx*/) override { }
  virtual void exitPluginDirOption(MySQLParser::PluginDirOptionContext * /*ctx*/) override { }

  virtual void enterReplicaThreadOptions(MySQLParser::ReplicaThreadOptionsContext * /*ctx*/) override { }
  virtual void exitReplicaThreadOptions(MySQLParser::ReplicaThreadOptionsContext * /*ctx*/) override { }

  virtual void enterReplicaThreadOption(MySQLParser::ReplicaThreadOptionContext * /*ctx*/) override { }
  virtual void exitReplicaThreadOption(MySQLParser::ReplicaThreadOptionContext * /*ctx*/) override { }

  virtual void enterGroupReplication(MySQLParser::GroupReplicationContext * /*ctx*/) override { }
  virtual void exitGroupReplication(MySQLParser::GroupReplicationContext * /*ctx*/) override { }

  virtual void enterGroupReplicationStartOptions(MySQLParser::GroupReplicationStartOptionsContext * /*ctx*/) override { }
  virtual void exitGroupReplicationStartOptions(MySQLParser::GroupReplicationStartOptionsContext * /*ctx*/) override { }

  virtual void enterGroupReplicationStartOption(MySQLParser::GroupReplicationStartOptionContext * /*ctx*/) override { }
  virtual void exitGroupReplicationStartOption(MySQLParser::GroupReplicationStartOptionContext * /*ctx*/) override { }

  virtual void enterGroupReplicationUser(MySQLParser::GroupReplicationUserContext * /*ctx*/) override { }
  virtual void exitGroupReplicationUser(MySQLParser::GroupReplicationUserContext * /*ctx*/) override { }

  virtual void enterGroupReplicationPassword(MySQLParser::GroupReplicationPasswordContext * /*ctx*/) override { }
  virtual void exitGroupReplicationPassword(MySQLParser::GroupReplicationPasswordContext * /*ctx*/) override { }

  virtual void enterGroupReplicationPluginAuth(MySQLParser::GroupReplicationPluginAuthContext * /*ctx*/) override { }
  virtual void exitGroupReplicationPluginAuth(MySQLParser::GroupReplicationPluginAuthContext * /*ctx*/) override { }

  virtual void enterReplica(MySQLParser::ReplicaContext * /*ctx*/) override { }
  virtual void exitReplica(MySQLParser::ReplicaContext * /*ctx*/) override { }

  virtual void enterPreparedStatement(MySQLParser::PreparedStatementContext * /*ctx*/) override { }
  virtual void exitPreparedStatement(MySQLParser::PreparedStatementContext * /*ctx*/) override { }

  virtual void enterExecuteStatement(MySQLParser::ExecuteStatementContext * /*ctx*/) override { }
  virtual void exitExecuteStatement(MySQLParser::ExecuteStatementContext * /*ctx*/) override { }

  virtual void enterExecuteVarList(MySQLParser::ExecuteVarListContext * /*ctx*/) override { }
  virtual void exitExecuteVarList(MySQLParser::ExecuteVarListContext * /*ctx*/) override { }

  virtual void enterCloneStatement(MySQLParser::CloneStatementContext * /*ctx*/) override { }
  virtual void exitCloneStatement(MySQLParser::CloneStatementContext * /*ctx*/) override { }

  virtual void enterDataDirSSL(MySQLParser::DataDirSSLContext * /*ctx*/) override { }
  virtual void exitDataDirSSL(MySQLParser::DataDirSSLContext * /*ctx*/) override { }

  virtual void enterSsl(MySQLParser::SslContext * /*ctx*/) override { }
  virtual void exitSsl(MySQLParser::SslContext * /*ctx*/) override { }

  virtual void enterAccountManagementStatement(MySQLParser::AccountManagementStatementContext * /*ctx*/) override { }
  virtual void exitAccountManagementStatement(MySQLParser::AccountManagementStatementContext * /*ctx*/) override { }

  virtual void enterAlterUserStatement(MySQLParser::AlterUserStatementContext * /*ctx*/) override { }
  virtual void exitAlterUserStatement(MySQLParser::AlterUserStatementContext * /*ctx*/) override { }

  virtual void enterAlterUserList(MySQLParser::AlterUserListContext * /*ctx*/) override { }
  virtual void exitAlterUserList(MySQLParser::AlterUserListContext * /*ctx*/) override { }

  virtual void enterAlterUser(MySQLParser::AlterUserContext * /*ctx*/) override { }
  virtual void exitAlterUser(MySQLParser::AlterUserContext * /*ctx*/) override { }

  virtual void enterOldAlterUser(MySQLParser::OldAlterUserContext * /*ctx*/) override { }
  virtual void exitOldAlterUser(MySQLParser::OldAlterUserContext * /*ctx*/) override { }

  virtual void enterUserFunction(MySQLParser::UserFunctionContext * /*ctx*/) override { }
  virtual void exitUserFunction(MySQLParser::UserFunctionContext * /*ctx*/) override { }

  virtual void enterCreateUserStatement(MySQLParser::CreateUserStatementContext * /*ctx*/) override { }
  virtual void exitCreateUserStatement(MySQLParser::CreateUserStatementContext * /*ctx*/) override { }

  virtual void enterCreateUserTail(MySQLParser::CreateUserTailContext * /*ctx*/) override { }
  virtual void exitCreateUserTail(MySQLParser::CreateUserTailContext * /*ctx*/) override { }

  virtual void enterUserAttributes(MySQLParser::UserAttributesContext * /*ctx*/) override { }
  virtual void exitUserAttributes(MySQLParser::UserAttributesContext * /*ctx*/) override { }

  virtual void enterDefaultRoleClause(MySQLParser::DefaultRoleClauseContext * /*ctx*/) override { }
  virtual void exitDefaultRoleClause(MySQLParser::DefaultRoleClauseContext * /*ctx*/) override { }

  virtual void enterRequireClause(MySQLParser::RequireClauseContext * /*ctx*/) override { }
  virtual void exitRequireClause(MySQLParser::RequireClauseContext * /*ctx*/) override { }

  virtual void enterConnectOptions(MySQLParser::ConnectOptionsContext * /*ctx*/) override { }
  virtual void exitConnectOptions(MySQLParser::ConnectOptionsContext * /*ctx*/) override { }

  virtual void enterAccountLockPasswordExpireOptions(MySQLParser::AccountLockPasswordExpireOptionsContext * /*ctx*/) override { }
  virtual void exitAccountLockPasswordExpireOptions(MySQLParser::AccountLockPasswordExpireOptionsContext * /*ctx*/) override { }

  virtual void enterDropUserStatement(MySQLParser::DropUserStatementContext * /*ctx*/) override { }
  virtual void exitDropUserStatement(MySQLParser::DropUserStatementContext * /*ctx*/) override { }

  virtual void enterGrantStatement(MySQLParser::GrantStatementContext * /*ctx*/) override { }
  virtual void exitGrantStatement(MySQLParser::GrantStatementContext * /*ctx*/) override { }

  virtual void enterGrantTargetList(MySQLParser::GrantTargetListContext * /*ctx*/) override { }
  virtual void exitGrantTargetList(MySQLParser::GrantTargetListContext * /*ctx*/) override { }

  virtual void enterGrantOptions(MySQLParser::GrantOptionsContext * /*ctx*/) override { }
  virtual void exitGrantOptions(MySQLParser::GrantOptionsContext * /*ctx*/) override { }

  virtual void enterExceptRoleList(MySQLParser::ExceptRoleListContext * /*ctx*/) override { }
  virtual void exitExceptRoleList(MySQLParser::ExceptRoleListContext * /*ctx*/) override { }

  virtual void enterWithRoles(MySQLParser::WithRolesContext * /*ctx*/) override { }
  virtual void exitWithRoles(MySQLParser::WithRolesContext * /*ctx*/) override { }

  virtual void enterGrantAs(MySQLParser::GrantAsContext * /*ctx*/) override { }
  virtual void exitGrantAs(MySQLParser::GrantAsContext * /*ctx*/) override { }

  virtual void enterVersionedRequireClause(MySQLParser::VersionedRequireClauseContext * /*ctx*/) override { }
  virtual void exitVersionedRequireClause(MySQLParser::VersionedRequireClauseContext * /*ctx*/) override { }

  virtual void enterRenameUserStatement(MySQLParser::RenameUserStatementContext * /*ctx*/) override { }
  virtual void exitRenameUserStatement(MySQLParser::RenameUserStatementContext * /*ctx*/) override { }

  virtual void enterRevokeStatement(MySQLParser::RevokeStatementContext * /*ctx*/) override { }
  virtual void exitRevokeStatement(MySQLParser::RevokeStatementContext * /*ctx*/) override { }

  virtual void enterAclType(MySQLParser::AclTypeContext * /*ctx*/) override { }
  virtual void exitAclType(MySQLParser::AclTypeContext * /*ctx*/) override { }

  virtual void enterRoleOrPrivilegesList(MySQLParser::RoleOrPrivilegesListContext * /*ctx*/) override { }
  virtual void exitRoleOrPrivilegesList(MySQLParser::RoleOrPrivilegesListContext * /*ctx*/) override { }

  virtual void enterRoleOrPrivilege(MySQLParser::RoleOrPrivilegeContext * /*ctx*/) override { }
  virtual void exitRoleOrPrivilege(MySQLParser::RoleOrPrivilegeContext * /*ctx*/) override { }

  virtual void enterGrantIdentifier(MySQLParser::GrantIdentifierContext * /*ctx*/) override { }
  virtual void exitGrantIdentifier(MySQLParser::GrantIdentifierContext * /*ctx*/) override { }

  virtual void enterRequireList(MySQLParser::RequireListContext * /*ctx*/) override { }
  virtual void exitRequireList(MySQLParser::RequireListContext * /*ctx*/) override { }

  virtual void enterRequireListElement(MySQLParser::RequireListElementContext * /*ctx*/) override { }
  virtual void exitRequireListElement(MySQLParser::RequireListElementContext * /*ctx*/) override { }

  virtual void enterGrantOption(MySQLParser::GrantOptionContext * /*ctx*/) override { }
  virtual void exitGrantOption(MySQLParser::GrantOptionContext * /*ctx*/) override { }

  virtual void enterSetRoleStatement(MySQLParser::SetRoleStatementContext * /*ctx*/) override { }
  virtual void exitSetRoleStatement(MySQLParser::SetRoleStatementContext * /*ctx*/) override { }

  virtual void enterRoleList(MySQLParser::RoleListContext * /*ctx*/) override { }
  virtual void exitRoleList(MySQLParser::RoleListContext * /*ctx*/) override { }

  virtual void enterRole(MySQLParser::RoleContext * /*ctx*/) override { }
  virtual void exitRole(MySQLParser::RoleContext * /*ctx*/) override { }

  virtual void enterTableAdministrationStatement(MySQLParser::TableAdministrationStatementContext * /*ctx*/) override { }
  virtual void exitTableAdministrationStatement(MySQLParser::TableAdministrationStatementContext * /*ctx*/) override { }

  virtual void enterHistogram(MySQLParser::HistogramContext * /*ctx*/) override { }
  virtual void exitHistogram(MySQLParser::HistogramContext * /*ctx*/) override { }

  virtual void enterCheckOption(MySQLParser::CheckOptionContext * /*ctx*/) override { }
  virtual void exitCheckOption(MySQLParser::CheckOptionContext * /*ctx*/) override { }

  virtual void enterRepairType(MySQLParser::RepairTypeContext * /*ctx*/) override { }
  virtual void exitRepairType(MySQLParser::RepairTypeContext * /*ctx*/) override { }

  virtual void enterUninstallStatement(MySQLParser::UninstallStatementContext * /*ctx*/) override { }
  virtual void exitUninstallStatement(MySQLParser::UninstallStatementContext * /*ctx*/) override { }

  virtual void enterInstallStatement(MySQLParser::InstallStatementContext * /*ctx*/) override { }
  virtual void exitInstallStatement(MySQLParser::InstallStatementContext * /*ctx*/) override { }

  virtual void enterInstallOptionType(MySQLParser::InstallOptionTypeContext * /*ctx*/) override { }
  virtual void exitInstallOptionType(MySQLParser::InstallOptionTypeContext * /*ctx*/) override { }

  virtual void enterInstallSetRvalue(MySQLParser::InstallSetRvalueContext * /*ctx*/) override { }
  virtual void exitInstallSetRvalue(MySQLParser::InstallSetRvalueContext * /*ctx*/) override { }

  virtual void enterInstallSetValue(MySQLParser::InstallSetValueContext * /*ctx*/) override { }
  virtual void exitInstallSetValue(MySQLParser::InstallSetValueContext * /*ctx*/) override { }

  virtual void enterInstallSetValueList(MySQLParser::InstallSetValueListContext * /*ctx*/) override { }
  virtual void exitInstallSetValueList(MySQLParser::InstallSetValueListContext * /*ctx*/) override { }

  virtual void enterSetStatement(MySQLParser::SetStatementContext * /*ctx*/) override { }
  virtual void exitSetStatement(MySQLParser::SetStatementContext * /*ctx*/) override { }

  virtual void enterStartOptionValueList(MySQLParser::StartOptionValueListContext * /*ctx*/) override { }
  virtual void exitStartOptionValueList(MySQLParser::StartOptionValueListContext * /*ctx*/) override { }

  virtual void enterTransactionCharacteristics(MySQLParser::TransactionCharacteristicsContext * /*ctx*/) override { }
  virtual void exitTransactionCharacteristics(MySQLParser::TransactionCharacteristicsContext * /*ctx*/) override { }

  virtual void enterTransactionAccessMode(MySQLParser::TransactionAccessModeContext * /*ctx*/) override { }
  virtual void exitTransactionAccessMode(MySQLParser::TransactionAccessModeContext * /*ctx*/) override { }

  virtual void enterIsolationLevel(MySQLParser::IsolationLevelContext * /*ctx*/) override { }
  virtual void exitIsolationLevel(MySQLParser::IsolationLevelContext * /*ctx*/) override { }

  virtual void enterOptionValueListContinued(MySQLParser::OptionValueListContinuedContext * /*ctx*/) override { }
  virtual void exitOptionValueListContinued(MySQLParser::OptionValueListContinuedContext * /*ctx*/) override { }

  virtual void enterOptionValueNoOptionType(MySQLParser::OptionValueNoOptionTypeContext * /*ctx*/) override { }
  virtual void exitOptionValueNoOptionType(MySQLParser::OptionValueNoOptionTypeContext * /*ctx*/) override { }

  virtual void enterOptionValue(MySQLParser::OptionValueContext * /*ctx*/) override { }
  virtual void exitOptionValue(MySQLParser::OptionValueContext * /*ctx*/) override { }

  virtual void enterStartOptionValueListFollowingOptionType(MySQLParser::StartOptionValueListFollowingOptionTypeContext * /*ctx*/) override { }
  virtual void exitStartOptionValueListFollowingOptionType(MySQLParser::StartOptionValueListFollowingOptionTypeContext * /*ctx*/) override { }

  virtual void enterOptionValueFollowingOptionType(MySQLParser::OptionValueFollowingOptionTypeContext * /*ctx*/) override { }
  virtual void exitOptionValueFollowingOptionType(MySQLParser::OptionValueFollowingOptionTypeContext * /*ctx*/) override { }

  virtual void enterSetExprOrDefault(MySQLParser::SetExprOrDefaultContext * /*ctx*/) override { }
  virtual void exitSetExprOrDefault(MySQLParser::SetExprOrDefaultContext * /*ctx*/) override { }

  virtual void enterShowDatabasesStatement(MySQLParser::ShowDatabasesStatementContext * /*ctx*/) override { }
  virtual void exitShowDatabasesStatement(MySQLParser::ShowDatabasesStatementContext * /*ctx*/) override { }

  virtual void enterShowTablesStatement(MySQLParser::ShowTablesStatementContext * /*ctx*/) override { }
  virtual void exitShowTablesStatement(MySQLParser::ShowTablesStatementContext * /*ctx*/) override { }

  virtual void enterShowTriggersStatement(MySQLParser::ShowTriggersStatementContext * /*ctx*/) override { }
  virtual void exitShowTriggersStatement(MySQLParser::ShowTriggersStatementContext * /*ctx*/) override { }

  virtual void enterShowEventsStatement(MySQLParser::ShowEventsStatementContext * /*ctx*/) override { }
  virtual void exitShowEventsStatement(MySQLParser::ShowEventsStatementContext * /*ctx*/) override { }

  virtual void enterShowTableStatusStatement(MySQLParser::ShowTableStatusStatementContext * /*ctx*/) override { }
  virtual void exitShowTableStatusStatement(MySQLParser::ShowTableStatusStatementContext * /*ctx*/) override { }

  virtual void enterShowOpenTablesStatement(MySQLParser::ShowOpenTablesStatementContext * /*ctx*/) override { }
  virtual void exitShowOpenTablesStatement(MySQLParser::ShowOpenTablesStatementContext * /*ctx*/) override { }

  virtual void enterShowParseTreeStatement(MySQLParser::ShowParseTreeStatementContext * /*ctx*/) override { }
  virtual void exitShowParseTreeStatement(MySQLParser::ShowParseTreeStatementContext * /*ctx*/) override { }

  virtual void enterShowPluginsStatement(MySQLParser::ShowPluginsStatementContext * /*ctx*/) override { }
  virtual void exitShowPluginsStatement(MySQLParser::ShowPluginsStatementContext * /*ctx*/) override { }

  virtual void enterShowEngineLogsStatement(MySQLParser::ShowEngineLogsStatementContext * /*ctx*/) override { }
  virtual void exitShowEngineLogsStatement(MySQLParser::ShowEngineLogsStatementContext * /*ctx*/) override { }

  virtual void enterShowEngineMutexStatement(MySQLParser::ShowEngineMutexStatementContext * /*ctx*/) override { }
  virtual void exitShowEngineMutexStatement(MySQLParser::ShowEngineMutexStatementContext * /*ctx*/) override { }

  virtual void enterShowEngineStatusStatement(MySQLParser::ShowEngineStatusStatementContext * /*ctx*/) override { }
  virtual void exitShowEngineStatusStatement(MySQLParser::ShowEngineStatusStatementContext * /*ctx*/) override { }

  virtual void enterShowColumnsStatement(MySQLParser::ShowColumnsStatementContext * /*ctx*/) override { }
  virtual void exitShowColumnsStatement(MySQLParser::ShowColumnsStatementContext * /*ctx*/) override { }

  virtual void enterShowBinaryLogsStatement(MySQLParser::ShowBinaryLogsStatementContext * /*ctx*/) override { }
  virtual void exitShowBinaryLogsStatement(MySQLParser::ShowBinaryLogsStatementContext * /*ctx*/) override { }

  virtual void enterShowBinaryLogStatusStatement(MySQLParser::ShowBinaryLogStatusStatementContext * /*ctx*/) override { }
  virtual void exitShowBinaryLogStatusStatement(MySQLParser::ShowBinaryLogStatusStatementContext * /*ctx*/) override { }

  virtual void enterShowReplicasStatement(MySQLParser::ShowReplicasStatementContext * /*ctx*/) override { }
  virtual void exitShowReplicasStatement(MySQLParser::ShowReplicasStatementContext * /*ctx*/) override { }

  virtual void enterShowBinlogEventsStatement(MySQLParser::ShowBinlogEventsStatementContext * /*ctx*/) override { }
  virtual void exitShowBinlogEventsStatement(MySQLParser::ShowBinlogEventsStatementContext * /*ctx*/) override { }

  virtual void enterShowRelaylogEventsStatement(MySQLParser::ShowRelaylogEventsStatementContext * /*ctx*/) override { }
  virtual void exitShowRelaylogEventsStatement(MySQLParser::ShowRelaylogEventsStatementContext * /*ctx*/) override { }

  virtual void enterShowKeysStatement(MySQLParser::ShowKeysStatementContext * /*ctx*/) override { }
  virtual void exitShowKeysStatement(MySQLParser::ShowKeysStatementContext * /*ctx*/) override { }

  virtual void enterShowEnginesStatement(MySQLParser::ShowEnginesStatementContext * /*ctx*/) override { }
  virtual void exitShowEnginesStatement(MySQLParser::ShowEnginesStatementContext * /*ctx*/) override { }

  virtual void enterShowCountWarningsStatement(MySQLParser::ShowCountWarningsStatementContext * /*ctx*/) override { }
  virtual void exitShowCountWarningsStatement(MySQLParser::ShowCountWarningsStatementContext * /*ctx*/) override { }

  virtual void enterShowCountErrorsStatement(MySQLParser::ShowCountErrorsStatementContext * /*ctx*/) override { }
  virtual void exitShowCountErrorsStatement(MySQLParser::ShowCountErrorsStatementContext * /*ctx*/) override { }

  virtual void enterShowWarningsStatement(MySQLParser::ShowWarningsStatementContext * /*ctx*/) override { }
  virtual void exitShowWarningsStatement(MySQLParser::ShowWarningsStatementContext * /*ctx*/) override { }

  virtual void enterShowErrorsStatement(MySQLParser::ShowErrorsStatementContext * /*ctx*/) override { }
  virtual void exitShowErrorsStatement(MySQLParser::ShowErrorsStatementContext * /*ctx*/) override { }

  virtual void enterShowProfilesStatement(MySQLParser::ShowProfilesStatementContext * /*ctx*/) override { }
  virtual void exitShowProfilesStatement(MySQLParser::ShowProfilesStatementContext * /*ctx*/) override { }

  virtual void enterShowProfileStatement(MySQLParser::ShowProfileStatementContext * /*ctx*/) override { }
  virtual void exitShowProfileStatement(MySQLParser::ShowProfileStatementContext * /*ctx*/) override { }

  virtual void enterShowStatusStatement(MySQLParser::ShowStatusStatementContext * /*ctx*/) override { }
  virtual void exitShowStatusStatement(MySQLParser::ShowStatusStatementContext * /*ctx*/) override { }

  virtual void enterShowProcessListStatement(MySQLParser::ShowProcessListStatementContext * /*ctx*/) override { }
  virtual void exitShowProcessListStatement(MySQLParser::ShowProcessListStatementContext * /*ctx*/) override { }

  virtual void enterShowVariablesStatement(MySQLParser::ShowVariablesStatementContext * /*ctx*/) override { }
  virtual void exitShowVariablesStatement(MySQLParser::ShowVariablesStatementContext * /*ctx*/) override { }

  virtual void enterShowCharacterSetStatement(MySQLParser::ShowCharacterSetStatementContext * /*ctx*/) override { }
  virtual void exitShowCharacterSetStatement(MySQLParser::ShowCharacterSetStatementContext * /*ctx*/) override { }

  virtual void enterShowCollationStatement(MySQLParser::ShowCollationStatementContext * /*ctx*/) override { }
  virtual void exitShowCollationStatement(MySQLParser::ShowCollationStatementContext * /*ctx*/) override { }

  virtual void enterShowPrivilegesStatement(MySQLParser::ShowPrivilegesStatementContext * /*ctx*/) override { }
  virtual void exitShowPrivilegesStatement(MySQLParser::ShowPrivilegesStatementContext * /*ctx*/) override { }

  virtual void enterShowGrantsStatement(MySQLParser::ShowGrantsStatementContext * /*ctx*/) override { }
  virtual void exitShowGrantsStatement(MySQLParser::ShowGrantsStatementContext * /*ctx*/) override { }

  virtual void enterShowCreateDatabaseStatement(MySQLParser::ShowCreateDatabaseStatementContext * /*ctx*/) override { }
  virtual void exitShowCreateDatabaseStatement(MySQLParser::ShowCreateDatabaseStatementContext * /*ctx*/) override { }

  virtual void enterShowCreateTableStatement(MySQLParser::ShowCreateTableStatementContext * /*ctx*/) override { }
  virtual void exitShowCreateTableStatement(MySQLParser::ShowCreateTableStatementContext * /*ctx*/) override { }

  virtual void enterShowCreateViewStatement(MySQLParser::ShowCreateViewStatementContext * /*ctx*/) override { }
  virtual void exitShowCreateViewStatement(MySQLParser::ShowCreateViewStatementContext * /*ctx*/) override { }

  virtual void enterShowMasterStatusStatement(MySQLParser::ShowMasterStatusStatementContext * /*ctx*/) override { }
  virtual void exitShowMasterStatusStatement(MySQLParser::ShowMasterStatusStatementContext * /*ctx*/) override { }

  virtual void enterShowReplicaStatusStatement(MySQLParser::ShowReplicaStatusStatementContext * /*ctx*/) override { }
  virtual void exitShowReplicaStatusStatement(MySQLParser::ShowReplicaStatusStatementContext * /*ctx*/) override { }

  virtual void enterShowCreateProcedureStatement(MySQLParser::ShowCreateProcedureStatementContext * /*ctx*/) override { }
  virtual void exitShowCreateProcedureStatement(MySQLParser::ShowCreateProcedureStatementContext * /*ctx*/) override { }

  virtual void enterShowCreateFunctionStatement(MySQLParser::ShowCreateFunctionStatementContext * /*ctx*/) override { }
  virtual void exitShowCreateFunctionStatement(MySQLParser::ShowCreateFunctionStatementContext * /*ctx*/) override { }

  virtual void enterShowCreateTriggerStatement(MySQLParser::ShowCreateTriggerStatementContext * /*ctx*/) override { }
  virtual void exitShowCreateTriggerStatement(MySQLParser::ShowCreateTriggerStatementContext * /*ctx*/) override { }

  virtual void enterShowCreateProcedureStatusStatement(MySQLParser::ShowCreateProcedureStatusStatementContext * /*ctx*/) override { }
  virtual void exitShowCreateProcedureStatusStatement(MySQLParser::ShowCreateProcedureStatusStatementContext * /*ctx*/) override { }

  virtual void enterShowCreateFunctionStatusStatement(MySQLParser::ShowCreateFunctionStatusStatementContext * /*ctx*/) override { }
  virtual void exitShowCreateFunctionStatusStatement(MySQLParser::ShowCreateFunctionStatusStatementContext * /*ctx*/) override { }

  virtual void enterShowCreateProcedureCodeStatement(MySQLParser::ShowCreateProcedureCodeStatementContext * /*ctx*/) override { }
  virtual void exitShowCreateProcedureCodeStatement(MySQLParser::ShowCreateProcedureCodeStatementContext * /*ctx*/) override { }

  virtual void enterShowCreateFunctionCodeStatement(MySQLParser::ShowCreateFunctionCodeStatementContext * /*ctx*/) override { }
  virtual void exitShowCreateFunctionCodeStatement(MySQLParser::ShowCreateFunctionCodeStatementContext * /*ctx*/) override { }

  virtual void enterShowCreateEventStatement(MySQLParser::ShowCreateEventStatementContext * /*ctx*/) override { }
  virtual void exitShowCreateEventStatement(MySQLParser::ShowCreateEventStatementContext * /*ctx*/) override { }

  virtual void enterShowCreateUserStatement(MySQLParser::ShowCreateUserStatementContext * /*ctx*/) override { }
  virtual void exitShowCreateUserStatement(MySQLParser::ShowCreateUserStatementContext * /*ctx*/) override { }

  virtual void enterShowCommandType(MySQLParser::ShowCommandTypeContext * /*ctx*/) override { }
  virtual void exitShowCommandType(MySQLParser::ShowCommandTypeContext * /*ctx*/) override { }

  virtual void enterEngineOrAll(MySQLParser::EngineOrAllContext * /*ctx*/) override { }
  virtual void exitEngineOrAll(MySQLParser::EngineOrAllContext * /*ctx*/) override { }

  virtual void enterFromOrIn(MySQLParser::FromOrInContext * /*ctx*/) override { }
  virtual void exitFromOrIn(MySQLParser::FromOrInContext * /*ctx*/) override { }

  virtual void enterInDb(MySQLParser::InDbContext * /*ctx*/) override { }
  virtual void exitInDb(MySQLParser::InDbContext * /*ctx*/) override { }

  virtual void enterProfileDefinitions(MySQLParser::ProfileDefinitionsContext * /*ctx*/) override { }
  virtual void exitProfileDefinitions(MySQLParser::ProfileDefinitionsContext * /*ctx*/) override { }

  virtual void enterProfileDefinition(MySQLParser::ProfileDefinitionContext * /*ctx*/) override { }
  virtual void exitProfileDefinition(MySQLParser::ProfileDefinitionContext * /*ctx*/) override { }

  virtual void enterOtherAdministrativeStatement(MySQLParser::OtherAdministrativeStatementContext * /*ctx*/) override { }
  virtual void exitOtherAdministrativeStatement(MySQLParser::OtherAdministrativeStatementContext * /*ctx*/) override { }

  virtual void enterKeyCacheListOrParts(MySQLParser::KeyCacheListOrPartsContext * /*ctx*/) override { }
  virtual void exitKeyCacheListOrParts(MySQLParser::KeyCacheListOrPartsContext * /*ctx*/) override { }

  virtual void enterKeyCacheList(MySQLParser::KeyCacheListContext * /*ctx*/) override { }
  virtual void exitKeyCacheList(MySQLParser::KeyCacheListContext * /*ctx*/) override { }

  virtual void enterAssignToKeycache(MySQLParser::AssignToKeycacheContext * /*ctx*/) override { }
  virtual void exitAssignToKeycache(MySQLParser::AssignToKeycacheContext * /*ctx*/) override { }

  virtual void enterAssignToKeycachePartition(MySQLParser::AssignToKeycachePartitionContext * /*ctx*/) override { }
  virtual void exitAssignToKeycachePartition(MySQLParser::AssignToKeycachePartitionContext * /*ctx*/) override { }

  virtual void enterCacheKeyList(MySQLParser::CacheKeyListContext * /*ctx*/) override { }
  virtual void exitCacheKeyList(MySQLParser::CacheKeyListContext * /*ctx*/) override { }

  virtual void enterKeyUsageElement(MySQLParser::KeyUsageElementContext * /*ctx*/) override { }
  virtual void exitKeyUsageElement(MySQLParser::KeyUsageElementContext * /*ctx*/) override { }

  virtual void enterKeyUsageList(MySQLParser::KeyUsageListContext * /*ctx*/) override { }
  virtual void exitKeyUsageList(MySQLParser::KeyUsageListContext * /*ctx*/) override { }

  virtual void enterFlushOption(MySQLParser::FlushOptionContext * /*ctx*/) override { }
  virtual void exitFlushOption(MySQLParser::FlushOptionContext * /*ctx*/) override { }

  virtual void enterLogType(MySQLParser::LogTypeContext * /*ctx*/) override { }
  virtual void exitLogType(MySQLParser::LogTypeContext * /*ctx*/) override { }

  virtual void enterFlushTables(MySQLParser::FlushTablesContext * /*ctx*/) override { }
  virtual void exitFlushTables(MySQLParser::FlushTablesContext * /*ctx*/) override { }

  virtual void enterFlushTablesOptions(MySQLParser::FlushTablesOptionsContext * /*ctx*/) override { }
  virtual void exitFlushTablesOptions(MySQLParser::FlushTablesOptionsContext * /*ctx*/) override { }

  virtual void enterPreloadTail(MySQLParser::PreloadTailContext * /*ctx*/) override { }
  virtual void exitPreloadTail(MySQLParser::PreloadTailContext * /*ctx*/) override { }

  virtual void enterPreloadList(MySQLParser::PreloadListContext * /*ctx*/) override { }
  virtual void exitPreloadList(MySQLParser::PreloadListContext * /*ctx*/) override { }

  virtual void enterPreloadKeys(MySQLParser::PreloadKeysContext * /*ctx*/) override { }
  virtual void exitPreloadKeys(MySQLParser::PreloadKeysContext * /*ctx*/) override { }

  virtual void enterAdminPartition(MySQLParser::AdminPartitionContext * /*ctx*/) override { }
  virtual void exitAdminPartition(MySQLParser::AdminPartitionContext * /*ctx*/) override { }

  virtual void enterResourceGroupManagement(MySQLParser::ResourceGroupManagementContext * /*ctx*/) override { }
  virtual void exitResourceGroupManagement(MySQLParser::ResourceGroupManagementContext * /*ctx*/) override { }

  virtual void enterCreateResourceGroup(MySQLParser::CreateResourceGroupContext * /*ctx*/) override { }
  virtual void exitCreateResourceGroup(MySQLParser::CreateResourceGroupContext * /*ctx*/) override { }

  virtual void enterResourceGroupVcpuList(MySQLParser::ResourceGroupVcpuListContext * /*ctx*/) override { }
  virtual void exitResourceGroupVcpuList(MySQLParser::ResourceGroupVcpuListContext * /*ctx*/) override { }

  virtual void enterVcpuNumOrRange(MySQLParser::VcpuNumOrRangeContext * /*ctx*/) override { }
  virtual void exitVcpuNumOrRange(MySQLParser::VcpuNumOrRangeContext * /*ctx*/) override { }

  virtual void enterResourceGroupPriority(MySQLParser::ResourceGroupPriorityContext * /*ctx*/) override { }
  virtual void exitResourceGroupPriority(MySQLParser::ResourceGroupPriorityContext * /*ctx*/) override { }

  virtual void enterResourceGroupEnableDisable(MySQLParser::ResourceGroupEnableDisableContext * /*ctx*/) override { }
  virtual void exitResourceGroupEnableDisable(MySQLParser::ResourceGroupEnableDisableContext * /*ctx*/) override { }

  virtual void enterAlterResourceGroup(MySQLParser::AlterResourceGroupContext * /*ctx*/) override { }
  virtual void exitAlterResourceGroup(MySQLParser::AlterResourceGroupContext * /*ctx*/) override { }

  virtual void enterSetResourceGroup(MySQLParser::SetResourceGroupContext * /*ctx*/) override { }
  virtual void exitSetResourceGroup(MySQLParser::SetResourceGroupContext * /*ctx*/) override { }

  virtual void enterThreadIdList(MySQLParser::ThreadIdListContext * /*ctx*/) override { }
  virtual void exitThreadIdList(MySQLParser::ThreadIdListContext * /*ctx*/) override { }

  virtual void enterDropResourceGroup(MySQLParser::DropResourceGroupContext * /*ctx*/) override { }
  virtual void exitDropResourceGroup(MySQLParser::DropResourceGroupContext * /*ctx*/) override { }

  virtual void enterUtilityStatement(MySQLParser::UtilityStatementContext * /*ctx*/) override { }
  virtual void exitUtilityStatement(MySQLParser::UtilityStatementContext * /*ctx*/) override { }

  virtual void enterDescribeStatement(MySQLParser::DescribeStatementContext * /*ctx*/) override { }
  virtual void exitDescribeStatement(MySQLParser::DescribeStatementContext * /*ctx*/) override { }

  virtual void enterExplainStatement(MySQLParser::ExplainStatementContext * /*ctx*/) override { }
  virtual void exitExplainStatement(MySQLParser::ExplainStatementContext * /*ctx*/) override { }

  virtual void enterExplainOptions(MySQLParser::ExplainOptionsContext * /*ctx*/) override { }
  virtual void exitExplainOptions(MySQLParser::ExplainOptionsContext * /*ctx*/) override { }

  virtual void enterExplainableStatement(MySQLParser::ExplainableStatementContext * /*ctx*/) override { }
  virtual void exitExplainableStatement(MySQLParser::ExplainableStatementContext * /*ctx*/) override { }

  virtual void enterExplainInto(MySQLParser::ExplainIntoContext * /*ctx*/) override { }
  virtual void exitExplainInto(MySQLParser::ExplainIntoContext * /*ctx*/) override { }

  virtual void enterHelpCommand(MySQLParser::HelpCommandContext * /*ctx*/) override { }
  virtual void exitHelpCommand(MySQLParser::HelpCommandContext * /*ctx*/) override { }

  virtual void enterUseCommand(MySQLParser::UseCommandContext * /*ctx*/) override { }
  virtual void exitUseCommand(MySQLParser::UseCommandContext * /*ctx*/) override { }

  virtual void enterRestartServer(MySQLParser::RestartServerContext * /*ctx*/) override { }
  virtual void exitRestartServer(MySQLParser::RestartServerContext * /*ctx*/) override { }

  virtual void enterExprOr(MySQLParser::ExprOrContext * /*ctx*/) override { }
  virtual void exitExprOr(MySQLParser::ExprOrContext * /*ctx*/) override { }

  virtual void enterExprNot(MySQLParser::ExprNotContext * /*ctx*/) override { }
  virtual void exitExprNot(MySQLParser::ExprNotContext * /*ctx*/) override { }

  virtual void enterExprIs(MySQLParser::ExprIsContext * /*ctx*/) override { }
  virtual void exitExprIs(MySQLParser::ExprIsContext * /*ctx*/) override { }

  virtual void enterExprAnd(MySQLParser::ExprAndContext * /*ctx*/) override { }
  virtual void exitExprAnd(MySQLParser::ExprAndContext * /*ctx*/) override { }

  virtual void enterExprXor(MySQLParser::ExprXorContext * /*ctx*/) override { }
  virtual void exitExprXor(MySQLParser::ExprXorContext * /*ctx*/) override { }

  virtual void enterPrimaryExprPredicate(MySQLParser::PrimaryExprPredicateContext * /*ctx*/) override { }
  virtual void exitPrimaryExprPredicate(MySQLParser::PrimaryExprPredicateContext * /*ctx*/) override { }

  virtual void enterPrimaryExprCompare(MySQLParser::PrimaryExprCompareContext * /*ctx*/) override { }
  virtual void exitPrimaryExprCompare(MySQLParser::PrimaryExprCompareContext * /*ctx*/) override { }

  virtual void enterPrimaryExprAllAny(MySQLParser::PrimaryExprAllAnyContext * /*ctx*/) override { }
  virtual void exitPrimaryExprAllAny(MySQLParser::PrimaryExprAllAnyContext * /*ctx*/) override { }

  virtual void enterPrimaryExprIsNull(MySQLParser::PrimaryExprIsNullContext * /*ctx*/) override { }
  virtual void exitPrimaryExprIsNull(MySQLParser::PrimaryExprIsNullContext * /*ctx*/) override { }

  virtual void enterCompOp(MySQLParser::CompOpContext * /*ctx*/) override { }
  virtual void exitCompOp(MySQLParser::CompOpContext * /*ctx*/) override { }

  virtual void enterPredicate(MySQLParser::PredicateContext * /*ctx*/) override { }
  virtual void exitPredicate(MySQLParser::PredicateContext * /*ctx*/) override { }

  virtual void enterPredicateExprIn(MySQLParser::PredicateExprInContext * /*ctx*/) override { }
  virtual void exitPredicateExprIn(MySQLParser::PredicateExprInContext * /*ctx*/) override { }

  virtual void enterPredicateExprBetween(MySQLParser::PredicateExprBetweenContext * /*ctx*/) override { }
  virtual void exitPredicateExprBetween(MySQLParser::PredicateExprBetweenContext * /*ctx*/) override { }

  virtual void enterPredicateExprLike(MySQLParser::PredicateExprLikeContext * /*ctx*/) override { }
  virtual void exitPredicateExprLike(MySQLParser::PredicateExprLikeContext * /*ctx*/) override { }

  virtual void enterPredicateExprRegex(MySQLParser::PredicateExprRegexContext * /*ctx*/) override { }
  virtual void exitPredicateExprRegex(MySQLParser::PredicateExprRegexContext * /*ctx*/) override { }

  virtual void enterBitExpr(MySQLParser::BitExprContext * /*ctx*/) override { }
  virtual void exitBitExpr(MySQLParser::BitExprContext * /*ctx*/) override { }

  virtual void enterSimpleExprConvert(MySQLParser::SimpleExprConvertContext * /*ctx*/) override { }
  virtual void exitSimpleExprConvert(MySQLParser::SimpleExprConvertContext * /*ctx*/) override { }

  virtual void enterSimpleExprCast(MySQLParser::SimpleExprCastContext * /*ctx*/) override { }
  virtual void exitSimpleExprCast(MySQLParser::SimpleExprCastContext * /*ctx*/) override { }

  virtual void enterSimpleExprUnary(MySQLParser::SimpleExprUnaryContext * /*ctx*/) override { }
  virtual void exitSimpleExprUnary(MySQLParser::SimpleExprUnaryContext * /*ctx*/) override { }

  virtual void enterSimpleExpressionRValue(MySQLParser::SimpleExpressionRValueContext * /*ctx*/) override { }
  virtual void exitSimpleExpressionRValue(MySQLParser::SimpleExpressionRValueContext * /*ctx*/) override { }

  virtual void enterSimpleExprOdbc(MySQLParser::SimpleExprOdbcContext * /*ctx*/) override { }
  virtual void exitSimpleExprOdbc(MySQLParser::SimpleExprOdbcContext * /*ctx*/) override { }

  virtual void enterSimpleExprRuntimeFunction(MySQLParser::SimpleExprRuntimeFunctionContext * /*ctx*/) override { }
  virtual void exitSimpleExprRuntimeFunction(MySQLParser::SimpleExprRuntimeFunctionContext * /*ctx*/) override { }

  virtual void enterSimpleExprFunction(MySQLParser::SimpleExprFunctionContext * /*ctx*/) override { }
  virtual void exitSimpleExprFunction(MySQLParser::SimpleExprFunctionContext * /*ctx*/) override { }

  virtual void enterSimpleExprCollate(MySQLParser::SimpleExprCollateContext * /*ctx*/) override { }
  virtual void exitSimpleExprCollate(MySQLParser::SimpleExprCollateContext * /*ctx*/) override { }

  virtual void enterSimpleExprMatch(MySQLParser::SimpleExprMatchContext * /*ctx*/) override { }
  virtual void exitSimpleExprMatch(MySQLParser::SimpleExprMatchContext * /*ctx*/) override { }

  virtual void enterSimpleExprWindowingFunction(MySQLParser::SimpleExprWindowingFunctionContext * /*ctx*/) override { }
  virtual void exitSimpleExprWindowingFunction(MySQLParser::SimpleExprWindowingFunctionContext * /*ctx*/) override { }

  virtual void enterSimpleExprBinary(MySQLParser::SimpleExprBinaryContext * /*ctx*/) override { }
  virtual void exitSimpleExprBinary(MySQLParser::SimpleExprBinaryContext * /*ctx*/) override { }

  virtual void enterSimpleExprColumnRef(MySQLParser::SimpleExprColumnRefContext * /*ctx*/) override { }
  virtual void exitSimpleExprColumnRef(MySQLParser::SimpleExprColumnRefContext * /*ctx*/) override { }

  virtual void enterSimpleExprParamMarker(MySQLParser::SimpleExprParamMarkerContext * /*ctx*/) override { }
  virtual void exitSimpleExprParamMarker(MySQLParser::SimpleExprParamMarkerContext * /*ctx*/) override { }

  virtual void enterSimpleExprSum(MySQLParser::SimpleExprSumContext * /*ctx*/) override { }
  virtual void exitSimpleExprSum(MySQLParser::SimpleExprSumContext * /*ctx*/) override { }

  virtual void enterSimpleExprCastTime(MySQLParser::SimpleExprCastTimeContext * /*ctx*/) override { }
  virtual void exitSimpleExprCastTime(MySQLParser::SimpleExprCastTimeContext * /*ctx*/) override { }

  virtual void enterSimpleExprConvertUsing(MySQLParser::SimpleExprConvertUsingContext * /*ctx*/) override { }
  virtual void exitSimpleExprConvertUsing(MySQLParser::SimpleExprConvertUsingContext * /*ctx*/) override { }

  virtual void enterSimpleExprSubQuery(MySQLParser::SimpleExprSubQueryContext * /*ctx*/) override { }
  virtual void exitSimpleExprSubQuery(MySQLParser::SimpleExprSubQueryContext * /*ctx*/) override { }

  virtual void enterSimpleExprGroupingOperation(MySQLParser::SimpleExprGroupingOperationContext * /*ctx*/) override { }
  virtual void exitSimpleExprGroupingOperation(MySQLParser::SimpleExprGroupingOperationContext * /*ctx*/) override { }

  virtual void enterSimpleExprNot(MySQLParser::SimpleExprNotContext * /*ctx*/) override { }
  virtual void exitSimpleExprNot(MySQLParser::SimpleExprNotContext * /*ctx*/) override { }

  virtual void enterSimpleExprValues(MySQLParser::SimpleExprValuesContext * /*ctx*/) override { }
  virtual void exitSimpleExprValues(MySQLParser::SimpleExprValuesContext * /*ctx*/) override { }

  virtual void enterSimpleExprUserVariableAssignment(MySQLParser::SimpleExprUserVariableAssignmentContext * /*ctx*/) override { }
  virtual void exitSimpleExprUserVariableAssignment(MySQLParser::SimpleExprUserVariableAssignmentContext * /*ctx*/) override { }

  virtual void enterSimpleExprDefault(MySQLParser::SimpleExprDefaultContext * /*ctx*/) override { }
  virtual void exitSimpleExprDefault(MySQLParser::SimpleExprDefaultContext * /*ctx*/) override { }

  virtual void enterSimpleExprList(MySQLParser::SimpleExprListContext * /*ctx*/) override { }
  virtual void exitSimpleExprList(MySQLParser::SimpleExprListContext * /*ctx*/) override { }

  virtual void enterSimpleExprInterval(MySQLParser::SimpleExprIntervalContext * /*ctx*/) override { }
  virtual void exitSimpleExprInterval(MySQLParser::SimpleExprIntervalContext * /*ctx*/) override { }

  virtual void enterSimpleExprCase(MySQLParser::SimpleExprCaseContext * /*ctx*/) override { }
  virtual void exitSimpleExprCase(MySQLParser::SimpleExprCaseContext * /*ctx*/) override { }

  virtual void enterSimpleExprConcat(MySQLParser::SimpleExprConcatContext * /*ctx*/) override { }
  virtual void exitSimpleExprConcat(MySQLParser::SimpleExprConcatContext * /*ctx*/) override { }

  virtual void enterSimpleExprLiteral(MySQLParser::SimpleExprLiteralContext * /*ctx*/) override { }
  virtual void exitSimpleExprLiteral(MySQLParser::SimpleExprLiteralContext * /*ctx*/) override { }

  virtual void enterArrayCast(MySQLParser::ArrayCastContext * /*ctx*/) override { }
  virtual void exitArrayCast(MySQLParser::ArrayCastContext * /*ctx*/) override { }

  virtual void enterJsonOperator(MySQLParser::JsonOperatorContext * /*ctx*/) override { }
  virtual void exitJsonOperator(MySQLParser::JsonOperatorContext * /*ctx*/) override { }

  virtual void enterSumExpr(MySQLParser::SumExprContext * /*ctx*/) override { }
  virtual void exitSumExpr(MySQLParser::SumExprContext * /*ctx*/) override { }

  virtual void enterGroupingOperation(MySQLParser::GroupingOperationContext * /*ctx*/) override { }
  virtual void exitGroupingOperation(MySQLParser::GroupingOperationContext * /*ctx*/) override { }

  virtual void enterWindowFunctionCall(MySQLParser::WindowFunctionCallContext * /*ctx*/) override { }
  virtual void exitWindowFunctionCall(MySQLParser::WindowFunctionCallContext * /*ctx*/) override { }

  virtual void enterWindowingClause(MySQLParser::WindowingClauseContext * /*ctx*/) override { }
  virtual void exitWindowingClause(MySQLParser::WindowingClauseContext * /*ctx*/) override { }

  virtual void enterLeadLagInfo(MySQLParser::LeadLagInfoContext * /*ctx*/) override { }
  virtual void exitLeadLagInfo(MySQLParser::LeadLagInfoContext * /*ctx*/) override { }

  virtual void enterStableInteger(MySQLParser::StableIntegerContext * /*ctx*/) override { }
  virtual void exitStableInteger(MySQLParser::StableIntegerContext * /*ctx*/) override { }

  virtual void enterParamOrVar(MySQLParser::ParamOrVarContext * /*ctx*/) override { }
  virtual void exitParamOrVar(MySQLParser::ParamOrVarContext * /*ctx*/) override { }

  virtual void enterNullTreatment(MySQLParser::NullTreatmentContext * /*ctx*/) override { }
  virtual void exitNullTreatment(MySQLParser::NullTreatmentContext * /*ctx*/) override { }

  virtual void enterJsonFunction(MySQLParser::JsonFunctionContext * /*ctx*/) override { }
  virtual void exitJsonFunction(MySQLParser::JsonFunctionContext * /*ctx*/) override { }

  virtual void enterInSumExpr(MySQLParser::InSumExprContext * /*ctx*/) override { }
  virtual void exitInSumExpr(MySQLParser::InSumExprContext * /*ctx*/) override { }

  virtual void enterIdentListArg(MySQLParser::IdentListArgContext * /*ctx*/) override { }
  virtual void exitIdentListArg(MySQLParser::IdentListArgContext * /*ctx*/) override { }

  virtual void enterIdentList(MySQLParser::IdentListContext * /*ctx*/) override { }
  virtual void exitIdentList(MySQLParser::IdentListContext * /*ctx*/) override { }

  virtual void enterFulltextOptions(MySQLParser::FulltextOptionsContext * /*ctx*/) override { }
  virtual void exitFulltextOptions(MySQLParser::FulltextOptionsContext * /*ctx*/) override { }

  virtual void enterRuntimeFunctionCall(MySQLParser::RuntimeFunctionCallContext * /*ctx*/) override { }
  virtual void exitRuntimeFunctionCall(MySQLParser::RuntimeFunctionCallContext * /*ctx*/) override { }

  virtual void enterReturningType(MySQLParser::ReturningTypeContext * /*ctx*/) override { }
  virtual void exitReturningType(MySQLParser::ReturningTypeContext * /*ctx*/) override { }

  virtual void enterGeometryFunction(MySQLParser::GeometryFunctionContext * /*ctx*/) override { }
  virtual void exitGeometryFunction(MySQLParser::GeometryFunctionContext * /*ctx*/) override { }

  virtual void enterTimeFunctionParameters(MySQLParser::TimeFunctionParametersContext * /*ctx*/) override { }
  virtual void exitTimeFunctionParameters(MySQLParser::TimeFunctionParametersContext * /*ctx*/) override { }

  virtual void enterFractionalPrecision(MySQLParser::FractionalPrecisionContext * /*ctx*/) override { }
  virtual void exitFractionalPrecision(MySQLParser::FractionalPrecisionContext * /*ctx*/) override { }

  virtual void enterWeightStringLevels(MySQLParser::WeightStringLevelsContext * /*ctx*/) override { }
  virtual void exitWeightStringLevels(MySQLParser::WeightStringLevelsContext * /*ctx*/) override { }

  virtual void enterWeightStringLevelListItem(MySQLParser::WeightStringLevelListItemContext * /*ctx*/) override { }
  virtual void exitWeightStringLevelListItem(MySQLParser::WeightStringLevelListItemContext * /*ctx*/) override { }

  virtual void enterDateTimeTtype(MySQLParser::DateTimeTtypeContext * /*ctx*/) override { }
  virtual void exitDateTimeTtype(MySQLParser::DateTimeTtypeContext * /*ctx*/) override { }

  virtual void enterTrimFunction(MySQLParser::TrimFunctionContext * /*ctx*/) override { }
  virtual void exitTrimFunction(MySQLParser::TrimFunctionContext * /*ctx*/) override { }

  virtual void enterSubstringFunction(MySQLParser::SubstringFunctionContext * /*ctx*/) override { }
  virtual void exitSubstringFunction(MySQLParser::SubstringFunctionContext * /*ctx*/) override { }

  virtual void enterFunctionCall(MySQLParser::FunctionCallContext * /*ctx*/) override { }
  virtual void exitFunctionCall(MySQLParser::FunctionCallContext * /*ctx*/) override { }

  virtual void enterUdfExprList(MySQLParser::UdfExprListContext * /*ctx*/) override { }
  virtual void exitUdfExprList(MySQLParser::UdfExprListContext * /*ctx*/) override { }

  virtual void enterUdfExpr(MySQLParser::UdfExprContext * /*ctx*/) override { }
  virtual void exitUdfExpr(MySQLParser::UdfExprContext * /*ctx*/) override { }

  virtual void enterUserVariable(MySQLParser::UserVariableContext * /*ctx*/) override { }
  virtual void exitUserVariable(MySQLParser::UserVariableContext * /*ctx*/) override { }

  virtual void enterUserVariableIdentifier(MySQLParser::UserVariableIdentifierContext * /*ctx*/) override { }
  virtual void exitUserVariableIdentifier(MySQLParser::UserVariableIdentifierContext * /*ctx*/) override { }

  virtual void enterInExpressionUserVariableAssignment(MySQLParser::InExpressionUserVariableAssignmentContext * /*ctx*/) override { }
  virtual void exitInExpressionUserVariableAssignment(MySQLParser::InExpressionUserVariableAssignmentContext * /*ctx*/) override { }

  virtual void enterRvalueSystemOrUserVariable(MySQLParser::RvalueSystemOrUserVariableContext * /*ctx*/) override { }
  virtual void exitRvalueSystemOrUserVariable(MySQLParser::RvalueSystemOrUserVariableContext * /*ctx*/) override { }

  virtual void enterLvalueVariable(MySQLParser::LvalueVariableContext * /*ctx*/) override { }
  virtual void exitLvalueVariable(MySQLParser::LvalueVariableContext * /*ctx*/) override { }

  virtual void enterRvalueSystemVariable(MySQLParser::RvalueSystemVariableContext * /*ctx*/) override { }
  virtual void exitRvalueSystemVariable(MySQLParser::RvalueSystemVariableContext * /*ctx*/) override { }

  virtual void enterWhenExpression(MySQLParser::WhenExpressionContext * /*ctx*/) override { }
  virtual void exitWhenExpression(MySQLParser::WhenExpressionContext * /*ctx*/) override { }

  virtual void enterThenExpression(MySQLParser::ThenExpressionContext * /*ctx*/) override { }
  virtual void exitThenExpression(MySQLParser::ThenExpressionContext * /*ctx*/) override { }

  virtual void enterElseExpression(MySQLParser::ElseExpressionContext * /*ctx*/) override { }
  virtual void exitElseExpression(MySQLParser::ElseExpressionContext * /*ctx*/) override { }

  virtual void enterCastType(MySQLParser::CastTypeContext * /*ctx*/) override { }
  virtual void exitCastType(MySQLParser::CastTypeContext * /*ctx*/) override { }

  virtual void enterExprList(MySQLParser::ExprListContext * /*ctx*/) override { }
  virtual void exitExprList(MySQLParser::ExprListContext * /*ctx*/) override { }

  virtual void enterCharset(MySQLParser::CharsetContext * /*ctx*/) override { }
  virtual void exitCharset(MySQLParser::CharsetContext * /*ctx*/) override { }

  virtual void enterNotRule(MySQLParser::NotRuleContext * /*ctx*/) override { }
  virtual void exitNotRule(MySQLParser::NotRuleContext * /*ctx*/) override { }

  virtual void enterNot2Rule(MySQLParser::Not2RuleContext * /*ctx*/) override { }
  virtual void exitNot2Rule(MySQLParser::Not2RuleContext * /*ctx*/) override { }

  virtual void enterInterval(MySQLParser::IntervalContext * /*ctx*/) override { }
  virtual void exitInterval(MySQLParser::IntervalContext * /*ctx*/) override { }

  virtual void enterIntervalTimeStamp(MySQLParser::IntervalTimeStampContext * /*ctx*/) override { }
  virtual void exitIntervalTimeStamp(MySQLParser::IntervalTimeStampContext * /*ctx*/) override { }

  virtual void enterExprListWithParentheses(MySQLParser::ExprListWithParenthesesContext * /*ctx*/) override { }
  virtual void exitExprListWithParentheses(MySQLParser::ExprListWithParenthesesContext * /*ctx*/) override { }

  virtual void enterExprWithParentheses(MySQLParser::ExprWithParenthesesContext * /*ctx*/) override { }
  virtual void exitExprWithParentheses(MySQLParser::ExprWithParenthesesContext * /*ctx*/) override { }

  virtual void enterSimpleExprWithParentheses(MySQLParser::SimpleExprWithParenthesesContext * /*ctx*/) override { }
  virtual void exitSimpleExprWithParentheses(MySQLParser::SimpleExprWithParenthesesContext * /*ctx*/) override { }

  virtual void enterOrderList(MySQLParser::OrderListContext * /*ctx*/) override { }
  virtual void exitOrderList(MySQLParser::OrderListContext * /*ctx*/) override { }

  virtual void enterOrderExpression(MySQLParser::OrderExpressionContext * /*ctx*/) override { }
  virtual void exitOrderExpression(MySQLParser::OrderExpressionContext * /*ctx*/) override { }

  virtual void enterGroupList(MySQLParser::GroupListContext * /*ctx*/) override { }
  virtual void exitGroupList(MySQLParser::GroupListContext * /*ctx*/) override { }

  virtual void enterGroupingExpression(MySQLParser::GroupingExpressionContext * /*ctx*/) override { }
  virtual void exitGroupingExpression(MySQLParser::GroupingExpressionContext * /*ctx*/) override { }

  virtual void enterChannel(MySQLParser::ChannelContext * /*ctx*/) override { }
  virtual void exitChannel(MySQLParser::ChannelContext * /*ctx*/) override { }

  virtual void enterCompoundStatement(MySQLParser::CompoundStatementContext * /*ctx*/) override { }
  virtual void exitCompoundStatement(MySQLParser::CompoundStatementContext * /*ctx*/) override { }

  virtual void enterReturnStatement(MySQLParser::ReturnStatementContext * /*ctx*/) override { }
  virtual void exitReturnStatement(MySQLParser::ReturnStatementContext * /*ctx*/) override { }

  virtual void enterIfStatement(MySQLParser::IfStatementContext * /*ctx*/) override { }
  virtual void exitIfStatement(MySQLParser::IfStatementContext * /*ctx*/) override { }

  virtual void enterIfBody(MySQLParser::IfBodyContext * /*ctx*/) override { }
  virtual void exitIfBody(MySQLParser::IfBodyContext * /*ctx*/) override { }

  virtual void enterThenStatement(MySQLParser::ThenStatementContext * /*ctx*/) override { }
  virtual void exitThenStatement(MySQLParser::ThenStatementContext * /*ctx*/) override { }

  virtual void enterCompoundStatementList(MySQLParser::CompoundStatementListContext * /*ctx*/) override { }
  virtual void exitCompoundStatementList(MySQLParser::CompoundStatementListContext * /*ctx*/) override { }

  virtual void enterCaseStatement(MySQLParser::CaseStatementContext * /*ctx*/) override { }
  virtual void exitCaseStatement(MySQLParser::CaseStatementContext * /*ctx*/) override { }

  virtual void enterElseStatement(MySQLParser::ElseStatementContext * /*ctx*/) override { }
  virtual void exitElseStatement(MySQLParser::ElseStatementContext * /*ctx*/) override { }

  virtual void enterLabeledBlock(MySQLParser::LabeledBlockContext * /*ctx*/) override { }
  virtual void exitLabeledBlock(MySQLParser::LabeledBlockContext * /*ctx*/) override { }

  virtual void enterUnlabeledBlock(MySQLParser::UnlabeledBlockContext * /*ctx*/) override { }
  virtual void exitUnlabeledBlock(MySQLParser::UnlabeledBlockContext * /*ctx*/) override { }

  virtual void enterLabel(MySQLParser::LabelContext * /*ctx*/) override { }
  virtual void exitLabel(MySQLParser::LabelContext * /*ctx*/) override { }

  virtual void enterBeginEndBlock(MySQLParser::BeginEndBlockContext * /*ctx*/) override { }
  virtual void exitBeginEndBlock(MySQLParser::BeginEndBlockContext * /*ctx*/) override { }

  virtual void enterLabeledControl(MySQLParser::LabeledControlContext * /*ctx*/) override { }
  virtual void exitLabeledControl(MySQLParser::LabeledControlContext * /*ctx*/) override { }

  virtual void enterUnlabeledControl(MySQLParser::UnlabeledControlContext * /*ctx*/) override { }
  virtual void exitUnlabeledControl(MySQLParser::UnlabeledControlContext * /*ctx*/) override { }

  virtual void enterLoopBlock(MySQLParser::LoopBlockContext * /*ctx*/) override { }
  virtual void exitLoopBlock(MySQLParser::LoopBlockContext * /*ctx*/) override { }

  virtual void enterWhileDoBlock(MySQLParser::WhileDoBlockContext * /*ctx*/) override { }
  virtual void exitWhileDoBlock(MySQLParser::WhileDoBlockContext * /*ctx*/) override { }

  virtual void enterRepeatUntilBlock(MySQLParser::RepeatUntilBlockContext * /*ctx*/) override { }
  virtual void exitRepeatUntilBlock(MySQLParser::RepeatUntilBlockContext * /*ctx*/) override { }

  virtual void enterSpDeclarations(MySQLParser::SpDeclarationsContext * /*ctx*/) override { }
  virtual void exitSpDeclarations(MySQLParser::SpDeclarationsContext * /*ctx*/) override { }

  virtual void enterSpDeclaration(MySQLParser::SpDeclarationContext * /*ctx*/) override { }
  virtual void exitSpDeclaration(MySQLParser::SpDeclarationContext * /*ctx*/) override { }

  virtual void enterVariableDeclaration(MySQLParser::VariableDeclarationContext * /*ctx*/) override { }
  virtual void exitVariableDeclaration(MySQLParser::VariableDeclarationContext * /*ctx*/) override { }

  virtual void enterConditionDeclaration(MySQLParser::ConditionDeclarationContext * /*ctx*/) override { }
  virtual void exitConditionDeclaration(MySQLParser::ConditionDeclarationContext * /*ctx*/) override { }

  virtual void enterSpCondition(MySQLParser::SpConditionContext * /*ctx*/) override { }
  virtual void exitSpCondition(MySQLParser::SpConditionContext * /*ctx*/) override { }

  virtual void enterSqlstate(MySQLParser::SqlstateContext * /*ctx*/) override { }
  virtual void exitSqlstate(MySQLParser::SqlstateContext * /*ctx*/) override { }

  virtual void enterHandlerDeclaration(MySQLParser::HandlerDeclarationContext * /*ctx*/) override { }
  virtual void exitHandlerDeclaration(MySQLParser::HandlerDeclarationContext * /*ctx*/) override { }

  virtual void enterHandlerCondition(MySQLParser::HandlerConditionContext * /*ctx*/) override { }
  virtual void exitHandlerCondition(MySQLParser::HandlerConditionContext * /*ctx*/) override { }

  virtual void enterCursorDeclaration(MySQLParser::CursorDeclarationContext * /*ctx*/) override { }
  virtual void exitCursorDeclaration(MySQLParser::CursorDeclarationContext * /*ctx*/) override { }

  virtual void enterIterateStatement(MySQLParser::IterateStatementContext * /*ctx*/) override { }
  virtual void exitIterateStatement(MySQLParser::IterateStatementContext * /*ctx*/) override { }

  virtual void enterLeaveStatement(MySQLParser::LeaveStatementContext * /*ctx*/) override { }
  virtual void exitLeaveStatement(MySQLParser::LeaveStatementContext * /*ctx*/) override { }

  virtual void enterGetDiagnosticsStatement(MySQLParser::GetDiagnosticsStatementContext * /*ctx*/) override { }
  virtual void exitGetDiagnosticsStatement(MySQLParser::GetDiagnosticsStatementContext * /*ctx*/) override { }

  virtual void enterSignalAllowedExpr(MySQLParser::SignalAllowedExprContext * /*ctx*/) override { }
  virtual void exitSignalAllowedExpr(MySQLParser::SignalAllowedExprContext * /*ctx*/) override { }

  virtual void enterStatementInformationItem(MySQLParser::StatementInformationItemContext * /*ctx*/) override { }
  virtual void exitStatementInformationItem(MySQLParser::StatementInformationItemContext * /*ctx*/) override { }

  virtual void enterConditionInformationItem(MySQLParser::ConditionInformationItemContext * /*ctx*/) override { }
  virtual void exitConditionInformationItem(MySQLParser::ConditionInformationItemContext * /*ctx*/) override { }

  virtual void enterSignalInformationItemName(MySQLParser::SignalInformationItemNameContext * /*ctx*/) override { }
  virtual void exitSignalInformationItemName(MySQLParser::SignalInformationItemNameContext * /*ctx*/) override { }

  virtual void enterSignalStatement(MySQLParser::SignalStatementContext * /*ctx*/) override { }
  virtual void exitSignalStatement(MySQLParser::SignalStatementContext * /*ctx*/) override { }

  virtual void enterResignalStatement(MySQLParser::ResignalStatementContext * /*ctx*/) override { }
  virtual void exitResignalStatement(MySQLParser::ResignalStatementContext * /*ctx*/) override { }

  virtual void enterSignalInformationItem(MySQLParser::SignalInformationItemContext * /*ctx*/) override { }
  virtual void exitSignalInformationItem(MySQLParser::SignalInformationItemContext * /*ctx*/) override { }

  virtual void enterCursorOpen(MySQLParser::CursorOpenContext * /*ctx*/) override { }
  virtual void exitCursorOpen(MySQLParser::CursorOpenContext * /*ctx*/) override { }

  virtual void enterCursorClose(MySQLParser::CursorCloseContext * /*ctx*/) override { }
  virtual void exitCursorClose(MySQLParser::CursorCloseContext * /*ctx*/) override { }

  virtual void enterCursorFetch(MySQLParser::CursorFetchContext * /*ctx*/) override { }
  virtual void exitCursorFetch(MySQLParser::CursorFetchContext * /*ctx*/) override { }

  virtual void enterSchedule(MySQLParser::ScheduleContext * /*ctx*/) override { }
  virtual void exitSchedule(MySQLParser::ScheduleContext * /*ctx*/) override { }

  virtual void enterColumnDefinition(MySQLParser::ColumnDefinitionContext * /*ctx*/) override { }
  virtual void exitColumnDefinition(MySQLParser::ColumnDefinitionContext * /*ctx*/) override { }

  virtual void enterCheckOrReferences(MySQLParser::CheckOrReferencesContext * /*ctx*/) override { }
  virtual void exitCheckOrReferences(MySQLParser::CheckOrReferencesContext * /*ctx*/) override { }

  virtual void enterCheckConstraint(MySQLParser::CheckConstraintContext * /*ctx*/) override { }
  virtual void exitCheckConstraint(MySQLParser::CheckConstraintContext * /*ctx*/) override { }

  virtual void enterConstraintEnforcement(MySQLParser::ConstraintEnforcementContext * /*ctx*/) override { }
  virtual void exitConstraintEnforcement(MySQLParser::ConstraintEnforcementContext * /*ctx*/) override { }

  virtual void enterTableConstraintDef(MySQLParser::TableConstraintDefContext * /*ctx*/) override { }
  virtual void exitTableConstraintDef(MySQLParser::TableConstraintDefContext * /*ctx*/) override { }

  virtual void enterConstraintName(MySQLParser::ConstraintNameContext * /*ctx*/) override { }
  virtual void exitConstraintName(MySQLParser::ConstraintNameContext * /*ctx*/) override { }

  virtual void enterFieldDefinition(MySQLParser::FieldDefinitionContext * /*ctx*/) override { }
  virtual void exitFieldDefinition(MySQLParser::FieldDefinitionContext * /*ctx*/) override { }

  virtual void enterColumnAttribute(MySQLParser::ColumnAttributeContext * /*ctx*/) override { }
  virtual void exitColumnAttribute(MySQLParser::ColumnAttributeContext * /*ctx*/) override { }

  virtual void enterColumnFormat(MySQLParser::ColumnFormatContext * /*ctx*/) override { }
  virtual void exitColumnFormat(MySQLParser::ColumnFormatContext * /*ctx*/) override { }

  virtual void enterStorageMedia(MySQLParser::StorageMediaContext * /*ctx*/) override { }
  virtual void exitStorageMedia(MySQLParser::StorageMediaContext * /*ctx*/) override { }

  virtual void enterNow(MySQLParser::NowContext * /*ctx*/) override { }
  virtual void exitNow(MySQLParser::NowContext * /*ctx*/) override { }

  virtual void enterNowOrSignedLiteral(MySQLParser::NowOrSignedLiteralContext * /*ctx*/) override { }
  virtual void exitNowOrSignedLiteral(MySQLParser::NowOrSignedLiteralContext * /*ctx*/) override { }

  virtual void enterGcolAttribute(MySQLParser::GcolAttributeContext * /*ctx*/) override { }
  virtual void exitGcolAttribute(MySQLParser::GcolAttributeContext * /*ctx*/) override { }

  virtual void enterReferences(MySQLParser::ReferencesContext * /*ctx*/) override { }
  virtual void exitReferences(MySQLParser::ReferencesContext * /*ctx*/) override { }

  virtual void enterDeleteOption(MySQLParser::DeleteOptionContext * /*ctx*/) override { }
  virtual void exitDeleteOption(MySQLParser::DeleteOptionContext * /*ctx*/) override { }

  virtual void enterKeyList(MySQLParser::KeyListContext * /*ctx*/) override { }
  virtual void exitKeyList(MySQLParser::KeyListContext * /*ctx*/) override { }

  virtual void enterKeyPart(MySQLParser::KeyPartContext * /*ctx*/) override { }
  virtual void exitKeyPart(MySQLParser::KeyPartContext * /*ctx*/) override { }

  virtual void enterKeyListWithExpression(MySQLParser::KeyListWithExpressionContext * /*ctx*/) override { }
  virtual void exitKeyListWithExpression(MySQLParser::KeyListWithExpressionContext * /*ctx*/) override { }

  virtual void enterKeyPartOrExpression(MySQLParser::KeyPartOrExpressionContext * /*ctx*/) override { }
  virtual void exitKeyPartOrExpression(MySQLParser::KeyPartOrExpressionContext * /*ctx*/) override { }

  virtual void enterIndexType(MySQLParser::IndexTypeContext * /*ctx*/) override { }
  virtual void exitIndexType(MySQLParser::IndexTypeContext * /*ctx*/) override { }

  virtual void enterIndexOption(MySQLParser::IndexOptionContext * /*ctx*/) override { }
  virtual void exitIndexOption(MySQLParser::IndexOptionContext * /*ctx*/) override { }

  virtual void enterCommonIndexOption(MySQLParser::CommonIndexOptionContext * /*ctx*/) override { }
  virtual void exitCommonIndexOption(MySQLParser::CommonIndexOptionContext * /*ctx*/) override { }

  virtual void enterVisibility(MySQLParser::VisibilityContext * /*ctx*/) override { }
  virtual void exitVisibility(MySQLParser::VisibilityContext * /*ctx*/) override { }

  virtual void enterIndexTypeClause(MySQLParser::IndexTypeClauseContext * /*ctx*/) override { }
  virtual void exitIndexTypeClause(MySQLParser::IndexTypeClauseContext * /*ctx*/) override { }

  virtual void enterFulltextIndexOption(MySQLParser::FulltextIndexOptionContext * /*ctx*/) override { }
  virtual void exitFulltextIndexOption(MySQLParser::FulltextIndexOptionContext * /*ctx*/) override { }

  virtual void enterSpatialIndexOption(MySQLParser::SpatialIndexOptionContext * /*ctx*/) override { }
  virtual void exitSpatialIndexOption(MySQLParser::SpatialIndexOptionContext * /*ctx*/) override { }

  virtual void enterDataTypeDefinition(MySQLParser::DataTypeDefinitionContext * /*ctx*/) override { }
  virtual void exitDataTypeDefinition(MySQLParser::DataTypeDefinitionContext * /*ctx*/) override { }

  virtual void enterDataType(MySQLParser::DataTypeContext * /*ctx*/) override { }
  virtual void exitDataType(MySQLParser::DataTypeContext * /*ctx*/) override { }

  virtual void enterNchar(MySQLParser::NcharContext * /*ctx*/) override { }
  virtual void exitNchar(MySQLParser::NcharContext * /*ctx*/) override { }

  virtual void enterRealType(MySQLParser::RealTypeContext * /*ctx*/) override { }
  virtual void exitRealType(MySQLParser::RealTypeContext * /*ctx*/) override { }

  virtual void enterFieldLength(MySQLParser::FieldLengthContext * /*ctx*/) override { }
  virtual void exitFieldLength(MySQLParser::FieldLengthContext * /*ctx*/) override { }

  virtual void enterFieldOptions(MySQLParser::FieldOptionsContext * /*ctx*/) override { }
  virtual void exitFieldOptions(MySQLParser::FieldOptionsContext * /*ctx*/) override { }

  virtual void enterCharsetWithOptBinary(MySQLParser::CharsetWithOptBinaryContext * /*ctx*/) override { }
  virtual void exitCharsetWithOptBinary(MySQLParser::CharsetWithOptBinaryContext * /*ctx*/) override { }

  virtual void enterAscii(MySQLParser::AsciiContext * /*ctx*/) override { }
  virtual void exitAscii(MySQLParser::AsciiContext * /*ctx*/) override { }

  virtual void enterUnicode(MySQLParser::UnicodeContext * /*ctx*/) override { }
  virtual void exitUnicode(MySQLParser::UnicodeContext * /*ctx*/) override { }

  virtual void enterWsNumCodepoints(MySQLParser::WsNumCodepointsContext * /*ctx*/) override { }
  virtual void exitWsNumCodepoints(MySQLParser::WsNumCodepointsContext * /*ctx*/) override { }

  virtual void enterTypeDatetimePrecision(MySQLParser::TypeDatetimePrecisionContext * /*ctx*/) override { }
  virtual void exitTypeDatetimePrecision(MySQLParser::TypeDatetimePrecisionContext * /*ctx*/) override { }

  virtual void enterFunctionDatetimePrecision(MySQLParser::FunctionDatetimePrecisionContext * /*ctx*/) override { }
  virtual void exitFunctionDatetimePrecision(MySQLParser::FunctionDatetimePrecisionContext * /*ctx*/) override { }

  virtual void enterCharsetName(MySQLParser::CharsetNameContext * /*ctx*/) override { }
  virtual void exitCharsetName(MySQLParser::CharsetNameContext * /*ctx*/) override { }

  virtual void enterCollationName(MySQLParser::CollationNameContext * /*ctx*/) override { }
  virtual void exitCollationName(MySQLParser::CollationNameContext * /*ctx*/) override { }

  virtual void enterCreateTableOptions(MySQLParser::CreateTableOptionsContext * /*ctx*/) override { }
  virtual void exitCreateTableOptions(MySQLParser::CreateTableOptionsContext * /*ctx*/) override { }

  virtual void enterCreateTableOptionsEtc(MySQLParser::CreateTableOptionsEtcContext * /*ctx*/) override { }
  virtual void exitCreateTableOptionsEtc(MySQLParser::CreateTableOptionsEtcContext * /*ctx*/) override { }

  virtual void enterCreatePartitioningEtc(MySQLParser::CreatePartitioningEtcContext * /*ctx*/) override { }
  virtual void exitCreatePartitioningEtc(MySQLParser::CreatePartitioningEtcContext * /*ctx*/) override { }

  virtual void enterCreateTableOptionsSpaceSeparated(MySQLParser::CreateTableOptionsSpaceSeparatedContext * /*ctx*/) override { }
  virtual void exitCreateTableOptionsSpaceSeparated(MySQLParser::CreateTableOptionsSpaceSeparatedContext * /*ctx*/) override { }

  virtual void enterCreateTableOption(MySQLParser::CreateTableOptionContext * /*ctx*/) override { }
  virtual void exitCreateTableOption(MySQLParser::CreateTableOptionContext * /*ctx*/) override { }

  virtual void enterTernaryOption(MySQLParser::TernaryOptionContext * /*ctx*/) override { }
  virtual void exitTernaryOption(MySQLParser::TernaryOptionContext * /*ctx*/) override { }

  virtual void enterDefaultCollation(MySQLParser::DefaultCollationContext * /*ctx*/) override { }
  virtual void exitDefaultCollation(MySQLParser::DefaultCollationContext * /*ctx*/) override { }

  virtual void enterDefaultEncryption(MySQLParser::DefaultEncryptionContext * /*ctx*/) override { }
  virtual void exitDefaultEncryption(MySQLParser::DefaultEncryptionContext * /*ctx*/) override { }

  virtual void enterDefaultCharset(MySQLParser::DefaultCharsetContext * /*ctx*/) override { }
  virtual void exitDefaultCharset(MySQLParser::DefaultCharsetContext * /*ctx*/) override { }

  virtual void enterPartitionClause(MySQLParser::PartitionClauseContext * /*ctx*/) override { }
  virtual void exitPartitionClause(MySQLParser::PartitionClauseContext * /*ctx*/) override { }

  virtual void enterPartitionDefKey(MySQLParser::PartitionDefKeyContext * /*ctx*/) override { }
  virtual void exitPartitionDefKey(MySQLParser::PartitionDefKeyContext * /*ctx*/) override { }

  virtual void enterPartitionDefHash(MySQLParser::PartitionDefHashContext * /*ctx*/) override { }
  virtual void exitPartitionDefHash(MySQLParser::PartitionDefHashContext * /*ctx*/) override { }

  virtual void enterPartitionDefRangeList(MySQLParser::PartitionDefRangeListContext * /*ctx*/) override { }
  virtual void exitPartitionDefRangeList(MySQLParser::PartitionDefRangeListContext * /*ctx*/) override { }

  virtual void enterSubPartitions(MySQLParser::SubPartitionsContext * /*ctx*/) override { }
  virtual void exitSubPartitions(MySQLParser::SubPartitionsContext * /*ctx*/) override { }

  virtual void enterPartitionKeyAlgorithm(MySQLParser::PartitionKeyAlgorithmContext * /*ctx*/) override { }
  virtual void exitPartitionKeyAlgorithm(MySQLParser::PartitionKeyAlgorithmContext * /*ctx*/) override { }

  virtual void enterPartitionDefinitions(MySQLParser::PartitionDefinitionsContext * /*ctx*/) override { }
  virtual void exitPartitionDefinitions(MySQLParser::PartitionDefinitionsContext * /*ctx*/) override { }

  virtual void enterPartitionDefinition(MySQLParser::PartitionDefinitionContext * /*ctx*/) override { }
  virtual void exitPartitionDefinition(MySQLParser::PartitionDefinitionContext * /*ctx*/) override { }

  virtual void enterPartitionValuesIn(MySQLParser::PartitionValuesInContext * /*ctx*/) override { }
  virtual void exitPartitionValuesIn(MySQLParser::PartitionValuesInContext * /*ctx*/) override { }

  virtual void enterPartitionOption(MySQLParser::PartitionOptionContext * /*ctx*/) override { }
  virtual void exitPartitionOption(MySQLParser::PartitionOptionContext * /*ctx*/) override { }

  virtual void enterSubpartitionDefinition(MySQLParser::SubpartitionDefinitionContext * /*ctx*/) override { }
  virtual void exitSubpartitionDefinition(MySQLParser::SubpartitionDefinitionContext * /*ctx*/) override { }

  virtual void enterPartitionValueItemListParen(MySQLParser::PartitionValueItemListParenContext * /*ctx*/) override { }
  virtual void exitPartitionValueItemListParen(MySQLParser::PartitionValueItemListParenContext * /*ctx*/) override { }

  virtual void enterPartitionValueItem(MySQLParser::PartitionValueItemContext * /*ctx*/) override { }
  virtual void exitPartitionValueItem(MySQLParser::PartitionValueItemContext * /*ctx*/) override { }

  virtual void enterDefinerClause(MySQLParser::DefinerClauseContext * /*ctx*/) override { }
  virtual void exitDefinerClause(MySQLParser::DefinerClauseContext * /*ctx*/) override { }

  virtual void enterIfExists(MySQLParser::IfExistsContext * /*ctx*/) override { }
  virtual void exitIfExists(MySQLParser::IfExistsContext * /*ctx*/) override { }

  virtual void enterIfExistsIdentifier(MySQLParser::IfExistsIdentifierContext * /*ctx*/) override { }
  virtual void exitIfExistsIdentifier(MySQLParser::IfExistsIdentifierContext * /*ctx*/) override { }

  virtual void enterPersistedVariableIdentifier(MySQLParser::PersistedVariableIdentifierContext * /*ctx*/) override { }
  virtual void exitPersistedVariableIdentifier(MySQLParser::PersistedVariableIdentifierContext * /*ctx*/) override { }

  virtual void enterIfNotExists(MySQLParser::IfNotExistsContext * /*ctx*/) override { }
  virtual void exitIfNotExists(MySQLParser::IfNotExistsContext * /*ctx*/) override { }

  virtual void enterIgnoreUnknownUser(MySQLParser::IgnoreUnknownUserContext * /*ctx*/) override { }
  virtual void exitIgnoreUnknownUser(MySQLParser::IgnoreUnknownUserContext * /*ctx*/) override { }

  virtual void enterProcedureParameter(MySQLParser::ProcedureParameterContext * /*ctx*/) override { }
  virtual void exitProcedureParameter(MySQLParser::ProcedureParameterContext * /*ctx*/) override { }

  virtual void enterFunctionParameter(MySQLParser::FunctionParameterContext * /*ctx*/) override { }
  virtual void exitFunctionParameter(MySQLParser::FunctionParameterContext * /*ctx*/) override { }

  virtual void enterCollate(MySQLParser::CollateContext * /*ctx*/) override { }
  virtual void exitCollate(MySQLParser::CollateContext * /*ctx*/) override { }

  virtual void enterTypeWithOptCollate(MySQLParser::TypeWithOptCollateContext * /*ctx*/) override { }
  virtual void exitTypeWithOptCollate(MySQLParser::TypeWithOptCollateContext * /*ctx*/) override { }

  virtual void enterSchemaIdentifierPair(MySQLParser::SchemaIdentifierPairContext * /*ctx*/) override { }
  virtual void exitSchemaIdentifierPair(MySQLParser::SchemaIdentifierPairContext * /*ctx*/) override { }

  virtual void enterViewRefList(MySQLParser::ViewRefListContext * /*ctx*/) override { }
  virtual void exitViewRefList(MySQLParser::ViewRefListContext * /*ctx*/) override { }

  virtual void enterUpdateList(MySQLParser::UpdateListContext * /*ctx*/) override { }
  virtual void exitUpdateList(MySQLParser::UpdateListContext * /*ctx*/) override { }

  virtual void enterUpdateElement(MySQLParser::UpdateElementContext * /*ctx*/) override { }
  virtual void exitUpdateElement(MySQLParser::UpdateElementContext * /*ctx*/) override { }

  virtual void enterCharsetClause(MySQLParser::CharsetClauseContext * /*ctx*/) override { }
  virtual void exitCharsetClause(MySQLParser::CharsetClauseContext * /*ctx*/) override { }

  virtual void enterFieldsClause(MySQLParser::FieldsClauseContext * /*ctx*/) override { }
  virtual void exitFieldsClause(MySQLParser::FieldsClauseContext * /*ctx*/) override { }

  virtual void enterFieldTerm(MySQLParser::FieldTermContext * /*ctx*/) override { }
  virtual void exitFieldTerm(MySQLParser::FieldTermContext * /*ctx*/) override { }

  virtual void enterLinesClause(MySQLParser::LinesClauseContext * /*ctx*/) override { }
  virtual void exitLinesClause(MySQLParser::LinesClauseContext * /*ctx*/) override { }

  virtual void enterLineTerm(MySQLParser::LineTermContext * /*ctx*/) override { }
  virtual void exitLineTerm(MySQLParser::LineTermContext * /*ctx*/) override { }

  virtual void enterUserList(MySQLParser::UserListContext * /*ctx*/) override { }
  virtual void exitUserList(MySQLParser::UserListContext * /*ctx*/) override { }

  virtual void enterCreateUserList(MySQLParser::CreateUserListContext * /*ctx*/) override { }
  virtual void exitCreateUserList(MySQLParser::CreateUserListContext * /*ctx*/) override { }

  virtual void enterCreateUser(MySQLParser::CreateUserContext * /*ctx*/) override { }
  virtual void exitCreateUser(MySQLParser::CreateUserContext * /*ctx*/) override { }

  virtual void enterCreateUserWithMfa(MySQLParser::CreateUserWithMfaContext * /*ctx*/) override { }
  virtual void exitCreateUserWithMfa(MySQLParser::CreateUserWithMfaContext * /*ctx*/) override { }

  virtual void enterIdentification(MySQLParser::IdentificationContext * /*ctx*/) override { }
  virtual void exitIdentification(MySQLParser::IdentificationContext * /*ctx*/) override { }

  virtual void enterIdentifiedByPassword(MySQLParser::IdentifiedByPasswordContext * /*ctx*/) override { }
  virtual void exitIdentifiedByPassword(MySQLParser::IdentifiedByPasswordContext * /*ctx*/) override { }

  virtual void enterIdentifiedByRandomPassword(MySQLParser::IdentifiedByRandomPasswordContext * /*ctx*/) override { }
  virtual void exitIdentifiedByRandomPassword(MySQLParser::IdentifiedByRandomPasswordContext * /*ctx*/) override { }

  virtual void enterIdentifiedWithPlugin(MySQLParser::IdentifiedWithPluginContext * /*ctx*/) override { }
  virtual void exitIdentifiedWithPlugin(MySQLParser::IdentifiedWithPluginContext * /*ctx*/) override { }

  virtual void enterIdentifiedWithPluginAsAuth(MySQLParser::IdentifiedWithPluginAsAuthContext * /*ctx*/) override { }
  virtual void exitIdentifiedWithPluginAsAuth(MySQLParser::IdentifiedWithPluginAsAuthContext * /*ctx*/) override { }

  virtual void enterIdentifiedWithPluginByPassword(MySQLParser::IdentifiedWithPluginByPasswordContext * /*ctx*/) override { }
  virtual void exitIdentifiedWithPluginByPassword(MySQLParser::IdentifiedWithPluginByPasswordContext * /*ctx*/) override { }

  virtual void enterIdentifiedWithPluginByRandomPassword(MySQLParser::IdentifiedWithPluginByRandomPasswordContext * /*ctx*/) override { }
  virtual void exitIdentifiedWithPluginByRandomPassword(MySQLParser::IdentifiedWithPluginByRandomPasswordContext * /*ctx*/) override { }

  virtual void enterInitialAuth(MySQLParser::InitialAuthContext * /*ctx*/) override { }
  virtual void exitInitialAuth(MySQLParser::InitialAuthContext * /*ctx*/) override { }

  virtual void enterRetainCurrentPassword(MySQLParser::RetainCurrentPasswordContext * /*ctx*/) override { }
  virtual void exitRetainCurrentPassword(MySQLParser::RetainCurrentPasswordContext * /*ctx*/) override { }

  virtual void enterDiscardOldPassword(MySQLParser::DiscardOldPasswordContext * /*ctx*/) override { }
  virtual void exitDiscardOldPassword(MySQLParser::DiscardOldPasswordContext * /*ctx*/) override { }

  virtual void enterUserRegistration(MySQLParser::UserRegistrationContext * /*ctx*/) override { }
  virtual void exitUserRegistration(MySQLParser::UserRegistrationContext * /*ctx*/) override { }

  virtual void enterFactor(MySQLParser::FactorContext * /*ctx*/) override { }
  virtual void exitFactor(MySQLParser::FactorContext * /*ctx*/) override { }

  virtual void enterReplacePassword(MySQLParser::ReplacePasswordContext * /*ctx*/) override { }
  virtual void exitReplacePassword(MySQLParser::ReplacePasswordContext * /*ctx*/) override { }

  virtual void enterUserIdentifierOrText(MySQLParser::UserIdentifierOrTextContext * /*ctx*/) override { }
  virtual void exitUserIdentifierOrText(MySQLParser::UserIdentifierOrTextContext * /*ctx*/) override { }

  virtual void enterUser(MySQLParser::UserContext * /*ctx*/) override { }
  virtual void exitUser(MySQLParser::UserContext * /*ctx*/) override { }

  virtual void enterLikeClause(MySQLParser::LikeClauseContext * /*ctx*/) override { }
  virtual void exitLikeClause(MySQLParser::LikeClauseContext * /*ctx*/) override { }

  virtual void enterLikeOrWhere(MySQLParser::LikeOrWhereContext * /*ctx*/) override { }
  virtual void exitLikeOrWhere(MySQLParser::LikeOrWhereContext * /*ctx*/) override { }

  virtual void enterOnlineOption(MySQLParser::OnlineOptionContext * /*ctx*/) override { }
  virtual void exitOnlineOption(MySQLParser::OnlineOptionContext * /*ctx*/) override { }

  virtual void enterNoWriteToBinLog(MySQLParser::NoWriteToBinLogContext * /*ctx*/) override { }
  virtual void exitNoWriteToBinLog(MySQLParser::NoWriteToBinLogContext * /*ctx*/) override { }

  virtual void enterUsePartition(MySQLParser::UsePartitionContext * /*ctx*/) override { }
  virtual void exitUsePartition(MySQLParser::UsePartitionContext * /*ctx*/) override { }

  virtual void enterFieldIdentifier(MySQLParser::FieldIdentifierContext * /*ctx*/) override { }
  virtual void exitFieldIdentifier(MySQLParser::FieldIdentifierContext * /*ctx*/) override { }

  virtual void enterColumnName(MySQLParser::ColumnNameContext * /*ctx*/) override { }
  virtual void exitColumnName(MySQLParser::ColumnNameContext * /*ctx*/) override { }

  virtual void enterColumnInternalRef(MySQLParser::ColumnInternalRefContext * /*ctx*/) override { }
  virtual void exitColumnInternalRef(MySQLParser::ColumnInternalRefContext * /*ctx*/) override { }

  virtual void enterColumnInternalRefList(MySQLParser::ColumnInternalRefListContext * /*ctx*/) override { }
  virtual void exitColumnInternalRefList(MySQLParser::ColumnInternalRefListContext * /*ctx*/) override { }

  virtual void enterColumnRef(MySQLParser::ColumnRefContext * /*ctx*/) override { }
  virtual void exitColumnRef(MySQLParser::ColumnRefContext * /*ctx*/) override { }

  virtual void enterInsertIdentifier(MySQLParser::InsertIdentifierContext * /*ctx*/) override { }
  virtual void exitInsertIdentifier(MySQLParser::InsertIdentifierContext * /*ctx*/) override { }

  virtual void enterIndexName(MySQLParser::IndexNameContext * /*ctx*/) override { }
  virtual void exitIndexName(MySQLParser::IndexNameContext * /*ctx*/) override { }

  virtual void enterIndexRef(MySQLParser::IndexRefContext * /*ctx*/) override { }
  virtual void exitIndexRef(MySQLParser::IndexRefContext * /*ctx*/) override { }

  virtual void enterTableWild(MySQLParser::TableWildContext * /*ctx*/) override { }
  virtual void exitTableWild(MySQLParser::TableWildContext * /*ctx*/) override { }

  virtual void enterSchemaName(MySQLParser::SchemaNameContext * /*ctx*/) override { }
  virtual void exitSchemaName(MySQLParser::SchemaNameContext * /*ctx*/) override { }

  virtual void enterSchemaRef(MySQLParser::SchemaRefContext * /*ctx*/) override { }
  virtual void exitSchemaRef(MySQLParser::SchemaRefContext * /*ctx*/) override { }

  virtual void enterProcedureName(MySQLParser::ProcedureNameContext * /*ctx*/) override { }
  virtual void exitProcedureName(MySQLParser::ProcedureNameContext * /*ctx*/) override { }

  virtual void enterProcedureRef(MySQLParser::ProcedureRefContext * /*ctx*/) override { }
  virtual void exitProcedureRef(MySQLParser::ProcedureRefContext * /*ctx*/) override { }

  virtual void enterFunctionName(MySQLParser::FunctionNameContext * /*ctx*/) override { }
  virtual void exitFunctionName(MySQLParser::FunctionNameContext * /*ctx*/) override { }

  virtual void enterFunctionRef(MySQLParser::FunctionRefContext * /*ctx*/) override { }
  virtual void exitFunctionRef(MySQLParser::FunctionRefContext * /*ctx*/) override { }

  virtual void enterTriggerName(MySQLParser::TriggerNameContext * /*ctx*/) override { }
  virtual void exitTriggerName(MySQLParser::TriggerNameContext * /*ctx*/) override { }

  virtual void enterTriggerRef(MySQLParser::TriggerRefContext * /*ctx*/) override { }
  virtual void exitTriggerRef(MySQLParser::TriggerRefContext * /*ctx*/) override { }

  virtual void enterViewName(MySQLParser::ViewNameContext * /*ctx*/) override { }
  virtual void exitViewName(MySQLParser::ViewNameContext * /*ctx*/) override { }

  virtual void enterViewRef(MySQLParser::ViewRefContext * /*ctx*/) override { }
  virtual void exitViewRef(MySQLParser::ViewRefContext * /*ctx*/) override { }

  virtual void enterTablespaceName(MySQLParser::TablespaceNameContext * /*ctx*/) override { }
  virtual void exitTablespaceName(MySQLParser::TablespaceNameContext * /*ctx*/) override { }

  virtual void enterTablespaceRef(MySQLParser::TablespaceRefContext * /*ctx*/) override { }
  virtual void exitTablespaceRef(MySQLParser::TablespaceRefContext * /*ctx*/) override { }

  virtual void enterLogfileGroupName(MySQLParser::LogfileGroupNameContext * /*ctx*/) override { }
  virtual void exitLogfileGroupName(MySQLParser::LogfileGroupNameContext * /*ctx*/) override { }

  virtual void enterLogfileGroupRef(MySQLParser::LogfileGroupRefContext * /*ctx*/) override { }
  virtual void exitLogfileGroupRef(MySQLParser::LogfileGroupRefContext * /*ctx*/) override { }

  virtual void enterEventName(MySQLParser::EventNameContext * /*ctx*/) override { }
  virtual void exitEventName(MySQLParser::EventNameContext * /*ctx*/) override { }

  virtual void enterEventRef(MySQLParser::EventRefContext * /*ctx*/) override { }
  virtual void exitEventRef(MySQLParser::EventRefContext * /*ctx*/) override { }

  virtual void enterUdfName(MySQLParser::UdfNameContext * /*ctx*/) override { }
  virtual void exitUdfName(MySQLParser::UdfNameContext * /*ctx*/) override { }

  virtual void enterServerName(MySQLParser::ServerNameContext * /*ctx*/) override { }
  virtual void exitServerName(MySQLParser::ServerNameContext * /*ctx*/) override { }

  virtual void enterServerRef(MySQLParser::ServerRefContext * /*ctx*/) override { }
  virtual void exitServerRef(MySQLParser::ServerRefContext * /*ctx*/) override { }

  virtual void enterEngineRef(MySQLParser::EngineRefContext * /*ctx*/) override { }
  virtual void exitEngineRef(MySQLParser::EngineRefContext * /*ctx*/) override { }

  virtual void enterTableName(MySQLParser::TableNameContext * /*ctx*/) override { }
  virtual void exitTableName(MySQLParser::TableNameContext * /*ctx*/) override { }

  virtual void enterFilterTableRef(MySQLParser::FilterTableRefContext * /*ctx*/) override { }
  virtual void exitFilterTableRef(MySQLParser::FilterTableRefContext * /*ctx*/) override { }

  virtual void enterTableRefWithWildcard(MySQLParser::TableRefWithWildcardContext * /*ctx*/) override { }
  virtual void exitTableRefWithWildcard(MySQLParser::TableRefWithWildcardContext * /*ctx*/) override { }

  virtual void enterTableRef(MySQLParser::TableRefContext * /*ctx*/) override { }
  virtual void exitTableRef(MySQLParser::TableRefContext * /*ctx*/) override { }

  virtual void enterTableRefList(MySQLParser::TableRefListContext * /*ctx*/) override { }
  virtual void exitTableRefList(MySQLParser::TableRefListContext * /*ctx*/) override { }

  virtual void enterTableAliasRefList(MySQLParser::TableAliasRefListContext * /*ctx*/) override { }
  virtual void exitTableAliasRefList(MySQLParser::TableAliasRefListContext * /*ctx*/) override { }

  virtual void enterParameterName(MySQLParser::ParameterNameContext * /*ctx*/) override { }
  virtual void exitParameterName(MySQLParser::ParameterNameContext * /*ctx*/) override { }

  virtual void enterLabelIdentifier(MySQLParser::LabelIdentifierContext * /*ctx*/) override { }
  virtual void exitLabelIdentifier(MySQLParser::LabelIdentifierContext * /*ctx*/) override { }

  virtual void enterLabelRef(MySQLParser::LabelRefContext * /*ctx*/) override { }
  virtual void exitLabelRef(MySQLParser::LabelRefContext * /*ctx*/) override { }

  virtual void enterRoleIdentifier(MySQLParser::RoleIdentifierContext * /*ctx*/) override { }
  virtual void exitRoleIdentifier(MySQLParser::RoleIdentifierContext * /*ctx*/) override { }

  virtual void enterPluginRef(MySQLParser::PluginRefContext * /*ctx*/) override { }
  virtual void exitPluginRef(MySQLParser::PluginRefContext * /*ctx*/) override { }

  virtual void enterComponentRef(MySQLParser::ComponentRefContext * /*ctx*/) override { }
  virtual void exitComponentRef(MySQLParser::ComponentRefContext * /*ctx*/) override { }

  virtual void enterResourceGroupRef(MySQLParser::ResourceGroupRefContext * /*ctx*/) override { }
  virtual void exitResourceGroupRef(MySQLParser::ResourceGroupRefContext * /*ctx*/) override { }

  virtual void enterWindowName(MySQLParser::WindowNameContext * /*ctx*/) override { }
  virtual void exitWindowName(MySQLParser::WindowNameContext * /*ctx*/) override { }

  virtual void enterPureIdentifier(MySQLParser::PureIdentifierContext * /*ctx*/) override { }
  virtual void exitPureIdentifier(MySQLParser::PureIdentifierContext * /*ctx*/) override { }

  virtual void enterIdentifier(MySQLParser::IdentifierContext * /*ctx*/) override { }
  virtual void exitIdentifier(MySQLParser::IdentifierContext * /*ctx*/) override { }

  virtual void enterIdentifierList(MySQLParser::IdentifierListContext * /*ctx*/) override { }
  virtual void exitIdentifierList(MySQLParser::IdentifierListContext * /*ctx*/) override { }

  virtual void enterIdentifierListWithParentheses(MySQLParser::IdentifierListWithParenthesesContext * /*ctx*/) override { }
  virtual void exitIdentifierListWithParentheses(MySQLParser::IdentifierListWithParenthesesContext * /*ctx*/) override { }

  virtual void enterQualifiedIdentifier(MySQLParser::QualifiedIdentifierContext * /*ctx*/) override { }
  virtual void exitQualifiedIdentifier(MySQLParser::QualifiedIdentifierContext * /*ctx*/) override { }

  virtual void enterSimpleIdentifier(MySQLParser::SimpleIdentifierContext * /*ctx*/) override { }
  virtual void exitSimpleIdentifier(MySQLParser::SimpleIdentifierContext * /*ctx*/) override { }

  virtual void enterDotIdentifier(MySQLParser::DotIdentifierContext * /*ctx*/) override { }
  virtual void exitDotIdentifier(MySQLParser::DotIdentifierContext * /*ctx*/) override { }

  virtual void enterUlong_number(MySQLParser::Ulong_numberContext * /*ctx*/) override { }
  virtual void exitUlong_number(MySQLParser::Ulong_numberContext * /*ctx*/) override { }

  virtual void enterReal_ulong_number(MySQLParser::Real_ulong_numberContext * /*ctx*/) override { }
  virtual void exitReal_ulong_number(MySQLParser::Real_ulong_numberContext * /*ctx*/) override { }

  virtual void enterUlonglong_number(MySQLParser::Ulonglong_numberContext * /*ctx*/) override { }
  virtual void exitUlonglong_number(MySQLParser::Ulonglong_numberContext * /*ctx*/) override { }

  virtual void enterReal_ulonglong_number(MySQLParser::Real_ulonglong_numberContext * /*ctx*/) override { }
  virtual void exitReal_ulonglong_number(MySQLParser::Real_ulonglong_numberContext * /*ctx*/) override { }

  virtual void enterSignedLiteral(MySQLParser::SignedLiteralContext * /*ctx*/) override { }
  virtual void exitSignedLiteral(MySQLParser::SignedLiteralContext * /*ctx*/) override { }

  virtual void enterSignedLiteralOrNull(MySQLParser::SignedLiteralOrNullContext * /*ctx*/) override { }
  virtual void exitSignedLiteralOrNull(MySQLParser::SignedLiteralOrNullContext * /*ctx*/) override { }

  virtual void enterLiteral(MySQLParser::LiteralContext * /*ctx*/) override { }
  virtual void exitLiteral(MySQLParser::LiteralContext * /*ctx*/) override { }

  virtual void enterLiteralOrNull(MySQLParser::LiteralOrNullContext * /*ctx*/) override { }
  virtual void exitLiteralOrNull(MySQLParser::LiteralOrNullContext * /*ctx*/) override { }

  virtual void enterNullAsLiteral(MySQLParser::NullAsLiteralContext * /*ctx*/) override { }
  virtual void exitNullAsLiteral(MySQLParser::NullAsLiteralContext * /*ctx*/) override { }

  virtual void enterStringList(MySQLParser::StringListContext * /*ctx*/) override { }
  virtual void exitStringList(MySQLParser::StringListContext * /*ctx*/) override { }

  virtual void enterTextStringLiteral(MySQLParser::TextStringLiteralContext * /*ctx*/) override { }
  virtual void exitTextStringLiteral(MySQLParser::TextStringLiteralContext * /*ctx*/) override { }

  virtual void enterTextString(MySQLParser::TextStringContext * /*ctx*/) override { }
  virtual void exitTextString(MySQLParser::TextStringContext * /*ctx*/) override { }

  virtual void enterTextStringHash(MySQLParser::TextStringHashContext * /*ctx*/) override { }
  virtual void exitTextStringHash(MySQLParser::TextStringHashContext * /*ctx*/) override { }

  virtual void enterTextLiteral(MySQLParser::TextLiteralContext * /*ctx*/) override { }
  virtual void exitTextLiteral(MySQLParser::TextLiteralContext * /*ctx*/) override { }

  virtual void enterTextStringNoLinebreak(MySQLParser::TextStringNoLinebreakContext * /*ctx*/) override { }
  virtual void exitTextStringNoLinebreak(MySQLParser::TextStringNoLinebreakContext * /*ctx*/) override { }

  virtual void enterTextStringLiteralList(MySQLParser::TextStringLiteralListContext * /*ctx*/) override { }
  virtual void exitTextStringLiteralList(MySQLParser::TextStringLiteralListContext * /*ctx*/) override { }

  virtual void enterNumLiteral(MySQLParser::NumLiteralContext * /*ctx*/) override { }
  virtual void exitNumLiteral(MySQLParser::NumLiteralContext * /*ctx*/) override { }

  virtual void enterBoolLiteral(MySQLParser::BoolLiteralContext * /*ctx*/) override { }
  virtual void exitBoolLiteral(MySQLParser::BoolLiteralContext * /*ctx*/) override { }

  virtual void enterNullLiteral(MySQLParser::NullLiteralContext * /*ctx*/) override { }
  virtual void exitNullLiteral(MySQLParser::NullLiteralContext * /*ctx*/) override { }

  virtual void enterInt64Literal(MySQLParser::Int64LiteralContext * /*ctx*/) override { }
  virtual void exitInt64Literal(MySQLParser::Int64LiteralContext * /*ctx*/) override { }

  virtual void enterTemporalLiteral(MySQLParser::TemporalLiteralContext * /*ctx*/) override { }
  virtual void exitTemporalLiteral(MySQLParser::TemporalLiteralContext * /*ctx*/) override { }

  virtual void enterFloatOptions(MySQLParser::FloatOptionsContext * /*ctx*/) override { }
  virtual void exitFloatOptions(MySQLParser::FloatOptionsContext * /*ctx*/) override { }

  virtual void enterStandardFloatOptions(MySQLParser::StandardFloatOptionsContext * /*ctx*/) override { }
  virtual void exitStandardFloatOptions(MySQLParser::StandardFloatOptionsContext * /*ctx*/) override { }

  virtual void enterPrecision(MySQLParser::PrecisionContext * /*ctx*/) override { }
  virtual void exitPrecision(MySQLParser::PrecisionContext * /*ctx*/) override { }

  virtual void enterTextOrIdentifier(MySQLParser::TextOrIdentifierContext * /*ctx*/) override { }
  virtual void exitTextOrIdentifier(MySQLParser::TextOrIdentifierContext * /*ctx*/) override { }

  virtual void enterLValueIdentifier(MySQLParser::LValueIdentifierContext * /*ctx*/) override { }
  virtual void exitLValueIdentifier(MySQLParser::LValueIdentifierContext * /*ctx*/) override { }

  virtual void enterRoleIdentifierOrText(MySQLParser::RoleIdentifierOrTextContext * /*ctx*/) override { }
  virtual void exitRoleIdentifierOrText(MySQLParser::RoleIdentifierOrTextContext * /*ctx*/) override { }

  virtual void enterSizeNumber(MySQLParser::SizeNumberContext * /*ctx*/) override { }
  virtual void exitSizeNumber(MySQLParser::SizeNumberContext * /*ctx*/) override { }

  virtual void enterParentheses(MySQLParser::ParenthesesContext * /*ctx*/) override { }
  virtual void exitParentheses(MySQLParser::ParenthesesContext * /*ctx*/) override { }

  virtual void enterEqual(MySQLParser::EqualContext * /*ctx*/) override { }
  virtual void exitEqual(MySQLParser::EqualContext * /*ctx*/) override { }

  virtual void enterOptionType(MySQLParser::OptionTypeContext * /*ctx*/) override { }
  virtual void exitOptionType(MySQLParser::OptionTypeContext * /*ctx*/) override { }

  virtual void enterRvalueSystemVariableType(MySQLParser::RvalueSystemVariableTypeContext * /*ctx*/) override { }
  virtual void exitRvalueSystemVariableType(MySQLParser::RvalueSystemVariableTypeContext * /*ctx*/) override { }

  virtual void enterSetVarIdentType(MySQLParser::SetVarIdentTypeContext * /*ctx*/) override { }
  virtual void exitSetVarIdentType(MySQLParser::SetVarIdentTypeContext * /*ctx*/) override { }

  virtual void enterJsonAttribute(MySQLParser::JsonAttributeContext * /*ctx*/) override { }
  virtual void exitJsonAttribute(MySQLParser::JsonAttributeContext * /*ctx*/) override { }

  virtual void enterIdentifierKeyword(MySQLParser::IdentifierKeywordContext * /*ctx*/) override { }
  virtual void exitIdentifierKeyword(MySQLParser::IdentifierKeywordContext * /*ctx*/) override { }

  virtual void enterIdentifierKeywordsAmbiguous1RolesAndLabels(MySQLParser::IdentifierKeywordsAmbiguous1RolesAndLabelsContext * /*ctx*/) override { }
  virtual void exitIdentifierKeywordsAmbiguous1RolesAndLabels(MySQLParser::IdentifierKeywordsAmbiguous1RolesAndLabelsContext * /*ctx*/) override { }

  virtual void enterIdentifierKeywordsAmbiguous2Labels(MySQLParser::IdentifierKeywordsAmbiguous2LabelsContext * /*ctx*/) override { }
  virtual void exitIdentifierKeywordsAmbiguous2Labels(MySQLParser::IdentifierKeywordsAmbiguous2LabelsContext * /*ctx*/) override { }

  virtual void enterLabelKeyword(MySQLParser::LabelKeywordContext * /*ctx*/) override { }
  virtual void exitLabelKeyword(MySQLParser::LabelKeywordContext * /*ctx*/) override { }

  virtual void enterIdentifierKeywordsAmbiguous3Roles(MySQLParser::IdentifierKeywordsAmbiguous3RolesContext * /*ctx*/) override { }
  virtual void exitIdentifierKeywordsAmbiguous3Roles(MySQLParser::IdentifierKeywordsAmbiguous3RolesContext * /*ctx*/) override { }

  virtual void enterIdentifierKeywordsUnambiguous(MySQLParser::IdentifierKeywordsUnambiguousContext * /*ctx*/) override { }
  virtual void exitIdentifierKeywordsUnambiguous(MySQLParser::IdentifierKeywordsUnambiguousContext * /*ctx*/) override { }

  virtual void enterRoleKeyword(MySQLParser::RoleKeywordContext * /*ctx*/) override { }
  virtual void exitRoleKeyword(MySQLParser::RoleKeywordContext * /*ctx*/) override { }

  virtual void enterLValueKeyword(MySQLParser::LValueKeywordContext * /*ctx*/) override { }
  virtual void exitLValueKeyword(MySQLParser::LValueKeywordContext * /*ctx*/) override { }

  virtual void enterIdentifierKeywordsAmbiguous4SystemVariables(MySQLParser::IdentifierKeywordsAmbiguous4SystemVariablesContext * /*ctx*/) override { }
  virtual void exitIdentifierKeywordsAmbiguous4SystemVariables(MySQLParser::IdentifierKeywordsAmbiguous4SystemVariablesContext * /*ctx*/) override { }

  virtual void enterRoleOrIdentifierKeyword(MySQLParser::RoleOrIdentifierKeywordContext * /*ctx*/) override { }
  virtual void exitRoleOrIdentifierKeyword(MySQLParser::RoleOrIdentifierKeywordContext * /*ctx*/) override { }

  virtual void enterRoleOrLabelKeyword(MySQLParser::RoleOrLabelKeywordContext * /*ctx*/) override { }
  virtual void exitRoleOrLabelKeyword(MySQLParser::RoleOrLabelKeywordContext * /*ctx*/) override { }


  virtual void enterEveryRule(antlr4::ParserRuleContext * /*ctx*/) override { }
  virtual void exitEveryRule(antlr4::ParserRuleContext * /*ctx*/) override { }
  virtual void visitTerminal(antlr4::tree::TerminalNode * /*node*/) override { }
  virtual void visitErrorNode(antlr4::tree::ErrorNode * /*node*/) override { }

};

}  // namespace parsers
