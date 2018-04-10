// Passing the parameters in the { param: value } format
var dictSession = mysqlx.getSession( {
        host: 'localhost', 'port': 33060,
        user: 'mike', password: 'paSSw0rd' } )

var db1 = dictSession.getSchema('test')

// Passing the parameters in the URL format
var uriSession = mysqlx.getSession('mike:paSSw0rd@localhost:33060')

var db2 = uriSession.getSchema('test')
