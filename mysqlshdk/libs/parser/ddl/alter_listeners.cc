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

#include "mysqlshdk/libs/parser/ddl/alter_listeners.h"
#include <string>
#include <vector>

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wextra-semi"
#endif

#include "mysqlshdk/libs/parser/mysql/MySQLLexer.h"
#include "mysqlshdk/libs/parser/mysql/MySQLParser.h"
#include "mysqlshdk/libs/parser/mysql/MySQLParserBaseListener.h"

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#include "mysqlshdk/libs/parser/ddl/create_listeners.h"
#include "mysqlshdk/libs/utils/utils_general.h"

namespace mysqlshdk {
namespace parser {

namespace {

std::string default_or_normalized_name(antlr4::ParserRuleContext *ctx,
                                       bool is_default) {
  if (is_default) return "default";

  return shcore::str_lower(parsers::MySQLBaseLexer::sourceTextForContext(ctx));
}

}  // namespace

void Alter_listener::add_change(const std::string &op,
                                const shcore::Dictionary_t &change) {
  // coalesce update changes to the same object
  if (op == "alter") {
    const auto type = change->get_string("_type");
    const auto schema = change->get_string("_schema");
    const auto table = change->get_string("_table");
    const auto name = change->get_string("_name");
    const auto replace = change->get_bool("_replace");

    for (const auto &c : **m_change_list) {
      auto cd = c.as_map();
      if (auto it = cd->find("alter"); it != cd->end()) {
        auto chg = it->second.as_map();

        if (chg->get_string("_type") == type &&
            chg->get_string("_schema") == schema &&
            chg->get_string("_table") == table &&
            chg->get_string("_name") == name &&
            chg->get_bool("_replace") == replace) {
          chg->merge_contents(change, true);
          return;
        }
      }
    }
  }
  (*m_change_list)->emplace_back(shcore::make_dict(op, shcore::Value(change)));
}

void Alter_listener::add_update(const std::string &type,
                                const std::string &name,
                                const shcore::Dictionary_t &updates) {
  updates->set("_type", type);
  if (!m_schema.empty()) updates->set("_schema", m_schema);
  updates->set("_name", name);

  add_change("alter", updates);
}

void Alter_listener::add_update(const std::string &type,
                                const std::string &name,
                                const std::string &table_name,
                                const shcore::Dictionary_t &updates) {
  updates->set("_type", type);
  updates->set("_schema", m_schema);
  updates->set("_table", table_name);
  updates->set("_name", name);

  add_change("alter", updates);
}

void Alter_listener::add_replace(const std::string &type,
                                 const std::string &name,
                                 const std::string &table_name,
                                 const shcore::Dictionary_t &updates) {
  updates->set("_type", type);
  updates->set("_schema", m_schema);
  updates->set("_table", table_name);
  updates->set("_name", name);
  updates->set("_replace", true);

  add_change("alter", updates);
}

void Alter_listener::add_drop(const std::string &type, const std::string &name,
                              const std::string &table_name) {
  auto d = shcore::make_dict("_type", type, "_schema", shcore::Value(m_schema),
                             "_name", name);

  if (!table_name.empty()) d->emplace("_table", table_name);
  add_change("drop", d);
}

void Alter_listener::add_create(const std::string &type,
                                const std::string &table_name,
                                const shcore::Dictionary_t &change) {
  change->set("_type", type);
  change->set("_schema", m_schema);
  if (!table_name.empty()) change->set("_table", table_name);
  add_change("create", change);
}

Schema_alter_listener::Schema_alter_listener(antlr4::tree::ParseTree *tree,
                                             shcore::Array_t *change_list)
    : Alter_listener("", change_list) {
  m_changes = shcore::make_dict();

  antlr4::tree::ParseTreeWalker::DEFAULT.walk(this, tree);
}

void Schema_alter_listener::exitAlterDatabase(
    parsers::MySQLParser::AlterDatabaseContext *ctx) {
  add_update("schema", Identifier_listener(ctx->schemaRef()).parts.back(),
             m_changes);
}

void Schema_alter_listener::exitAlterDatabaseOption(
    parsers::MySQLParser::AlterDatabaseOptionContext *ctx) {
  if (ctx->READ_SYMBOL() && ctx->ONLY_SYMBOL() && ctx->ternaryOption()) {
    m_changes->set(
        "readOnly",
        shcore::str_caseeq(ctx->ternaryOption()->getText(), "DEFAULT")
            ? shcore::Value("DEFAULT")
            : shcore::Value(std::stoi(ctx->ternaryOption()->getText()) != 0));
  }
}

void Schema_alter_listener::exitCharsetName(
    parsers::MySQLParser::CharsetNameContext *ctx) {
  m_changes->set("defaultCharacterSet",
                 default_or_normalized_name(ctx, ctx->DEFAULT_SYMBOL()));
}

void Schema_alter_listener::exitCollationName(
    parsers::MySQLParser::CollationNameContext *ctx) {
  m_changes->set("defaultCollate",
                 default_or_normalized_name(ctx, ctx->DEFAULT_SYMBOL()));
}

void Schema_alter_listener::exitDefaultEncryption(
    parsers::MySQLParser::DefaultEncryptionContext *ctx) {
  m_changes->set("defaultEncryption",
                 shcore::Value(shcore::str_caseeq(
                     parsers::MySQLBaseLexer::sourceTextForContext(
                         ctx->textStringLiteral()),
                     "Y")));
}

//-----------------------------------------------------------------------------

Table_alter_listener::Table_alter_listener(antlr4::tree::ParseTree *tree,
                                           const std::string &current_schema,
                                           shcore::Array_t *change_list,
                                           bool no_backslash_escapes)
    : Alter_listener(current_schema, change_list, no_backslash_escapes) {
  antlr4::tree::ParseTreeWalker::DEFAULT.walk(this, tree);
}

void Table_alter_listener::enterAlterTable(
    parsers::MySQLParser::AlterTableContext *ctx) {
  Identifier_listener identifier(ctx->tableRef());
  if (identifier.parts.size() > 1) m_schema = identifier.parts[0];
  m_table_name = identifier.parts.back();
}

void Table_alter_listener::exitTableConstraintDef(
    parsers::MySQLParser::TableConstraintDefContext *ctx) {
  auto tmp = shcore::make_dict();
  Key_definition_listener(ctx, tmp, m_no_backslash_escapes);

  if (auto fks = tmp->get_array("foreignKeys")) {
    for (const auto &fk : *fks) {
      add_create("foreignKey", m_table_name, fk.as_map());
    }
  }

  if (auto idxs = tmp->get_array("indexes")) {
    for (const auto &idx : *idxs) {
      add_create("index", m_table_name, idx.as_map());
    }
  }

  if (auto chks = tmp->get_array("checks")) {
    for (const auto &chk : *chks) {
      add_create("check", m_table_name, chk.as_map());
    }
  }
}

void Table_alter_listener::exitTableElement(
    parsers::MySQLParser::TableElementContext *ctx) {
  if (ctx->columnDefinition()) {
    auto tmp = shcore::make_dict();
    Column_definition_listener listener(ctx, tmp, m_no_backslash_escapes);

    auto changes = tmp->get_array("columns")->at(0).as_map();
    add_create("column", m_table_name, changes);
  } else {
    // handled as tableConstraint
  }
}

void Table_alter_listener::exitPartitionClause(
    parsers::MySQLParser::PartitionClauseContext *ctx) {
  add_update("table", m_table_name,
             shcore::make_dict(
                 "partitionClauseSql",
                 parsers::MySQLBaseLexer::sourceTextForContext(ctx, true)));
}

void Table_alter_listener::exitRemovePartitioning(
    parsers::MySQLParser::RemovePartitioningContext *) {
  add_update("table", m_table_name,
             shcore::make_dict("partitionClauseSql", shcore::Value::Null()));
}

void Table_alter_listener::exitAlterPartition(
    parsers::MySQLParser::AlterPartitionContext *ctx) {
  add_update("table", m_table_name,
             shcore::make_dict(
                 "partitionOperationSql",
                 parsers::MySQLBaseLexer::sourceTextForContext(ctx, true)));
}

void Table_alter_listener::exitAlterListItem(
    parsers::MySQLParser::AlterListItemContext *ctx) {
  auto parse_as_column =
      [this](parsers::MySQLParser::AlterListItemContext *item) {
        auto tmp = shcore::make_dict();
        Column_definition_listener listener(item, tmp, m_no_backslash_escapes);

        auto changes = tmp->get_array("columns")->at(0).as_map();
        if (item->identifier())
          changes->set("name",
                       Identifier_listener(item->identifier()).parts.back());

        if (item->place()) {
          if (item->place()->AFTER_SYMBOL())
            changes->set("placeAfter",
                         shcore::Value(item->place()->identifier()->getText()));
          else
            changes->set("placeFirst", shcore::Value::True());
        }
        return changes;
      };

  if (ctx->ADD_SYMBOL()) {
    if (ctx->tableConstraintDef()) {
      // handled as tableConstraintDef
    } else {
      if (ctx->OPEN_PAR_SYMBOL()) {
        // handled as tableElementList
      } else {
        add_create("column", m_table_name, parse_as_column(ctx));
      }
    }
  } else if (ctx->CHANGE_SYMBOL()) {
    auto changes = parse_as_column(ctx);
    changes->set("name", Identifier_listener(ctx->identifier()).parts.back());

    add_replace("column",
                Identifier_listener(ctx->columnInternalRef()).parts.back(),
                m_table_name, changes);
  } else if (ctx->MODIFY_SYMBOL()) {
    add_replace("column",
                Identifier_listener(ctx->columnInternalRef()).parts.back(),
                m_table_name, parse_as_column(ctx));
  } else if (ctx->ALTER_SYMBOL()) {
    if (ctx->INDEX_SYMBOL()) {
      add_update(
          "index", Identifier_listener(ctx->indexRef()).parts.back(),
          m_table_name,
          shcore::make_dict("visible", ctx->visibility()->VISIBLE_SYMBOL()
                                           ? shcore::Value::True()
                                           : shcore::Value::False()));
    } else if (ctx->CHECK_SYMBOL() || ctx->CONSTRAINT_SYMBOL()) {
      add_update("check", Identifier_listener(ctx->identifier()).parts.back(),
                 m_table_name,
                 shcore::make_dict("enforced",
                                   ctx->constraintEnforcement()->NOT_SYMBOL()
                                       ? shcore::Value::False()
                                       : shcore::Value::True()));
    } else {
      std::string column = ctx->columnInternalRef()->getText();

      if (ctx->DROP_SYMBOL() && ctx->DEFAULT_SYMBOL()) {
        add_update("column", column, m_table_name,
                   shcore::make_dict("defaultValue", shcore::Value::Null()));
      } else if (ctx->SET_SYMBOL() && ctx->visibility()) {
        add_update(
            "column", column, m_table_name,
            shcore::make_dict("visible", ctx->visibility()->VISIBLE_SYMBOL()
                                             ? shcore::Value::True()
                                             : shcore::Value::False()));
      } else if (ctx->SET_SYMBOL() && ctx->DEFAULT_SYMBOL()) {
        if (ctx->exprWithParentheses()) {
          add_update(
              "column", column, m_table_name,
              shcore::make_dict("defaultValue",
                                shcore::Value(parsers::MySQLParser::getText(
                                    ctx->exprWithParentheses(), false))));
        } else {
          add_update(
              "column", column, m_table_name,
              shcore::make_dict("defaultValue",
                                shcore::Value(parsers::MySQLParser::getText(
                                    ctx->signedLiteralOrNull(), false))));
        }
      } else {
        assert(0);
      }
    }
  } else if (ctx->DROP_SYMBOL()) {
    if (ctx->FOREIGN_SYMBOL()) {
      add_drop("foreignKey",
               Identifier_listener(ctx->columnInternalRef()).parts.back(),
               m_table_name);
    } else if (ctx->PRIMARY_SYMBOL()) {
      add_drop("index", "PRIMARY", m_table_name);
    } else if (ctx->keyOrIndex()) {
      add_drop("index", Identifier_listener(ctx->indexRef()).parts.back(),
               m_table_name);
    } else if (ctx->CHECK_SYMBOL()) {
      add_drop("check", Identifier_listener(ctx->identifier()).parts.back(),
               m_table_name);
    } else if (ctx->CONSTRAINT_SYMBOL()) {
      // generate as a CONSTRAINT type, because drop constraint can work no both
      // FKs and CHECKs
      add_drop("constraint",
               Identifier_listener(ctx->identifier()).parts.back(),
               m_table_name);
    } else {
      // column
      add_drop("column",
               Identifier_listener(ctx->columnInternalRef()).parts.back(),
               m_table_name);
    }
  } else if (ctx->RENAME_SYMBOL()) {
    if (ctx->COLUMN_SYMBOL()) {
      // rename column
      add_update("column",
                 Identifier_listener(ctx->columnInternalRef()).parts.back(),
                 m_table_name,
                 shcore::make_dict(
                     "name",
                     shcore::Value(
                         Identifier_listener(ctx->identifier()).parts.back())));
    } else if (ctx->keyOrIndex() != nullptr) {
      // rename index
      add_update(
          "index", Identifier_listener(ctx->indexRef()).parts.back(),
          m_table_name,
          shcore::make_dict(
              "name", shcore::Value(
                          Identifier_listener(ctx->indexName()).parts.back())));
    } else if (ctx->TO_SYMBOL() || ctx->AS_SYMBOL()) {
      // rename table
      Identifier_listener listener(ctx->tableName());
      auto updates =
          shcore::make_dict("name", shcore::Value(listener.parts.back()));
      if (listener.parts.size() > 1 && !listener.parts[0].empty())
        updates->set("schema", listener.parts[0]);
      add_update("table", m_table_name, updates);
    } else {
      assert(0);
    }
  } else if (ctx->CONVERT_SYMBOL()) {
    if (ctx->DEFAULT_SYMBOL())
      add_update(
          "table", m_table_name,
          shcore::make_dict("convertCharacterSet", shcore::Value("DEFAULT")));
    else
      add_update(
          "table", m_table_name,
          shcore::make_dict("convertCharacterSet",
                            shcore::Value(ctx->charsetName()->getText())));
    if (ctx->collate()) {
      add_update(
          "table", m_table_name,
          shcore::make_dict(
              "convertCollate",
              shcore::Value(ctx->collate()->collationName()->getText())));
    }
  } else if (ctx->DISABLE_SYMBOL()) {
    // DISABLE KEYS - ignored because not DDL, also myisam only
  } else if (ctx->ENABLE_SYMBOL()) {
    // ENABLE KEYS - ignored because not DDL
  } else if (ctx->FORCE_SYMBOL()) {
    // FORCE - ignored because not DDL
  } else if (ctx->ORDER_SYMBOL()) {
    // ORDER BY - ignored because not DDL, myisam specific
  } else if (ctx->UPGRADE_SYMBOL()) {
    add_update("table", m_table_name,
               shcore::make_dict(
                   "partitionOperationSql",
                   parsers::MySQLBaseLexer::sourceTextForContext(ctx, true)));
  }
}

void Table_alter_listener::exitCreateTableOptionsSpaceSeparated(
    parsers::MySQLParser::CreateTableOptionsSpaceSeparatedContext *ctx) {
  auto options = shcore::make_dict();
  Create_table_option_listener(ctx, options, m_no_backslash_escapes);

  add_update("table", m_table_name, options);
}

//-----------------------------------------------------------------------------

View_alter_listener::View_alter_listener(antlr4::tree::ParseTree *tree,
                                         const std::string &current_schema,
                                         shcore::Array_t *change_list)
    : Alter_listener(current_schema, change_list) {
  m_changes = shcore::make_dict();
  antlr4::tree::ParseTreeWalker::DEFAULT.walk(this, tree);
}

void View_alter_listener::exitAlterView(
    parsers::MySQLParser::AlterViewContext *ctx) {
  Identifier_listener listener(ctx->viewRef());

  if (listener.parts.size() > 1 && !listener.parts[0].empty())
    m_schema = listener.parts[0];

  add_update("view", listener.parts.back(), m_changes);
}

void View_alter_listener::exitViewCheckOption(
    parsers::MySQLParser::ViewCheckOptionContext *ctx) {
  std::string check;
  if (ctx->LOCAL_SYMBOL())
    check = "LOCAL";
  else if (ctx->CASCADED_SYMBOL())
    check = "CASCADED";
  m_changes->set("checkOption", check);
}

void View_alter_listener::exitViewAlgorithm(
    parsers::MySQLParser::ViewAlgorithmContext *ctx) {
  switch (ctx->algorithm->getType()) {
    case parsers::MySQLLexer::MERGE_SYMBOL:
      m_changes->set("algorithm", "MERGE");
      break;
    case parsers::MySQLLexer::TEMPTABLE_SYMBOL:
      m_changes->set("algorithm", "TEMPTABLE");
      break;
    default:
      m_changes->set("algorithm", "UNDEFINED");
      break;
  }
}

void View_alter_listener::exitDefinerClause(
    parsers::MySQLParser::DefinerClauseContext *ctx) {
  m_changes->set("definer", parsers::MySQLBaseLexer::sourceTextForContext(
                                ctx->user(), true));
}

void View_alter_listener::exitViewTail(
    parsers::MySQLParser::ViewTailContext *ctx) {
  m_changes->set(
      "sqlBody",
      parsers::MySQLBaseLexer::sourceTextForContext(
          ctx->viewQueryBlock()->queryExpressionWithOptLockingClauses(), true));
}

void View_alter_listener::exitViewSuid(
    parsers::MySQLParser::ViewSuidContext *ctx) {
  if (ctx->INVOKER_SYMBOL() != nullptr)
    m_changes->set("sqlSecurity", "INVOKER");
  else if (ctx->DEFINER_SYMBOL() != nullptr)
    m_changes->set("sqlSecurity", "DEFINER");
  else
    assert(0);
}

void View_alter_listener::exitColumnInternalRefList(
    parsers::MySQLParser::ColumnInternalRefListContext *ctx) {
  m_changes->set("columnList",
                 parsers::MySQLBaseLexer::sourceTextForContext(ctx, true));
}

//-----------------------------------------------------------------------------

Routine_alter_listener::Routine_alter_listener(
    antlr4::tree::ParseTree *tree, const std::string &current_schema,
    shcore::Array_t *change_list, bool no_backslash_escapes)
    : Alter_listener(current_schema, change_list, no_backslash_escapes) {
  m_changes = shcore::make_dict();
  antlr4::tree::ParseTreeWalker::DEFAULT.walk(this, tree);
}

void Routine_alter_listener::exitAlterStatement(
    parsers::MySQLParser::AlterStatementContext *ctx) {
  if (ctx->PROCEDURE_SYMBOL()) {
    Identifier_listener identifier(ctx->procedureRef());
    if (identifier.parts.size() > 1) m_schema = identifier.parts[0];
    add_update("procedure", identifier.parts.back(), m_changes);
  } else {
    Identifier_listener identifier(ctx->functionRef());
    if (identifier.parts.size() > 1) m_schema = identifier.parts[0];
    add_update("function", identifier.parts.back(), m_changes);
  }
}

void Routine_alter_listener::exitRoutineCreateOption(
    parsers::MySQLParser::RoutineCreateOptionContext *ctx) {
  if (ctx->DETERMINISTIC_SYMBOL()) {
    if (ctx->NOT_SYMBOL())
      m_changes->set("deterministic", shcore::Value::False());
    else
      m_changes->set("deterministic", shcore::Value::True());
  }
}

void Routine_alter_listener::exitRoutineOption(
    parsers::MySQLParser::RoutineOptionContext *ctx) {
  shcore::Dictionary_t routine = m_changes;

  switch (ctx->option->getType()) {
    case parsers::MySQLLexer::SQL_SYMBOL:
      routine->set("sqlSecurity", ctx->security->getText());
      break;

    case parsers::MySQLLexer::COMMENT_SYMBOL:
      routine->set("comment", unquote_text_literal(ctx->textLiteral(),
                                                   m_no_backslash_escapes));
      break;

    case parsers::MySQLLexer::LANGUAGE_SYMBOL:
      routine->set("language", internal::read_routine_language(ctx));
      break;

    case parsers::MySQLLexer::NO_SYMBOL:
      // NO SQL
      routine->set("sqlCharacteristic", "NO SQL");
      break;

    case parsers::MySQLLexer::CONTAINS_SYMBOL:
      // CONTAINS SQL
      routine->set("sqlCharacteristic", "CONTAINS SQL");
      break;

    case parsers::MySQLLexer::READS_SYMBOL:
      // READS SQL DATA
      routine->set("sqlCharacteristic", "READS SQL DATA");
      break;

    case parsers::MySQLLexer::MODIFIES_SYMBOL:
      // MODIFIES SQL DATA
      routine->set("sqlCharacteristic", "MODIFIES SQL DATA");
      break;

    default:
      break;
  }
}

//-----------------------------------------------------------------------------

Event_alter_listener::Event_alter_listener(antlr4::tree::ParseTree *tree,
                                           const std::string &current_schema,
                                           shcore::Array_t *change_list,
                                           bool no_backslash_escapes)
    : Alter_listener(current_schema, change_list, no_backslash_escapes) {
  m_changes = shcore::make_dict();
  antlr4::tree::ParseTreeWalker::DEFAULT.walk(this, tree);
}

//----------------------------------------------------------------------------------------------------------------------

void Event_alter_listener::exitDefinerClause(
    parsers::MySQLParser::DefinerClauseContext *ctx) {
  shcore::Dictionary_t event = m_changes;
  event->set("definer",
             parsers::MySQLBaseLexer::sourceTextForContext(ctx->user(), true));
}

//----------------------------------------------------------------------------------------------------------------------

void Event_alter_listener::exitAlterEvent(
    parsers::MySQLParser::AlterEventContext *ctx) {
  shcore::Dictionary_t event = m_changes;

  if (ctx->PRESERVE_SYMBOL() != nullptr)
    event->set("preserve", ctx->NOT_SYMBOL() == nullptr);

  if (ctx->ENABLE_SYMBOL() != nullptr) {
    event->set("enable", "ENABLE");
  } else if (ctx->DISABLE_SYMBOL() != nullptr) {
    if (!ctx->ON_SYMBOL().empty() && ctx->replica() != nullptr)
      event->set("enable", "DISABLE ON " + ctx->replica()->getText());
    else
      event->set("enable", "DISABLE");
  }

  if (ctx->RENAME_SYMBOL() != nullptr) {
    Identifier_listener listener(ctx->identifier());

    event->set("name", listener.parts.back());
  }

  if (ctx->COMMENT_SYMBOL() != nullptr)
    event->set("comment", unquote_text_literal(ctx->textLiteral(),
                                               m_no_backslash_escapes));

  if (ctx->compoundStatement())
    event->set("sqlBody", parsers::MySQLBaseLexer::sourceTextForContext(
                              ctx->compoundStatement(), true));

  Identifier_listener listener(ctx->eventRef());

  if (listener.parts.size() > 1 && !listener.parts[0].empty())
    m_schema = listener.parts[0];

  add_update("event", listener.parts.back(), m_changes);
}

//----------------------------------------------------------------------------------------------------------------------

void Event_alter_listener::exitSchedule(
    parsers::MySQLParser::ScheduleContext *ctx) {
  shcore::Dictionary_t event = m_changes;

  if (ctx->AT_SYMBOL() != nullptr) {
    event->set("at", parsers::MySQLBaseLexer::sourceTextForContext(ctx->expr(0),
                                                                   true));
  } else {
    event->set("interval", parsers::MySQLBaseLexer::sourceTextForContext(
                               ctx->expr(0), true) +
                               " " +
                               parsers::MySQLBaseLexer::sourceTextForContext(
                                   ctx->interval(), true));

    size_t expressionIndex = 1;
    if (ctx->STARTS_SYMBOL() != nullptr)
      event->set("intervalStart", parsers::MySQLBaseLexer::sourceTextForContext(
                                      ctx->expr(expressionIndex++), true));

    if (ctx->ENDS_SYMBOL() != nullptr)
      event->set("intervalEnd", parsers::MySQLBaseLexer::sourceTextForContext(
                                    ctx->expr(expressionIndex), true));
  }
}

}  // namespace parser
}  // namespace mysqlshdk
