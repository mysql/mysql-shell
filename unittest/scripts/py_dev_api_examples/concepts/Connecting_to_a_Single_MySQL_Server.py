# Passing the parameters in the { param: value } format
dictSession = mysqlx.get_session( {
        'host': 'localhost', 'port': 33060,
        'dbUser': 'mike', 'dbPassword': 's3cr3t!' } )

db1 = dictSession.get_schema('test')

# Passing the parameters in the URL format
uriSession = mysqlx.get_session('mike:s3cr3t!@localhost:33060')

db2 = uriSession.get_schema('test')
