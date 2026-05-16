# Assumptions: validate_members exists
# from mysqlsh import mysql

# The tests assume the next variables have been put in place
# on the JS Context
# __uri: <user>@<host>
# __host: <host>
# __port: <port>
# __user: <user>
# __uripwd: <uri>:<pwd>@<host>


#@<> mysql module: exports
validate_members(mysql, [
  'get_classic_session',
  'get_session',
  'ErrorCode',
  'Type',
  'help',
  'parse_data_type',
  'parse_ddl_script',
  'parse_statement_ast',
  'quote_identifier',
  'quote_string',
  'split_script',
  'tokenize_statement',
  'unquote_identifier',
  'unquote_string',
  'make_account',
  'split_account'
])

#@# get_classic_session errors
mysql.get_classic_session()
mysql.get_classic_session(1, 2, 3)
mysql.get_classic_session(["bla"])
mysql.get_classic_session("some@uri", 25)

#@# get_session errors
mysql.get_session()
mysql.get_session(1, 2, 3)
mysql.get_session(["bla"])
mysql.get_session("some@uri", 25)

#@<> ErrorCode
assert 1045 == mysql.ErrorCode.ER_ACCESS_DENIED_ERROR

#@<> parser options
assert "pureIdentifier" in str(mysql.parse_statement_ast('SELECT "1"'))
double_quoted_literal_ast = str(mysql.parse_statement_ast('SELECT "1"', {"ansiQuotes": False}))
assert "textStringLiteral" in double_quoted_literal_ast
assert "SINGLE_QUOTED_TEXT" in double_quoted_literal_ast
assert "DOUBLE_QUOTED_TEXT" not in double_quoted_literal_ast
double_quoted_identifier_ast = str(mysql.parse_statement_ast('SELECT "quoted"', {"ansiQuotes": True}))
assert "pureIdentifier" in double_quoted_identifier_ast
assert "BACK_TICK_QUOTED_ID" in double_quoted_identifier_ast

try:
  mysql.parse_statement_ast('SELECT "a" FROM "t"', {"ansiQuotes": False})
  raise AssertionError("parse_statement_ast should fail when ansiQuotes is disabled for double-quoted identifiers")
except RuntimeError:
  pass

assert len([
  token for token in mysql.tokenize_statement('SELECT "a"')
  if token["type"] == "BACK_TICK_QUOTED_ID"
]) == 1

assert len([
  token for token in mysql.tokenize_statement('SELECT "a"', {"ansiQuotes": False})
  if token["type"] == "SINGLE_QUOTED_TEXT"
]) == 1

assert len([
  token for token in mysql.tokenize_statement('SELECT "a"', {"ansiQuotes": True})
  if token["type"] == "BACK_TICK_QUOTED_ID"
]) == 1

quoted_tokens = [
  token for token in mysql.tokenize_statement("SELECT 'a\\''", {"noBackslashEscapes": False})
  if token["type"] == "SINGLE_QUOTED_TEXT"
]
assert len(quoted_tokens) == 1

try:
  mysql.tokenize_statement("SELECT 'a\\''", {"noBackslashEscapes": True})
  raise AssertionError("tokenize_statement should fail when noBackslashEscapes is enabled")
except RuntimeError:
  pass

assert len(mysql.split_script("SELECT 'a\\''; SELECT 2", {"noBackslashEscapes": False})) == 2
assert len(mysql.split_script("SELECT 'a\\''; SELECT 2", {"noBackslashEscapes": True})) == 1

#@<> parse_ddl_script options
mysql.parse_ddl_script("CREATE FUNCTION IF NOT EXISTS f() RETURNS INT RETURN 1")

#@<> parse_data_type
data_type = mysql.parse_data_type("DECIMAL(10,2) UNSIGNED")
assert data_type["type"] == "DECIMAL"
assert data_type["precision"] == 10
assert data_type["scale"] == 2
assert data_type["length"] is None
assert data_type["flags"] == ["UNSIGNED"]

try:
  mysql.parse_ddl_script("CREATE FUNCTION IF NOT EXISTS f() RETURNS INT RETURN 1", {"version": "8.0.27"})
  raise AssertionError("parse_ddl_script should honor the requested parser version")
except RuntimeError:
  pass

ansi_quoted_ddl = mysql.parse_ddl_script('CREATE TABLE "quoted table" (id INT)', {"ansiQuotes": True})
assert ansi_quoted_ddl["changes"][0]["create"]["name"] == "quoted table"

try:
  mysql.parse_ddl_script('CREATE TABLE "quoted table" (id INT)', {"ansiQuotes": False})
  raise AssertionError("parse_ddl_script should reject double-quoted identifiers when ansiQuotes is disabled")
except RuntimeError:
  pass

assert len(mysql.parse_ddl_script("CREATE EVENT e ON SCHEDULE EVERY 1 DAY DO SELECT 'a\\''", {"noBackslashEscapes": False})["changes"]) == 1

try:
  mysql.parse_ddl_script("CREATE EVENT e ON SCHEDULE EVERY 1 DAY DO SELECT 'a\\''", {"noBackslashEscapes": True})
  raise AssertionError("parse_ddl_script should honor noBackslashEscapes")
except RuntimeError:
  pass

#@<> make_account
mysql.make_account("user", "localhost")
EXPECT_OUTPUT_CONTAINS("'user'@'localhost'")
mysql.make_account("oth'er", "%")
EXPECT_OUTPUT_CONTAINS("'oth\\'er'@'%'")

#<>@ split_account
mysql.split_account("user@localhost")
EXPECT_OUTPUT_CONTAINS("\"user\": \"user\"")
EXPECT_OUTPUT_CONTAINS("\"host\": \"localhost\"")
mysql.split_account("oth'er@%")
EXPECT_OUTPUT_CONTAINS("\"user\": \"oth'er\"")
EXPECT_OUTPUT_CONTAINS("\"host\": \"%\"")
mysql.split_account("'name'@'somehost'")
EXPECT_OUTPUT_CONTAINS("\"user\": \"name\"")
EXPECT_OUTPUT_CONTAINS("\"host\": \"somehost\"")
