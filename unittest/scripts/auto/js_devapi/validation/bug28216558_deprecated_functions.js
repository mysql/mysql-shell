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
    "hasData": true,
    "rows": [
        {
            "1": 1,
            "2": 2,
            "3": 3
        }
    ],
    "executionTime": "[[*]]",
    "affectedRowCount": 0,
    "affectedItemsCount": 0,
    "warningCount": 0,
    "warningsCount": 0,
    "warnings": [],
    "info": "",
    "autoIncrementValue": 0
}

//@ cleanup
||
