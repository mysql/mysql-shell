//@ switch to "table" output format
||

//@<OUT> check if there are any deprecated warnings (1)
+---+---+---+
| 1 | 2 | 3 |
+---+---+---+
| 1 | 2 | 3 |
+---+---+---+

//@ switch to "json" output format
||

//@<OUT> check if there are any deprecated warnings (2)
{
    "1": 1,
    "2": 2,
    "3": 3
}
1 row in set ([[*]] sec)

//@ cleanup
||
