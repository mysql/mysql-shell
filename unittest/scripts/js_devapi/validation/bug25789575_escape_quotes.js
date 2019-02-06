//@ Add data to collection
|Query OK, 1 item affected|
|Query OK, 1 item affected|
|Query OK, 1 item affected|
|Query OK, 1 item affected|
|Query OK, 1 item affected|
|Query OK, 1 item affected|
|Query OK, 1 item affected|

//@<OUT> find()
{
    "a": 1,
    "_id": "1"
}
{
    "_id": "2",
    "key'with'single'quotes": "value'with'single'quotes"
}
{
    "_id": "3",
    "key\"with\"double\"quotes": "value\"with\"double\"quotes"
}
{
    "_id": "4",
    "key\"with\"mixed'quo'\"tes": "value\"with'mixed\"quotes''\""
}
{
    "_id": "5",
    "utf8": "I ❤ MySQL Shell"
}
{
    "_id": "6",
    "new_line": "\\n is a newline\nthis is second line."
}
{
    "_id": "7",
    "tab_stop": "\\t is a tab stop\n\tnewline with tab stop."
}


//@<OUT> find().execute().fetchAll()
[
    {
        "_id": "1",
        "a": 1
    },
    {
        "_id": "2",
        "key'with'single'quotes": "value'with'single'quotes"
    },
    {
        "_id": "3",
        "key\"with\"double\"quotes": "value\"with\"double\"quotes"
    },
    {
        "_id": "4",
        "key\"with\"mixed'quo'\"tes": "value\"with'mixed\"quotes''\""
    },
    {
        "_id": "5",
        "utf8": "I ❤ MySQL Shell"
    },
    {
        "_id": "6",
        "new_line": "\\n is a newline
this is second line."
    },
    {
        "_id": "7",
        "tab_stop": "\\t is a tab stop
	newline with tab stop."
    }
]

//@<OUT> get as table
<Table:bug25789575>

//@<OUT> select()
+-----------------------------------------------------------------------------+-----+
| doc                                                                         | _id |
+-----------------------------------------------------------------------------+-----+
| {"a": 1, "_id": "1"}                                                        | 1   |
| {"_id": "2", "key'with'single'quotes": "value'with'single'quotes"}          | 2   |
| {"_id": "3", "key\"with\"double\"quotes": "value\"with\"double\"quotes"}    | 3   |
| {"_id": "4", "key\"with\"mixed'quo'\"tes": "value\"with'mixed\"quotes''\""} | 4   |
| {"_id": "5", "utf8": "I ❤ MySQL Shell"}                                     | 5   |
| {"_id": "6", "new_line": "\\n is a newline\nthis is second line."}          | 6   |
| {"_id": "7", "tab_stop": "\\t is a tab stop\n\tnewline with tab stop."}     | 7   |
+-----------------------------------------------------------------------------+-----+

//@<OUT> execute().fetchAll()
[
    [
        "{\"a\": 1, \"_id\": \"1\"}",
        "1"
    ],
    [
        "{\"_id\": \"2\", \"key'with'single'quotes\": \"value'with'single'quotes\"}",
        "2"
    ],
    [
        "{\"_id\": \"3\", \"key\\\"with\\\"double\\\"quotes\": \"value\\\"with\\\"double\\\"quotes\"}",
        "3"
    ],
    [
        "{\"_id\": \"4\", \"key\\\"with\\\"mixed'quo'\\\"tes\": \"value\\\"with'mixed\\\"quotes''\\\"\"}",
        "4"
    ],
    [
        "{\"_id\": \"5\", \"utf8\": \"I ❤ MySQL Shell\"}",
        "5"
    ],
    [
        "{\"_id\": \"6\", \"new_line\": \"\\\\n is a newline\\nthis is second line.\"}",
        "6"
    ],
    [
        "{\"_id\": \"7\", \"tab_stop\": \"\\\\t is a tab stop\\n\\tnewline with tab stop.\"}",
        "7"
    ]
]

//@<OUT> Cleanup
o/
