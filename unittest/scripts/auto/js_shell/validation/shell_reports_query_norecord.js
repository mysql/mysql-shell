//@ create a new session
|<Session:<<<__uri>>>>|

//@ 'query' - create database
|Report returned no data.|

//@ 'query' - show tables, no result
|Report returned no data.|

//@ 'query' - create sample table
|Report returned no data.|

//@<OUT> 'query' - show tables, result contains the table
+-----------------------+
| Tables_in_report_test |
+-----------------------+
| sample                |
+-----------------------+

//@<OUT> 'query' - show description of sample table {VER(<8.0.19)}
+----------+------------------+------+-----+---------+-------+
| Field    | Type             | Null | Key | Default | Extra |
+----------+------------------+------+-----+---------+-------+
| one      | varchar(20)      | YES  |     | NULL    |       |
| two      | int(11)          | YES  |     | NULL    |       |
| three    | int(10) unsigned | YES  |     | NULL    |       |
| four     | float            | YES  |     | NULL    |       |
| five     | double           | YES  |     | NULL    |       |
| six      | decimal(10,0)    | YES  |     | NULL    |       |
| seven    | date             | YES  |     | NULL    |       |
| eight    | datetime         | YES  |     | NULL    |       |
| nine     | bit(1)           | YES  |     | NULL    |       |
| ten      | blob             | YES  |     | NULL    |       |
| eleven   | geometry         | YES  |     | NULL    |       |
| twelve   | json             | YES  |     | NULL    |       |
| thirteen | time             | YES  |     | NULL    |       |
| fourteen | enum('a','b')    | YES  |     | NULL    |       |
| fifteen  | set('c','d')     | YES  |     | NULL    |       |
+----------+------------------+------+-----+---------+-------+

//@<OUT> 'query' - show description of sample table {VER(>=8.0.19)}
+----------+---------------+------+-----+---------+-------+
| Field    | Type          | Null | Key | Default | Extra |
+----------+---------------+------+-----+---------+-------+
| one      | varchar(20)   | YES  |     | NULL    |       |
| two      | int           | YES  |     | NULL    |       |
| three    | int unsigned  | YES  |     | NULL    |       |
| four     | float         | YES  |     | NULL    |       |
| five     | double        | YES  |     | NULL    |       |
| six      | decimal(10,0) | YES  |     | NULL    |       |
| seven    | date          | YES  |     | NULL    |       |
| eight    | datetime      | YES  |     | NULL    |       |
| nine     | bit(1)        | YES  |     | NULL    |       |
| ten      | blob          | YES  |     | NULL    |       |
| eleven   | geometry      | YES  |     | NULL    |       |
| twelve   | json          | YES  |     | NULL    |       |
| thirteen | time          | YES  |     | NULL    |       |
| fourteen | enum('a','b') | YES  |     | NULL    |       |
| fifteen  | set('c','d')  | YES  |     | NULL    |       |
+----------+---------------+------+-----+---------+-------+

//@ 'query' - insert some values
|Report returned no data.|

//@<OUT> 'query' - query for values
+-----+-----+-------+------+-------+-----+------------+---------------------+------+------+--------+----------+----------+----------+---------+
| one | two | three | four | five  | six | seven      | eight               | nine | ten  | eleven | twelve   | thirteen | fourteen | fifteen |
+-----+-----+-------+------+-------+-----+------------+---------------------+------+------+--------+----------+----------+----------+---------+
| z   | -1  | 2     | 3.14 | -6.28 | 123 | 2018-11-23 | 2018-11-23 08:06:34 | 0    | BLOB | NULL   | {"a": 3} | 08:06:34 | b        | c,d     |
+-----+-----+-------+------+-------+-----+------------+---------------------+------+------+--------+----------+----------+----------+---------+

//@ 'query' - delete the database
|Report returned no data.|

//@<OUT> 'query' - --help
NAME
      query - Executes the SQL statement given as arguments.

SYNTAX
      \show query [OPTIONS] [ARGS]
      \watch query [OPTIONS] [ARGS]

DESCRIPTION
      Options:

      --help, -h  Display this help and exit.

      --vertical, -E
                  Display records vertically.

      Arguments:

      This report accepts 1-* arguments.


//@<OUT> 'query' - help system
NAME
      query - Executes the SQL statement given as arguments.

SYNTAX
      shell.reports.query(session, argv)

WHERE
      session: Object - A Session object to be used to execute the report.
      argv: Array - Extra arguments. Report expects 1-* arguments.

DESCRIPTION
      This is a 'list' type report.

      The session parameter must be any of ClassicSession, Session.

//@ 'query' - call method with null arguments
||reports.query: Argument #2 'argv' is expecting 1-* arguments. (ArgumentError)

//@ 'query' - call method with null options
||reports.query: Invalid number of arguments, expected 2 but got 3 (ArgumentError)

//@ cleanup - disconnect the session
||
