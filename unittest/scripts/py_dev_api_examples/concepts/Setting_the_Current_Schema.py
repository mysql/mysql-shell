from mysqlsh import mysqlx

# Direct connect with no client side default schema defined
mySession = mysqlx.get_session('mike:paSSw0rd@localhost')
mySession.set_current_schema("test")
