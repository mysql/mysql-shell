//@ Installing the plugin via --dba enableXProtocol
testutil.callMysqlsh(['-i', '--uri', __mysqluripwd, '--dba', 'enableXProtocol']);

//@ Uninstalling the x plugin {VER(<8.0.5)}
testutil.callMysqlsh(['--interactive=full', '--uri', __mysqluripwd, '--sqlc', '-e', 'uninstall plugin mysqlx;']);

//@ Re-installing the plugin via --dba enableXProtocol {VER(<8.0.5)}
testutil.callMysqlsh(['-i', '--uri', __mysqluripwd, '--dba', 'enableXProtocol']);
