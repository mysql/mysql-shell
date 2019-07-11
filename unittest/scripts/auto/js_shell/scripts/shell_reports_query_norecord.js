// -----------------------------------------------------------------------------
// test 'query' report

//@ create a new session
shell.connect(__uripwd);

//@ 'query' - create database
\show query CREATE DATABASE report_test

//@ 'query' - show tables, no result
\show query SHOW TABLES IN report_test

//@ 'query' - create sample table
\show query CREATE TABLE report_test.sample (one VARCHAR(20), two INT, three INT UNSIGNED, four FLOAT, five DOUBLE, six DECIMAL, seven DATE, eight DATETIME, nine BIT, ten BLOB, eleven GEOMETRY, twelve JSON, thirteen TIME, fourteen ENUM('a', 'b'), fifteen SET('c', 'd'))

//@ 'query' - show tables, result contains the table
\show query SHOW TABLES IN report_test

//@ 'query' - show description of sample table
\show query DESC report_test.sample

//@ 'query' - insert some values
\show query INSERT INTO report_test.sample VALUES ('z', -1, 2, 3.14, -6.28, 123, '2018-11-23', '2018-11-23 08:06:34', 0, 'BLOB', NULL, '{\"a\" : 3}', '08:06:34', 'b', 'c,d')

//@ 'query' - query for values
\show query SELECT * FROM report_test.sample

//@ 'query' - delete the database
\show query DROP DATABASE report_test

//@ 'query' - --help
\show query --help

//@ 'query' - help system
\help shell.reports.query

//@ 'query' - call method with null arguments
shell.reports.query(session, null)

//@ 'query' - call method with empty arguments [USE: 'query' - call method with null arguments]
shell.reports.query(session, [])

//@ 'query' - call method with null options
shell.reports.query(session, ['argv0'], null)

//@ 'query' - call method with empty options [USE: 'query' - call method with null options]
shell.reports.query(session, ['argv0'], {})

// -----------------------------------------------------------------------------
//@ cleanup - disconnect the session
session.close();
