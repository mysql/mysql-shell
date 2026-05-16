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

#include <cctype>
#include <charconv>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
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
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlshdk {
namespace parser {
using antlr4::ParserRuleContext;
namespace tree = antlr4::tree;

namespace {

shcore::Dictionary_t read_library_reference(
    parsers::MySQLParser::LibraryNameWithAliasContext *ctx,
    const std::string &current_schema) {
  Identifier_listener listener(ctx->libraryName());

  auto library = shcore::make_dict("_schema",
                                   listener.parts.size() > 1
                                       ? shcore::Value(listener.parts[0])
                                       : shcore::Value(current_schema),
                                   "name", listener.parts.back());

  if (ctx->identifier())
    library->set("alias", Identifier_listener(ctx->identifier()).parts.back());

  return library;
}

size_t number_value(std::string_view value) {
  // Value can have a suffix. And it can be a hex number.
  size_t factor = 1;
  switch (std::tolower(static_cast<unsigned char>(value.back()))) {
    case 'g':
      factor *= 1024;
      [[fallthrough]];
    case 'm':
      factor *= 1024;
      [[fallthrough]];
    case 'k':
      factor *= 1024;
      value.remove_suffix(1);
  }

  int base = 10;
  if (value.size() > 2 && value[0] == '0' &&
      (value[1] == 'x' || value[1] == 'X')) {
    value.remove_prefix(2);
    base = 16;
  }

  size_t result;
  const auto [ptr, error] =
      std::from_chars(value.data(), value.data() + value.size(), result, base);
  if (error != std::errc{} || ptr != value.data() + value.size())
    throw std::invalid_argument("Invalid size number");

  return factor * result;
}

std::string unquote_text_string_token(std::string_view text,
                                      bool no_backslash_escapes) {
  const auto quote_pos = text.find_first_of("'\"");

  if (quote_pos == std::string::npos) return std::string{text};

  text = text.substr(quote_pos);
  const auto quote = text.front();

  return shcore::unquote_sql_string(text, quote, no_backslash_escapes);
}

}  // namespace

//----------------------------------------------------------------------------------------------------------------------

std::string unquote_text_string_literal(
    parsers::MySQLParser::TextStringLiteralContext *ctx,
    bool no_backslash_escapes) {
  return unquote_text_string_token(ctx->getText(), no_backslash_escapes);
}

std::string unquote_text_literal(parsers::MySQLParser::TextLiteralContext *ctx,
                                 bool no_backslash_escapes) {
  std::string result;

  if (const auto nchar_text = ctx->NCHAR_TEXT())
    result +=
        unquote_text_string_token(nchar_text->getText(), no_backslash_escapes);

  for (const auto literal : ctx->textStringLiteral())
    result += unquote_text_string_literal(literal, no_backslash_escapes);

  return result;
}

namespace {

std::string unquoteIdentifier(std::string_view text) {
  return shcore::unquote_identifier(text, true);
}

//----------------------------------------------------------------------------------------------------------------------

void parse_references(parsers::MySQLParser::ReferencesContext *ctx,
                      Db_object_references *references) {
  Identifier_listener listener(ctx->tableRef());
  Identifier identifier;
  switch (listener.parts.size()) {
    case 1:
      identifier.name = listener.parts[0];
      break;
    case 2:
      identifier.schema = listener.parts[0];
      identifier.name = listener.parts[1];
      break;
    default:
      throw std::invalid_argument(
          "Invalid reference: " +
          parsers::MySQLBaseLexer::sourceTextForContext(ctx->tableRef()));
  }

  references->target_identifier = std::move(identifier);
  if (ctx->identifierListWithParentheses() != nullptr) {
    for (auto column :
         ctx->identifierListWithParentheses()->identifierList()->identifier())
      references->column_names.push_back(unquoteIdentifier(column->getText()));
  }

  if (ctx->match != nullptr) {
    references->foreign_key->set("match", ctx->match->getText());
  }

  if (ctx->option != nullptr) {
    if (ctx->option->getType() == parsers::MySQLLexer::UPDATE_SYMBOL) {
      references->foreign_key->set(
          "updateRule",
          parsers::MySQLBaseLexer::sourceTextForContext(ctx->deleteOption(0)));
      if (ctx->deleteOption().size() > 1)  // 2 rules actually: UPDATE DELETE
        references->foreign_key->set(
            "deleteRule", parsers::MySQLBaseLexer::sourceTextForContext(
                              ctx->deleteOption(1)));
    } else {
      references->foreign_key->set(
          "deleteRule",
          parsers::MySQLBaseLexer::sourceTextForContext(ctx->deleteOption(0)));
      if (ctx->deleteOption().size() > 1)  // 2 rules actually: DELETE UPDATE
        references->foreign_key->set(
            "updateRule", parsers::MySQLBaseLexer::sourceTextForContext(
                              ctx->deleteOption(1)));
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Collects only the names of each entry in a key list into the references
 * entry.
 */
void column_names_from_key_list(parsers::MySQLParser::KeyListContext *ctx,
                                Db_object_references *references) {
  for (auto part : ctx->keyPart())
    references->column_names.push_back(
        unquoteIdentifier(part->identifier()->getText()));
}

//----------------------------------------------------------------------------------------------------------------------

void parse_key_list(ParserRuleContext *ctx, shcore::Dictionary_t index) {
  shcore::Array_t columns = shcore::make_array();
  index->set("columns", columns);

  auto keyList = dynamic_cast<parsers::MySQLParser::KeyListContext *>(ctx);
  if (keyList != nullptr) {
    for (auto part : keyList->keyPart()) {
      shcore::Dictionary_t indexColumn(shcore::make_dict());
      indexColumn->set("referencedColumn",
                       Identifier_listener(part->identifier()).parts.back());
      columns->emplace_back(shcore::Value(indexColumn));

      if (part->fieldLength() != nullptr) {
        auto child =
            dynamic_cast<tree::ParseTree *>(part->fieldLength()->children[1]);
        indexColumn->set("columnLength", number_value(child->getText()));
      }

      if (part->direction() != nullptr)
        indexColumn->set("descend",
                         (part->direction()->DESC_SYMBOL() != nullptr));
    }
  } else {
    auto key_list_with_expression =
        dynamic_cast<parsers::MySQLParser::KeyListWithExpressionContext *>(ctx);
    assert(key_list_with_expression != nullptr);
    for (auto keyPartOrExpression :
         key_list_with_expression->keyPartOrExpression()) {
      shcore::Dictionary_t indexColumn(shcore::make_dict());
      if (keyPartOrExpression->keyPart() != nullptr) {
        auto part = keyPartOrExpression->keyPart();
        indexColumn->set("referencedColumn",
                         unquoteIdentifier(part->identifier()->getText()));

        if (part->fieldLength() != nullptr) {
          auto child =
              dynamic_cast<tree::ParseTree *>(part->fieldLength()->children[1]);
          indexColumn->set("columnLength", static_cast<uint64_t>(
                                               std::stoull(child->getText())));
        }

        if (part->direction() != nullptr)
          indexColumn->set("descend",
                           part->direction()->DESC_SYMBOL() != nullptr);
      } else {
        auto expression = parsers::MySQLBaseLexer::sourceTextForContext(
            keyPartOrExpression->exprWithParentheses()->expr());
        indexColumn->set("expression", expression);
        if (keyPartOrExpression->direction() != nullptr)
          indexColumn->set(
              "descend",
              keyPartOrExpression->direction()->DESC_SYMBOL() != nullptr);
      }

      columns->emplace_back(shcore::Value(indexColumn));
    }
  }
}

ParserRuleContext *key_list_context(
    parsers::MySQLParser::TableConstraintDefContext *ctx) {
  if (ctx->keyListWithExpression() != nullptr)
    return ctx->keyListWithExpression();
  return ctx->keyList();
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Helper to bring the index type string into a commonly used form.
 */
std::string format_index_type(std::string_view index_type) {
  index_type = index_type.substr(
      0, index_type.find(' '));  // Only first word is meaningful.
  const auto i = shcore::str_upper(index_type);
  if (i == "KEY") return "INDEX";
  return i;
}

}  // namespace

//----------------------------------------------------------------------------------------------------------------------

shcore::Array_t ensure_array(shcore::Dictionary_t dict,
                             const std::string &field) {
  auto array = dict->get_array(field);
  if (!array) {
    array = shcore::make_array();
    dict->set(field, array);
  }
  return array;
}

//----------------------------------------------------------------------------------------------------------------------

// We have a number of partial listeners for parts that need to be evaluated in
// more than one object listener.

//----------------- Identifier_listener
//---------------------------------------------------------------------------------

Identifier_listener::Identifier_listener(tree::ParseTree *tree) {
  tree::ParseTreeWalker::DEFAULT.walk(this, tree);
}

//----------------------------------------------------------------------------------------------------------------------

void Identifier_listener::enterIdentifier(
    parsers::MySQLParser::IdentifierContext *ctx) {
  parts.push_back(unquoteIdentifier(ctx->getText()));
}

//----------------- Column_definition_listener
//---------------------------------------------------------------------------

Column_definition_listener::Column_definition_listener(
    tree::ParseTree *tree, shcore::Dictionary_t table,
    bool no_backslash_escapes)
    : column(shcore::make_dict()),
      m_table(table),
      m_no_backslash_escapes(no_backslash_escapes) {
  tree::ParseTreeWalker::DEFAULT.walk(this, tree);
}

void Column_definition_listener::enterColumnDefinition(
    parsers::MySQLParser::ColumnDefinitionContext *ctx) {
  // For servers < 8 the column name can be qualified. With 8+ this changed to
  // a simple identifier.
  Identifier_listener listener(ctx->columnName()->identifier());
  column->set("name", std::move(listener.parts.back()));
}

void Column_definition_listener::exitFieldDefinition(
    parsers::MySQLParser::FieldDefinitionContext *ctx) {
  if (ctx->dataType() != nullptr) {
    Data_type_listener type_listener(ctx->dataType(),
                                     ensure_array(column, "flags"));

    // char types can also include the CHARACTER SET clause, which we want
    // to keep separate
    column->set("dataType",
                type_listener.data_type_name + type_listener.data_type_params);

    if (!type_listener.charset_name.empty())
      column->set(
          "characterSet",
          type_listener.charset_name);  // Charset comes from data type,
                                        // collation from column attributes.
  }

  if (ctx->AS_SYMBOL() != nullptr) {
    // Only there for generated columns.
    column->set("generated", shcore::Value::True());
    column->set("expression", parsers::MySQLBaseLexer::sourceTextForContext(
                                  ctx->exprWithParentheses()->expr()));

    if (ctx->VIRTUAL_SYMBOL() != nullptr)
      column->set("generatedStorage", "VIRTUAL");
    if (ctx->STORED_SYMBOL() != nullptr)
      column->set("generatedStorage", "STORED");

    if (ctx->collate() != nullptr) {
      // Collation clause for generated columns.
      column->set("collate", ctx->collate()->collationName()->getText());
    }
  }
  ensure_array(m_table, "columns")->emplace_back(shcore::Value(column));
}

void Column_definition_listener::exitColumnAttribute(
    parsers::MySQLParser::ColumnAttributeContext *ctx) {
  if (ctx->nullLiteral() != nullptr) {
    column->set("isNotNull", (ctx->NOT_SYMBOL() != nullptr)
                                 ? shcore::Value::True()
                                 : shcore::Value::False());
    return;
  }

  if (ctx->collate() != nullptr) {
    column->set("collate", ctx->collate()->collationName()->getText());
    return;
  }

  if (ctx->checkConstraint() != nullptr) {
    if (ctx->constraintName() != nullptr &&
        ctx->constraintName()->CONSTRAINT_SYMBOL() != nullptr &&
        ctx->constraintName()->identifier() != nullptr) {
      column->set("checkName", parsers::MySQLBaseLexer::sourceTextForContext(
                                   ctx->constraintName()->identifier()));
    }

    column->set("check", parsers::MySQLBaseLexer::sourceTextForContext(
                             ctx->checkConstraint()->exprWithParentheses()));
    return;
  }

  if (ctx->constraintEnforcement() != nullptr &&
      ctx->constraintEnforcement()->ENFORCED_SYMBOL() != nullptr) {
    if (ctx->constraintEnforcement()->NOT_SYMBOL() != nullptr)
      column->set("checkEnforced", shcore::Value::False());
    else
      column->set("checkEnforced", shcore::Value::True());
    return;
  }

  if (ctx->visibility() != nullptr) {
    if (ctx->visibility()->VISIBLE_SYMBOL() != nullptr)
      column->set("invisible", shcore::Value::False());
    else if (ctx->visibility()->INVISIBLE_SYMBOL() != nullptr)
      column->set("invisible", shcore::Value::True());
    return;
  }

  if (ctx->value) {
    switch (ctx->value->getType()) {
      case parsers::MySQLLexer::DEFAULT_SYMBOL: {
        if (ctx->nowOrSignedLiteral() != nullptr) {
          column->set("defaultValue", parsers::MySQLBaseRecognizer::getText(
                                          ctx->nowOrSignedLiteral(), false));
        } else if (ctx->exprWithParentheses()) {
          // Expressions in parentheses.
          column->set("defaultValue", parsers::MySQLBaseRecognizer::getText(
                                          ctx->exprWithParentheses(), false));
        } else if (ctx->NOW_SYMBOL()) {
          column->set("defaultValue",
                      "NOW " + parsers::MySQLBaseRecognizer::getText(
                                   ctx->timeFunctionParameters(), false));
        } else {
          assert(0);
        }
        break;
      }

      case parsers::MySQLLexer::ON_SYMBOL: {
        // As mentioned above we combine DEFAULT NOW and ON UPDATE NOW into a
        // common default value.
        std::string newDefault = column->get_string("defaultValue");
        if (!newDefault.empty())
          newDefault += " ON UPDATE CURRENT_TIMESTAMP";
        else
          newDefault = "ON UPDATE CURRENT_TIMESTAMP";

        if (ctx->timeFunctionParameters() != nullptr)  // Additional precision.
          newDefault += parsers::MySQLBaseLexer::sourceTextForContext(
              ctx->timeFunctionParameters());

        column->set("defaultValue", std::move(newDefault));

        break;
      }

      case parsers::MySQLLexer::AUTO_INCREMENT_SYMBOL:
        column->set("autoIncrement", 1);
        break;

      case parsers::MySQLLexer::SERIAL_SYMBOL:  // SERIAL DEFAULT VALUE is an
        // alias for NOT NULL
        // AUTO_INCREMENT UNIQUE.
        break;
      case parsers::MySQLLexer::UNIQUE_SYMBOL: {
        if (ctx->DEFAULT_SYMBOL() != nullptr) {
          column->set("isNotNull", 1);
          column->set("autoIncrement", 1);
        }

        column->set("isUniqueKey", shcore::Value::True());
        break;
      }

      case parsers::MySQLLexer::PRIMARY_SYMBOL:
      case parsers::MySQLLexer::KEY_SYMBOL: {
        column->set("isPrimaryKey", shcore::Value::True());
        break;
      }

      case parsers::MySQLLexer::COMMENT_SYMBOL:
        column->set("comment", unquote_text_literal(ctx->textLiteral(),
                                                    m_no_backslash_escapes));
        break;

      case parsers::MySQLLexer::COLUMN_FORMAT_SYMBOL:
        column->set("columnFormat", ctx->columnFormat()->getText());
        break;

      case parsers::MySQLLexer::STORAGE_SYMBOL:
        column->set("storage", ctx->storageMedia()->getText());
        break;

      case parsers::MySQLLexer::ENGINE_ATTRIBUTE_SYMBOL:
        column->set(
            "engineAttribute",
            shcore::unquote_string(ctx->jsonAttribute()->getText(), '\''));
        break;

      case parsers::MySQLLexer::SECONDARY_ENGINE_ATTRIBUTE_SYMBOL:
        column->set(
            "secondaryEngineAttribute",
            shcore::unquote_string(ctx->jsonAttribute()->getText(), '\''));
        break;

      default:
        break;
    }
  }
}

void Column_definition_listener::exitReferences(
    parsers::MySQLParser::ReferencesContext *) {
  // This is a so called "inline references specification", which is not
  // supported by MySQL.

  // Throw them away just like in mysql...
}

Key_definition_listener::Key_definition_listener(tree::ParseTree *tree,
                                                 shcore::Dictionary_t table,
                                                 bool no_backslash_escapes)
    : m_table(table),
      m_current_index(shcore::make_dict()),
      m_no_backslash_escapes(no_backslash_escapes) {
  tree::ParseTreeWalker::DEFAULT.walk(this, tree);
}

void Key_definition_listener::exitTableConstraintDef(
    parsers::MySQLParser::TableConstraintDefContext *ctx) {
  std::string constraintName;

  // PKs, FKs, checks and UNIQUE indexes are types of CONSTRAINTs

  // It's possible for CONSTRAINT and specific object names to be given
  // - For UNIQUE keys: both are allowed, but index name takes precedence
  // - For PKs: constraint is allowed, but ignored
  // - For FKs: both are allowed, but name different things. The FK is named
  // after the CONSTRAINT name
  // - For checks: only the CONSTRAINT name is considered

  // This class will parse both constraints and also regular indexes

  if (ctx->checkConstraint()) {
    m_current_index->set(
        "expression",
        parsers::MySQLBaseLexer::sourceTextForContext(
            ctx->checkConstraint()->exprWithParentheses()->expr()));

    if (ctx->constraintEnforcement() != nullptr &&
        ctx->constraintEnforcement()->ENFORCED_SYMBOL() != nullptr) {
      m_current_index->set("enforced",
                           ctx->constraintEnforcement()->NOT_SYMBOL() != nullptr
                               ? shcore::Value::False()
                               : shcore::Value::True());
    }

    m_current_index->set(
        "name", ctx->constraintName() && ctx->constraintName()->identifier()
                    ? Identifier_listener(ctx->constraintName()->identifier())
                          .parts.back()
                    : "");
    ensure_array(m_table, "checks")
        ->emplace_back(shcore::Value(m_current_index));
    return;
  }

  switch (ctx->type->getType()) {
    case parsers::MySQLLexer::PRIMARY_SYMBOL:
      m_current_index->set("name", "PRIMARY");
      m_current_index->set("indexKind", "PRIMARY");

      parse_key_list(key_list_context(ctx), m_current_index);
      ensure_array(m_table, "indexes")
          ->emplace_back(shcore::Value(m_current_index));
      break;

    case parsers::MySQLLexer::UNIQUE_SYMBOL:
      if (ctx->indexNameAndType())
        m_current_index->set(
            "name", Identifier_listener(ctx->indexNameAndType()).parts.back());
      else if (ctx->constraintName() && ctx->constraintName()->identifier())
        m_current_index->set(
            "name", Identifier_listener(ctx->constraintName()->identifier())
                        .parts.back());
      else
        m_current_index->set("name", "");
      m_current_index->set("indexKind", "UNIQUE");

      parse_key_list(key_list_context(ctx), m_current_index);
      ensure_array(m_table, "indexes")
          ->emplace_back(shcore::Value(m_current_index));
      break;

    case parsers::MySQLLexer::INDEX_SYMBOL:
    case parsers::MySQLLexer::KEY_SYMBOL:
      if (ctx->indexNameAndType())
        m_current_index->set(
            "name", Identifier_listener(ctx->indexNameAndType()).parts.back());
      else
        m_current_index->set("name", "");

      m_current_index->set("indexKind",
                           format_index_type(ctx->type->getText()));
      parse_key_list(key_list_context(ctx), m_current_index);
      ensure_array(m_table, "indexes")
          ->emplace_back(shcore::Value(m_current_index));
      break;

    case parsers::MySQLLexer::FULLTEXT_SYMBOL:
    case parsers::MySQLLexer::SPATIAL_SYMBOL:
      if (ctx->indexName())
        m_current_index->set(
            "name", Identifier_listener(ctx->indexName()).parts.back());
      else
        m_current_index->set("name", "");

      m_current_index->set("indexKind",
                           format_index_type(ctx->type->getText()));
      parse_key_list(key_list_context(ctx), m_current_index);
      ensure_array(m_table, "indexes")
          ->emplace_back(shcore::Value(m_current_index));
      break;

    case parsers::MySQLLexer::FOREIGN_SYMBOL: {
      shcore::Dictionary_t fk(shcore::make_dict());
      if (ctx->constraintName() && ctx->constraintName()->identifier())
        fk->set("name", Identifier_listener(ctx->constraintName()->identifier())
                            .parts.back());
      else
        fk->set("name", "");

      if (ctx->indexName())
        fk->set("keyName", Identifier_listener(ctx->indexName()).parts.back());

      // List of columns in the FK.
      {
        Db_object_references references(
            fk, Db_object_references::Reference_type::Referencing);

        // Columns used in the FK might not have been parsed yet, so add the
        // column refs to our cache as well and resolve them when we are done
        // with the overall structure.
        references.target_identifier.name = m_table->get_string("name");
        references.table = m_table;

        column_names_from_key_list(ctx->keyList(), &references);

        shcore::Array_t list = shcore::make_array();
        for (const auto &col : references.column_names)
          list->emplace_back(shcore::Value(col));
        fk->set("referencingColumns", list);
      }

      Db_object_references references(
          fk, Db_object_references::Reference_type::Referenced);
      references.table = m_table;
      parse_references(ctx->references(), &references);

      if (!references.target_identifier.schema.empty()) {
        fk->set(
            "referencedTable",
            shcore::make_dict("ref:schema", references.target_identifier.schema,
                              "ref:name", references.target_identifier.name));
      } else {
        fk->set(
            "referencedTable",
            shcore::make_dict("ref:name", references.target_identifier.name));
      }

      shcore::Array_t list = shcore::make_array();
      for (const auto &col : references.column_names)
        list->emplace_back(shcore::Value(col));
      fk->set("referencedColumns", list);

      ensure_array(m_table, "foreignKeys")->emplace_back(shcore::Value(fk));
      break;
    }

    default:  // Anything else we ignore for now.
      break;
  }
}

void Key_definition_listener::exitIndexType(
    parsers::MySQLParser::IndexTypeContext *ctx) {
  m_current_index->set("indexType",
                       shcore::str_upper(ctx->algorithm->getText()));
}

void Key_definition_listener::exitCommonIndexOption(
    parsers::MySQLParser::CommonIndexOptionContext *ctx) {
  if (ctx->KEY_BLOCK_SIZE_SYMBOL() != nullptr)
    m_current_index->set(
        "keyBlockSize",
        static_cast<uint64_t>(std::stoull(ctx->ulong_number()->getText())));
  else if (ctx->COMMENT_SYMBOL() != nullptr)
    m_current_index->set(
        "comment",
        unquote_text_literal(ctx->textLiteral(), m_no_backslash_escapes));

  else if (ctx->visibility() != nullptr)
    m_current_index->set("visible",
                         (ctx->visibility()->VISIBLE_SYMBOL() != nullptr));
  else if (ctx->ENGINE_ATTRIBUTE_SYMBOL() != nullptr)
    m_current_index->set(
        "engineAttribute",
        parsers::MySQLBaseLexer::sourceTextForContext(ctx->jsonAttribute()));
  else if (ctx->SECONDARY_ENGINE_ATTRIBUTE_SYMBOL() != nullptr)
    m_current_index->set(
        "secondaryEngineAttribute",
        parsers::MySQLBaseLexer::sourceTextForContext(ctx->jsonAttribute()));
}

void Key_definition_listener::exitFulltextIndexOption(
    parsers::MySQLParser::FulltextIndexOptionContext *ctx) {
  if (ctx->WITH_SYMBOL() != nullptr)
    m_current_index->set("withParser", ctx->identifier()->getText());
}

//----------------- Object_listener
//-------------------------------------------------------------------------------------

Object_listener::Object_listener(shcore::Dictionary_t anObject,
                                 bool no_backslash_escapes)
    : m_object(std::move(anObject)),
      m_no_backslash_escapes(no_backslash_escapes) {}

//----------------- Data_type_listener
//-----------------------------------------------------------------------------------

Data_type_listener::Data_type_listener(tree::ParseTree *tree,
                                       shcore::Array_t flags)
    : m_flags(std::move(flags)) {
  tree::ParseTreeWalker::DEFAULT.walk(this, tree);
}

//----------------------------------------------------------------------------------------------------------------------

void Data_type_listener::exitDataType(
    parsers::MySQLParser::DataTypeContext *ctx) {
  // A type name can consist of up to 3 parts. Most however just have a single
  // part.
  size_t type = (ctx->nchar() != nullptr) ? ctx->nchar()->type->getType()
                                          : ctx->type->getType();
  std::string typeName = (ctx->nchar() != nullptr)
                             ? "NCHAR"
                             : shcore::str_upper(ctx->type->getText());
  switch (type) {
    case parsers::MySQLLexer::NATIONAL_SYMBOL:
      if (ctx->CHAR_SYMBOL() != nullptr) typeName += " CHAR";
      if (ctx->VARYING_SYMBOL() != nullptr) typeName += " VARYING";
      if (ctx->VARCHAR_SYMBOL() != nullptr) typeName += " VARCHAR";
      break;

    case parsers::MySQLLexer::CHAR_SYMBOL:
      if (ctx->VARYING_SYMBOL() != nullptr) typeName += " VARYING";
      break;

    case parsers::MySQLLexer::NCHAR_SYMBOL:
      if (ctx->VARCHAR_SYMBOL() != nullptr) typeName += " VARCHAR";
      if (ctx->VARYING_SYMBOL() != nullptr) typeName += " VARYING";
      break;

    case parsers::MySQLLexer::LONG_SYMBOL:
      if (ctx->VARBINARY_SYMBOL() != nullptr) typeName += " VARBINARY";
      if (ctx->CHAR_SYMBOL() != nullptr) typeName += " CHAR VARYING";
      if (ctx->VARCHAR_SYMBOL() != nullptr) typeName += " VARCHAR";
      break;

    default:
      break;
  }

  data_type_name = std::move(typeName);
}

//----------------------------------------------------------------------------------------------------------------------

void Data_type_listener::exitFieldLength(
    parsers::MySQLParser::FieldLengthContext *ctx) {
  if (ctx->DECIMAL_NUMBER() != nullptr) {
    length = std::stoi(ctx->DECIMAL_NUMBER()->getText());
    data_type_params += "(" + ctx->DECIMAL_NUMBER()->getText() + ")";
  } else {
    length = std::stoi(ctx->real_ulonglong_number()->getText());
    data_type_params += "(" + ctx->real_ulonglong_number()->getText() + ")";
  }
}

//----------------------------------------------------------------------------------------------------------------------

void Data_type_listener::exitPrecision(
    parsers::MySQLParser::PrecisionContext *ctx) {
  auto precision_ = ctx->INT_NUMBER(0)->getText();
  auto scale_ = ctx->INT_NUMBER(1)->getText();

  this->precision = std::stoi(precision_);
  this->scale = std::stoi(scale_);
  data_type_params += "(" + precision_ + "," + scale_ + ")";
}

//----------------------------------------------------------------------------------------------------------------------

static void add_flag(shcore::Array_t list, const std::string &flag) {
  if (std::find_if(list->begin(), list->end(), [&flag](const shcore::Value &v) {
        return v.as_string() == flag;
      }) == list->end())
    list->emplace_back(shcore::Value(flag));
}

void Data_type_listener::exitFieldOptions(
    parsers::MySQLParser::FieldOptionsContext *ctx) {
  if (!ctx->UNSIGNED_SYMBOL().empty()) {
    add_flag(m_flags, "UNSIGNED");
  }
  if (!ctx->SIGNED_SYMBOL().empty()) {
    add_flag(m_flags, "SIGNED");
  }
  if (!ctx->ZEROFILL_SYMBOL().empty()) {
    add_flag(m_flags, "ZEROFILL");
  }
}

//----------------------------------------------------------------------------------------------------------------------

void Data_type_listener::exitCharsetWithOptBinary(
    parsers::MySQLParser::CharsetWithOptBinaryContext *ctx) {
  std::string flag;
  bool insertBinary = false;

  if (ctx->ascii() != nullptr) {
    flag = "ASCII";
    insertBinary = ctx->ascii()->BINARY_SYMBOL() != nullptr;
  } else if (ctx->unicode() != nullptr) {
    flag = "UNICODE";
    insertBinary = ctx->unicode()->BINARY_SYMBOL() != nullptr;
  } else if (ctx->BYTE_SYMBOL() != nullptr) {
    flag = "BYTE";
  } else if (ctx->BINARY_SYMBOL() != nullptr || ctx->charset() != nullptr) {
    insertBinary = ctx->BINARY_SYMBOL() != nullptr;
  }

  if (!flag.empty()) add_flag(m_flags, flag);
  if (insertBinary) add_flag(m_flags, "BINARY");
}

//----------------------------------------------------------------------------------------------------------------------

void Data_type_listener::exitCharsetName(
    parsers::MySQLParser::CharsetNameContext *ctx) {
  charset_name = ctx->getText();
}

//----------------------------------------------------------------------------------------------------------------------

void Data_type_listener::exitTypeDatetimePrecision(
    parsers::MySQLParser::TypeDatetimePrecisionContext *ctx) {
  precision = std::stoi(ctx->INT_NUMBER()->getText());
  data_type_params += "(" + ctx->INT_NUMBER()->getText() + ")";
}

//----------------------------------------------------------------------------------------------------------------------

void Data_type_listener::exitStringList(
    parsers::MySQLParser::StringListContext *ctx) {
  std::string params;
  for (auto entry : ctx->textString()) {
    if (!params.empty()) params += ", ";
    params += entry->getText();
  }
  explicit_params = "(" + params + ")";

  data_type_params += explicit_params;
}

//----------------- Schema_listener
//-------------------------------------------------------------------------------------

Schema_listener::Schema_listener(tree::ParseTree *tree,
                                 shcore::Dictionary_t anObject)
    : Object_listener(anObject) {
  tree::ParseTreeWalker::DEFAULT.walk(this, tree);
}

//----------------------------------------------------------------------------------------------------------------------

void Schema_listener::exitCreateDatabase(
    parsers::MySQLParser::CreateDatabaseContext *ctx) {
  shcore::Dictionary_t schema = m_object;
  schema->set("name",
              parsers::MySQLBaseLexer::sourceTextForContext(ctx->schemaName()));
  ignore_if_exists = ctx->ifNotExists() != nullptr;
}

//----------------------------------------------------------------------------------------------------------------------

void Schema_listener::exitCharsetName(
    parsers::MySQLParser::CharsetNameContext *ctx) {
  shcore::Dictionary_t schema = m_object;
  std::string charsetName;
  if (ctx->DEFAULT_SYMBOL() != nullptr)
    charsetName = "default";
  else
    charsetName =
        shcore::str_lower(parsers::MySQLBaseLexer::sourceTextForContext(ctx));

  schema->set("defaultCharacterSet", std::move(charsetName));
}

//----------------------------------------------------------------------------------------------------------------------

void Schema_listener::exitCollationName(
    parsers::MySQLParser::CollationNameContext *ctx) {
  shcore::Dictionary_t schema = m_object;
  std::string collationName;
  if (ctx->DEFAULT_SYMBOL() == nullptr)
    collationName =
        shcore::str_lower(parsers::MySQLBaseLexer::sourceTextForContext(ctx));
  else
    collationName = "default";

  schema->set("defaultCollate", std::move(collationName));
}

//----------------------------------------------------------------------------------------------------------------------

void Schema_listener::exitDefaultEncryption(
    parsers::MySQLParser::DefaultEncryptionContext *ctx) {
  m_object->set(
      "defaultEncryption",
      shcore::str_caseeq(parsers::MySQLBaseLexer::sourceTextForContext(
                             ctx->textStringLiteral()),
                         "Y"));
}

//----------------- Table_listener
//--------------------------------------------------------------------------------------

Table_listener::Table_listener(tree::ParseTree *tree,
                               shcore::Dictionary_t table,
                               bool no_backslash_escapes)
    : Object_listener(table, no_backslash_escapes) {
  table->set("indexes", shcore::make_array());
  table->set("columns", shcore::make_array());
  table->set("foreignKeys", shcore::make_array());

  tree::ParseTreeWalker::DEFAULT.walk(this, tree);
}

//----------------------------------------------------------------------------------------------------------------------

void Table_listener::exitTableName(
    parsers::MySQLParser::TableNameContext *ctx) {
  shcore::Dictionary_t table = m_object;

  Identifier_listener listener(ctx);

  table->set("name", std::move(listener.parts.back()));
  if (listener.parts.size() > 1 && !listener.parts[0].empty()) {
    table->set("_schema", std::move(listener.parts[0]));
  }
}

//----------------------------------------------------------------------------------------------------------------------

void Table_listener::exitCreateTable(
    parsers::MySQLParser::CreateTableContext *ctx) {
  shcore::Dictionary_t table = m_object;

  table->set("isTemporary", ctx->TEMPORARY_SYMBOL() != nullptr);
  ignore_if_exists = ctx->ifNotExists() != nullptr;

  if (ctx->tableElementList()) {
    for (auto item : ctx->tableElementList()->tableElement()) {
      if (item->columnDefinition() != nullptr)
        Column_definition_listener(item->columnDefinition(), table,
                                   m_no_backslash_escapes);
      else
        Key_definition_listener(item->tableConstraintDef(), table,
                                m_no_backslash_escapes);
    }
  } else if (ctx->LIKE_SYMBOL() != nullptr && ctx->tableRef() != nullptr) {
    Identifier_listener listener(ctx->tableRef());

    if (listener.parts.size() > 1 && !listener.parts[0].empty())
      table->set("likeTableSchema", std::move(listener.parts[0]));
    table->set("likeTable", std::move(listener.parts.back()));
  }
}

//----------------------------------------------------------------------------------------------------------------------

void Table_listener::exitPartitionClause(
    parsers::MySQLParser::PartitionClauseContext *ctx) {
  shcore::Dictionary_t table = m_object;

  table->set("partitionClauseSql",
             parsers::MySQLBaseLexer::sourceTextForContext(ctx));
}

//----------------------------------------------------------------------------------------------------------------------

void Table_listener::exitDuplicateAsQe(
    parsers::MySQLParser::DuplicateAsQeContext *ctx) {
  if (ctx->REPLACE_SYMBOL()) {
    m_object->set("asQueryOnDuplicate", "REPLACE");
  } else if (ctx->IGNORE_SYMBOL()) {
    m_object->set("asQueryOnDuplicate", "IGNORE");
  }
  m_object->set("asQuery", parsers::MySQLBaseLexer::sourceTextForContext(
                               ctx->asCreateQueryExpression()
                                   ->queryExpressionWithOptLockingClauses()));
}

//----------------------------------------------------------------------------------------------------------------------

void Table_listener::exitCreateTableOptions(
    parsers::MySQLParser::CreateTableOptionsContext *ctx) {
  for (auto option : ctx->createTableOption()) {
    Create_table_option_listener(option, m_object, m_no_backslash_escapes);
  }
}

//-------------------------------------------------------------------------------

Create_table_option_listener::Create_table_option_listener(
    antlr4::tree::ParseTree *tree, shcore::Dictionary_t table,
    bool no_backslash_escapes)
    : Object_listener(table, no_backslash_escapes) {
  tree::ParseTreeWalker::DEFAULT.walk(this, tree);
}

//-------------------------------------------------------------------------------

void Create_table_option_listener::exitCreateTableOption(
    parsers::MySQLParser::CreateTableOptionContext *option) {
  if (option->option == nullptr) {
    // Collation/Charset handling.
    if (option->defaultCollation() != nullptr) {
      m_object->set("defaultCollate",
                    parsers::MySQLBaseLexer::sourceTextForContext(
                        option->defaultCollation()->collationName()));
    }
    if (option->defaultCharset() != nullptr) {
      m_object->set("defaultCharacterSet",
                    parsers::MySQLBaseLexer::sourceTextForContext(
                        option->defaultCharset()->charsetName()));
    }
    if (option->tsOptionAutoextendSize() != nullptr) {
      m_object->set("autoExtendSize",
                    option->tsOptionAutoextendSize()->sizeNumber()->getText());
    }
    return;
  }

  switch (option->option->getType()) {
    case parsers::MySQLLexer::AUTO_INCREMENT_SYMBOL:
      m_object->set("autoIncrement", option->ulonglongNumber()->getText());
      break;

    case parsers::MySQLLexer::AVG_ROW_LENGTH_SYMBOL:
      m_object->set("avgRowLength", option->ulonglongNumber()->getText());
      break;

    case parsers::MySQLLexer::CHECKSUM_SYMBOL:
    case parsers::MySQLLexer::TABLE_CHECKSUM_SYMBOL:
      m_object->set("checksum", static_cast<uint64_t>(std::stoull(
                                    option->ulong_number()->getText())));
      break;

    case parsers::MySQLLexer::COMMENT_SYMBOL:
      m_object->set("comment",
                    unquote_text_string_literal(option->textStringLiteral(),
                                                m_no_backslash_escapes));
      break;

    case parsers::MySQLLexer::COMPRESSION_SYMBOL:
      m_object->set(
          "compression",
          parsers::MySQLBaseLexer::sourceTextForContext(option->textString()));
      break;

    case parsers::MySQLLexer::CONNECTION_SYMBOL:
      m_object->set("connection", parsers::MySQLBaseLexer::sourceTextForContext(
                                      option->textString()));
      break;

    case parsers::MySQLLexer::DATA_SYMBOL:
      m_object->set(
          "dataDirectory",
          parsers::MySQLBaseLexer::sourceTextForContext(option->textString()));
      break;

    case parsers::MySQLLexer::INDEX_SYMBOL:
      m_object->set(
          "indexDirectory",
          parsers::MySQLBaseLexer::sourceTextForContext(option->textString()));
      break;

    case parsers::MySQLLexer::DELAY_KEY_WRITE_SYMBOL:
      m_object->set("delayKeyWrite", static_cast<uint64_t>(std::stoull(
                                         option->ulong_number()->getText())));
      break;

    case parsers::MySQLLexer::ENCRYPTION_SYMBOL:
      m_object->set("encryption", parsers::MySQLBaseLexer::sourceTextForContext(
                                      option->textString()));
      break;

    case parsers::MySQLLexer::ENGINE_SYMBOL:
    case parsers::MySQLLexer::TYPE_SYMBOL:
      m_object->set("engine", option->engineRef()->getText());
      break;

    case parsers::MySQLLexer::ENGINE_ATTRIBUTE_SYMBOL:
      m_object->set("engineAttribute",
                    parsers::MySQLBaseLexer::sourceTextForContext(
                        option->jsonAttribute()));
      break;

    case parsers::MySQLLexer::INSERT_METHOD_SYMBOL:
      m_object->set("insertMethod", option->method->getText());
      break;

    case parsers::MySQLLexer::KEY_BLOCK_SIZE_SYMBOL:
      m_object->set("keyBlockSize", option->ulonglongNumber()->getText());
      break;

    case parsers::MySQLLexer::MAX_ROWS_SYMBOL:
      m_object->set("maxRows", option->ulonglongNumber()->getText());
      break;

    case parsers::MySQLLexer::MIN_ROWS_SYMBOL:
      m_object->set("minRows", option->ulonglongNumber()->getText());
      break;

    case parsers::MySQLLexer::PASSWORD_SYMBOL:
      m_object->set("password", parsers::MySQLBaseLexer::sourceTextForContext(
                                    option->textStringLiteral()));
      break;

    case parsers::MySQLLexer::PACK_KEYS_SYMBOL:
      if (option->ternaryOption()->DEFAULT_SYMBOL() != nullptr)
        m_object->set("packKeys",
                      option->ternaryOption()->DEFAULT_SYMBOL()->getText());
      else
        m_object->set("packKeys",
                      option->ternaryOption()->ulong_number()->getText());
      break;

    case parsers::MySQLLexer::SECONDARY_ENGINE_SYMBOL:
      m_object->set("secondaryEngine", option->textOrIdentifier()->getText());
      break;

    case parsers::MySQLLexer::SECONDARY_ENGINE_ATTRIBUTE_SYMBOL:
      m_object->set("secondaryEngineAttribute",
                    parsers::MySQLBaseLexer::sourceTextForContext(
                        option->jsonAttribute()));
      break;

    case parsers::MySQLLexer::STATS_AUTO_RECALC_SYMBOL:
      if (option->ternaryOption()->DEFAULT_SYMBOL() != nullptr)
        m_object->set("statsAutoRecalc",
                      option->ternaryOption()->DEFAULT_SYMBOL()->getText());
      else
        m_object->set("statsAutoRecalc",
                      option->ternaryOption()->ulong_number()->getText());
      break;

    case parsers::MySQLLexer::STATS_PERSISTENT_SYMBOL:
      if (option->ternaryOption()->DEFAULT_SYMBOL() != nullptr)
        m_object->set("statsPersistent",
                      option->ternaryOption()->DEFAULT_SYMBOL()->getText());
      else
        m_object->set("statsPersistent",
                      option->ternaryOption()->ulong_number()->getText());
      break;

    case parsers::MySQLLexer::STATS_SAMPLE_PAGES_SYMBOL:
      // Note: we denote the default as value 0 (like it is done in the
      // server).
      if (option->ternaryOption()->DEFAULT_SYMBOL() != nullptr)
        m_object->set("statsSamplePages", 0);
      else
        m_object->set("statsSamplePages",
                      static_cast<uint64_t>(std::stoull(
                          option->ternaryOption()->ulong_number()->getText())));
      break;

    case parsers::MySQLLexer::ROW_FORMAT_SYMBOL:
      m_object->set("rowFormat", option->format->getText());
      break;

    case parsers::MySQLLexer::UNION_SYMBOL:  // Only for merge engine.
      break;

    case parsers::MySQLLexer::TABLESPACE_SYMBOL:
      m_object->set("tablespace", option->identifier()->getText());
      break;

    case parsers::MySQLLexer::STORAGE_SYMBOL:
      m_object->set("tablespaceStorage",
                    option->DISK_SYMBOL() ? "DISK" : "MEMORY");
      break;

    default:
      break;
  }
}

//----------------- Logfile_group_listener
//-------------------------------------------------------------------------------

Logfile_group_listener::Logfile_group_listener(tree::ParseTree *tree,
                                               shcore::Dictionary_t anObject,
                                               bool no_backslash_escapes)
    : Object_listener(anObject, no_backslash_escapes) {
  tree::ParseTreeWalker::DEFAULT.walk(this, tree);
}

//----------------------------------------------------------------------------------------------------------------------

void Logfile_group_listener::exitCreateLogfileGroup(
    parsers::MySQLParser::CreateLogfileGroupContext *ctx) {
  Identifier_listener listener(ctx->logfileGroupName());

  shcore::Dictionary_t group = m_object;
  group->set("name", listener.parts[0]);

  group->set("undoFile",
             parsers::MySQLBaseLexer::sourceTextForContext(ctx->textLiteral()));
}

//----------------------------------------------------------------------------------------------------------------------

void Logfile_group_listener::exitTsOptionInitialSize(
    parsers::MySQLParser::TsOptionInitialSizeContext *ctx) {
  shcore::Dictionary_t group = m_object;
  group->set("initialSize",
             static_cast<uint64_t>(number_value(ctx->sizeNumber()->getText())));
}

//----------------------------------------------------------------------------------------------------------------------

void Logfile_group_listener::exitTsOptionUndoRedoBufferSize(
    parsers::MySQLParser::TsOptionUndoRedoBufferSizeContext *ctx) {
  shcore::Dictionary_t group = m_object;
  if (ctx->UNDO_BUFFER_SIZE_SYMBOL() != nullptr)
    group->set(
        "undoBufferSize",
        static_cast<uint64_t>(number_value(ctx->sizeNumber()->getText())));
  else
    group->set(
        "redoBufferSize",
        static_cast<uint64_t>(number_value(ctx->sizeNumber()->getText())));
}

//----------------------------------------------------------------------------------------------------------------------

void Logfile_group_listener::exitTsOptionNodegroup(
    parsers::MySQLParser::TsOptionNodegroupContext *ctx) {
  shcore::Dictionary_t group = m_object;
  group->set(
      "nodeGroupId",
      static_cast<uint64_t>(number_value(ctx->real_ulong_number()->getText())));
}

//----------------------------------------------------------------------------------------------------------------------

void Logfile_group_listener::exitTsOptionEngine(
    parsers::MySQLParser::TsOptionEngineContext *ctx) {
  shcore::Dictionary_t group = m_object;
  group->set("engine", unquoteIdentifier(ctx->engineRef()->getText()));
}

//----------------------------------------------------------------------------------------------------------------------

void Logfile_group_listener::exitTsOptionWait(
    parsers::MySQLParser::TsOptionWaitContext *ctx) {
  shcore::Dictionary_t group = m_object;
  group->set("wait", ctx->WAIT_SYMBOL() != nullptr);
}

//----------------------------------------------------------------------------------------------------------------------

void Logfile_group_listener::exitTsOptionComment(
    parsers::MySQLParser::TsOptionCommentContext *ctx) {
  shcore::Dictionary_t group = m_object;
  group->set("comment",
             unquote_text_literal(ctx->textLiteral(), m_no_backslash_escapes));
}

//----------------- Library_listener
//------------------------------------------------------------------------------------

Library_listener::Library_listener(tree::ParseTree *tree,
                                   shcore::Dictionary_t anObject)
    : Object_listener(anObject) {
  tree::ParseTreeWalker::DEFAULT.walk(this, tree);
}

//----------------------------------------------------------------------------------------------------------------------

void Library_listener::exitCreateLibrary(
    parsers::MySQLParser::CreateLibraryContext *ctx) {
  Identifier_listener listener(ctx->libraryName());

  if (listener.parts.size() > 1 && !listener.parts[0].empty())
    m_object->set("_schema", listener.parts[0]);

  m_object->set("name", listener.parts.back());
  m_object->set("language",
                Identifier_listener(ctx->identifier()).parts.back());
  m_object->set("sqlBody",
                "AS " + parsers::MySQLBaseLexer::sourceTextForContext(
                            ctx->routineString(), true));
  ignore_if_exists = ctx->ifNotExists() != nullptr;
}

//----------------- Routine_listener
//------------------------------------------------------------------------------------

Routine_listener::Routine_listener(tree::ParseTree *tree,
                                   shcore::Dictionary_t routine,
                                   bool no_backslash_escapes)
    : Object_listener(routine, no_backslash_escapes) {
  // routine->set("params", shcore::make_array());

  tree::ParseTreeWalker::DEFAULT.walk(this, tree);
}

//----------------------------------------------------------------------------------------------------------------------

void Routine_listener::exitDefinerClause(
    parsers::MySQLParser::DefinerClauseContext *ctx) {
  shcore::Dictionary_t routine = m_object;
  routine->set("definer", parsers::MySQLBaseLexer::sourceTextForContext(
                              ctx->user(), true));
}

//----------------------------------------------------------------------------------------------------------------------

void Routine_listener::enterCreateProcedure(
    parsers::MySQLParser::CreateProcedureContext *ctx) {
  readRoutineName(ctx->procedureName());
}

//----------------------------------------------------------------------------------------------------------------------

void Routine_listener::exitCreateProcedure(
    parsers::MySQLParser::CreateProcedureContext *ctx) {
  shcore::Dictionary_t routine = m_object;

  std::string signature;
  for (const auto &param : ctx->procedureParameter()) {
    if (!signature.empty()) signature += ", ";
    signature += parsers::MySQLBaseLexer::sourceTextForContext(param);
  }
  routine->set("signature", std::move(signature));

  routine->set("sqlBody", parsers::MySQLBaseLexer::sourceTextForContext(
                              ctx->storedRoutineBody(), true));
}

//----------------------------------------------------------------------------------------------------------------------

void Routine_listener::enterCreateFunction(
    parsers::MySQLParser::CreateFunctionContext *ctx) {
  readRoutineName(ctx->functionName());
}

//----------------------------------------------------------------------------------------------------------------------

void Routine_listener::exitCreateFunction(
    parsers::MySQLParser::CreateFunctionContext *ctx) {
  shcore::Dictionary_t routine = m_object;

  routine->set("returnDataType", parsers::MySQLBaseLexer::sourceTextForContext(
                                     ctx->typeWithOptCollate()));

  std::string signature;
  for (const auto &param : ctx->functionParameter()) {
    if (!signature.empty()) signature += ", ";
    signature += parsers::MySQLBaseLexer::sourceTextForContext(param);
  }
  routine->set("signature", std::move(signature));

  routine->set("sqlBody", parsers::MySQLBaseLexer::sourceTextForContext(
                              ctx->storedRoutineBody(), true));
}

//----------------------------------------------------------------------------------------------------------------------

void Routine_listener::exitCreateUdf(
    parsers::MySQLParser::CreateUdfContext *ctx) {
  shcore::Dictionary_t routine = m_object;

  routine->set("routineType", "udf");
  readRoutineName(ctx->udfName());

  routine->set("returnDataType", ctx->type->getText());

  // SONAME is currently ignored.
}

//----------------------------------------------------------------------------------------------------------------------

void Routine_listener::exitProcedureParameter(
    parsers::MySQLParser::ProcedureParameterContext *ctx) {
  if (ctx->type != nullptr)
    m_current_parameter->set("paramType", ctx->type->getText());
  else
    m_current_parameter->set("paramType", "IN");
}

//----------------------------------------------------------------------------------------------------------------------

void Routine_listener::enterFunctionParameter(
    parsers::MySQLParser::FunctionParameterContext *) {
  shcore::Dictionary_t routine = m_object;

  m_current_parameter = shcore::make_dict();
}

//----------------------------------------------------------------------------------------------------------------------

void Routine_listener::exitFunctionParameter(
    parsers::MySQLParser::FunctionParameterContext *ctx) {
  // Called for both functions and procedures.
  m_current_parameter->set(
      "name",
      parsers::MySQLBaseLexer::sourceTextForContext(ctx->parameterName()));
  m_current_parameter->set(
      "dataType",
      parsers::MySQLBaseLexer::sourceTextForContext(ctx->typeWithOptCollate()));
}

//----------------------------------------------------------------------------------------------------------------------

void Routine_listener::exitRoutineCreateOption(
    parsers::MySQLParser::RoutineCreateOptionContext *ctx) {
  if (ctx->DETERMINISTIC_SYMBOL()) {
    if (ctx->NOT_SYMBOL())
      m_object->set("deterministic", shcore::Value::False());
    else
      m_object->set("deterministic", shcore::Value::True());
  } else if (ctx->libraryList()) {
    addRoutineLibraries(ctx->libraryList());
  }
}

//----------------------------------------------------------------------------------------------------------------------

void Routine_listener::exitRoutineOption(
    parsers::MySQLParser::RoutineOptionContext *ctx) {
  shcore::Dictionary_t routine = m_object;

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

//----------------------------------------------------------------------------------------------------------------------

void Routine_listener::addRoutineLibraries(
    parsers::MySQLParser::LibraryListContext *ctx) {
  const auto libraries = ensure_array(m_object, "libraries");
  const auto current_schema = m_object->get_string("_schema");

  for (const auto library : ctx->libraryNameWithAlias()) {
    libraries->emplace_back(
        shcore::Value(read_library_reference(library, current_schema)));
  }
}

//----------------------------------------------------------------------------------------------------------------------

void Routine_listener::readRoutineName(ParserRuleContext *ctx) {
  shcore::Dictionary_t routine = m_object;

  Identifier_listener listener(ctx);
  routine->set("name", listener.parts.back());

  if (listener.parts.size() > 1 && !listener.parts[0].empty())
    routine->set("_schema", listener.parts[0]);
}

//----------------- Index_listener
//--------------------------------------------------------------------------------------

Index_listener::Index_listener(tree::ParseTree *tree,
                               shcore::Dictionary_t index,
                               bool no_backslash_escapes)
    : Object_listener(index, no_backslash_escapes) {
  tree::ParseTreeWalker::DEFAULT.walk(this, tree);
}

//----------------------------------------------------------------------------------------------------------------------

void Index_listener::exitCreateIndex(
    parsers::MySQLParser::CreateIndexContext *ctx) {
  shcore::Dictionary_t index = m_object;

  switch (ctx->type->getType()) {
    case parsers::MySQLLexer::INDEX_SYMBOL:
      if (ctx->UNIQUE_SYMBOL() != nullptr) {
        index->set("indexKind", "UNIQUE");
      } else {
        index->set("indexKind", format_index_type(ctx->type->getText()));
      }
      break;

    case parsers::MySQLLexer::FULLTEXT_SYMBOL:
    case parsers::MySQLLexer::SPATIAL_SYMBOL:
      index->set("indexKind", format_index_type(ctx->type->getText()));
      break;

    default:
      break;
  }

  index->set("name", unquoteIdentifier(ctx->indexName()->getText()));
}

//----------------------------------------------------------------------------------------------------------------------

void Index_listener::exitIndexType(
    parsers::MySQLParser::IndexTypeContext *ctx) {
  shcore::Dictionary_t index = m_object;
  index->set("indexType", ctx->algorithm->getText());
}

//----------------------------------------------------------------------------------------------------------------------

void Index_listener::exitCreateIndexTarget(
    parsers::MySQLParser::CreateIndexTargetContext *ctx) {
  shcore::Dictionary_t index = m_object;

  Identifier_listener listener(ctx->tableRef());

  if (listener.parts.size() > 1 && !listener.parts[0].empty())
    index->set("_schema", listener.parts[0]);
  index->set("_table", listener.parts.back());

  parse_key_list(ctx->keyListWithExpression(), index);
}

//----------------------------------------------------------------------------------------------------------------------

void Index_listener::exitCommonIndexOption(
    parsers::MySQLParser::CommonIndexOptionContext *ctx) {
  shcore::Dictionary_t index = m_object;

  if (ctx->KEY_BLOCK_SIZE_SYMBOL() != nullptr)
    index->set(
        "keyBlockSize",
        static_cast<uint64_t>(std::stoull(ctx->ulong_number()->getText())));
  else if (ctx->COMMENT_SYMBOL() != nullptr)
    index->set("comment", unquote_text_literal(ctx->textLiteral(),
                                               m_no_backslash_escapes));
  else if (ctx->visibility() != nullptr)
    index->set("visible", (ctx->visibility()->VISIBLE_SYMBOL() != nullptr));
  else if (ctx->ENGINE_ATTRIBUTE_SYMBOL() != nullptr)
    index->set("engineAttribute", parsers::MySQLBaseLexer::sourceTextForContext(
                                      ctx->jsonAttribute()));
  else if (ctx->SECONDARY_ENGINE_ATTRIBUTE_SYMBOL() != nullptr)
    index->set(
        "secondaryEngineAttribute",
        parsers::MySQLBaseLexer::sourceTextForContext(ctx->jsonAttribute()));
}

//----------------------------------------------------------------------------------------------------------------------

void Index_listener::exitFulltextIndexOption(
    parsers::MySQLParser::FulltextIndexOptionContext *ctx) {
  shcore::Dictionary_t index = m_object;

  if (ctx->WITH_SYMBOL() != nullptr)
    index->set("withParser", ctx->identifier()->getText());
}

//----------------------------------------------------------------------------------------------------------------------

void Index_listener::exitAlterAlgorithmOption(
    parsers::MySQLParser::AlterAlgorithmOptionContext *ctx) {
  shcore::Dictionary_t index = m_object;

  if (ctx->DEFAULT_SYMBOL() != nullptr) {
    index->set("algorithm", "DEFAULT");
  } else {
    // The algorithm can be any text, but allowed are only a small number of
    // values.
    std::string algorithm = shcore::str_upper(ctx->identifier()->getText());
    if (algorithm == "INPLACE" || algorithm == "COPY")
      index->set("algorithm", std::move(algorithm));
  }
}

//----------------------------------------------------------------------------------------------------------------------

void Index_listener::exitAlterLockOption(
    parsers::MySQLParser::AlterLockOptionContext *ctx) {
  shcore::Dictionary_t index = m_object;

  if (ctx->DEFAULT_SYMBOL() != nullptr) {
    index->set("lockOption", "DEFAULT");
  } else {
    // The lock type can be any text, but allowed are only a small number of
    // values.
    std::string lock = shcore::str_upper(ctx->identifier()->getText());
    if (lock == "NONE" || lock == "SHARED" || lock == "EXCLUSIVE")
      index->set("lockOption", std::move(lock));
  }
}

//----------------- Trigger_listener
//------------------------------------------------------------------------------------

Trigger_listener::Trigger_listener(tree::ParseTree *tree,
                                   shcore::Dictionary_t trigger)
    : Object_listener(trigger) {
  tree::ParseTreeWalker::DEFAULT.walk(this, tree);
}

//----------------------------------------------------------------------------------------------------------------------

void Trigger_listener::exitDefinerClause(
    parsers::MySQLParser::DefinerClauseContext *ctx) {
  shcore::Dictionary_t trigger = m_object;

  trigger->set("definer", parsers::MySQLBaseLexer::sourceTextForContext(
                              ctx->user(), true));
}

//----------------------------------------------------------------------------------------------------------------------

void Trigger_listener::exitCreateTrigger(
    parsers::MySQLParser::CreateTriggerContext *ctx) {
  shcore::Dictionary_t trigger = m_object;

  Identifier_listener listener(ctx->triggerName());

  // We store triggers relative to the tables they act on, so we ignore here any
  // qualifying schema.
  trigger->set("name", listener.parts.back());

  trigger->set("time", ctx->timing->getText());
  trigger->set("event", ctx->event->getText());

  trigger->set("sqlBody", parsers::MySQLBaseLexer::sourceTextForContext(
                              ctx->compoundStatement(), true));

  // Trigger table referencing is a bit different than for other objects because
  // we need the table now to add the trigger to it. We cannot defer that to the
  // resolveReferences() call. This has the implication that we can only work
  // with tables we have found so far.
  listener.parts.clear();
  tree::ParseTreeWalker::DEFAULT.walk(&listener, ctx->tableRef());

  if (listener.parts.size() > 1 && !listener.parts[0].empty())
    trigger->set("_schema", listener.parts[0]);
  trigger->set("_table", listener.parts.back());
}

//----------------------------------------------------------------------------------------------------------------------

void Trigger_listener::exitTriggerFollowsPrecedesClause(
    parsers::MySQLParser::TriggerFollowsPrecedesClauseContext *ctx) {
  shcore::Dictionary_t trigger = m_object;

  if (ctx->FOLLOWS_SYMBOL())
    trigger->set(
        "followsTrigger",
        parsers::MySQLBaseLexer::sourceTextForContext(ctx->textOrIdentifier()));
  else
    trigger->set(
        "precedesTrigger",
        parsers::MySQLBaseLexer::sourceTextForContext(ctx->textOrIdentifier()));
}

//----------------- View_listener
//---------------------------------------------------------------------------------------

View_listener::View_listener(tree::ParseTree *tree,
                             shcore::Dictionary_t anObject)
    : Object_listener(anObject) {
  tree::ParseTreeWalker::DEFAULT.walk(this, tree);
}

//----------------------------------------------------------------------------------------------------------------------

void View_listener::exitCreateView(
    parsers::MySQLParser::CreateViewContext *ctx) {
  shcore::Dictionary_t view = m_object;

  Identifier_listener listener(ctx->viewName());
  view->set("name", listener.parts.back());

  if (listener.parts.size() > 1 && !listener.parts[0].empty())
    view->set("_schema", listener.parts[0]);
}

//----------------------------------------------------------------------------------------------------------------------

void View_listener::exitViewCheckOption(
    parsers::MySQLParser::ViewCheckOptionContext *ctx) {
  shcore::Dictionary_t view = m_object;
  std::string check;
  if (ctx->LOCAL_SYMBOL())
    check = "LOCAL";
  else if (ctx->CASCADED_SYMBOL())
    check = "CASCADED";
  view->set("checkOption", check);
}

//----------------------------------------------------------------------------------------------------------------------

void View_listener::exitViewAlgorithm(
    parsers::MySQLParser::ViewAlgorithmContext *ctx) {
  shcore::Dictionary_t view = m_object;

  switch (ctx->algorithm->getType()) {
    case parsers::MySQLLexer::MERGE_SYMBOL:
      view->set("algorithm", "MERGE");
      break;
    case parsers::MySQLLexer::TEMPTABLE_SYMBOL:
      view->set("algorithm", "TEMPTABLE");
      break;
    default:
      view->set("algorithm", "UNDEFINED");
      break;
  }
}

//----------------------------------------------------------------------------------------------------------------------

void View_listener::exitDefinerClause(
    parsers::MySQLParser::DefinerClauseContext *ctx) {
  shcore::Dictionary_t view = m_object;
  view->set("definer",
            parsers::MySQLBaseLexer::sourceTextForContext(ctx->user(), true));
}

//----------------------------------------------------------------------------------------------------------------------

void View_listener::exitViewQueryBlock(
    parsers::MySQLParser::ViewQueryBlockContext *ctx) {
  shcore::Dictionary_t view = m_object;

  view->set("sqlBody", parsers::MySQLBaseLexer::sourceTextForContext(
                           ctx->queryExpressionWithOptLockingClauses(), true));
}

//-------------------------------------------------------------------------------------

void View_listener::exitViewSuid(parsers::MySQLParser::ViewSuidContext *ctx) {
  shcore::Dictionary_t view = m_object;
  if (ctx->INVOKER_SYMBOL() != nullptr)
    view->set("sqlSecurity", "INVOKER");
  else if (ctx->DEFINER_SYMBOL() != nullptr)
    view->set("sqlSecurity", "DEFINER");
  else
    assert(0);
}

//-------------------------------------------------------------------------------------

void View_listener::exitColumnInternalRefList(
    parsers::MySQLParser::ColumnInternalRefListContext *ctx) {
  shcore::Dictionary_t view = m_object;
  view->set("columnList",
            parsers::MySQLBaseLexer::sourceTextForContext(ctx, true));
}

//----------------- Server_listener
//-------------------------------------------------------------------------------------

Server_listener::Server_listener(tree::ParseTree *tree,
                                 shcore::Dictionary_t anObject,
                                 bool no_backslash_escapes)
    : Object_listener(anObject, no_backslash_escapes) {
  tree::ParseTreeWalker::DEFAULT.walk(this, tree);
}

//----------------------------------------------------------------------------------------------------------------------

void Server_listener::exitCreateServer(
    parsers::MySQLParser::CreateServerContext *ctx) {
  shcore::Dictionary_t server = m_object;

  Identifier_listener listener(ctx->serverName());
  server->set("name", listener.parts.back());

  server->set("wrapperName",
              unquoteIdentifier(ctx->textOrIdentifier()->getText()));
}

//----------------------------------------------------------------------------------------------------------------------

void Server_listener::exitServerOption(
    parsers::MySQLParser::ServerOptionContext *ctx) {
  shcore::Dictionary_t server = m_object;

  switch (ctx->option->getType()) {
    case parsers::MySQLLexer::HOST_SYMBOL:
      server->set("host", unquote_text_literal(ctx->textLiteral(),
                                               m_no_backslash_escapes));
      break;
    case parsers::MySQLLexer::DATABASE_SYMBOL:
      server->set("schema", unquote_text_literal(ctx->textLiteral(),
                                                 m_no_backslash_escapes));
      break;
    case parsers::MySQLLexer::USER_SYMBOL:
      server->set("user", unquote_text_literal(ctx->textLiteral(),
                                               m_no_backslash_escapes));
      break;
    case parsers::MySQLLexer::PASSWORD_SYMBOL:
      server->set("password", unquote_text_literal(ctx->textLiteral(),
                                                   m_no_backslash_escapes));
      break;
    case parsers::MySQLLexer::SOCKET_SYMBOL:
      server->set("socket", unquote_text_literal(ctx->textLiteral(),
                                                 m_no_backslash_escapes));
      break;
    case parsers::MySQLLexer::OWNER_SYMBOL:
      server->set("ownerUser", unquote_text_literal(ctx->textLiteral(),
                                                    m_no_backslash_escapes));
      break;
    case parsers::MySQLLexer::PORT_SYMBOL:
      server->set("port", ctx->ulong_number()->getText());
      break;
  }
}

//----------------- Tablespace_listener
//---------------------------------------------------------------------------------

Tablespace_listener::Tablespace_listener(tree::ParseTree *tree,
                                         shcore::Dictionary_t anObject,
                                         bool no_backslash_escapes)
    : Object_listener(anObject, no_backslash_escapes) {
  tree::ParseTreeWalker::DEFAULT.walk(this, tree);
}

//----------------------------------------------------------------------------------------------------------------------

void Tablespace_listener::exitCreateTablespace(
    parsers::MySQLParser::CreateTablespaceContext *ctx) {
  shcore::Dictionary_t tablespace = m_object;

  Identifier_listener listener(ctx->tablespaceName());
  tablespace->set("name", listener.parts.back());
}

//----------------------------------------------------------------------------------------------------------------------

void Tablespace_listener::exitLogfileGroupRef(
    parsers::MySQLParser::LogfileGroupRefContext *ctx) {
  shcore::Dictionary_t tablespace = m_object;

  tablespace->set("logFileGroup", unquoteIdentifier(ctx->getText()));
}

//----------------------------------------------------------------------------------------------------------------------

void Tablespace_listener::exitTsDataFile(
    parsers::MySQLParser::TsDataFileContext *ctx) {
  shcore::Dictionary_t tablespace = m_object;
  tablespace->set("dataFile", unquote_text_literal(ctx->textLiteral(),
                                                   m_no_backslash_escapes));
}

//----------------------------------------------------------------------------------------------------------------------

void Tablespace_listener::exitTsOptionInitialSize(
    parsers::MySQLParser::TsOptionInitialSizeContext *ctx) {
  shcore::Dictionary_t tablespace = m_object;
  tablespace->set(
      "initialSize",
      static_cast<uint64_t>(number_value(ctx->sizeNumber()->getText())));
}

//----------------------------------------------------------------------------------------------------------------------

void Tablespace_listener::exitTsOptionAutoextendSize(
    parsers::MySQLParser::TsOptionAutoextendSizeContext *ctx) {
  shcore::Dictionary_t tablespace = m_object;
  tablespace->set(
      "autoExtendSize",
      static_cast<uint64_t>(number_value(ctx->sizeNumber()->getText())));
}

//----------------------------------------------------------------------------------------------------------------------

void Tablespace_listener::exitTsOptionMaxSize(
    parsers::MySQLParser::TsOptionMaxSizeContext *ctx) {
  shcore::Dictionary_t tablespace = m_object;
  tablespace->set("maxSize", static_cast<uint64_t>(
                                 number_value(ctx->sizeNumber()->getText())));
}

//----------------------------------------------------------------------------------------------------------------------

void Tablespace_listener::exitTsOptionExtentSize(
    parsers::MySQLParser::TsOptionExtentSizeContext *ctx) {
  shcore::Dictionary_t tablespace = m_object;
  tablespace->set(
      "extentSize",
      static_cast<uint64_t>(number_value(ctx->sizeNumber()->getText())));
}

//----------------------------------------------------------------------------------------------------------------------

void Tablespace_listener::exitTsOptionNodegroup(
    parsers::MySQLParser::TsOptionNodegroupContext *ctx) {
  shcore::Dictionary_t tablespace = m_object;

  // An integer or hex number (no suffix).
  tablespace->set(
      "nodeGroupId",
      static_cast<uint64_t>(number_value(ctx->real_ulong_number()->getText())));
}

//----------------------------------------------------------------------------------------------------------------------

void Tablespace_listener::exitTsOptionEngine(
    parsers::MySQLParser::TsOptionEngineContext *ctx) {
  shcore::Dictionary_t tablespace = m_object;
  tablespace->set("engine", unquoteIdentifier(ctx->engineRef()->getText()));
}

//----------------------------------------------------------------------------------------------------------------------

void Tablespace_listener::exitTsOptionWait(
    parsers::MySQLParser::TsOptionWaitContext *ctx) {
  shcore::Dictionary_t tablespace = m_object;
  tablespace->set("wait", ctx->WAIT_SYMBOL() != nullptr);
}

//----------------------------------------------------------------------------------------------------------------------

void Tablespace_listener::exitTsOptionComment(
    parsers::MySQLParser::TsOptionCommentContext *ctx) {
  shcore::Dictionary_t tablespace = m_object;
  tablespace->set("comment", unquote_text_literal(ctx->textLiteral(),
                                                  m_no_backslash_escapes));
}

//----------------------------------------------------------------------------------------------------------------------

void Tablespace_listener::exitTsOptionFileblockSize(
    parsers::MySQLParser::TsOptionFileblockSizeContext *ctx) {
  shcore::Dictionary_t tablespace = m_object;
  tablespace->set(
      "fileBlockSize",
      static_cast<uint64_t>(number_value(ctx->sizeNumber()->getText())));
}

//----------------------------------------------------------------------------------------------------------------------

void Tablespace_listener::exitTsOptionEncryption(
    parsers::MySQLParser::TsOptionEncryptionContext *) {}

//----------------- Event_listener
//--------------------------------------------------------------------------------------

Event_listener::Event_listener(tree::ParseTree *tree,
                               shcore::Dictionary_t anObject,
                               bool no_backslash_escapes)
    : Object_listener(anObject, no_backslash_escapes) {
  tree::ParseTreeWalker::DEFAULT.walk(this, tree);
}

//----------------------------------------------------------------------------------------------------------------------

void Event_listener::exitDefinerClause(
    parsers::MySQLParser::DefinerClauseContext *ctx) {
  shcore::Dictionary_t event = m_object;
  event->set("definer",
             parsers::MySQLBaseLexer::sourceTextForContext(ctx->user(), true));
}

//----------------------------------------------------------------------------------------------------------------------

void Event_listener::exitCreateEvent(
    parsers::MySQLParser::CreateEventContext *ctx) {
  shcore::Dictionary_t event = m_object;

  ignore_if_exists = ctx->ifNotExists() != nullptr;

  Identifier_listener listener(ctx->eventName());
  event->set("name", listener.parts.back());
  if (listener.parts.size() > 1 && !listener.parts[0].empty())
    event->set("_schema", listener.parts[0]);

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

  if (ctx->COMMENT_SYMBOL() != nullptr)
    event->set("comment", unquote_text_literal(ctx->textLiteral(),
                                               m_no_backslash_escapes));

  event->set("sqlBody", parsers::MySQLBaseLexer::sourceTextForContext(
                            ctx->compoundStatement(), true));
}

//----------------------------------------------------------------------------------------------------------------------

void Event_listener::exitSchedule(parsers::MySQLParser::ScheduleContext *ctx) {
  shcore::Dictionary_t event = m_object;

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

//----------------------------------------------------------------------------------------------------------------------
}  // namespace parser
}  // namespace mysqlshdk
