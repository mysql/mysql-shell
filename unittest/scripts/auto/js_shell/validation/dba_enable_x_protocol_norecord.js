//@ Installing the plugin via --dba enableXProtocol: plugin already enabled
|enableXProtocol: The X Protocol plugin is already enabled and listening for connections on port|

//@ Uninstalling the x plugin
|Query OK, 0 rows affected|


//@<OUT> Attempt installing the plugin using a busy port {VER(<8.0.11)}
enableXProtocol: Installing plugin mysqlx...
enableXProtocol: Verifying plugin is active...
enableXProtocol: WARNING: The X Protocol plugin is enabled, however a different server is listening for TCP connections at port <<<x_port>>>, check the MySQL Server log for more details

//@<OUT> Re-installing the plugin via --dba enableXProtocol
enableXProtocol: Installing plugin mysqlx...
enableXProtocol: Verifying plugin is active...
enableXProtocol: successfully installed the X protocol plugin!
