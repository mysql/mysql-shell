// Assumptions: ensure auth_socket plugin enabled
// Assumes __uripwd is defined as <user>:<pwd>@<host>:<mysql_port>

//@# Install auth_socket plugin if needed
ensure_plugin_enabled("auth_socket");

//@# Classic connection
var sess = mysql.getClassicSession(__uripwd)
sess
sess.isOpen()

//@# Create user identified with auth_socket
sess.runSql("CREATE USER '" + __system_user + "'@'localhost' IDENTIFIED WITH auth_socket;")
sess.runSql("CREATE USER '" + __system_user + "'@'%' IDENTIFIED WITH auth_socket;")

//@# Get unix socket path
var sock = sess.runSql("select @@socket").fetchOne()[0]
sock
!!sock
sess.close()

//@# Connect using auth_socket plugin
sess = mysql.getClassicSession(__system_user + ":@("+ __socket +")")
sess.isOpen()

//@# Close session
sess.close()
sess

//@# Cleanup
sess = mysql.getClassicSession(__uripwd)
sess.runSql("DROP USER '" + __system_user + "'@'localhost'")
sess.runSql("DROP USER '" + __system_user + "'@'%'")
sess.close()
sess

//@# Uninstall auth_socket plugin
ensure_plugin_disabled("auth_socket");
