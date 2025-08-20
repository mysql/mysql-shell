// Assumptions: validateMembers exists
var mysql = require('mysql');

// The tests assume the next variables have been put in place
// on the JS Context
// __uri: <user>@<host>
// __host: <host>
// __port: <port>
// __user: <user>
// __uripwd: <uri>:<pwd>@<host>

//@<> mysql module: exports
validateMembers(mysql, [
    'getClassicSession',
    'ErrorCode',
    'Type',
    'getSession',
    'help',
    'parseStatementAst',
    'tokenizeStatement',
    'quoteIdentifier',
    'splitScript',
    'unquoteIdentifier',
    'makeAccount',
    'splitAccount'
])

//@# getClassicSession errors
mysql.getClassicSession()
mysql.getClassicSession(1, 2, 3)
mysql.getClassicSession(["bla"])
mysql.getClassicSession("some@uri", 25)

//@# getSession errors
mysql.getSession()
mysql.getSession(1, 2, 3)
mysql.getSession(["bla"])
mysql.getSession("some@uri", 25)

//@<> ErrorCode
EXPECT_EQ(1045, mysql.ErrorCode.ER_ACCESS_DENIED_ERROR)

//@# parseStatementAst errors
mysql.parseStatementAst(1);
mysql.parseStatementAst({});

//@ parseStatementAst
mysql.parseStatementAst("this is not valid sql");
mysql.parseStatementAst("");
mysql.parseStatementAst("SELECT");

//@ tokenizeStatement
mysql.tokenizeStatement("select * from /* comment */ foo.bar /* `\"'` '*/")

//@# tokenizeStatement errors
mysql.tokenizeStatement("/*")
mysql.tokenizeStatement("'")
mysql.tokenizeStatement("`")
mysql.tokenizeStatement('"')
mysql.tokenizeStatement('\\')

//@<> BUG#37018247 - quoted literals when ANSI quotes is disabled - TODO
EXPECT_NO_THROWS(function () { mysql.parseStatementAst('SELECT "1"'); });

//@ splitScript
mysql.splitScript("select 1")
mysql.splitScript("select 1; select 2;")
mysql.splitScript("delimiter $$\nselect 1$$select 2;select3$$")
mysql.splitScript("DELIMITER A\nSELECT 1 A SELECT 2 A SELECT 3; SELECT 4")

//@# splitScript errors
mysql.splitScript(1)
mysql.splitScript(["select"])

//<>@ makeAccount
mysql.makeAccount("user", "localhost")
EXPECT_OUTPUT_CONTAINS("'user'@'localhost'")
mysql.makeAccount("oth'er", "%")
EXPECT_OUTPUT_CONTAINS("'oth\\'er'@'%'")

//<>@ splitAccount
mysql.splitAccount("user@localhost")
EXPECT_OUTPUT_CONTAINS("\"user\": \"user\"")
EXPECT_OUTPUT_CONTAINS("\"host\": \"localhost\"")
mysql.splitAccount("oth'er@%")
EXPECT_OUTPUT_CONTAINS("\"user\": \"oth'er\"")
EXPECT_OUTPUT_CONTAINS("\"host\": \"%\"")
mysql.splitAccount("'name'@'somehost'")
EXPECT_OUTPUT_CONTAINS("\"user\": \"name\"")
EXPECT_OUTPUT_CONTAINS("\"host\": \"somehost\"")
