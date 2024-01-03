/*
 * Copyright (c) 2024, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_LIBS_YPARSER_MYSQL_PARSER_H_
#define MYSQLSHDK_LIBS_YPARSER_MYSQL_PARSER_H_

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#ifdef BUILDING_PARSER_PLUGIN
#define PARSER_PUBLIC __declspec(dllexport)
#else  // !BUILDING_PARSER_PLUGIN
#define PARSER_PUBLIC __declspec(dllimport)
#endif  // !BUILDING_PARSER_PLUGIN
#else   // !_WIN32
#define PARSER_PUBLIC __attribute__((visibility("default")))
#endif  // !_WIN32

/**
 * Result of a parser operation.
 */
enum Parser_result {
  PARSER_RESULT_OK = 0,
  PARSER_RESULT_INVALID_HANDLE,
  PARSER_RESULT_FAILURE,
};

/**
 * Invalid handle.
 */
#define PARSER_INVALID_HANDLE NULL

/**
 * Opaque type for the parser.
 */
struct Parser;

/**
 * Handle to a parser.
 */
typedef Parser *Parser_h;

/**
 * Opaque type for the parser error.
 */
struct Parser_error;

/**
 * Handle to a parser error.
 */
typedef Parser_error *Parser_error_h;

/**
 * Provides MAJOR.MINOR.PATCH version of the supported MySQL syntax.
 *
 * Note: do not free this memory.
 *
 * @returns Version of the MySQL syntax.
 */
PARSER_PUBLIC const char *parser_version();

/**
 * Creates a parser. Memory needs to be freed using `parser_destroy()`.
 *
 * NOTE: It's unsafe to move this handle between threads.
 *
 * @param[out] parser Created parser or PARSER_INVALID_HANDLE.
 *
 * @returns Result of this operation.
 */
PARSER_PUBLIC Parser_result parser_create(Parser_h *parser);

/**
 * Destroys a parser.
 *
 * @param parser Parser to be destroyed.
 *
 * @returns Result of this operation.
 */
PARSER_PUBLIC Parser_result parser_destroy(Parser_h parser);

/**
 * Sets the SQL mode to be used by the parser.
 *
 * @param parser Parser to be used.
 * @param sql_mode New SQL mode.
 * @param length Length of the SQL mode.
 *
 * @returns Result of this operation.
 */
PARSER_PUBLIC Parser_result parser_set_sql_mode(Parser_h parser,
                                                const char *sql_mode,
                                                size_t length);

/**
 * Checks syntax of the given SQL statement.
 *
 * @param parser Parser to be used.
 * @param statement Statement to be checked.
 * @param length Length of the statement.
 *
 * @returns Result of this operation.
 */
PARSER_PUBLIC Parser_result parser_check_syntax(Parser_h parser,
                                                const char *statement,
                                                size_t length);

/**
 * Fetches number of pending (still to be fetched) errors.
 *
 * @param parser Parser to be used.
 *
 * @returns Number of pending errors.
 */
PARSER_PUBLIC size_t parser_pending_errors(Parser_h parser);

/**
 * Fetches next pending error.
 *
 * Note: handle is valid till another parser operation.
 *
 * @param parser Parser to be used.
 *
 * @returns Error or PARSER_INVALID_HANDLE if there are no more errors.
 */
PARSER_PUBLIC Parser_error_h parser_next_error(Parser_h parser);

/**
 * Provides error message.
 *
 * Note: do not free this memory.
 *
 * @param error Handle to be used.
 *
 * @returns Error message, or NULL if handle is not valid.
 */
PARSER_PUBLIC const char *parser_error_message(const Parser_error_h error);

/**
 * Provides 0-based offset of the erroneous token.
 *
 * Use parser_error_token_length() to check if error is associated with an input
 * string.
 *
 * @param error Handle to be used.
 *
 * @returns Offset of the erroneous token.
 */
PARSER_PUBLIC size_t parser_error_token_offset(const Parser_error_h error);

/**
 * Provides the length of the erroneous token.
 *
 * @param error Handle to be used.
 *
 * @returns Length of the erroneous token, 0 if error is not associated with an
 *          input string.
 */
PARSER_PUBLIC size_t parser_error_token_length(const Parser_error_h error);

/**
 * Provides 1-based line number where error has occurred.
 *
 * @param error Handle to be used.
 *
 * @returns Line number, or 0 if handle is not valid or if error is not
 *          associated with an input string.
 */
PARSER_PUBLIC size_t parser_error_line(const Parser_error_h error);

/**
 * Provides 1-based offset within a line where error has occurred.
 *
 * @param error Handle to be used.
 *
 * @returns Offset, or 0 if handle is not valid or if error is not associated
 *          with an input string.
 */
PARSER_PUBLIC size_t parser_error_line_offset(const Parser_error_h error);

#ifdef __cplusplus
}
#endif

#endif  // MYSQLSHDK_LIBS_YPARSER_MYSQL_PARSER_H_
