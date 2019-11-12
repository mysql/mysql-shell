//@<> Setup
testutil.deploySandbox(__mysql_sandbox_port1, "root");

session1 = mysql.getSession(__sandbox_uri1);

sockpath1 = get_socket_path(session1);
sockpath2 = get_socket_path(mysql.getSession(__mysqluripwd));

// ** Tests for socket connections on MYSQL_UNIX_PORT

//@ mysql.getSession() socket, environment variable {__os_type != "windows" && !__replaying}

// Connect to sb1
testutil.callMysqlsh(["--py", "-e", "s = mysql.get_session('root:root@localhost'); print(s.uri, s.run_sql('select @@port').fetch_one()[0])"], "", ["MYSQL_UNIX_PORT=" + sockpath1]);


//@ mysql.getSession() socket, environment variable, try override {__os_type != "windows" && !__replaying}

// Bug #30533318	UNRELIABLE/MISLEADING CONNECTIONS WITH MYSQL_UNIX_PORT
// MYSQL_UNIX_PORT is cached when libmysqlclient is initialized, so it will be
// ignored if changed. OTOH the shell was incorrectly assuming it did take effect
// and thus, was reporting being connected to the wrong server.

testutil.callMysqlsh(["--py", "-e", "import os; os.putenv('MYSQL_UNIX_PORT', '" + sockpath2 + "'); s = mysql.get_session('root:root@localhost'); print(s.uri, s.run_sql('select @@port').fetch_one()[0])"], "", ["MYSQL_UNIX_PORT=" + sockpath1]);


//@ shell.connect() socket, environment variable {__os_type != "windows" && !__replaying}
testutil.callMysqlsh(["--py", "-e", "shell.connect({'user':'root', 'password':'root', 'scheme':'mysql'}); print(session.uri, session.run_sql('select @@port').fetch_one()[0])"], "", ["MYSQL_UNIX_PORT=" + sockpath1]);

//@<> Cleanup
testutil.setenv("MYSQL_UNIX_PORT");

testutil.destroySandbox(__mysql_sandbox_port1);
