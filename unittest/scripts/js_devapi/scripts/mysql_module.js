// Assumptions: validateMembers exists
var mysql=require('mysql');

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
    'getSession',
    'help'])

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
