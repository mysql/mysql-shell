#@<OUT> WL12813 SQL Test 01
+------+----------------+---------------------------------------------------------------------+
| like | notnested.like | doc                                                                 |
+------+----------------+---------------------------------------------------------------------+
| foo  | foo            | {"like": "foo", "nested": {"like": "foo"}, "notnested.like": "foo"} |
+------+----------------+---------------------------------------------------------------------+
1 row in set [[*]]

#@ BUG#38661681 - exceptions
||Insufficient number of values for placeholders in query
||Insufficient number of values for placeholders in query
