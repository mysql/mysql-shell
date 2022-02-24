shell.connect(__mysqluripwd)

let binary_types = ["TINYBLOB", "BLOB", "MEDIUMBLOB", "LONGBLOB", "VARBINARY(20)", "BINARY(30)"]
let custom_values={}
custom_values["BINARY(30)"] = ["some\0value\0\0\0\0\0", "0x73006F006D0065000000760061006C007500650000000000000000000000"]

session.runSql("drop schema if exists js_binary_data")
session.runSql("create schema js_binary_data")

function str2ab(str) {
    var buf = new ArrayBuffer(str.length * 2);
    var bufView = new Uint16Array(buf);
    for (var i = 0; i < str.length; i++) {
      bufView[i] = str.charCodeAt(i);
    }
    return buf;
}

function ab2str(buf) {
    return String.charAt.apply(null, new Uint16Array(buf));
}

function validate(row, binary_type, expected_array_buffer, expected_str) {
    EXPECT_EQ("ArrayBuffer", String(type(row[0])), binary_type)
    EXPECT_EQ(expected_array_buffer, row[0], binary_type)
    // Value as the shell prints it
    print(row)
    EXPECT_STDOUT_CONTAINS(expected_str, binary_type)
    // Value as printed by python
    print(row[0])
    EXPECT_STDOUT_CONTAINS(expected_str, binary_type)
}

function test_runSql(asession, binary_type, array_buffer, expected_array_buffer, expected_str) {
    asession.runSql("insert into js_binary_data.sample values (?)", [array_buffer])
    res = asession.runSql("select * from js_binary_data.sample")
    row = res.fetchOne()
    validate(row, binary_type, expected_array_buffer, expected_str)
    session.runSql("delete from js_binary_data.sample")
}


function test_classic(binary_type, array_buffer, expected_array_buffer, expected_str) {
    asession = mysql.getSession(__mysqluripwd)
    test_runSql(asession, binary_type, array_buffer, expected_array_buffer, expected_str)
    asession.runSql("delete from js_binary_data.sample")
    asession.close()
}

function test_table_crud(asession, binary_type, array_buffer, expected_array_buffer, expected_str) {
    schema = asession.getSchema('js_binary_data')
    table = schema.sample
    table.insert().values(array_buffer).execute()
    row = table.select().execute().fetchOne()
    validate(row, binary_type, expected_array_buffer, expected_str)
}

function test_x(binary_type, array_buffer, expected_array_buffer, expected_str) {
    asession = mysqlx.getSession(__uripwd)
    test_runSql(asession, binary_type, array_buffer, expected_array_buffer, expected_str)
    asession.runSql("delete from js_binary_data.sample")
    test_table_crud(asession, binary_type, array_buffer, expected_array_buffer, expected_str)
    asession.runSql("delete from js_binary_data.sample")
}


for (index in binary_types) {
    binary_type = binary_types[index]
    // An array buffer will be created to hold the data to be stored on the database
    // By default it is expected that once the data is retrieved an identical array buffer
    // is returned except for BINARY(30), see below.
    let array_buffer = str2ab("some\0value")
    let expected_array_buffer = array_buffer
    expected_str = "0x73006F006D0065000000760061006C0075006500"

    // On BINARY(30) the row will be filled with 0's to fulfill the row size, so even the same
    // data is stored, a different ArrayBuffer is returned
    if (binary_type in custom_values) {
       expected_array_buffer = str2ab(custom_values[binary_type][0])
       expected_str = custom_values[binary_type][1]
    }

    session.runSql(`create table js_binary_data.sample(data ${binary_type})`)
    // Test data insertion and retrieval using ClassicSession.runSql
    test_classic(binary_type, array_buffer, expected_array_buffer, expected_str)
    test_x(binary_type, array_buffer, expected_array_buffer, expected_str)
    session.runSql("drop table js_binary_data.sample")
}

session.runSql("drop schema js_binary_data")
shell.disconnect()
