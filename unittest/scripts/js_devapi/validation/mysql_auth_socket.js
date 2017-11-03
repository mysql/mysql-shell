//@# Install auth_socket plugin if needed
||

//@# Classic connection
|true|

//@# Create user identified with auth_socket
|Query OK, 0 rows affected |
|Query OK, 0 rows affected |

//@# Get unix socket path
|true|

//@# Connect using auth_socket plugin
|true|

//@# Close session
|ClassicSession:disconnected>|

//@# Cleanup
|Query OK, 0 rows affected |
|Query OK, 0 rows affected |
|ClassicSession:disconnected>|

//@# Uninstall auth_socket plugin
||
