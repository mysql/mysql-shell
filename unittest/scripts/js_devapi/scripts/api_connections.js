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
session.sql("show status like 'Mysqlx_ssl_cipher'").execute();
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

//@<> Connection Attributes Tests
get_connection_atts = "select ATTR_NAME, ATTR_VALUE from performance_schema.session_account_connect_attrs where processlist_id=connection_id() order by ATTR_NAME asc";

function print_attributes(session) {
    var res = session.runSql(get_connection_atts);
    count = shell.dumpRows(res);
    if (count == 0)
        println("No attributes found!");
}

function create_uri(port, connection_attributes) {
    var uri = uri_base_no_pwd + port;
    if (connection_attributes) {
        uri = uri + "?connection-attributes=" + connection_attributes;
    } else {
        uri = uri + "?connection-attributes";
    }

    if (secure_transport == 'disabled') {
        uri = uri + "&ssl-mode=disabled";
    }
    return uri;
}

function test_classic(connection_attributes) {
    var uri = create_uri(__my_port, connection_attributes);
    var mySession = mysql.getClassicSession(uri, 'root');
    print_attributes(mySession);
    mySession.close();
}

function test_x(connection_attributes) {
    var uri = create_uri(__my_x_port, connection_attributes);
    var mySession = mysqlx.getSession(uri, 'root');
    print_attributes(mySession);
    mySession.close();
}

function test_shell_connect(port, connection_attributes) {
    var uri = create_uri(port, connection_attributes);
    shell.connect(uri, 'root');
    print_attributes(session);
    session.close();
}

var connection_attributes_supported = true;
const connection_attributes_error = "Capability 'session_connect_attrs' doesn't exist";
try {
    test_x("[sample=1]");
} catch (err) {
    if (err.message.indexOf(connection_attributes_error) !== -1) {
        connection_attributes_supported = false;
    }
}

//@ WL12446-TS3_1 mysql.getClassicSession
// TS5_1, TS8_1, TS8_3, TS9_1
test_classic("[att1=value,att2,att3=45,%61%74%74%34=%3cval%3e,att5=]");

//@ WL12446-TS3_1 shell.connect (classic) [USE:WL12446-TS3_1 mysql.getClassicSession]
// TS5_1, TS8_1, TS8_3, TS9_1
test_shell_connect(__my_port, "[att1=value,att2,att3=45,%61%74%74%34=%3cval%3e,att5=]");

//@ WL12446-TS3_1 mysqlx.getSession
// TS5_1, TS8_1, TS8_3, TS9_1
test_x("[att1=value,att2,att3=45,%61%74%74%34=%3cval%3e,att5=]");

//@ WL12446-TS3_1 shell.connect (X) [USE:WL12446-TS3_1 mysqlx.getSession] {connection_attributes_supported}
// TS5_1, TS8_1, TS8_3, TS9_1
test_shell_connect(__my_x_port, "[att1=value,att2,att3=45,%61%74%74%34=%3cval%3e,att5=]");


//@ WL12446-TS7_1 Connection Attributes Starting with _ (classic)
test_classic("[att1=value,att2,_att3=error]");

//@ WL12446-TS7_1 Connection Attributes Starting with _ (X) [USE:WL12446-TS7_1 Connection Attributes Starting with _ (classic)]
test_x("[att1=value,att2,_att3=error]");

//@ WL12446-TS10_1_1 Disabled Connection Attributes using false (Classical)
test_classic("false");

//@ WL12446-TS10_1_1 Disabled Connection Attributes using 0 (Classical) [USE:WL12446-TS10_1_1 Disabled Connection Attributes using false (Classical)]
test_classic("0");

//@ WL12446-TS10_1_1 Disabled Connection Attributes using false (X)
test_x("false");

//@ WL12446-TS10_1_1 Disabled Connection Attributes using 0 (X) [USE:WL12446-TS10_1_1 Disabled Connection Attributes using false (X)]
test_x("0")

//@ WL12446-TS11_1 Default Connection Attributes Behavior X {connection_attributes_supported}
test_x()

//@ WL12446-TS11_1 Default Connection Attributes Behavior X Empty [USE:WL12446-TS11_1 Default Connection Attributes Behavior X] {connection_attributes_supported}
test_x("")

//@ WL12446-TS11_1 Default Connection Attributes Behavior X 1 [USE:WL12446-TS11_1 Default Connection Attributes Behavior X] {connection_attributes_supported}
test_x("1")

//@ WL12446-TS11_1 Default Connection Attributes Behavior X true [USE:WL12446-TS11_1 Default Connection Attributes Behavior X] {connection_attributes_supported}
test_x("true")

//@ WL12446-TS11_1 Default Connection Attributes Behavior X [] [USE:WL12446-TS11_1 Default Connection Attributes Behavior X] {connection_attributes_supported}
test_x("[]")


//@ WL12446-TS11_1 Default Connection Attributes Behavior Classic
test_classic()

//@ WL12446-TS11_1 Default Connection Attributes Behavior Classic Empty [USE:WL12446-TS11_1 Default Connection Attributes Behavior Classic]
test_classic("")

//@ WL12446-TS11_1 Default Connection Attributes Behavior Classic 1 [USE:WL12446-TS11_1 Default Connection Attributes Behavior Classic]
test_classic("1")

//@ WL12446-TS11_1 Default Connection Attributes Behavior Classic true [USE:WL12446-TS11_1 Default Connection Attributes Behavior Classic]
test_classic("true")

//@ WL12446-TS11_1 Default Connection Attributes Behavior Classic [] [USE:WL12446-TS11_1 Default Connection Attributes Behavior Classic]
test_classic("[]")

//@ WL12446-TS12_1 Invalid Value for Connection Attributes
test_classic("custom-value")

//@ WL12446-TS13_1 Duplicate Key
test_classic("[key1=val1,key2=val2,key1=val3]")


//@ WL12446-TS_E1 Attribute Longer Than Allowed X {connection_attributes_supported}
test_x("[att012345678901234567980123456789=val]")

//@ WL12446-TS_E1 Attribute Longer Than Allowed Classic
test_classic("[att012345678901234567980123456789=val]")

//@<> Connection Attributes in Dictionary Data
connection_data = {
    user: __my_user,
    host: "localhost",
    port: __my_port,
    password: 'root',
    "connection-attributes":["att1=value","att2","att3=45","att4=<val>","att5="]
}

if (secure_transport == 'disabled') {
    connection_data["ssl-mode"] = "disabled";
} else if (secure_transport == "on") {
    connection_data["ssl-mode"] = "preferred";
}


//@ WL12446 Test Classic Connection Attributes Dictionary+Attribute List [USE:WL12446-TS3_1 mysql.getClassicSession]
var mySession = mysql.getClassicSession(connection_data);
print_attributes(mySession);
mySession.close();

//@ WL12446 Test X Connection Attributes Dictionary+Attribute List [USE:WL12446-TS3_1 mysqlx.getSession] {connection_attributes_supported}
connection_data.port = __my_x_port;
var mySession = mysqlx.getSession(connection_data);
print_attributes(mySession);
mySession.close();


//@ WL12446 Test Classic Connection Attributes Dictionary+Attribute Map [USE:WL12446-TS3_1 mysql.getClassicSession]
connection_data['connection-attributes'] = {
    att1: "value",
    att2: "",
    att3: 45,
    att4: "<val>",
    att5: ""
}
connection_data.port = __my_port;

var mySession = mysql.getClassicSession(connection_data);
print_attributes(mySession);
mySession.close();


//@ WL12446 Test X Connection Attributes Dictionary+Attribute Map [USE:WL12446-TS3_1 mysqlx.getSession] {connection_attributes_supported}
connection_data.port = __my_x_port;
var mySession = mysqlx.getSession(connection_data);
print_attributes(mySession);
mySession.close();
