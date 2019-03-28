shell.connect(__uripwd);
session.dropSchema('session_sql');
var schema = session.createSchema('session_sql');
session.sql("use session_sql");
var result = session.sql('create table wl12813 (`like` varchar(50), `notnested.like` varchar(50), `doc` json)').execute();
var table = schema.getTable('wl12813');
result = table.insert().values('foo', 'foo', '{"like":"foo", "nested":{"like":"foo"}, "notnested.like":"foo"}').execute()

//@ WL12813 SQL Test 01
session.sql('SELECT * FROM wl12813 WHERE `like` = "foo"').execute()

//@ WL12813 SQL Test 02 [USE:WL12813 SQL Test 01]
session.sql('SELECT * FROM wl12813 WHERE `like` = ?').bind('foo').execute()

//@ WL12813 SQL Test 03 [USE:WL12813 SQL Test 01]
session.sql('SELECT * FROM wl12813 WHERE doc->>"$.like" = "foo"').execute()

//@ WL12813 SQL Test 04 [USE:WL12813 SQL Test 01]
session.sql('SELECT * FROM wl12813 WHERE doc->>"$.like" = ?').bind('foo').execute()

//@ WL12813 SQL Test 05 [USE:WL12813 SQL Test 01]
session.sql('SELECT * FROM wl12813 WHERE doc->>"$.nested.like" = "foo"').execute()

//@ WL12813 SQL Test 06 [USE:WL12813 SQL Test 01]
session.sql('SELECT * FROM wl12813 WHERE doc->>"$.nested.like" = ?').bind('foo').execute()

//@ WL12813 SQL Test 07 [USE:WL12813 SQL Test 01]
session.sql('SELECT * FROM wl12813 WHERE doc->>\'$."notnested.like"\' = "foo"').execute()

//@ WL12813 SQL Test 08 [USE:WL12813 SQL Test 01]
session.sql('SELECT * FROM wl12813 WHERE doc->>\'$."notnested.like"\' = ?').bind('foo').execute()

//@<> Finalizing
session.dropSchema('session_sql');
session.close();
