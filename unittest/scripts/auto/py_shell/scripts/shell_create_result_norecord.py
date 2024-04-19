import datetime
import json

#@<> Invalid parameters in create result
def sample():
    pass

invalid_params = [1, "sample", True, 45.3, sample]

for param in invalid_params:
    EXPECT_THROWS(lambda: shell.create_result(param),
    "Shell.create_result: Argument #1 is expected to be either a map or an array",
    f"Using Param {param}")
    WIPE_OUTPUT()

#@<> Parsing Errors
EXPECT_THROWS(lambda:  shell.create_result({"affectedItemsCount": "whatever"}), "Shell.create_result: Error processing result affectedItemsCount: Invalid typecast: UInteger expected, but value is String")
EXPECT_THROWS(lambda:  shell.create_result({"affectedItemsCount": -100}), "Shell.create_result: Error processing result affectedItemsCount: Invalid typecast: UInteger expected, but Integer value is out of range")
EXPECT_THROWS(lambda:  shell.create_result({"executionTime": "whatever"}), "Shell.create_result: Error processing result executionTime: Invalid typecast: Float expected, but value is String")
EXPECT_THROWS(lambda:  shell.create_result({"executionTime": -100}), "Shell.create_result: Error processing result executionTime: the value can not be negative.")
EXPECT_THROWS(lambda:  shell.create_result({"info": 45}), "Shell.create_result: Error processing result info: Invalid typecast: String expected, but value is Integer")
EXPECT_THROWS(lambda:  shell.create_result({"warnings": 45}), "Shell.create_result: Error processing result warnings: Invalid typecast: Array expected, but value is Integer")
EXPECT_THROWS(lambda:  shell.create_result({"data": {}}), "Shell.create_result: Error processing result data: Invalid typecast: Array expected, but value is Map")

#@<> Parsing Errors on Warnings
EXPECT_THROWS(lambda:  shell.create_result({"warnings": [{}]}).get_warnings(), "Error processing result warning message: mandatory field, can not be empty")
EXPECT_THROWS(lambda:  shell.create_result({"warnings": [{"level":"1"}]}).get_warnings(), "Error processing result warning message: mandatory field, can not be empty")
EXPECT_THROWS(lambda:  shell.create_result({"warnings": [{"level":"1", "message":""}]}).get_warnings(), "Error processing result warning message: mandatory field, can not be empty")
EXPECT_THROWS(lambda:  shell.create_result({"warnings": [{"level":"1", "message":1}]}).get_warnings(), "Error processing result warning message: Invalid typecast: String expected, but value is Integer")
EXPECT_THROWS(lambda:  shell.create_result({"warnings": [{"level":1, "message":"My Warning"}]}).get_warnings(), "Error processing result warning level: Invalid typecast: String expected, but value is Integer")
EXPECT_THROWS(lambda:  shell.create_result({"warnings": [{"level":"1", "message":"My Warning"}]}).get_warnings(), "Error processing result warning level: Invalid value for level '1' at 'My Warning', allowed values: warning, note")
EXPECT_THROWS(lambda:  shell.create_result({"warnings": [{"level":"error", "message":"My Warning", "code":""}]}).get_warnings(), "Error processing result warning level: Invalid value for level 'error' at 'My Warning', allowed values: warning, note")
EXPECT_THROWS(lambda:  shell.create_result({"warnings": [{"level":"note", "message":"My Warning", "code":""}]}).get_warnings(), "Error processing result warning code: Invalid typecast: UInteger expected, but value is String")

#@<> Data Consistency Errors

# Data is scalar
EXPECT_THROWS(lambda:  shell.create_result({"data": 45}), "Shell.create_result: Error processing result data: Invalid typecast: Array expected, but value is Integer")

# Invalid record format, no columns provided
EXPECT_THROWS(lambda:  shell.create_result({"data": [1,2,3]}), "Shell.create_result: A record is represented as a dictionary, unexpected format: 1")

# Invalid record format, columns provided
EXPECT_THROWS(lambda:  shell.create_result({"columns":['one','two'], "data": [1,2,3]}), "Shell.create_result: A record is represented as a list of values or a dictionary, unexpected format: 1")

# Using list with no columns
EXPECT_THROWS(lambda:  shell.create_result({"data": [[1,2,3]]}), "Shell.create_result: A record can not be represented as a list of values if the columns are not defined.")

# Using mistmatched number of columns/record values
EXPECT_THROWS(lambda:  shell.create_result({"columns": ['one'], "data": [[1,2,3]]}), "Shell.create_result: The number of values in a record must match the number of columns.")

# Using both lists and dictionaries for records
EXPECT_THROWS(lambda:  shell.create_result({"columns": ['one'], "data": [[1], {"one":1}]}), "Shell.create_result: Inconsistent data in result, all the records should be either lists or dictionaries, but not mixed.")


#@<> Data Type Validation
EXPECT_THROWS(lambda:  shell.create_result({"data": [{"one":shell}]}), "Shell.create_result: Unsupported data type")

#@<> Explicit column metadata validations
EXPECT_THROWS(lambda:  shell.create_result({"columns": {}}), "Shell.create_result: Error processing result columns: Invalid typecast: Array expected, but value is Map")
EXPECT_THROWS(lambda:  shell.create_result({"columns":[1,2,3], "data": [{}]}), "Shell.create_result: Unsupported column definition format: 1")
EXPECT_THROWS(lambda:  shell.create_result({"columns":[{}], "data": [{}]}), "Shell.create_result: Missing column name at column #1")
EXPECT_THROWS(lambda:  shell.create_result({"columns":[{"name":None}], "data": [{}]}), "Shell.create_result: Error processing name for column #1: Invalid typecast: String expected, but value is Null")
EXPECT_THROWS(lambda:  shell.create_result({"columns":[{"name":'one', "type":'whatever'}], "data": [{}]}), "Shell.create_result: Error processing type for column #1: Unsupported data type.")
EXPECT_THROWS(lambda:  shell.create_result({'executionTime':-1}), "Shell.create_result: Error processing result executionTime: the value can not be negative.")

def sample():
    pass

EXPECT_THROWS(lambda: shell.create_result({'data':[{'column':sample}]}), "Shell.create_result: Unsupported data type in custom results: Function")
EXPECT_THROWS(lambda: shell.create_result({'data':[{'column':mysqlx}]}), "Shell.create_result: Unsupported data type in custom results: Object")


#@<> Verifies the allowed data types for user defined columns
allowed_types = ['string', 'integer', 'float', 'double', 'json', 'date', 'time', 'datetime', 'bytes']
for allowed_type in allowed_types:
    EXPECT_NO_THROWS(lambda:  shell.create_result({"columns":[{"name":'one', "type":allowed_type}], "data": [{}]}), f"Unexpected error using {allowed_type} data type on custom result.")

#@<> Empty result dumping...
shell.create_result()
EXPECT_STDOUT_CONTAINS('Query OK, 0 rows affected (0.0000 sec)')
WIPE_OUTPUT()

shell.create_result(None)
EXPECT_STDOUT_CONTAINS('Query OK, 0 rows affected (0.0000 sec)')
WIPE_OUTPUT()

shell.create_result([])
EXPECT_STDOUT_CONTAINS('Query OK, 0 rows affected (0.0000 sec)')
WIPE_OUTPUT()

shell.create_result({})
EXPECT_STDOUT_CONTAINS('Query OK, 0 rows affected (0.0000 sec)')
WIPE_OUTPUT()

#@<> Empty result - data verification
result = shell.create_result()
EXPECT_EQ(0, result.affected_items_count, "affectedItemsCount")
EXPECT_EQ(0, result.get_affected_items_count(), "get_affectedItemsCount()")
EXPECT_EQ("0.0000 sec", result.execution_time, "executionTime")
EXPECT_EQ("0.0000 sec", result.get_execution_time(), "get_executionTime()")
EXPECT_EQ("", result.info, "info")
EXPECT_EQ("", result.get_info(), "get_Info()")

EXPECT_EQ(0, result.warnings_count, "warnings_count")
EXPECT_EQ(0, result.get_warnings_count(), "get_warnings_count()")
EXPECT_EQ([], result.warnings, "warnings")
EXPECT_EQ([], result.get_warnings(), "get_warnings()")

EXPECT_EQ(0, result.column_count, "column_count")
EXPECT_EQ(0, result.get_column_count(), "get_column_count()")
EXPECT_EQ([], result.column_names, "column_names")
EXPECT_EQ([], result.get_column_names(), "get_column_names()")
EXPECT_EQ([], result.columns, "columns")
EXPECT_EQ([], result.get_columns(), "get_columns()")

EXPECT_EQ(False, result.has_data(), "has_data()")
EXPECT_EQ(None, result.fetch_one(), "fetch_one()")
EXPECT_EQ([], result.fetch_all(), "fetch_all()")

#@<> Column Mapping Setup
binary = b'Hi!'
date = datetime.date(2000, 1, 1)
time = datetime.time(11, 59, 59, 0)
date_time = datetime.datetime(2000, 1, 1, 11, 59, 59, 0)

# COLUMN MAPPING
# key: represents some data type
# Value Elements:
# - Position 0: The DB Data Type to be used for that value
# - Position 1: The expected value once mapped into the record
# - Position 3: Normalization function to apply to the actual value
# - Position 4: Implicit flags for the actual value
mapping = {
    'None': [mysql.Type.STRING, None, None, ""],
    'string': [mysql.Type.STRING, "sample", None, ""],
    'integer': [mysql.Type.BIGINT, 1, None, "NUM"],
    'float': [mysql.Type.DOUBLE, 1.5, None, "NUM"],
    'False': [mysql.Type.TINYINT, 0, None, "NUM"],
    'True': [mysql.Type.TINYINT, 1, None, "NUM"],
    'array': [mysql.Type.JSON, "[1, 2, 3]", None, "BINARY"],
    'dict': [mysql.Type.JSON, "{\"one\": 1}", None, "BINARY"],
    'binary': [mysql.Type.BYTES, b"Hi!", None, "BINARY"],
    'date': [mysql.Type.DATE, date, None, "BINARY"],
    'time': [mysql.Type.TIME, time, None, "BINARY"],
    'datetime': [mysql.Type.DATETIME, date_time, None, "BINARY"],
}

# Result Data To Be Used

result_data = [
    {
        "string": "sample",
        "integer": 1,
        "float": 1.5,
        "True": True,
        "False": False,
        "dict": {"one":1},
        "array": [1,2,3],
        "date": date,
        "time": time,
        "datetime": date_time,
        "binary": binary,
        "None": None
    },
    # This second record is to verify that all of the columns are Noneable
    {
        "string": None,
        "integer": None,
        "float": None,
        "True": None,
        "False": None,
        "dict": None,
        "array": None,
        "date": None,
        "time": None,
        "datetime": None,
        "binary": None,
        "None": None
    }]

result_data_list = [
        [
            "sample",
            1,
            1.5,
            True,
            False,
            date,
            time,
            date_time,
            binary,
            [1,2,3],
            {"one":1},
            None
        ],
        # This second record is to verify that all of the columns are nullable
        [
            None,
            None,
            None,
            None,
            None,
            None,
            None,
            None,
            None,
            None,
            None,
            None
        ]]

# Auto-mapping Result
result = shell.create_result({"data":result_data})

#@<> Auto-mapping - implicit column order
# Order is defined lexicographycally
names = result.get_column_names()
expected = list(mapping.keys())
expected.sort()
EXPECT_EQ(expected, names, "Unexpected order of columns")

#@<> Auto-mapping - implicit column order DB Types and Flags
columns = result.get_columns()
index = 0
for name in names:
    EXPECT_EQ(mapping[name][0].data, columns[index].get_type().data, f"Unexpected type for column {name}")
    EXPECT_EQ(mapping[name][3], columns[index].get_flags(), f"Unexpected flags for column {name}")
    index = index + 1

#@<> Auto-mapping - implicit column order Data Mapping
row = result.fetch_one()
index = 0
for name in names:
    value = row[index]
    if mapping[name][2] != None:
        value = mapping[name][2](value)
    EXPECT_EQ(mapping[name][1], value, f"Unexpected value for column {name}")
    index = index + 1

#@<> Auto-mapping - implicit column order result dump
result = shell.create_result({"data":result_data})
result
EXPECT_STDOUT_CONTAINS_MULTILINE("""
+-------+------+------+-----------+----------+------------+---------------------+------------+-------+---------+--------+----------+
| False | None | True | array     | binary   | date       | datetime            | dict       | float | integer | string | time     |
+-------+------+------+-----------+----------+------------+---------------------+------------+-------+---------+--------+----------+
|     0 | NULL |    1 | [1, 2, 3] | 0x486921 | 2000-01-01 | 2000-01-01 11:59:59 | {"one": 1} |   1.5 |       1 | sample | 11:59:59 |
|  NULL | NULL | NULL | NULL      | NULL     | NULL       | NULL                | NULL       |  NULL |    NULL | NULL   | NULL     |
+-------+------+------+-----------+----------+------------+---------------------+------------+-------+---------+--------+----------+
2 rows in set (0.0000 sec)
""")

#@<> Auto-mapping - explicit column order (string columns)
column_names = ['string', 'integer', 'float', 'True', 'False', 'date', 'time', 'datetime', 'binary', 'array', 'dict', 'None']

result_2 = shell.create_result({"columns":column_names, "data":result_data})
# Order is defined lexicographycally
names_2 = result_2.get_column_names()
EXPECT_EQ(column_names, names_2, "Unexpected order of columns")


#@<> Auto-mapping - explicit column order DB Types and Flags (string columns)
columns_2 = result_2.get_columns()
index = 0
for name in names_2:
    EXPECT_EQ(mapping[name][0].data, columns_2[index].get_type().data, f"Unexpected type for column {name}")
    EXPECT_EQ(mapping[name][3], columns_2[index].get_flags(), f"Unexpected flags for column {name}")
    index = index + 1

#@<> Auto-mapping - explicit column order Data Mapping (string columns)
row = result_2.fetch_one()
index = 0
for name in names_2:
    value = row[index]
    if mapping[name][2] != None:
        value = mapping[name][2](value)
    EXPECT_EQ(mapping[name][1], value, f"Unexpected value for column {name}")
    index = index + 1

#@<> Auto-mapping - explicit column order result dump (string columns)
result_2 = shell.create_result({"columns":column_names, "data":result_data})
result_2
EXPECT_STDOUT_CONTAINS_MULTILINE("""
+--------+---------+-------+------+-------+------------+----------+---------------------+----------+-----------+------------+------+
| string | integer | float | True | False | date       | time     | datetime            | binary   | array     | dict       | None |
+--------+---------+-------+------+-------+------------+----------+---------------------+----------+-----------+------------+------+
| sample |       1 |   1.5 |    1 |     0 | 2000-01-01 | 11:59:59 | 2000-01-01 11:59:59 | 0x486921 | [1, 2, 3] | {"one": 1} | NULL |
| NULL   |    NULL |  NULL | NULL |  NULL | NULL       | NULL     | NULL                | NULL     | NULL      | NULL       | NULL |
+--------+---------+-------+------+-------+------------+----------+---------------------+----------+-----------+------------+------+
2 rows in set (0.0000 sec)
""")

#@<> Auto-mapping - explicit column order (string columns, data in lists)
result_3 = shell.create_result({"columns":column_names, "data":result_data_list})
# Order is defined lexicographycally
names_3 = result_3.get_column_names()
EXPECT_EQ(column_names, names_3, "Unexpected order of columns")


#@<> Auto-mapping - explicit column order DB Types and Flags (string columns, data in lists)
columns_3 = result_3.get_columns()
index = 0
for name in names_3:
    EXPECT_EQ(mapping[name][0].data, columns_3[index].get_type().data, f"Unexpected type for column {name}")
    EXPECT_EQ(mapping[name][3], columns_3[index].get_flags(), f"Unexpected flags for column {name}")
    index = index + 1

#@<> Auto-mapping - explicit column order Data Mapping (string columns, data in lists)
row = result_3.fetch_one()
index = 0
for name in names_3:
    value = row[index]
    if mapping[name][2] != None:
        value = mapping[name][2](value)
    EXPECT_EQ(mapping[name][1], value, f"Unexpected value for column {name}")
    index = index + 1

#@<> Auto-mapping - explicit column order result dump (string columns, data in lists)
result_3 = shell.create_result({"columns":column_names, "data":result_data_list})
result_3
EXPECT_STDOUT_CONTAINS_MULTILINE("""
+--------+---------+-------+------+-------+------------+----------+---------------------+----------+-----------+------------+------+
| string | integer | float | True | False | date       | time     | datetime            | binary   | array     | dict       | None |
+--------+---------+-------+------+-------+------------+----------+---------------------+----------+-----------+------------+------+
| sample |       1 |   1.5 |    1 |     0 | 2000-01-01 | 11:59:59 | 2000-01-01 11:59:59 | 0x486921 | [1, 2, 3] | {"one": 1} | NULL |
| NULL   |    NULL |  NULL | NULL |  NULL | NULL       | NULL     | NULL                | NULL     | NULL      | NULL       | NULL |
+--------+---------+-------+------+-------+------------+----------+---------------------+----------+-----------+------------+------+
2 rows in set (0.0000 sec)
""")

# Auto-mapping Result - explicit column order (dictionary columns)
column_name_dicts = [{"name":'string'}, {"name":'integer'}, {"name":'float'}, {"name":'True'}, {"name":'False'}, {"name":'date'}, {"name":'time'}, {"name":'datetime'}, {"name":'binary'}, {"name":'array'}, {"name":'dict'}, {"name":'None'}]
result_4 = shell.create_result({"columns":column_name_dicts, "data":result_data})

#@<> Auto-mapping - explicit column order (dictionary columns)
# Order is defined lexicographycally
names_4 = result_4.get_column_names()
EXPECT_EQ(column_names, names_4, "Unexpected order of columns")


#@<> Auto-mapping - explicit column order DB Types and Flags (dictionary columns)
columns_4 = result_4.get_columns()
index = 0
for name in names_4:
    EXPECT_EQ(mapping[name][0].data, columns_4[index].get_type().data, f"Unexpected type for column {name}")
    EXPECT_EQ(mapping[name][3], columns_4[index].get_flags(), f"Unexpected flags for column {name}")
    index = index + 1

#@<> Auto-mapping - explicit column order Data Mapping (dictionary columns)
row = result_4.fetch_one()
index = 0
for name in names_4:
    value = row[index]
    if mapping[name][2] != None:
        value = mapping[name][2](value)
    EXPECT_EQ(mapping[name][1], value, f"Unexpected value for column {name}")
    index = index + 1

#@<> Auto-mapping - explicit column order result dump (dictionary columns)
result_4 = shell.create_result({"columns":column_name_dicts, "data":result_data})
result_4
EXPECT_STDOUT_CONTAINS_MULTILINE("""
+--------+---------+-------+------+-------+------------+----------+---------------------+----------+-----------+------------+------+
| string | integer | float | True | False | date       | time     | datetime            | binary   | array     | dict       | None |
+--------+---------+-------+------+-------+------------+----------+---------------------+----------+-----------+------------+------+
| sample |       1 |   1.5 |    1 |     0 | 2000-01-01 | 11:59:59 | 2000-01-01 11:59:59 | 0x486921 | [1, 2, 3] | {"one": 1} | NULL |
| NULL   |    NULL |  NULL | NULL |  NULL | NULL       | NULL     | NULL                | NULL     | NULL      | NULL       | NULL |
+--------+---------+-------+------+-------+------------+----------+---------------------+----------+-----------+------------+------+
2 rows in set (0.0000 sec)
""")


#@<> Auto-mapping - explicit column order (dictionary columns, data in lists)
column_name_dicts = [{"name":'string'}, {"name":'integer'}, {"name":'float'}, {"name":'True'}, {"name":'False'}, {"name":'date'}, {"name":'time'}, {"name":'datetime'}, {"name":'binary'}, {"name":'array'}, {"name":'dict'}, {"name":'None'}]
result_5 = shell.create_result({"columns":column_name_dicts, "data":result_data_list})

names_5 = result_5.get_column_names()
EXPECT_EQ(column_names, names_5, "Unexpected order of columns")


#@<> Auto-mapping - explicit column order DB Types and Flags (dictionary columns, data in lists)
columns_5 = result_5.get_columns()
index = 0
for name in names_5:
    EXPECT_EQ(mapping[name][0].data, columns_5[index].get_type().data, f"Unexpected type for column {name}")
    EXPECT_EQ(mapping[name][3], columns_5[index].get_flags(), f"Unexpected flags for column {name}")
    index = index + 1

#@<> Auto-mapping - explicit column order Data Mapping (dictionary columns, data in lists)
row = result_5.fetch_one()
index = 0
for name in names_5:
    value = row[index]
    if mapping[name][2] != None:
        value = mapping[name][2](value)
    EXPECT_EQ(mapping[name][1], value, f"Unexpected value for column {name}")
    index = index + 1

#@<> Auto-mapping - explicit column order result dump (dictionary columns, data in lists)
result_5 = shell.create_result({"columns":column_name_dicts, "data":result_data_list})
result_5
EXPECT_STDOUT_CONTAINS_MULTILINE("""
+--------+---------+-------+------+-------+------------+----------+---------------------+----------+-----------+------------+------+
| string | integer | float | True | False | date       | time     | datetime            | binary   | array     | dict       | None |
+--------+---------+-------+------+-------+------------+----------+---------------------+----------+-----------+------------+------+
| sample |       1 |   1.5 |    1 |     0 | 2000-01-01 | 11:59:59 | 2000-01-01 11:59:59 | 0x486921 | [1, 2, 3] | {"one": 1} | NULL |
| NULL   |    NULL |  NULL | NULL |  NULL | NULL       | NULL     | NULL                | NULL     | NULL      | NULL       | NULL |
+--------+---------+-------+------+-------+------------+----------+---------------------+----------+-----------+------------+------+
2 rows in set (0.0000 sec)
""")


#@<> Explicit Type mapping - explicit column order
user_columns_full = [
    {"name":'string', "type":'string'},
    {"name":'integer', "type":'string'}, # overrides default
    {"name":'float', "type":'float'},
    {"name":'True', "type":'string'},   # overrides default
    {"name":'False', "type": 'string'}, # overrides default
    {"name":'date', "type":'date'},
    {"name":'time', "type":'time'},
    {"name":'datetime', "type":'datetime'},
    {"name":'binary', "type":'bytes'},
    {"name":'array', "type":'string'},  # overrides default
    {"name":'dict', "type":'string'},   # overrides default
    {"name":'None', "type":'integer'}   # overrides default
]

result_6 = shell.create_result({"columns":user_columns_full, "data":result_data})

names_6 = result_6.get_column_names()
EXPECT_EQ(column_names, names_6, "Unexpected order of columns")


#@<> Explicit Type mapping - explicit column order DB Types
# Override the types explicitly defined different to the default ones (and the flags)
mapping['date'][0] = mysql.Type.DATE
mapping['array'][0] = mysql.Type.STRING
mapping['array'][3] = ""
mapping['dict'][0] = mysql.Type.STRING
mapping['dict'][3] = ""
mapping['None'][0] = mysql.Type.BIGINT
mapping['None'][3] = "NUM"
mapping['integer'][0] = mysql.Type.STRING
mapping['integer'][3] = ""
mapping['True'][0] = mysql.Type.STRING
mapping['True'][3] = ""
mapping['False'][0] = mysql.Type.STRING
mapping['False'][3] = ""

# Override the expected values that change
mapping['integer'][1] = "1"
mapping['True'][1] = "true"
mapping['False'][1] = "false"

columns_6 = result_6.get_columns()
index = 0
for name in names_6:
    EXPECT_EQ(mapping[name][0].data, columns_6[index].get_type().data, f"Unexpected type for column {name}")
    EXPECT_EQ(mapping[name][3], columns_6[index].get_flags(), f"Unexpected flags for column {name}")
    index = index + 1

#@<> Explicit Type mapping - explicit column order Data Mapping
row = result_6.fetch_one()
index = 0
for name in names_6:
    value = row[index]
    if mapping[name][2] != None:
        value = mapping[name][2](value)
    EXPECT_EQ(mapping[name][1], value, "Unexpected value for column {name}")
    index = index + 1

#@<> Explicit Type mapping - explicit column order result dump (dictionary columns)
result_6 = shell.create_result({"columns":user_columns_full, "data":result_data})
result_6
EXPECT_STDOUT_CONTAINS_MULTILINE("""
+--------+---------+-------+------+-------+------------+----------+---------------------+----------+-----------+------------+------+
| string | integer | float | True | False | date       | time     | datetime            | binary   | array     | dict       | None |
+--------+---------+-------+------+-------+------------+----------+---------------------+----------+-----------+------------+------+
| sample | 1       |   1.5 | true | false | 2000-01-01 | 11:59:59 | 2000-01-01 11:59:59 | 0x486921 | [1, 2, 3] | {"one": 1} | NULL |
| NULL   | NULL    |  NULL | NULL | NULL  | NULL       | NULL     | NULL                | NULL     | NULL      | NULL       | NULL |
+--------+---------+-------+------+-------+------------+----------+---------------------+----------+-----------+------------+------+
2 rows in set (0.0000 sec)
""")


#@<> Explicit Type mapping - explicit column order (data in lists)
result_7 = shell.create_result({"columns":user_columns_full, "data":result_data_list})

names_7 = result_7.get_column_names()
EXPECT_EQ(column_names, names_7, "Unexpected order of columns")

#@<> Explicit Type mapping - explicit column order DB Types and Flags (data in lists)
columns_7 = result_7.get_columns()
index = 0
for name in names_7:
    EXPECT_EQ(mapping[name][0].data, columns_7[index].get_type().data, f"Unexpected type for column {name}")
    EXPECT_EQ(mapping[name][3], columns_7[index].get_flags(), f"Unexpected flags for column {name}")
    index = index + 1

#@<> Explicit Type mapping - explicit column order Data Mapping (data in lists)
row = result_7.fetch_one()
index = 0
for name in names_7:
    value = row[index]
    if mapping[name][2] != None:
        value = mapping[name][2](value)
    EXPECT_EQ(mapping[name][1], value, "Unexpected value for column {name}")
    index = index + 1

#@<> Explicit Type mapping - explicit column order result dump (data in lists)
result_7 = shell.create_result({"columns":user_columns_full, "data":result_data_list})
result_7
EXPECT_STDOUT_CONTAINS_MULTILINE("""
+--------+---------+-------+------+-------+------------+----------+---------------------+----------+-----------+------------+------+
| string | integer | float | True | False | date       | time     | datetime            | binary   | array     | dict       | None |
+--------+---------+-------+------+-------+------------+----------+---------------------+----------+-----------+------------+------+
| sample | 1       |   1.5 | true | false | 2000-01-01 | 11:59:59 | 2000-01-01 11:59:59 | 0x486921 | [1, 2, 3] | {"one": 1} | NULL |
| NULL   | NULL    |  NULL | NULL | NULL  | NULL       | NULL     | NULL                | NULL     | NULL      | NULL       | NULL |
+--------+---------+-------+------+-------+------------+----------+---------------------+----------+-----------+------------+------+
2 rows in set (0.0000 sec)
""")

#@<> Full Single Result
def create_single_result():
    return shell.create_result({
        "affectedItemsCount": 1,
        "data": [{"one":1, "two":2}, {"one":3, "two":4}],
        "executionTime": 1.5,
        "warnings": [
            {
                "level": "warning",
                "message": "sample warning",
                "code": 1
            },
            {
                "level": "note",
                "message": "sample note warning",
                "code": 2
            }
        ],
        "info": "Successful sample result!"})


result = create_single_result()
EXPECT_EQ(1, result.affected_items_count, "affectedItemsCount")
EXPECT_EQ(1, result.get_affected_items_count(), "get_affectedItemsCount()")
EXPECT_EQ("1.5000 sec", result.execution_time, "executionTime")
EXPECT_EQ("1.5000 sec", result.get_execution_time(), "get_executionTime()")
EXPECT_EQ("Successful sample result!", result.info, "info")
EXPECT_EQ("Successful sample result!", result.get_info(), "get_Info()")

EXPECT_EQ(2, result.warnings_count, "warnings_count")
EXPECT_EQ(2, result.get_warnings_count(), "get_warnings_count()")
EXPECT_EQ(str([["Warning", 1, "sample warning"], ["Note", 2, "sample note warning"]]), str(result.warnings).replace('"', "'"), "warnings")

# TODO(rennox): There's a problem with the way the warnings are fetched
# in all the result classes. i.e. once they are fetched once, they are gone
# This is more critical i.e. if autocompletion is called in JS mode, as it
# will cause the warnings to be consumed even without user having requested it.
EXPECT_EQ([], result.warnings, "warnings-2")
EXPECT_EQ([], result.get_warnings(), "get_warnings()")

EXPECT_EQ(2, result.column_count, "column_count")
EXPECT_EQ(2, result.get_column_count(), "get_column_count()")
EXPECT_EQ(['one', 'two'], result.column_names, "column_names")
EXPECT_EQ(['one', 'two'], result.get_column_names(), "get_column_names()")
EXPECT_EQ('one', result.columns[0].column_label, "columns")
EXPECT_EQ('two', result.columns[1].column_label, "columns")

EXPECT_EQ(True, result.has_data(), "has_data()")
EXPECT_EQ([1, 2], list(result.fetch_one()), "fetch_one()")
EXPECT_EQ([[3, 4]], [list(elem) for elem in list(result.fetch_all())], "fetch_all()")

result = create_single_result()
EXPECT_EQ([[1, 2], [3, 4]], [list(elem) for elem in list(result.fetch_all())], "fetch_all()")

#@<> Dumping result with data
result = create_single_result()
result
EXPECT_STDOUT_CONTAINS_MULTILINE("""
+-----+-----+
| one | two |
+-----+-----+
|   1 |   2 |
|   3 |   4 |
+-----+-----+
2 rows in set, 2 warnings (1.5000 sec)

Successful sample result!
Warning (code 1): sample warning
Note (code 2): sample note warning
""")
WIPE_OUTPUT()

#@<> Multi-Result-Handling
result = shell.create_result([
        {
            "data": [{"one":1, "two":2}, {"one":3, "two":4}],
            "executionTime": 1.5,
            "info": "First Result!"
        },
        {
            "info": "Second Result"
        },
        {
            "data": [{"fname":"John", "lname":"Doe"}, {"fname":"Jane", "lname":"Doe"}],
            "executionTime": 0.34,
            "info": "Last Result!"
        },
    ])

# Digging into the first result
EXPECT_EQ(True, result.has_data())
EXPECT_EQ("1.5000 sec", result.execution_time)
EXPECT_EQ("First Result!", result.info)
EXPECT_EQ(['one', 'two'], result.column_names)
EXPECT_EQ([[1,2], [3,4]], [list(elem) for elem in list(result.fetch_all())])

# Switching to the second result
EXPECT_EQ(True, result.next_result())
EXPECT_EQ(False, result.has_data())
EXPECT_EQ("0.0000 sec", result.execution_time)
EXPECT_EQ("Second Result", result.info)
EXPECT_EQ([], result.column_names)
EXPECT_EQ([], result.fetch_all())

# Switching to the third result
EXPECT_EQ(True, result.next_result())
EXPECT_EQ(True, result.has_data())
EXPECT_EQ("0.3400 sec", result.execution_time)
EXPECT_EQ("Last Result!", result.info)
EXPECT_EQ(['fname', 'lname'], result.column_names)
EXPECT_EQ([['John', 'Doe'],['Jane', 'Doe']], [list(elem) for elem in list(result.fetch_all())])

# No more results!
EXPECT_EQ(False, result.next_result())

#@<> Dumping Multi-Result Object
WIPE_OUTPUT()
shell.create_result([
    {
        "data": [{"one":1, "two":2}, {"one":3, "two":4}],
        "executionTime": 1.5,
        "info": "First Result!"
    },
    {
        "info": "Second Result"
    },
    {
        "data": [{"fname":"John", "lname":"Doe"}, {"fname":"Jane", "lname":"Doe"}],
        "executionTime": 0.34,
        "info": "Last Result!"
    },
])

EXPECT_STDOUT_CONTAINS_MULTILINE("""
+-----+-----+
| one | two |
+-----+-----+
|   1 |   2 |
|   3 |   4 |
+-----+-----+
2 rows in set (1.5000 sec)

First Result!

Query OK, 0 rows affected (0.0000 sec)

Second Result

+-------+-------+
| fname | lname |
+-------+-------+
| John  | Doe   |
| Jane  | Doe   |
+-------+-------+
2 rows in set (0.3400 sec)

Last Result!
""")
WIPE_OUTPUT()


#@<> Navigating a Multi-Result Object that includes an error
WIPE_OUTPUT()
result = shell.create_result([
    {
        "data": [{"one":1, "two":2}, {"one":3, "two":4}],
        "executionTime": 1.5,
        "info": "First Result!"
    },
    {
        "info": "Second Result"
    },
    {
        "error": "An error occurred in a multiresult operation"
    },
])

EXPECT_EQ(True, result.has_data())
EXPECT_EQ("1.5000 sec", result.execution_time)
EXPECT_EQ("First Result!", result.info)
EXPECT_EQ(['one', 'two'], result.column_names)
EXPECT_EQ([[1,2], [3,4]], [list(elem) for elem in list(result.fetch_all())])

# Switching to the second result
EXPECT_EQ(True, result.next_result())
EXPECT_EQ(False, result.has_data())
EXPECT_EQ("0.0000 sec", result.execution_time)
EXPECT_EQ("Second Result", result.info)
EXPECT_EQ([], result.column_names)
EXPECT_EQ([], result.fetch_all())

# Switching to the error result
EXPECT_THROWS(lambda:  result.next_result(), "An error occurred in a multiresult operation")

#@<> Data type validation
result = shell.create_result({"data": [[1234, 4567], [89, 0.1]], "columns": ["a", "b"]})
EXPECT_NO_THROWS(lambda: result.fetch_one())
EXPECT_THROWS(lambda: result.fetch_one(), "ShellResult.fetch_one: Invalid typecast: Integer expected, but Float value is out of range, at row 1, column 'b'")

