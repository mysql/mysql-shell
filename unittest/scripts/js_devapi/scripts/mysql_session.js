// Assumptions: ensure_schema_does_not_exist is available
// Assumes __uripwd is defined as <user>:<pwd>@<host>:<mysql_port>
var mysql = require('mysql');

//@<> Session: validating members
var classicSession = mysql.getClassicSession(__uripwd);

validateMembers(classicSession, [
    'close',
    'commit',
    'getClientData',
    'getConnectionId',
    'getUri',
    'getSqlMode',
    'getSshUri',
    'help',
    'isOpen',
    'startTransaction',
    'setClientData',
    'setQueryAttributes',
    'rollback',
    'runSql',
    'connectionId',
    'sshUri',
    'trackSystemVariable',
    'setOptionTrackerFeatureId',
    'uri',
    '_getSocketFd'])

//@<> BUG#30516645 character sets should be set to utf8mb4
classicSession.runSql("SHOW SESSION VARIABLES LIKE 'character\\_set\\_%'");

EXPECT_STDOUT_CONTAINS_MULTILINE(`+--------------------------+-[[*]]+
| Variable_name            | Value [[*]]|
+--------------------------+-[[*]]+
| character_set_client     | utf8mb4 [[*]]|
| character_set_connection | utf8mb4 [[*]]|
| character_set_database   | [[*]]|
| character_set_filesystem | [[*]]|
| character_set_results    | utf8mb4 [[*]]|
| character_set_server     | [[*]]|
| character_set_system     | [[*]]|
+--------------------------+-[[*]]+`);

//@<> BUG#30516645 colations should be set to expected value
const expected_collation = testutil.versionCheck(__version, "<", "8.0.0") ? 'utf8mb4_general_ci' : 'utf8mb4_0900_ai_ci';

classicSession.runSql("SHOW SESSION VARIABLES LIKE 'collation\\_%'");

EXPECT_STDOUT_CONTAINS_MULTILINE(`+----------------------+--------------------+
| Variable_name        | Value              |
+----------------------+--------------------+
| collation_connection | ${expected_collation} |
| collation_database   | [[*]]|
| collation_server     | [[*]]|
+----------------------+--------------------+`);

//@<> BUG#30516645 this query should not throw
classicSession.runSql("SELECT * FROM (SELECT VARIABLE_VALUE FROM performance_schema.session_variables WHERE VARIABLE_NAME IN ('collation_connection') UNION ALL SELECT CAST(NULL as CHAR(32))) as c;");

EXPECT_STDOUT_CONTAINS_MULTILINE(`+--------------------+
| VARIABLE_VALUE     |
+--------------------+
| ${expected_collation} |
| NULL               |
+--------------------+`);

//@<> ClassicSession: accessing Schemas
EXPECT_THROWS_LIKE(function() {classicSession.getSchemas()}, /invokeMember \(getSchemas\).*failed due to: Unknown identifier: getSchemas/)

//@<> ClassicSession: accessing individual schema
EXPECT_THROWS_LIKE(function() {classicSession.getSchema('mysql')}, /invokeMember \(getSchema\).*failed due to: Unknown identifier: getSchema/)

//@<> ClassicSession: accessing default schema
EXPECT_THROWS_LIKE(function() {classicSession.getDefaultSchema()}, /invokeMember \(getDefaultSchema\).*failed due to: Unknown identifier: getDefaultSchema/)

//@<> ClassicSession: accessing current schema
EXPECT_THROWS_LIKE(function() {classicSession.getCurrentSchema()}, /invokeMember \(getCurrentSchema\).*failed due to: Unknown identifier: getCurrentSchema/)

//@<> ClassicSession: create schema
EXPECT_THROWS_LIKE(function() {classicSession.createSchema('classic_session_schema')}, /invokeMember \(createSchema\).*failed due to: Unknown identifier: createSchema/)

//@<> ClassicSession: set current schema
EXPECT_THROWS_LIKE(function() {classicSession.setCurrentSchema('classic_session_schema')}, /invokeMember \(setCurrentSchema\).*failed due to: Unknown identifier: setCurrentSchema/)

//@<> ClassicSession: drop schema
EXPECT_THROWS_LIKE(function() {classicSession.dropSchema('node_session_schema')}, /invokeMember \(dropSchema\).*failed due to: Unknown identifier: dropSchema/)

//@Preparation for transaction tests
var result = classicSession.runSql('drop schema if exists classic_session_schema');
var result = classicSession.runSql('create schema classic_session_schema');
var result = classicSession.runSql('use classic_session_schema');

//@ ClassicSession: Transaction handling: rollback
var result = classicSession.runSql('create table sample (name varchar(50) primary key)');
classicSession.startTransaction();
var res1 = classicSession.runSql('insert into sample values ("john")');
var res2 = classicSession.runSql('insert into sample values ("carol")');
var res3 = classicSession.runSql('insert into sample values ("jack")');
classicSession.rollback();

var result = classicSession.runSql('select * from sample');
print('Inserted Documents:', result.fetchAll().length);

//@ ClassicSession: Transaction handling: commit
//! [ClassicSession: SQL execution example]
// Begins a transaction
classicSession.startTransaction();

// Inserts some records
var res1 = classicSession.runSql('insert into sample values ("john")');
var res2 = classicSession.runSql('insert into sample values ("carol")');
var res3 = classicSession.runSql('insert into sample values ("jack")');

// Commits the transaction
classicSession.commit();
//! [ClassicSession: SQL execution example]

var result = classicSession.runSql('select * from sample');
print('Inserted Documents:', result.fetchAll().length);

//@ ClassicSession: date handling
classicSession.runSql("select cast('9999-12-31 23:59:59.999999' as datetime(6))");

//@# ClassicSession: runSql errors
classicSession.runSql();
classicSession.runSql(1, 2, 3);
classicSession.runSql(1);
classicSession.runSql('select ?', 5);
classicSession.runSql('select ?, ?', [1, 2, 3]);
classicSession.runSql('select ?, ?', [1]);

//@<OUT> ClassicSession: runSql placeholders
classicSession.runSql("select ?, ?", ['hello', 1234]);

//@<OUT> ClassicSession: runSql with various parameter types
classicSession.runSql('select ?,?,?,?,?', [null, 1234, -0.12345, 3.14159265359, 'hellooooo']).fetchOne();

//@<> ClassicSession: runSql with ! placeholders
EXPECT_EQ(__user, classicSession.runSql('select user from !.! where user=?', ['mysql', 'user', __user]).fetchOne()[0])

// Cleanup
classicSession.close();
