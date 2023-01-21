#@{VER(<8.0.0)}
#@ Sandbox Deployment
import os
testutil.deploy_raw_sandbox(port=__mysql_sandbox_port1, rootpass="root")
shell.connect(__sandbox_uri1)

util.check_for_server_upgrade()

#@ Invalid Name Check
datadir = testutil.get_sandbox_datadir(__mysql_sandbox_port1)
session.run_sql("CREATE SCHEMA uctest1")
session.run_sql("CREATE TABLE uctest1.demo (ID INT)")
session.run_sql("CREATE SCHEMA uctest2")

testutil.stop_sandbox(__mysql_sandbox_port1, options={"wait":1})

old_table_name = os.path.join(datadir, "uctest1", "demo")
new_table_name = os.path.join(datadir, "uctest1", "lost+found")
old_schema_folder = os.path.join(datadir, "uctest2")
new_schema_folder = os.path.join(datadir, "lost+found")

os.rename(old_schema_folder, new_schema_folder)
os.rename(old_table_name + ".frm", new_table_name + ".frm")
os.rename(old_table_name + ".ibd", new_table_name + ".ibd")

testutil.start_sandbox(__mysql_sandbox_port1)
shell.connect(__sandbox_uri1)

util.check_for_server_upgrade()

#@<> Cleanup
testutil.destroy_sandbox(__mysql_sandbox_port1)