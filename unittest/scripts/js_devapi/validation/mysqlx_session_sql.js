//@<OUT> WL12813 SQL Test 01
+------+----------------+---------------------------------------------------------------------+
| like | notnested.like | doc                                                                 |
+------+----------------+---------------------------------------------------------------------+
| foo  | foo            | {"like": "foo", "nested": {"like": "foo"}, "notnested.like": "foo"} |
+------+----------------+---------------------------------------------------------------------+
1 row in set [[*]]

//@ WL12813 SQL Test With Error (no placeholders)
||You have an error in your SQL syntax; check the manual that corresponds to your MySQL server version for the right syntax to use near '?' at line 1 (MySQL Error 1064)

//@ WL12813 SQL Test With Error (missing placeholder)
||Insufficient number of values for placeholders in query (ArgumentError)

//@<OUT> runSql with various parameter types
[
    null,
    1234,
    "-0.12345",
    "3.14159265359",
    "hellooooo"
]

//@<OUT> BUG#34715428: runSql with ! placeholders
<<<__user>>>
