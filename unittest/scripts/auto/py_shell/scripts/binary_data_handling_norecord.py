shell.connect(__mysqluripwd)

binary_types = ["TINYBLOB", "BLOB", "MEDIUMBLOB", "LONGBLOB", "VARBINARY(20)", "BINARY(20)"]
custom_values={"BINARY(20)": (b'some\x00value\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00', "0x736F6D650076616C756500000000000000000000", "b'some\\x00value\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00'")}

session.run_sql("drop schema if exists py_binary_data")
session.run_sql("create schema py_binary_data")


def validate(row, binary_type, expected_binary_value, expected_str_shell, expected_str_python):
    EXPECT_EQ("<class 'bytes'>", str(type(row[0])), binary_type)
    EXPECT_EQ(expected_binary_value, row[0], binary_type)
    # Value as the shell prints it
    print(row)
    EXPECT_STDOUT_CONTAINS(expected_str_shell, binary_type)
    # Value as printed by python
    print(row[0])
    EXPECT_STDOUT_CONTAINS(expected_str_python, binary_type)

def test_run_sql(asession, binary_type, binary_value, expected_binary_value, expected_str_shell, expected_str_python):
    # Test data insertion and retrieval using ClassicSession.run_sql
    asession.run_sql("insert into py_binary_data.sample values (?)", [b'some\0value'])
    res = asession.run_sql("select * from py_binary_data.sample")
    row = res.fetch_one()
    validate(row, binary_type, expected_binary_value, expected_str_shell, expected_str_python)
    session.run_sql("delete from py_binary_data.sample")


def test_classic(binary_type, binary_value, expected_binary_value, expected_str_shell, expected_str_python):
    asession = mysql.get_session(__mysqluripwd)
    test_run_sql(asession, binary_type, binary_value, expected_binary_value, expected_str_shell, expected_str_python)
    asession.run_sql("delete from py_binary_data.sample")
    asession.close()

def test_table_crud(asession, binary_type, binary_value, expected_binary_value, expected_str_shell, expected_str_python):
    schema = asession.get_schema('py_binary_data')
    table = schema.sample
    table.insert().values(binary_value).execute()
    row = table.select().execute().fetch_one()
    validate(row, binary_type, expected_binary_value, expected_str_shell, expected_str_python)


def test_x(binary_type, binary_value, expected_binary_value, expected_str_shell, expected_str_python):
    asession = mysqlx.get_session(__uripwd)
    test_run_sql(asession, binary_type, binary_value, expected_binary_value, expected_str_shell, expected_str_python)
    asession.run_sql("delete from py_binary_data.sample")
    test_table_crud(asession, binary_type, binary_value, expected_binary_value, expected_str_shell, expected_str_python)
    asession.run_sql("delete from py_binary_data.sample")
    asession.close()

for binary_type in binary_types:
    # An binary value will be created to hold the data to be stored on the database
    # By default it is expected that once the data is retrieved an identical binary value
    # is returned except for BINARY(30), see below.
    binary_value=b'some\x00value'
    expected_binary_value = binary_value
    expected_str_shell = "0x736F6D650076616C7565"
    expected_str_python = "b'some\\x00value'"
    # On BINARY(20) the row will be filled with 0's to fulfill the row size, so even the same
    # data is stored, a different binary value is returned
    if binary_type in custom_values:
       expected_binary_value, expected_str_shell, expected_str_python = custom_values[binary_type]
    session.run_sql(f"create table py_binary_data.sample(data {binary_type})")
    # Test data insertion and retrieval using ClassicSession.run_sql
    test_classic(binary_type, binary_value, expected_binary_value, expected_str_shell, expected_str_python)
    test_x(binary_type, binary_value, expected_binary_value, expected_str_shell, expected_str_python)
    session.run_sql("drop table py_binary_data.sample")

session.run_sql("drop schema py_binary_data")
shell.disconnect()

#Shell_scripted/Auto_script_py.run_and_check/binary_data_handling