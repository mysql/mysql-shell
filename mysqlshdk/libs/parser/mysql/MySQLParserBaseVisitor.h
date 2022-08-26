// clang-format off
/*
 * Copyright (c) 2020, 2022, Oracle and/or its affiliates.
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




// Generated from /Users/kojima/dev/ngshell/mysqlshdk/libs/parser/grammars/MySQLParser.g4 by ANTLR 4.10.1

#pragma once


#include "antlr4-runtime.h"
#include "MySQLParserVisitor.h"


namespace parsers {

/**
 * This class provides an empty implementation of MySQLParserVisitor, which can be
 * extended to create a visitor which only needs to handle a subset of the available methods.
 */
class PARSERS_PUBLIC_TYPE MySQLParserBaseVisitor : public MySQLParserVisitor {
public:

  virtual std::any visitQuery(MySQLParser::QueryContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSimpleStatement(MySQLParser::SimpleStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAlterStatement(MySQLParser::AlterStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAlterDatabase(MySQLParser::AlterDatabaseContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAlterDatabaseOption(MySQLParser::AlterDatabaseOptionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAlterEvent(MySQLParser::AlterEventContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAlterLogfileGroup(MySQLParser::AlterLogfileGroupContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAlterLogfileGroupOptions(MySQLParser::AlterLogfileGroupOptionsContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAlterLogfileGroupOption(MySQLParser::AlterLogfileGroupOptionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAlterServer(MySQLParser::AlterServerContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAlterTable(MySQLParser::AlterTableContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAlterTableActions(MySQLParser::AlterTableActionsContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAlterCommandList(MySQLParser::AlterCommandListContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAlterCommandsModifierList(MySQLParser::AlterCommandsModifierListContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitStandaloneAlterCommands(MySQLParser::StandaloneAlterCommandsContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAlterPartition(MySQLParser::AlterPartitionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAlterList(MySQLParser::AlterListContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAlterCommandsModifier(MySQLParser::AlterCommandsModifierContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAlterListItem(MySQLParser::AlterListItemContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitPlace(MySQLParser::PlaceContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitRestrict(MySQLParser::RestrictContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAlterOrderList(MySQLParser::AlterOrderListContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAlterAlgorithmOption(MySQLParser::AlterAlgorithmOptionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAlterLockOption(MySQLParser::AlterLockOptionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitIndexLockAndAlgorithm(MySQLParser::IndexLockAndAlgorithmContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitWithValidation(MySQLParser::WithValidationContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitRemovePartitioning(MySQLParser::RemovePartitioningContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAllOrPartitionNameList(MySQLParser::AllOrPartitionNameListContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAlterTablespace(MySQLParser::AlterTablespaceContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAlterUndoTablespace(MySQLParser::AlterUndoTablespaceContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitUndoTableSpaceOptions(MySQLParser::UndoTableSpaceOptionsContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitUndoTableSpaceOption(MySQLParser::UndoTableSpaceOptionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAlterTablespaceOptions(MySQLParser::AlterTablespaceOptionsContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAlterTablespaceOption(MySQLParser::AlterTablespaceOptionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitChangeTablespaceOption(MySQLParser::ChangeTablespaceOptionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAlterView(MySQLParser::AlterViewContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitViewTail(MySQLParser::ViewTailContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitViewQueryBlock(MySQLParser::ViewQueryBlockContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitViewCheckOption(MySQLParser::ViewCheckOptionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAlterInstanceStatement(MySQLParser::AlterInstanceStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitCreateStatement(MySQLParser::CreateStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitCreateDatabase(MySQLParser::CreateDatabaseContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitCreateDatabaseOption(MySQLParser::CreateDatabaseOptionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitCreateTable(MySQLParser::CreateTableContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTableElementList(MySQLParser::TableElementListContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTableElement(MySQLParser::TableElementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitDuplicateAsQueryExpression(MySQLParser::DuplicateAsQueryExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitQueryExpressionOrParens(MySQLParser::QueryExpressionOrParensContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitCreateRoutine(MySQLParser::CreateRoutineContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitCreateProcedure(MySQLParser::CreateProcedureContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitCreateFunction(MySQLParser::CreateFunctionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitCreateUdf(MySQLParser::CreateUdfContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitRoutineCreateOption(MySQLParser::RoutineCreateOptionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitRoutineAlterOptions(MySQLParser::RoutineAlterOptionsContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitRoutineOption(MySQLParser::RoutineOptionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitCreateIndex(MySQLParser::CreateIndexContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitIndexNameAndType(MySQLParser::IndexNameAndTypeContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitCreateIndexTarget(MySQLParser::CreateIndexTargetContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitCreateLogfileGroup(MySQLParser::CreateLogfileGroupContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLogfileGroupOptions(MySQLParser::LogfileGroupOptionsContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLogfileGroupOption(MySQLParser::LogfileGroupOptionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitCreateServer(MySQLParser::CreateServerContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitServerOptions(MySQLParser::ServerOptionsContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitServerOption(MySQLParser::ServerOptionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitCreateTablespace(MySQLParser::CreateTablespaceContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitCreateUndoTablespace(MySQLParser::CreateUndoTablespaceContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTsDataFileName(MySQLParser::TsDataFileNameContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTsDataFile(MySQLParser::TsDataFileContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTablespaceOptions(MySQLParser::TablespaceOptionsContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTablespaceOption(MySQLParser::TablespaceOptionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTsOptionInitialSize(MySQLParser::TsOptionInitialSizeContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTsOptionUndoRedoBufferSize(MySQLParser::TsOptionUndoRedoBufferSizeContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTsOptionAutoextendSize(MySQLParser::TsOptionAutoextendSizeContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTsOptionMaxSize(MySQLParser::TsOptionMaxSizeContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTsOptionExtentSize(MySQLParser::TsOptionExtentSizeContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTsOptionNodegroup(MySQLParser::TsOptionNodegroupContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTsOptionEngine(MySQLParser::TsOptionEngineContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTsOptionWait(MySQLParser::TsOptionWaitContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTsOptionComment(MySQLParser::TsOptionCommentContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTsOptionFileblockSize(MySQLParser::TsOptionFileblockSizeContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTsOptionEncryption(MySQLParser::TsOptionEncryptionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTsOptionEngineAttribute(MySQLParser::TsOptionEngineAttributeContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitCreateView(MySQLParser::CreateViewContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitViewReplaceOrAlgorithm(MySQLParser::ViewReplaceOrAlgorithmContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitViewAlgorithm(MySQLParser::ViewAlgorithmContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitViewSuid(MySQLParser::ViewSuidContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitCreateTrigger(MySQLParser::CreateTriggerContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTriggerFollowsPrecedesClause(MySQLParser::TriggerFollowsPrecedesClauseContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitCreateEvent(MySQLParser::CreateEventContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitCreateRole(MySQLParser::CreateRoleContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitCreateSpatialReference(MySQLParser::CreateSpatialReferenceContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSrsAttribute(MySQLParser::SrsAttributeContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitDropStatement(MySQLParser::DropStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitDropDatabase(MySQLParser::DropDatabaseContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitDropEvent(MySQLParser::DropEventContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitDropFunction(MySQLParser::DropFunctionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitDropProcedure(MySQLParser::DropProcedureContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitDropIndex(MySQLParser::DropIndexContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitDropLogfileGroup(MySQLParser::DropLogfileGroupContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitDropLogfileGroupOption(MySQLParser::DropLogfileGroupOptionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitDropServer(MySQLParser::DropServerContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitDropTable(MySQLParser::DropTableContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitDropTableSpace(MySQLParser::DropTableSpaceContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitDropTrigger(MySQLParser::DropTriggerContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitDropView(MySQLParser::DropViewContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitDropRole(MySQLParser::DropRoleContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitDropSpatialReference(MySQLParser::DropSpatialReferenceContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitDropUndoTablespace(MySQLParser::DropUndoTablespaceContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitRenameTableStatement(MySQLParser::RenameTableStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitRenamePair(MySQLParser::RenamePairContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTruncateTableStatement(MySQLParser::TruncateTableStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitImportStatement(MySQLParser::ImportStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitCallStatement(MySQLParser::CallStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitDeleteStatement(MySQLParser::DeleteStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitPartitionDelete(MySQLParser::PartitionDeleteContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitDeleteStatementOption(MySQLParser::DeleteStatementOptionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitDoStatement(MySQLParser::DoStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitHandlerStatement(MySQLParser::HandlerStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitHandlerReadOrScan(MySQLParser::HandlerReadOrScanContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitInsertStatement(MySQLParser::InsertStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitInsertLockOption(MySQLParser::InsertLockOptionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitInsertFromConstructor(MySQLParser::InsertFromConstructorContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFields(MySQLParser::FieldsContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitInsertValues(MySQLParser::InsertValuesContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitInsertQueryExpression(MySQLParser::InsertQueryExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitValueList(MySQLParser::ValueListContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitValues(MySQLParser::ValuesContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitValuesReference(MySQLParser::ValuesReferenceContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitInsertUpdateList(MySQLParser::InsertUpdateListContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLoadStatement(MySQLParser::LoadStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitDataOrXml(MySQLParser::DataOrXmlContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitXmlRowsIdentifiedBy(MySQLParser::XmlRowsIdentifiedByContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLoadDataFileTail(MySQLParser::LoadDataFileTailContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLoadDataFileTargetList(MySQLParser::LoadDataFileTargetListContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFieldOrVariableList(MySQLParser::FieldOrVariableListContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitReplaceStatement(MySQLParser::ReplaceStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSelectStatement(MySQLParser::SelectStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSelectStatementWithInto(MySQLParser::SelectStatementWithIntoContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitQueryExpression(MySQLParser::QueryExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitQueryExpressionBody(MySQLParser::QueryExpressionBodyContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitQueryExpressionParens(MySQLParser::QueryExpressionParensContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitQueryPrimary(MySQLParser::QueryPrimaryContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitQuerySpecification(MySQLParser::QuerySpecificationContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSubquery(MySQLParser::SubqueryContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitQuerySpecOption(MySQLParser::QuerySpecOptionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLimitClause(MySQLParser::LimitClauseContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSimpleLimitClause(MySQLParser::SimpleLimitClauseContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLimitOptions(MySQLParser::LimitOptionsContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLimitOption(MySQLParser::LimitOptionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitIntoClause(MySQLParser::IntoClauseContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitProcedureAnalyseClause(MySQLParser::ProcedureAnalyseClauseContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitHavingClause(MySQLParser::HavingClauseContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitWindowClause(MySQLParser::WindowClauseContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitWindowDefinition(MySQLParser::WindowDefinitionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitWindowSpec(MySQLParser::WindowSpecContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitWindowSpecDetails(MySQLParser::WindowSpecDetailsContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitWindowFrameClause(MySQLParser::WindowFrameClauseContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitWindowFrameUnits(MySQLParser::WindowFrameUnitsContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitWindowFrameExtent(MySQLParser::WindowFrameExtentContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitWindowFrameStart(MySQLParser::WindowFrameStartContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitWindowFrameBetween(MySQLParser::WindowFrameBetweenContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitWindowFrameBound(MySQLParser::WindowFrameBoundContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitWindowFrameExclusion(MySQLParser::WindowFrameExclusionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitWithClause(MySQLParser::WithClauseContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitCommonTableExpression(MySQLParser::CommonTableExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitGroupByClause(MySQLParser::GroupByClauseContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitOlapOption(MySQLParser::OlapOptionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitOrderClause(MySQLParser::OrderClauseContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitDirection(MySQLParser::DirectionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFromClause(MySQLParser::FromClauseContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTableReferenceList(MySQLParser::TableReferenceListContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTableValueConstructor(MySQLParser::TableValueConstructorContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitExplicitTable(MySQLParser::ExplicitTableContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitRowValueExplicit(MySQLParser::RowValueExplicitContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSelectOption(MySQLParser::SelectOptionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLockingClauseList(MySQLParser::LockingClauseListContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLockingClause(MySQLParser::LockingClauseContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLockStrengh(MySQLParser::LockStrenghContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLockedRowAction(MySQLParser::LockedRowActionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSelectItemList(MySQLParser::SelectItemListContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSelectItem(MySQLParser::SelectItemContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSelectAlias(MySQLParser::SelectAliasContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitWhereClause(MySQLParser::WhereClauseContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTableReference(MySQLParser::TableReferenceContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitEscapedTableReference(MySQLParser::EscapedTableReferenceContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitJoinedTable(MySQLParser::JoinedTableContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitNaturalJoinType(MySQLParser::NaturalJoinTypeContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitInnerJoinType(MySQLParser::InnerJoinTypeContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitOuterJoinType(MySQLParser::OuterJoinTypeContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTableFactor(MySQLParser::TableFactorContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSingleTable(MySQLParser::SingleTableContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSingleTableParens(MySQLParser::SingleTableParensContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitDerivedTable(MySQLParser::DerivedTableContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTableReferenceListParens(MySQLParser::TableReferenceListParensContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTableFunction(MySQLParser::TableFunctionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitColumnsClause(MySQLParser::ColumnsClauseContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitJtColumn(MySQLParser::JtColumnContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitOnEmptyOrError(MySQLParser::OnEmptyOrErrorContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitOnEmptyOrErrorJsonTable(MySQLParser::OnEmptyOrErrorJsonTableContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitOnEmpty(MySQLParser::OnEmptyContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitOnError(MySQLParser::OnErrorContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitJsonOnResponse(MySQLParser::JsonOnResponseContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitUnionOption(MySQLParser::UnionOptionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTableAlias(MySQLParser::TableAliasContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitIndexHintList(MySQLParser::IndexHintListContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitIndexHint(MySQLParser::IndexHintContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitIndexHintType(MySQLParser::IndexHintTypeContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitKeyOrIndex(MySQLParser::KeyOrIndexContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitConstraintKeyType(MySQLParser::ConstraintKeyTypeContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitIndexHintClause(MySQLParser::IndexHintClauseContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitIndexList(MySQLParser::IndexListContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitIndexListElement(MySQLParser::IndexListElementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitUpdateStatement(MySQLParser::UpdateStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTransactionOrLockingStatement(MySQLParser::TransactionOrLockingStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTransactionStatement(MySQLParser::TransactionStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitBeginWork(MySQLParser::BeginWorkContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitStartTransactionOptionList(MySQLParser::StartTransactionOptionListContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSavepointStatement(MySQLParser::SavepointStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLockStatement(MySQLParser::LockStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLockItem(MySQLParser::LockItemContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLockOption(MySQLParser::LockOptionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitXaStatement(MySQLParser::XaStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitXaConvert(MySQLParser::XaConvertContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitXid(MySQLParser::XidContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitReplicationStatement(MySQLParser::ReplicationStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitResetOption(MySQLParser::ResetOptionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSourceResetOptions(MySQLParser::SourceResetOptionsContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitReplicationLoad(MySQLParser::ReplicationLoadContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitChangeReplicationSource(MySQLParser::ChangeReplicationSourceContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitChangeSource(MySQLParser::ChangeSourceContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSourceDefinitions(MySQLParser::SourceDefinitionsContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSourceDefinition(MySQLParser::SourceDefinitionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitChangeReplicationSourceAutoPosition(MySQLParser::ChangeReplicationSourceAutoPositionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitChangeReplicationSourceHost(MySQLParser::ChangeReplicationSourceHostContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitChangeReplicationSourceBind(MySQLParser::ChangeReplicationSourceBindContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitChangeReplicationSourceUser(MySQLParser::ChangeReplicationSourceUserContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitChangeReplicationSourcePassword(MySQLParser::ChangeReplicationSourcePasswordContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitChangeReplicationSourcePort(MySQLParser::ChangeReplicationSourcePortContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitChangeReplicationSourceConnectRetry(MySQLParser::ChangeReplicationSourceConnectRetryContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitChangeReplicationSourceRetryCount(MySQLParser::ChangeReplicationSourceRetryCountContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitChangeReplicationSourceDelay(MySQLParser::ChangeReplicationSourceDelayContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitChangeReplicationSourceSSL(MySQLParser::ChangeReplicationSourceSSLContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitChangeReplicationSourceSSLCA(MySQLParser::ChangeReplicationSourceSSLCAContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitChangeReplicationSourceSSLCApath(MySQLParser::ChangeReplicationSourceSSLCApathContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitChangeReplicationSourceSSLCipher(MySQLParser::ChangeReplicationSourceSSLCipherContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitChangeReplicationSourceSSLCLR(MySQLParser::ChangeReplicationSourceSSLCLRContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitChangeReplicationSourceSSLCLRpath(MySQLParser::ChangeReplicationSourceSSLCLRpathContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitChangeReplicationSourceSSLKey(MySQLParser::ChangeReplicationSourceSSLKeyContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitChangeReplicationSourceSSLVerifyServerCert(MySQLParser::ChangeReplicationSourceSSLVerifyServerCertContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitChangeReplicationSourceTLSVersion(MySQLParser::ChangeReplicationSourceTLSVersionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitChangeReplicationSourceTLSCiphersuites(MySQLParser::ChangeReplicationSourceTLSCiphersuitesContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitChangeReplicationSourceSSLCert(MySQLParser::ChangeReplicationSourceSSLCertContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitChangeReplicationSourcePublicKey(MySQLParser::ChangeReplicationSourcePublicKeyContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitChangeReplicationSourceGetSourcePublicKey(MySQLParser::ChangeReplicationSourceGetSourcePublicKeyContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitChangeReplicationSourceHeartbeatPeriod(MySQLParser::ChangeReplicationSourceHeartbeatPeriodContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitChangeReplicationSourceCompressionAlgorithm(MySQLParser::ChangeReplicationSourceCompressionAlgorithmContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitChangeReplicationSourceZstdCompressionLevel(MySQLParser::ChangeReplicationSourceZstdCompressionLevelContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitPrivilegeCheckDef(MySQLParser::PrivilegeCheckDefContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTablePrimaryKeyCheckDef(MySQLParser::TablePrimaryKeyCheckDefContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAssignGtidsToAnonymousTransactionsDefinition(MySQLParser::AssignGtidsToAnonymousTransactionsDefinitionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSourceTlsCiphersuitesDef(MySQLParser::SourceTlsCiphersuitesDefContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSourceFileDef(MySQLParser::SourceFileDefContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSourceLogFile(MySQLParser::SourceLogFileContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSourceLogPos(MySQLParser::SourceLogPosContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitServerIdList(MySQLParser::ServerIdListContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitChangeReplication(MySQLParser::ChangeReplicationContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFilterDefinition(MySQLParser::FilterDefinitionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFilterDbList(MySQLParser::FilterDbListContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFilterTableList(MySQLParser::FilterTableListContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFilterStringList(MySQLParser::FilterStringListContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFilterWildDbTableString(MySQLParser::FilterWildDbTableStringContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFilterDbPairList(MySQLParser::FilterDbPairListContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitStartReplicaStatement(MySQLParser::StartReplicaStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitStopReplicaStatement(MySQLParser::StopReplicaStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitReplicaUntil(MySQLParser::ReplicaUntilContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitUserOption(MySQLParser::UserOptionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitPasswordOption(MySQLParser::PasswordOptionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitDefaultAuthOption(MySQLParser::DefaultAuthOptionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitPluginDirOption(MySQLParser::PluginDirOptionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitReplicaThreadOptions(MySQLParser::ReplicaThreadOptionsContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitReplicaThreadOption(MySQLParser::ReplicaThreadOptionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitGroupReplication(MySQLParser::GroupReplicationContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitGroupReplicationStartOptions(MySQLParser::GroupReplicationStartOptionsContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitGroupReplicationStartOption(MySQLParser::GroupReplicationStartOptionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitGroupReplicationUser(MySQLParser::GroupReplicationUserContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitGroupReplicationPassword(MySQLParser::GroupReplicationPasswordContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitGroupReplicationPluginAuth(MySQLParser::GroupReplicationPluginAuthContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitReplica(MySQLParser::ReplicaContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitPreparedStatement(MySQLParser::PreparedStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitExecuteStatement(MySQLParser::ExecuteStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitExecuteVarList(MySQLParser::ExecuteVarListContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitCloneStatement(MySQLParser::CloneStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitDataDirSSL(MySQLParser::DataDirSSLContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSsl(MySQLParser::SslContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAccountManagementStatement(MySQLParser::AccountManagementStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAlterUserStatement(MySQLParser::AlterUserStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAlterUserList(MySQLParser::AlterUserListContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAlterUser(MySQLParser::AlterUserContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitOldAlterUser(MySQLParser::OldAlterUserContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitUserFunction(MySQLParser::UserFunctionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitCreateUserStatement(MySQLParser::CreateUserStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitCreateUserTail(MySQLParser::CreateUserTailContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitUserAttributes(MySQLParser::UserAttributesContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitDefaultRoleClause(MySQLParser::DefaultRoleClauseContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitRequireClause(MySQLParser::RequireClauseContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitConnectOptions(MySQLParser::ConnectOptionsContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAccountLockPasswordExpireOptions(MySQLParser::AccountLockPasswordExpireOptionsContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitDropUserStatement(MySQLParser::DropUserStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitGrantStatement(MySQLParser::GrantStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitGrantTargetList(MySQLParser::GrantTargetListContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitGrantOptions(MySQLParser::GrantOptionsContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitExceptRoleList(MySQLParser::ExceptRoleListContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitWithRoles(MySQLParser::WithRolesContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitGrantAs(MySQLParser::GrantAsContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitVersionedRequireClause(MySQLParser::VersionedRequireClauseContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitRenameUserStatement(MySQLParser::RenameUserStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitRevokeStatement(MySQLParser::RevokeStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitOnTypeTo(MySQLParser::OnTypeToContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAclType(MySQLParser::AclTypeContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitRoleOrPrivilegesList(MySQLParser::RoleOrPrivilegesListContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitRoleOrPrivilege(MySQLParser::RoleOrPrivilegeContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitGrantIdentifier(MySQLParser::GrantIdentifierContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitRequireList(MySQLParser::RequireListContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitRequireListElement(MySQLParser::RequireListElementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitGrantOption(MySQLParser::GrantOptionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSetRoleStatement(MySQLParser::SetRoleStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitRoleList(MySQLParser::RoleListContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitRole(MySQLParser::RoleContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTableAdministrationStatement(MySQLParser::TableAdministrationStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitHistogram(MySQLParser::HistogramContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitCheckOption(MySQLParser::CheckOptionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitRepairType(MySQLParser::RepairTypeContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitInstallUninstallStatement(MySQLParser::InstallUninstallStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSetStatement(MySQLParser::SetStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitStartOptionValueList(MySQLParser::StartOptionValueListContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTransactionCharacteristics(MySQLParser::TransactionCharacteristicsContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTransactionAccessMode(MySQLParser::TransactionAccessModeContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitIsolationLevel(MySQLParser::IsolationLevelContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitOptionValueListContinued(MySQLParser::OptionValueListContinuedContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitOptionValueNoOptionType(MySQLParser::OptionValueNoOptionTypeContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitOptionValue(MySQLParser::OptionValueContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitStartOptionValueListFollowingOptionType(MySQLParser::StartOptionValueListFollowingOptionTypeContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitOptionValueFollowingOptionType(MySQLParser::OptionValueFollowingOptionTypeContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSetExprOrDefault(MySQLParser::SetExprOrDefaultContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitShowDatabasesStatement(MySQLParser::ShowDatabasesStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitShowTablesStatement(MySQLParser::ShowTablesStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitShowTriggersStatement(MySQLParser::ShowTriggersStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitShowEventsStatement(MySQLParser::ShowEventsStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitShowTableStatusStatement(MySQLParser::ShowTableStatusStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitShowOpenTablesStatement(MySQLParser::ShowOpenTablesStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitShowPluginsStatement(MySQLParser::ShowPluginsStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitShowEngineLogsStatement(MySQLParser::ShowEngineLogsStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitShowEngineMutexStatement(MySQLParser::ShowEngineMutexStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitShowEngineStatusStatement(MySQLParser::ShowEngineStatusStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitShowColumnsStatement(MySQLParser::ShowColumnsStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitShowBinaryLogsStatement(MySQLParser::ShowBinaryLogsStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitShowReplicasStatement(MySQLParser::ShowReplicasStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitShowBinlogEventsStatement(MySQLParser::ShowBinlogEventsStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitShowRelaylogEventsStatement(MySQLParser::ShowRelaylogEventsStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitShowKeysStatement(MySQLParser::ShowKeysStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitShowEnginesStatement(MySQLParser::ShowEnginesStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitShowCountWarningsStatement(MySQLParser::ShowCountWarningsStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitShowCountErrorsStatement(MySQLParser::ShowCountErrorsStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitShowWarningsStatement(MySQLParser::ShowWarningsStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitShowErrorsStatement(MySQLParser::ShowErrorsStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitShowProfilesStatement(MySQLParser::ShowProfilesStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitShowProfileStatement(MySQLParser::ShowProfileStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitShowStatusStatement(MySQLParser::ShowStatusStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitShowProcessListStatement(MySQLParser::ShowProcessListStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitShowVariablesStatement(MySQLParser::ShowVariablesStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitShowCharacterSetStatement(MySQLParser::ShowCharacterSetStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitShowCollationStatement(MySQLParser::ShowCollationStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitShowPrivilegesStatement(MySQLParser::ShowPrivilegesStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitShowGrantsStatement(MySQLParser::ShowGrantsStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitShowCreateDatabaseStatement(MySQLParser::ShowCreateDatabaseStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitShowCreateTableStatement(MySQLParser::ShowCreateTableStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitShowCreateViewStatement(MySQLParser::ShowCreateViewStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitShowMasterStatusStatement(MySQLParser::ShowMasterStatusStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitShowReplicaStatusStatement(MySQLParser::ShowReplicaStatusStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitShowCreateProcedureStatement(MySQLParser::ShowCreateProcedureStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitShowCreateFunctionStatement(MySQLParser::ShowCreateFunctionStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitShowCreateTriggerStatement(MySQLParser::ShowCreateTriggerStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitShowCreateProcedureStatusStatement(MySQLParser::ShowCreateProcedureStatusStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitShowCreateFunctionStatusStatement(MySQLParser::ShowCreateFunctionStatusStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitShowCreateProcedureCodeStatement(MySQLParser::ShowCreateProcedureCodeStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitShowCreateFunctionCodeStatement(MySQLParser::ShowCreateFunctionCodeStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitShowCreateEventStatement(MySQLParser::ShowCreateEventStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitShowCreateUserStatement(MySQLParser::ShowCreateUserStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitShowCommandType(MySQLParser::ShowCommandTypeContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitEngineOrAll(MySQLParser::EngineOrAllContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFromOrIn(MySQLParser::FromOrInContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitInDb(MySQLParser::InDbContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitProfileDefinitions(MySQLParser::ProfileDefinitionsContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitProfileDefinition(MySQLParser::ProfileDefinitionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitOtherAdministrativeStatement(MySQLParser::OtherAdministrativeStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitKeyCacheListOrParts(MySQLParser::KeyCacheListOrPartsContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitKeyCacheList(MySQLParser::KeyCacheListContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAssignToKeycache(MySQLParser::AssignToKeycacheContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAssignToKeycachePartition(MySQLParser::AssignToKeycachePartitionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitCacheKeyList(MySQLParser::CacheKeyListContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitKeyUsageElement(MySQLParser::KeyUsageElementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitKeyUsageList(MySQLParser::KeyUsageListContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFlushOption(MySQLParser::FlushOptionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLogType(MySQLParser::LogTypeContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFlushTables(MySQLParser::FlushTablesContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFlushTablesOptions(MySQLParser::FlushTablesOptionsContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitPreloadTail(MySQLParser::PreloadTailContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitPreloadList(MySQLParser::PreloadListContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitPreloadKeys(MySQLParser::PreloadKeysContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAdminPartition(MySQLParser::AdminPartitionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitResourceGroupManagement(MySQLParser::ResourceGroupManagementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitCreateResourceGroup(MySQLParser::CreateResourceGroupContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitResourceGroupVcpuList(MySQLParser::ResourceGroupVcpuListContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitVcpuNumOrRange(MySQLParser::VcpuNumOrRangeContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitResourceGroupPriority(MySQLParser::ResourceGroupPriorityContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitResourceGroupEnableDisable(MySQLParser::ResourceGroupEnableDisableContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAlterResourceGroup(MySQLParser::AlterResourceGroupContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSetResourceGroup(MySQLParser::SetResourceGroupContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitThreadIdList(MySQLParser::ThreadIdListContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitDropResourceGroup(MySQLParser::DropResourceGroupContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitUtilityStatement(MySQLParser::UtilityStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitDescribeStatement(MySQLParser::DescribeStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitExplainStatement(MySQLParser::ExplainStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitExplainableStatement(MySQLParser::ExplainableStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitHelpCommand(MySQLParser::HelpCommandContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitUseCommand(MySQLParser::UseCommandContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitRestartServer(MySQLParser::RestartServerContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitExprOr(MySQLParser::ExprOrContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitExprNot(MySQLParser::ExprNotContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitExprIs(MySQLParser::ExprIsContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitExprAnd(MySQLParser::ExprAndContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitExprXor(MySQLParser::ExprXorContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitPrimaryExprPredicate(MySQLParser::PrimaryExprPredicateContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitPrimaryExprCompare(MySQLParser::PrimaryExprCompareContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitPrimaryExprAllAny(MySQLParser::PrimaryExprAllAnyContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitPrimaryExprIsNull(MySQLParser::PrimaryExprIsNullContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitCompOp(MySQLParser::CompOpContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitPredicate(MySQLParser::PredicateContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitPredicateExprIn(MySQLParser::PredicateExprInContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitPredicateExprBetween(MySQLParser::PredicateExprBetweenContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitPredicateExprLike(MySQLParser::PredicateExprLikeContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitPredicateExprRegex(MySQLParser::PredicateExprRegexContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitBitExpr(MySQLParser::BitExprContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSimpleExprConvert(MySQLParser::SimpleExprConvertContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSimpleExprCast(MySQLParser::SimpleExprCastContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSimpleExprUnary(MySQLParser::SimpleExprUnaryContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSimpleExpressionRValue(MySQLParser::SimpleExpressionRValueContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSimpleExprOdbc(MySQLParser::SimpleExprOdbcContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSimpleExprRuntimeFunction(MySQLParser::SimpleExprRuntimeFunctionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSimpleExprFunction(MySQLParser::SimpleExprFunctionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSimpleExprCollate(MySQLParser::SimpleExprCollateContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSimpleExprMatch(MySQLParser::SimpleExprMatchContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSimpleExprWindowingFunction(MySQLParser::SimpleExprWindowingFunctionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSimpleExprBinary(MySQLParser::SimpleExprBinaryContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSimpleExprColumnRef(MySQLParser::SimpleExprColumnRefContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSimpleExprParamMarker(MySQLParser::SimpleExprParamMarkerContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSimpleExprSum(MySQLParser::SimpleExprSumContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSimpleExprCastTime(MySQLParser::SimpleExprCastTimeContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSimpleExprConvertUsing(MySQLParser::SimpleExprConvertUsingContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSimpleExprSubQuery(MySQLParser::SimpleExprSubQueryContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSimpleExprGroupingOperation(MySQLParser::SimpleExprGroupingOperationContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSimpleExprNot(MySQLParser::SimpleExprNotContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSimpleExprValues(MySQLParser::SimpleExprValuesContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSimpleExprUserVariableAssignment(MySQLParser::SimpleExprUserVariableAssignmentContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSimpleExprDefault(MySQLParser::SimpleExprDefaultContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSimpleExprList(MySQLParser::SimpleExprListContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSimpleExprInterval(MySQLParser::SimpleExprIntervalContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSimpleExprCase(MySQLParser::SimpleExprCaseContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSimpleExprConcat(MySQLParser::SimpleExprConcatContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSimpleExprLiteral(MySQLParser::SimpleExprLiteralContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitArrayCast(MySQLParser::ArrayCastContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitJsonOperator(MySQLParser::JsonOperatorContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSumExpr(MySQLParser::SumExprContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitGroupingOperation(MySQLParser::GroupingOperationContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitWindowFunctionCall(MySQLParser::WindowFunctionCallContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitWindowingClause(MySQLParser::WindowingClauseContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLeadLagInfo(MySQLParser::LeadLagInfoContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitStableInteger(MySQLParser::StableIntegerContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitParamOrVar(MySQLParser::ParamOrVarContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitNullTreatment(MySQLParser::NullTreatmentContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitJsonFunction(MySQLParser::JsonFunctionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitInSumExpr(MySQLParser::InSumExprContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitIdentListArg(MySQLParser::IdentListArgContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitIdentList(MySQLParser::IdentListContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFulltextOptions(MySQLParser::FulltextOptionsContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitRuntimeFunctionCall(MySQLParser::RuntimeFunctionCallContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitReturningType(MySQLParser::ReturningTypeContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitGeometryFunction(MySQLParser::GeometryFunctionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTimeFunctionParameters(MySQLParser::TimeFunctionParametersContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFractionalPrecision(MySQLParser::FractionalPrecisionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitWeightStringLevels(MySQLParser::WeightStringLevelsContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitWeightStringLevelListItem(MySQLParser::WeightStringLevelListItemContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitDateTimeTtype(MySQLParser::DateTimeTtypeContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTrimFunction(MySQLParser::TrimFunctionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSubstringFunction(MySQLParser::SubstringFunctionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFunctionCall(MySQLParser::FunctionCallContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitUdfExprList(MySQLParser::UdfExprListContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitUdfExpr(MySQLParser::UdfExprContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitUserVariable(MySQLParser::UserVariableContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitUserVariableIdentifier(MySQLParser::UserVariableIdentifierContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitInExpressionUserVariableAssignment(MySQLParser::InExpressionUserVariableAssignmentContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitRvalueSystemOrUserVariable(MySQLParser::RvalueSystemOrUserVariableContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLvalueVariable(MySQLParser::LvalueVariableContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitRvalueSystemVariable(MySQLParser::RvalueSystemVariableContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitWhenExpression(MySQLParser::WhenExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitThenExpression(MySQLParser::ThenExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitElseExpression(MySQLParser::ElseExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitCastType(MySQLParser::CastTypeContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitExprList(MySQLParser::ExprListContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitCharset(MySQLParser::CharsetContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitNotRule(MySQLParser::NotRuleContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitNot2Rule(MySQLParser::Not2RuleContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitInterval(MySQLParser::IntervalContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitIntervalTimeStamp(MySQLParser::IntervalTimeStampContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitExprListWithParentheses(MySQLParser::ExprListWithParenthesesContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitExprWithParentheses(MySQLParser::ExprWithParenthesesContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSimpleExprWithParentheses(MySQLParser::SimpleExprWithParenthesesContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitOrderList(MySQLParser::OrderListContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitOrderExpression(MySQLParser::OrderExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitGroupList(MySQLParser::GroupListContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitGroupingExpression(MySQLParser::GroupingExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitChannel(MySQLParser::ChannelContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitCompoundStatement(MySQLParser::CompoundStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitReturnStatement(MySQLParser::ReturnStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitIfStatement(MySQLParser::IfStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitIfBody(MySQLParser::IfBodyContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitThenStatement(MySQLParser::ThenStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitCompoundStatementList(MySQLParser::CompoundStatementListContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitCaseStatement(MySQLParser::CaseStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitElseStatement(MySQLParser::ElseStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLabeledBlock(MySQLParser::LabeledBlockContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitUnlabeledBlock(MySQLParser::UnlabeledBlockContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLabel(MySQLParser::LabelContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitBeginEndBlock(MySQLParser::BeginEndBlockContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLabeledControl(MySQLParser::LabeledControlContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitUnlabeledControl(MySQLParser::UnlabeledControlContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLoopBlock(MySQLParser::LoopBlockContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitWhileDoBlock(MySQLParser::WhileDoBlockContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitRepeatUntilBlock(MySQLParser::RepeatUntilBlockContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSpDeclarations(MySQLParser::SpDeclarationsContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSpDeclaration(MySQLParser::SpDeclarationContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitVariableDeclaration(MySQLParser::VariableDeclarationContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitConditionDeclaration(MySQLParser::ConditionDeclarationContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSpCondition(MySQLParser::SpConditionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSqlstate(MySQLParser::SqlstateContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitHandlerDeclaration(MySQLParser::HandlerDeclarationContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitHandlerCondition(MySQLParser::HandlerConditionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitCursorDeclaration(MySQLParser::CursorDeclarationContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitIterateStatement(MySQLParser::IterateStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLeaveStatement(MySQLParser::LeaveStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitGetDiagnosticsStatement(MySQLParser::GetDiagnosticsStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSignalAllowedExpr(MySQLParser::SignalAllowedExprContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitStatementInformationItem(MySQLParser::StatementInformationItemContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitConditionInformationItem(MySQLParser::ConditionInformationItemContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSignalInformationItemName(MySQLParser::SignalInformationItemNameContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSignalStatement(MySQLParser::SignalStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitResignalStatement(MySQLParser::ResignalStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSignalInformationItem(MySQLParser::SignalInformationItemContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitCursorOpen(MySQLParser::CursorOpenContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitCursorClose(MySQLParser::CursorCloseContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitCursorFetch(MySQLParser::CursorFetchContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSchedule(MySQLParser::ScheduleContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitColumnDefinition(MySQLParser::ColumnDefinitionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitCheckOrReferences(MySQLParser::CheckOrReferencesContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitCheckConstraint(MySQLParser::CheckConstraintContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitConstraintEnforcement(MySQLParser::ConstraintEnforcementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTableConstraintDef(MySQLParser::TableConstraintDefContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitConstraintName(MySQLParser::ConstraintNameContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFieldDefinition(MySQLParser::FieldDefinitionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitColumnAttribute(MySQLParser::ColumnAttributeContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitColumnFormat(MySQLParser::ColumnFormatContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitStorageMedia(MySQLParser::StorageMediaContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitNow(MySQLParser::NowContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitNowOrSignedLiteral(MySQLParser::NowOrSignedLiteralContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitGcolAttribute(MySQLParser::GcolAttributeContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitReferences(MySQLParser::ReferencesContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitDeleteOption(MySQLParser::DeleteOptionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitKeyList(MySQLParser::KeyListContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitKeyPart(MySQLParser::KeyPartContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitKeyListWithExpression(MySQLParser::KeyListWithExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitKeyPartOrExpression(MySQLParser::KeyPartOrExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitKeyListVariants(MySQLParser::KeyListVariantsContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitIndexType(MySQLParser::IndexTypeContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitIndexOption(MySQLParser::IndexOptionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitCommonIndexOption(MySQLParser::CommonIndexOptionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitVisibility(MySQLParser::VisibilityContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitIndexTypeClause(MySQLParser::IndexTypeClauseContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFulltextIndexOption(MySQLParser::FulltextIndexOptionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSpatialIndexOption(MySQLParser::SpatialIndexOptionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitDataTypeDefinition(MySQLParser::DataTypeDefinitionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitDataType(MySQLParser::DataTypeContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitNchar(MySQLParser::NcharContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitRealType(MySQLParser::RealTypeContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFieldLength(MySQLParser::FieldLengthContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFieldOptions(MySQLParser::FieldOptionsContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitCharsetWithOptBinary(MySQLParser::CharsetWithOptBinaryContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAscii(MySQLParser::AsciiContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitUnicode(MySQLParser::UnicodeContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitWsNumCodepoints(MySQLParser::WsNumCodepointsContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTypeDatetimePrecision(MySQLParser::TypeDatetimePrecisionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFunctionDatetimePrecision(MySQLParser::FunctionDatetimePrecisionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitCharsetName(MySQLParser::CharsetNameContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitCollationName(MySQLParser::CollationNameContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitCreateTableOptions(MySQLParser::CreateTableOptionsContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitCreateTableOptionsSpaceSeparated(MySQLParser::CreateTableOptionsSpaceSeparatedContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitCreateTableOption(MySQLParser::CreateTableOptionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTernaryOption(MySQLParser::TernaryOptionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitDefaultCollation(MySQLParser::DefaultCollationContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitDefaultEncryption(MySQLParser::DefaultEncryptionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitDefaultCharset(MySQLParser::DefaultCharsetContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitPartitionClause(MySQLParser::PartitionClauseContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitPartitionDefKey(MySQLParser::PartitionDefKeyContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitPartitionDefHash(MySQLParser::PartitionDefHashContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitPartitionDefRangeList(MySQLParser::PartitionDefRangeListContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSubPartitions(MySQLParser::SubPartitionsContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitPartitionKeyAlgorithm(MySQLParser::PartitionKeyAlgorithmContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitPartitionDefinitions(MySQLParser::PartitionDefinitionsContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitPartitionDefinition(MySQLParser::PartitionDefinitionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitPartitionValuesIn(MySQLParser::PartitionValuesInContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitPartitionOption(MySQLParser::PartitionOptionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSubpartitionDefinition(MySQLParser::SubpartitionDefinitionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitPartitionValueItemListParen(MySQLParser::PartitionValueItemListParenContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitPartitionValueItem(MySQLParser::PartitionValueItemContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitDefinerClause(MySQLParser::DefinerClauseContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitIfExists(MySQLParser::IfExistsContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitIfNotExists(MySQLParser::IfNotExistsContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitProcedureParameter(MySQLParser::ProcedureParameterContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFunctionParameter(MySQLParser::FunctionParameterContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitCollate(MySQLParser::CollateContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTypeWithOptCollate(MySQLParser::TypeWithOptCollateContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSchemaIdentifierPair(MySQLParser::SchemaIdentifierPairContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitViewRefList(MySQLParser::ViewRefListContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitUpdateList(MySQLParser::UpdateListContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitUpdateElement(MySQLParser::UpdateElementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitCharsetClause(MySQLParser::CharsetClauseContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFieldsClause(MySQLParser::FieldsClauseContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFieldTerm(MySQLParser::FieldTermContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLinesClause(MySQLParser::LinesClauseContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLineTerm(MySQLParser::LineTermContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitUserList(MySQLParser::UserListContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitCreateUserList(MySQLParser::CreateUserListContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitCreateUser(MySQLParser::CreateUserContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitCreateUserWithMfa(MySQLParser::CreateUserWithMfaContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitIdentification(MySQLParser::IdentificationContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitIdentifiedByPassword(MySQLParser::IdentifiedByPasswordContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitIdentifiedByRandomPassword(MySQLParser::IdentifiedByRandomPasswordContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitIdentifiedWithPlugin(MySQLParser::IdentifiedWithPluginContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitIdentifiedWithPluginAsAuth(MySQLParser::IdentifiedWithPluginAsAuthContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitIdentifiedWithPluginByPassword(MySQLParser::IdentifiedWithPluginByPasswordContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitIdentifiedWithPluginByRandomPassword(MySQLParser::IdentifiedWithPluginByRandomPasswordContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitInitialAuth(MySQLParser::InitialAuthContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitRetainCurrentPassword(MySQLParser::RetainCurrentPasswordContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitDiscardOldPassword(MySQLParser::DiscardOldPasswordContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitUserRegistration(MySQLParser::UserRegistrationContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFactor(MySQLParser::FactorContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitReplacePassword(MySQLParser::ReplacePasswordContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitUserIdentifierOrText(MySQLParser::UserIdentifierOrTextContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitUser(MySQLParser::UserContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLikeClause(MySQLParser::LikeClauseContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLikeOrWhere(MySQLParser::LikeOrWhereContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitOnlineOption(MySQLParser::OnlineOptionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitNoWriteToBinLog(MySQLParser::NoWriteToBinLogContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitUsePartition(MySQLParser::UsePartitionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFieldIdentifier(MySQLParser::FieldIdentifierContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitColumnName(MySQLParser::ColumnNameContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitColumnInternalRef(MySQLParser::ColumnInternalRefContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitColumnInternalRefList(MySQLParser::ColumnInternalRefListContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitColumnRef(MySQLParser::ColumnRefContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitInsertIdentifier(MySQLParser::InsertIdentifierContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitIndexName(MySQLParser::IndexNameContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitIndexRef(MySQLParser::IndexRefContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTableWild(MySQLParser::TableWildContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSchemaName(MySQLParser::SchemaNameContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSchemaRef(MySQLParser::SchemaRefContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitProcedureName(MySQLParser::ProcedureNameContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitProcedureRef(MySQLParser::ProcedureRefContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFunctionName(MySQLParser::FunctionNameContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFunctionRef(MySQLParser::FunctionRefContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTriggerName(MySQLParser::TriggerNameContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTriggerRef(MySQLParser::TriggerRefContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitViewName(MySQLParser::ViewNameContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitViewRef(MySQLParser::ViewRefContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTablespaceName(MySQLParser::TablespaceNameContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTablespaceRef(MySQLParser::TablespaceRefContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLogfileGroupName(MySQLParser::LogfileGroupNameContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLogfileGroupRef(MySQLParser::LogfileGroupRefContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitEventName(MySQLParser::EventNameContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitEventRef(MySQLParser::EventRefContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitUdfName(MySQLParser::UdfNameContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitServerName(MySQLParser::ServerNameContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitServerRef(MySQLParser::ServerRefContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitEngineRef(MySQLParser::EngineRefContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTableName(MySQLParser::TableNameContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFilterTableRef(MySQLParser::FilterTableRefContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTableRefWithWildcard(MySQLParser::TableRefWithWildcardContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTableRef(MySQLParser::TableRefContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTableRefList(MySQLParser::TableRefListContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTableAliasRefList(MySQLParser::TableAliasRefListContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitParameterName(MySQLParser::ParameterNameContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLabelIdentifier(MySQLParser::LabelIdentifierContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLabelRef(MySQLParser::LabelRefContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitRoleIdentifier(MySQLParser::RoleIdentifierContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitPluginRef(MySQLParser::PluginRefContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitComponentRef(MySQLParser::ComponentRefContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitResourceGroupRef(MySQLParser::ResourceGroupRefContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitWindowName(MySQLParser::WindowNameContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitPureIdentifier(MySQLParser::PureIdentifierContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitIdentifier(MySQLParser::IdentifierContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitIdentifierList(MySQLParser::IdentifierListContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitIdentifierListWithParentheses(MySQLParser::IdentifierListWithParenthesesContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitQualifiedIdentifier(MySQLParser::QualifiedIdentifierContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSimpleIdentifier(MySQLParser::SimpleIdentifierContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitDotIdentifier(MySQLParser::DotIdentifierContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitUlong_number(MySQLParser::Ulong_numberContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitReal_ulong_number(MySQLParser::Real_ulong_numberContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitUlonglong_number(MySQLParser::Ulonglong_numberContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitReal_ulonglong_number(MySQLParser::Real_ulonglong_numberContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSignedLiteral(MySQLParser::SignedLiteralContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSignedLiteralOrNull(MySQLParser::SignedLiteralOrNullContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLiteral(MySQLParser::LiteralContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLiteralOrNull(MySQLParser::LiteralOrNullContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitNullAsLiteral(MySQLParser::NullAsLiteralContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitStringList(MySQLParser::StringListContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTextStringLiteral(MySQLParser::TextStringLiteralContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTextString(MySQLParser::TextStringContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTextStringHash(MySQLParser::TextStringHashContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTextLiteral(MySQLParser::TextLiteralContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTextStringNoLinebreak(MySQLParser::TextStringNoLinebreakContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTextStringLiteralList(MySQLParser::TextStringLiteralListContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitNumLiteral(MySQLParser::NumLiteralContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitBoolLiteral(MySQLParser::BoolLiteralContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitNullLiteral(MySQLParser::NullLiteralContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitInt64Literal(MySQLParser::Int64LiteralContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTemporalLiteral(MySQLParser::TemporalLiteralContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFloatOptions(MySQLParser::FloatOptionsContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitStandardFloatOptions(MySQLParser::StandardFloatOptionsContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitPrecision(MySQLParser::PrecisionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTextOrIdentifier(MySQLParser::TextOrIdentifierContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLValueIdentifier(MySQLParser::LValueIdentifierContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitRoleIdentifierOrText(MySQLParser::RoleIdentifierOrTextContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSizeNumber(MySQLParser::SizeNumberContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitParentheses(MySQLParser::ParenthesesContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitEqual(MySQLParser::EqualContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitOptionType(MySQLParser::OptionTypeContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitRvalueSystemVariableType(MySQLParser::RvalueSystemVariableTypeContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSetVarIdentType(MySQLParser::SetVarIdentTypeContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitJsonAttribute(MySQLParser::JsonAttributeContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitIdentifierKeyword(MySQLParser::IdentifierKeywordContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitIdentifierKeywordsAmbiguous1RolesAndLabels(MySQLParser::IdentifierKeywordsAmbiguous1RolesAndLabelsContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitIdentifierKeywordsAmbiguous2Labels(MySQLParser::IdentifierKeywordsAmbiguous2LabelsContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLabelKeyword(MySQLParser::LabelKeywordContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitIdentifierKeywordsAmbiguous3Roles(MySQLParser::IdentifierKeywordsAmbiguous3RolesContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitIdentifierKeywordsUnambiguous(MySQLParser::IdentifierKeywordsUnambiguousContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitRoleKeyword(MySQLParser::RoleKeywordContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLValueKeyword(MySQLParser::LValueKeywordContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitIdentifierKeywordsAmbiguous4SystemVariables(MySQLParser::IdentifierKeywordsAmbiguous4SystemVariablesContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitRoleOrIdentifierKeyword(MySQLParser::RoleOrIdentifierKeywordContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitRoleOrLabelKeyword(MySQLParser::RoleOrLabelKeywordContext *ctx) override {
    return visitChildren(ctx);
  }


};

}  // namespace parsers
