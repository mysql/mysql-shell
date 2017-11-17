# Assumptions: validateMember exists
from mysqlsh import mysql

# The tests assume the next variables have been put in place
# on the JS Context
# __uri: <user>@<host>
# __host: <host>
# __port: <port>
# __user: <user>
# __uripwd: <uri>:<pwd>@<host>
# __pwd: <pwd>


#@ mysql module: exports
all_exports = dir(mysql)

# Remove the python built in members
exports = []
for member in all_exports:
  if not member.startswith('__'):
    exports.append(member)

# The dir function appends 3 built in members
print 'Exported Items:', len(exports)
validateMember(exports, 'get_classic_session');
validateMember(exports, 'get_session');
validateMember(exports, 'help');

#@<OUT> help
mysql.help()

#@<OUT> Help on get_classic_session
mysql.help('get_classic_session');

#@# get_classic_session errors
mysql.get_classic_session()
mysql.get_classic_session(1, 2, 3)
mysql.get_classic_session(["bla"])
mysql.get_classic_session("some@uri", 25)


#@<OUT> Help on get_session
mysql.help('get_session');

#@# get_session errors
mysql.get_session()
mysql.get_session(1, 2, 3)
mysql.get_session(["bla"])
mysql.get_session("some@uri", 25)
