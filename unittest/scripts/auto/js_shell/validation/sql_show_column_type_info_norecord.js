//@ Setup
|Query OK|
|Query OK|
|Query OK|

//@<OUT> Column type info disabled X
WARNING: Using a password on the command line interface can be insecure.
0

//@<OUT> Column type info enabled X
WARNING: Using a password on the command line interface can be insecure.
Field 1
Name:      `tint`
Org_name:  `tint`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      Integer
DbType:    TINY
Collation: binary (63)
Length:    4
Decimals:  0
Flags:     NUM

Field 2
Name:      `tintu`
Org_name:  `tintu`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      UInteger
DbType:    TINY
Collation: binary (63)
Length:    3
Decimals:  0
Flags:     UNSIGNED NUM

Field 3
Name:      `smallint`
Org_name:  `smallint`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      Integer
DbType:    SHORT
Collation: binary (63)
Length:    6
Decimals:  0
Flags:     NUM

Field 4
Name:      `smallintu`
Org_name:  `smallintu`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      UInteger
DbType:    SHORT
Collation: binary (63)
Length:    5
Decimals:  0
Flags:     UNSIGNED NUM

Field 5
Name:      `mediumint`
Org_name:  `mediumint`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      Integer
DbType:    INT24
Collation: binary (63)
Length:    9
Decimals:  0
Flags:     NUM

Field 6
Name:      `mediumintu`
Org_name:  `mediumintu`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      UInteger
DbType:    INT24
Collation: binary (63)
Length:    8
Decimals:  0
Flags:     UNSIGNED NUM

Field 7
Name:      `int`
Org_name:  `int`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      Integer
DbType:    LONG
Collation: binary (63)
Length:    11
Decimals:  0
Flags:     NUM

Field 8
Name:      `intu`
Org_name:  `intu`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      UInteger
DbType:    LONG
Collation: binary (63)
Length:    10
Decimals:  0
Flags:     UNSIGNED NUM

Field 9
Name:      `bigint`
Org_name:  `bigint`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      Integer
DbType:    LONGLONG
Collation: binary (63)
Length:    20
Decimals:  0
Flags:     NOT_NULL PRI_KEY AUTO_INCREMENT NUM

Field 10
Name:      `bigintu`
Org_name:  `bigintu`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      UInteger
DbType:    LONGLONG
Collation: binary (63)
Length:    20
Decimals:  0
Flags:     UNSIGNED NUM

Field 11
Name:      `float`
Org_name:  `float`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      Float
DbType:    FLOAT
Collation: binary (63)
Length:    10
Decimals:  2
Flags:     NUM

Field 12
Name:      `double`
Org_name:  `double`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      Double
DbType:    DOUBLE
Collation: binary (63)
Length:    22
Decimals:  31
Flags:     NUM

Field 13
Name:      `decimal`
Org_name:  `decimal`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      Decimal
DbType:    NEWDECIMAL
Collation: binary (63)
Length:    12
Decimals:  2
Flags:     NUM

Field 14
Name:      `date`
Org_name:  `date`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      Date
DbType:    DATE
Collation: binary (63)
Length:    10
Decimals:  0
Flags:     BINARY

Field 15
Name:      `datetime`
Org_name:  `datetime`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      DateTime
DbType:    DATETIME
Collation: binary (63)
Length:    19
Decimals:  0
Flags:     BINARY

Field 16
Name:      `timestamp`
Org_name:  `timestamp`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      DateTime
DbType:    TIMESTAMP
Collation: binary (63)
Length:    19
Decimals:  0
Flags:     BINARY TIMESTAMP

Field 17
Name:      `time`
Org_name:  `time`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      Time
DbType:    TIME
Collation: binary (63)
Length:    10
Decimals:  0
Flags:     BINARY

Field 18
Name:      `year`
Org_name:  `year`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      UInteger
DbType:    TINY
Collation: binary (63)
Length:    4
Decimals:  0
Flags:     NOT_NULL UNSIGNED NUM

Field 19
Name:      `tinyblob`
Org_name:  `tinyblob`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      Bytes
DbType:    BLOB
Collation: binary (63)
Length:    255
Decimals:  0
Flags:     BLOB BINARY

Field 20
Name:      `tinytext`
Org_name:  `tinytext`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      String
DbType:    BLOB
Collation: utf8mb4_0900_ai_ci (255)
Length:    255
Decimals:  0
Flags:     BLOB

Field 21
Name:      `blob`
Org_name:  `blob`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      Bytes
DbType:    BLOB
Collation: binary (63)
Length:    65535
Decimals:  0
Flags:     BLOB BINARY

Field 22
Name:      `text`
Org_name:  `text`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      String
DbType:    BLOB
Collation: utf8mb4_0900_ai_ci (255)
Length:    65535
Decimals:  0
Flags:     BLOB

Field 23
Name:      `mediumblob`
Org_name:  `mediumblob`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      Bytes
DbType:    BLOB
Collation: binary (63)
Length:    16777215
Decimals:  0
Flags:     BLOB BINARY

Field 24
Name:      `mediumtext`
Org_name:  `mediumtext`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      String
DbType:    BLOB
Collation: utf8mb4_0900_ai_ci (255)
Length:    16777215
Decimals:  0
Flags:     BLOB

Field 25
Name:      `longblob`
Org_name:  `longblob`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      Bytes
DbType:    BLOB
Collation: binary (63)
Length:    4294967295
Decimals:  0
Flags:     BLOB BINARY

Field 26
Name:      `longtext`
Org_name:  `longtext`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      String
DbType:    BLOB
Collation: utf8mb4_0900_ai_ci (255)
Length:    4294967295
Decimals:  0
Flags:     BLOB

Field 27
Name:      `enum`
Org_name:  `enum`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      Enum
DbType:    STRING
Collation: utf8mb4_0900_ai_ci (255)
Length:    0
Decimals:  0
Flags:     ENUM

Field 28
Name:      `set`
Org_name:  `set`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      Set
DbType:    STRING
Collation: utf8mb4_0900_ai_ci (255)
Length:    0
Decimals:  0
Flags:     SET

Field 29
Name:      `json`
Org_name:  `json`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      Json
DbType:    JSON
Collation: binary (63)
Length:    4294967295
Decimals:  0
Flags:     BLOB BINARY

Field 30
Name:      `geometry`
Org_name:  `geometry`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      Geometry
DbType:    GEOMETRY
Collation: binary (63)
Length:    0
Decimals:  0
Flags:     BLOB BINARY

Field 31
Name:      `bool`
Org_name:  `bool`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      Integer
DbType:    TINY
Collation: binary (63)
Length:    1
Decimals:  0
Flags:     NUM

Field 32
Name:      `char`
Org_name:  `char`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      String
DbType:    VAR_STRING
Collation: utf8mb4_0900_ai_ci (255)
Length:    40
Decimals:  0
Flags:

Field 33
Name:      `varchar`
Org_name:  `varchar`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      String
DbType:    VAR_STRING
Collation: utf8mb4_0900_ai_ci (255)
Length:    80
Decimals:  0
Flags:

Field 34
Name:      `binary`
Org_name:  `binary`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      Bytes
DbType:    VAR_STRING
Collation: binary (63)
Length:    20
Decimals:  0
Flags:     BINARY

Field 35
Name:      `varbinary`
Org_name:  `varbinary`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      Bytes
DbType:    VAR_STRING
Collation: binary (63)
Length:    20
Decimals:  0
Flags:     BINARY

0

//@<OUT> Column type info enabled X JSON
{
    "warning": "Using a password on the command line interface can be insecure.\n"
}
{
    "Field 1": {
        "Name": "`tint`",
        "Org_name": "`tint`",
        "Catalog": "`def`",
        "Database": "`scti_test`",
        "Table": "`t`",
        "Org_table": "`t`",
        "Type": "Integer",
        "DbType": "TINY",
        "Collation": "binary (63)",
        "Length": "4",
        "Decimals": "0",
        "Flags": "NUM"
    },
    "Field 2": {
        "Name": "`tintu`",
        "Org_name": "`tintu`",
        "Catalog": "`def`",
        "Database": "`scti_test`",
        "Table": "`t`",
        "Org_table": "`t`",
        "Type": "UInteger",
        "DbType": "TINY",
        "Collation": "binary (63)",
        "Length": "3",
        "Decimals": "0",
        "Flags": "UNSIGNED NUM"
    },
    "Field 3": {
        "Name": "`smallint`",
        "Org_name": "`smallint`",
        "Catalog": "`def`",
        "Database": "`scti_test`",
        "Table": "`t`",
        "Org_table": "`t`",
        "Type": "Integer",
        "DbType": "SHORT",
        "Collation": "binary (63)",
        "Length": "6",
        "Decimals": "0",
        "Flags": "NUM"
    },
    "Field 4": {
        "Name": "`smallintu`",
        "Org_name": "`smallintu`",
        "Catalog": "`def`",
        "Database": "`scti_test`",
        "Table": "`t`",
        "Org_table": "`t`",
        "Type": "UInteger",
        "DbType": "SHORT",
        "Collation": "binary (63)",
        "Length": "5",
        "Decimals": "0",
        "Flags": "UNSIGNED NUM"
    },
    "Field 5": {
        "Name": "`mediumint`",
        "Org_name": "`mediumint`",
        "Catalog": "`def`",
        "Database": "`scti_test`",
        "Table": "`t`",
        "Org_table": "`t`",
        "Type": "Integer",
        "DbType": "INT24",
        "Collation": "binary (63)",
        "Length": "9",
        "Decimals": "0",
        "Flags": "NUM"
    },
    "Field 6": {
        "Name": "`mediumintu`",
        "Org_name": "`mediumintu`",
        "Catalog": "`def`",
        "Database": "`scti_test`",
        "Table": "`t`",
        "Org_table": "`t`",
        "Type": "UInteger",
        "DbType": "INT24",
        "Collation": "binary (63)",
        "Length": "8",
        "Decimals": "0",
        "Flags": "UNSIGNED NUM"
    },
    "Field 7": {
        "Name": "`int`",
        "Org_name": "`int`",
        "Catalog": "`def`",
        "Database": "`scti_test`",
        "Table": "`t`",
        "Org_table": "`t`",
        "Type": "Integer",
        "DbType": "LONG",
        "Collation": "binary (63)",
        "Length": "11",
        "Decimals": "0",
        "Flags": "NUM"
    },
    "Field 8": {
        "Name": "`intu`",
        "Org_name": "`intu`",
        "Catalog": "`def`",
        "Database": "`scti_test`",
        "Table": "`t`",
        "Org_table": "`t`",
        "Type": "UInteger",
        "DbType": "LONG",
        "Collation": "binary (63)",
        "Length": "10",
        "Decimals": "0",
        "Flags": "UNSIGNED NUM"
    },
    "Field 9": {
        "Name": "`bigint`",
        "Org_name": "`bigint`",
        "Catalog": "`def`",
        "Database": "`scti_test`",
        "Table": "`t`",
        "Org_table": "`t`",
        "Type": "Integer",
        "DbType": "LONGLONG",
        "Collation": "binary (63)",
        "Length": "20",
        "Decimals": "0",
        "Flags": "NOT_NULL PRI_KEY AUTO_INCREMENT NUM"
    },
    "Field 10": {
        "Name": "`bigintu`",
        "Org_name": "`bigintu`",
        "Catalog": "`def`",
        "Database": "`scti_test`",
        "Table": "`t`",
        "Org_table": "`t`",
        "Type": "UInteger",
        "DbType": "LONGLONG",
        "Collation": "binary (63)",
        "Length": "20",
        "Decimals": "0",
        "Flags": "UNSIGNED NUM"
    },
    "Field 11": {
        "Name": "`float`",
        "Org_name": "`float`",
        "Catalog": "`def`",
        "Database": "`scti_test`",
        "Table": "`t`",
        "Org_table": "`t`",
        "Type": "Float",
        "DbType": "FLOAT",
        "Collation": "binary (63)",
        "Length": "10",
        "Decimals": "2",
        "Flags": "NUM"
    },
    "Field 12": {
        "Name": "`double`",
        "Org_name": "`double`",
        "Catalog": "`def`",
        "Database": "`scti_test`",
        "Table": "`t`",
        "Org_table": "`t`",
        "Type": "Double",
        "DbType": "DOUBLE",
        "Collation": "binary (63)",
        "Length": "22",
        "Decimals": "31",
        "Flags": "NUM"
    },
    "Field 13": {
        "Name": "`decimal`",
        "Org_name": "`decimal`",
        "Catalog": "`def`",
        "Database": "`scti_test`",
        "Table": "`t`",
        "Org_table": "`t`",
        "Type": "Decimal",
        "DbType": "NEWDECIMAL",
        "Collation": "binary (63)",
        "Length": "12",
        "Decimals": "2",
        "Flags": "NUM"
    },
    "Field 14": {
        "Name": "`date`",
        "Org_name": "`date`",
        "Catalog": "`def`",
        "Database": "`scti_test`",
        "Table": "`t`",
        "Org_table": "`t`",
        "Type": "Date",
        "DbType": "DATE",
        "Collation": "binary (63)",
        "Length": "10",
        "Decimals": "0",
        "Flags": "BINARY"
    },
    "Field 15": {
        "Name": "`datetime`",
        "Org_name": "`datetime`",
        "Catalog": "`def`",
        "Database": "`scti_test`",
        "Table": "`t`",
        "Org_table": "`t`",
        "Type": "DateTime",
        "DbType": "DATETIME",
        "Collation": "binary (63)",
        "Length": "19",
        "Decimals": "0",
        "Flags": "BINARY"
    },
    "Field 16": {
        "Name": "`timestamp`",
        "Org_name": "`timestamp`",
        "Catalog": "`def`",
        "Database": "`scti_test`",
        "Table": "`t`",
        "Org_table": "`t`",
        "Type": "DateTime",
        "DbType": "TIMESTAMP",
        "Collation": "binary (63)",
        "Length": "19",
        "Decimals": "0",
        "Flags": "BINARY TIMESTAMP"
    },
    "Field 17": {
        "Name": "`time`",
        "Org_name": "`time`",
        "Catalog": "`def`",
        "Database": "`scti_test`",
        "Table": "`t`",
        "Org_table": "`t`",
        "Type": "Time",
        "DbType": "TIME",
        "Collation": "binary (63)",
        "Length": "10",
        "Decimals": "0",
        "Flags": "BINARY"
    },
    "Field 18": {
        "Name": "`year`",
        "Org_name": "`year`",
        "Catalog": "`def`",
        "Database": "`scti_test`",
        "Table": "`t`",
        "Org_table": "`t`",
        "Type": "UInteger",
        "DbType": "TINY",
        "Collation": "binary (63)",
        "Length": "4",
        "Decimals": "0",
        "Flags": "NOT_NULL UNSIGNED NUM"
    },
    "Field 19": {
        "Name": "`tinyblob`",
        "Org_name": "`tinyblob`",
        "Catalog": "`def`",
        "Database": "`scti_test`",
        "Table": "`t`",
        "Org_table": "`t`",
        "Type": "Bytes",
        "DbType": "BLOB",
        "Collation": "binary (63)",
        "Length": "255",
        "Decimals": "0",
        "Flags": "BLOB BINARY"
    },
    "Field 20": {
        "Name": "`tinytext`",
        "Org_name": "`tinytext`",
        "Catalog": "`def`",
        "Database": "`scti_test`",
        "Table": "`t`",
        "Org_table": "`t`",
        "Type": "String",
        "DbType": "BLOB",
        "Collation": "utf8mb4_0900_ai_ci (255)",
        "Length": "255",
        "Decimals": "0",
        "Flags": "BLOB"
    },
    "Field 21": {
        "Name": "`blob`",
        "Org_name": "`blob`",
        "Catalog": "`def`",
        "Database": "`scti_test`",
        "Table": "`t`",
        "Org_table": "`t`",
        "Type": "Bytes",
        "DbType": "BLOB",
        "Collation": "binary (63)",
        "Length": "65535",
        "Decimals": "0",
        "Flags": "BLOB BINARY"
    },
    "Field 22": {
        "Name": "`text`",
        "Org_name": "`text`",
        "Catalog": "`def`",
        "Database": "`scti_test`",
        "Table": "`t`",
        "Org_table": "`t`",
        "Type": "String",
        "DbType": "BLOB",
        "Collation": "utf8mb4_0900_ai_ci (255)",
        "Length": "65535",
        "Decimals": "0",
        "Flags": "BLOB"
    },
    "Field 23": {
        "Name": "`mediumblob`",
        "Org_name": "`mediumblob`",
        "Catalog": "`def`",
        "Database": "`scti_test`",
        "Table": "`t`",
        "Org_table": "`t`",
        "Type": "Bytes",
        "DbType": "BLOB",
        "Collation": "binary (63)",
        "Length": "16777215",
        "Decimals": "0",
        "Flags": "BLOB BINARY"
    },
    "Field 24": {
        "Name": "`mediumtext`",
        "Org_name": "`mediumtext`",
        "Catalog": "`def`",
        "Database": "`scti_test`",
        "Table": "`t`",
        "Org_table": "`t`",
        "Type": "String",
        "DbType": "BLOB",
        "Collation": "utf8mb4_0900_ai_ci (255)",
        "Length": "16777215",
        "Decimals": "0",
        "Flags": "BLOB"
    },
    "Field 25": {
        "Name": "`longblob`",
        "Org_name": "`longblob`",
        "Catalog": "`def`",
        "Database": "`scti_test`",
        "Table": "`t`",
        "Org_table": "`t`",
        "Type": "Bytes",
        "DbType": "BLOB",
        "Collation": "binary (63)",
        "Length": "4294967295",
        "Decimals": "0",
        "Flags": "BLOB BINARY"
    },
    "Field 26": {
        "Name": "`longtext`",
        "Org_name": "`longtext`",
        "Catalog": "`def`",
        "Database": "`scti_test`",
        "Table": "`t`",
        "Org_table": "`t`",
        "Type": "String",
        "DbType": "BLOB",
        "Collation": "utf8mb4_0900_ai_ci (255)",
        "Length": "4294967295",
        "Decimals": "0",
        "Flags": "BLOB"
    },
    "Field 27": {
        "Name": "`enum`",
        "Org_name": "`enum`",
        "Catalog": "`def`",
        "Database": "`scti_test`",
        "Table": "`t`",
        "Org_table": "`t`",
        "Type": "Enum",
        "DbType": "STRING",
        "Collation": "utf8mb4_0900_ai_ci (255)",
        "Length": "0",
        "Decimals": "0",
        "Flags": "ENUM"
    },
    "Field 28": {
        "Name": "`set`",
        "Org_name": "`set`",
        "Catalog": "`def`",
        "Database": "`scti_test`",
        "Table": "`t`",
        "Org_table": "`t`",
        "Type": "Set",
        "DbType": "STRING",
        "Collation": "utf8mb4_0900_ai_ci (255)",
        "Length": "0",
        "Decimals": "0",
        "Flags": "SET"
    },
    "Field 29": {
        "Name": "`json`",
        "Org_name": "`json`",
        "Catalog": "`def`",
        "Database": "`scti_test`",
        "Table": "`t`",
        "Org_table": "`t`",
        "Type": "Json",
        "DbType": "JSON",
        "Collation": "binary (63)",
        "Length": "4294967295",
        "Decimals": "0",
        "Flags": "BLOB BINARY"
    },
    "Field 30": {
        "Name": "`geometry`",
        "Org_name": "`geometry`",
        "Catalog": "`def`",
        "Database": "`scti_test`",
        "Table": "`t`",
        "Org_table": "`t`",
        "Type": "Geometry",
        "DbType": "GEOMETRY",
        "Collation": "binary (63)",
        "Length": "0",
        "Decimals": "0",
        "Flags": "BLOB BINARY"
    },
    "Field 31": {
        "Name": "`bool`",
        "Org_name": "`bool`",
        "Catalog": "`def`",
        "Database": "`scti_test`",
        "Table": "`t`",
        "Org_table": "`t`",
        "Type": "Integer",
        "DbType": "TINY",
        "Collation": "binary (63)",
        "Length": "1",
        "Decimals": "0",
        "Flags": "NUM"
    },
    "Field 32": {
        "Name": "`char`",
        "Org_name": "`char`",
        "Catalog": "`def`",
        "Database": "`scti_test`",
        "Table": "`t`",
        "Org_table": "`t`",
        "Type": "String",
        "DbType": "VAR_STRING",
        "Collation": "utf8mb4_0900_ai_ci (255)",
        "Length": "40",
        "Decimals": "0",
        "Flags": ""
    },
    "Field 33": {
        "Name": "`varchar`",
        "Org_name": "`varchar`",
        "Catalog": "`def`",
        "Database": "`scti_test`",
        "Table": "`t`",
        "Org_table": "`t`",
        "Type": "String",
        "DbType": "VAR_STRING",
        "Collation": "utf8mb4_0900_ai_ci (255)",
        "Length": "80",
        "Decimals": "0",
        "Flags": ""
    },
    "Field 34": {
        "Name": "`binary`",
        "Org_name": "`binary`",
        "Catalog": "`def`",
        "Database": "`scti_test`",
        "Table": "`t`",
        "Org_table": "`t`",
        "Type": "Bytes",
        "DbType": "VAR_STRING",
        "Collation": "binary (63)",
        "Length": "20",
        "Decimals": "0",
        "Flags": "BINARY"
    },
    "Field 35": {
        "Name": "`varbinary`",
        "Org_name": "`varbinary`",
        "Catalog": "`def`",
        "Database": "`scti_test`",
        "Table": "`t`",
        "Org_table": "`t`",
        "Type": "Bytes",
        "DbType": "VAR_STRING",
        "Collation": "binary (63)",
        "Length": "20",
        "Decimals": "0",
        "Flags": "BINARY"
    }
}
{
    "hasData": true,
    "rows": [],
    "executionTime": [[*]],
    "affectedRowCount": 0,
    "affectedItemsCount": 0,
    "warningCount": 0,
    "warningsCount": 0,
    "warnings": [],
    "info": "",
    "autoIncrementValue": 0
}
0

//@<OUT> Column type info disabled classic
WARNING: Using a password on the command line interface can be insecure.
0

//@<OUT> Column type info enabled classic
WARNING: Using a password on the command line interface can be insecure.
Field 1
Name:      `tint`
Org_name:  `tint`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      Integer
DbType:    TINY
Collation: binary (63)
Length:    4
Decimals:  0
Flags:     NUM

Field 2
Name:      `tintu`
Org_name:  `tintu`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      UInteger
DbType:    TINY
Collation: binary (63)
Length:    3
Decimals:  0
Flags:     UNSIGNED NUM

Field 3
Name:      `smallint`
Org_name:  `smallint`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      Integer
DbType:    SHORT
Collation: binary (63)
Length:    6
Decimals:  0
Flags:     NUM

Field 4
Name:      `smallintu`
Org_name:  `smallintu`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      UInteger
DbType:    SHORT
Collation: binary (63)
Length:    5
Decimals:  0
Flags:     UNSIGNED NUM

Field 5
Name:      `mediumint`
Org_name:  `mediumint`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      Integer
DbType:    INT24
Collation: binary (63)
Length:    9
Decimals:  0
Flags:     NUM

Field 6
Name:      `mediumintu`
Org_name:  `mediumintu`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      UInteger
DbType:    INT24
Collation: binary (63)
Length:    8
Decimals:  0
Flags:     UNSIGNED NUM

Field 7
Name:      `int`
Org_name:  `int`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      Integer
DbType:    LONG
Collation: binary (63)
Length:    11
Decimals:  0
Flags:     NUM

Field 8
Name:      `intu`
Org_name:  `intu`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      UInteger
DbType:    LONG
Collation: binary (63)
Length:    10
Decimals:  0
Flags:     UNSIGNED NUM

Field 9
Name:      `bigint`
Org_name:  `bigint`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      Integer
DbType:    LONGLONG
Collation: binary (63)
Length:    20
Decimals:  0
Flags:     NOT_NULL PRI_KEY AUTO_INCREMENT NUM PART_KEY

Field 10
Name:      `bigintu`
Org_name:  `bigintu`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      UInteger
DbType:    LONGLONG
Collation: binary (63)
Length:    20
Decimals:  0
Flags:     UNSIGNED NUM

Field 11
Name:      `float`
Org_name:  `float`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      Float
DbType:    FLOAT
Collation: binary (63)
Length:    10
Decimals:  2
Flags:     NUM

Field 12
Name:      `double`
Org_name:  `double`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      Double
DbType:    DOUBLE
Collation: binary (63)
Length:    22
Decimals:  31
Flags:     NUM

Field 13
Name:      `decimal`
Org_name:  `decimal`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      Decimal
DbType:    NEWDECIMAL
Collation: binary (63)
Length:    12
Decimals:  2
Flags:     NUM

Field 14
Name:      `date`
Org_name:  `date`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      Date
DbType:    DATE
Collation: binary (63)
Length:    10
Decimals:  0
Flags:     BINARY

Field 15
Name:      `datetime`
Org_name:  `datetime`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      DateTime
DbType:    DATETIME
Collation: binary (63)
Length:    19
Decimals:  0
Flags:     BINARY

Field 16
Name:      `timestamp`
Org_name:  `timestamp`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      DateTime
DbType:    TIMESTAMP
Collation: binary (63)
Length:    19
Decimals:  0
Flags:     BINARY

Field 17
Name:      `time`
Org_name:  `time`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      Time
DbType:    TIME
Collation: binary (63)
Length:    10
Decimals:  0
Flags:     BINARY

Field 18
Name:      `year`
Org_name:  `year`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      UInteger
DbType:    YEAR
Collation: binary (63)
Length:    4
Decimals:  0
Flags:     NOT_NULL UNSIGNED ZEROFILL NO_DEFAULT_VALUE NUM

Field 19
Name:      `tinyblob`
Org_name:  `tinyblob`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      Bytes
DbType:    BLOB
Collation: binary (63)
Length:    255
Decimals:  0
Flags:     BLOB BINARY

Field 20
Name:      `tinytext`
Org_name:  `tinytext`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      String
DbType:    BLOB
Collation: utf8mb4_0900_ai_ci (255)
Length:    1020
Decimals:  0
Flags:     BLOB

Field 21
Name:      `blob`
Org_name:  `blob`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      Bytes
DbType:    BLOB
Collation: binary (63)
Length:    65535
Decimals:  0
Flags:     BLOB BINARY

Field 22
Name:      `text`
Org_name:  `text`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      String
DbType:    BLOB
Collation: utf8mb4_0900_ai_ci (255)
Length:    262140
Decimals:  0
Flags:     BLOB

Field 23
Name:      `mediumblob`
Org_name:  `mediumblob`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      Bytes
DbType:    BLOB
Collation: binary (63)
Length:    16777215
Decimals:  0
Flags:     BLOB BINARY

Field 24
Name:      `mediumtext`
Org_name:  `mediumtext`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      String
DbType:    BLOB
Collation: utf8mb4_0900_ai_ci (255)
Length:    67108860
Decimals:  0
Flags:     BLOB

Field 25
Name:      `longblob`
Org_name:  `longblob`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      Bytes
DbType:    BLOB
Collation: binary (63)
Length:    4294967295
Decimals:  0
Flags:     BLOB BINARY

Field 26
Name:      `longtext`
Org_name:  `longtext`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      String
DbType:    BLOB
Collation: utf8mb4_0900_ai_ci (255)
Length:    4294967295
Decimals:  0
Flags:     BLOB

Field 27
Name:      `enum`
Org_name:  `enum`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      Enum
DbType:    STRING
Collation: utf8mb4_0900_ai_ci (255)
Length:    4
Decimals:  0
Flags:     ENUM

Field 28
Name:      `set`
Org_name:  `set`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      Set
DbType:    STRING
Collation: utf8mb4_0900_ai_ci (255)
Length:    20
Decimals:  0
Flags:     SET

Field 29
Name:      `json`
Org_name:  `json`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      Json
DbType:    JSON
Collation: binary (63)
Length:    4294967295
Decimals:  0
Flags:     BLOB BINARY

Field 30
Name:      `geometry`
Org_name:  `geometry`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      Geometry
DbType:    GEOMETRY
Collation: binary (63)
Length:    4294967295
Decimals:  0
Flags:     BLOB BINARY

Field 31
Name:      `bool`
Org_name:  `bool`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      Integer
DbType:    TINY
Collation: binary (63)
Length:    1
Decimals:  0
Flags:     NUM

Field 32
Name:      `char`
Org_name:  `char`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      String
DbType:    STRING
Collation: utf8mb4_0900_ai_ci (255)
Length:    40
Decimals:  0
Flags:

Field 33
Name:      `varchar`
Org_name:  `varchar`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      String
DbType:    VAR_STRING
Collation: utf8mb4_0900_ai_ci (255)
Length:    80
Decimals:  0
Flags:

Field 34
Name:      `binary`
Org_name:  `binary`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      Bytes
DbType:    STRING
Collation: binary (63)
Length:    20
Decimals:  0
Flags:     BINARY

Field 35
Name:      `varbinary`
Org_name:  `varbinary`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      Bytes
DbType:    VAR_STRING
Collation: binary (63)
Length:    20
Decimals:  0
Flags:     BINARY

0

//@<OUT> Column type info enabled X 5.7
WARNING: Using a password on the command line interface can be insecure.
Field 1
Name:      `tint`
Org_name:  `tint`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      Integer
DbType:    TINY
Collation: binary (63)
Length:    4
Decimals:  0
Flags:     NUM

Field 2
Name:      `tintu`
Org_name:  `tintu`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      UInteger
DbType:    TINY
Collation: binary (63)
Length:    3
Decimals:  0
Flags:     UNSIGNED NUM

Field 3
Name:      `smallint`
Org_name:  `smallint`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      Integer
DbType:    SHORT
Collation: binary (63)
Length:    6
Decimals:  0
Flags:     NUM

Field 4
Name:      `smallintu`
Org_name:  `smallintu`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      UInteger
DbType:    SHORT
Collation: binary (63)
Length:    5
Decimals:  0
Flags:     UNSIGNED NUM

Field 5
Name:      `mediumint`
Org_name:  `mediumint`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      Integer
DbType:    INT24
Collation: binary (63)
Length:    9
Decimals:  0
Flags:     NUM

Field 6
Name:      `mediumintu`
Org_name:  `mediumintu`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      UInteger
DbType:    INT24
Collation: binary (63)
Length:    8
Decimals:  0
Flags:     UNSIGNED NUM

Field 7
Name:      `int`
Org_name:  `int`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      Integer
DbType:    LONG
Collation: binary (63)
Length:    11
Decimals:  0
Flags:     NUM

Field 8
Name:      `intu`
Org_name:  `intu`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      UInteger
DbType:    LONG
Collation: binary (63)
Length:    10
Decimals:  0
Flags:     UNSIGNED NUM

Field 9
Name:      `bigint`
Org_name:  `bigint`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      Integer
DbType:    LONGLONG
Collation: binary (63)
Length:    20
Decimals:  0
Flags:     NOT_NULL PRI_KEY AUTO_INCREMENT NUM

Field 10
Name:      `bigintu`
Org_name:  `bigintu`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      UInteger
DbType:    LONGLONG
Collation: binary (63)
Length:    20
Decimals:  0
Flags:     UNSIGNED NUM

Field 11
Name:      `float`
Org_name:  `float`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      Float
DbType:    FLOAT
Collation: binary (63)
Length:    10
Decimals:  2
Flags:     NUM

Field 12
Name:      `double`
Org_name:  `double`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      Double
DbType:    DOUBLE
Collation: binary (63)
Length:    22
Decimals:  31
Flags:     NUM

Field 13
Name:      `decimal`
Org_name:  `decimal`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      Decimal
DbType:    NEWDECIMAL
Collation: binary (63)
Length:    12
Decimals:  2
Flags:     NUM

Field 14
Name:      `date`
Org_name:  `date`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      Date
DbType:    DATE
Collation: binary (63)
Length:    10
Decimals:  0
Flags:     BINARY

Field 15
Name:      `datetime`
Org_name:  `datetime`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      DateTime
DbType:    DATETIME
Collation: binary (63)
Length:    19
Decimals:  0
Flags:     BINARY

Field 16
Name:      `timestamp`
Org_name:  `timestamp`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      DateTime
DbType:    TIMESTAMP
Collation: binary (63)
Length:    19
Decimals:  0
Flags:     BINARY TIMESTAMP

Field 17
Name:      `time`
Org_name:  `time`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      Time
DbType:    TIME
Collation: binary (63)
Length:    10
Decimals:  0
Flags:     BINARY

Field 18
Name:      `year`
Org_name:  `year`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      UInteger
DbType:    TINY
Collation: binary (63)
Length:    4
Decimals:  0
Flags:     NOT_NULL UNSIGNED NUM

Field 19
Name:      `tinyblob`
Org_name:  `tinyblob`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      Bytes
DbType:    BLOB
Collation: binary (63)
Length:    255
Decimals:  0
Flags:     BLOB BINARY

Field 20
Name:      `tinytext`
Org_name:  `tinytext`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      String
DbType:    BLOB
Collation: latin1_swedish_ci (8)
Length:    255
Decimals:  0
Flags:     BLOB

Field 21
Name:      `blob`
Org_name:  `blob`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      Bytes
DbType:    BLOB
Collation: binary (63)
Length:    65535
Decimals:  0
Flags:     BLOB BINARY

Field 22
Name:      `text`
Org_name:  `text`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      String
DbType:    BLOB
Collation: latin1_swedish_ci (8)
Length:    65535
Decimals:  0
Flags:     BLOB

Field 23
Name:      `mediumblob`
Org_name:  `mediumblob`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      Bytes
DbType:    BLOB
Collation: binary (63)
Length:    16777215
Decimals:  0
Flags:     BLOB BINARY

Field 24
Name:      `mediumtext`
Org_name:  `mediumtext`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      String
DbType:    BLOB
Collation: latin1_swedish_ci (8)
Length:    16777215
Decimals:  0
Flags:     BLOB

Field 25
Name:      `longblob`
Org_name:  `longblob`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      Bytes
DbType:    BLOB
Collation: binary (63)
Length:    4294967295
Decimals:  0
Flags:     BLOB BINARY

Field 26
Name:      `longtext`
Org_name:  `longtext`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      String
DbType:    BLOB
Collation: latin1_swedish_ci (8)
Length:    4294967295
Decimals:  0
Flags:     BLOB

Field 27
Name:      `enum`
Org_name:  `enum`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      Enum
DbType:    STRING
Collation: latin1_swedish_ci (8)
Length:    1
Decimals:  0
Flags:     ENUM

Field 28
Name:      `set`
Org_name:  `set`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      Set
DbType:    STRING
Collation: latin1_swedish_ci (8)
Length:    5
Decimals:  0
Flags:     SET

Field 29
Name:      `json`
Org_name:  `json`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      Json
DbType:    JSON
Collation: binary (63)
Length:    4294967295
Decimals:  0
Flags:     BLOB BINARY

Field 30
Name:      `geometry`
Org_name:  `geometry`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      Geometry
DbType:    GEOMETRY
Collation: binary (63)
Length:    4294967295
Decimals:  0
Flags:     BLOB BINARY

Field 31
Name:      `bool`
Org_name:  `bool`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      Integer
DbType:    TINY
Collation: binary (63)
Length:    1
Decimals:  0
Flags:     NUM

Field 32
Name:      `char`
Org_name:  `char`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      String
DbType:    VAR_STRING
Collation: latin1_swedish_ci (8)
Length:    10
Decimals:  0
Flags:

Field 33
Name:      `varchar`
Org_name:  `varchar`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      String
DbType:    VAR_STRING
Collation: latin1_swedish_ci (8)
Length:    20
Decimals:  0
Flags:

Field 34
Name:      `binary`
Org_name:  `binary`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      Bytes
DbType:    VAR_STRING
Collation: binary (63)
Length:    20
Decimals:  0
Flags:     BINARY

Field 35
Name:      `varbinary`
Org_name:  `varbinary`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      Bytes
DbType:    VAR_STRING
Collation: binary (63)
Length:    20
Decimals:  0
Flags:     BINARY

0

//@<OUT> Column type info enabled classic 5.7
WARNING: Using a password on the command line interface can be insecure.
Field 1
Name:      `tint`
Org_name:  `tint`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      Integer
DbType:    TINY
Collation: binary (63)
Length:    4
Decimals:  0
Flags:     NUM

Field 2
Name:      `tintu`
Org_name:  `tintu`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      UInteger
DbType:    TINY
Collation: binary (63)
Length:    3
Decimals:  0
Flags:     UNSIGNED NUM

Field 3
Name:      `smallint`
Org_name:  `smallint`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      Integer
DbType:    SHORT
Collation: binary (63)
Length:    6
Decimals:  0
Flags:     NUM

Field 4
Name:      `smallintu`
Org_name:  `smallintu`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      UInteger
DbType:    SHORT
Collation: binary (63)
Length:    5
Decimals:  0
Flags:     UNSIGNED NUM

Field 5
Name:      `mediumint`
Org_name:  `mediumint`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      Integer
DbType:    INT24
Collation: binary (63)
Length:    9
Decimals:  0
Flags:     NUM

Field 6
Name:      `mediumintu`
Org_name:  `mediumintu`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      UInteger
DbType:    INT24
Collation: binary (63)
Length:    8
Decimals:  0
Flags:     UNSIGNED NUM

Field 7
Name:      `int`
Org_name:  `int`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      Integer
DbType:    LONG
Collation: binary (63)
Length:    11
Decimals:  0
Flags:     NUM

Field 8
Name:      `intu`
Org_name:  `intu`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      UInteger
DbType:    LONG
Collation: binary (63)
Length:    10
Decimals:  0
Flags:     UNSIGNED NUM

Field 9
Name:      `bigint`
Org_name:  `bigint`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      Integer
DbType:    LONGLONG
Collation: binary (63)
Length:    20
Decimals:  0
Flags:     NOT_NULL PRI_KEY AUTO_INCREMENT NUM PART_KEY

Field 10
Name:      `bigintu`
Org_name:  `bigintu`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      UInteger
DbType:    LONGLONG
Collation: binary (63)
Length:    20
Decimals:  0
Flags:     UNSIGNED NUM

Field 11
Name:      `float`
Org_name:  `float`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      Float
DbType:    FLOAT
Collation: binary (63)
Length:    10
Decimals:  2
Flags:     NUM

Field 12
Name:      `double`
Org_name:  `double`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      Double
DbType:    DOUBLE
Collation: binary (63)
Length:    22
Decimals:  31
Flags:     NUM

Field 13
Name:      `decimal`
Org_name:  `decimal`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      Decimal
DbType:    NEWDECIMAL
Collation: binary (63)
Length:    12
Decimals:  2
Flags:     NUM

Field 14
Name:      `date`
Org_name:  `date`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      Date
DbType:    DATE
Collation: binary (63)
Length:    10
Decimals:  0
Flags:     BINARY

Field 15
Name:      `datetime`
Org_name:  `datetime`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      DateTime
DbType:    DATETIME
Collation: binary (63)
Length:    19
Decimals:  0
Flags:     BINARY

Field 16
Name:      `timestamp`
Org_name:  `timestamp`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      DateTime
DbType:    TIMESTAMP
Collation: binary (63)
Length:    19
Decimals:  0
Flags:     NOT_NULL BINARY TIMESTAMP ON_UPDATE_NOW

Field 17
Name:      `time`
Org_name:  `time`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      Time
DbType:    TIME
Collation: binary (63)
Length:    10
Decimals:  0
Flags:     BINARY

Field 18
Name:      `year`
Org_name:  `year`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      UInteger
DbType:    YEAR
Collation: binary (63)
Length:    4
Decimals:  0
Flags:     NOT_NULL UNSIGNED ZEROFILL NO_DEFAULT_VALUE NUM

Field 19
Name:      `tinyblob`
Org_name:  `tinyblob`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      Bytes
DbType:    BLOB
Collation: binary (63)
Length:    255
Decimals:  0
Flags:     BLOB BINARY

Field 20
Name:      `tinytext`
Org_name:  `tinytext`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      String
DbType:    BLOB
Collation: utf8mb4_general_ci (45)
Length:    1020
Decimals:  0
Flags:     BLOB

Field 21
Name:      `blob`
Org_name:  `blob`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      Bytes
DbType:    BLOB
Collation: binary (63)
Length:    65535
Decimals:  0
Flags:     BLOB BINARY

Field 22
Name:      `text`
Org_name:  `text`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      String
DbType:    BLOB
Collation: utf8mb4_general_ci (45)
Length:    262140
Decimals:  0
Flags:     BLOB

Field 23
Name:      `mediumblob`
Org_name:  `mediumblob`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      Bytes
DbType:    BLOB
Collation: binary (63)
Length:    16777215
Decimals:  0
Flags:     BLOB BINARY

Field 24
Name:      `mediumtext`
Org_name:  `mediumtext`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      String
DbType:    BLOB
Collation: utf8mb4_general_ci (45)
Length:    67108860
Decimals:  0
Flags:     BLOB

Field 25
Name:      `longblob`
Org_name:  `longblob`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      Bytes
DbType:    BLOB
Collation: binary (63)
Length:    4294967295
Decimals:  0
Flags:     BLOB BINARY

Field 26
Name:      `longtext`
Org_name:  `longtext`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      String
DbType:    BLOB
Collation: utf8mb4_general_ci (45)
Length:    4294967295
Decimals:  0
Flags:     BLOB

Field 27
Name:      `enum`
Org_name:  `enum`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      Enum
DbType:    STRING
Collation: utf8mb4_general_ci (45)
Length:    4
Decimals:  0
Flags:     ENUM

Field 28
Name:      `set`
Org_name:  `set`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      Set
DbType:    STRING
Collation: utf8mb4_general_ci (45)
Length:    20
Decimals:  0
Flags:     SET

Field 29
Name:      `json`
Org_name:  `json`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      Json
DbType:    JSON
Collation: binary (63)
Length:    4294967295
Decimals:  0
Flags:     BLOB BINARY

Field 30
Name:      `geometry`
Org_name:  `geometry`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      Geometry
DbType:    GEOMETRY
Collation: binary (63)
Length:    4294967295
Decimals:  0
Flags:     BLOB BINARY

Field 31
Name:      `bool`
Org_name:  `bool`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      Integer
DbType:    TINY
Collation: binary (63)
Length:    1
Decimals:  0
Flags:     NUM

Field 32
Name:      `char`
Org_name:  `char`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      String
DbType:    STRING
Collation: utf8mb4_general_ci (45)
Length:    40
Decimals:  0
Flags:

Field 33
Name:      `varchar`
Org_name:  `varchar`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      String
DbType:    VAR_STRING
Collation: utf8mb4_general_ci (45)
Length:    80
Decimals:  0
Flags:

Field 34
Name:      `binary`
Org_name:  `binary`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      Bytes
DbType:    STRING
Collation: binary (63)
Length:    20
Decimals:  0
Flags:     BINARY

Field 35
Name:      `varbinary`
Org_name:  `varbinary`
Catalog:   `def`
Database:  `scti_test`
Table:     `t`
Org_table: `t`
Type:      Bytes
DbType:    VAR_STRING
Collation: binary (63)
Length:    20
Decimals:  0
Flags:     BINARY

0
