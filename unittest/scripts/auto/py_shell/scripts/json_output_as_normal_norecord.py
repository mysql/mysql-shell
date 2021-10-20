#@ {VER(>=8.0.27)}
#@<> Initialization
import os
import json

def callMysqlsh(additional_args):
    base_args = [__sandbox_uri1, "--quiet-start=2", "--log-level=debug"]
    return testutil.call_mysqlsh(base_args + additional_args, "", ["MYSQLSH_TERM_COLOR_MODE=nocolor"])

def rmdir(path):
    if os.path.exists(path):
        testutil.rmdir(path, True)


testutil.deploy_sandbox(__mysql_sandbox_port1, 'root')
shell.connect(__sandbox_uri1)

schemas = ['schema_a', 'schema_b', 'schema_c']
tables = ['table_a', 'table_b', 'table_c']

for schema in schemas:
    session.run_sql(f'drop schema if exists {schema}');
    session.run_sql(f'create schema {schema}')
    for table in tables:
        session.run_sql(f"create table {schema}.{table} (id INT PRIMARY KEY, column_1 VARCHAR(10), column_2 VARCHAR(10))")
        session.run_sql(f"insert into {schema}.{table} values (1, 'john','doe')")
        session.run_sql(f"CREATE SQL SECURITY DEFINER VIEW {schema}.{table}_v AS SELECT * FROM {schema}.{table}")

#@<OUT> Normal Dump
WIPE_OUTPUT()
rmdir('test-dump')
rc = callMysqlsh(["--", "util", "dump-instance", 'test-dump', '--excludeSchemas=\"schema_a\",\"schema_b\"'])
EXPECT_EQ(0, rc);


#@<OUT> JSON Dump
WIPE_OUTPUT()
rmdir('test-dump')
rc = callMysqlsh(["--json=raw", "--", "util", "dump-instance", 'test-dump', '--excludeSchemas=\"schema_a\",\"schema_b\"'])
json_output = testutil.fetch_captured_stdout(False)

#@<OUT> Processed JSON output [USE:Normal Dump]
# The entire output is json docs, one per line
string_docs = json_output.split("\n")

processed_json_outout = ""
for str_doc in string_docs:
    try:
        doc=json.loads(str_doc)
        for v in doc.values():
            processed_json_outout += v
    except json.decoder.JSONDecodeError:
        # Trailing empty lines are simply ignored
        pass

print(processed_json_outout)


#@<> Cleanup
for schema in schemas:
    session.run_sql(f'drop schema if exists {schema}');
session.close()

testutil.destroy_sandbox(__mysql_sandbox_port1);
