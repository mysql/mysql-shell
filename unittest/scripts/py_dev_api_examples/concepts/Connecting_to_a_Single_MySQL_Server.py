
# Passing the parameters in the { param: value } format
dictSession = mysqlx.getSession( {
        'host': 'localhost', 'port': 33060,
        'dbUser': 'mike', 'dbPassword': 's3cr3t!' } )

db1 = dictSession.getSchema('test')

# Passing the parameters in the URL format
uriSession = mysqlx.getSession('mike:s3cr3t!@localhost:33060')

db2 = uriSession.getSchema('test')
