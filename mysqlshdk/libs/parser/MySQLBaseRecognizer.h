/*
 * Copyright (c) 2016, 2024, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is designed to work with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms,
 * as designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have either included with
 * the program or referenced in the documentation.
 *
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#pragma once

#include <string>

#include <CommonToken.h>
#include <Parser.h>

#include "mysqlshdk/libs/parser/MySQLBaseLexer.h"
#include "mysqlshdk/libs/parser/MySQLRecognizerCommon.h"

namespace antlr4 {
class PARSERS_PUBLIC_TYPE Parser;
}

namespace parsers {

class PARSERS_PUBLIC_TYPE MySQLBaseRecognizer : public antlr4::Parser,
                                                public MySQLRecognizerCommon {
 public:
  explicit MySQLBaseRecognizer(antlr4::TokenStream *input);

  void reset() override;

  // A specialized function to get the text from a given context. This falls
  // back to context->getText() in the general case, but provides special
  // behavior for certain contexts (e.g. the implicit string concatenation used
  // in MySQL).
  static std::string getText(antlr4::RuleContext *context, bool convertEscapes);

 protected:
  // Checks the token at the given position relative to the current position,
  // whether it matches the expected value. For positions > 1 this looks ahead,
  // otherwise it looks back. Note: position == 0 is not defined. position == 1
  // is the current position.
  bool look(ssize_t position, size_t expected);

  // Validation function used to check that a string that is not allowed to
  // contain line breaks really doesn't.
  bool containsLinebreak(const std::string &text) const;
};

}  // namespace parsers
