//@ Initialization
||

// ---------------- CLASSIC TESTS URI -------------------------

//@ getClassicSession with URI, no ssl-mode (Use Required) {secure_transport != 'disabled'}
|<<<__default_cipher>>>|

//@ getClassicSession with URI, no ssl-mode (Use Required) {secure_transport == 'disabled'}
||SSL connection error: SSL is required but the server doesn't support it

//@ shell.connect, with classic URI, no ssl-mode (Use Preferred) {secure_transport != 'disabled'}
|<<<__default_cipher>>>|

//@ shell.connect, with classic URI, no ssl-mode (Use Preferred) {secure_transport == 'disabled'}
|~<<<__default_cipher>>>|

//@ getClassicSession with URI, ssl-mode=PREFERRED {secure_transport != 'disabled'}
|<<<__default_cipher>>>|

//@ getClassicSession with URI, ssl-mode=PREFERRED {secure_transport == 'disabled'}
|~<<<__default_cipher>>>|


//@ getClassicSession with URI, no ssl-mode with ssl-ca (Use Verify_Ca) {secure_transport != 'disabled'}
|<<<__default_cipher>>>|

//@ getClassicSession with URI, no ssl-mode with ssl-ca (Use Verify_Ca) {secure_transport == 'disabled'}
||SSL connection error: SSL is required but the server doesn't support it

//@ shell.connect, with classic URI, no ssl-mode with ssl-ca (Use Verify_Ca) {secure_transport != 'disabled'}
|<<<__default_cipher>>>|

//@ shell.connect, with classic URI, no ssl-mode with ssl-ca (Use Verify_Ca) {secure_transport == 'disabled'}
||SSL connection error: SSL is required but the server doesn't support it

//@ getClassicSession with URI, ssl-mode=DISABLED {secure_transport=='on'}
||Connections using insecure transport are prohibited while --require_secure_transport=ON.

//@ getClassicSession with URI, ssl-mode=DISABLED {secure_transport!='on'}
|~<<<__default_cipher>>>|

//@ shell.connect, with classic URI, ssl-mode=DISABLED {secure_transport=='on'}
||Connections using insecure transport are prohibited while --require_secure_transport=ON.

//@ shell.connect, with classic URI, ssl-mode=DISABLED {secure_transport!='on'}
|~<<<__default_cipher>>>|

//@ getClassicSession with URI, ssl-mode=DISABLED and other ssl option
||SSL options are not allowed when ssl-mode is set to 'disabled'.

//@ shell.connect, with classic URI, ssl-mode=DISABLED and other ssl option
||SSL options are not allowed when ssl-mode is set to 'disabled'.

//@ getClassicSession with URI, ssl-mode=REQUIRED and ssl-ca
||Invalid ssl-mode, value should be either 'verify_ca' or 'verify_identity' when any of 'ssl-ca', 'ssl-capath', 'ssl-crl' or 'ssl-crlpath' are provided.

//@ shell.connect, with classic URI, ssl-mode=REQUIRED and ssl-ca
||Invalid ssl-mode, value should be either 'verify_ca' or 'verify_identity' when any of 'ssl-ca', 'ssl-capath', 'ssl-crl' or 'ssl-crlpath' are provided.

//@ getClassicSession using URI with duplicated parameters
||Invalid URI: The SSL Connection option 'ssl-mode' is already defined as 'REQUIRED'.

//@ shell.connect using URI with duplicated parameters
||Invalid URI: The SSL Connection option 'ssl-mode' is already defined as 'REQUIRED'.


// ---------------- CLASSIC TESTS DICT -------------------------

//@ getClassicSession with Dict, no ssl-mode (Use Required) {secure_transport != 'disabled'}
|<<<__default_cipher>>>|

//@ getClassicSession with Dict, no ssl-mode (Use Required) {secure_transport == 'disabled'}
||SSL connection error: SSL is required but the server doesn't support it

//@ shell.connect, with classic Dict, no ssl-mode (Use Preferred) {secure_transport != 'disabled'}
|<<<__default_cipher>>>|

//@ shell.connect, with classic Dict, no ssl-mode (Use Preferred) {secure_transport == 'disabled'}
|~<<<__default_cipher>>>|

//@ getClassicSession with Dict, ssl-mode=PREFERRED {secure_transport != 'disabled'}
|<<<__default_cipher>>>|

//@ getClassicSession with Dict, ssl-mode=PREFERRED {secure_transport == 'disabled'}
|~<<<__default_cipher>>>|

//@ getClassicSession with Dict, no ssl-mode with ssl-ca (Use Verify_Ca) {secure_transport != 'disabled'}
|<<<__default_cipher>>>|

//@ getClassicSession with Dict, no ssl-mode with ssl-ca (Use Verify_Ca) {secure_transport == 'disabled'}
||SSL connection error: SSL is required but the server doesn't support it

//@ shell.connect, with classic Dict, no ssl-mode with ssl-ca (Use Verify_Ca) {secure_transport != 'disabled'}
|<<<__default_cipher>>>|

//@ shell.connect, with classic Dict, no ssl-mode with ssl-ca (Use Verify_Ca) {secure_transport == 'disabled'}
||SSL connection error: SSL is required but the server doesn't support it

//@ getClassicSession with Dict, ssl-mode=DISABLED {secure_transport=='on'}
||Connections using insecure transport are prohibited while --require_secure_transport=ON.

//@ getClassicSession with Dict, ssl-mode=DISABLED {secure_transport!='on'}
|~<<<__default_cipher>>>|

//@ shell.connect, with classic Dict, ssl-mode=DISABLED {secure_transport=='on'}
||Connections using insecure transport are prohibited while --require_secure_transport=ON.

//@ shell.connect, with classic Dict, ssl-mode=DISABLED {secure_transport!='o'}
|~<<<__default_cipher>>>|

//@ getClassicSession with Dict, ssl-mode=DISABLED and other ssl option
||SSL options are not allowed when ssl-mode is set to 'disabled'.

//@ shell.connect, with classic Dict, ssl-mode=DISABLED and other ssl option
||SSL options are not allowed when ssl-mode is set to 'disabled'.

//@ getClassicSession with Dict, ssl-mode=REQUIRED and ssl-ca
||Invalid ssl-mode, value should be either 'verify_ca' or 'verify_identity' when any of 'ssl-ca', 'ssl-capath', 'ssl-crl' or 'ssl-crlpath' are provided.

//@ shell.connect, with classic Dict, ssl-mode=REQUIRED and ssl-ca
||Invalid ssl-mode, value should be either 'verify_ca' or 'verify_identity' when any of 'ssl-ca', 'ssl-capath', 'ssl-crl' or 'ssl-crlpath' are provided.

//@ getClassicSession using dictionary with duplicated parameters
||The SSL Connection option 'ssl-mode' is already defined as 'DISABLED'.

//@ shell.connect using dictionary with duplicated parameters
||The SSL Connection option 'ssl-mode' is already defined as 'DISABLED'.

// ---------------- X TESTS URI -------------------------

//@ getSession with URI, no ssl-mode (Use Required) {secure_transport != 'disabled'}
|<<<__default_cipher>>>|

//@ getSession with URI, no ssl-mode (Use Required) {secure_transport == 'disabled'}
||Capability prepare failed for 'tls'

//@ shell.connect, with X URI, no ssl-mode (Use Preferred) {secure_transport != 'disabled'}
|<<<__default_cipher>>>|

//@ shell.connect, with X URI, no ssl-mode (Use Preferred) {secure_transport == 'disabled'}
|~<<<__default_cipher>>>|

//@ getSession with URI, ssl-mode=PREFERRED {secure_transport != 'disabled'}
|<<<__default_cipher>>>|

//@ getSession with URI, ssl-mode=PREFERRED {secure_transport == 'disabled'}
|~<<<__default_cipher>>>|

//@ getSession with URI, no ssl-mode with ssl-ca (Use Verify_Ca) {secure_transport != 'disabled'}
|<<<__default_cipher>>>|

//@ getSession with URI, no ssl-mode with ssl-ca (Use Verify_Ca) {secure_transport == 'disabled'}
||Capability prepare failed for 'tls'

//@ shell.connect, with X URI, no ssl-mode with ssl-ca (Use Verify_Ca) {secure_transport != 'disabled'}
|<<<__default_cipher>>>|

//@ shell.connect, with X URI, no ssl-mode with ssl-ca (Use Verify_Ca) {secure_transport == 'disabled'}
||Capability prepare failed for 'tls'

//@ getSession with URI, ssl-mode=DISABLED {VER(<8.0.4) && secure_transport == 'on'}
||Secure transport required. To log in you must use TCP+SSL or UNIX socket connection.

//@ getSession with URI, ssl-mode=DISABLED {VER(>=8.0.4) && secure_transport == 'on'}
||Connections using insecure transport are prohibited while --require_secure_transport=ON.

//@ getSession with URI, ssl-mode=DISABLED {secure_transport != 'on'}
|~<<<__default_cipher>>>|

//@ shell.connect, with X URI, ssl-mode=DISABLED {VER(<8.0.4) && secure_transport == 'on'}
||Secure transport required. To log in you must use TCP+SSL or UNIX socket connection.
||ERROR: SqlExecute.execute: Not connected

//@ shell.connect, with X URI, ssl-mode=DISABLED {VER(>=8.0.4) && secure_transport == 'on'}
||Connections using insecure transport are prohibited while --require_secure_transport=ON.


//@ shell.connect, with X URI, ssl-mode=DISABLED {secure_transport != 'on'}
|~<<<__default_cipher>>>|

//@ getSession with URI, ssl-mode=DISABLED and other ssl option
||SSL options are not allowed when ssl-mode is set to 'disabled'.

//@ shell.connect, with X URI, ssl-mode=DISABLED and other ssl option
||SSL options are not allowed when ssl-mode is set to 'disabled'.

//@ getSession with URI, ssl-mode=REQUIRED and ssl-ca
||Invalid ssl-mode, value should be either 'verify_ca' or 'verify_identity' when any of 'ssl-ca', 'ssl-capath', 'ssl-crl' or 'ssl-crlpath' are provided.

//@ shell.connect, with X URI, ssl-mode=REQUIRED and ssl-ca
||Invalid ssl-mode, value should be either 'verify_ca' or 'verify_identity' when any of 'ssl-ca', 'ssl-capath', 'ssl-crl' or 'ssl-crlpath' are provided.

//@ getSession using URI with duplicated parameters
||Invalid URI: The SSL Connection option 'ssl-mode' is already defined as 'REQUIRED'.

// ---------------- X TESTS DICT -------------------------

//@ getSession with Dict, no ssl-mode (Use Required) {secure_transport != 'disabled'}
|<<<__default_cipher>>>|

//@ getSession with Dict, no ssl-mode (Use Required) {secure_transport == 'disabled'}
||Capability prepare failed for 'tls'

//@ shell.connect, with X Dict, no ssl-mode (Use Preferred) {secure_transport != 'disabled'}
|<<<__default_cipher>>>|

//@ shell.connect, with X Dict, no ssl-mode (Use Preferred) {secure_transport == 'disabled'}
|~<<<__default_cipher>>>|

//@ getSession with Dict, ssl-mode=PREFERRED {secure_transport != 'disabled'}
|<<<__default_cipher>>>|

//@ getSession with Dict, ssl-mode=PREFERRED {secure_transport == 'disabled'}
|~<<<__default_cipher>>>|

//@ getSession with Dict, no ssl-mode with ssl-ca (Use Verify_Ca) {secure_transport != 'disabled'}
|<<<__default_cipher>>>|

//@ getSession with Dict, no ssl-mode with ssl-ca (Use Verify_Ca) {secure_transport == 'disabled'}
||Capability prepare failed for 'tls'

//@ shell.connect, with X Dict, no ssl-mode with ssl-ca (Use Verify_Ca) {secure_transport != 'disabled'}
|<<<__default_cipher>>>|

//@ shell.connect, with X Dict, no ssl-mode with ssl-ca (Use Verify_Ca) {secure_transport == 'disabled'}
||Capability prepare failed for 'tls'

//@ getSession with Dict, ssl-mode=DISABLED {VER(<8.0.4) && secure_transport == 'on'}
||Secure transport required. To log in you must use TCP+SSL or UNIX socket connection.

//@ getSession with Dict, ssl-mode=DISABLED {VER(>=8.0.4) && secure_transport == 'on'}
||Connections using insecure transport are prohibited while --require_secure_transport=ON.


//@ getSession with Dict, ssl-mode=DISABLED {secure_transport != 'on'}
|~<<<__default_cipher>>>|

//@ shell.connect, with X Dict, ssl-mode=DISABLED {VER(<8.0.4) && secure_transport == 'on'}
||Secure transport required. To log in you must use TCP+SSL or UNIX socket connection.

//@ shell.connect, with X Dict, ssl-mode=DISABLED {VER(>=8.0.4) && secure_transport == 'on'}
||Connections using insecure transport are prohibited while --require_secure_transport=ON.


//@ shell.connect, with X Dict, ssl-mode=DISABLED {secure_transport != 'on'}
|~<<<__default_cipher>>>|

//@ getSession with Dict, ssl-mode=DISABLED and other ssl option
||SSL options are not allowed when ssl-mode is set to 'disabled'.

//@ shell.connect, with X Dict, ssl-mode=DISABLED and other ssl option
||SSL options are not allowed when ssl-mode is set to 'disabled'.

//@ getSession with Dict, ssl-mode=REQUIRED and ssl-ca
||Invalid ssl-mode, value should be either 'verify_ca' or 'verify_identity' when any of 'ssl-ca', 'ssl-capath', 'ssl-crl' or 'ssl-crlpath' are provided.

//@ shell.connect, with X Dict, ssl-mode=REQUIRED and ssl-ca
||Invalid ssl-mode, value should be either 'verify_ca' or 'verify_identity' when any of 'ssl-ca', 'ssl-capath', 'ssl-crl' or 'ssl-crlpath' are provided.

//@ getSession using dictionary with duplicated parameters
||The SSL Connection option 'ssl-mode' is already defined as 'DISABLED'.

//@<OUT> WL12446-TS3_1 mysql.getClassicSession
+-----------------+------------+
| ATTR_NAME       | ATTR_VALUE |
+-----------------+------------+
| _client_name    | libmysql   |
| _client_version | [[*]]
| _os             | [[*]]
| _pid            | [[*]]
| _platform       | [[*]]
?{__os_type=='windows'}
| _thread         | [[*]]
?{}
| att1            | value      |
| att2            | NULL       |
| att3            | 45         |
| att4            | <val>      |
| att5            | NULL       |
| program_name    | mysqlsh    |
+-----------------+------------+

//@<OUT> WL12446-TS3_1 mysqlx.getSession {connection_attributes_supported}
+-----------------+-----------------+
| ATTR_NAME       | ATTR_VALUE      |
+-----------------+-----------------+
| _client_license | [[*]]
| _client_name    | libmysqlxclient |
| _client_version | [[*]]
| _os             | [[*]]
| _pid            | [[*]]
| _platform       | [[*]]
?{__os_type=='windows'}
| _thread         | [[*]]
?{}
| att1            | value           |
| att2            | NULL            |
| att3            | 45              |
| att4            | <val>           |
| att5            | NULL            |
| program_name    | mysqlsh         |
+-----------------+-----------------+
                                    
//@ WL12446-TS3_1 mysqlx.getSession {!connection_attributes_supported}
||<<<connection_attributes_error>>>

//@ WL12446-TS7_1 Connection Attributes Starting with _ (classic)
||Invalid URI: Key names in 'connection-attributes' cannot start with '_' (ArgumentError)

//@<OUT> WL12446-TS10_1_1 Disabled Connection Attributes using false (Classical)
+-----------------+------------+
| ATTR_NAME       | ATTR_VALUE |
+-----------------+------------+
| _client_name    | libmysql   |
| _client_version | [[*]]
| _os             | [[*]]
| _pid            | [[*]]
| _platform       | [[*]]
?{__os_type=='windows'}
| _thread         | [[*]]
?{}
+-----------------+------------+

//@<OUT> WL12446-TS10_1_1 Disabled Connection Attributes using false (X)
No attributes found!

//@<OUT> WL12446-TS11_1 Default Connection Attributes Behavior X
+-----------------+-----------------+
| ATTR_NAME       | ATTR_VALUE      |
+-----------------+-----------------+
| _client_license | [[*]]
| _client_name    | libmysqlxclient |
| _client_version | [[*]]
| _os             | [[*]]
| _pid            | [[*]]
| _platform       | [[*]]
?{__os_type=='windows'}
| _thread         | [[*]]
?{}
| program_name    | mysqlsh         |
+-----------------+-----------------+

//@<OUT> WL12446-TS11_1 Default Connection Attributes Behavior Classic
+-----------------+------------+
| ATTR_NAME       | ATTR_VALUE |
+-----------------+------------+
| _client_name    | libmysql   |
| _client_version | [[*]]
| _os             | [[*]]
| _pid            | [[*]]
| _platform       | [[*]]
?{__os_type=='windows'}
| _thread         | [[*]]
?{}
| program_name    | mysqlsh    |
+-----------------+------------+

//@ WL12446-TS12_1 Invalid Value for Connection Attributes
||Invalid URI: The value of 'connection-attributes' must be either a boolean or a list of key-value pairs. (ArgumentError)

//@ WL12446-TS13_1 Duplicate Key
||Invalid URI: Duplicate key 'key1' used in 'connection-attributes'. (ArgumentError)
                                    
//@ WL12446-TS_E1 Attribute Longer Than Allowed X
||Key name beginning with 'att01234567890123456798012345678'... is too long, currently limited to 32 (MySQL Error 5005)

//@<OUT> WL12446-TS_E1 Attribute Longer Than Allowed Classic
+----------------------------------+------------+
| ATTR_NAME                        | ATTR_VALUE |
+----------------------------------+------------+
| _client_name                     | libmysql   |
| _client_version                  | [[*]]
| _os                              | [[*]]
| _pid                             | [[*]]
| _platform                        | [[*]]
?{__os_type=='windows'}
| _thread                          | [[*]]
?{}
| att01234567890123456798012345678 | val        |
| program_name                     | mysqlsh    |
+----------------------------------+------------+

