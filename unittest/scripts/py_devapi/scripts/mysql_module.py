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

print 'get_classic_session:', type(mysql.get_classic_session)
print 'help:', type(mysql.get_classic_session)
