#@{VER(<8.0.0)}

#@<> Setup
test_schema1 = "ucfilterschema_test1"
test_schema2 = "ucfilterschema_test2"
test_schema3 = "ucfilterschema_test3"

shell.connect(__mysql_uri)

for schema_name in [test_schema1, test_schema2, test_schema3]:
    session.run_sql(f"drop schema if exists `{schema_name}`;")
    session.run_sql(f"create schema `{schema_name}`;")
    session.run_sql(f"use `{schema_name}`;")
    session.run_sql("create table System(JSON_TABLE integer, cube int);")
    session.run_sql("create trigger first_value AFTER INSERT on System FOR EACH ROW delete from Clone where JSON_TABLE>0;")
    session.run_sql("create trigger last_value AFTER INSERT on System FOR EACH ROW delete from Clone where JSON_TABLE<0;")
    session.run_sql("create view LATERAL as select now();")
    session.run_sql("create view NTile as select * from System;")
    session.run_sql(f"CREATE EVENT persist ON SCHEDULE AT CURRENT_TIMESTAMP + INTERVAL 1 HOUR DO UPDATE {schema_name}.System SET cube = 0;")
    session.run_sql(f"CREATE EVENT rank ON SCHEDULE AT CURRENT_TIMESTAMP + INTERVAL 1 HOUR DO UPDATE {schema_name}.System SET cube = 0;")
    session.run_sql("CREATE FUNCTION Array (s CHAR(20)) RETURNS CHAR(50) DETERMINISTIC RETURN CONCAT('Hello, ',s,'!');")
    session.run_sql("CREATE FUNCTION rows (s CHAR(20)) RETURNS CHAR(50) DETERMINISTIC RETURN CONCAT('Hello, ',s,'!');")
    session.run_sql("CREATE FUNCTION full (s CHAR(20)) RETURNS CHAR(50) DETERMINISTIC RETURN CONCAT('Hello, ',s,'!');")
    session.run_sql("CREATE FUNCTION inteRsect (s CHAR(20)) RETURNS CHAR(50) DETERMINISTIC RETURN CONCAT('Hello, ',s,'!');")

#@ Sanity test - UC show all schemas on reservedKeywords check
EXPECT_NO_THROWS(lambda: util.check_for_server_upgrade(None, {"include": ["reservedKeywords"]}))

#@ includeSchemas - filter in only test_schema1
WIPE_OUTPUT()
EXPECT_NO_THROWS(lambda: util.check_for_server_upgrade(None, {"include": ["reservedKeywords"], "includeSchemas": [f"{test_schema1}"]}))

#@ includeSchemas - filter in only test_schema1, test_schema3 with tick quotes
WIPE_OUTPUT()
EXPECT_NO_THROWS(lambda: util.check_for_server_upgrade(None, {"include": ["reservedKeywords"], "includeSchemas": [f"{test_schema1}", f"`{test_schema3}`"]}))

#@ includeSchemas - filters in test_schema1, test_schema2, ignores invalid one
WIPE_OUTPUT()
EXPECT_NO_THROWS(lambda: util.check_for_server_upgrade(None, {"include": ["reservedKeywords"], "includeSchemas": [f"{test_schema1}", "invalid_schema", f"{test_schema2}"]}))

#@ excludeSchemas - filter out only test_schema1
WIPE_OUTPUT()
EXPECT_NO_THROWS(lambda: util.check_for_server_upgrade(None, {"include": ["reservedKeywords"], "excludeSchemas": [f"{test_schema1}"]}))

#@ excludeSchemas - filter out only test_schema1, test_schema3 with tick quotes
WIPE_OUTPUT()
EXPECT_NO_THROWS(lambda: util.check_for_server_upgrade(None, {"include": ["reservedKeywords"], "excludeSchemas": [f"{test_schema1}", f"`{test_schema3}`"]}))

#@ excludeSchemas - filters out test_schema1, test_schema2, ignores invalid one
WIPE_OUTPUT()
EXPECT_NO_THROWS(lambda: util.check_for_server_upgrade(None, {"include": ["reservedKeywords"], "excludeSchemas": [f"{test_schema1}", "invalid_schema", f"{test_schema2}"]}))

#@ includeSchemas - test_schema1, excludeSchemas - test_schema3, includeSchemas should take priority
WIPE_OUTPUT()
EXPECT_NO_THROWS(lambda: util.check_for_server_upgrade(None, {"include": ["reservedKeywords"], "includeSchemas": [f"{test_schema1}"], "excludeSchemas": [f"{test_schema3}"]}))



#@ includeTables - filters in System and LATERAL from test_schema1
# Filters out triggers, routines and events (using include filters that match nothing)
WIPE_OUTPUT()
EXPECT_NO_THROWS(lambda: util.check_for_server_upgrade(None, {
    "include": ["reservedKeywords"],
    "includeTables": [f"{test_schema1}.System", f"{test_schema1}.LATERAL", "invalid.schema"],
    "includeTriggers": [f"{test_schema1}.System.All"],
    "includeRoutines": ["Exclude.All"],
    "includeEvents": ["Exclude.All"],}))

#@ excludeTables - filters out System and LATERAL from test_schema1
# Filters out triggers, routines and events (using include filters that match nothing)
WIPE_OUTPUT()
EXPECT_NO_THROWS(lambda: util.check_for_server_upgrade(None, {
    "include": ["reservedKeywords"],
    "excludeTables": [f"{test_schema1}.System",f"{test_schema1}.LATERAL", "invalid.schema"],
    "includeTriggers": [f"{test_schema2}.System.All"],
    "includeRoutines": ["Exclude.All"],
    "includeEvents": ["Exclude.All"]}))


#@ includeTriggers - no filter, so list all triggers in selected table
# Filters out routines and events (using include filters that match nothing)
WIPE_OUTPUT()
EXPECT_NO_THROWS(lambda: util.check_for_server_upgrade(None, {
    "include": ["reservedKeywords"],
    "includeTables": [f"{test_schema1}.System"],
    "includeRoutines": ["Exclude.All"],
    "includeEvents": ["Exclude.All"],}))

#@ includeTriggers - filters in first_value trigger from test_schema1.System
# Filters out routines and events (using include filters that match nothing)
WIPE_OUTPUT()
EXPECT_NO_THROWS(lambda: util.check_for_server_upgrade(None, {
    "include": ["reservedKeywords"],
    "includeTables": [f"{test_schema1}.System"],
    "includeTriggers": [f"{test_schema1}.System.first_value", f"{test_schema1}.System.unexisting_trigger"],
    "includeRoutines": ["Exclude.All"],
    "includeEvents": ["Exclude.All"],}))

#@ excludeTriggers - filters out first_value trigger from test_schema1.System
# Filters out routines and events (using include filters that match nothing)
WIPE_OUTPUT()
EXPECT_NO_THROWS(lambda: util.check_for_server_upgrade(None, {
    "include": ["reservedKeywords"],
    "includeTables": [f"{test_schema1}.System"],
    "excludeTriggers": [f"{test_schema1}.System.first_value", f"{test_schema1}.System.unexisting_trigger"],
    "includeRoutines": ["Exclude.All"],
    "includeEvents": ["Exclude.All"],}))


#@ routines with no filters
# Filters out tables and events (using include filters that match nothing)
WIPE_OUTPUT()
EXPECT_NO_THROWS(lambda: util.check_for_server_upgrade(None, {
    "include": ["reservedKeywords"],
    "includeTables": [f"{test_schema2}.None"],
    "includeEvents": ["Exclude.All"]}))

#@ includeRoutines - includes Array and rows routines in test_schema2
# Filters out tables and events (using include filters that match nothing)
WIPE_OUTPUT()
EXPECT_NO_THROWS(lambda: util.check_for_server_upgrade(None, {
    "include": ["reservedKeywords"],
    "includeRoutines": [f"{test_schema2}.Array", f"{test_schema2}.rows", f"{test_schema2}.unexisting_routine"],
    "includeTables": [f"{test_schema2}.None"],
    "includeEvents": ["Exclude.All"]}))

#@ excludeRoutines - includes Array and rows routines in test_schema2
# Filters out tables and events (using include filters that match nothing)
WIPE_OUTPUT()
EXPECT_NO_THROWS(lambda: util.check_for_server_upgrade(None, {
    "include": ["reservedKeywords"],
    "excludeRoutines": [f"{test_schema2}.Array", f"{test_schema2}.rows", f"{test_schema2}.unexisting_routine"],
    "includeTables": [f"{test_schema2}.None"],
    "includeEvents": ["Exclude.All"]}))


#@<> events with no filters
# Filters out tables and routines (using include filters that match nothing)
WIPE_OUTPUT()
EXPECT_NO_THROWS(lambda: util.check_for_server_upgrade(None, {
    "include": ["reservedKeywords"],
    "includeTables": [f"{test_schema2}.None"],
    "includeRoutines": ["Exclude.All"]}))
EXPECT_OUTPUT_CONTAINS("ucfilterschema_test3.rank - Event name")
EXPECT_OUTPUT_CONTAINS("ucfilterschema_test3.persist - Event name")
EXPECT_OUTPUT_CONTAINS("ucfilterschema_test2.rank - Event name")
EXPECT_OUTPUT_CONTAINS("ucfilterschema_test2.persist - Event name")
EXPECT_OUTPUT_CONTAINS("ucfilterschema_test1.persist - Event name")
EXPECT_OUTPUT_CONTAINS("ucfilterschema_test1.rank - Event name")



#@<> includeEvents - includes the persist event in test_schema3
# Filters out tables and routines (using include filters that match nothing)
WIPE_OUTPUT()
EXPECT_NO_THROWS(lambda: util.check_for_server_upgrade(None, {
    "include": ["reservedKeywords"],
    "includeTables": [f"{test_schema3}.None"],
    "includeRoutines": ["Exclude.All"],
    "includeEvents": [f"{test_schema3}.persist", f"{test_schema3}.unexisting_event"]}))
EXPECT_OUTPUT_NOT_CONTAINS("ucfilterschema_test3.rank - Event name")
EXPECT_OUTPUT_CONTAINS("ucfilterschema_test3.persist - Event name")
EXPECT_OUTPUT_NOT_CONTAINS("ucfilterschema_test2.rank - Event name")
EXPECT_OUTPUT_NOT_CONTAINS("ucfilterschema_test2.persist - Event name")
EXPECT_OUTPUT_NOT_CONTAINS("ucfilterschema_test1.persist - Event name")
EXPECT_OUTPUT_NOT_CONTAINS("ucfilterschema_test1.rank - Event name")

#@<> excludeEvents - excludes the persist event in test_schema3
# Filters out tables and routines (using include filters that match nothing)
WIPE_OUTPUT()
EXPECT_NO_THROWS(lambda: util.check_for_server_upgrade(None, {
    "include": ["reservedKeywords"],
    "includeTables": [f"{test_schema3}.None"],
    "includeRoutines": ["Exclude.All"],
    "excludeEvents": [f"{test_schema3}.persist", f"{test_schema3}.unexisting_event"]}))
EXPECT_OUTPUT_CONTAINS("ucfilterschema_test3.rank - Event name")
EXPECT_OUTPUT_NOT_CONTAINS("ucfilterschema_test3.persist - Event name")
EXPECT_OUTPUT_CONTAINS("ucfilterschema_test2.rank - Event name")
EXPECT_OUTPUT_CONTAINS("ucfilterschema_test2.persist - Event name")
EXPECT_OUTPUT_CONTAINS("ucfilterschema_test1.persist - Event name")
EXPECT_OUTPUT_CONTAINS("ucfilterschema_test1.rank - Event name")

#@<> Cleanup
for schema_name in [test_schema1, test_schema2, test_schema3]:
    session.run_sql(f"drop schema if exists `{schema_name}`")