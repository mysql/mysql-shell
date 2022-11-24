#@<> Preparation
shell.connect(__mysqluripwd)
result = result = session.run_sql("select table_name, column_name, referenced_table_name, referenced_column_name  from information_schema.key_column_usage;")
row = result.fetch_one()
fields = dir(row)

#@<> Column Mapping {VER(>=8.0.0)}
EXPECT_TRUE("TABLE_NAME" in fields)
EXPECT_TRUE("COLUMN_NAME" in fields)
EXPECT_TRUE("REFERENCED_TABLE_NAME" in fields)
EXPECT_TRUE("REFERENCED_COLUMN_NAME" in fields)


#@<> Column Mapping {VER(<8.0.0)}
EXPECT_TRUE("table_name" in fields)
EXPECT_TRUE("column_name" in fields)
EXPECT_TRUE("referenced_table_name" in fields)
EXPECT_TRUE("referenced_column_name" in fields)

#@<> Cleanup
shell.disconnect()
