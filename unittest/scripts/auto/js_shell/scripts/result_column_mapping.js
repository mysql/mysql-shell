//@<> Preparation
shell.connect(__mysqluripwd)
let result = session.runSql("select table_name, column_name, referenced_table_name, referenced_column_name  from information_schema.key_column_usage;")
let row = result.fetchOne()
let fields = dir(row)

//@<> Column Mapping {VER(>=8.0.0)}
EXPECT_TRUE(fields.includes("TABLE_NAME"))
EXPECT_TRUE(fields.includes("COLUMN_NAME"))
EXPECT_TRUE(fields.includes("REFERENCED_TABLE_NAME"))
EXPECT_TRUE(fields.includes("REFERENCED_COLUMN_NAME"))


//@<> Column Mapping {VER(<8.0.0)}
EXPECT_TRUE(fields.includes("table_name"))
EXPECT_TRUE(fields.includes("column_name"))
EXPECT_TRUE(fields.includes("referenced_table_name"))
EXPECT_TRUE(fields.includes("referenced_column_name"))

//@<> Cleanup
shell.disconnect()
