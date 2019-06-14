// Assumptions: ensure_schema_does_not_exist is available
// Assumes __uripwd is defined as <user>:<pwd>@<host>:<mysql_port>
// validateMemer and validateNotMember are defined on the setup script
var mysql = require('mysql');

//@ Session: validating members
var classicSession = mysql.getClassicSession(__uripwd);
var sessionMembers = dir(classicSession);

validateMember(sessionMembers, 'close');
validateMember(sessionMembers, 'createSchema');
validateMember(sessionMembers, 'getCurrentSchema');
validateMember(sessionMembers, 'getDefaultSchema');
validateMember(sessionMembers, 'getSchema');
validateMember(sessionMembers, 'getSchemas');
validateMember(sessionMembers, 'getUri');
validateMember(sessionMembers, 'setCurrentSchema');
validateMember(sessionMembers, 'query');
validateMember(sessionMembers, 'runSql');
validateMember(sessionMembers, 'defaultSchema');
validateMember(sessionMembers, 'uri');
validateMember(sessionMembers, 'currentSchema');

//@ ClassicSession: accessing Schemas
var schemas = classicSession.getSchemas();

//@ ClassicSession: accessing individual schema
var schema = classicSession.getSchema('mysql');

//@ ClassicSession: accessing default schema
var dschema = classicSession.getDefaultSchema();

//@ ClassicSession: accessing current schema
var cschema = classicSession.getCurrentSchema();

//@ ClassicSession: create schema
var sf = classicSession.createSchema('classic_session_schema');

//@ ClassicSession: set current schema
classicSession.setCurrentSchema('classic_session_schema');

//@ ClassicSession: drop schema
classicSession.dropSchema('node_session_schema');

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

//@# ClassicSession: query errors
classicSession.query();
classicSession.query(1, 2, 3);
classicSession.query(1);
classicSession.query('select ?', 5);
classicSession.query('select ?, ?', [1, 2, 3]);
classicSession.query('select ?, ?', [1]);

//@<OUT> ClassicSession: query placeholders
classicSession.query("select ?, ?", ['hello', 1234]);

// Cleanup
classicSession.close();
