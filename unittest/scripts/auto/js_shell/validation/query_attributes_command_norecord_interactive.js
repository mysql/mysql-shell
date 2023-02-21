//@<> Setting query attributes without the component installed
||

//@ Query attributes without the component installed
||ERROR: 1305 (42000): FUNCTION mysql.mysql_query_attribute_string does not exist

//@ Installing the query attributes component
|Query OK, 0 rows affected|

//@<OUT> Basic query attribute definition
+------+------+------+------+------+
| at1  | at2  | at3  | at4  | at5  |
+------+------+------+------+------+
| val1 | val2 | val3 | val4 | NULL |
+------+------+------+------+------+

//@<OUT> Query attributes belong only to the next query
+------+------+------+------+------+
| at1  | at2  | at3  | at4  | at5  |
+------+------+------+------+------+
| NULL | NULL | NULL | NULL | NULL |
+------+------+------+------+------+

//@<OUT> Calling \query_attributes multiple times, last attribute set overrides initial
+-----+-----+-----+------+------+
| at1 | at2 | at3 | at4  | at5  |
+-----+-----+-----+------+------+
| v1  | v2  | v3  | NULL | NULL |
+-----+-----+-----+------+------+

//@<OUT> Calling \query_attributes without arguments cleans previouly defined attributes
+------+------+------+------+------+
| at1  | at2  | at3  | at4  | at5  |
+------+------+------+------+------+
| NULL | NULL | NULL | NULL | NULL |
+------+------+------+------+------+

//@<OUT> Trying quoted attribute/values with "
+------------+
| at1        |
+------------+
| some value |
+------------+

//@<OUT> Trying quoted attribute/values with '
+------------+
| at1        |
+------------+
| some value |
+------------+

//@<OUT> Trying quoted attribute/values with `
+------------+
| at1        |
+------------+
| some value |
+------------+

//@<OUT> Multiple <attributes><space><value> with multiple spaces and tabs as separator
+------+------+------+------+------+
| at1  | at2  | at3  | at4  | at5  |
+------+------+------+------+------+
| val1 | val2 | val3 | val4 | NULL |
+------+------+------+------+------+

//@<OUT> Query attributes with multiple spaces and tabs as separator
+------+------+------+------+------+
| at1  | at2  | at3  | at4  | at5  |
+------+------+------+------+------+
| val1 | val2 | val3 | val4 | NULL |
+------+------+------+------+------+

//@<OUT> Query attributes with space as key
+------+------+------+------+------+
| at1  | at2  | at3  | at4  | at5  |
+------+------+------+------+------+
| val1 | val2 | val3 | val4 | NULL |
+------+------+------+------+------+

//@<OUT> Query attributes edge cases: space key, spaces key, empty value
+-------+-------------+---------------+
| SPACE | EMPTY_VALUE | NULL_EXPECTED |
+-------+-------------+---------------+
| val1  |             | val1          |
+-------+-------------+---------------+

//@<OUT> Query attributes with using key<space>
ERROR: The \query_attributes shell command receives attribute value pairs as arguments, odd number of arguments received.
Usage: \query_attributes name1 value1 name2 value2 ... nameN valueN.

//@<OUT> Query attributes with using key<space>value <other>
ERROR: The \query_attributes shell command receives attribute value pairs as arguments, odd number of arguments received.
Usage: \query_attributes name1 value1 name2 value2 ... nameN valueN.

//@<OUT> Query attributes with >32 key value pairs
WARNING: Invalid query attributes found, they will be ignored: The following query attribute exceed the maximum limit (32): 33
+------------------------------------+------------------------------------+
| mysql_query_attribute_string("32") | mysql_query_attribute_string("33") |
+------------------------------------+------------------------------------+
| 32                                 | NULL                               |
+------------------------------------+------------------------------------+

//@<OUT> Query attribute, long attribute name (1025 chars)
WARNING: Invalid query attributes found, they will be ignored: The following query attribute exceed the maximum name length (1024): 01234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234
+-------------------------------------+-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+-------------------------------------+
| mysql_query_attribute_string("at1") | mysql_query_attribute_string("012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234 | mysql_query_attribute_string("at3") |
+-------------------------------------+-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+-------------------------------------+
| val1                                | NULL                                                                                                                                                                                                                                                            | val3                                |
+-------------------------------------+-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+-------------------------------------+

//@<OUT> Query attribute, long value (1025 chars)
WARNING: Invalid query attributes found, they will be ignored: The following query attribute exceed the maximum value length (1024): at2
+-------------------------------------+-------------------------------------+-------------------------------------+
| mysql_query_attribute_string("at1") | mysql_query_attribute_string("at2") | mysql_query_attribute_string("at3") |
+-------------------------------------+-------------------------------------+-------------------------------------+
| val1                                | NULL                                | val3                                |
+-------------------------------------+-------------------------------------+-------------------------------------+

//@<OUT> Query attributes, after reconnect 1
The global session was successfully reconnected.
+-------------------------------------+-------------------------------------+
| mysql_query_attribute_string("at2") | mysql_query_attribute_string("at2") |
+-------------------------------------+-------------------------------------+
| val2                                | val2                                |
+-------------------------------------+-------------------------------------+

//@<OUT> Query attributes, after reconnect 2
The global session was successfully reconnected.
+-------------------------------------+-------------------------------------+
| mysql_query_attribute_string("at2") | mysql_query_attribute_string("at2") |
+-------------------------------------+-------------------------------------+
| val2                                | val2                                |
+-------------------------------------+-------------------------------------+

//@<OUT> Defining query attributes before restart
Switching to JavaScript mode...

//@<ERR> Reconnection will be triggered here
ERROR: 2013 (HY000): Lost connection to MySQL server during query

//@<OUT> Reconnection will be triggered here
The global session was successfully reconnected.

//@<OUT> Retrieving query attributes after restart
+-------------------------------------+-------------------------------------+
| mysql_query_attribute_string("at2") | mysql_query_attribute_string("at2") |
+-------------------------------------+-------------------------------------+
| val2                                | val2                                |
+-------------------------------------+-------------------------------------+

//@<OUT> Query attributes with no global session
ERROR: An open session through the MySQL protocol is required to execute the \query_attributes shell command.

//@<OUT> Query attributes with session through X protocol
ERROR: An open session through the MySQL protocol is required to execute the \query_attributes shell command.


//@<ERR> \query_attributes in Python
Unknown command: '\query_attributes at1 '

//@<ERR> \query_attributes in JavaScript
Unknown command: '\query_attributes at1 '
