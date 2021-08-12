//@ Installing the plugin via --dba enableXProtocol: plugin already enabled
testutil.deployRawSandbox(__mysql_sandbox_port1, "root");
testutil.callMysqlsh(['-i', '--uri', __sandbox_uri1, '--dba', 'enableXProtocol']);

//@ Uninstalling the x plugin {VER(<8.0.11)}
testutil.callMysqlsh(['--interactive=full', '--uri', __sandbox_uri1, '--sqlc', '-e', 'uninstall plugin mysqlx;']);

//@ Attempt installing the plugin using a busy port {VER(<8.0.11)}
let x_port = __mysql_sandbox_port1 * 10;
testutil.deployRawSandbox(__mysql_sandbox_port2, "root", {mysqlx_port:x_port});
testutil.callMysqlsh(['-i', '--uri', __sandbox_uri1, '--dba', 'enableXProtocol']);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.callMysqlsh(['--interactive=full', '--uri', __sandbox_uri1, '--sqlc', '-e', 'uninstall plugin mysqlx;']);

//@ Re-installing the plugin via --dba enableXProtocol {VER(<8.0.11)}
testutil.callMysqlsh(['-i', '--uri', __sandbox_uri1, '--dba', 'enableXProtocol']);

//@<> Cleanup
testutil.destroySandbox(__mysql_sandbox_port1);
