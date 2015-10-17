# Assumptions: test schema exists
import mysqlx

session = mysqlx.getNodeSession({
  'dataSourceFile': 'mysqlxconfig.json', 'app': 'myapp',
  'dbUser': 'mike', 'dbPassword': 's3cr3t!'})

default_schema = session.getDefaultSchema().name
session.setCurrentSchema(default_schema)

# print the current schema name
print session.getCurrentSchema().name