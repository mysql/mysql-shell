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
    "affectedItemsCount": 0,
    "warningCount": 0,
    "warningsCount": 0,
    "warnings": [],
    "rows": [
        {
            "1": 1,
            "2": 2,
            "3": 3
        }
    ],
    "hasData": true,
    "affectedRowCount": 0,
    "autoIncrementValue": 0
}

//@ cleanup
||
