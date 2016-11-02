from mysqlsh import mysqlx

# Direct connect with no client side default schema defined
mySession = mysqlx.get_node_session('mike:s3cr3t!@localhost')
mySession.set_current_schema("test")
