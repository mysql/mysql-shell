# Assumptions: test schema exists
import mysqlx

session = mysqlx.getSession({
'dataSourceFile': 'mysqlxconfig.json', 'app': 'myapp',
'dbUser': 'mike', 'dbPassword': 's3cr3t!'})

schema = session.getDefaultSchema()
