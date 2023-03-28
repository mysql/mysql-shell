// Tests handling for Kerberos GSSAPI authentication on Windows.
// These tests require a specific setup:
//  - An kerberos authentication server (and its host/ip address)
//  - An registered user on the kerberos authentication server (user:test2, password:Testpw1)
//  - An linux server with installed mit kerberos libriaries and mysql server (with kerberos plugin)
//    - the linux server must have an specific realm configuration (/etc/krb5.conf) to work with mentioned kerberos server:
//[realms]
//MYSQL.LOCAL = {
//	kdc = kerberos.auth.server.com
//	admin_server = kerberos.auth.server.com
//	default_domain = MYSQL.LOCAL
//}
//    - mysql server should have an set veriable: authentication_kerberos_service_principal=mysql_service/kerberos_auth_host@MYSQL.LOCAL
//
//  - this script should be run on a windows machine:
//    - the MIT Kerberos Client for windows must be installed and running in the background
//    - mysqlsh(rec) with authentication_kerberos_client plugin
//    - in some cases, the mentioned kerberos server must be added to the /drivers/etc/hosts file on windows
//
// Execute as follows:
//          mysqlshrec -f <thisfile>, from the build dir.
//
// Making sure the path to the mysql server to use is on PATH
//
// Deploys a server with the authentication plugin enabled
var port = 3332;
var host = 'localhost'
var rootpwd = 'root'

testutil.deployRawSandbox(port, "root", { "plugin-load-add": "authentication_kerberos.so" });

// Create a user with the authentication plugin
shell.connect(`root:${rootpwd}@${host}:${port}`);
session.runSql("create user test2 identified by 'Testpw1' and identified with authentication_kerberos");

// Attempt to execute a query
// EXPECTS: a configured user on linux mysql database and configured user on kerberos authentication server connected to it
// PRODUCES:
// user()
// test2@**host**
testutil.callMysqlsh([`test2:Testpw1@${host}:${port}`, "--plugin-authentication-kerberos-client-mode=GSSAPI", "--sql", "-e", 'select user()'])

// Attempt to execute a query
// EXPECTS: a configured user on linux mysql database and configured user on kerberos authentication server connected to it
// PRODUCES:
// If there is no configured kerberos SSPI on system, this will fail
testutil.callMysqlsh([`test2:Testpw1@${host}:${port}`, "--plugin-authentication-kerberos-client-mode=SSPI", "--sql", "-e", 'select user()'])

// Attempt to execute a query
// EXPECTS: a configured user on linux mysql database and configured user on kerberos authentication server connected to it
// PRODUCES:
// Same as above = if there is no configured kerberos SSPI on system, this will fail
testutil.callMysqlsh([`test2:Testpw1@${host}:${port}`, "--sql", "-e", 'select user()'])

// Drops the sandbox
testutil.destroySandbox(port);