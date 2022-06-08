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




// Generated from /home/paandrus/dev/ngshell/mysqlshdk/libs/parser/grammars/MySQLParser.g4 by ANTLR 4.10.1

#pragma once


#include "antlr4-runtime.h"
#include "MySQLParser.h"


namespace parsers {

/**
 * This class defines an abstract visitor for a parse tree
 * produced by MySQLParser.
 */
class PARSERS_PUBLIC_TYPE MySQLParserVisitor : public antlr4::tree::AbstractParseTreeVisitor {
public:

  /**
   * Visit parse trees produced by MySQLParser.
   */
    virtual std::any visitQuery(MySQLParser::QueryContext *context) = 0;

    virtual std::any visitSimpleStatement(MySQLParser::SimpleStatementContext *context) = 0;

    virtual std::any visitAlterStatement(MySQLParser::AlterStatementContext *context) = 0;

    virtual std::any visitAlterDatabase(MySQLParser::AlterDatabaseContext *context) = 0;

    virtual std::any visitAlterDatabaseOption(MySQLParser::AlterDatabaseOptionContext *context) = 0;

    virtual std::any visitAlterEvent(MySQLParser::AlterEventContext *context) = 0;

    virtual std::any visitAlterLogfileGroup(MySQLParser::AlterLogfileGroupContext *context) = 0;

    virtual std::any visitAlterLogfileGroupOptions(MySQLParser::AlterLogfileGroupOptionsContext *context) = 0;

    virtual std::any visitAlterLogfileGroupOption(MySQLParser::AlterLogfileGroupOptionContext *context) = 0;

    virtual std::any visitAlterServer(MySQLParser::AlterServerContext *context) = 0;

    virtual std::any visitAlterTable(MySQLParser::AlterTableContext *context) = 0;

    virtual std::any visitAlterTableActions(MySQLParser::AlterTableActionsContext *context) = 0;

    virtual std::any visitAlterCommandList(MySQLParser::AlterCommandListContext *context) = 0;

    virtual std::any visitAlterCommandsModifierList(MySQLParser::AlterCommandsModifierListContext *context) = 0;

    virtual std::any visitStandaloneAlterCommands(MySQLParser::StandaloneAlterCommandsContext *context) = 0;

    virtual std::any visitAlterPartition(MySQLParser::AlterPartitionContext *context) = 0;

    virtual std::any visitAlterList(MySQLParser::AlterListContext *context) = 0;

    virtual std::any visitAlterCommandsModifier(MySQLParser::AlterCommandsModifierContext *context) = 0;

    virtual std::any visitAlterListItem(MySQLParser::AlterListItemContext *context) = 0;

    virtual std::any visitPlace(MySQLParser::PlaceContext *context) = 0;

    virtual std::any visitRestrict(MySQLParser::RestrictContext *context) = 0;

    virtual std::any visitAlterOrderList(MySQLParser::AlterOrderListContext *context) = 0;

    virtual std::any visitAlterAlgorithmOption(MySQLParser::AlterAlgorithmOptionContext *context) = 0;

    virtual std::any visitAlterLockOption(MySQLParser::AlterLockOptionContext *context) = 0;

    virtual std::any visitIndexLockAndAlgorithm(MySQLParser::IndexLockAndAlgorithmContext *context) = 0;

    virtual std::any visitWithValidation(MySQLParser::WithValidationContext *context) = 0;

    virtual std::any visitRemovePartitioning(MySQLParser::RemovePartitioningContext *context) = 0;

    virtual std::any visitAllOrPartitionNameList(MySQLParser::AllOrPartitionNameListContext *context) = 0;

    virtual std::any visitAlterTablespace(MySQLParser::AlterTablespaceContext *context) = 0;

    virtual std::any visitAlterUndoTablespace(MySQLParser::AlterUndoTablespaceContext *context) = 0;

    virtual std::any visitUndoTableSpaceOptions(MySQLParser::UndoTableSpaceOptionsContext *context) = 0;

    virtual std::any visitUndoTableSpaceOption(MySQLParser::UndoTableSpaceOptionContext *context) = 0;

    virtual std::any visitAlterTablespaceOptions(MySQLParser::AlterTablespaceOptionsContext *context) = 0;

    virtual std::any visitAlterTablespaceOption(MySQLParser::AlterTablespaceOptionContext *context) = 0;

    virtual std::any visitChangeTablespaceOption(MySQLParser::ChangeTablespaceOptionContext *context) = 0;

    virtual std::any visitAlterView(MySQLParser::AlterViewContext *context) = 0;

    virtual std::any visitViewTail(MySQLParser::ViewTailContext *context) = 0;

    virtual std::any visitViewQueryBlock(MySQLParser::ViewQueryBlockContext *context) = 0;

    virtual std::any visitViewCheckOption(MySQLParser::ViewCheckOptionContext *context) = 0;

    virtual std::any visitAlterInstanceStatement(MySQLParser::AlterInstanceStatementContext *context) = 0;

    virtual std::any visitCreateStatement(MySQLParser::CreateStatementContext *context) = 0;

    virtual std::any visitCreateDatabase(MySQLParser::CreateDatabaseContext *context) = 0;

    virtual std::any visitCreateDatabaseOption(MySQLParser::CreateDatabaseOptionContext *context) = 0;

    virtual std::any visitCreateTable(MySQLParser::CreateTableContext *context) = 0;

    virtual std::any visitTableElementList(MySQLParser::TableElementListContext *context) = 0;

    virtual std::any visitTableElement(MySQLParser::TableElementContext *context) = 0;

    virtual std::any visitDuplicateAsQueryExpression(MySQLParser::DuplicateAsQueryExpressionContext *context) = 0;

    virtual std::any visitQueryExpressionOrParens(MySQLParser::QueryExpressionOrParensContext *context) = 0;

    virtual std::any visitCreateRoutine(MySQLParser::CreateRoutineContext *context) = 0;

    virtual std::any visitCreateProcedure(MySQLParser::CreateProcedureContext *context) = 0;

    virtual std::any visitCreateFunction(MySQLParser::CreateFunctionContext *context) = 0;

    virtual std::any visitCreateUdf(MySQLParser::CreateUdfContext *context) = 0;

    virtual std::any visitRoutineCreateOption(MySQLParser::RoutineCreateOptionContext *context) = 0;

    virtual std::any visitRoutineAlterOptions(MySQLParser::RoutineAlterOptionsContext *context) = 0;

    virtual std::any visitRoutineOption(MySQLParser::RoutineOptionContext *context) = 0;

    virtual std::any visitCreateIndex(MySQLParser::CreateIndexContext *context) = 0;

    virtual std::any visitIndexNameAndType(MySQLParser::IndexNameAndTypeContext *context) = 0;

    virtual std::any visitCreateIndexTarget(MySQLParser::CreateIndexTargetContext *context) = 0;

    virtual std::any visitCreateLogfileGroup(MySQLParser::CreateLogfileGroupContext *context) = 0;

    virtual std::any visitLogfileGroupOptions(MySQLParser::LogfileGroupOptionsContext *context) = 0;

    virtual std::any visitLogfileGroupOption(MySQLParser::LogfileGroupOptionContext *context) = 0;

    virtual std::any visitCreateServer(MySQLParser::CreateServerContext *context) = 0;

    virtual std::any visitServerOptions(MySQLParser::ServerOptionsContext *context) = 0;

    virtual std::any visitServerOption(MySQLParser::ServerOptionContext *context) = 0;

    virtual std::any visitCreateTablespace(MySQLParser::CreateTablespaceContext *context) = 0;

    virtual std::any visitCreateUndoTablespace(MySQLParser::CreateUndoTablespaceContext *context) = 0;

    virtual std::any visitTsDataFileName(MySQLParser::TsDataFileNameContext *context) = 0;

    virtual std::any visitTsDataFile(MySQLParser::TsDataFileContext *context) = 0;

    virtual std::any visitTablespaceOptions(MySQLParser::TablespaceOptionsContext *context) = 0;

    virtual std::any visitTablespaceOption(MySQLParser::TablespaceOptionContext *context) = 0;

    virtual std::any visitTsOptionInitialSize(MySQLParser::TsOptionInitialSizeContext *context) = 0;

    virtual std::any visitTsOptionUndoRedoBufferSize(MySQLParser::TsOptionUndoRedoBufferSizeContext *context) = 0;

    virtual std::any visitTsOptionAutoextendSize(MySQLParser::TsOptionAutoextendSizeContext *context) = 0;

    virtual std::any visitTsOptionMaxSize(MySQLParser::TsOptionMaxSizeContext *context) = 0;

    virtual std::any visitTsOptionExtentSize(MySQLParser::TsOptionExtentSizeContext *context) = 0;

    virtual std::any visitTsOptionNodegroup(MySQLParser::TsOptionNodegroupContext *context) = 0;

    virtual std::any visitTsOptionEngine(MySQLParser::TsOptionEngineContext *context) = 0;

    virtual std::any visitTsOptionWait(MySQLParser::TsOptionWaitContext *context) = 0;

    virtual std::any visitTsOptionComment(MySQLParser::TsOptionCommentContext *context) = 0;

    virtual std::any visitTsOptionFileblockSize(MySQLParser::TsOptionFileblockSizeContext *context) = 0;

    virtual std::any visitTsOptionEncryption(MySQLParser::TsOptionEncryptionContext *context) = 0;

    virtual std::any visitTsOptionEngineAttribute(MySQLParser::TsOptionEngineAttributeContext *context) = 0;

    virtual std::any visitCreateView(MySQLParser::CreateViewContext *context) = 0;

    virtual std::any visitViewReplaceOrAlgorithm(MySQLParser::ViewReplaceOrAlgorithmContext *context) = 0;

    virtual std::any visitViewAlgorithm(MySQLParser::ViewAlgorithmContext *context) = 0;

    virtual std::any visitViewSuid(MySQLParser::ViewSuidContext *context) = 0;

    virtual std::any visitCreateTrigger(MySQLParser::CreateTriggerContext *context) = 0;

    virtual std::any visitTriggerFollowsPrecedesClause(MySQLParser::TriggerFollowsPrecedesClauseContext *context) = 0;

    virtual std::any visitCreateEvent(MySQLParser::CreateEventContext *context) = 0;

    virtual std::any visitCreateRole(MySQLParser::CreateRoleContext *context) = 0;

    virtual std::any visitCreateSpatialReference(MySQLParser::CreateSpatialReferenceContext *context) = 0;

    virtual std::any visitSrsAttribute(MySQLParser::SrsAttributeContext *context) = 0;

    virtual std::any visitDropStatement(MySQLParser::DropStatementContext *context) = 0;

    virtual std::any visitDropDatabase(MySQLParser::DropDatabaseContext *context) = 0;

    virtual std::any visitDropEvent(MySQLParser::DropEventContext *context) = 0;

    virtual std::any visitDropFunction(MySQLParser::DropFunctionContext *context) = 0;

    virtual std::any visitDropProcedure(MySQLParser::DropProcedureContext *context) = 0;

    virtual std::any visitDropIndex(MySQLParser::DropIndexContext *context) = 0;

    virtual std::any visitDropLogfileGroup(MySQLParser::DropLogfileGroupContext *context) = 0;

    virtual std::any visitDropLogfileGroupOption(MySQLParser::DropLogfileGroupOptionContext *context) = 0;

    virtual std::any visitDropServer(MySQLParser::DropServerContext *context) = 0;

    virtual std::any visitDropTable(MySQLParser::DropTableContext *context) = 0;

    virtual std::any visitDropTableSpace(MySQLParser::DropTableSpaceContext *context) = 0;

    virtual std::any visitDropTrigger(MySQLParser::DropTriggerContext *context) = 0;

    virtual std::any visitDropView(MySQLParser::DropViewContext *context) = 0;

    virtual std::any visitDropRole(MySQLParser::DropRoleContext *context) = 0;

    virtual std::any visitDropSpatialReference(MySQLParser::DropSpatialReferenceContext *context) = 0;

    virtual std::any visitDropUndoTablespace(MySQLParser::DropUndoTablespaceContext *context) = 0;

    virtual std::any visitRenameTableStatement(MySQLParser::RenameTableStatementContext *context) = 0;

    virtual std::any visitRenamePair(MySQLParser::RenamePairContext *context) = 0;

    virtual std::any visitTruncateTableStatement(MySQLParser::TruncateTableStatementContext *context) = 0;

    virtual std::any visitImportStatement(MySQLParser::ImportStatementContext *context) = 0;

    virtual std::any visitCallStatement(MySQLParser::CallStatementContext *context) = 0;

    virtual std::any visitDeleteStatement(MySQLParser::DeleteStatementContext *context) = 0;

    virtual std::any visitPartitionDelete(MySQLParser::PartitionDeleteContext *context) = 0;

    virtual std::any visitDeleteStatementOption(MySQLParser::DeleteStatementOptionContext *context) = 0;

    virtual std::any visitDoStatement(MySQLParser::DoStatementContext *context) = 0;

    virtual std::any visitHandlerStatement(MySQLParser::HandlerStatementContext *context) = 0;

    virtual std::any visitHandlerReadOrScan(MySQLParser::HandlerReadOrScanContext *context) = 0;

    virtual std::any visitInsertStatement(MySQLParser::InsertStatementContext *context) = 0;

    virtual std::any visitInsertLockOption(MySQLParser::InsertLockOptionContext *context) = 0;

    virtual std::any visitInsertFromConstructor(MySQLParser::InsertFromConstructorContext *context) = 0;

    virtual std::any visitFields(MySQLParser::FieldsContext *context) = 0;

    virtual std::any visitInsertValues(MySQLParser::InsertValuesContext *context) = 0;

    virtual std::any visitInsertQueryExpression(MySQLParser::InsertQueryExpressionContext *context) = 0;

    virtual std::any visitValueList(MySQLParser::ValueListContext *context) = 0;

    virtual std::any visitValues(MySQLParser::ValuesContext *context) = 0;

    virtual std::any visitValuesReference(MySQLParser::ValuesReferenceContext *context) = 0;

    virtual std::any visitInsertUpdateList(MySQLParser::InsertUpdateListContext *context) = 0;

    virtual std::any visitLoadStatement(MySQLParser::LoadStatementContext *context) = 0;

    virtual std::any visitDataOrXml(MySQLParser::DataOrXmlContext *context) = 0;

    virtual std::any visitXmlRowsIdentifiedBy(MySQLParser::XmlRowsIdentifiedByContext *context) = 0;

    virtual std::any visitLoadDataFileTail(MySQLParser::LoadDataFileTailContext *context) = 0;

    virtual std::any visitLoadDataFileTargetList(MySQLParser::LoadDataFileTargetListContext *context) = 0;

    virtual std::any visitFieldOrVariableList(MySQLParser::FieldOrVariableListContext *context) = 0;

    virtual std::any visitReplaceStatement(MySQLParser::ReplaceStatementContext *context) = 0;

    virtual std::any visitSelectStatement(MySQLParser::SelectStatementContext *context) = 0;

    virtual std::any visitSelectStatementWithInto(MySQLParser::SelectStatementWithIntoContext *context) = 0;

    virtual std::any visitQueryExpression(MySQLParser::QueryExpressionContext *context) = 0;

    virtual std::any visitQueryExpressionBody(MySQLParser::QueryExpressionBodyContext *context) = 0;

    virtual std::any visitQueryExpressionParens(MySQLParser::QueryExpressionParensContext *context) = 0;

    virtual std::any visitQueryPrimary(MySQLParser::QueryPrimaryContext *context) = 0;

    virtual std::any visitQuerySpecification(MySQLParser::QuerySpecificationContext *context) = 0;

    virtual std::any visitSubquery(MySQLParser::SubqueryContext *context) = 0;

    virtual std::any visitQuerySpecOption(MySQLParser::QuerySpecOptionContext *context) = 0;

    virtual std::any visitLimitClause(MySQLParser::LimitClauseContext *context) = 0;

    virtual std::any visitSimpleLimitClause(MySQLParser::SimpleLimitClauseContext *context) = 0;

    virtual std::any visitLimitOptions(MySQLParser::LimitOptionsContext *context) = 0;

    virtual std::any visitLimitOption(MySQLParser::LimitOptionContext *context) = 0;

    virtual std::any visitIntoClause(MySQLParser::IntoClauseContext *context) = 0;

    virtual std::any visitProcedureAnalyseClause(MySQLParser::ProcedureAnalyseClauseContext *context) = 0;

    virtual std::any visitHavingClause(MySQLParser::HavingClauseContext *context) = 0;

    virtual std::any visitWindowClause(MySQLParser::WindowClauseContext *context) = 0;

    virtual std::any visitWindowDefinition(MySQLParser::WindowDefinitionContext *context) = 0;

    virtual std::any visitWindowSpec(MySQLParser::WindowSpecContext *context) = 0;

    virtual std::any visitWindowSpecDetails(MySQLParser::WindowSpecDetailsContext *context) = 0;

    virtual std::any visitWindowFrameClause(MySQLParser::WindowFrameClauseContext *context) = 0;

    virtual std::any visitWindowFrameUnits(MySQLParser::WindowFrameUnitsContext *context) = 0;

    virtual std::any visitWindowFrameExtent(MySQLParser::WindowFrameExtentContext *context) = 0;

    virtual std::any visitWindowFrameStart(MySQLParser::WindowFrameStartContext *context) = 0;

    virtual std::any visitWindowFrameBetween(MySQLParser::WindowFrameBetweenContext *context) = 0;

    virtual std::any visitWindowFrameBound(MySQLParser::WindowFrameBoundContext *context) = 0;

    virtual std::any visitWindowFrameExclusion(MySQLParser::WindowFrameExclusionContext *context) = 0;

    virtual std::any visitWithClause(MySQLParser::WithClauseContext *context) = 0;

    virtual std::any visitCommonTableExpression(MySQLParser::CommonTableExpressionContext *context) = 0;

    virtual std::any visitGroupByClause(MySQLParser::GroupByClauseContext *context) = 0;

    virtual std::any visitOlapOption(MySQLParser::OlapOptionContext *context) = 0;

    virtual std::any visitOrderClause(MySQLParser::OrderClauseContext *context) = 0;

    virtual std::any visitDirection(MySQLParser::DirectionContext *context) = 0;

    virtual std::any visitFromClause(MySQLParser::FromClauseContext *context) = 0;

    virtual std::any visitTableReferenceList(MySQLParser::TableReferenceListContext *context) = 0;

    virtual std::any visitTableValueConstructor(MySQLParser::TableValueConstructorContext *context) = 0;

    virtual std::any visitExplicitTable(MySQLParser::ExplicitTableContext *context) = 0;

    virtual std::any visitRowValueExplicit(MySQLParser::RowValueExplicitContext *context) = 0;

    virtual std::any visitSelectOption(MySQLParser::SelectOptionContext *context) = 0;

    virtual std::any visitLockingClauseList(MySQLParser::LockingClauseListContext *context) = 0;

    virtual std::any visitLockingClause(MySQLParser::LockingClauseContext *context) = 0;

    virtual std::any visitLockStrengh(MySQLParser::LockStrenghContext *context) = 0;

    virtual std::any visitLockedRowAction(MySQLParser::LockedRowActionContext *context) = 0;

    virtual std::any visitSelectItemList(MySQLParser::SelectItemListContext *context) = 0;

    virtual std::any visitSelectItem(MySQLParser::SelectItemContext *context) = 0;

    virtual std::any visitSelectAlias(MySQLParser::SelectAliasContext *context) = 0;

    virtual std::any visitWhereClause(MySQLParser::WhereClauseContext *context) = 0;

    virtual std::any visitTableReference(MySQLParser::TableReferenceContext *context) = 0;

    virtual std::any visitEscapedTableReference(MySQLParser::EscapedTableReferenceContext *context) = 0;

    virtual std::any visitJoinedTable(MySQLParser::JoinedTableContext *context) = 0;

    virtual std::any visitNaturalJoinType(MySQLParser::NaturalJoinTypeContext *context) = 0;

    virtual std::any visitInnerJoinType(MySQLParser::InnerJoinTypeContext *context) = 0;

    virtual std::any visitOuterJoinType(MySQLParser::OuterJoinTypeContext *context) = 0;

    virtual std::any visitTableFactor(MySQLParser::TableFactorContext *context) = 0;

    virtual std::any visitSingleTable(MySQLParser::SingleTableContext *context) = 0;

    virtual std::any visitSingleTableParens(MySQLParser::SingleTableParensContext *context) = 0;

    virtual std::any visitDerivedTable(MySQLParser::DerivedTableContext *context) = 0;

    virtual std::any visitTableReferenceListParens(MySQLParser::TableReferenceListParensContext *context) = 0;

    virtual std::any visitTableFunction(MySQLParser::TableFunctionContext *context) = 0;

    virtual std::any visitColumnsClause(MySQLParser::ColumnsClauseContext *context) = 0;

    virtual std::any visitJtColumn(MySQLParser::JtColumnContext *context) = 0;

    virtual std::any visitOnEmptyOrError(MySQLParser::OnEmptyOrErrorContext *context) = 0;

    virtual std::any visitOnEmptyOrErrorJsonTable(MySQLParser::OnEmptyOrErrorJsonTableContext *context) = 0;

    virtual std::any visitOnEmpty(MySQLParser::OnEmptyContext *context) = 0;

    virtual std::any visitOnError(MySQLParser::OnErrorContext *context) = 0;

    virtual std::any visitJsonOnResponse(MySQLParser::JsonOnResponseContext *context) = 0;

    virtual std::any visitUnionOption(MySQLParser::UnionOptionContext *context) = 0;

    virtual std::any visitTableAlias(MySQLParser::TableAliasContext *context) = 0;

    virtual std::any visitIndexHintList(MySQLParser::IndexHintListContext *context) = 0;

    virtual std::any visitIndexHint(MySQLParser::IndexHintContext *context) = 0;

    virtual std::any visitIndexHintType(MySQLParser::IndexHintTypeContext *context) = 0;

    virtual std::any visitKeyOrIndex(MySQLParser::KeyOrIndexContext *context) = 0;

    virtual std::any visitConstraintKeyType(MySQLParser::ConstraintKeyTypeContext *context) = 0;

    virtual std::any visitIndexHintClause(MySQLParser::IndexHintClauseContext *context) = 0;

    virtual std::any visitIndexList(MySQLParser::IndexListContext *context) = 0;

    virtual std::any visitIndexListElement(MySQLParser::IndexListElementContext *context) = 0;

    virtual std::any visitUpdateStatement(MySQLParser::UpdateStatementContext *context) = 0;

    virtual std::any visitTransactionOrLockingStatement(MySQLParser::TransactionOrLockingStatementContext *context) = 0;

    virtual std::any visitTransactionStatement(MySQLParser::TransactionStatementContext *context) = 0;

    virtual std::any visitBeginWork(MySQLParser::BeginWorkContext *context) = 0;

    virtual std::any visitStartTransactionOptionList(MySQLParser::StartTransactionOptionListContext *context) = 0;

    virtual std::any visitSavepointStatement(MySQLParser::SavepointStatementContext *context) = 0;

    virtual std::any visitLockStatement(MySQLParser::LockStatementContext *context) = 0;

    virtual std::any visitLockItem(MySQLParser::LockItemContext *context) = 0;

    virtual std::any visitLockOption(MySQLParser::LockOptionContext *context) = 0;

    virtual std::any visitXaStatement(MySQLParser::XaStatementContext *context) = 0;

    virtual std::any visitXaConvert(MySQLParser::XaConvertContext *context) = 0;

    virtual std::any visitXid(MySQLParser::XidContext *context) = 0;

    virtual std::any visitReplicationStatement(MySQLParser::ReplicationStatementContext *context) = 0;

    virtual std::any visitResetOption(MySQLParser::ResetOptionContext *context) = 0;

    virtual std::any visitSourceResetOptions(MySQLParser::SourceResetOptionsContext *context) = 0;

    virtual std::any visitReplicationLoad(MySQLParser::ReplicationLoadContext *context) = 0;

    virtual std::any visitChangeReplicationSource(MySQLParser::ChangeReplicationSourceContext *context) = 0;

    virtual std::any visitChangeSource(MySQLParser::ChangeSourceContext *context) = 0;

    virtual std::any visitSourceDefinitions(MySQLParser::SourceDefinitionsContext *context) = 0;

    virtual std::any visitSourceDefinition(MySQLParser::SourceDefinitionContext *context) = 0;

    virtual std::any visitChangeReplicationSourceAutoPosition(MySQLParser::ChangeReplicationSourceAutoPositionContext *context) = 0;

    virtual std::any visitChangeReplicationSourceHost(MySQLParser::ChangeReplicationSourceHostContext *context) = 0;

    virtual std::any visitChangeReplicationSourceBind(MySQLParser::ChangeReplicationSourceBindContext *context) = 0;

    virtual std::any visitChangeReplicationSourceUser(MySQLParser::ChangeReplicationSourceUserContext *context) = 0;

    virtual std::any visitChangeReplicationSourcePassword(MySQLParser::ChangeReplicationSourcePasswordContext *context) = 0;

    virtual std::any visitChangeReplicationSourcePort(MySQLParser::ChangeReplicationSourcePortContext *context) = 0;

    virtual std::any visitChangeReplicationSourceConnectRetry(MySQLParser::ChangeReplicationSourceConnectRetryContext *context) = 0;

    virtual std::any visitChangeReplicationSourceRetryCount(MySQLParser::ChangeReplicationSourceRetryCountContext *context) = 0;

    virtual std::any visitChangeReplicationSourceDelay(MySQLParser::ChangeReplicationSourceDelayContext *context) = 0;

    virtual std::any visitChangeReplicationSourceSSL(MySQLParser::ChangeReplicationSourceSSLContext *context) = 0;

    virtual std::any visitChangeReplicationSourceSSLCA(MySQLParser::ChangeReplicationSourceSSLCAContext *context) = 0;

    virtual std::any visitChangeReplicationSourceSSLCApath(MySQLParser::ChangeReplicationSourceSSLCApathContext *context) = 0;

    virtual std::any visitChangeReplicationSourceSSLCipher(MySQLParser::ChangeReplicationSourceSSLCipherContext *context) = 0;

    virtual std::any visitChangeReplicationSourceSSLCLR(MySQLParser::ChangeReplicationSourceSSLCLRContext *context) = 0;

    virtual std::any visitChangeReplicationSourceSSLCLRpath(MySQLParser::ChangeReplicationSourceSSLCLRpathContext *context) = 0;

    virtual std::any visitChangeReplicationSourceSSLKey(MySQLParser::ChangeReplicationSourceSSLKeyContext *context) = 0;

    virtual std::any visitChangeReplicationSourceSSLVerifyServerCert(MySQLParser::ChangeReplicationSourceSSLVerifyServerCertContext *context) = 0;

    virtual std::any visitChangeReplicationSourceTLSVersion(MySQLParser::ChangeReplicationSourceTLSVersionContext *context) = 0;

    virtual std::any visitChangeReplicationSourceTLSCiphersuites(MySQLParser::ChangeReplicationSourceTLSCiphersuitesContext *context) = 0;

    virtual std::any visitChangeReplicationSourceSSLCert(MySQLParser::ChangeReplicationSourceSSLCertContext *context) = 0;

    virtual std::any visitChangeReplicationSourcePublicKey(MySQLParser::ChangeReplicationSourcePublicKeyContext *context) = 0;

    virtual std::any visitChangeReplicationSourceGetSourcePublicKey(MySQLParser::ChangeReplicationSourceGetSourcePublicKeyContext *context) = 0;

    virtual std::any visitChangeReplicationSourceHeartbeatPeriod(MySQLParser::ChangeReplicationSourceHeartbeatPeriodContext *context) = 0;

    virtual std::any visitChangeReplicationSourceCompressionAlgorithm(MySQLParser::ChangeReplicationSourceCompressionAlgorithmContext *context) = 0;

    virtual std::any visitChangeReplicationSourceZstdCompressionLevel(MySQLParser::ChangeReplicationSourceZstdCompressionLevelContext *context) = 0;

    virtual std::any visitPrivilegeCheckDef(MySQLParser::PrivilegeCheckDefContext *context) = 0;

    virtual std::any visitTablePrimaryKeyCheckDef(MySQLParser::TablePrimaryKeyCheckDefContext *context) = 0;

    virtual std::any visitAssignGtidsToAnonymousTransactionsDefinition(MySQLParser::AssignGtidsToAnonymousTransactionsDefinitionContext *context) = 0;

    virtual std::any visitSourceTlsCiphersuitesDef(MySQLParser::SourceTlsCiphersuitesDefContext *context) = 0;

    virtual std::any visitSourceFileDef(MySQLParser::SourceFileDefContext *context) = 0;

    virtual std::any visitSourceLogFile(MySQLParser::SourceLogFileContext *context) = 0;

    virtual std::any visitSourceLogPos(MySQLParser::SourceLogPosContext *context) = 0;

    virtual std::any visitServerIdList(MySQLParser::ServerIdListContext *context) = 0;

    virtual std::any visitChangeReplication(MySQLParser::ChangeReplicationContext *context) = 0;

    virtual std::any visitFilterDefinition(MySQLParser::FilterDefinitionContext *context) = 0;

    virtual std::any visitFilterDbList(MySQLParser::FilterDbListContext *context) = 0;

    virtual std::any visitFilterTableList(MySQLParser::FilterTableListContext *context) = 0;

    virtual std::any visitFilterStringList(MySQLParser::FilterStringListContext *context) = 0;

    virtual std::any visitFilterWildDbTableString(MySQLParser::FilterWildDbTableStringContext *context) = 0;

    virtual std::any visitFilterDbPairList(MySQLParser::FilterDbPairListContext *context) = 0;

    virtual std::any visitStartReplicaStatement(MySQLParser::StartReplicaStatementContext *context) = 0;

    virtual std::any visitStopReplicaStatement(MySQLParser::StopReplicaStatementContext *context) = 0;

    virtual std::any visitReplicaUntil(MySQLParser::ReplicaUntilContext *context) = 0;

    virtual std::any visitUserOption(MySQLParser::UserOptionContext *context) = 0;

    virtual std::any visitPasswordOption(MySQLParser::PasswordOptionContext *context) = 0;

    virtual std::any visitDefaultAuthOption(MySQLParser::DefaultAuthOptionContext *context) = 0;

    virtual std::any visitPluginDirOption(MySQLParser::PluginDirOptionContext *context) = 0;

    virtual std::any visitReplicaThreadOptions(MySQLParser::ReplicaThreadOptionsContext *context) = 0;

    virtual std::any visitReplicaThreadOption(MySQLParser::ReplicaThreadOptionContext *context) = 0;

    virtual std::any visitGroupReplication(MySQLParser::GroupReplicationContext *context) = 0;

    virtual std::any visitGroupReplicationStartOptions(MySQLParser::GroupReplicationStartOptionsContext *context) = 0;

    virtual std::any visitGroupReplicationStartOption(MySQLParser::GroupReplicationStartOptionContext *context) = 0;

    virtual std::any visitGroupReplicationUser(MySQLParser::GroupReplicationUserContext *context) = 0;

    virtual std::any visitGroupReplicationPassword(MySQLParser::GroupReplicationPasswordContext *context) = 0;

    virtual std::any visitGroupReplicationPluginAuth(MySQLParser::GroupReplicationPluginAuthContext *context) = 0;

    virtual std::any visitReplica(MySQLParser::ReplicaContext *context) = 0;

    virtual std::any visitPreparedStatement(MySQLParser::PreparedStatementContext *context) = 0;

    virtual std::any visitExecuteStatement(MySQLParser::ExecuteStatementContext *context) = 0;

    virtual std::any visitExecuteVarList(MySQLParser::ExecuteVarListContext *context) = 0;

    virtual std::any visitCloneStatement(MySQLParser::CloneStatementContext *context) = 0;

    virtual std::any visitDataDirSSL(MySQLParser::DataDirSSLContext *context) = 0;

    virtual std::any visitSsl(MySQLParser::SslContext *context) = 0;

    virtual std::any visitAccountManagementStatement(MySQLParser::AccountManagementStatementContext *context) = 0;

    virtual std::any visitAlterUserStatement(MySQLParser::AlterUserStatementContext *context) = 0;

    virtual std::any visitAlterUserList(MySQLParser::AlterUserListContext *context) = 0;

    virtual std::any visitAlterUser(MySQLParser::AlterUserContext *context) = 0;

    virtual std::any visitOldAlterUser(MySQLParser::OldAlterUserContext *context) = 0;

    virtual std::any visitUserFunction(MySQLParser::UserFunctionContext *context) = 0;

    virtual std::any visitCreateUserStatement(MySQLParser::CreateUserStatementContext *context) = 0;

    virtual std::any visitCreateUserTail(MySQLParser::CreateUserTailContext *context) = 0;

    virtual std::any visitUserAttributes(MySQLParser::UserAttributesContext *context) = 0;

    virtual std::any visitDefaultRoleClause(MySQLParser::DefaultRoleClauseContext *context) = 0;

    virtual std::any visitRequireClause(MySQLParser::RequireClauseContext *context) = 0;

    virtual std::any visitConnectOptions(MySQLParser::ConnectOptionsContext *context) = 0;

    virtual std::any visitAccountLockPasswordExpireOptions(MySQLParser::AccountLockPasswordExpireOptionsContext *context) = 0;

    virtual std::any visitDropUserStatement(MySQLParser::DropUserStatementContext *context) = 0;

    virtual std::any visitGrantStatement(MySQLParser::GrantStatementContext *context) = 0;

    virtual std::any visitGrantTargetList(MySQLParser::GrantTargetListContext *context) = 0;

    virtual std::any visitGrantOptions(MySQLParser::GrantOptionsContext *context) = 0;

    virtual std::any visitExceptRoleList(MySQLParser::ExceptRoleListContext *context) = 0;

    virtual std::any visitWithRoles(MySQLParser::WithRolesContext *context) = 0;

    virtual std::any visitGrantAs(MySQLParser::GrantAsContext *context) = 0;

    virtual std::any visitVersionedRequireClause(MySQLParser::VersionedRequireClauseContext *context) = 0;

    virtual std::any visitRenameUserStatement(MySQLParser::RenameUserStatementContext *context) = 0;

    virtual std::any visitRevokeStatement(MySQLParser::RevokeStatementContext *context) = 0;

    virtual std::any visitOnTypeTo(MySQLParser::OnTypeToContext *context) = 0;

    virtual std::any visitAclType(MySQLParser::AclTypeContext *context) = 0;

    virtual std::any visitRoleOrPrivilegesList(MySQLParser::RoleOrPrivilegesListContext *context) = 0;

    virtual std::any visitRoleOrPrivilege(MySQLParser::RoleOrPrivilegeContext *context) = 0;

    virtual std::any visitGrantIdentifier(MySQLParser::GrantIdentifierContext *context) = 0;

    virtual std::any visitRequireList(MySQLParser::RequireListContext *context) = 0;

    virtual std::any visitRequireListElement(MySQLParser::RequireListElementContext *context) = 0;

    virtual std::any visitGrantOption(MySQLParser::GrantOptionContext *context) = 0;

    virtual std::any visitSetRoleStatement(MySQLParser::SetRoleStatementContext *context) = 0;

    virtual std::any visitRoleList(MySQLParser::RoleListContext *context) = 0;

    virtual std::any visitRole(MySQLParser::RoleContext *context) = 0;

    virtual std::any visitTableAdministrationStatement(MySQLParser::TableAdministrationStatementContext *context) = 0;

    virtual std::any visitHistogram(MySQLParser::HistogramContext *context) = 0;

    virtual std::any visitCheckOption(MySQLParser::CheckOptionContext *context) = 0;

    virtual std::any visitRepairType(MySQLParser::RepairTypeContext *context) = 0;

    virtual std::any visitInstallUninstallStatement(MySQLParser::InstallUninstallStatementContext *context) = 0;

    virtual std::any visitSetStatement(MySQLParser::SetStatementContext *context) = 0;

    virtual std::any visitStartOptionValueList(MySQLParser::StartOptionValueListContext *context) = 0;

    virtual std::any visitTransactionCharacteristics(MySQLParser::TransactionCharacteristicsContext *context) = 0;

    virtual std::any visitTransactionAccessMode(MySQLParser::TransactionAccessModeContext *context) = 0;

    virtual std::any visitIsolationLevel(MySQLParser::IsolationLevelContext *context) = 0;

    virtual std::any visitOptionValueListContinued(MySQLParser::OptionValueListContinuedContext *context) = 0;

    virtual std::any visitOptionValueNoOptionType(MySQLParser::OptionValueNoOptionTypeContext *context) = 0;

    virtual std::any visitOptionValue(MySQLParser::OptionValueContext *context) = 0;

    virtual std::any visitStartOptionValueListFollowingOptionType(MySQLParser::StartOptionValueListFollowingOptionTypeContext *context) = 0;

    virtual std::any visitOptionValueFollowingOptionType(MySQLParser::OptionValueFollowingOptionTypeContext *context) = 0;

    virtual std::any visitSetExprOrDefault(MySQLParser::SetExprOrDefaultContext *context) = 0;

    virtual std::any visitShowDatabasesStatement(MySQLParser::ShowDatabasesStatementContext *context) = 0;

    virtual std::any visitShowTablesStatement(MySQLParser::ShowTablesStatementContext *context) = 0;

    virtual std::any visitShowTriggersStatement(MySQLParser::ShowTriggersStatementContext *context) = 0;

    virtual std::any visitShowEventsStatement(MySQLParser::ShowEventsStatementContext *context) = 0;

    virtual std::any visitShowTableStatusStatement(MySQLParser::ShowTableStatusStatementContext *context) = 0;

    virtual std::any visitShowOpenTablesStatement(MySQLParser::ShowOpenTablesStatementContext *context) = 0;

    virtual std::any visitShowPluginsStatement(MySQLParser::ShowPluginsStatementContext *context) = 0;

    virtual std::any visitShowEngineLogsStatement(MySQLParser::ShowEngineLogsStatementContext *context) = 0;

    virtual std::any visitShowEngineMutexStatement(MySQLParser::ShowEngineMutexStatementContext *context) = 0;

    virtual std::any visitShowEngineStatusStatement(MySQLParser::ShowEngineStatusStatementContext *context) = 0;

    virtual std::any visitShowColumnsStatement(MySQLParser::ShowColumnsStatementContext *context) = 0;

    virtual std::any visitShowBinaryLogsStatement(MySQLParser::ShowBinaryLogsStatementContext *context) = 0;

    virtual std::any visitShowReplicasStatement(MySQLParser::ShowReplicasStatementContext *context) = 0;

    virtual std::any visitShowBinlogEventsStatement(MySQLParser::ShowBinlogEventsStatementContext *context) = 0;

    virtual std::any visitShowRelaylogEventsStatement(MySQLParser::ShowRelaylogEventsStatementContext *context) = 0;

    virtual std::any visitShowKeysStatement(MySQLParser::ShowKeysStatementContext *context) = 0;

    virtual std::any visitShowEnginesStatement(MySQLParser::ShowEnginesStatementContext *context) = 0;

    virtual std::any visitShowCountWarningsStatement(MySQLParser::ShowCountWarningsStatementContext *context) = 0;

    virtual std::any visitShowCountErrorsStatement(MySQLParser::ShowCountErrorsStatementContext *context) = 0;

    virtual std::any visitShowWarningsStatement(MySQLParser::ShowWarningsStatementContext *context) = 0;

    virtual std::any visitShowErrorsStatement(MySQLParser::ShowErrorsStatementContext *context) = 0;

    virtual std::any visitShowProfilesStatement(MySQLParser::ShowProfilesStatementContext *context) = 0;

    virtual std::any visitShowProfileStatement(MySQLParser::ShowProfileStatementContext *context) = 0;

    virtual std::any visitShowStatusStatement(MySQLParser::ShowStatusStatementContext *context) = 0;

    virtual std::any visitShowProcessListStatement(MySQLParser::ShowProcessListStatementContext *context) = 0;

    virtual std::any visitShowVariablesStatement(MySQLParser::ShowVariablesStatementContext *context) = 0;

    virtual std::any visitShowCharacterSetStatement(MySQLParser::ShowCharacterSetStatementContext *context) = 0;

    virtual std::any visitShowCollationStatement(MySQLParser::ShowCollationStatementContext *context) = 0;

    virtual std::any visitShowPrivilegesStatement(MySQLParser::ShowPrivilegesStatementContext *context) = 0;

    virtual std::any visitShowGrantsStatement(MySQLParser::ShowGrantsStatementContext *context) = 0;

    virtual std::any visitShowCreateDatabaseStatement(MySQLParser::ShowCreateDatabaseStatementContext *context) = 0;

    virtual std::any visitShowCreateTableStatement(MySQLParser::ShowCreateTableStatementContext *context) = 0;

    virtual std::any visitShowCreateViewStatement(MySQLParser::ShowCreateViewStatementContext *context) = 0;

    virtual std::any visitShowMasterStatusStatement(MySQLParser::ShowMasterStatusStatementContext *context) = 0;

    virtual std::any visitShowReplicaStatusStatement(MySQLParser::ShowReplicaStatusStatementContext *context) = 0;

    virtual std::any visitShowCreateProcedureStatement(MySQLParser::ShowCreateProcedureStatementContext *context) = 0;

    virtual std::any visitShowCreateFunctionStatement(MySQLParser::ShowCreateFunctionStatementContext *context) = 0;

    virtual std::any visitShowCreateTriggerStatement(MySQLParser::ShowCreateTriggerStatementContext *context) = 0;

    virtual std::any visitShowCreateProcedureStatusStatement(MySQLParser::ShowCreateProcedureStatusStatementContext *context) = 0;

    virtual std::any visitShowCreateFunctionStatusStatement(MySQLParser::ShowCreateFunctionStatusStatementContext *context) = 0;

    virtual std::any visitShowCreateProcedureCodeStatement(MySQLParser::ShowCreateProcedureCodeStatementContext *context) = 0;

    virtual std::any visitShowCreateFunctionCodeStatement(MySQLParser::ShowCreateFunctionCodeStatementContext *context) = 0;

    virtual std::any visitShowCreateEventStatement(MySQLParser::ShowCreateEventStatementContext *context) = 0;

    virtual std::any visitShowCreateUserStatement(MySQLParser::ShowCreateUserStatementContext *context) = 0;

    virtual std::any visitShowCommandType(MySQLParser::ShowCommandTypeContext *context) = 0;

    virtual std::any visitEngineOrAll(MySQLParser::EngineOrAllContext *context) = 0;

    virtual std::any visitFromOrIn(MySQLParser::FromOrInContext *context) = 0;

    virtual std::any visitInDb(MySQLParser::InDbContext *context) = 0;

    virtual std::any visitProfileDefinitions(MySQLParser::ProfileDefinitionsContext *context) = 0;

    virtual std::any visitProfileDefinition(MySQLParser::ProfileDefinitionContext *context) = 0;

    virtual std::any visitOtherAdministrativeStatement(MySQLParser::OtherAdministrativeStatementContext *context) = 0;

    virtual std::any visitKeyCacheListOrParts(MySQLParser::KeyCacheListOrPartsContext *context) = 0;

    virtual std::any visitKeyCacheList(MySQLParser::KeyCacheListContext *context) = 0;

    virtual std::any visitAssignToKeycache(MySQLParser::AssignToKeycacheContext *context) = 0;

    virtual std::any visitAssignToKeycachePartition(MySQLParser::AssignToKeycachePartitionContext *context) = 0;

    virtual std::any visitCacheKeyList(MySQLParser::CacheKeyListContext *context) = 0;

    virtual std::any visitKeyUsageElement(MySQLParser::KeyUsageElementContext *context) = 0;

    virtual std::any visitKeyUsageList(MySQLParser::KeyUsageListContext *context) = 0;

    virtual std::any visitFlushOption(MySQLParser::FlushOptionContext *context) = 0;

    virtual std::any visitLogType(MySQLParser::LogTypeContext *context) = 0;

    virtual std::any visitFlushTables(MySQLParser::FlushTablesContext *context) = 0;

    virtual std::any visitFlushTablesOptions(MySQLParser::FlushTablesOptionsContext *context) = 0;

    virtual std::any visitPreloadTail(MySQLParser::PreloadTailContext *context) = 0;

    virtual std::any visitPreloadList(MySQLParser::PreloadListContext *context) = 0;

    virtual std::any visitPreloadKeys(MySQLParser::PreloadKeysContext *context) = 0;

    virtual std::any visitAdminPartition(MySQLParser::AdminPartitionContext *context) = 0;

    virtual std::any visitResourceGroupManagement(MySQLParser::ResourceGroupManagementContext *context) = 0;

    virtual std::any visitCreateResourceGroup(MySQLParser::CreateResourceGroupContext *context) = 0;

    virtual std::any visitResourceGroupVcpuList(MySQLParser::ResourceGroupVcpuListContext *context) = 0;

    virtual std::any visitVcpuNumOrRange(MySQLParser::VcpuNumOrRangeContext *context) = 0;

    virtual std::any visitResourceGroupPriority(MySQLParser::ResourceGroupPriorityContext *context) = 0;

    virtual std::any visitResourceGroupEnableDisable(MySQLParser::ResourceGroupEnableDisableContext *context) = 0;

    virtual std::any visitAlterResourceGroup(MySQLParser::AlterResourceGroupContext *context) = 0;

    virtual std::any visitSetResourceGroup(MySQLParser::SetResourceGroupContext *context) = 0;

    virtual std::any visitThreadIdList(MySQLParser::ThreadIdListContext *context) = 0;

    virtual std::any visitDropResourceGroup(MySQLParser::DropResourceGroupContext *context) = 0;

    virtual std::any visitUtilityStatement(MySQLParser::UtilityStatementContext *context) = 0;

    virtual std::any visitDescribeStatement(MySQLParser::DescribeStatementContext *context) = 0;

    virtual std::any visitExplainStatement(MySQLParser::ExplainStatementContext *context) = 0;

    virtual std::any visitExplainableStatement(MySQLParser::ExplainableStatementContext *context) = 0;

    virtual std::any visitHelpCommand(MySQLParser::HelpCommandContext *context) = 0;

    virtual std::any visitUseCommand(MySQLParser::UseCommandContext *context) = 0;

    virtual std::any visitRestartServer(MySQLParser::RestartServerContext *context) = 0;

    virtual std::any visitExprOr(MySQLParser::ExprOrContext *context) = 0;

    virtual std::any visitExprNot(MySQLParser::ExprNotContext *context) = 0;

    virtual std::any visitExprIs(MySQLParser::ExprIsContext *context) = 0;

    virtual std::any visitExprAnd(MySQLParser::ExprAndContext *context) = 0;

    virtual std::any visitExprXor(MySQLParser::ExprXorContext *context) = 0;

    virtual std::any visitPrimaryExprPredicate(MySQLParser::PrimaryExprPredicateContext *context) = 0;

    virtual std::any visitPrimaryExprCompare(MySQLParser::PrimaryExprCompareContext *context) = 0;

    virtual std::any visitPrimaryExprAllAny(MySQLParser::PrimaryExprAllAnyContext *context) = 0;

    virtual std::any visitPrimaryExprIsNull(MySQLParser::PrimaryExprIsNullContext *context) = 0;

    virtual std::any visitCompOp(MySQLParser::CompOpContext *context) = 0;

    virtual std::any visitPredicate(MySQLParser::PredicateContext *context) = 0;

    virtual std::any visitPredicateExprIn(MySQLParser::PredicateExprInContext *context) = 0;

    virtual std::any visitPredicateExprBetween(MySQLParser::PredicateExprBetweenContext *context) = 0;

    virtual std::any visitPredicateExprLike(MySQLParser::PredicateExprLikeContext *context) = 0;

    virtual std::any visitPredicateExprRegex(MySQLParser::PredicateExprRegexContext *context) = 0;

    virtual std::any visitBitExpr(MySQLParser::BitExprContext *context) = 0;

    virtual std::any visitSimpleExprConvert(MySQLParser::SimpleExprConvertContext *context) = 0;

    virtual std::any visitSimpleExprCast(MySQLParser::SimpleExprCastContext *context) = 0;

    virtual std::any visitSimpleExprUnary(MySQLParser::SimpleExprUnaryContext *context) = 0;

    virtual std::any visitSimpleExpressionRValue(MySQLParser::SimpleExpressionRValueContext *context) = 0;

    virtual std::any visitSimpleExprOdbc(MySQLParser::SimpleExprOdbcContext *context) = 0;

    virtual std::any visitSimpleExprRuntimeFunction(MySQLParser::SimpleExprRuntimeFunctionContext *context) = 0;

    virtual std::any visitSimpleExprFunction(MySQLParser::SimpleExprFunctionContext *context) = 0;

    virtual std::any visitSimpleExprCollate(MySQLParser::SimpleExprCollateContext *context) = 0;

    virtual std::any visitSimpleExprMatch(MySQLParser::SimpleExprMatchContext *context) = 0;

    virtual std::any visitSimpleExprWindowingFunction(MySQLParser::SimpleExprWindowingFunctionContext *context) = 0;

    virtual std::any visitSimpleExprBinary(MySQLParser::SimpleExprBinaryContext *context) = 0;

    virtual std::any visitSimpleExprColumnRef(MySQLParser::SimpleExprColumnRefContext *context) = 0;

    virtual std::any visitSimpleExprParamMarker(MySQLParser::SimpleExprParamMarkerContext *context) = 0;

    virtual std::any visitSimpleExprSum(MySQLParser::SimpleExprSumContext *context) = 0;

    virtual std::any visitSimpleExprCastTime(MySQLParser::SimpleExprCastTimeContext *context) = 0;

    virtual std::any visitSimpleExprConvertUsing(MySQLParser::SimpleExprConvertUsingContext *context) = 0;

    virtual std::any visitSimpleExprSubQuery(MySQLParser::SimpleExprSubQueryContext *context) = 0;

    virtual std::any visitSimpleExprGroupingOperation(MySQLParser::SimpleExprGroupingOperationContext *context) = 0;

    virtual std::any visitSimpleExprNot(MySQLParser::SimpleExprNotContext *context) = 0;

    virtual std::any visitSimpleExprValues(MySQLParser::SimpleExprValuesContext *context) = 0;

    virtual std::any visitSimpleExprUserVariableAssignment(MySQLParser::SimpleExprUserVariableAssignmentContext *context) = 0;

    virtual std::any visitSimpleExprDefault(MySQLParser::SimpleExprDefaultContext *context) = 0;

    virtual std::any visitSimpleExprList(MySQLParser::SimpleExprListContext *context) = 0;

    virtual std::any visitSimpleExprInterval(MySQLParser::SimpleExprIntervalContext *context) = 0;

    virtual std::any visitSimpleExprCase(MySQLParser::SimpleExprCaseContext *context) = 0;

    virtual std::any visitSimpleExprConcat(MySQLParser::SimpleExprConcatContext *context) = 0;

    virtual std::any visitSimpleExprLiteral(MySQLParser::SimpleExprLiteralContext *context) = 0;

    virtual std::any visitArrayCast(MySQLParser::ArrayCastContext *context) = 0;

    virtual std::any visitJsonOperator(MySQLParser::JsonOperatorContext *context) = 0;

    virtual std::any visitSumExpr(MySQLParser::SumExprContext *context) = 0;

    virtual std::any visitGroupingOperation(MySQLParser::GroupingOperationContext *context) = 0;

    virtual std::any visitWindowFunctionCall(MySQLParser::WindowFunctionCallContext *context) = 0;

    virtual std::any visitWindowingClause(MySQLParser::WindowingClauseContext *context) = 0;

    virtual std::any visitLeadLagInfo(MySQLParser::LeadLagInfoContext *context) = 0;

    virtual std::any visitStableInteger(MySQLParser::StableIntegerContext *context) = 0;

    virtual std::any visitParamOrVar(MySQLParser::ParamOrVarContext *context) = 0;

    virtual std::any visitNullTreatment(MySQLParser::NullTreatmentContext *context) = 0;

    virtual std::any visitJsonFunction(MySQLParser::JsonFunctionContext *context) = 0;

    virtual std::any visitInSumExpr(MySQLParser::InSumExprContext *context) = 0;

    virtual std::any visitIdentListArg(MySQLParser::IdentListArgContext *context) = 0;

    virtual std::any visitIdentList(MySQLParser::IdentListContext *context) = 0;

    virtual std::any visitFulltextOptions(MySQLParser::FulltextOptionsContext *context) = 0;

    virtual std::any visitRuntimeFunctionCall(MySQLParser::RuntimeFunctionCallContext *context) = 0;

    virtual std::any visitReturningType(MySQLParser::ReturningTypeContext *context) = 0;

    virtual std::any visitGeometryFunction(MySQLParser::GeometryFunctionContext *context) = 0;

    virtual std::any visitTimeFunctionParameters(MySQLParser::TimeFunctionParametersContext *context) = 0;

    virtual std::any visitFractionalPrecision(MySQLParser::FractionalPrecisionContext *context) = 0;

    virtual std::any visitWeightStringLevels(MySQLParser::WeightStringLevelsContext *context) = 0;

    virtual std::any visitWeightStringLevelListItem(MySQLParser::WeightStringLevelListItemContext *context) = 0;

    virtual std::any visitDateTimeTtype(MySQLParser::DateTimeTtypeContext *context) = 0;

    virtual std::any visitTrimFunction(MySQLParser::TrimFunctionContext *context) = 0;

    virtual std::any visitSubstringFunction(MySQLParser::SubstringFunctionContext *context) = 0;

    virtual std::any visitFunctionCall(MySQLParser::FunctionCallContext *context) = 0;

    virtual std::any visitUdfExprList(MySQLParser::UdfExprListContext *context) = 0;

    virtual std::any visitUdfExpr(MySQLParser::UdfExprContext *context) = 0;

    virtual std::any visitUserVariable(MySQLParser::UserVariableContext *context) = 0;

    virtual std::any visitUserVariableIdentifier(MySQLParser::UserVariableIdentifierContext *context) = 0;

    virtual std::any visitInExpressionUserVariableAssignment(MySQLParser::InExpressionUserVariableAssignmentContext *context) = 0;

    virtual std::any visitRvalueSystemOrUserVariable(MySQLParser::RvalueSystemOrUserVariableContext *context) = 0;

    virtual std::any visitLvalueVariable(MySQLParser::LvalueVariableContext *context) = 0;

    virtual std::any visitRvalueSystemVariable(MySQLParser::RvalueSystemVariableContext *context) = 0;

    virtual std::any visitWhenExpression(MySQLParser::WhenExpressionContext *context) = 0;

    virtual std::any visitThenExpression(MySQLParser::ThenExpressionContext *context) = 0;

    virtual std::any visitElseExpression(MySQLParser::ElseExpressionContext *context) = 0;

    virtual std::any visitCastType(MySQLParser::CastTypeContext *context) = 0;

    virtual std::any visitExprList(MySQLParser::ExprListContext *context) = 0;

    virtual std::any visitCharset(MySQLParser::CharsetContext *context) = 0;

    virtual std::any visitNotRule(MySQLParser::NotRuleContext *context) = 0;

    virtual std::any visitNot2Rule(MySQLParser::Not2RuleContext *context) = 0;

    virtual std::any visitInterval(MySQLParser::IntervalContext *context) = 0;

    virtual std::any visitIntervalTimeStamp(MySQLParser::IntervalTimeStampContext *context) = 0;

    virtual std::any visitExprListWithParentheses(MySQLParser::ExprListWithParenthesesContext *context) = 0;

    virtual std::any visitExprWithParentheses(MySQLParser::ExprWithParenthesesContext *context) = 0;

    virtual std::any visitSimpleExprWithParentheses(MySQLParser::SimpleExprWithParenthesesContext *context) = 0;

    virtual std::any visitOrderList(MySQLParser::OrderListContext *context) = 0;

    virtual std::any visitOrderExpression(MySQLParser::OrderExpressionContext *context) = 0;

    virtual std::any visitGroupList(MySQLParser::GroupListContext *context) = 0;

    virtual std::any visitGroupingExpression(MySQLParser::GroupingExpressionContext *context) = 0;

    virtual std::any visitChannel(MySQLParser::ChannelContext *context) = 0;

    virtual std::any visitCompoundStatement(MySQLParser::CompoundStatementContext *context) = 0;

    virtual std::any visitReturnStatement(MySQLParser::ReturnStatementContext *context) = 0;

    virtual std::any visitIfStatement(MySQLParser::IfStatementContext *context) = 0;

    virtual std::any visitIfBody(MySQLParser::IfBodyContext *context) = 0;

    virtual std::any visitThenStatement(MySQLParser::ThenStatementContext *context) = 0;

    virtual std::any visitCompoundStatementList(MySQLParser::CompoundStatementListContext *context) = 0;

    virtual std::any visitCaseStatement(MySQLParser::CaseStatementContext *context) = 0;

    virtual std::any visitElseStatement(MySQLParser::ElseStatementContext *context) = 0;

    virtual std::any visitLabeledBlock(MySQLParser::LabeledBlockContext *context) = 0;

    virtual std::any visitUnlabeledBlock(MySQLParser::UnlabeledBlockContext *context) = 0;

    virtual std::any visitLabel(MySQLParser::LabelContext *context) = 0;

    virtual std::any visitBeginEndBlock(MySQLParser::BeginEndBlockContext *context) = 0;

    virtual std::any visitLabeledControl(MySQLParser::LabeledControlContext *context) = 0;

    virtual std::any visitUnlabeledControl(MySQLParser::UnlabeledControlContext *context) = 0;

    virtual std::any visitLoopBlock(MySQLParser::LoopBlockContext *context) = 0;

    virtual std::any visitWhileDoBlock(MySQLParser::WhileDoBlockContext *context) = 0;

    virtual std::any visitRepeatUntilBlock(MySQLParser::RepeatUntilBlockContext *context) = 0;

    virtual std::any visitSpDeclarations(MySQLParser::SpDeclarationsContext *context) = 0;

    virtual std::any visitSpDeclaration(MySQLParser::SpDeclarationContext *context) = 0;

    virtual std::any visitVariableDeclaration(MySQLParser::VariableDeclarationContext *context) = 0;

    virtual std::any visitConditionDeclaration(MySQLParser::ConditionDeclarationContext *context) = 0;

    virtual std::any visitSpCondition(MySQLParser::SpConditionContext *context) = 0;

    virtual std::any visitSqlstate(MySQLParser::SqlstateContext *context) = 0;

    virtual std::any visitHandlerDeclaration(MySQLParser::HandlerDeclarationContext *context) = 0;

    virtual std::any visitHandlerCondition(MySQLParser::HandlerConditionContext *context) = 0;

    virtual std::any visitCursorDeclaration(MySQLParser::CursorDeclarationContext *context) = 0;

    virtual std::any visitIterateStatement(MySQLParser::IterateStatementContext *context) = 0;

    virtual std::any visitLeaveStatement(MySQLParser::LeaveStatementContext *context) = 0;

    virtual std::any visitGetDiagnosticsStatement(MySQLParser::GetDiagnosticsStatementContext *context) = 0;

    virtual std::any visitSignalAllowedExpr(MySQLParser::SignalAllowedExprContext *context) = 0;

    virtual std::any visitStatementInformationItem(MySQLParser::StatementInformationItemContext *context) = 0;

    virtual std::any visitConditionInformationItem(MySQLParser::ConditionInformationItemContext *context) = 0;

    virtual std::any visitSignalInformationItemName(MySQLParser::SignalInformationItemNameContext *context) = 0;

    virtual std::any visitSignalStatement(MySQLParser::SignalStatementContext *context) = 0;

    virtual std::any visitResignalStatement(MySQLParser::ResignalStatementContext *context) = 0;

    virtual std::any visitSignalInformationItem(MySQLParser::SignalInformationItemContext *context) = 0;

    virtual std::any visitCursorOpen(MySQLParser::CursorOpenContext *context) = 0;

    virtual std::any visitCursorClose(MySQLParser::CursorCloseContext *context) = 0;

    virtual std::any visitCursorFetch(MySQLParser::CursorFetchContext *context) = 0;

    virtual std::any visitSchedule(MySQLParser::ScheduleContext *context) = 0;

    virtual std::any visitColumnDefinition(MySQLParser::ColumnDefinitionContext *context) = 0;

    virtual std::any visitCheckOrReferences(MySQLParser::CheckOrReferencesContext *context) = 0;

    virtual std::any visitCheckConstraint(MySQLParser::CheckConstraintContext *context) = 0;

    virtual std::any visitConstraintEnforcement(MySQLParser::ConstraintEnforcementContext *context) = 0;

    virtual std::any visitTableConstraintDef(MySQLParser::TableConstraintDefContext *context) = 0;

    virtual std::any visitConstraintName(MySQLParser::ConstraintNameContext *context) = 0;

    virtual std::any visitFieldDefinition(MySQLParser::FieldDefinitionContext *context) = 0;

    virtual std::any visitColumnAttribute(MySQLParser::ColumnAttributeContext *context) = 0;

    virtual std::any visitColumnFormat(MySQLParser::ColumnFormatContext *context) = 0;

    virtual std::any visitStorageMedia(MySQLParser::StorageMediaContext *context) = 0;

    virtual std::any visitNow(MySQLParser::NowContext *context) = 0;

    virtual std::any visitNowOrSignedLiteral(MySQLParser::NowOrSignedLiteralContext *context) = 0;

    virtual std::any visitGcolAttribute(MySQLParser::GcolAttributeContext *context) = 0;

    virtual std::any visitReferences(MySQLParser::ReferencesContext *context) = 0;

    virtual std::any visitDeleteOption(MySQLParser::DeleteOptionContext *context) = 0;

    virtual std::any visitKeyList(MySQLParser::KeyListContext *context) = 0;

    virtual std::any visitKeyPart(MySQLParser::KeyPartContext *context) = 0;

    virtual std::any visitKeyListWithExpression(MySQLParser::KeyListWithExpressionContext *context) = 0;

    virtual std::any visitKeyPartOrExpression(MySQLParser::KeyPartOrExpressionContext *context) = 0;

    virtual std::any visitKeyListVariants(MySQLParser::KeyListVariantsContext *context) = 0;

    virtual std::any visitIndexType(MySQLParser::IndexTypeContext *context) = 0;

    virtual std::any visitIndexOption(MySQLParser::IndexOptionContext *context) = 0;

    virtual std::any visitCommonIndexOption(MySQLParser::CommonIndexOptionContext *context) = 0;

    virtual std::any visitVisibility(MySQLParser::VisibilityContext *context) = 0;

    virtual std::any visitIndexTypeClause(MySQLParser::IndexTypeClauseContext *context) = 0;

    virtual std::any visitFulltextIndexOption(MySQLParser::FulltextIndexOptionContext *context) = 0;

    virtual std::any visitSpatialIndexOption(MySQLParser::SpatialIndexOptionContext *context) = 0;

    virtual std::any visitDataTypeDefinition(MySQLParser::DataTypeDefinitionContext *context) = 0;

    virtual std::any visitDataType(MySQLParser::DataTypeContext *context) = 0;

    virtual std::any visitNchar(MySQLParser::NcharContext *context) = 0;

    virtual std::any visitRealType(MySQLParser::RealTypeContext *context) = 0;

    virtual std::any visitFieldLength(MySQLParser::FieldLengthContext *context) = 0;

    virtual std::any visitFieldOptions(MySQLParser::FieldOptionsContext *context) = 0;

    virtual std::any visitCharsetWithOptBinary(MySQLParser::CharsetWithOptBinaryContext *context) = 0;

    virtual std::any visitAscii(MySQLParser::AsciiContext *context) = 0;

    virtual std::any visitUnicode(MySQLParser::UnicodeContext *context) = 0;

    virtual std::any visitWsNumCodepoints(MySQLParser::WsNumCodepointsContext *context) = 0;

    virtual std::any visitTypeDatetimePrecision(MySQLParser::TypeDatetimePrecisionContext *context) = 0;

    virtual std::any visitFunctionDatetimePrecision(MySQLParser::FunctionDatetimePrecisionContext *context) = 0;

    virtual std::any visitCharsetName(MySQLParser::CharsetNameContext *context) = 0;

    virtual std::any visitCollationName(MySQLParser::CollationNameContext *context) = 0;

    virtual std::any visitCreateTableOptions(MySQLParser::CreateTableOptionsContext *context) = 0;

    virtual std::any visitCreateTableOptionsSpaceSeparated(MySQLParser::CreateTableOptionsSpaceSeparatedContext *context) = 0;

    virtual std::any visitCreateTableOption(MySQLParser::CreateTableOptionContext *context) = 0;

    virtual std::any visitTernaryOption(MySQLParser::TernaryOptionContext *context) = 0;

    virtual std::any visitDefaultCollation(MySQLParser::DefaultCollationContext *context) = 0;

    virtual std::any visitDefaultEncryption(MySQLParser::DefaultEncryptionContext *context) = 0;

    virtual std::any visitDefaultCharset(MySQLParser::DefaultCharsetContext *context) = 0;

    virtual std::any visitPartitionClause(MySQLParser::PartitionClauseContext *context) = 0;

    virtual std::any visitPartitionDefKey(MySQLParser::PartitionDefKeyContext *context) = 0;

    virtual std::any visitPartitionDefHash(MySQLParser::PartitionDefHashContext *context) = 0;

    virtual std::any visitPartitionDefRangeList(MySQLParser::PartitionDefRangeListContext *context) = 0;

    virtual std::any visitSubPartitions(MySQLParser::SubPartitionsContext *context) = 0;

    virtual std::any visitPartitionKeyAlgorithm(MySQLParser::PartitionKeyAlgorithmContext *context) = 0;

    virtual std::any visitPartitionDefinitions(MySQLParser::PartitionDefinitionsContext *context) = 0;

    virtual std::any visitPartitionDefinition(MySQLParser::PartitionDefinitionContext *context) = 0;

    virtual std::any visitPartitionValuesIn(MySQLParser::PartitionValuesInContext *context) = 0;

    virtual std::any visitPartitionOption(MySQLParser::PartitionOptionContext *context) = 0;

    virtual std::any visitSubpartitionDefinition(MySQLParser::SubpartitionDefinitionContext *context) = 0;

    virtual std::any visitPartitionValueItemListParen(MySQLParser::PartitionValueItemListParenContext *context) = 0;

    virtual std::any visitPartitionValueItem(MySQLParser::PartitionValueItemContext *context) = 0;

    virtual std::any visitDefinerClause(MySQLParser::DefinerClauseContext *context) = 0;

    virtual std::any visitIfExists(MySQLParser::IfExistsContext *context) = 0;

    virtual std::any visitIfNotExists(MySQLParser::IfNotExistsContext *context) = 0;

    virtual std::any visitProcedureParameter(MySQLParser::ProcedureParameterContext *context) = 0;

    virtual std::any visitFunctionParameter(MySQLParser::FunctionParameterContext *context) = 0;

    virtual std::any visitCollate(MySQLParser::CollateContext *context) = 0;

    virtual std::any visitTypeWithOptCollate(MySQLParser::TypeWithOptCollateContext *context) = 0;

    virtual std::any visitSchemaIdentifierPair(MySQLParser::SchemaIdentifierPairContext *context) = 0;

    virtual std::any visitViewRefList(MySQLParser::ViewRefListContext *context) = 0;

    virtual std::any visitUpdateList(MySQLParser::UpdateListContext *context) = 0;

    virtual std::any visitUpdateElement(MySQLParser::UpdateElementContext *context) = 0;

    virtual std::any visitCharsetClause(MySQLParser::CharsetClauseContext *context) = 0;

    virtual std::any visitFieldsClause(MySQLParser::FieldsClauseContext *context) = 0;

    virtual std::any visitFieldTerm(MySQLParser::FieldTermContext *context) = 0;

    virtual std::any visitLinesClause(MySQLParser::LinesClauseContext *context) = 0;

    virtual std::any visitLineTerm(MySQLParser::LineTermContext *context) = 0;

    virtual std::any visitUserList(MySQLParser::UserListContext *context) = 0;

    virtual std::any visitCreateUserList(MySQLParser::CreateUserListContext *context) = 0;

    virtual std::any visitCreateUser(MySQLParser::CreateUserContext *context) = 0;

    virtual std::any visitCreateUserWithMfa(MySQLParser::CreateUserWithMfaContext *context) = 0;

    virtual std::any visitIdentification(MySQLParser::IdentificationContext *context) = 0;

    virtual std::any visitIdentifiedByPassword(MySQLParser::IdentifiedByPasswordContext *context) = 0;

    virtual std::any visitIdentifiedByRandomPassword(MySQLParser::IdentifiedByRandomPasswordContext *context) = 0;

    virtual std::any visitIdentifiedWithPlugin(MySQLParser::IdentifiedWithPluginContext *context) = 0;

    virtual std::any visitIdentifiedWithPluginAsAuth(MySQLParser::IdentifiedWithPluginAsAuthContext *context) = 0;

    virtual std::any visitIdentifiedWithPluginByPassword(MySQLParser::IdentifiedWithPluginByPasswordContext *context) = 0;

    virtual std::any visitIdentifiedWithPluginByRandomPassword(MySQLParser::IdentifiedWithPluginByRandomPasswordContext *context) = 0;

    virtual std::any visitInitialAuth(MySQLParser::InitialAuthContext *context) = 0;

    virtual std::any visitRetainCurrentPassword(MySQLParser::RetainCurrentPasswordContext *context) = 0;

    virtual std::any visitDiscardOldPassword(MySQLParser::DiscardOldPasswordContext *context) = 0;

    virtual std::any visitUserRegistration(MySQLParser::UserRegistrationContext *context) = 0;

    virtual std::any visitFactor(MySQLParser::FactorContext *context) = 0;

    virtual std::any visitReplacePassword(MySQLParser::ReplacePasswordContext *context) = 0;

    virtual std::any visitUserIdentifierOrText(MySQLParser::UserIdentifierOrTextContext *context) = 0;

    virtual std::any visitUser(MySQLParser::UserContext *context) = 0;

    virtual std::any visitLikeClause(MySQLParser::LikeClauseContext *context) = 0;

    virtual std::any visitLikeOrWhere(MySQLParser::LikeOrWhereContext *context) = 0;

    virtual std::any visitOnlineOption(MySQLParser::OnlineOptionContext *context) = 0;

    virtual std::any visitNoWriteToBinLog(MySQLParser::NoWriteToBinLogContext *context) = 0;

    virtual std::any visitUsePartition(MySQLParser::UsePartitionContext *context) = 0;

    virtual std::any visitFieldIdentifier(MySQLParser::FieldIdentifierContext *context) = 0;

    virtual std::any visitColumnName(MySQLParser::ColumnNameContext *context) = 0;

    virtual std::any visitColumnInternalRef(MySQLParser::ColumnInternalRefContext *context) = 0;

    virtual std::any visitColumnInternalRefList(MySQLParser::ColumnInternalRefListContext *context) = 0;

    virtual std::any visitColumnRef(MySQLParser::ColumnRefContext *context) = 0;

    virtual std::any visitInsertIdentifier(MySQLParser::InsertIdentifierContext *context) = 0;

    virtual std::any visitIndexName(MySQLParser::IndexNameContext *context) = 0;

    virtual std::any visitIndexRef(MySQLParser::IndexRefContext *context) = 0;

    virtual std::any visitTableWild(MySQLParser::TableWildContext *context) = 0;

    virtual std::any visitSchemaName(MySQLParser::SchemaNameContext *context) = 0;

    virtual std::any visitSchemaRef(MySQLParser::SchemaRefContext *context) = 0;

    virtual std::any visitProcedureName(MySQLParser::ProcedureNameContext *context) = 0;

    virtual std::any visitProcedureRef(MySQLParser::ProcedureRefContext *context) = 0;

    virtual std::any visitFunctionName(MySQLParser::FunctionNameContext *context) = 0;

    virtual std::any visitFunctionRef(MySQLParser::FunctionRefContext *context) = 0;

    virtual std::any visitTriggerName(MySQLParser::TriggerNameContext *context) = 0;

    virtual std::any visitTriggerRef(MySQLParser::TriggerRefContext *context) = 0;

    virtual std::any visitViewName(MySQLParser::ViewNameContext *context) = 0;

    virtual std::any visitViewRef(MySQLParser::ViewRefContext *context) = 0;

    virtual std::any visitTablespaceName(MySQLParser::TablespaceNameContext *context) = 0;

    virtual std::any visitTablespaceRef(MySQLParser::TablespaceRefContext *context) = 0;

    virtual std::any visitLogfileGroupName(MySQLParser::LogfileGroupNameContext *context) = 0;

    virtual std::any visitLogfileGroupRef(MySQLParser::LogfileGroupRefContext *context) = 0;

    virtual std::any visitEventName(MySQLParser::EventNameContext *context) = 0;

    virtual std::any visitEventRef(MySQLParser::EventRefContext *context) = 0;

    virtual std::any visitUdfName(MySQLParser::UdfNameContext *context) = 0;

    virtual std::any visitServerName(MySQLParser::ServerNameContext *context) = 0;

    virtual std::any visitServerRef(MySQLParser::ServerRefContext *context) = 0;

    virtual std::any visitEngineRef(MySQLParser::EngineRefContext *context) = 0;

    virtual std::any visitTableName(MySQLParser::TableNameContext *context) = 0;

    virtual std::any visitFilterTableRef(MySQLParser::FilterTableRefContext *context) = 0;

    virtual std::any visitTableRefWithWildcard(MySQLParser::TableRefWithWildcardContext *context) = 0;

    virtual std::any visitTableRef(MySQLParser::TableRefContext *context) = 0;

    virtual std::any visitTableRefList(MySQLParser::TableRefListContext *context) = 0;

    virtual std::any visitTableAliasRefList(MySQLParser::TableAliasRefListContext *context) = 0;

    virtual std::any visitParameterName(MySQLParser::ParameterNameContext *context) = 0;

    virtual std::any visitLabelIdentifier(MySQLParser::LabelIdentifierContext *context) = 0;

    virtual std::any visitLabelRef(MySQLParser::LabelRefContext *context) = 0;

    virtual std::any visitRoleIdentifier(MySQLParser::RoleIdentifierContext *context) = 0;

    virtual std::any visitPluginRef(MySQLParser::PluginRefContext *context) = 0;

    virtual std::any visitComponentRef(MySQLParser::ComponentRefContext *context) = 0;

    virtual std::any visitResourceGroupRef(MySQLParser::ResourceGroupRefContext *context) = 0;

    virtual std::any visitWindowName(MySQLParser::WindowNameContext *context) = 0;

    virtual std::any visitPureIdentifier(MySQLParser::PureIdentifierContext *context) = 0;

    virtual std::any visitIdentifier(MySQLParser::IdentifierContext *context) = 0;

    virtual std::any visitIdentifierList(MySQLParser::IdentifierListContext *context) = 0;

    virtual std::any visitIdentifierListWithParentheses(MySQLParser::IdentifierListWithParenthesesContext *context) = 0;

    virtual std::any visitQualifiedIdentifier(MySQLParser::QualifiedIdentifierContext *context) = 0;

    virtual std::any visitSimpleIdentifier(MySQLParser::SimpleIdentifierContext *context) = 0;

    virtual std::any visitDotIdentifier(MySQLParser::DotIdentifierContext *context) = 0;

    virtual std::any visitUlong_number(MySQLParser::Ulong_numberContext *context) = 0;

    virtual std::any visitReal_ulong_number(MySQLParser::Real_ulong_numberContext *context) = 0;

    virtual std::any visitUlonglong_number(MySQLParser::Ulonglong_numberContext *context) = 0;

    virtual std::any visitReal_ulonglong_number(MySQLParser::Real_ulonglong_numberContext *context) = 0;

    virtual std::any visitSignedLiteral(MySQLParser::SignedLiteralContext *context) = 0;

    virtual std::any visitSignedLiteralOrNull(MySQLParser::SignedLiteralOrNullContext *context) = 0;

    virtual std::any visitLiteral(MySQLParser::LiteralContext *context) = 0;

    virtual std::any visitLiteralOrNull(MySQLParser::LiteralOrNullContext *context) = 0;

    virtual std::any visitNullAsLiteral(MySQLParser::NullAsLiteralContext *context) = 0;

    virtual std::any visitStringList(MySQLParser::StringListContext *context) = 0;

    virtual std::any visitTextStringLiteral(MySQLParser::TextStringLiteralContext *context) = 0;

    virtual std::any visitTextString(MySQLParser::TextStringContext *context) = 0;

    virtual std::any visitTextStringHash(MySQLParser::TextStringHashContext *context) = 0;

    virtual std::any visitTextLiteral(MySQLParser::TextLiteralContext *context) = 0;

    virtual std::any visitTextStringNoLinebreak(MySQLParser::TextStringNoLinebreakContext *context) = 0;

    virtual std::any visitTextStringLiteralList(MySQLParser::TextStringLiteralListContext *context) = 0;

    virtual std::any visitNumLiteral(MySQLParser::NumLiteralContext *context) = 0;

    virtual std::any visitBoolLiteral(MySQLParser::BoolLiteralContext *context) = 0;

    virtual std::any visitNullLiteral(MySQLParser::NullLiteralContext *context) = 0;

    virtual std::any visitInt64Literal(MySQLParser::Int64LiteralContext *context) = 0;

    virtual std::any visitTemporalLiteral(MySQLParser::TemporalLiteralContext *context) = 0;

    virtual std::any visitFloatOptions(MySQLParser::FloatOptionsContext *context) = 0;

    virtual std::any visitStandardFloatOptions(MySQLParser::StandardFloatOptionsContext *context) = 0;

    virtual std::any visitPrecision(MySQLParser::PrecisionContext *context) = 0;

    virtual std::any visitTextOrIdentifier(MySQLParser::TextOrIdentifierContext *context) = 0;

    virtual std::any visitLValueIdentifier(MySQLParser::LValueIdentifierContext *context) = 0;

    virtual std::any visitRoleIdentifierOrText(MySQLParser::RoleIdentifierOrTextContext *context) = 0;

    virtual std::any visitSizeNumber(MySQLParser::SizeNumberContext *context) = 0;

    virtual std::any visitParentheses(MySQLParser::ParenthesesContext *context) = 0;

    virtual std::any visitEqual(MySQLParser::EqualContext *context) = 0;

    virtual std::any visitOptionType(MySQLParser::OptionTypeContext *context) = 0;

    virtual std::any visitRvalueSystemVariableType(MySQLParser::RvalueSystemVariableTypeContext *context) = 0;

    virtual std::any visitSetVarIdentType(MySQLParser::SetVarIdentTypeContext *context) = 0;

    virtual std::any visitJsonAttribute(MySQLParser::JsonAttributeContext *context) = 0;

    virtual std::any visitIdentifierKeyword(MySQLParser::IdentifierKeywordContext *context) = 0;

    virtual std::any visitIdentifierKeywordsAmbiguous1RolesAndLabels(MySQLParser::IdentifierKeywordsAmbiguous1RolesAndLabelsContext *context) = 0;

    virtual std::any visitIdentifierKeywordsAmbiguous2Labels(MySQLParser::IdentifierKeywordsAmbiguous2LabelsContext *context) = 0;

    virtual std::any visitLabelKeyword(MySQLParser::LabelKeywordContext *context) = 0;

    virtual std::any visitIdentifierKeywordsAmbiguous3Roles(MySQLParser::IdentifierKeywordsAmbiguous3RolesContext *context) = 0;

    virtual std::any visitIdentifierKeywordsUnambiguous(MySQLParser::IdentifierKeywordsUnambiguousContext *context) = 0;

    virtual std::any visitRoleKeyword(MySQLParser::RoleKeywordContext *context) = 0;

    virtual std::any visitLValueKeyword(MySQLParser::LValueKeywordContext *context) = 0;

    virtual std::any visitIdentifierKeywordsAmbiguous4SystemVariables(MySQLParser::IdentifierKeywordsAmbiguous4SystemVariablesContext *context) = 0;

    virtual std::any visitRoleOrIdentifierKeyword(MySQLParser::RoleOrIdentifierKeywordContext *context) = 0;

    virtual std::any visitRoleOrLabelKeyword(MySQLParser::RoleOrLabelKeywordContext *context) = 0;


};

}  // namespace parsers
