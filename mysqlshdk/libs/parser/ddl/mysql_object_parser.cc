/*
 * Copyright (c) 2013, 2026, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "mysqlshdk/libs/parser/ddl/mysql_object_parser.h"
#include <algorithm>
#include <cctype>
#include <map>
#include <memory>
#include <set>
#include <string_view>
#include <vector>
#include "mysqlshdk/libs/parser/ddl/alter_listeners.h"
#include "mysqlshdk/libs/parser/ddl/create_listeners.h"
#include "mysqlshdk/libs/parser/parsers-common.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_lexing.h"
#include "mysqlshdk/libs/utils/utils_mysql_parsing.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlshdk {
namespace parser {
namespace {
std::string interval_to_string(const antlr4::misc::IntervalSet &set,
                               size_t max_count,
                               const antlr4::dfa::Vocabulary &vocabulary) {
  std::vector<ssize_t> symbols = set.toList();

  if (symbols.empty()) {
    return "";
  }

  std::string ss;
  bool first_entry = true;
  max_count = std::min(max_count, symbols.size());
  for (size_t i = 0; i < max_count; ++i) {
    ssize_t symbol = symbols[i];
    if (!first_entry) ss += ", ";
    first_entry = false;

    if (symbol < 0) {
      ss += "EOF";
    } else {
      std::string name = vocabulary.getDisplayName(symbol);
      if (name.find("_SYMBOL") != std::string::npos) {
        name = name.substr(0, name.size() - 7);
      } else if (name.find("_OPERATOR") != std::string::npos) {
        name = name.substr(0, name.size() - 9);
      } else if (name.find("_NUMBER") != std::string::npos) {
        name = name.substr(0, name.size() - 7) + " number";
      } else {
        static const std::map<std::string, std::string> friendly_description = {
            {"BACK_TICK_QUOTED_ID", "`text`"},
            {"DOUBLE_QUOTED_TEXT", "\"text\""},
            {"SINGLE_QUOTED_TEXT", "'text'"},
        };
        if (const auto it = friendly_description.find(name);
            it != friendly_description.end())
          name = it->second;
      }
      ss += name;
    }
  }

  if (max_count < symbols.size()) ss += ", ...";

  return ss;
}

bool token_text_equals(antlr4::Token *token, std::string_view expected) {
  const auto text = token->getText();
  if (text.size() != expected.size()) return false;

  return shcore::str_caseeq(text, expected);
}

std::string strip_statement_body(std::string_view body) {
  auto result = shcore::str_strip(body);
  if (!result.empty() && result.back() == ';') {
    result.pop_back();
    result = shcore::str_strip(result);
  }
  return result;
}

std::string unquote_text_token(std::string_view text,
                               bool no_backslash_escapes) {
  const auto quote_pos = text.find_first_of("'\"");

  if (quote_pos == std::string::npos)
    throw shcore::Exception::parser_error("Cannot parse text literal");

  text = text.substr(quote_pos);
  return shcore::unquote_sql_string(text, text.front(), no_backslash_escapes);
}

std::string read_resource_group_vcpu(
    parsers::MySQLParser::ResourceGroupVcpuListContext *ctx) {
  std::string vcpu;

  for (const auto range : ctx->vcpuNumOrRange()) {
    if (!vcpu.empty()) vcpu += ", ";
    vcpu += parsers::MySQLBaseLexer::sourceTextForContext(range);
  }

  return vcpu;
}

void read_resource_group_options(
    const shcore::Dictionary_t &resource_group,
    parsers::MySQLParser::ResourceGroupVcpuListContext *vcpu,
    parsers::MySQLParser::ResourceGroupPriorityContext *priority,
    parsers::MySQLParser::ResourceGroupEnableDisableContext *state) {
  if (vcpu) resource_group->set("vcpu", read_resource_group_vcpu(vcpu));

  if (priority) {
    resource_group->set("threadPriority",
                        shcore::Value(static_cast<int64_t>(
                            std::stoll(priority->INT_NUMBER()->getText()))));
  }

  if (state) resource_group->set("enabled", state->ENABLE_SYMBOL() != nullptr);
}

}  // namespace

class Mysql_parser_context_impl;

class Lexer_error_listener : public antlr4::BaseErrorListener {
 public:
  explicit Lexer_error_listener(Mysql_parser_context_impl *aOwner)
      : owner(aOwner) {}

  void syntaxError(antlr4::Recognizer *recognizer,
                   antlr4::Token *offendingSymbol, size_t line,
                   size_t charPositionInLine, const std::string &msg,
                   std::exception_ptr e) override;

 public:
  Mysql_parser_context_impl *owner;
};

class Parser_error_listener : public antlr4::BaseErrorListener {
 public:
  explicit Parser_error_listener(Mysql_parser_context_impl *aOwner)
      : owner(aOwner) {}

  void syntaxError(antlr4::Recognizer *recognizer,
                   antlr4::Token *offendingSymbol, size_t line,
                   size_t charPositionInLine, const std::string &msg,
                   std::exception_ptr e) override;

 protected:
  Mysql_parser_context_impl *owner;
};

class Grant_statement_listener : public parsers::MySQLParserBaseListener {
 public:
  explicit Grant_statement_listener(bool no_backslash_escapes = false)
      : m_no_backslash_escapes(no_backslash_escapes) {
    data = shcore::make_dict();

    m_privileges = shcore::make_array();
    m_users = shcore::make_dict();
    m_requirements = shcore::make_dict();
    m_options = shcore::make_dict();
  }

  explicit Grant_statement_listener(antlr4::tree::ParseTree *tree,
                                    bool no_backslash_escapes = false)
      : Grant_statement_listener(no_backslash_escapes) {
    data->set("privileges", m_privileges);
    data->set("users", m_users);
    data->set("options", m_options);

    antlr4::tree::ParseTreeWalker::DEFAULT.walk(this, tree);
  }

  void exitGrantStatement(
      parsers::MySQLParser::GrantStatementContext *ctx) override {
    if (ctx->ON_SYMBOL() != nullptr && ctx->PROXY_SYMBOL() == nullptr) {
      // Handle specialities for the classic grant command.
      std::string target;
      if (ctx->aclType() != nullptr) target = ctx->aclType()->getText() + " ";
      target +=
          parsers::MySQLBaseLexer::sourceTextForContext(ctx->grantIdentifier());
      data->set("target", target);

      if (ctx->ALL_SYMBOL() != nullptr) {
        std::string priv = ctx->ALL_SYMBOL()->getText();
        if (ctx->PRIVILEGES_SYMBOL() != nullptr)
          priv += " " + ctx->PRIVILEGES_SYMBOL()->getText();
        m_privileges->emplace_back(shcore::Value(priv));
      }
    }
  }

  void exitRoleOrPrivilege(
      parsers::MySQLParser::RoleOrPrivilegeContext *ctx) override {
    m_privileges->emplace_back(
        shcore::Value(parsers::MySQLBaseLexer::sourceTextForContext(ctx)));
  }

  void enterUser(parsers::MySQLParser::UserContext *) override {
    // There are 3 places where a user is parsed: as source user in a proxy
    // grant, in the user list of a roles-to-user grant and the grant list of
    // the classic grant command. The handling is different however. In the
    // first case we keep the user as proxy user reference, while for the other
    // 2 the user goes to the user list.
    m_current_user = shcore::make_dict();
  }

  /**
   * For server 8 and newer. There's no password allowed anymore.
   */
  void exitUserList(parsers::MySQLParser::UserListContext *) override {
    m_current_user->set("id_method", "");
    m_current_user->set("id_string", "");
  }

  void exitUser(parsers::MySQLParser::UserContext *ctx) override {
    auto name = fillUserDetails(ctx, m_current_user);

    // Is the parent the grant rule? If so this is the proxy user, otherwise a
    // user in an assignee list.
    auto parent = dynamic_cast<parsers::MySQLParser::GrantStatementContext *>(
        ctx->parent);
    if (parent != nullptr) {
      if (parent->WITH_SYMBOL()) m_options->set("GRANT", "");
      data->set("proxyUser", m_current_user);
    } else {
      m_users->set(name, m_current_user);
    }
  }

  std::string fillUserDetails(parsers::MySQLParser::UserContext *ctx,
                              shcore::Dictionary_t user) {
    if (ctx->CURRENT_USER_SYMBOL() != nullptr) {
      user->set("user", ctx->CURRENT_USER_SYMBOL()->getText());
      return parsers::MySQLBaseLexer::sourceTextForContext(ctx, true);
    }

    auto account = ctx->userIdentifierOrText();
    user->set("user", text_or_identifier_value(account->textOrIdentifier(0)));

    if (account->AT_SIGN_SYMBOL()) {
      if (const auto host = account->SIMPLE_IDENTIFIER()) {
        user->set("host", host->getText());
      } else {
        user->set("host",
                  text_or_identifier_value(account->textOrIdentifier(1)));
      }
    }

    return parsers::MySQLBaseLexer::sourceTextForContext(ctx, true);
  }

  void exitRequireClause(
      parsers::MySQLParser::RequireClauseContext *ctx) override {
    if (ctx->option != nullptr)
      m_requirements->set(shcore::unquote_identifier(ctx->option->getText()),
                          "");
    data->set("requirements", m_requirements);
  }

  void exitRequireListElement(
      parsers::MySQLParser::RequireListElementContext *ctx) override {
    m_requirements->set(
        ctx->element->getText(),
        shcore::unquote_identifier(ctx->textString()->getText()));
  }

  void exitGrantOption(parsers::MySQLParser::GrantOptionContext *ctx) override {
    std::string value;
    if (ctx->ulong_number() != nullptr) value = ctx->ulong_number()->getText();
    m_options->set(ctx->option->getText(), value);
  }

 public:
  shcore::Dictionary_t data;

 private:
  std::string text_or_identifier_value(
      parsers::MySQLParser::TextOrIdentifierContext *ctx) const {
    if (const auto literal = ctx->textStringLiteral()) {
      return unquote_text_string_literal(literal, m_no_backslash_escapes);
    }

    return Identifier_listener(ctx->identifier()).parts.back();
  }

  shcore::Array_t m_privileges;
  shcore::Dictionary_t m_users;
  shcore::Dictionary_t m_current_user;
  shcore::Dictionary_t m_requirements;
  shcore::Dictionary_t m_options;
  bool m_no_backslash_escapes = false;
};

class Mysql_parser_context_impl : public Mysql_parser_context {
 public:
  explicit Mysql_parser_context_impl(const Parser_context_config &config)
      : lexer(&input),
        tokens(&lexer),
        parser(&tokens),
        lexerErrorListener(this),
        parserErrorListener(this),
        case_sensitive(config.case_sensitive) {
    update_server_version(config.version);
    lexer.sqlMode = parsers::MySQLRecognizerCommon::NoMode;
    if (config.ansi_quotes) {
      lexer.sqlMode = parsers::MySQLRecognizerCommon::SqlMode(
          lexer.sqlMode | parsers::MySQLRecognizerCommon::AnsiQuotes);
    }
    if (config.no_backslash_escapes) {
      lexer.sqlMode = parsers::MySQLRecognizerCommon::SqlMode(
          lexer.sqlMode | parsers::MySQLRecognizerCommon::NoBackslashEscapes);
    }
    parser.sqlMode = lexer.sqlMode;

    lexer.removeErrorListeners();
    lexer.addErrorListener(&lexerErrorListener);

    parser.removeParseListeners();
    parser.removeErrorListeners();
    parser.addErrorListener(&parserErrorListener);
  }

  bool is_case_sensitive() const override { return case_sensitive; }

  void update_server_version(
      const mysqlshdk::utils::Version &new_version) override {
    if (version != new_version) {
      version = new_version;
      lexer.set_server_version(version.numeric());
      parser.serverVersion = lexer.serverVersion;
    }
  }

  void update_sql_mode(const std::string &mode_) override {
    lexer.sqlModeFromString(mode_);
    parser.sqlMode = lexer.sqlMode;
  }

  const mysqlshdk::utils::Version &server_version() const override {
    return version;
  }

  void add_error(std::string const &message, size_t tokenType,
                 size_t startIndex, size_t line, size_t column, size_t length) {
    log_debug("at %zi:%zi:offset %zi: %s", line, column, startIndex,
              message.c_str());
    if (length == 0) length = 1;
    errors.emplace_back(parsers::ParserErrorInfo{message, tokenType, startIndex,
                                                 line, column, length});
  }

  /**
   * Returns a collection of errors from the last parser run. The start
   * position is offset by the given value (used to adjust error position in a
   * larger context).
   */
  std::vector<parsers::ParserErrorInfo> errors_with_offset(
      size_t offset) const override {
    std::vector<parsers::ParserErrorInfo> result = errors;

    for (auto &entry : result) entry.charOffset += offset;

    return result;
  }

  bool is_identifier(size_t type) const override {
    return lexer.isIdentifier(type);
  }

  antlr4::tree::ParseTree *parse(const std::string &text, MySQLParseUnit unit) {
    input.load(text);
    return start_parsing(false, unit);
  }

  MySQLQueryType determine_query_type(const std::string &text) {
    // Important: this call invalidates any previous parse result (because we
    // have to reset lexer and parser to avoid
    //            dangling token references).
    parser.reset();
    errors.clear();

    input.load(text);
    lexer.setInputStream(&input);
    tokens.setTokenSource(&lexer);

    return lexer.determineQueryType();
  }

  std::vector<std::unique_ptr<antlr4::Token>> scan_default_tokens(
      const std::string &text) {
    parser.reset();
    errors.clear();

    input.load(text);
    lexer.reset();
    lexer.setInputStream(&input);

    std::vector<std::unique_ptr<antlr4::Token>> result;
    while (true) {
      auto token = lexer.nextToken();
      const auto type = token->getType();

      if (token->getChannel() == antlr4::Token::DEFAULT_CHANNEL)
        result.emplace_back(std::move(token));

      if (type == antlr4::Token::EOF) break;
    }

    if (!errors.empty())
      throw shcore::Exception::parser_error(errors.front().message);

    return result;
  }

  bool read_qualified_identifier(
      const std::vector<std::unique_ptr<antlr4::Token>> &token_list,
      size_t *index, std::string *schema, std::string *name) const {
    if (*index >= token_list.size() ||
        !is_identifier(token_list[*index]->getType()))
      return false;

    auto first =
        shcore::unquote_identifier(token_list[*index]->getText(), true);
    ++*index;

    if (*index < token_list.size() &&
        token_list[*index]->getType() == parsers::MySQLLexer::DOT_SYMBOL) {
      ++*index;
      if (*index >= token_list.size() ||
          !is_identifier(token_list[*index]->getType()))
        return false;

      *schema = first;
      *name = shcore::unquote_identifier(token_list[*index]->getText(), true);
      ++*index;
    } else {
      *name = first;
    }

    return true;
  }

  shcore::Dictionary_t read_json_duality_view(const std::string &query,
                                              const std::string &current_schema,
                                              bool alter) {
    auto default_tokens = scan_default_tokens(query);

    size_t view_index = default_tokens.size();
    for (size_t i = 0; i + 2 < default_tokens.size(); ++i) {
      if (default_tokens[i]->getType() != parsers::MySQLLexer::JSON_SYMBOL)
        continue;

      size_t duality_index = i + 1;
      if (duality_index < default_tokens.size() &&
          token_text_equals(default_tokens[duality_index].get(),
                            "RELATIONAL")) {
        ++duality_index;
      }

      if (duality_index + 1 < default_tokens.size() &&
          token_text_equals(default_tokens[duality_index].get(), "DUALITY") &&
          default_tokens[duality_index + 1]->getType() ==
              parsers::MySQLLexer::VIEW_SYMBOL) {
        view_index = duality_index + 1;
        break;
      }
    }

    if (view_index == default_tokens.size())
      throw shcore::Exception::parser_error(
          "Cannot parse JSON duality view statement");

    size_t name_index = view_index + 1;
    if (!alter && name_index + 2 < default_tokens.size() &&
        default_tokens[name_index]->getType() ==
            parsers::MySQLLexer::IF_SYMBOL &&
        default_tokens[name_index + 1]->getType() ==
            parsers::MySQLLexer::NOT_SYMBOL &&
        default_tokens[name_index + 2]->getType() ==
            parsers::MySQLLexer::EXISTS_SYMBOL) {
      name_index += 3;
    }

    std::string schema = current_schema;
    std::string name;
    if (!read_qualified_identifier(default_tokens, &name_index, &schema, &name))
      throw shcore::Exception::parser_error(
          "Cannot parse JSON duality view name");

    if (name_index >= default_tokens.size() ||
        default_tokens[name_index]->getType() != parsers::MySQLLexer::AS_SYMBOL)
      throw shcore::Exception::parser_error(
          "Cannot parse JSON duality view body");

    if (name_index + 1 >= default_tokens.size() ||
        default_tokens[name_index + 1]->getType() == antlr4::Token::EOF)
      throw shcore::Exception::parser_error(
          "Cannot parse JSON duality view body");

    shcore::Dictionary_t view = shcore::make_dict();
    view->set("_type", "view");
    view->set("_schema", schema);
    view->set(alter ? "_name" : "name", name);
    view->set("viewType", "JSON_DUALITY");
    view->set("sqlBody", strip_statement_body(query.substr(
                             default_tokens[name_index]->getStopIndex() + 1)));

    return view;
  }

  shcore::Dictionary_t read_alter_library(const std::string &query,
                                          const std::string &current_schema,
                                          bool no_backslash_escapes) {
    auto default_tokens = scan_default_tokens(query);

    if (default_tokens.size() < 5 ||
        default_tokens[0]->getType() != parsers::MySQLLexer::ALTER_SYMBOL ||
        default_tokens[1]->getType() != parsers::MySQLLexer::LIBRARY_SYMBOL)
      throw shcore::Exception::parser_error("Cannot parse library name");

    size_t name_index = 2;

    std::string schema = current_schema;
    std::string name;
    if (!read_qualified_identifier(default_tokens, &name_index, &schema, &name))
      throw shcore::Exception::parser_error("Cannot parse library name");

    shcore::Dictionary_t library = shcore::make_dict();
    library->set("_type", "library");
    library->set("_schema", schema);
    library->set("_name", name);

    if (name_index >= default_tokens.size() ||
        default_tokens[name_index]->getType() !=
            parsers::MySQLLexer::COMMENT_SYMBOL ||
        name_index + 1 >= default_tokens.size())
      throw shcore::Exception::parser_error("Cannot parse library comment");

    library->set("comment",
                 unquote_text_token(default_tokens[name_index + 1]->getText(),
                                    no_backslash_escapes));

    if (name_index + 2 >= default_tokens.size() ||
        default_tokens[name_index + 2]->getType() != antlr4::Token::EOF)
      throw shcore::Exception::parser_error(
          "Unexpected token after library comment");

    return library;
  }

  shcore::Dictionary_t read_one_create(antlr4::tree::ParseTree *ctx,
                                       MySQLQueryType queryType,
                                       const std::string &current_schema,
                                       bool no_backslash_escapes) {
    switch (queryType) {
      case QtCreateDatabase: {
        shcore::Dictionary_t schema = shcore::make_dict();
        schema->set("_type", "schema");
        Schema_listener listener(ctx, schema);

        return schema;
      }

      case QtCreateTable: {
        shcore::Dictionary_t table = shcore::make_dict();
        table->set("_type", "table");
        table->set("_schema", current_schema);
        Table_listener listener(ctx, table, no_backslash_escapes);

        return table;
      }

      case QtCreateIndex: {
        shcore::Dictionary_t index = shcore::make_dict();
        index->set("_type", "index");
        index->set("_schema", current_schema);

        Index_listener listener(ctx, index, no_backslash_escapes);

        return index;
      }

      case QtCreateEvent: {
        shcore::Dictionary_t event = shcore::make_dict();
        event->set("_type", "event");
        event->set("_schema", current_schema);

        Event_listener listener(ctx, event, no_backslash_escapes);

        return event;
      }

      case QtCreateView: {
        shcore::Dictionary_t view = shcore::make_dict();
        view->set("_type", "view");
        view->set("_schema", current_schema);

        View_listener listener(ctx, view);

        return view;
      }

      case QtCreateProcedure:
      case QtCreateFunction:
      case QtCreateUdf: {
        shcore::Dictionary_t routine = shcore::make_dict();
        switch (queryType) {
          case QtCreateProcedure:
            routine->set("_type", "procedure");
            break;
          case QtCreateFunction:
            routine->set("_type", "function");
            break;
          case QtCreateUdf:
            routine->set("_type", "udf");
            break;
          default:
            assert(0);
        }
        routine->set("_schema", current_schema);

        Routine_listener listener(ctx, routine, no_backslash_escapes);

        return routine;
      }

      case QtCreateTrigger: {
        shcore::Dictionary_t trigger = shcore::make_dict();
        trigger->set("_schema", current_schema);
        trigger->set("_type", "trigger");

        Trigger_listener listener(ctx, trigger);

        return trigger;
      }

      case QtCreateLogFileGroup: {
        shcore::Dictionary_t group = shcore::make_dict();
        group->set("_type", "logFileGroup");

        Logfile_group_listener listener(ctx, group, no_backslash_escapes);

        return group;
      }

      case QtCreateServer: {
        shcore::Dictionary_t server = shcore::make_dict();
        server->set("_type", "server");

        Server_listener listener(ctx, server, no_backslash_escapes);

        return server;
      }

      case QtCreateTableSpace: {
        shcore::Dictionary_t tablespace = shcore::make_dict();
        tablespace->set("_type", "tablespace");

        Tablespace_listener listener(ctx, tablespace, no_backslash_escapes);
        return tablespace;
      }

      case QtCreateUndoTablespace: {
        const auto create_statement =
            dynamic_cast<parsers::MySQLParser::CreateStatementContext *>(ctx);
        const auto undo_tablespace = create_statement->createUndoTablespace();

        shcore::Dictionary_t tablespace = shcore::make_dict();
        tablespace->set("_type", "undoTablespace");
        tablespace->set("name",
                        Identifier_listener(undo_tablespace->tablespaceName())
                            .parts.back());

        if (const auto data_file = undo_tablespace->tsDataFile()) {
          tablespace->set("dataFile",
                          unquote_text_literal(data_file->textLiteral(),
                                               no_backslash_escapes));
        }

        return tablespace;
      }

      case QtCreateLibrary: {
        shcore::Dictionary_t library = shcore::make_dict();
        library->set("_type", "library");
        library->set("_schema", current_schema);

        Library_listener listener(ctx, library);

        return library;
      }

      case QtCreateSpatialReference: {
        const auto create_statement =
            dynamic_cast<parsers::MySQLParser::CreateStatementContext *>(ctx);
        const auto spatial_reference =
            create_statement->createSpatialReference();

        shcore::Dictionary_t srs = shcore::make_dict();
        srs->set("_type", "spatialReferenceSystem");
        srs->set("name", spatial_reference->real_ulonglong_number()->getText());

        return srs;
      }

      default:
        throw std::logic_error("Invalid query type");
    }
  }

  /**
   * Parse a DDL script into a list of change descriptors.
   */
  shcore::Dictionary_t parse_ddl_script(const std::string &sql,
                                        const std::string &schema,
                                        bool ignore_errors) override {
    static std::set<MySQLQueryType> relevantQueryTypes = {
        QtCreateTable, QtCreateIndex, QtCreateDatabase, QtCreateEvent,
        QtCreateView, QtCreateJsonDualityView, QtCreateRoutine,
        QtCreateProcedure, QtCreateFunction, QtCreateUdf, QtCreateTrigger,
        QtCreateLogFileGroup, QtCreateServer, QtCreateTableSpace,
        QtCreateLibrary, QtCreateRole, QtCreateSpatialReference,
        QtCreateResourceGroup, QtCreateUndoTablespace,

        QtAlterDatabase,
        // QtAlterLogFileGroup,
        QtAlterFunction, QtAlterProcedure,
        // QtAlterServer,
        QtAlterTable,
        // QtAlterTableSpace,
        QtAlterEvent, QtAlterView, QtAlterJsonDualityView, QtAlterResourceGroup,
        QtAlterUndoTablespace, QtAlterLibrary,

        QtDropDatabase, QtDropEvent, QtDropFunction, QtDropProcedure,
        QtDropIndex,
        // QtDropLogfileGroup,
        // QtDropServer,
        QtDropTable,
        // QtDropTablespace,
        QtDropTrigger, QtDropView, QtDropLibrary, QtDropRole,
        QtDropSpatialReference, QtDropResourceGroup, QtDropUndoTablespace,

        QtRenameTable,

        QtUse};

    std::string current_schema = schema;
    const bool ansi_quotes =
        lexer.isSqlModeActive(parsers::MySQLRecognizerCommon::AnsiQuotes);
    const bool no_backslash_escapes = lexer.isSqlModeActive(
        parsers::MySQLRecognizerCommon::NoBackslashEscapes);

    shcore::Array_t errors_ = shcore::make_array();
    shcore::Array_t change_list = shcore::make_array();

    auto add_drop = [change_list, &current_schema](
                        const std::string &type,
                        antlr4::tree::ParseTree *ident) {
      Identifier_listener listener(ident);

      std::string drop_schema = current_schema;
      if (listener.parts.size() > 1 && !listener.parts[0].empty())
        drop_schema = listener.parts[0];

      change_list->push_back(shcore::Value(shcore::make_dict(
          "drop", shcore::make_dict("_type", shcore::Value(type), "_schema",
                                    shcore::Value(drop_schema), "_name",
                                    shcore::Value(listener.parts.back())))));
    };

    auto add_role_changes = [change_list](
                                const std::string &operation,
                                parsers::MySQLParser::RoleListContext *roles) {
      for (auto role : roles->role()) {
        const auto role_name =
            parsers::MySQLBaseLexer::sourceTextForContext(role, true);
        auto role_change = shcore::make_dict("_type", shcore::Value("role"));
        role_change->set(operation == "create" ? "name" : "_name", role_name);
        change_list->push_back(shcore::Value(
            shcore::make_dict(operation, shcore::Value(role_change))));
      }
    };

    std::stringstream stream(sql);
    mysqlshdk::utils::iterate_sql_stream(
        &stream, 1024 * 16,
        [this, &errors_, &current_schema, &change_list, &ignore_errors,
         &add_drop, &add_role_changes, &ansi_quotes, &no_backslash_escapes](
            std::string_view statement, std::string_view, size_t lnum, size_t) {
          // Collect textual FK references into a local cache. At the end this
          // is used to find actual ref tables + columns, when all tables have
          // been parsed.

          if (statement.empty()) return true;

          const char *s = statement.data();
          const size_t len = statement.size();

          // ignore single-line comments
          if ((len == 2 && s[0] == '-' && s[1] == '-') ||
              (len > 2 && s[0] == '-' && s[1] == '-' && isspace(s[2])))
            return true;

          std::string query(s, len);
          MySQLQueryType queryType = determine_query_type(query);

          if (queryType == QtAmbiguous || queryType == QtUnknown) {
            if (!ignore_errors) {
              log_debug("Unhandled query: %s", query.c_str());
              throw shcore::Exception::parser_error(
                  std::to_string(lnum) + ": Cannot determine query type for '" +
                  query.substr(0, 16) + "...'");
            }
            errors_->emplace_back(
                shcore::Value("at line " + std::to_string(lnum) +
                              ": Cannot determine query type for '" +
                              query.substr(0, 16) + "...'"));
            return true;
          }

          if (relevantQueryTypes.count(queryType) == 0)
            return true;  // Something we are not interested in. Don't bother
                          // parsing it.

          if (queryType == QtCreateJsonDualityView ||
              queryType == QtAlterJsonDualityView) {
            try {
              const bool alter = queryType == QtAlterJsonDualityView;
              change_list->push_back(shcore::Value(shcore::make_dict(
                  alter ? "alter" : "create",
                  read_json_duality_view(query, current_schema, alter))));
            } catch (const shcore::Exception &error) {
              errors_->emplace_back(shcore::Value(
                  "at line " + std::to_string(lnum) + ": " + error.what()));
              if (!ignore_errors) throw;
            }
            return true;
          }

          if (queryType == QtAlterLibrary) {
            try {
              change_list->push_back(shcore::Value(shcore::make_dict(
                  "alter", read_alter_library(query, current_schema,
                                              no_backslash_escapes))));
            } catch (const shcore::Exception &error) {
              errors_->emplace_back(shcore::Value(
                  "at line " + std::to_string(lnum) + ": " + error.what()));
              if (!ignore_errors) throw;
            }
            return true;
          }

          auto tree = parse(query, queryType == QtDropLibrary
                                       ? MySQLParseUnit::PuDropLibrary
                                       : MySQLParseUnit::PuGeneric);
          if (!errors.empty()) {
            for (auto &error : errors)
              errors_->emplace_back(shcore::Value(
                  "at line " + std::to_string(lnum) + ": " + error.message));

            if (!ignore_errors) {
              log_debug("%s: %s", errors.front().message.c_str(),
                        query.c_str());
              throw shcore::Exception::parser_error(errors.front().message);
            }

            return true;
          }

          if (queryType == QtDropLibrary) {
            add_drop(
                "library",
                dynamic_cast<parsers::MySQLParser::DropLibraryContext *>(tree)
                    ->libraryRef());
            return true;
          }

          auto statementContext =
              dynamic_cast<parsers::MySQLParser::QueryContext *>(tree)
                  ->simpleStatement();
          switch (queryType) {
            case QtUse:
              current_schema = shcore::unquote_identifier(
                  statementContext->utilityStatement()
                      ->useCommand()
                      ->schemaRef()
                      ->getText(),
                  ansi_quotes);
              break;

            case QtCreateTable:
            case QtCreateIndex:
            case QtCreateDatabase:
            case QtCreateEvent:
            case QtCreateView:
            case QtCreateRoutine:
            case QtCreateProcedure:
            case QtCreateFunction:
            case QtCreateUdf:
            case QtCreateTrigger:
            case QtCreateLogFileGroup:
            case QtCreateServer:
            case QtCreateTableSpace:
            case QtCreateLibrary:
            case QtCreateSpatialReference:
            case QtCreateUndoTablespace:
              change_list->push_back(shcore::Value(shcore::make_dict(
                  "create", read_one_create(statementContext->createStatement(),
                                            queryType, current_schema,
                                            no_backslash_escapes))));
              break;

            case QtCreateRole:
              add_role_changes("create", statementContext->createStatement()
                                             ->createRole()
                                             ->roleList());
              break;

            case QtCreateResourceGroup: {
              auto create_resource_group =
                  statementContext->resourceGroupManagement()
                      ->createResourceGroup();
              auto resource_group = shcore::make_dict(
                  "_type", shcore::Value("resourceGroup"), "name",
                  shcore::Value(
                      Identifier_listener(create_resource_group->identifier())
                          .parts.back()),
                  "type",
                  shcore::Value(create_resource_group->USER_SYMBOL() != nullptr
                                    ? "USER"
                                    : "SYSTEM"));

              read_resource_group_options(
                  resource_group,
                  create_resource_group->resourceGroupVcpuList(),
                  create_resource_group->resourceGroupPriority(),
                  create_resource_group->resourceGroupEnableDisable());

              change_list->push_back(shcore::Value(
                  shcore::make_dict("create", shcore::Value(resource_group))));
              break;
            }

            case QtDropDatabase:
              change_list->push_back(shcore::Value(shcore::make_dict(
                  "drop",
                  shcore::make_dict(
                      "_type", shcore::Value("schema"), "_name",
                      Identifier_listener(statementContext->dropStatement()
                                              ->dropDatabase()
                                              ->schemaRef())
                          .parts.back()))));
              break;

            case QtDropEvent:
              add_drop(
                  "event",
                  statementContext->dropStatement()->dropEvent()->eventRef());
              break;

            case QtDropProcedure:
              add_drop("procedure", statementContext->dropStatement()
                                        ->dropProcedure()
                                        ->procedureRef());

              break;

            case QtDropFunction:  // Including UDFs.
              add_drop("function", statementContext->dropStatement()
                                       ->dropFunction()
                                       ->functionRef());
              break;

            case QtDropIndex: {
              Identifier_listener listener(
                  statementContext->dropStatement()->dropIndex()->indexRef());
              Identifier_listener table_listener(
                  statementContext->dropStatement()->dropIndex()->tableRef());

              std::string index_schema = current_schema;
              if (table_listener.parts.size() > 1 &&
                  !table_listener.parts[0].empty())
                index_schema = table_listener.parts[0];

              change_list->push_back(shcore::Value(shcore::make_dict(
                  "drop", shcore::make_dict(
                              "_type", shcore::Value("index"), "_name",
                              shcore::Value(listener.parts.back()), "_table",
                              shcore::Value(table_listener.parts.back()),
                              "_schema", shcore::Value(index_schema)))));
              break;
            }

            case QtDropTable:
              for (auto tableRef : statementContext->dropStatement()
                                       ->dropTable()
                                       ->tableRefList()
                                       ->tableRef()) {
                add_drop("table", tableRef);
              }
              break;

            case QtDropView:
              for (auto tableRef : statementContext->dropStatement()
                                       ->dropView()
                                       ->viewRefList()
                                       ->viewRef()) {
                add_drop("view", tableRef);
              }
              break;

            case QtDropTrigger:
              add_drop("trigger", statementContext->dropStatement()
                                      ->dropTrigger()
                                      ->triggerRef());
              break;

            case QtDropRole:
              add_role_changes(
                  "drop",
                  statementContext->dropStatement()->dropRole()->roleList());
              break;

            case QtDropSpatialReference:
              change_list->push_back(shcore::Value(shcore::make_dict(
                  "drop",
                  shcore::make_dict(
                      "_type", shcore::Value("spatialReferenceSystem"), "_name",
                      shcore::Value(statementContext->dropStatement()
                                        ->dropSpatialReference()
                                        ->real_ulonglong_number()
                                        ->getText())))));
              break;

            case QtDropUndoTablespace:
              change_list->push_back(shcore::Value(shcore::make_dict(
                  "drop",
                  shcore::make_dict(
                      "_type", shcore::Value("undoTablespace"), "_name",
                      shcore::Value(
                          Identifier_listener(statementContext->dropStatement()
                                                  ->dropUndoTablespace()
                                                  ->tablespaceRef())
                              .parts.back())))));
              break;

            case QtDropResourceGroup: {
              auto drop_resource_group =
                  statementContext->resourceGroupManagement()
                      ->dropResourceGroup();
              auto resource_group = shcore::make_dict(
                  "_type", shcore::Value("resourceGroup"), "_name",
                  shcore::Value(Identifier_listener(
                                    drop_resource_group->resourceGroupRef())
                                    .parts.back()));

              if (drop_resource_group->FORCE_SYMBOL())
                resource_group->set("force", true);

              change_list->push_back(shcore::Value(
                  shcore::make_dict("drop", shcore::Value(resource_group))));
              break;
            }

            case QtRenameTable: {
              // Renaming a table is special as you can use it also to rename
              // a view and to move a table from one schema to the other (not
              // for views, though). Due to the way we store triggers we have
              // an easy life wrt. related triggers.
              for (auto renamePair :
                   statementContext->renameTableStatement()->renamePair()) {
                Identifier_listener sourceListener(renamePair->tableRef());
                Identifier_listener targetListener(renamePair->tableName());

                auto alter = shcore::make_dict(
                    "_type", shcore::Value("table"), "_schema",
                    shcore::Value(sourceListener.parts.size() > 1
                                      ? sourceListener.parts[0]
                                      : current_schema),
                    "_name", shcore::Value(sourceListener.parts.back()),
                    "schema",
                    shcore::Value(targetListener.parts.size() > 1
                                      ? targetListener.parts[0]
                                      : current_schema),
                    "name", shcore::Value(targetListener.parts.back()));

                change_list->push_back(
                    shcore::Value(shcore::make_dict("alter", alter)));
              }
              break;
            }

            case QtAlterDatabase: {
              Schema_alter_listener(statementContext, &change_list);
              break;
            }

            case QtAlterFunction: {
              Routine_alter_listener(statementContext, current_schema,
                                     &change_list, no_backslash_escapes);
              break;
            }

            case QtAlterProcedure: {
              Routine_alter_listener(statementContext, current_schema,
                                     &change_list, no_backslash_escapes);
              break;
            }

            case QtAlterTable: {
              Table_alter_listener(statementContext, current_schema,
                                   &change_list, no_backslash_escapes);
              break;
            }

            case QtAlterView: {
              View_alter_listener(statementContext, current_schema,
                                  &change_list);
              break;
            }

            case QtAlterEvent: {
              Event_alter_listener(statementContext, current_schema,
                                   &change_list, no_backslash_escapes);
              break;
            }

            case QtAlterUndoTablespace: {
              auto alter_undo_tablespace =
                  statementContext->alterStatement()->alterUndoTablespace();
              auto tablespace = shcore::make_dict(
                  "_type", shcore::Value("undoTablespace"), "_name",
                  shcore::Value(Identifier_listener(
                                    alter_undo_tablespace->tablespaceRef())
                                    .parts.back()));

              if (alter_undo_tablespace->ACTIVE_SYMBOL())
                tablespace->set("state", "ACTIVE");
              else if (alter_undo_tablespace->INACTIVE_SYMBOL())
                tablespace->set("state", "INACTIVE");

              change_list->push_back(shcore::Value(
                  shcore::make_dict("alter", shcore::Value(tablespace))));
              break;
            }

            case QtAlterResourceGroup: {
              auto alter_resource_group =
                  statementContext->resourceGroupManagement()
                      ->alterResourceGroup();
              auto resource_group = shcore::make_dict(
                  "_type", shcore::Value("resourceGroup"), "_name",
                  shcore::Value(Identifier_listener(
                                    alter_resource_group->resourceGroupRef())
                                    .parts.back()));

              read_resource_group_options(
                  resource_group, alter_resource_group->resourceGroupVcpuList(),
                  alter_resource_group->resourceGroupPriority(),
                  alter_resource_group->resourceGroupEnableDisable());

              if (alter_resource_group->FORCE_SYMBOL())
                resource_group->set("force", true);

              change_list->push_back(shcore::Value(
                  shcore::make_dict("alter", shcore::Value(resource_group))));
              break;
            }

            default:
              throw std::runtime_error("Unsupported statement type");
          }

          return true;
        },
        [](std::string_view err) {
          throw std::runtime_error(std::string(err));
        },
        ansi_quotes, no_backslash_escapes);

    if (errors_ && !errors_->empty())
      return shcore::make_dict("changes", shcore::Value(change_list), "errors",
                               shcore::Value(errors_));
    else
      return shcore::make_dict("changes", shcore::Value(change_list));
  }

  shcore::Dictionary_t parse_statement(const std::string &sql) override {
    // This part can potentially grow very large because of the sheer amount of
    // possible query types. So it should be moved into an own file if it grows
    // beyond a few 100 lines.

    // First do the query determination, then parse, or we invalidate the tokens
    // from the parse run.
    MySQLQueryType queryType = determine_query_type(sql);

    auto tree = parse(sql, MySQLParseUnit::PuGeneric);
    if (!errors.empty()) {
      // Return the error message in case of syntax errors.
      throw shcore::Exception::parser_error(errors[0].message);
    }

    switch (queryType) {
      case QtGrant:
      case QtGrantProxy: {
        Grant_statement_listener listener(
            tree, lexer.isSqlModeActive(
                      parsers::MySQLRecognizerCommon::NoBackslashEscapes));
        return listener.data;
      }

      default:
        throw shcore::Exception::parser_error("Unsupported query type (" +
                                              std::to_string(queryType) + ")");
    }
  }

 private:
  antlr4::tree::ParseTree *start_parsing(bool fast, MySQLParseUnit unit) {
    errors.clear();
    lexer.reset();
    lexer.setInputStream(&input);  // Not just reset(), which only rewinds the
                                   // current position.
    tokens.setTokenSource(&lexer);

    parser.reset();
    parser.setBuildParseTree(!fast);

    // First parse with the bail error strategy to get quick feedback for
    // correct queries.
    parser.setErrorHandler(std::make_shared<antlr4::BailErrorStrategy>());
    parser.getInterpreter<antlr4::atn::ParserATNSimulator>()->setPredictionMode(
        antlr4::atn::PredictionMode::SLL);

    antlr4::tree::ParseTree *tree;
    try {
      tree = parse_unit(unit);
    } catch (const antlr4::ParseCancellationException &) {
      // Even in fast mode we have to do a second run if we got no error yet
      // (BailErrorStrategy does not do full processing).
      if (fast && !errors.empty()) {
        tree = nullptr;
      } else {
        // If parsing was canceled we either really have a syntax error or we
        // need to do a second step, now with the default strategy and LL
        // parsing.
        tokens.reset();
        parser.reset();
        errors.clear();
        parser.setErrorHandler(
            std::make_shared<antlr4::DefaultErrorStrategy>());
        parser.getInterpreter<antlr4::atn::ParserATNSimulator>()
            ->setPredictionMode(antlr4::atn::PredictionMode::LL);
        tree = parse_unit(unit);
      }
    }

    return tree;
  }

  antlr4::tree::ParseTree *parse_unit(MySQLParseUnit unit) {
    switch (unit) {
      case MySQLParseUnit::PuCreateRoutine:
        return parser.createRoutine();

      case MySQLParseUnit::PuDataType:
        return parser.dataTypeDefinition();

      case MySQLParseUnit::PuDropLibrary:
        return parse_drop_library();

      default:
        return parser.query();
    }
  }

  antlr4::tree::ParseTree *parse_drop_library() {
    auto tree = parser.dropLibrary();
    require_eof_or_semicolon();
    return tree;
  }

  void require_eof_or_semicolon() {
    auto token = tokens.LT(1);
    if (token->getType() == parsers::MySQLParser::SEMICOLON_SYMBOL) {
      tokens.consume();
      token = tokens.LT(1);
    }

    if (token->getType() == antlr4::Token::EOF) return;

    auto wrong_text = token->getText();
    if (wrong_text.empty()) wrong_text = " ";
    if (wrong_text[0] != '"' && wrong_text[0] != '\'' && wrong_text[0] != '`')
      wrong_text = "\"" + wrong_text + "\"";

    add_error("Extraneous input " + wrong_text + " found, expecting: EOF, ';'",
              token->getType(), token->getStartIndex(), token->getLine(),
              token->getCharPositionInLine(),
              token->getStopIndex() - token->getStartIndex() + 1);
  }

 private:
  antlr4::ANTLRInputStream input;
  parsers::MySQLLexer lexer;
  antlr4::CommonTokenStream tokens;
  parsers::MySQLParser parser;
  Lexer_error_listener lexerErrorListener;
  Parser_error_listener parserErrorListener;

  mysqlshdk::utils::Version version;
  bool case_sensitive;
  std::vector<parsers::ParserErrorInfo> errors;
};

void Lexer_error_listener::syntaxError(antlr4::Recognizer *recognizer,
                                       antlr4::Token *, size_t line,
                                       size_t charPositionInLine,
                                       const std::string &,
                                       std::exception_ptr e) {
  // The passed in string is the ANTLR generated error message which we want
  // to improve here. The token reference is always null in a lexer error.
  std::string message;
  try {
    std::rethrow_exception(e);
  } catch (const antlr4::LexerNoViableAltException &) {
    antlr4::Lexer *lexer = dynamic_cast<antlr4::Lexer *>(recognizer);
    antlr4::CharStream *input = lexer->getInputStream();
    std::string text = lexer->getErrorDisplay(input->getText(
        antlr4::misc::Interval(lexer->tokenStartCharIndex, input->index())));
    if (text.empty()) text = " ";  // Should never happen.

    switch (text[0]) {
      case '/':
        message = "Unfinished multiline comment";
        break;
      case '"':
        message = "Unfinished double quoted string literal";
        break;
      case '\'':
        message = "Unfinished single quoted string literal";
        break;
      case '`':
        message = "Unfinished back tick quoted string literal";
        break;

      default:
        // Hex or bin string?
        if (text.size() > 1 && text[1] == '\'' &&
            (text[0] == 'x' || text[0] == 'b')) {
          message = std::string("Unfinished ") +
                    (text[0] == 'x' ? "hex" : "binary") + " string literal";
          break;
        }

        // Something else the lexer couldn't make sense of (likely there is no
        // rule that accepts this input).
        message = "\"" + text + "\" is no valid input at all";
        break;
    }
    owner->add_error(message, 0, lexer->tokenStartCharIndex, line,
                     charPositionInLine,
                     input->index() - lexer->tokenStartCharIndex);
  }
}

void Parser_error_listener::syntaxError(antlr4::Recognizer *recognizer,
                                        antlr4::Token *offendingSymbol,
                                        size_t line, size_t charPositionInLine,
                                        const std::string &msg,
                                        std::exception_ptr e) {
  std::string message;

  parsers::MySQLParser *parser =
      dynamic_cast<parsers::MySQLParser *>(recognizer);
  parsers::MySQLBaseLexer *lexer = dynamic_cast<parsers::MySQLBaseLexer *>(
      parser->getTokenStream()->getTokenSource());
  bool isEof = offendingSymbol->getType() == antlr4::Token::EOF;
  if (isEof) {
    // In order to show a good error marker look at the previous token if we
    // are at the input end.
    antlr4::Token *previous = parser->getTokenStream()->LT(-1);
    if (previous != nullptr) offendingSymbol = previous;
  }

  std::string wrongText = offendingSymbol->getText();

  // getExpectedTokens() ignores predicates, so it might include the token for
  // which we got this syntax error, if that was excluded by a predicate
  // (which in our case is always a version check). That's a good indicator to
  // tell the user that this keyword is not valid *for the current server
  // version*.
  antlr4::misc::IntervalSet expected = parser->getExpectedTokens();
  bool invalidForVersion = false;
  size_t tokenType = offendingSymbol->getType();
  if (tokenType != parsers::MySQLLexer::IDENTIFIER &&
      expected.contains(tokenType)) {
    invalidForVersion = true;
  } else {
    tokenType = lexer->keywordFromText(wrongText);
    if (expected.contains(tokenType)) invalidForVersion = true;
  }

  if (invalidForVersion) {
    expected.remove(tokenType);
  }

  // Try to find the expected input by examining the current parser context
  // and the expected interval set. The latter often lists useless keywords,
  // especially if they are allowed as identifiers.
  std::string expectedText;

  static std::set<size_t> simpleRules = {
      parsers::MySQLParser::RuleIdentifier,
      parsers::MySQLParser::RuleQualifiedIdentifier,
  };

  static std::map<size_t, std::string> objectNames = {
      {parsers::MySQLParser::RuleColumnName, "column"},
      {parsers::MySQLParser::RuleColumnRef, "column"},
      {parsers::MySQLParser::RuleColumnInternalRef, "column"},
      {parsers::MySQLParser::RuleIndexName, "index"},
      {parsers::MySQLParser::RuleIndexRef, "index"},
      {parsers::MySQLParser::RuleSchemaName, "schema"},
      {parsers::MySQLParser::RuleSchemaRef, "schema"},
      {parsers::MySQLParser::RuleProcedureName, "procedure"},
      {parsers::MySQLParser::RuleProcedureRef, "procedure"},
      {parsers::MySQLParser::RuleFunctionName, "function"},
      {parsers::MySQLParser::RuleFunctionRef, "function"},
      {parsers::MySQLParser::RuleTriggerName, "trigger"},
      {parsers::MySQLParser::RuleTriggerRef, "trigger"},
      {parsers::MySQLParser::RuleViewName, "view"},
      {parsers::MySQLParser::RuleViewRef, "view"},
      {parsers::MySQLParser::RuleTablespaceName, "tablespace"},
      {parsers::MySQLParser::RuleTablespaceRef, "tablespace"},
      {parsers::MySQLParser::RuleLogfileGroupName, "logfile group"},
      {parsers::MySQLParser::RuleLogfileGroupRef, "logfile group"},
      {parsers::MySQLParser::RuleEventName, "event"},
      {parsers::MySQLParser::RuleEventRef, "event"},
      {parsers::MySQLParser::RuleUdfName, "udf"},
      {parsers::MySQLParser::RuleServerName, "server"},
      {parsers::MySQLParser::RuleServerRef, "server"},
      {parsers::MySQLParser::RuleEngineRef, "engine"},
      {parsers::MySQLParser::RuleTableName, "table"},
      {parsers::MySQLParser::RuleTableRef, "table"},
      {parsers::MySQLParser::RuleFilterTableRef, "table"},
      {parsers::MySQLParser::RuleTableRefWithWildcard, "table"},
      {parsers::MySQLParser::RuleParameterName, "parameter"},
      {parsers::MySQLParser::RuleLabelIdentifier, "label"},
      {parsers::MySQLParser::RuleLabelRef, "label"},
      {parsers::MySQLParser::RuleRoleIdentifier, "role"},
      {parsers::MySQLParser::RulePluginRef, "plugin"},
      {parsers::MySQLParser::RuleComponentRef, "component"},
      {parsers::MySQLParser::RuleResourceGroupRef, "resource group"},
      {parsers::MySQLParser::RuleWindowName, "window"},
  };

  // Walk up from generic rules to reach something that gives us more context,
  // if needed.
  antlr4::ParserRuleContext *context = parser->getRuleContext();
  while (simpleRules.find(context->getRuleIndex()) != simpleRules.end())
    context = dynamic_cast<antlr4::ParserRuleContext *>(context->parent);

  switch (context->getRuleIndex()) {
    case parsers::MySQLParser::RuleFunctionCall:
      expectedText = "a complete function call or other expression";
      break;

    case parsers::MySQLParser::RuleExpr:
      expectedText = " an expression";
      break;

    case parsers::MySQLParser::RuleColumnName:
    case parsers::MySQLParser::RuleIndexName:
    case parsers::MySQLParser::RuleSchemaName:
    case parsers::MySQLParser::RuleProcedureName:
    case parsers::MySQLParser::RuleFunctionName:
    case parsers::MySQLParser::RuleTriggerName:
    case parsers::MySQLParser::RuleViewName:
    case parsers::MySQLParser::RuleTablespaceName:
    case parsers::MySQLParser::RuleLogfileGroupName:
    case parsers::MySQLParser::RuleEventName:
    case parsers::MySQLParser::RuleUdfName:
    case parsers::MySQLParser::RuleServerName:
    case parsers::MySQLParser::RuleTableName:
    case parsers::MySQLParser::RuleParameterName:
    case parsers::MySQLParser::RuleLabelIdentifier:
    case parsers::MySQLParser::RuleRoleIdentifier:
    case parsers::MySQLParser::RuleWindowName: {
      auto iterator = objectNames.find(context->getRuleIndex());
      if (iterator == objectNames.end())
        expectedText = " a new object name";
      else
        expectedText = std::string(" a new ") + iterator->second + " name";
      break;
    }

    case parsers::MySQLParser::RuleColumnRef:
    case parsers::MySQLParser::RuleIndexRef:
    case parsers::MySQLParser::RuleSchemaRef:
    case parsers::MySQLParser::RuleProcedureRef:
    case parsers::MySQLParser::RuleFunctionRef:
    case parsers::MySQLParser::RuleTriggerRef:
    case parsers::MySQLParser::RuleViewRef:
    case parsers::MySQLParser::RuleTablespaceRef:
    case parsers::MySQLParser::RuleLogfileGroupRef:
    case parsers::MySQLParser::RuleEventRef:
    case parsers::MySQLParser::RuleServerRef:
    case parsers::MySQLParser::RuleEngineRef:
    case parsers::MySQLParser::RuleTableRef:
    case parsers::MySQLParser::RuleFilterTableRef:
    case parsers::MySQLParser::RuleTableRefWithWildcard:
    case parsers::MySQLParser::RuleLabelRef:
    case parsers::MySQLParser::RulePluginRef:
    case parsers::MySQLParser::RuleComponentRef:
    case parsers::MySQLParser::RuleResourceGroupRef: {
      auto iterator = objectNames.find(context->getRuleIndex());
      if (iterator == objectNames.end())
        expectedText = " the name of an existing object";
      else
        expectedText =
            std::string(" the name of an existing ") + iterator->second;
      break;
    }

    case parsers::MySQLParser::RuleColumnInternalRef:
      expectedText = " a column name from this table";
      break;

    default: {
      // If the expected set contains the IDENTIFIER token we likely want an
      // identifier at this position. Due to the fact that MySQL defines a
      // number of keywords as possible identifiers, we get all those whenever
      // an identifier is actually required, bloating so the expected set with
      // irrelevant elements. Hence we check for the identifier entry and
      // assume we *only* want an identifier. This gives an unprecise result
      // if both certain keywords *and* an identifier are expected.
      if (expected.contains(
              static_cast<ssize_t>(parsers::MySQLLexer::IDENTIFIER)))
        expectedText = " an identifier";
      else
        expectedText =
            ": " + interval_to_string(expected, 6, parser->getVocabulary());
      break;
    }
  }

  if (wrongText[0] != '"' && wrongText[0] != '\'' && wrongText[0] != '`')
    wrongText = "\"" + wrongText + "\"";

  if (e == nullptr) {
    // Missing or unwanted tokens.
    if (msg.find("missing") != std::string::npos) {
      if (expected.size() == 1) {
        message = std::string("Expected") + expectedText + ", but found " +
                  wrongText + " instead";
      }
    } else {
      message = std::string("Extraneous input ") + wrongText +
                " found, expecting" + expectedText;
    }
  } else {
    try {
      std::rethrow_exception(e);
    } catch (const antlr4::InputMismatchException &) {
      if (isEof)
        message = "Statement is incomplete";
      else
        message = wrongText + " is not valid at this position";

      if (!expectedText.empty()) message += ", expecting" + expectedText;
    } catch (const antlr4::FailedPredicateException &) {
      message = wrongText + " is not valid at this position";
      if (invalidForVersion) message += " for this server version";
      if (!expectedText.empty()) message += ", expecting" + expectedText;
    } catch (const antlr4::NoViableAltException &) {
      if (isEof) {
        message = "Statement is incomplete";
      } else {
        message = wrongText + " is not valid at this position";
        if (invalidForVersion) message += " for this server version";
      }

      if (!expectedText.empty()) message += ", expecting" + expectedText;
    }
  }

  owner->add_error(
      message, offendingSymbol->getType(), offendingSymbol->getStartIndex(),
      line, charPositionInLine,
      offendingSymbol->getStopIndex() - offendingSymbol->getStartIndex() + 1);
}

std::shared_ptr<Mysql_parser_context> create_parser_context(
    const Parser_context_config &config) {
  return std::make_shared<Mysql_parser_context_impl>(config);
}

shcore::Dictionary_t parse_ddl_script(
    const std::shared_ptr<Mysql_parser_context> &context,
    const std::string &sql, const std::string &schema, bool ignore_errors) {
  return context->parse_ddl_script(sql, schema, ignore_errors);
}

shcore::Dictionary_t parse_statement(
    const std::shared_ptr<Mysql_parser_context> &context,
    const std::string &sql) {
  return context->parse_statement(sql);
}

bool parse_type_definition(const std::string &type_definition,
                           Data_type_definition *out_definition) {
  antlr4::ANTLRInputStream input(type_definition);
  parsers::MySQLLexer lexer(&input);
  antlr4::CommonTokenStream tokens(&lexer);
  parsers::MySQLParser parser(&tokens);

  parser.setBuildParseTree(true);
  parser.removeParseListeners();
  parser.removeErrorListeners();
  parser.setErrorHandler(std::make_shared<antlr4::BailErrorStrategy>());
  parser.getInterpreter<antlr4::atn::ParserATNSimulator>()->setPredictionMode(
      antlr4::atn::PredictionMode::SLL);

  antlr4::tree::ParseTree *tree;
  try {
    tree = parser.dataTypeDefinition();
  } catch (const antlr4::ParseCancellationException &) {
    tokens.reset();
    parser.reset();
    parser.setErrorHandler(std::make_shared<antlr4::DefaultErrorStrategy>());
    parser.getInterpreter<antlr4::atn::ParserATNSimulator>()->setPredictionMode(
        antlr4::atn::PredictionMode::LL);
    tree = parser.dataTypeDefinition();
  }

  if (parser.getNumberOfSyntaxErrors() > 0) return false;

  const auto data_type =
      dynamic_cast<parsers::MySQLParser::DataTypeDefinitionContext *>(tree);
  auto flags = shcore::make_array();
  Data_type_listener listener(data_type->dataType(), flags);
  const auto as_optional = [](int value) -> std::optional<int> {
    return value == -1 ? std::nullopt : std::optional{value};
  };

  out_definition->type = listener.data_type_name;
  out_definition->precision = as_optional(listener.precision);
  out_definition->scale = as_optional(listener.scale);
  out_definition->length = as_optional(listener.length);
  out_definition->explicit_params = listener.explicit_params;
  out_definition->flags.clear();
  for (const auto &flag : *flags)
    out_definition->flags.push_back(flag.as_string());

  return true;
}

}  // namespace parser
}  // namespace mysqlshdk
