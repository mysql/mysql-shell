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
  'help',
  'parse_statement_ast',
  'quote_identifier',
  'split_script',
  'unquote_identifier'])

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
