#@<> Preparation
import json
import base64

shell.connect(__mysqluripwd)

over_limit = b""
for index in range(26):
    over_limit = over_limit + b"0123456789"
on_limit = over_limit[0:-4]
truncated_data = over_limit[0:-3]
under_limit = over_limit[0:100]


session.run_sql("drop schema if exists py_json_shell_binary_data")
session.run_sql("create schema py_json_shell_binary_data")
session.run_sql("create table py_json_shell_binary_data.sample(id int, data blob)")
session.run_sql("insert into py_json_shell_binary_data.sample values (1, ?)", [under_limit])
session.run_sql("insert into py_json_shell_binary_data.sample values (2, ?)", [on_limit])
session.run_sql("insert into py_json_shell_binary_data.sample values (3, ?)", [over_limit])


shell.disconnect()


def execute_query(query):
    testutil.call_mysqlsh(["--sql", "-e", query, __mysqluripwd], "", ["MYSQLSH_JSON_SHELL=1", "MYSQLSH_TERM_COLOR_MODE=nocolor"])


validations =[              # Data length
    (1,under_limit),        # Lower than the 256 limit, should be complete
    (2,on_limit),           # On the limit, should be complete
    (3,truncated_data)      # Over the limit, should be truncated to 257 (one extra)
]

#@<> Testing...
for data in validations:
    record_id, expected = data
    WIPE_OUTPUT()
    execute_query(f"select * from py_json_shell_binary_data.sample where id = {record_id}")
    output = testutil.fetch_captured_stdout(False)
    WIPE_OUTPUT()
    lines = output.strip().split("\n")
    for line in lines:
        doc = json.loads(line)
        if "rows" in doc:
            data = doc["rows"][0]["data"]
            EXPECT_EQ(expected, base64.b64decode(data))
