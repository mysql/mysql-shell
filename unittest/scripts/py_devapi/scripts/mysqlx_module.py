from __future__ import print_function
from mysqlsh import mysqlx

# The tests assume the next variables have been put in place
# on the JS Context
# __uri: <user>@<host>
# __host: <host>
# __port: <port>
# __user: <user>
# __uripwd: <uri>:<pwd>@<host>
# __pwd: <pwd>


#@ mysqlx module: exports
all_members = dir(mysqlx)

# Remove the python built in members
exports = []
for member in all_members:
  if not member.startswith('__'):
    exports.append(member)

print('Exported Items:', len(exports))

print('get_session:', type(mysqlx.get_session), '\n')
print('expr:', type(mysqlx.expr), '\n')
print('dateValue:', type(mysqlx.date_value), '\n')
print('help:', type(mysqlx.date_value), '\n')
print('Type:', mysqlx.Type, '\n')
print('LockContention:', mysqlx.LockContention, '\n')

#@# mysqlx module: expression errors
expr = mysqlx.expr()
expr = mysqlx.expr(5)

#@ mysqlx module: expression
expr = mysqlx.expr('5+6')
print(expr)

#@ mysqlx module: date_value() diffrent parameters
mysqlx.date_value(2025, 10, 15);
mysqlx.date_value(2017, 12, 10, 10, 10, 10);
mysqlx.date_value(2017, 12, 10, 10, 10, 10, 500000);
mysqlx.date_value(2017, 12, 10, 10, 10, 10, 599999);

