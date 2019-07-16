#@ ClassicSession: accessing Schemas
||unknown attribute: get_schemas

#@ ClassicSession: accessing individual schema
||unknown attribute: get_schema

#@ ClassicSession: accessing default schema
||unknown attribute: get_default_schema

#@ ClassicSession: accessing current schema
||unknown attribute: get_current_schema

#@ ClassicSession: create schema
||unknown attribute: create_schema

#@ ClassicSession: set current schema
||unknown attribute: set_current_schema

#@ ClassicSession: drop schema
||unknown attribute: drop_schema

#@Preparation for transaction tests
||

#@ ClassicSession: Transaction handling: rollback
|Inserted Documents: 0|

#@ ClassicSession: Transaction handling: commit
|Inserted Documents: 3|

#@ ClassicSession: date handling
|9999-12-31 23:59:59.999999|

#@# ClassicSession: run_sql errors
||ClassicSession.run_sql: Invalid number of arguments, expected 1 to 2 but got 0
||ClassicSession.run_sql: Invalid number of arguments, expected 1 to 2 but got 3
||ClassicSession.run_sql: Argument #1 is expected to be a string
||ClassicSession.run_sql: Argument #2 is expected to be an array
||ClassicSession.run_sql: Error formatting SQL query: more arguments than escapes while substituting placeholder value at index #2
||ClassicSession.run_sql: Insufficient number of values for placeholders in query


#@<OUT> ClassicSession: run_sql placeholders
| hello | 1234 |

#@# ClassicSession: query errors
||ClassicSession.query: Invalid number of arguments, expected 1 to 2 but got 0
||ClassicSession.query: Invalid number of arguments, expected 1 to 2 but got 3
||ClassicSession.query: Argument #1 is expected to be a string
||ClassicSession.query: Argument #2 is expected to be an array
||ClassicSession.query: Error formatting SQL query: more arguments than escapes while substituting placeholder value at index #2
||ClassicSession.query: Insufficient number of values for placeholders in query


#@<OUT> ClassicSession: query placeholders
WARNING: 'query' is deprecated, use ClassicSession.run_sql instead.
+-------+------+
| hello | 1234 |
+-------+------+
| hello | 1234 |
+-------+------+
