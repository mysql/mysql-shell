//@<> Invalid parameters in create result
function sample(){}

let invalid_params = [1, "sample", true, 45.3, sample];

for (param of invalid_params) {
    EXPECT_THROWS(function() {shell.createResult(param); },
    "Argument #1 is expected to be either a map or an array",
    `Using Param ${param}`);
    WIPE_OUTPUT();
}

//@<> Parsing Errors
EXPECT_THROWS(function() { shell.createResult({affectedItemsCount: -100}); }, "Error processing result affectedItemsCount: Invalid typecast: UInteger expected, but Integer value is out of range");
EXPECT_THROWS(function() { shell.createResult({affectedItemsCount: "whatever"}); }, "Error processing result affectedItemsCount: Invalid typecast: UInteger expected, but value is String");
EXPECT_THROWS(function() { shell.createResult({executionTime: "whatever"}); }, "Error processing result executionTime: Invalid typecast: Float expected, but value is String");
EXPECT_THROWS(function() { shell.createResult({executionTime: -100}); }, "Error processing result executionTime: the value can not be negative.");
EXPECT_THROWS(function() { shell.createResult({info: 45}); }, "Error processing result info: Invalid typecast: String expected, but value is Integer");
EXPECT_THROWS(function() { shell.createResult({warnings: 45}); }, "Error processing result warnings: Invalid typecast: Array expected, but value is Integer");
EXPECT_THROWS(function() { shell.createResult({data: {}}); }, "Error processing result data: Invalid typecast: Array expected, but value is Map");

//@<> Parsing Errors on Warnings
EXPECT_THROWS(function() { shell.createResult({warnings: [{}]}).getWarnings(); }, "Error processing result warning message: mandatory field, can not be empty");
EXPECT_THROWS(function() { shell.createResult({warnings: [{level:"1"}]}).getWarnings(); }, "Error processing result warning message: mandatory field, can not be empty");
EXPECT_THROWS(function() { shell.createResult({warnings: [{level:"1", message:""}]}).getWarnings(); }, "Error processing result warning message: mandatory field, can not be empty");
EXPECT_THROWS(function() { shell.createResult({warnings: [{level:"1", message:1}]}).getWarnings(); }, "Error processing result warning message: Invalid typecast: String expected, but value is Integer");
EXPECT_THROWS(function() { shell.createResult({warnings: [{level:1, message:"My Warning"}]}).getWarnings(); }, "Error processing result warning level: Invalid typecast: String expected, but value is Integer");
EXPECT_THROWS(function() { shell.createResult({warnings: [{level:"1", message:"My Warning"}]}).getWarnings(); }, "Error processing result warning level: Invalid value for level '1' at 'My Warning', allowed values: warning, note");
EXPECT_THROWS(function() { shell.createResult({warnings: [{level:"error", message:"My Warning", code:""}]}).getWarnings(); }, "Error processing result warning level: Invalid value for level 'error' at 'My Warning', allowed values: warning, note");
EXPECT_THROWS(function() { shell.createResult({warnings: [{level:"note", message:"My Warning", code:""}]}).getWarnings(); }, "Error processing result warning code: Invalid typecast: UInteger expected, but value is String");

//@<> Data Consistency Errors
// Data is scalar
EXPECT_THROWS(function() { shell.createResult({data: 45}); }, "Error processing result data: Invalid typecast: Array expected, but value is Integer");

// Invalid record format, no columns provided
EXPECT_THROWS(function() { shell.createResult({data: [1,2,3]}); }, "A record is represented as a dictionary, unexpected format: 1");

// Invalid record format, columns provided
EXPECT_THROWS(function() { shell.createResult({columns:['one','two'], data: [1,2,3]}); }, "A record is represented as a list of values or a dictionary, unexpected format: 1");

// Using list with no columns
EXPECT_THROWS(function() { shell.createResult({data: [[1,2,3]]}); }, "A record can not be represented as a list of values if the columns are not defined.");

// Using mistmatched number of columns/record values
EXPECT_THROWS(function() { shell.createResult({columns: ['one'], data: [[1,2,3]]}); }, "The number of values in a record must match the number of columns.");

// Using both lists and dictionaries for records
EXPECT_THROWS(function() { shell.createResult({columns: ['one'], data: [[1], {one:1}]}); }, "Inconsistent data in result, all the records should be either lists or dictionaries, but not mixed.");


//@<> Data Type Validation
EXPECT_THROWS(function() { shell.createResult({data: [{one:shell}]}); }, "Unsupported data type");

//@<> Explicit column metadata validations
EXPECT_THROWS(function() { shell.createResult({columns: {}}); }, "Error processing result columns: Invalid typecast: Array expected, but value is Map");
EXPECT_THROWS(function() { shell.createResult({columns:[1,2,3], data: [{}]}); }, "Unsupported column definition format: 1");
EXPECT_THROWS(function() { shell.createResult({columns:[{}], data: [{}]}); }, "Missing column name at column #1");
EXPECT_THROWS(function() { shell.createResult({columns:[{name:null}], data: [{}]}); }, "Error processing name for column #1: Invalid typecast: String expected, but value is Null");
EXPECT_THROWS(function() { shell.createResult({columns:[{name:'one', type:'whatever'}], data: [{}]}); }, "Error processing type for column #1: Unsupported data type.");
EXPECT_THROWS(function() {  shell.createResult({executionTime:-1}) }, "Error processing result executionTime: the value can not be negative.");

function sample(){}

EXPECT_THROWS(function() { shell.createResult({data:[{column:sample}]}) }, "Unsupported data type in custom results: Function");
EXPECT_THROWS(function() { shell.createResult({data:[{column:mysqlx}]}) }, "Unsupported data type in custom results: Object");


//@<> Verifies the allowed data types for user defined columns
var allowed_types = ['string', 'integer', 'float', 'double', 'json', 'date', 'time', 'datetime', 'bytes']
for(type of allowed_types) {
    EXPECT_NO_THROWS(function() { shell.createResult({columns:[{name:'one', type:type}], data: [{}]}); }, `Unexpected error using ${type} data type on custom result.`);
}

//@<> Empty result dumping...
shell.createResult()
EXPECT_OUTPUT_CONTAINS('Query OK, 0 rows affected (0.0000 sec)');
WIPE_OUTPUT()

shell.createResult(null)
EXPECT_OUTPUT_CONTAINS('Query OK, 0 rows affected (0.0000 sec)');
WIPE_OUTPUT()

shell.createResult([])
EXPECT_OUTPUT_CONTAINS('Query OK, 0 rows affected (0.0000 sec)');
WIPE_OUTPUT()

shell.createResult({})
EXPECT_OUTPUT_CONTAINS('Query OK, 0 rows affected (0.0000 sec)');
WIPE_OUTPUT()

//@<> Empty result - data verification
var result = shell.createResult()
EXPECT_EQ(0, result.affectedItemsCount, "affectedItemsCount")
EXPECT_EQ(0, result.getAffectedItemsCount(), "getAffectedItemsCount()")
EXPECT_EQ("0.0000 sec", result.executionTime, "executionTime")
EXPECT_EQ("0.0000 sec", result.getExecutionTime(), "getExecutionTime()")
EXPECT_EQ("", result.info, "info")
EXPECT_EQ("", result.getInfo(), "getInfo()")

EXPECT_EQ(0, result.warningsCount, "warningsCount")
EXPECT_EQ(0, result.getWarningsCount(), "getWarningsCount()")
EXPECT_EQ([], result.warnings, "warnings")
EXPECT_EQ([], result.getWarnings(), "getWarnings()")

EXPECT_EQ(0, result.columnCount, "columnCount")
EXPECT_EQ(0, result.getColumnCount(), "getColumnCount()")
EXPECT_EQ([], result.columnNames, "columnNames")
EXPECT_EQ([], result.getColumnNames(), "getColumnNames()")
EXPECT_EQ([], result.columns, "columns")
EXPECT_EQ([], result.getColumns(), "getColumns()")

EXPECT_EQ(false, result.hasData(), "hasData()")
EXPECT_EQ(null, result.fetchOne(), "fetchOne()")
EXPECT_EQ([], result.fetchAll(), "fetchAll()")

//@<> Column Mapping Setup
function str2ab(str) {
    var buf = new ArrayBuffer(str.length * 2);
    var bufView = new Uint16Array(buf);
    for (var i = 0; i < str.length; i++) {
      bufView[i] = str.charCodeAt(i);
    }
    return buf;
}

function ab2str(buf) {
    return String.fromCharCode.apply(null, new Uint16Array(buf));
}

var binary = str2ab('Hi!');
var date = new Date(2000, 0, 1)
var date_time = new Date(2000, 0, 1, 11, 59, 59)

// COLUMN MAPPING
// key: represents some data type
// Value Elements:
// - Position 0: The DB Data Type to be used for that value
// - Position 1: The expected value once mapped into the record
// - Position 3: Normalization function to apply to the actual value
// - Position 4: Implicit flags for the actual value
var mapping = {
    null: [mysql.Type.STRING, null, null, ""],
    string: [mysql.Type.STRING, "sample", null, ""],
    integer: [mysql.Type.BIGINT, 1, null, "NUM"],
    float: [mysql.Type.DOUBLE, 1.5, null, "NUM"],
    false: [mysql.Type.TINYINT, 0, null, "NUM"],
    true: [mysql.Type.TINYINT, 1, null, "NUM"],
    array: [mysql.Type.JSON, "[1, 2, 3]", null, "BINARY"],
    dict: [mysql.Type.JSON, "{\"one\": 1}", null, "BINARY"],
    binary: [mysql.Type.BYTES, "Hi!", function(value){ return ab2str(value)}, "BINARY"],
    date: [mysql.Type.DATETIME, date.toString(), function(value){ return value.toString()}, "BINARY"],
    datetime: [mysql.Type.DATETIME, date_time.toString(), function(value){ return value.toString()}, "BINARY"],
}

// Result Data To Be Used

var result_data = [
    {
        string: "sample",
        integer: 1,
        float: 1.5,
        true: true,
        false: false,
        dict: {one:1},
        array: [1,2,3],
        date: date,
        datetime: date_time,
        binary: binary,
        null: null
    },
    // This second record is to verify that all of the columns are nullable
    {
        string: null,
        integer: null,
        float: null,
        true: null,
        false: null,
        dict: null,
        array: null,
        date: null,
        datetime: null,
        binary: null,
        null: null
    }]

    var result_data_list = [
        [
            "sample",
            1,
            1.5,
            true,
            false,
            {one:1},
            [1,2,3],
            date,
            date_time,
            binary,
            null
        ],
        // This second record is to verify that all of the columns are nullable
        [
            null,
            null,
            null,
            null,
            null,
            null,
            null,
            null,
            null,
            null,
            null
        ]]

// Auto-mapping Result
var result_1 = shell.createResult({data:result_data})

//@<> Auto-mapping - implicit column order
// Order is defined lexicographycally
var names = result_1.getColumnNames()
EXPECT_EQ(dir(mapping).sort(), names, "Unexpected order of columns")

//@<> Auto-mapping - implicit column order DB Types and Flags
var columns = result_1.getColumns()
var index = 0;
for(name of names) {
    EXPECT_EQ(mapping[name][0].data, columns[index].getType().data, `Unexpected type for column ${name}`)
    EXPECT_EQ(mapping[name][3], columns[index].getFlags(), `Unexpected flags for column ${name}`)
    index = index + 1;
}

//@<> Auto-mapping - implicit column order Data Mapping
var row = result_1.fetchOne();
var index = 0;
for(name of names) {
    value = row[index];
    if (mapping[name][2] != null) {
        value = mapping[name][2](value);
    }
    EXPECT_EQ(mapping[name][1], value, `Unexpected value for column ${name}`)
    index = index + 1;
}

//@<> Auto-mapping - implicit column order result dump
shell.createResult({data:result_data})
EXPECT_OUTPUT_CONTAINS_MULTILINE(`
+-----------+----------------+---------------------+---------------------+------------+-------+-------+---------+------+--------+------+
| array     | binary         | date                | datetime            | dict       | false | float | integer | null | string | true |
+-----------+----------------+---------------------+---------------------+------------+-------+-------+---------+------+--------+------+
| [1, 2, 3] | 0x480069002100 | 2000-01-01 00:00:00 | 2000-01-01 11:59:59 | {"one": 1} |     0 |   1.5 |       1 | NULL | sample |    1 |
| NULL      | NULL           | NULL                | NULL                | NULL       |  NULL |  NULL |    NULL | NULL | NULL   | NULL |
+-----------+----------------+---------------------+---------------------+------------+-------+-------+---------+------+--------+------+
2 rows in set (0.0000 sec)
`)

// Auto-mapping Result - explicit column order (string columns)
var column_names = ['string', 'integer', 'float', 'true', 'false', 'date', 'datetime', 'binary', 'array', 'dict', 'null']
var result_2 = shell.createResult({columns:column_names, data:result_data})

//@<> Auto-mapping - explicit column order (string columns)
// Order is defined lexicographycally
var names_2 = result_2.getColumnNames()
EXPECT_EQ(column_names, names_2, "Unexpected order of columns")


//@<> Auto-mapping - explicit column order DB Types and Flags(string columns)
var columns_2 = result_2.getColumns()
var index = 0;
for(name of names_2) {
    EXPECT_EQ(mapping[name][0].data, columns_2[index].getType().data, `Unexpected type for column ${name}`)
    EXPECT_EQ(mapping[name][3], columns_2[index].getFlags(), `Unexpected flags for column ${name}`)
    index = index + 1;
}

//@<> Auto-mapping - explicit column order Data Mapping (string columns)
var row = result_2.fetchOne();
var index = 0;
for(name of names_2) {
    value = row[index];
    if (mapping[name][2] != null) {
        value = mapping[name][2](value);
    }
    EXPECT_EQ(mapping[name][1], value, `Unexpected value for column ${name}`)
    index = index + 1;
}

//@<> Auto-mapping - explicit column order result dump (string columns)
shell.createResult({columns:column_names, data:result_data})
EXPECT_OUTPUT_CONTAINS_MULTILINE(`
+--------+---------+-------+------+-------+---------------------+---------------------+----------------+-----------+------------+------+
| string | integer | float | true | false | date                | datetime            | binary         | array     | dict       | null |
+--------+---------+-------+------+-------+---------------------+---------------------+----------------+-----------+------------+------+
| sample |       1 |   1.5 |    1 |     0 | 2000-01-01 00:00:00 | 2000-01-01 11:59:59 | 0x480069002100 | [1, 2, 3] | {"one": 1} | NULL |
| NULL   |    NULL |  NULL | NULL |  NULL | NULL                | NULL                | NULL           | NULL      | NULL       | NULL |
+--------+---------+-------+------+-------+---------------------+---------------------+----------------+-----------+------------+------+
2 rows in set (0.0000 sec)
`)


//@<> Auto-mapping - explicit column order (string columns, data in lists)
var result_data_list = [
    [
        "sample",
        1,
        1.5,
        true,
        false,
        date,
        date_time,
        binary,
        [1,2,3],
        {one:1},
        null
    ],
    // This second record is to verify that all of the columns are nullable
    [
        null,
        null,
        null,
        null,
        null,
        null,
        null,
        null,
        null,
        null,
        null
    ]]

var result_3 = shell.createResult({columns:column_names, data:result_data_list})

// Order is defined lexicographycally
var names_3 = result_3.getColumnNames()
EXPECT_EQ(column_names, names_3, "Unexpected order of columns")


//@<> Auto-mapping - explicit column order DB Types and Flags (string columns, data in lists)
var columns_3 = result_3.getColumns()
var index = 0;
for(name of names_3) {
    EXPECT_EQ(mapping[name][0].data, columns_3[index].getType().data, `Unexpected type for column ${name}`)
    EXPECT_EQ(mapping[name][3], columns_3[index].getFlags(), `Unexpected flags for column ${name}`)
    index = index + 1;
}

//@<> Auto-mapping - explicit column order Data Mapping (string columns, data in lists)
var row = result_3.fetchOne();
var index = 0;
for(name of names_3) {
    value = row[index];
    if (mapping[name][2] != null) {
        value = mapping[name][2](value);
    }
    EXPECT_EQ(mapping[name][1], value, `Unexpected value for column ${name}`)
    index = index + 1;
}

//@<> Auto-mapping - explicit column order result dump (string columns, data in lists)
shell.createResult({columns:column_names, data:result_data_list})
EXPECT_OUTPUT_CONTAINS_MULTILINE(`
+--------+---------+-------+------+-------+---------------------+---------------------+----------------+-----------+------------+------+
| string | integer | float | true | false | date                | datetime            | binary         | array     | dict       | null |
+--------+---------+-------+------+-------+---------------------+---------------------+----------------+-----------+------------+------+
| sample |       1 |   1.5 |    1 |     0 | 2000-01-01 00:00:00 | 2000-01-01 11:59:59 | 0x480069002100 | [1, 2, 3] | {"one": 1} | NULL |
| NULL   |    NULL |  NULL | NULL |  NULL | NULL                | NULL                | NULL           | NULL      | NULL       | NULL |
+--------+---------+-------+------+-------+---------------------+---------------------+----------------+-----------+------------+------+
2 rows in set (0.0000 sec)
`)

// Auto-mapping Result - explicit column order (dictionary columns)
var column_name_dicts = [{name:'string'}, {name:'integer'}, {name:'float'}, {name:'true'}, {name:'false'}, {name:'date'}, {name:'datetime'}, {name:'binary'}, {name:'array'}, {name:'dict'}, {name:'null'}]
var result_4 = shell.createResult({columns:column_name_dicts, data:result_data})

//@<> Auto-mapping - explicit column order (dictionary columns)
// Order is defined lexicographycally
var names_4 = result_4.getColumnNames()
EXPECT_EQ(column_names, names_4, "Unexpected order of columns")


//@<> Auto-mapping - explicit column order DB Types and Flags (dictionary columns)
var columns_4 = result_4.getColumns()
var index = 0;
for(name of names_4) {
    EXPECT_EQ(mapping[name][0].data, columns_4[index].getType().data, `Unexpected type for column ${name}`)
    EXPECT_EQ(mapping[name][3], columns_4[index].getFlags(), `Unexpected flags for column ${name}`)
    index = index + 1;
}

//@<> Auto-mapping - explicit column order Data Mapping (dictionary columns)
var row = result_4.fetchOne();
var index = 0;
for(name of names_4) {
    value = row[index];
    if (mapping[name][2] != null) {
        value = mapping[name][2](value);
    }
    EXPECT_EQ(mapping[name][1], value, `Unexpected value for column ${name}`)
    index = index + 1;
}

//@<> Auto-mapping - explicit column order result dump (dictionary columns)
shell.createResult({columns:column_name_dicts, data:result_data})
EXPECT_OUTPUT_CONTAINS_MULTILINE(`
+--------+---------+-------+------+-------+---------------------+---------------------+----------------+-----------+------------+------+
| string | integer | float | true | false | date                | datetime            | binary         | array     | dict       | null |
+--------+---------+-------+------+-------+---------------------+---------------------+----------------+-----------+------------+------+
| sample |       1 |   1.5 |    1 |     0 | 2000-01-01 00:00:00 | 2000-01-01 11:59:59 | 0x480069002100 | [1, 2, 3] | {"one": 1} | NULL |
| NULL   |    NULL |  NULL | NULL |  NULL | NULL                | NULL                | NULL           | NULL      | NULL       | NULL |
+--------+---------+-------+------+-------+---------------------+---------------------+----------------+-----------+------------+------+
2 rows in set (0.0000 sec)
`)


// Auto-mapping Result - explicit column order (dictionary columns, data in lists)
var column_name_dicts = [{name:'string'}, {name:'integer'}, {name:'float'}, {name:'true'}, {name:'false'}, {name:'date'}, {name:'datetime'}, {name:'binary'}, {name:'array'}, {name:'dict'}, {name:'null'}]
var result_5 = shell.createResult({columns:column_name_dicts, data:result_data_list})

//@<> Auto-mapping - explicit column order (dictionary columns, data in lists)
// Order is defined lexicographycally
var names_5 = result_5.getColumnNames()
EXPECT_EQ(column_names, names_5, "Unexpected order of columns")


//@<> Auto-mapping - explicit column order DB Types (dictionary columns, data in lists)
var columns_5 = result_5.getColumns()
var index = 0;
for(name of names_5) {
    EXPECT_EQ(mapping[name][0].data, columns_5[index].getType().data, `Unexpected type for column ${name}`)
    EXPECT_EQ(mapping[name][3], columns_5[index].getFlags(), `Unexpected flags for column ${name}`)
    index = index + 1;
}

//@<> Auto-mapping - explicit column order Data Mapping (dictionary columns, data in lists)
var row = result_5.fetchOne();
var index = 0;
for(name of names_5) {
    value = row[index];
    if (mapping[name][2] != null) {
        value = mapping[name][2](value);
    }
    EXPECT_EQ(mapping[name][1], value, `Unexpected value for column ${name}`)
    index = index + 1;
}

//@<> Auto-mapping - explicit column order result dump (dictionary columns, data in lists)
shell.createResult({columns:column_name_dicts, data:result_data_list})
EXPECT_OUTPUT_CONTAINS_MULTILINE(`
+--------+---------+-------+------+-------+---------------------+---------------------+----------------+-----------+------------+------+
| string | integer | float | true | false | date                | datetime            | binary         | array     | dict       | null |
+--------+---------+-------+------+-------+---------------------+---------------------+----------------+-----------+------------+------+
| sample |       1 |   1.5 |    1 |     0 | 2000-01-01 00:00:00 | 2000-01-01 11:59:59 | 0x480069002100 | [1, 2, 3] | {"one": 1} | NULL |
| NULL   |    NULL |  NULL | NULL |  NULL | NULL                | NULL                | NULL           | NULL      | NULL       | NULL |
+--------+---------+-------+------+-------+---------------------+---------------------+----------------+-----------+------------+------+
2 rows in set (0.0000 sec)
`)


//@<> Explicit Type mapping - explicit column order
var user_columns_full = [
    {name:'string', type:'string'},
    {name:'integer', type:'string'},    // overrides the default
    {name:'float', type:'float'},
    {name:'true', type:'string'},       // overrides the default
    {name:'false', type: 'string'},     // overrides the default
    {name:'date', type:'date'},         // overrides the default
    {name:'datetime', type:'datetime'},
    {name:'binary', type:'bytes'},
    {name:'array', type:'string'},      // overrides the default
    {name:'dict', type:'string'},       // overrides the default
    {name:'null', type:'integer'}       // overrides the default
]

var result_6 = shell.createResult({columns:user_columns_full, data:result_data})
// Order is defined lexicographycally
var names_6 = result_6.getColumnNames()
EXPECT_EQ(column_names, names_6, "Unexpected order of columns")


//@<> Explicit Type mapping - explicit column order DB Types
// Override the types explicitly defined different to the default ones
mapping['date'][0] = mysqlx.Type.DATE;
mapping['array'][0] = mysqlx.Type.STRING;
mapping['array'][3] = "";
mapping['dict'][0] = mysqlx.Type.STRING;
mapping['dict'][3] = "";
mapping['null'][0] = mysqlx.Type.BIGINT;
mapping['null'][3] = "NUM";
mapping['integer'][0] = mysqlx.Type.STRING;
mapping['integer'][3] = "";
mapping['true'][0] = mysqlx.Type.STRING;
mapping['true'][3] = "";
mapping['false'][0] = mysqlx.Type.STRING;
mapping['false'][3] = "";

// Override the expected values that change
mapping['integer'][1] = "1";
mapping['true'][1] = "true";
mapping['false'][1] = "false";

var columns_6 = result_6.getColumns()
var index = 0;
for(name of names_6) {
    EXPECT_EQ(mapping[name][0].data, columns_6[index].getType().data, `Unexpected type for column ${name}`)
    EXPECT_EQ(mapping[name][3], columns_6[index].getFlags(), `Unexpected flags for column ${name}`)
    index = index + 1;
}

//@<> Explicit Type mapping - explicit column order Data Mapping
var row = result_6.fetchOne();
var index = 0;
for(name of names_6) {
    value = row[index];
    if (mapping[name][2] != null) {
        value = mapping[name][2](value);
    }
    EXPECT_EQ(mapping[name][1], value, `Unexpected value for column ${name}`)
    index = index + 1;
}

//@<> Explicit Type mapping - explicit column order result dump (dictionary columns)
shell.createResult({columns:user_columns_full, data:result_data})
EXPECT_OUTPUT_CONTAINS_MULTILINE(`
+--------+---------+-------+------+-------+---------------------+---------------------+----------------+-----------+------------+------+
| string | integer | float | true | false | date                | datetime            | binary         | array     | dict       | null |
+--------+---------+-------+------+-------+---------------------+---------------------+----------------+-----------+------------+------+
| sample | 1       |   1.5 | true | false | 2000-01-01 00:00:00 | 2000-01-01 11:59:59 | 0x480069002100 | [1, 2, 3] | {"one": 1} | NULL |
| NULL   | NULL    |  NULL | NULL | NULL  | NULL                | NULL                | NULL           | NULL      | NULL       | NULL |
+--------+---------+-------+------+-------+---------------------+---------------------+----------------+-----------+------------+------+
2 rows in set (0.0000 sec)
`)


//@<> Explicit Type mapping - explicit column order (data in lists)
var user_columns_full = [{name:'string', type:'string'}, {name:'integer', type:'string'}, {name:'float', type:'float'}, {name:'true', type:'string'}, {name:'false', type: 'string'}, {name:'date', type:'date'}, {name:'datetime', type:'datetime'}, {name:'binary', type:'bytes'}, {name:'array', type:'string'}, {name:'dict', type:'string'}, {name:'null', type:'integer'}]
var result_7 = shell.createResult({columns:user_columns_full, data:result_data_list})
// Order is defined lexicographycally
var names_7 = result_7.getColumnNames()
EXPECT_EQ(column_names, names_7, "Unexpected order of columns")


//@<> Explicit Type mapping - explicit column order DB Types (data in lists)
var columns_7 = result_7.getColumns()
var index = 0;
for(name of names_7) {
    EXPECT_EQ(mapping[name][0].data, columns_7[index].getType().data, `Unexpected type for column ${name}`)
    EXPECT_EQ(mapping[name][3], columns_7[index].getFlags(), `Unexpected flags for column ${name}`)
    index = index + 1;
}

//@<> Explicit Type mapping - explicit column order Data Mapping (data in lists)
var row = result_7.fetchOne();
var index = 0;
for(name of names_7) {
    value = row[index];
    if (mapping[name][2] != null) {
        value = mapping[name][2](value);
    }
    EXPECT_EQ(mapping[name][1], value, `Unexpected value for column ${name}`)
    index = index + 1;
}

//@<> Explicit Type mapping - explicit column order result dump (data in lists)
shell.createResult({columns:user_columns_full, data:result_data_list})
EXPECT_OUTPUT_CONTAINS_MULTILINE(`
+--------+---------+-------+------+-------+---------------------+---------------------+----------------+-----------+------------+------+
| string | integer | float | true | false | date                | datetime            | binary         | array     | dict       | null |
+--------+---------+-------+------+-------+---------------------+---------------------+----------------+-----------+------------+------+
| sample | 1       |   1.5 | true | false | 2000-01-01 00:00:00 | 2000-01-01 11:59:59 | 0x480069002100 | [1, 2, 3] | {"one": 1} | NULL |
| NULL   | NULL    |  NULL | NULL | NULL  | NULL                | NULL                | NULL           | NULL      | NULL       | NULL |
+--------+---------+-------+------+-------+---------------------+---------------------+----------------+-----------+------------+------+
2 rows in set (0.0000 sec)
`)

//@<> Full Single Result
function getSingleResult() {
    return shell.createResult({
    affectedItemsCount: 1,
    data: [{one:1, two:2}, {one:3, two:4}],
    executionTime: 1.5,
    warnings: [
        {
            level: "warning",
            message: "sample warning",
            code: 1
        },
        {
            level: "note",
            message: "sample note warning",
            code: 2
        }
    ],
    info: "Successful sample result!"});
}

var result_8 = getSingleResult();
EXPECT_EQ(1, result_8.affectedItemsCount, "affectedItemsCount")
EXPECT_EQ(1, result_8.getAffectedItemsCount(), "getAffectedItemsCount()")
EXPECT_EQ("1.5000 sec", result_8.executionTime, "executionTime")
EXPECT_EQ("1.5000 sec", result_8.getExecutionTime(), "getExecutionTime()")
EXPECT_EQ("Successful sample result!", result_8.info, "info")
EXPECT_EQ("Successful sample result!", result_8.getInfo(), "getInfo()")

EXPECT_EQ(2, result_8.warningsCount, "warningsCount")
EXPECT_EQ(2, result_8.getWarningsCount(), "getWarningsCount()")
EXPECT_EQ([["Warning", 1, "sample warning"], ["Note", 2, "sample note warning"]], result_8.warnings, "warnings")

// TODO(rennox): There's a problem with the way the warnings are fetched
// in all the result classes. i.e. once they are fetched once, they are gone
// This is more critical i.e. if autocompletion is called in JS mode, as it
// will cause the warnings to be consumed even without user having requested it.
EXPECT_EQ([], result_8.warnings, "warnings-2")
EXPECT_EQ([], result_8.getWarnings(), "getWarnings()")

EXPECT_EQ(2, result_8.columnCount, "columnCount")
EXPECT_EQ(2, result_8.getColumnCount(), "getColumnCount()")
EXPECT_EQ(['one', 'two'], result_8.columnNames, "columnNames")
EXPECT_EQ(['one', 'two'], result_8.getColumnNames(), "getColumnNames()")
EXPECT_EQ('one', result_8.columns[0].columnLabel, "columns")
EXPECT_EQ('two', result_8.columns[1].columnLabel, "columns")

EXPECT_EQ(true, result_8.hasData(), "hasData()")
EXPECT_EQ([1, 2], result_8.fetchOne(), "fetchOne()")
EXPECT_EQ([[3, 4]], result_8.fetchAll(), "fetchAll()")

var result_9 = getSingleResult();
EXPECT_EQ([[1, 2], [3, 4]], result_9.fetchAll(), "fetchAll()")

//@<> Dumping result with data
getSingleResult();
EXPECT_OUTPUT_CONTAINS_MULTILINE(`
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
`)
WIPE_OUTPUT()

//@<> Multi-Result-Handling
var result_10 = shell.createResult([
        {
            data: [{one:1, two:2}, {one:3, two:4}],
            executionTime: 1.5,
            info: "First Result!"
        },
        {
            info: "Second Result"
        },
        {
            data: [{fname:"John", lname:"Doe"}, {fname:"Jane", lname:"Doe"}],
            executionTime: 0.34,
            info: "Last Result!"
        },
    ]);

// Digging into the first result
EXPECT_EQ(true, result_10.hasData());
EXPECT_EQ("1.5000 sec", result_10.executionTime);
EXPECT_EQ("First Result!", result_10.info);
EXPECT_EQ(['one', 'two'], result_10.columnNames);
EXPECT_EQ([[1,2], [3,4]], result_10.fetchAll());

// Switching to the second result
EXPECT_EQ(true, result_10.nextResult())
EXPECT_EQ(false, result_10.hasData());
EXPECT_EQ("0.0000 sec", result_10.executionTime);
EXPECT_EQ("Second Result", result_10.info);
EXPECT_EQ([], result_10.columnNames);
EXPECT_EQ([], result_10.fetchAll());

// Switching to the third result
EXPECT_EQ(true, result_10.nextResult())
EXPECT_EQ(true, result_10.hasData());
EXPECT_EQ("0.3400 sec", result_10.executionTime);
EXPECT_EQ("Last Result!", result_10.info);
EXPECT_EQ(['fname', 'lname'], result_10.columnNames);
EXPECT_EQ([['John', 'Doe'],['Jane', 'Doe']], result_10.fetchAll());

// No more results!
EXPECT_EQ(false, result_9.nextResult())

//@<> Dumping Multi-Result Object
WIPE_OUTPUT()
shell.createResult([
    {
        data: [{one:1, two:2}, {one:3, two:4}],
        executionTime: 1.5,
        info: "First Result!"
    },
    {
        info: "Second Result"
    },
    {
        data: [{fname:"John", lname:"Doe"}, {fname:"Jane", lname:"Doe"}],
        executionTime: 0.34,
        info: "Last Result!"
    },
]);

EXPECT_OUTPUT_CONTAINS_MULTILINE(`
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
`)
WIPE_OUTPUT()


//@<> Navigating a Multi-Result Object that includes an error
WIPE_OUTPUT()
var result_11 = shell.createResult([
    {
        data: [{one:1, two:2}, {one:3, two:4}],
        executionTime: 1.5,
        info: "First Result!"
    },
    {
        info: "Second Result"
    },
    {
        error: "An error occurred in a multiresult operation"
    },
]);

EXPECT_EQ(true, result_11.hasData());
EXPECT_EQ("1.5000 sec", result_11.executionTime);
EXPECT_EQ("First Result!", result_11.info);
EXPECT_EQ(['one', 'two'], result_11.columnNames);
EXPECT_EQ([[1,2], [3,4]], result_11.fetchAll());

// Switching to the second result
EXPECT_EQ(true, result_11.nextResult())
EXPECT_EQ(false, result_11.hasData());
EXPECT_EQ("0.0000 sec", result_11.executionTime);
EXPECT_EQ("Second Result", result_11.info);
EXPECT_EQ([], result_11.columnNames);
EXPECT_EQ([], result_11.fetchAll());

// Switching to the error result
EXPECT_THROWS(function() { result_11.nextResult(); }, "An error occurred in a multiresult operation")

//@<> Data type validation
let res = shell.createResult({"data": [[1234, 4567], [89, 0.1]], "columns": ["a", "b"]})
EXPECT_NO_THROWS(function() {res.fetchOne()})
EXPECT_THROWS(function() { res.fetchOne() }, "Invalid typecast: Integer expected, but Float value is out of range, at row 1, column 'b'")
