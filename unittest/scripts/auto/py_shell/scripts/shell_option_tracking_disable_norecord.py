#@ {__version_num >= 90500}
#@<> Setup
import os
import shutil

testutil.set_execution_context("disable")

testutil.deploy_raw_sandbox(__mysql_sandbox_port1, "root", {
    "local_infile": 1
})
instance_count = 1

testutil.import_data(__sandbox_uri1, __data_path+"/sql/sakila-schema.sql")
testutil.import_data(__sandbox_uri1, __data_path+"/sql/sakila-data.sql", "sakila")


main_session = mysql.get_session(__sandbox_uri1)
main_session.run_sql("INSTALL COMPONENT 'file://component_option_tracker'")

def get_variables(a_session):
    result = a_session.run_sql("select * from performance_schema.global_status where VARIABLE_NAME like 'option_tracker_usage:Mysql Shell%'")
    data = {}
    row = result.fetch_one()
    while(row):
        data[row.VARIABLE_NAME] = int(row.VARIABLE_VALUE)
        row = result.fetch_one()
    return data

# NOTE: This is the core of this test, the initial values of the counters are retrieved (expected_variables), and they
# will go increasing during the test, comparing with the actual counters in the database, to make sure they are
# increased as required
def verify_variables(expected, a_session):
    actual_variables = get_variables(a_session)
    for key in expected:
        EXPECT_EQ(expected[key], actual_variables[key], f"Mistmatched Value for {key}, expected: {expected[key]}, actual: {actual_variables[key]}")

def print_variables(variables):
    for key, value in variables.items():
        print(f"{key}: {value}")

expected_variables = get_variables(main_session)
verify_variables(expected_variables, main_session)
print_variables(expected_variables)

#@<> WL16659-FR2.2 in relation to FR1.1
# The Mysql Shell counter will be increased once for each user session created
# by any of the following
test_session = shell.open_session(__sandbox_uri1)
test_session.run_sql("select 1")
# expected_variables['option_tracker_usage:Mysql Shell for VS Code'] += 1
test_session.close()
verify_variables(expected_variables, main_session)

test_session = mysql.get_session(__sandbox_uri1)
test_session.run_sql("select 1")
# expected_variables['option_tracker_usage:Mysql Shell for VS Code'] += 1
test_session.close()
verify_variables(expected_variables, main_session)

test_session = mysql.get_classic_session(__sandbox_uri1)
test_session.run_sql("select 1")
# expected_variables['option_tracker_usage:Mysql Shell for VS Code'] += 1
test_session.close()
verify_variables(expected_variables, main_session)

testutil.call_mysqlsh([__sandbox_uri1, '--execution-context=DISABLE', '-e', 'select 1'])
# expected_variables['option_tracker_usage:Mysql Shell for VS Code'] += 1
verify_variables(expected_variables, main_session)

with open('sample.sql', 'w') as file:
    file.write(f"\\connect {__sandbox_uri1}")
testutil.call_mysqlsh(['--interactive', '--execution-context=disable', '-f', 'sample.sql'])
os.remove('sample.sql')
# expected_variables['option_tracker_usage:Mysql Shell for VS Code'] += 1
verify_variables(expected_variables, main_session)


# NOTE: This will create the global session let for the rest of the tests
shell.connect(__sandbox_uri1)
# expected_variables['option_tracker_usage:Mysql Shell for VS Code'] += 1
verify_variables(expected_variables, main_session)

#@<> WL16659-FR2.2 in relation to FR1.2
# The MySQL Shell - Dump counter will be increased once every time any of the
# dump utilities is used
util.dump_instance('instance')
# expected_variables['option_tracker_usage:Mysql Shell VSC - Dump'] += 1
verify_variables(expected_variables, main_session)

util.dump_schemas(['sakila'], 'schemas')
# expected_variables['option_tracker_usage:Mysql Shell VSC - Dump'] += 1
verify_variables(expected_variables, main_session)
shutil.rmtree('schemas')

util.dump_tables('sakila', ['actor'], 'tables')
# expected_variables['option_tracker_usage:Mysql Shell VSC - Dump'] += 1
verify_variables(expected_variables, main_session)
shutil.rmtree('tables')

session.run_sql("drop schema sakila")


#@<> WL16659-FR2.2 in relation to FR1.3
# The MySQL Shell - Dump Load counter will be increased once every time the
# util.loadDump(...) utility is used.
util.load_dump('instance')
# expected_variables['option_tracker_usage:Mysql Shell VSC - Dump - Load'] += 1
verify_variables(expected_variables, main_session)
shutil.rmtree('instance')

#@<> WL16659-FR2.2 in relation to FR1.4
# The MySQL Shell - Copy counter will be increased for both the source and
# target instances once every time any of the copy utilities is used
testutil.deploy_raw_sandbox(__mysql_sandbox_port2, "root", {
    "local_infile": 1
})
instance_count += 1
secondary_session = mysql.get_session(__sandbox_uri2)
secondary_session.run_sql("INSTALL COMPONENT 'file://component_option_tracker'")

expected_variables2 = get_variables(secondary_session)
verify_variables(expected_variables2, secondary_session)

# Copy Instance
util.copy_instance(__sandbox_uri2, {"users": False})
# expected_variables['option_tracker_usage:Mysql Shell - Copy'] += 1
verify_variables(expected_variables, main_session)

# expected_variables2['option_tracker_usage:Mysql Shell - Copy'] += 1
verify_variables(expected_variables2, secondary_session)


# Copy Schemas
secondary_session.run_sql("drop schema sakila")
util.copy_schemas(['sakila'], __sandbox_uri2)
# expected_variables['option_tracker_usage:Mysql Shell - Copy'] += 1
verify_variables(expected_variables, main_session)

# expected_variables2['option_tracker_usage:Mysql Shell - Copy'] += 1
verify_variables(expected_variables2, secondary_session)

# Copy Tables
secondary_session.run_sql("drop table sakila.film_category")
util.copy_tables('sakila', ['film_category'], __sandbox_uri2)
# expected_variables['option_tracker_usage:Mysql Shell - Copy'] += 1
verify_variables(expected_variables, main_session)

# expected_variables2['option_tracker_usage:Mysql Shell - Copy'] += 1
verify_variables(expected_variables2, secondary_session)


#@<> WL16659-FR2.2 in relation to FR1.5 {__version_num < __mysh_version_num}
# The MySQL Shell - Upgrade Checker counter will be increased once every time
# the util.checkForServerUpgrade(...) utility is used.
util.check_for_server_upgrade()
# expected_variables['option_tracker_usage:Mysql Shell - Upgrade Checker'] += 1
verify_variables(expected_variables, main_session)

#@<> WL16659-FR2.2 in relation to FR3/FR4.2
session.set_option_tracker_feature_id("mysql_ot_msh.vsc.hw_chat")
session.run_sql("SELECT 1")
#expected_variables['option_tracker_usage:Mysql Shell VSC - HeatWave Chat'] += 1
verify_variables(expected_variables, main_session)


#@<> WL16659-FR2.2 in relation to FR3/FR4.4 
# The Mysql Shell VSC - HeatWave Chat counter will be increased once, every time
# a prompt is executed in the \chat mode in the SQL Editor
session.set_option_tracker_feature_id("mysql_ot_msh<ctx>.mrs")
session.run_sql("SELECT 1")
#expected_variables['option_tracker_usage:Mysql Shell VSC - MRS'] += 1
verify_variables(expected_variables, main_session)

#@<> WL16659-FR2.2 in relation to FR3/FR4.5
# The Mysql Shell VSC - Natural Language to SQL counter will be increased once,
# every time the \nl command is executed in the SQL Editor
session.set_option_tracker_feature_id("mysql_ot_msh.vsc.nl2sql")
session.run_sql("SELECT 1")
#expected_variables['option_tracker_usage:Mysql Shell VSC - Natural Language to SQL'] += 1
verify_variables(expected_variables, main_session)

#@<> WL16659-FR2.2 in relation to /FR4.6
# The Mysql Shell VSC - LakeHouse Navigator counter will be increased once,
# every time the data loading process is launched in the MySQL Heatwave Navigator
session.set_option_tracker_feature_id("mysql_ot_msh.vsc.lh_nav")
session.run_sql("SELECT 1")
# expected_variables['option_tracker_usage:Mysql Shell VSC - LakeHouse Navigator'] += 1
verify_variables(expected_variables, main_session)


#@<> Finalize
session.close()
if instance_count:
    testutil.destroy_sandbox(__mysql_sandbox_port1)

if instance_count > 1:
    testutil.destroy_sandbox(__mysql_sandbox_port2)