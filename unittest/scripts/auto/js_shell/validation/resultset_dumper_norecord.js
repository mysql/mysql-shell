//@ Test Schema Creation
||

//@<OUT> X Table Format
table
+--------------+
| data         |
+--------------+
| 0x6162006364 |
| 0x6162096364 |
| 0x61620A6364 |
+--------------+


//@<OUT> X Vertical Format
*************************** 1. row ***************************
data: 0x6162006364
*************************** 2. row ***************************
data: 0x6162096364
*************************** 3. row ***************************
data: 0x61620A6364

//@<OUT> X Tabbed Format
data
0x6162006364
0x6162096364
0x61620A6364

//@<OUT> X Json Format
{
    "data": "YWIAY2Q="
}
{
    "data": "YWIJY2Q="
}
{
    "data": "YWIKY2Q="
}
3 rows in set ([[*]] sec)

//@<OUT> X Raw Json Format
{"data":"YWIAY2Q="}
{"data":"YWIJY2Q="}
{"data":"YWIKY2Q="}
3 rows in set ([[*]] sec)

//@<OUT> X Json Wrapping
{
    "hasData": true,
    "rows": [
        {
            "data": "YWIAY2Q="
        },
        {
            "data": "YWIJY2Q="
        },
        {
            "data": "YWIKY2Q="
        }
    ],
    "executionTime": "[[*]] sec",
    "affectedRowCount": 0,
    "affectedItemsCount": 0,
    "warningCount": 0,
    "warningsCount": 0,
    "warnings": [],
    "info": "",
    "autoIncrementValue": 0
}

//@<OUT> Classic Table Format
+--------------+
| data         |
+--------------+
| 0x6162006364 |
| 0x6162096364 |
| 0x61620A6364 |
+--------------+

//@<OUT> Classic Vertical Format
*************************** 1. row ***************************
data: 0x6162006364
*************************** 2. row ***************************
data: 0x6162096364
*************************** 3. row ***************************
data: 0x61620A6364

//@<OUT> Classic Tabbed Format
data
0x6162006364
0x6162096364
0x61620A6364

//@<OUT> Classic Json Format
{
    "data": "YWIAY2Q="
}
{
    "data": "YWIJY2Q="
}
{
    "data": "YWIKY2Q="
}
3 rows in set ([[*]] sec)

//@<OUT> Classic Raw Json Format
{"data":"YWIAY2Q="}
{"data":"YWIJY2Q="}
{"data":"YWIKY2Q="}
3 rows in set ([[*]] sec)

//@<OUT> Classic Json Wrapping
{
    "hasData": true,
    "rows": [
        {
            "data": "YWIAY2Q="
        },
        {
            "data": "YWIJY2Q="
        },
        {
            "data": "YWIKY2Q="
        }
    ],
    "executionTime": "[[*]] sec",
    "affectedRowCount": 0,
    "affectedItemsCount": 0,
    "warningCount": 0,
    "warningsCount": 0,
    "warnings": [],
    "info": "",
    "autoIncrementValue": 0
}


//@<OUT> table in table format {__os_type != "windows"}
+---------------------------+
| data                      |
+---------------------------+
| 生活是美好的 生活是美好的 |
| 辛德勒的名单 辛德勒的名单 |
| 指環王 指環王             |
| 尋找尼莫 尋找尼莫         |
| 😁😍😠 😭🙅🙉                   |
| ✅✨✋ ✈❄❔➗                  |
| 🚀🚑 🚙🚬🚻🛀                   |
| 🇯🇵🈳🆕🆒                     |
| ®7⃣⏰☕♒♣ ⛽🌄🌠🎨🐍🐾             |
| ascii text                |
| látin1 text               |
+---------------------------+

//@<OUT> table in table format {__os_type == "windows"}
+---------------------------+
| data                      |
+---------------------------+
| 生活是美好的 生活是美好的 |
| 辛德勒的名单 辛德勒的名单 |
| 指環王 指環王             |
| 尋找尼莫 尋找尼莫         |
| 😁😍😠 😭🙅🙉             |
| ✅✨✋ ✈❄❔➗                  |
| 🚀🚑 🚙🚬🚻🛀             |
| 🇯🇵🈳🆕🆒                |
| ®7⃣⏰☕♒♣ ⛽🌄🌠🎨🐍🐾        |
| ascii text                |
| látin1 text               |
+---------------------------+

//@<OUT> table in tabbed format
data
生活是美好的\0生活是美好的
辛德勒的名单\0辛德勒的名单
指環王\0指環王
尋找尼莫\0尋找尼莫
😁😍😠\0😭🙅🙉
✅✨✋\0✈❄❔➗
🚀🚑\0🚙🚬🚻🛀
🇯🇵🈳🆕🆒
®7⃣⏰☕♒♣\0⛽🌄🌠🎨🐍🐾


//@<OUT> table in vertical format
*************************** 1. row ***************************
data: 生活是美好的 生活是美好的
*************************** 2. row ***************************
data: 辛德勒的名单 辛德勒的名单
*************************** 3. row ***************************
data: 指環王 指環王
*************************** 4. row ***************************
data: 尋找尼莫 尋找尼莫
*************************** 5. row ***************************
data: 😁😍😠 😭🙅🙉
*************************** 6. row ***************************
data: ✅✨✋ ✈❄❔➗
*************************** 7. row ***************************
data: 🚀🚑 🚙🚬🚻🛀
*************************** 8. row ***************************
data: 🇯🇵🈳🆕🆒
*************************** 9. row ***************************
data: ®7⃣⏰☕♒♣ ⛽🌄🌠🎨🐍🐾


//@<OUT> Pulling as collection in JSON format
{
    "_id": "1",
    "name": "生活是美好的",
    "year": 1997
}
{
    "_id": "10",
    "name": "®7⃣⏰☕♒♣⛽🌄🌠🎨🐍🐾",
    "year": 2004
}
{
    "_id": "11",
    "name": "pure ascii text",
    "year": 2014
}
{
    "_id": "12",
    "name": "látiñ text row",
    "year": 2016
}
{
    "_id": "2",
    "name": "辛德勒的名单",
    "year": 1993
}
{
    "_id": "3",
    "name": "指環王",
    "year": 2001
}
{
    "_id": "4",
    "name": "尋找尼莫",
    "year": 2003
}
{
    "_id": "5",
    "name": "الجنة الآن",
    "year": 2003
}
{
    "_id": "6",
    "name": "😁😍😠😭🙅🙉",
    "year": 2004
}
{
    "_id": "7",
    "name": "✅✨✋✈❄❔➗",
    "year": 2004
}
{
    "_id": "8",
    "name": "🚀🚑🚙🚬🚻🛀",
    "year": 2004
}
{
    "_id": "9",
    "name": "🇯🇵🈳🆕🆒",
    "year": 2004
}


//@<OUT> pulling as table in table format {__os_type != "windows" && VER(< 8.0.19)}
+--------------------------------------------------------+-----+
| doc                                                    | _id |
+--------------------------------------------------------+-----+
| {"_id": "1", "name": "生活是美好的", "year": 1997}     | 1   |
| {"_id": "10", "name": "®7⃣⏰☕♒♣⛽🌄🌠🎨🐍🐾", "year": 2004}    | 10  |
| {"_id": "11", "name": "pure ascii text", "year": 2014} | 11  |
| {"_id": "12", "name": "látiñ text row", "year": 2016}  | 12  |
| {"_id": "2", "name": "辛德勒的名单", "year": 1993}     | 2   |
| {"_id": "3", "name": "指環王", "year": 2001}           | 3   |
| {"_id": "4", "name": "尋找尼莫", "year": 2003}         | 4   |
| {"_id": "5", "name": "الجنة الآن", "year": 2003}       | 5   |
| {"_id": "6", "name": "😁😍😠😭🙅🙉", "year": 2004}           | 6   |
| {"_id": "7", "name": "✅✨✋✈❄❔➗", "year": 2004}          | 7   |
| {"_id": "8", "name": "🚀🚑🚙🚬🚻🛀", "year": 2004}           | 8   |
| {"_id": "9", "name": "🇯🇵🈳🆕🆒", "year": 2004}            | 9   |
+--------------------------------------------------------+-----+

//@<OUT> pulling as table in table format {__os_type != "windows" && VER(>= 8.0.19)}
+--------------------------------------------------------+--------+--------------------+
| doc                                                    | _id    | _json_schema       |
+--------------------------------------------------------+--------+--------------------+
| {"_id": "1", "name": "生活是美好的", "year": 1997}     | 0x31   | {"type": "object"} |
| {"_id": "10", "name": "®7⃣⏰☕♒♣⛽🌄🌠🎨🐍🐾", "year": 2004}    | 0x3130 | {"type": "object"} |
| {"_id": "11", "name": "pure ascii text", "year": 2014} | 0x3131 | {"type": "object"} |
| {"_id": "12", "name": "látiñ text row", "year": 2016}  | 0x3132 | {"type": "object"} |
| {"_id": "2", "name": "辛德勒的名单", "year": 1993}     | 0x32   | {"type": "object"} |
| {"_id": "3", "name": "指環王", "year": 2001}           | 0x33   | {"type": "object"} |
| {"_id": "4", "name": "尋找尼莫", "year": 2003}         | 0x34   | {"type": "object"} |
| {"_id": "5", "name": "الجنة الآن", "year": 2003}       | 0x35   | {"type": "object"} |
| {"_id": "6", "name": "😁😍😠😭🙅🙉", "year": 2004}           | 0x36   | {"type": "object"} |
| {"_id": "7", "name": "✅✨✋✈❄❔➗", "year": 2004}          | 0x37   | {"type": "object"} |
| {"_id": "8", "name": "🚀🚑🚙🚬🚻🛀", "year": 2004}           | 0x38   | {"type": "object"} |
| {"_id": "9", "name": "🇯🇵🈳🆕🆒", "year": 2004}            | 0x39   | {"type": "object"} |
+--------------------------------------------------------+--------+--------------------+

//@<OUT> pulling as table in table format {__os_type == "windows" && VER(< 8.0.19)}
+----------------------------------------------------------+-----+
| doc                                                      | _id |
+----------------------------------------------------------+-----+
| {"_id": "1", "name": "生活是美好的", "year": 1997}       | 1   |
| {"_id": "10", "name": "®7⃣⏰☕♒♣⛽🌄🌠🎨🐍🐾", "year": 2004} | 10  |
| {"_id": "11", "name": "pure ascii text", "year": 2014}   | 11  |
| {"_id": "12", "name": "látiñ text row", "year": 2016}    | 12  |
| {"_id": "2", "name": "辛德勒的名单", "year": 1993}       | 2   |
| {"_id": "3", "name": "指環王", "year": 2001}             | 3   |
| {"_id": "4", "name": "尋找尼莫", "year": 2003}           | 4   |
| {"_id": "5", "name": "الجنة الآن", "year": 2003}         | 5   |
| {"_id": "6", "name": "😁😍😠😭🙅🙉", "year": 2004}       | 6   |
| {"_id": "7", "name": "✅✨✋✈❄❔➗", "year": 2004}            | 7   |
| {"_id": "8", "name": "🚀🚑🚙🚬🚻🛀", "year": 2004}       | 8   |
| {"_id": "9", "name": "🇯🇵🈳🆕🆒", "year": 2004}         | 9   |
+----------------------------------------------------------+-----+

//@<OUT> pulling as table in table format {__os_type == "windows" && VER(>= 8.0.19)}
+----------------------------------------------------------+--------+--------------------+
| doc                                                      | _id    | _json_schema       |
+----------------------------------------------------------+--------+--------------------+
| {"_id": "1", "name": "生活是美好的", "year": 1997}       | 0x31   | {"type": "object"} |
| {"_id": "10", "name": "®7⃣⏰☕♒♣⛽🌄🌠🎨🐍🐾", "year": 2004} | 0x3130 | {"type": "object"} |
| {"_id": "11", "name": "pure ascii text", "year": 2014}   | 0x3131 | {"type": "object"} |
| {"_id": "12", "name": "látiñ text row", "year": 2016}    | 0x3132 | {"type": "object"} |
| {"_id": "2", "name": "辛德勒的名单", "year": 1993}       | 0x32   | {"type": "object"} |
| {"_id": "3", "name": "指環王", "year": 2001}             | 0x33   | {"type": "object"} |
| {"_id": "4", "name": "尋找尼莫", "year": 2003}           | 0x34   | {"type": "object"} |
| {"_id": "5", "name": "الجنة الآن", "year": 2003}         | 0x35   | {"type": "object"} |
| {"_id": "6", "name": "😁😍😠😭🙅🙉", "year": 2004}       | 0x36   | {"type": "object"} |
| {"_id": "7", "name": "✅✨✋✈❄❔➗", "year": 2004}            | 0x37   | {"type": "object"} |
| {"_id": "8", "name": "🚀🚑🚙🚬🚻🛀", "year": 2004}       | 0x38   | {"type": "object"} |
| {"_id": "9", "name": "🇯🇵🈳🆕🆒", "year": 2004}         | 0x39   | {"type": "object"} |
+----------------------------------------------------------+--------+--------------------+

//@<OUT> pulling as table in tabbed format {VER(< 8.0.19)}
doc	_id
{"_id": "1", "name": "生活是美好的", "year": 1997}	1
{"_id": "10", "name": "®7⃣⏰☕♒♣⛽🌄🌠🎨🐍🐾", "year": 2004}	10
{"_id": "11", "name": "pure ascii text", "year": 2014}	11
{"_id": "12", "name": "látiñ text row", "year": 2016}	12
{"_id": "2", "name": "辛德勒的名单", "year": 1993}	2
{"_id": "3", "name": "指環王", "year": 2001}	3
{"_id": "4", "name": "尋找尼莫", "year": 2003}	4
{"_id": "5", "name": "الجنة الآن", "year": 2003}	5
{"_id": "6", "name": "😁😍😠😭🙅🙉", "year": 2004}	6
{"_id": "7", "name": "✅✨✋✈❄❔➗", "year": 2004}	7
{"_id": "8", "name": "🚀🚑🚙🚬🚻🛀", "year": 2004}	8
{"_id": "9", "name": "🇯🇵🈳🆕🆒", "year": 2004}	9

//@<OUT> pulling as table in tabbed format {VER(>= 8.0.19)}
doc	_id	_json_schema
{"_id": "1", "name": "生活是美好的", "year": 1997}	0x31	{"type": "object"}
{"_id": "10", "name": "®7⃣⏰☕♒♣⛽🌄🌠🎨🐍🐾", "year": 2004}	0x3130	{"type": "object"}
{"_id": "11", "name": "pure ascii text", "year": 2014}	0x3131	{"type": "object"}
{"_id": "12", "name": "látiñ text row", "year": 2016}	0x3132	{"type": "object"}
{"_id": "2", "name": "辛德勒的名单", "year": 1993}	0x32	{"type": "object"}
{"_id": "3", "name": "指環王", "year": 2001}	0x33	{"type": "object"}
{"_id": "4", "name": "尋找尼莫", "year": 2003}	0x34	{"type": "object"}
{"_id": "5", "name": "الجنة الآن", "year": 2003}	0x35	{"type": "object"}
{"_id": "6", "name": "😁😍😠😭🙅🙉", "year": 2004}	0x36	{"type": "object"}
{"_id": "7", "name": "✅✨✋✈❄❔➗", "year": 2004}	0x37	{"type": "object"}
{"_id": "8", "name": "🚀🚑🚙🚬🚻🛀", "year": 2004}	0x38	{"type": "object"}
{"_id": "9", "name": "🇯🇵🈳🆕🆒", "year": 2004}	0x39	{"type": "object"}

//@<OUT> pulling as table in vertical format {VER(< 8.0.19)}
*************************** 1. row ***************************
doc: {"_id": "1", "name": "生活是美好的", "year": 1997}
_id: 1
*************************** 2. row ***************************
doc: {"_id": "10", "name": "®7⃣⏰☕♒♣⛽🌄🌠🎨🐍🐾", "year": 2004}
_id: 10
*************************** 3. row ***************************
doc: {"_id": "11", "name": "pure ascii text", "year": 2014}
_id: 11
*************************** 4. row ***************************
doc: {"_id": "12", "name": "látiñ text row", "year": 2016}
_id: 12
*************************** 5. row ***************************
doc: {"_id": "2", "name": "辛德勒的名单", "year": 1993}
_id: 2
*************************** 6. row ***************************
doc: {"_id": "3", "name": "指環王", "year": 2001}
_id: 3
*************************** 7. row ***************************
doc: {"_id": "4", "name": "尋找尼莫", "year": 2003}
_id: 4
*************************** 8. row ***************************
doc: {"_id": "5", "name": "الجنة الآن", "year": 2003}
_id: 5
*************************** 9. row ***************************
doc: {"_id": "6", "name": "😁😍😠😭🙅🙉", "year": 2004}
_id: 6
*************************** 10. row ***************************
doc: {"_id": "7", "name": "✅✨✋✈❄❔➗", "year": 2004}
_id: 7
*************************** 11. row ***************************
doc: {"_id": "8", "name": "🚀🚑🚙🚬🚻🛀", "year": 2004}
_id: 8
*************************** 12. row ***************************
doc: {"_id": "9", "name": "🇯🇵🈳🆕🆒", "year": 2004}
_id: 9

//@<OUT> pulling as table in vertical format {VER(>= 8.0.19)}
*************************** 1. row ***************************
         doc: {"_id": "1", "name": "生活是美好的", "year": 1997}
         _id: 0x31
_json_schema: {"type": "object"}
*************************** 2. row ***************************
         doc: {"_id": "10", "name": "®7⃣⏰☕♒♣⛽🌄🌠🎨🐍🐾", "year": 2004}
         _id: 0x3130
_json_schema: {"type": "object"}
*************************** 3. row ***************************
         doc: {"_id": "11", "name": "pure ascii text", "year": 2014}
         _id: 0x3131
_json_schema: {"type": "object"}
*************************** 4. row ***************************
         doc: {"_id": "12", "name": "látiñ text row", "year": 2016}
         _id: 0x3132
_json_schema: {"type": "object"}
*************************** 5. row ***************************
         doc: {"_id": "2", "name": "辛德勒的名单", "year": 1993}
         _id: 0x32
_json_schema: {"type": "object"}
*************************** 6. row ***************************
         doc: {"_id": "3", "name": "指環王", "year": 2001}
         _id: 0x33
_json_schema: {"type": "object"}
*************************** 7. row ***************************
         doc: {"_id": "4", "name": "尋找尼莫", "year": 2003}
         _id: 0x34
_json_schema: {"type": "object"}
*************************** 8. row ***************************
         doc: {"_id": "5", "name": "الجنة الآن", "year": 2003}
         _id: 0x35
_json_schema: {"type": "object"}
*************************** 9. row ***************************
         doc: {"_id": "6", "name": "😁😍😠😭🙅🙉", "year": 2004}
         _id: 0x36
_json_schema: {"type": "object"}
*************************** 10. row ***************************
         doc: {"_id": "7", "name": "✅✨✋✈❄❔➗", "year": 2004}
         _id: 0x37
_json_schema: {"type": "object"}
*************************** 11. row ***************************
         doc: {"_id": "8", "name": "🚀🚑🚙🚬🚻🛀", "year": 2004}
         _id: 0x38
_json_schema: {"type": "object"}
*************************** 12. row ***************************
         doc: {"_id": "9", "name": "🇯🇵🈳🆕🆒", "year": 2004}
         _id: 0x39
_json_schema: {"type": "object"}


//@<OUT> dump a few rows to get a table with narrow values only
+-------+----------+------+--------------------+
| col1  | col2     | col3 | col4               |
+-------+----------+------+--------------------+
| hello | 0.809643 | 0    | bla bla            |
| world |     NULL | 1    | bla blabla blaaaaa |
| NULL  |        1 | 1    | NULL               |
| hello | 0.809643 | NULL | bla bla            |
| world |     NULL | 0    | bla blabla blaaaaa |
| NULL  |        1 | 1    | NULL               |
| hello | 0.809643 | 1    | bla bla            |
| world |     NULL | NULL | bla blabla blaaaaa |
| NULL  |        1 | 0    | NULL               |
| hello | 0.809643 | 1    | bla bla            |
+-------+----------+------+--------------------+

//@<OUT> dump a few rows to get a table with slightly wider values
+-----------------------------+------------------+------+-----------------------------------------------------------------+
| col1                        | col2             | col3 | col4                                                            |
+-----------------------------+------------------+------+-----------------------------------------------------------------+
| hello                       |         0.809643 | 0    | bla bla                                                         |
| world                       |             NULL | 1    | bla blabla blaaaaa                                              |
| NULL                        |                1 | 1    | NULL                                                            |
| hello                       |         0.809643 | NULL | bla bla                                                         |
| world                       |             NULL | 0    | bla blabla blaaaaa                                              |
| NULL                        |                1 | 1    | NULL                                                            |
| hello                       |         0.809643 | 1    | bla bla                                                         |
| world                       |             NULL | NULL | bla blabla blaaaaa                                              |
| NULL                        |                1 | 0    | NULL                                                            |
| hello                       |         0.809643 | 1    | bla bla                                                         |
| world                       | 0.39888085877797 | 1    | bla bla                                                         |
| foo bar                     |      0.972853873 | 85   | bla blabla blaaaaa                                              |
| fóo                         |                1 | 0    | blablablablablab lablablablablabla blablablabl ablablablablabla |
| foo–bar                     | 0.70964040738497 | 1    | bla bla                                                         |
| foo-bar                     | 0.39888085877797 | 85   | bla blabla blaaaaa                                              |
| many values                 |      0.972853873 | 0    | blablablablablab lablablablablabla blablablabl ablablablablabla |
| Park_Güell                  |                1 | 1    | bla bla                                                         |
| Ashmore_and_Cartier_Islands | 0.70964040738497 | 85   | bla blabla blaaaaa                                              |
| hello                       | 0.39888085877797 | 0    | blablablablablab lablablablablabla blablablabl ablablablablabla |
| world                       |      0.972853873 | 1    | bla bla                                                         |
+-----------------------------+------------------+------+-----------------------------------------------------------------+

//@# dump everything
@+-----------------------------+------------------+------+-----------------------------------------------------------------+@
@| col1                        | col2             | col3 | col4                                                            |@
@+-----------------------------+------------------+------+-----------------------------------------------------------------+@
@| hello                       |         0.809643 | 0    | bla bla                                                         |@
@| world                       |             NULL | 1    | bla blabla blaaaaa                                              |@
@| NULL                        |                1 | 1    | NULL                                                            |@
@| hello                       |         0.809643 | NULL | bla bla                                                         |@
@| world                       |             NULL | 0    | bla blabla blaaaaa                                              |@
@| NULL                        |                1 | 1    | NULL                                                            |@
@| hello                       |         0.809643 | 1    | bla bla                                                         |@
@| world                       |             NULL | NULL | bla blabla blaaaaa                                              |@
@| NULL                        |                1 | 0    | NULL                                                            |@
@| hello                       |         0.809643 | 1    | bla bla                                                         |@
@| world                       | 0.39888085877797 | 1    | bla bla                                                         |@
@| foo bar                     |      0.972853873 | 85   | bla blabla blaaaaa                                              |@
@| fóo                         |                1 | 0    | blablablablablab lablablablablabla blablablabl ablablablablabla |@
@| foo–bar                     | 0.70964040738497 | 1    | bla bla                                                         |@
@| foo-bar                     | 0.39888085877797 | 85   | bla blabla blaaaaa                                              |@
@| many values                 |      0.972853873 | 0    | blablablablablab lablablablablabla blablablabl ablablablablabla |@
@| Park_Güell                  |                1 | 1    | bla bla                                                         |@
@| Alfonso_Aráu                |      0.398880858 | 1    | blablablabla blablablabla blablablabla blablablabla blablablabla@
@blablablabla blablablabla blablablabla@
@blablablabla bla! |@
@| André-Marie_Ampère          |        0.9733873 | 707460108 | bla bla                                                         |@
@| Very long text but not that long really, but at least longer than before | 0.1180964040738497 | 0    | blablablabla blablablabla blablablabla blablablabla blablablabla@
@blablablabla blablablabla blablablabla@
@blablablabla bla! |@
@| hello world                 |      0.398880858 | 1    | bla bla                                                         |@
@| Alfonso_Aráu                |        0.9733873 | 707460108 | blablablabla blablablabla blablablabla blablablabla blablablabla@
@blablablabla blablablabla blablablabla@
@blablablabla bla! |@
@| André-Marie_Ampère          | 0.1180964040738497 | 0    | bla bla                                                         |@
@| Very long text but not that long really, but at least longer than before |      0.398880858 | 1    | blablablabla blablablabla blablablabla blablablabla blablablabla@
@blablablabla blablablabla blablablabla@
@blablablabla bla! |@
@| hello world                 |        0.9733873 | 707460108 | bla bla                                                         |@
@| Alfonso_Aráu                | 0.1180964040738497 | 0    | blablablabla blablablabla blablablabla blablablabla blablablabla@
@blablablabla blablablabla blablablabla@
@blablablabla bla! |@
@| André-Marie_Ampère          |      0.398880858 | 1    | bla bla                                                         |@
@| Very long text but not that long really, but at least longer than before |        0.9733873 | 707460108 | blablablabla blablablabla blablablabla blablablabla blablablabla@
@blablablabla blablablabla blablablabla@
@blablablabla bla! |@
@| hello world                 | 0.1180964040738497 | 0    | bla bla                                                         |@
@| Alfonso_Aráu                |      0.398880858 | 1    | blablablabla blablablabla blablablabla blablablabla blablablabla@
@blablablabla blablablabla blablablabla@
@blablablabla bla! |@
@| André-Marie_Ampère          |        0.9733873 | 707460108 | bla bla                                                         |@
@| Very long text but not that long really, but at least longer than before | 0.1180964040738497 | 0    | blablablabla blablablabla blablablabla blablablabla blablablabla@
@blablablabla blablablabla blablablabla@
@blablablabla bla! |@
@| hello world                 |      0.398880858 | 1    | bla bla                                                         |@
@| Alfonso_Aráu                |        0.9733873 | 707460108 | blablablabla blablablabla blablablabla blablablabla blablablabla@
@blablablabla blablablabla blablablabla@
@blablablabla bla! |@
@| André-Marie_Ampère          | 0.1180964040738497 | 0    | bla bla                                                         |@
@| Very long text but not that long really, but at least longer than before |      0.398880858 | 1    | blablablabla blablablabla blablablabla blablablabla blablablabla@
@blablablabla blablablabla blablablabla@
@blablablabla bla! |@
@+-----------------------------+------------------+------+-----------------------------------------------------------------+@

