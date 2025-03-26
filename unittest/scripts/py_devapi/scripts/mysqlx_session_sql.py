shell.connect(__uripwd);
session.drop_schema('session_sql');
schema = session.create_schema('session_sql');
session.sql("use session_sql");
result = session.sql('create table wl12813 (`like` varchar(50), `notnested.like` varchar(50), `doc` json)').execute();
table = schema.get_table('wl12813');
result = table.insert().values('foo', 'foo', '{"like":"foo", "nested":{"like":"foo"}, "notnested.like":"foo"}').execute()

#@ WL12813 SQL Test 01
session.sql('SELECT * FROM wl12813 WHERE `like` = "foo"').execute()

#@ WL12813 SQL Test 02 [USE:WL12813 SQL Test 01]
session.sql('SELECT * FROM wl12813 WHERE `like` = ?').bind('foo').execute()

#@ WL12813 SQL Test 03 [USE:WL12813 SQL Test 01]
session.sql('SELECT * FROM wl12813 WHERE doc->>"$.like" = "foo"').execute()

#@ WL12813 SQL Test 04 [USE:WL12813 SQL Test 01]
session.sql('SELECT * FROM wl12813 WHERE doc->>"$.like" = ?').bind('foo').execute()

#@ WL12813 SQL Test 05 [USE:WL12813 SQL Test 01]
session.sql('SELECT * FROM wl12813 WHERE doc->>"$.nested.like" = "foo"').execute()

#@ WL12813 SQL Test 06 [USE:WL12813 SQL Test 01]
session.sql('SELECT * FROM wl12813 WHERE doc->>"$.nested.like" = ?').bind('foo').execute()

#@ WL12813 SQL Test 07 [USE:WL12813 SQL Test 01]
session.sql('SELECT * FROM wl12813 WHERE doc->>\'$."notnested.like"\' = "foo"').execute()

#@ WL12813 SQL Test 08 [USE:WL12813 SQL Test 01]
session.sql('SELECT * FROM wl12813 WHERE doc->>\'$."notnested.like"\' = ?').bind('foo').execute()

#@<> BUG#34715428: runSql with ! placeholders
assert __user == session.run_sql('select user from !.! where user=?', ['mysql', 'user', __user]).fetch_one()[0]

#@<> Finalizing
session.drop_schema('session_sql')
session.close()
