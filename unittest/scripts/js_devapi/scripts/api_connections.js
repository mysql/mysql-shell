// Assumptions: smart deployment routines available
//@ Initialization
var uri_base = __my_user + ":root@localhost:";
var connection_data = {user:__my_user,
                       password: 'root',
                       host:'localhost'};

var uri_base_no_pwd = __my_user + "@localhost:";
var connection_data_no_pwd = {user:__my_user,
                              host:'localhost'};

// ---------------- CLASSIC TESTS URI -------------------------

//@ getClassicSession with URI, no ssl-mode (Use Required)
var uri = uri_base_no_pwd + __my_port;
var mySession = mysql.getClassicSession(uri, 'root');
mySession.runSql("show status like 'Ssl_cipher'");
mySession.close();

//@ shell.connect, with classic URI, no ssl-mode (Use Preferred)
var uri = 'mysql://' + uri_base_no_pwd + __my_port;
shell.connect(uri, 'root')
session.runSql("show status like 'Ssl_cipher'");
session.close();

//@ getClassicSession with URI, ssl-mode=PREFERRED
var uri = uri_base + __my_port + '?ssl-mode=PREFERRED';
var mySession = mysql.getClassicSession(uri);
mySession.runSql("show status like 'Ssl_cipher'");
mySession.close();

//@ getClassicSession with URI, no ssl-mode with ssl-ca (Use Verify_Ca)
var uri = uri_base + __my_port + '?ssl-ca=' + __my_ca_file_uri;
println (uri);
var mySession = mysql.getClassicSession(uri);
mySession.runSql("show status like 'Ssl_cipher'");
mySession.close();

//@ shell.connect, with classic URI, no ssl-mode with ssl-ca (Use Verify_Ca)
var uri = 'mysql://' + uri_base + __my_port + '?ssl-ca=' + __my_ca_file_uri;
shell.connect(uri)
session.runSql("show status like 'Ssl_cipher'");
session.close();

//@ getClassicSession with URI, ssl-mode=DISABLED
var uri = uri_base + __my_port + '?ssl-mode=DISABLED';
var mySession = mysql.getClassicSession(uri);
mySession.runSql("show status like 'Ssl_cipher'");
mySession.close();

//@ shell.connect, with classic URI, ssl-mode=DISABLED
var uri = 'mysql://' + uri_base + __my_port + '?ssl-mode=DISABLED';
shell.connect(uri)
session.runSql("show status like 'Ssl_cipher'");
session.close();

//@ getClassicSession with URI, ssl-mode=DISABLED and other ssl option
var uri = uri_base + __my_port + '?ssl-mode=DISABLED&ssl-ca=one';
var mySession = mysql.getClassicSession(uri);
mySession.runSql("show status like 'Ssl_cipher'");
mySession.close();

//@ shell.connect, with classic URI, ssl-mode=DISABLED and other ssl option
var uri = 'mysql://' + uri_base + __my_port + '?ssl-mode=DISABLED&ssl-ca=one';
shell.connect(uri)
session.runSql("show status like 'Ssl_cipher'");
session.close();

//@ getClassicSession with URI, ssl-mode=REQUIRED and ssl-ca
var uri = uri_base + __my_port + '?ssl-mode=REQUIRED&ssl-ca=one';
var mySession = mysql.getClassicSession(uri);
mySession.runSql("show status like 'Ssl_cipher'");
mySession.close();

//@ shell.connect, with classic URI, ssl-mode=REQUIRED and ssl-ca
var uri = 'mysql://' + uri_base + __my_port + '?ssl-mode=REQUIRED&ssl-ca=one';
shell.connect(uri)
session.runSql("show status like 'Ssl_cipher'");
session.close();

//@ getClassicSession using URI with duplicated parameters
var uri = 'mysql://' + uri_base + __my_port + '?ssl-mode=REQUIRED&ssl-mode=DISABLED';
shell.connect(uri)
var mySslSession = mysql.getClassicSession(uri);

//@ shell.connect using URI with duplicated parameters
var mySslSession = mysql.getClassicSession(uri);

// ---------------- CLASSIC TESTS DICT -------------------------

//@ getClassicSession with Dict, no ssl-mode (Use Required)
connection_data_no_pwd['port'] = __my_port;
var mySession = mysql.getClassicSession(connection_data_no_pwd, 'root');
mySession.runSql("show status like 'Ssl_cipher'");
mySession.close();

//@ shell.connect, with classic Dict, no ssl-mode (Use Preferred)
shell.connect(connection_data_no_pwd, 'root')
session.runSql("show status like 'Ssl_cipher'");
session.close();

//@ getClassicSession with Dict, ssl-mode=PREFERRED
connection_data['port'] = __my_port;
connection_data['ssl-mode'] = 'PREFERRED';
var mySession = mysql.getClassicSession(connection_data);
mySession.runSql("show status like 'Ssl_cipher'");
mySession.close();

//@ getClassicSession with Dict, no ssl-mode with ssl-ca (Use Verify_Ca)
delete connection_data['ssl-mode'];
connection_data['ssl-ca'] = __my_ca_file;
var mySession = mysql.getClassicSession(connection_data);
mySession.runSql("show status like 'Ssl_cipher'");
mySession.close();

//@ shell.connect, with classic Dict, no ssl-mode with ssl-ca (Use Verify_Ca)
shell.connect(connection_data)
session.runSql("show status like 'Ssl_cipher'");
session.close();
delete connection_data['ssl-ca'];

//@ getClassicSession with Dict, ssl-mode=DISABLED
connection_data['ssl-mode'] = 'DISABLED';
var mySession = mysql.getClassicSession(connection_data);
mySession.runSql("show status like 'Ssl_cipher'");
mySession.close();

//@ shell.connect, with classic Dict, ssl-mode=DISABLED
shell.connect(connection_data)
session.runSql("show status like 'Ssl_cipher'");
session.close();

//@ getClassicSession with Dict, ssl-mode=DISABLED and other ssl option
connection_data['ssl-ca'] = 'one';
var mySession = mysql.getClassicSession(connection_data);
mySession.runSql("show status like 'Ssl_cipher'");
mySession.close();

//@ shell.connect, with classic Dict, ssl-mode=DISABLED and other ssl option
shell.connect(connection_data)
session.runSql("show status like 'Ssl_cipher'");
session.close();

//@ getClassicSession with Dict, ssl-mode=REQUIRED and ssl-ca
connection_data['ssl-mode'] = 'REQUIRED';
var mySession = mysql.getClassicSession(connection_data);
mySession.runSql("show status like 'Ssl_cipher'");
mySession.close();

//@ shell.connect, with classic Dict, ssl-mode=REQUIRED and ssl-ca
shell.connect(connection_data)
session.runSql("show status like 'Ssl_cipher'");
session.close();
delete connection_data['ssl-ca'];

//@ getClassicSession using dictionary with duplicated parameters
connection_data['Ssl-Mode'] = "DISABLED";
var mySession = mysql.getClassicSession(connection_data)

//@ shell.connect using dictionary with duplicated parameters
shell.connect(connection_data)

delete connection_data['Ssl-Mode'];
delete connection_data['ssl-mode'];

// ---------------- X TESTS URI -------------------------

//@ getSession with URI, no ssl-mode (Use Required)
var uri = uri_base_no_pwd + __my_x_port;
var mySession = mysqlx.getSession(uri, 'root');
mySession.sql("show status like 'Mysqlx_ssl_cipher'");
mySession.close();

//@ shell.connect, with X URI, no ssl-mode (Use Preferred)
var uri = 'mysqlx://' + uri_base_no_pwd + __my_x_port;
shell.connect(uri, 'root')
session.sql("show status like 'Mysqlx_ssl_cipher'");
session.close();

//@ getSession with URI, ssl-mode=PREFERRED
var uri = uri_base + __my_x_port + '?ssl-mode=PREFERRED';
var mySession = mysqlx.getSession(uri);
mySession.sql("show status like 'Mysqlx_ssl_cipher'");
mySession.close();

//@ getSession with URI, no ssl-mode with ssl-ca (Use Verify_Ca)
var uri = uri_base + __my_x_port + '?ssl-ca=' + __my_ca_file_uri;
var mySession = mysqlx.getSession(uri);
mySession.sql("show status like 'Mysqlx_ssl_cipher'");
mySession.close();

//@ shell.connect, with X URI, no ssl-mode with ssl-ca (Use Verify_Ca)
var uri = 'mysqlx://' + uri_base + __my_x_port + '?ssl-ca=' + __my_ca_file_uri;
shell.connect(uri)
session.sql("show status like 'Mysqlx_ssl_cipher'");
session.close();

//@ getSession with URI, ssl-mode=DISABLED
var uri = uri_base + __my_x_port + '?ssl-mode=DISABLED';
var mySession = mysqlx.getSession(uri);
mySession.sql("show status like 'Mysqlx_ssl_cipher'");
mySession.close();

//@ shell.connect, with X URI, ssl-mode=DISABLED
var uri = 'mysqlx://' + uri_base + __my_x_port + '?ssl-mode=DISABLED';
shell.connect(uri)
session.sql("show status like 'Mysqlx_ssl_cipher'");
session.close();

//@ getSession with URI, ssl-mode=DISABLED and other ssl option
var uri = uri_base + __my_x_port + '?ssl-mode=DISABLED&ssl-ca=one';
var mySession = mysqlx.getSession(uri);
mySession.sql("show status like 'Mysqlx_ssl_cipher'");
mySession.close();

//@ shell.connect, with X URI, ssl-mode=DISABLED and other ssl option
var uri = 'mysqlx://' + uri_base + __my_x_port + '?ssl-mode=DISABLED&ssl-ca=one';
shell.connect(uri)
session.sql("show status like 'Mysqlx_ssl_cipher'");
session.close();

//@ getSession with URI, ssl-mode=REQUIRED and ssl-ca
var uri = uri_base + __my_x_port + '?ssl-mode=REQUIRED&ssl-ca=one';
var mySession = mysqlx.getSession(uri);
mySession.sql("show status like 'Mysqlx_ssl_cipher'");
mySession.close();

//@ shell.connect, with X URI, ssl-mode=REQUIRED and ssl-ca
var uri = 'mysqlx://' + uri_base + __my_x_port + '?ssl-mode=REQUIRED&ssl-ca=one';
shell.connect(uri)
session.sql("show status like 'Mysqlx_ssl_cipher'");
session.close();

//@ getSession using URI with duplicated parameters
var uri = 'mysql://' + uri_base + __my_port + '?ssl-mode=REQUIRED&ssl-mode=DISABLED';
shell.connect(uri)
var mySslSession = mysql.getClassicSession(uri);

// ---------------- X TESTS DICT -------------------------

//@ getSession with Dict, no ssl-mode (Use Required)
connection_data_no_pwd['port'] = __my_x_port;
var mySession = mysqlx.getSession(connection_data_no_pwd, 'root');
mySession.sql("show status like 'Mysqlx_ssl_cipher'");
mySession.close();

//@ shell.connect, with X Dict, no ssl-mode (Use Preferred)
shell.connect(connection_data_no_pwd, 'root')
session.sql("show status like 'Mysqlx_ssl_cipher'");
session.close();

//@ getSession with Dict, ssl-mode=PREFERRED
connection_data['port'] = __my_x_port;
connection_data['ssl-mode'] = 'PREFERRED';
var mySession = mysqlx.getSession(connection_data);
mySession.sql("show status like 'Mysqlx_ssl_cipher'");
mySession.close();

//@ getSession with Dict, no ssl-mode with ssl-ca (Use Verify_Ca)
delete connection_data['ssl-mode'];
connection_data['ssl-ca'] = __my_ca_file;
var mySession = mysqlx.getSession(connection_data);
mySession.sql("show status like 'Mysqlx_ssl_cipher'");
mySession.close();

//@ shell.connect, with X Dict, no ssl-mode with ssl-ca (Use Verify_Ca)
shell.connect(connection_data)
session.sql("show status like 'Mysqlx_ssl_cipher'");
session.close();
delete connection_data['ssl-ca'];

//@ getSession with Dict, ssl-mode=DISABLED
connection_data['ssl-mode'] = 'DISABLED';
var mySession = mysqlx.getSession(connection_data);
mySession.sql("show status like 'Mysqlx_ssl_cipher'");
mySession.close();

//@ shell.connect, with X Dict, ssl-mode=DISABLED
shell.connect(connection_data)
session.sql("show status like 'Mysqlx_ssl_cipher'");
session.close();

//@ getSession with Dict, ssl-mode=DISABLED and other ssl option
connection_data['ssl-ca'] = 'one';
var mySession = mysqlx.getSession(connection_data);
mySession.sql("show status like 'Mysqlx_ssl_cipher'");
mySession.close();

//@ shell.connect, with X Dict, ssl-mode=DISABLED and other ssl option
shell.connect(connection_data)
session.sql("show status like 'Mysqlx_ssl_cipher'");
session.close();

//@ getSession with Dict, ssl-mode=REQUIRED and ssl-ca
connection_data['ssl-mode'] = 'REQUIRED';
var mySession = mysqlx.getSession(connection_data);
mySession.sql("show status like 'Mysqlx_ssl_cipher'");
mySession.close();

//@ shell.connect, with X Dict, ssl-mode=REQUIRED and ssl-ca
shell.connect(connection_data)
session.sql("show status like 'Mysqlx_ssl_cipher'");
session.close();
delete connection_data['ssl-ca'];

//@ getSession using dictionary with duplicated parameters
connection_data['Ssl-Mode'] = "DISABLED";
var mySession = mysql.getClassicSession(connection_data)

delete connection_data['Ssl-Mode'];
delete connection_data['ssl-mode'];
