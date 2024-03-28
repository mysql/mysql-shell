import math
import multiprocessing
from timeit import default_timer as timer

#@<> setup
def check_for_instance(output, number_instances, target_address):
    count = 0
    target = None
    for i in output:
        count += 1
        EXPECT_TRUE("instance" in i, "Object is missing field \"instance\".")
        EXPECT_TRUE("address" in i["instance"], "Instance is missing field \"address\".")
        if i["instance"]["address"] == target_address:
            target = i
    EXPECT_EQ(number_instances, count, "Incorrect number of instances on the output.")
    return target

def check_output(i, columns):
    EXPECT_TRUE("output" in i, "Object is missing field \"output\".")
    EXPECT_FALSE("error" in i, "Object shouldn't have \"error\" field.")
    EXPECT_TRUE("executionTime" in i, "Output is missing field \"executionTime\".")
    output = i["output"]
    EXPECT_EQ(1, len(output), "Output must have a single resultset.")
    output = output[0];
    EXPECT_TRUE("columnNames" in output, "Output is missing field \"columnNames\".")
    EXPECT_TRUE("rows" in output, "Output is missing field \"rows\".")
    EXPECT_EQ(len(columns), len(output["columnNames"]), "Size of columns array doesn't match")
    for i in range(0, len(columns)):
       EXPECT_EQ(columns[i], output["columnNames"][i], f'Column names don\'t match: \'{columns[i]}\' vs \'{output["columnNames"][i]}\'')

def check_error(i, type, msg):
    EXPECT_TRUE("error" in i, "Object is missing field \"error\".")
    EXPECT_FALSE("output" in i, "Object shouldn't have \"output\" field.")
    error = i["error"]
    EXPECT_TRUE("type" in error, "Error is missing field \"type\".")
    EXPECT_TRUE("message" in error, "Error is missing field \"message\".")
    EXPECT_EQ(type, error["type"], "Error type doesn't match")
    if msg is not None:
        EXPECT_EQ(msg, error["message"], "Error message doesn't match")

def check_warning(i, msg):
    EXPECT_TRUE("warnings" in i, "Object is missing field \"warnings\".")
    warnings = i["warnings"]
    EXPECT_TRUE(msg in warnings, "Warning message not found.")

testutil.deploy_sandbox(__mysql_sandbox_port1, 'root', {'report_host': hostname})
testutil.deploy_sandbox(__mysql_sandbox_port2, 'root', {'report_host': hostname})
testutil.deploy_sandbox(__mysql_sandbox_port3, 'root', {'report_host': hostname})
testutil.deploy_sandbox(__mysql_sandbox_port4, 'root', {'report_host': hostname})
testutil.deploy_sandbox(__mysql_sandbox_port5, 'root', {'report_host': hostname})

shell.connect(__sandbox_uri1)

cluster = dba.create_cluster('cluster', {"gtidSetIsComplete": True})
cluster.add_instance(__sandbox_uri2)
cluster.add_instance(__sandbox_uri3)
cluster.add_replica_instance(__sandbox_uri4)

#@<> validate arguments
EXPECT_THROWS(lambda: cluster.execute("SELECT 1;", "") , f'"" is not a valid keyword for option \'instances\'')
EXPECT_THROWS(lambda: cluster.execute("SELECT 1;", []) , f'The array of instance addresses in option \'instances\' cannot be empty.')
EXPECT_THROWS(lambda: cluster.execute("SELECT 1;", "bar") , f'"bar" is not a valid keyword for option \'instances\'')
EXPECT_THROWS(lambda: cluster.execute("SELECT 1;", "all", {"exclude": True}) , f'Argument #3: The option \'exclude\' only accepts a keyword (string) or a list of instance addresses.')
EXPECT_THROWS(lambda: cluster.execute("SELECT 1;", "all", {"exclude": int}) , f'Argument #3: The option \'exclude\' only accepts a keyword (string) or a list of instance addresses.')
EXPECT_THROWS(lambda: cluster.execute("SELECT 1;", "all", {"exclude": ""}) , f'"" is not a valid keyword for option \'exclude\'')
EXPECT_THROWS(lambda: cluster.execute("SELECT 1;", "all", {"exclude": []}) , f'The array of instance addresses in option \'exclude\' cannot be empty.')
EXPECT_THROWS(lambda: cluster.execute("SELECT 1;", "all", {"exclude": "bar"}) , f'"bar" is not a valid keyword for option \'exclude\'')
EXPECT_THROWS(lambda: cluster.execute("SELECT 1;", "all", {"exclude": "all"}) , f'"all" is not a valid keyword for option \'exclude\'')

#@<> validate arguments: incorrect instances
WIPE_OUTPUT()
EXPECT_THROWS(lambda: cluster.execute("SELECT 1;", [f'{hostname}:{__mysql_sandbox_port5}']), f'Invalid instance specified: instance \'{hostname}:{__mysql_sandbox_port5}\' doesn\'t belong to the Cluster')
EXPECT_STDOUT_CONTAINS(f'The instance \'{hostname}:{__mysql_sandbox_port5}\' doesn\'t belong to Cluster \'cluster\'')

WIPE_OUTPUT()
EXPECT_THROWS(lambda: cluster.execute("SELECT 1;", [f'{hostname}:{__mysql_sandbox_port6}']), f'Invalid instance specified: unable to connect to instance \'{hostname}:{__mysql_sandbox_port6}\'')
EXPECT_STDOUT_CONTAINS(f'Unable to connect to instance \'{hostname}:{__mysql_sandbox_port6}\': Can\'t connect to MySQL server on \'{hostname}:{__mysql_sandbox_port6}\'')

WIPE_OUTPUT()
EXPECT_THROWS(lambda: cluster.execute("SELECT 1;", "all", {"exclude": [f'{hostname}:{__mysql_sandbox_port5}']}), f'Invalid instance specified: instance \'{hostname}:{__mysql_sandbox_port5}\' doesn\'t belong to the Cluster')
EXPECT_STDOUT_CONTAINS(f'The instance \'{hostname}:{__mysql_sandbox_port5}\' doesn\'t belong to Cluster \'cluster\'')

WIPE_OUTPUT()
EXPECT_THROWS(lambda: cluster.execute("SELECT 1;", "all", {"exclude": [f'{hostname}:{__mysql_sandbox_port6}']}), f'Invalid instance specified: unable to connect to instance \'{hostname}:{__mysql_sandbox_port6}\'')
EXPECT_STDOUT_CONTAINS(f'Unable to connect to instance \'{hostname}:{__mysql_sandbox_port6}\': Can\'t connect to MySQL server on \'{hostname}:{__mysql_sandbox_port6}\'')

#@<> test execution using keywords

WIPE_OUTPUT()
result = EXPECT_NO_THROWS(lambda: cluster.execute("SELECT 1;", "all"))
EXPECT_STDOUT_CONTAINS(f'Statement will be executed at: \'{hostname}:{__mysql_sandbox_port1}\', \'{hostname}:{__mysql_sandbox_port2}\', \'{hostname}:{__mysql_sandbox_port3}\', \'{hostname}:{__mysql_sandbox_port4}\'')
check_output(check_for_instance(result, 4, f'{hostname}:{__mysql_sandbox_port1}'), ["1"])
check_output(check_for_instance(result, 4, f'{hostname}:{__mysql_sandbox_port2}'), ["1"])
check_output(check_for_instance(result, 4, f'{hostname}:{__mysql_sandbox_port3}'), ["1"])
check_output(check_for_instance(result, 4, f'{hostname}:{__mysql_sandbox_port4}'), ["1"])

WIPE_OUTPUT()
result = EXPECT_NO_THROWS(lambda: cluster.execute("SELECT 1;", "a"))
EXPECT_STDOUT_CONTAINS(f'Statement will be executed at: \'{hostname}:{__mysql_sandbox_port1}\', \'{hostname}:{__mysql_sandbox_port2}\', \'{hostname}:{__mysql_sandbox_port3}\', \'{hostname}:{__mysql_sandbox_port4}\'')
check_output(check_for_instance(result, 4, f'{hostname}:{__mysql_sandbox_port1}'), ["1"])
check_output(check_for_instance(result, 4, f'{hostname}:{__mysql_sandbox_port2}'), ["1"])
check_output(check_for_instance(result, 4, f'{hostname}:{__mysql_sandbox_port3}'), ["1"])
check_output(check_for_instance(result, 4, f'{hostname}:{__mysql_sandbox_port4}'), ["1"])

WIPE_OUTPUT()
result = EXPECT_NO_THROWS(lambda: cluster.execute("SELECT 2;", "primary"))
EXPECT_STDOUT_CONTAINS(f'Statement will be executed at: \'{hostname}:{__mysql_sandbox_port1}\'')
check_output(check_for_instance(result, 1, f'{hostname}:{__mysql_sandbox_port1}'), ["2"])

WIPE_OUTPUT()
result = EXPECT_NO_THROWS(lambda: cluster.execute("SELECT 2;", "p"))
EXPECT_STDOUT_CONTAINS(f'Statement will be executed at: \'{hostname}:{__mysql_sandbox_port1}\'')
check_output(check_for_instance(result, 1, f'{hostname}:{__mysql_sandbox_port1}'), ["2"])

WIPE_OUTPUT()
result = EXPECT_NO_THROWS(lambda: cluster.execute("SELECT 3;", "secondaries"))
EXPECT_STDOUT_CONTAINS(f'Statement will be executed at: \'{hostname}:{__mysql_sandbox_port2}\', \'{hostname}:{__mysql_sandbox_port3}\'')
check_output(check_for_instance(result, 2, f'{hostname}:{__mysql_sandbox_port2}'), ["3"])
check_output(check_for_instance(result, 2, f'{hostname}:{__mysql_sandbox_port3}'), ["3"])

WIPE_OUTPUT()
result = EXPECT_NO_THROWS(lambda: cluster.execute("SELECT 3;", "s"))
EXPECT_STDOUT_CONTAINS(f'Statement will be executed at: \'{hostname}:{__mysql_sandbox_port2}\', \'{hostname}:{__mysql_sandbox_port3}\'')
check_output(check_for_instance(result, 2, f'{hostname}:{__mysql_sandbox_port2}'), ["3"])
check_output(check_for_instance(result, 2, f'{hostname}:{__mysql_sandbox_port3}'), ["3"])

WIPE_OUTPUT()
result = EXPECT_NO_THROWS(lambda: cluster.execute("SELECT 4;", "read-replicas"))
EXPECT_STDOUT_CONTAINS(f'Statement will be executed at: \'{hostname}:{__mysql_sandbox_port4}\'')
check_output(check_for_instance(result, 1, f'{hostname}:{__mysql_sandbox_port4}'), ["4"])

WIPE_OUTPUT()
result = EXPECT_NO_THROWS(lambda: cluster.execute("SELECT 5;", "rr"))
EXPECT_STDOUT_CONTAINS(f'Statement will be executed at: \'{hostname}:{__mysql_sandbox_port4}\'')
check_output(check_for_instance(result, 1, f'{hostname}:{__mysql_sandbox_port4}'), ["5"])

WIPE_OUTPUT()
result = EXPECT_NO_THROWS(lambda: cluster.execute("SELECT 'test1';", "all", {"exclude": "primary"}))
EXPECT_STDOUT_CONTAINS(f'Statement will be executed at: \'{hostname}:{__mysql_sandbox_port2}\', \'{hostname}:{__mysql_sandbox_port3}\', \'{hostname}:{__mysql_sandbox_port4}\'')
check_output(check_for_instance(result, 3, f'{hostname}:{__mysql_sandbox_port2}'), ["test1"])
check_output(check_for_instance(result, 3, f'{hostname}:{__mysql_sandbox_port3}'), ["test1"])
check_output(check_for_instance(result, 3, f'{hostname}:{__mysql_sandbox_port4}'), ["test1"])

WIPE_OUTPUT()
result = EXPECT_NO_THROWS(lambda: cluster.execute("SELECT 'test2';", "all", {"exclude": "p"}))
EXPECT_STDOUT_CONTAINS(f'Statement will be executed at: \'{hostname}:{__mysql_sandbox_port2}\', \'{hostname}:{__mysql_sandbox_port3}\', \'{hostname}:{__mysql_sandbox_port4}\'')
check_output(check_for_instance(result, 3, f'{hostname}:{__mysql_sandbox_port2}'), ["test2"])
check_output(check_for_instance(result, 3, f'{hostname}:{__mysql_sandbox_port3}'), ["test2"])
check_output(check_for_instance(result, 3, f'{hostname}:{__mysql_sandbox_port4}'), ["test2"])

WIPE_OUTPUT()
result = EXPECT_NO_THROWS(lambda: cluster.execute("SELECT 'test3';", "all", {"exclude": "s"}))
EXPECT_STDOUT_CONTAINS(f'Statement will be executed at: \'{hostname}:{__mysql_sandbox_port1}\', \'{hostname}:{__mysql_sandbox_port4}\'')
check_output(check_for_instance(result, 2, f'{hostname}:{__mysql_sandbox_port1}'), ["test3"])
check_output(check_for_instance(result, 2, f'{hostname}:{__mysql_sandbox_port4}'), ["test3"])

WIPE_OUTPUT()
result = EXPECT_NO_THROWS(lambda: cluster.execute("SELECT 'test4';", "all", {"exclude": "secondaries"}))
EXPECT_STDOUT_CONTAINS(f'Statement will be executed at: \'{hostname}:{__mysql_sandbox_port1}\', \'{hostname}:{__mysql_sandbox_port4}\'')
check_output(check_for_instance(result, 2, f'{hostname}:{__mysql_sandbox_port1}'), ["test4"])
check_output(check_for_instance(result, 2, f'{hostname}:{__mysql_sandbox_port4}'), ["test4"])

WIPE_OUTPUT()
result = EXPECT_NO_THROWS(lambda: cluster.execute("SELECT 'test5';", "all", {"exclude": "rr"}))
EXPECT_STDOUT_CONTAINS(f'Statement will be executed at: \'{hostname}:{__mysql_sandbox_port1}\', \'{hostname}:{__mysql_sandbox_port2}\', \'{hostname}:{__mysql_sandbox_port3}\'')
check_output(check_for_instance(result, 3, f'{hostname}:{__mysql_sandbox_port1}'), ["test5"])
check_output(check_for_instance(result, 3, f'{hostname}:{__mysql_sandbox_port2}'), ["test5"])
check_output(check_for_instance(result, 3, f'{hostname}:{__mysql_sandbox_port3}'), ["test5"])

WIPE_OUTPUT()
result = EXPECT_NO_THROWS(lambda: cluster.execute("SELECT 'test6';", "all", {"exclude": "read-replicas"}))
EXPECT_STDOUT_CONTAINS(f'Statement will be executed at: \'{hostname}:{__mysql_sandbox_port1}\', \'{hostname}:{__mysql_sandbox_port2}\', \'{hostname}:{__mysql_sandbox_port3}\'')
check_output(check_for_instance(result, 3, f'{hostname}:{__mysql_sandbox_port1}'), ["test6"])
check_output(check_for_instance(result, 3, f'{hostname}:{__mysql_sandbox_port2}'), ["test6"])
check_output(check_for_instance(result, 3, f'{hostname}:{__mysql_sandbox_port3}'), ["test6"])

#@<> test empty list use case

cluster.remove_instance(__sandbox_uri4)

WIPE_OUTPUT()
result = EXPECT_NO_THROWS(lambda: cluster.execute("SELECT 1;", "rr"))
EXPECT_STDOUT_CONTAINS("With the specified combination of parameters and options, the list of target instances, where the command should be executed, is empty.")
EXPECT_EQ(result, None)

cluster.add_replica_instance(__sandbox_uri4)

#@<> test multi-primary mode

cluster.switch_to_multi_primary_mode()

WIPE_OUTPUT()
EXPECT_THROWS(lambda: cluster.execute("SELECT 1;", "s"), "Unsupported keyword on a multi primary topology")
EXPECT_STDOUT_CONTAINS("The keyword 'secondaries' isn't valid on a multi primary Cluster.")

WIPE_OUTPUT()
EXPECT_THROWS(lambda: cluster.execute("SELECT 1;", "a", {"exclude": "s"}), "Unsupported keyword on a multi primary topology")
EXPECT_STDOUT_CONTAINS("The keyword 'secondaries' isn't valid on a multi primary Cluster.")

WIPE_OUTPUT()
result = EXPECT_NO_THROWS(lambda: cluster.execute("SELECT 'multi primary';", "all"))
EXPECT_STDOUT_CONTAINS(f'Statement will be executed at: \'{hostname}:{__mysql_sandbox_port1}\', \'{hostname}:{__mysql_sandbox_port2}\', \'{hostname}:{__mysql_sandbox_port3}\', \'{hostname}:{__mysql_sandbox_port4}\'')
check_output(check_for_instance(result, 4, f'{hostname}:{__mysql_sandbox_port1}'), ["multi primary"])
check_output(check_for_instance(result, 4, f'{hostname}:{__mysql_sandbox_port2}'), ["multi primary"])
check_output(check_for_instance(result, 4, f'{hostname}:{__mysql_sandbox_port3}'), ["multi primary"])
check_output(check_for_instance(result, 4, f'{hostname}:{__mysql_sandbox_port3}'), ["multi primary"])

WIPE_OUTPUT()
result = EXPECT_NO_THROWS(lambda: cluster.execute("SELECT 'multi primary';", "p"))
EXPECT_STDOUT_CONTAINS(f'Statement will be executed at: \'{hostname}:{__mysql_sandbox_port1}\', \'{hostname}:{__mysql_sandbox_port2}\', \'{hostname}:{__mysql_sandbox_port3}\'')
check_output(check_for_instance(result, 3, f'{hostname}:{__mysql_sandbox_port1}'), ["multi primary"])
check_output(check_for_instance(result, 3, f'{hostname}:{__mysql_sandbox_port2}'), ["multi primary"])
check_output(check_for_instance(result, 3, f'{hostname}:{__mysql_sandbox_port3}'), ["multi primary"])

WIPE_OUTPUT()
result = EXPECT_NO_THROWS(lambda: cluster.execute("SELECT 'multi primary';", "a", {"exclude": "p"}))
EXPECT_STDOUT_CONTAINS(f'Statement will be executed at: \'{hostname}:{__mysql_sandbox_port4}\'')
check_output(check_for_instance(result, 1, f'{hostname}:{__mysql_sandbox_port4}'), ["multi primary"])

cluster.switch_to_single_primary_mode()

#@<> test execution using instance addresses

WIPE_OUTPUT()
EXPECT_NO_THROWS(lambda: cluster.execute("SELECT 1;", [f'{hostname}:{__mysql_sandbox_port1}', f'{hostname}:{__mysql_sandbox_port3}', f'{hostname}:{__mysql_sandbox_port4}']))
EXPECT_STDOUT_CONTAINS(f'Statement will be executed at: \'{hostname}:{__mysql_sandbox_port1}\', \'{hostname}:{__mysql_sandbox_port3}\', \'{hostname}:{__mysql_sandbox_port4}\'')

WIPE_OUTPUT()
EXPECT_NO_THROWS(lambda: cluster.execute("SELECT 1;", [f'{hostname}:{__mysql_sandbox_port1}', f'{hostname}:{__mysql_sandbox_port3}', f'{hostname}:{__mysql_sandbox_port4}'], {"exclude": "s"}))
EXPECT_STDOUT_CONTAINS(f'Statement will be executed at: \'{hostname}:{__mysql_sandbox_port1}\', \'{hostname}:{__mysql_sandbox_port4}\'')

WIPE_OUTPUT()
EXPECT_NO_THROWS(lambda: cluster.execute("SELECT 1;", [f'{hostname}:{__mysql_sandbox_port1}', f'{hostname}:{__mysql_sandbox_port3}', f'{hostname}:{__mysql_sandbox_port4}'], {"exclude": "rr"}))
EXPECT_STDOUT_CONTAINS(f'Statement will be executed at: \'{hostname}:{__mysql_sandbox_port1}\', \'{hostname}:{__mysql_sandbox_port3}\'')

WIPE_OUTPUT()
EXPECT_NO_THROWS(lambda: cluster.execute("SELECT 1;", [f'{hostname}:{__mysql_sandbox_port1}', f'{hostname}:{__mysql_sandbox_port3}', f'{hostname}:{__mysql_sandbox_port4}'], {"exclude": [f'{hostname}:{__mysql_sandbox_port1}']}))
EXPECT_STDOUT_CONTAINS(f'Statement will be executed at: \'{hostname}:{__mysql_sandbox_port3}\', \'{hostname}:{__mysql_sandbox_port4}\'')

#@<> check errors on output

result = EXPECT_NO_THROWS(lambda: cluster.execute("SELECT * FROM;", "all"))
EXPECT_STDOUT_CONTAINS(f'Statement will be executed at: \'{hostname}:{__mysql_sandbox_port1}\', \'{hostname}:{__mysql_sandbox_port2}\', \'{hostname}:{__mysql_sandbox_port3}\', \'{hostname}:{__mysql_sandbox_port4}\'')
check_error(check_for_instance(result, 4, f'{hostname}:{__mysql_sandbox_port1}'), "mysql", None)
check_error(check_for_instance(result, 4, f'{hostname}:{__mysql_sandbox_port2}'), "mysql", None)
check_error(check_for_instance(result, 4, f'{hostname}:{__mysql_sandbox_port3}'), "mysql", None)
check_error(check_for_instance(result, 4, f'{hostname}:{__mysql_sandbox_port4}'), "mysql", None)

#@<> when there are unreachable instances

testutil.kill_sandbox(__mysql_sandbox_port3)

result = EXPECT_NO_THROWS(lambda: cluster.execute("SELECT version() as foo;", "all"))
check_output(check_for_instance(result, 4, f'{hostname}:{__mysql_sandbox_port1}'), ["foo"])
check_output(check_for_instance(result, 4, f'{hostname}:{__mysql_sandbox_port2}'), ["foo"])
check_error(check_for_instance(result, 4, f'{hostname}:{__mysql_sandbox_port3}'), "mysqlsh", "Instance isn't reachable.")
check_output(check_for_instance(result, 4, f'{hostname}:{__mysql_sandbox_port4}'), ["foo"])

WIPE_OUTPUT()
EXPECT_THROWS(lambda: cluster.execute("SELECT 1;", [f'{hostname}:{__mysql_sandbox_port3}']), f'Invalid instance specified: unable to connect to instance \'{hostname}:{__mysql_sandbox_port3}\'')
EXPECT_STDOUT_CONTAINS(f'Unable to connect to instance \'{hostname}:{__mysql_sandbox_port3}\': Can\'t connect to MySQL server on \'{hostname}:{__mysql_sandbox_port3}\'')

testutil.start_sandbox(__mysql_sandbox_port3)
cluster.rejoin_instance(__sandbox_uri3)

#@<> check dryRun
result = EXPECT_NO_THROWS(lambda: cluster.execute("SELECT version() as foo;", "all", {"dryRun": True}))

check_warning(check_for_instance(result, 4, f'{hostname}:{__mysql_sandbox_port1}'), "dry run execution")
check_warning(check_for_instance(result, 4, f'{hostname}:{__mysql_sandbox_port2}'), "dry run execution")
check_warning(check_for_instance(result, 4, f'{hostname}:{__mysql_sandbox_port3}'), "dry run execution")
check_warning(check_for_instance(result, 4, f'{hostname}:{__mysql_sandbox_port4}'), "dry run execution")

#@<> check parallelism and timeout {not __replaying}

try:
    num_cpus = multiprocessing.cpu_count()
    print(f'Number of CPUs available: {num_cpus}')
    start = timer()
    EXPECT_NO_THROWS(lambda: cluster.execute("SELECT sleep(2);", "all"))
    elapsed = round(timer() - start)
    print(f'Elapsed time: {elapsed}')
    if num_cpus >= 4:
        EXPECT_EQ(2, elapsed)
    else:
        EXPECT_EQ(int(math.ceil(4 / num_cpus)), elapsed)
except Exception as e:
    print(f'Unable to read CPU count, skipping test: {e}')

try:
    num_cpus = multiprocessing.cpu_count()
    print(f'Number of CPUs available: {num_cpus}')
    if num_cpus >= 4:
        start = timer()
        EXPECT_NO_THROWS(lambda: cluster.execute("SELECT sleep(2);", "all", {"timeout": 1}))
        elapsed = round(timer() - start)
        print(f'Elapsed time: {elapsed}')
        EXPECT_EQ(1, elapsed)
    else:
        print(f'Unable to test query timeout: need at least 4 CPUs but currently have: {num_cpus}')
except Exception as e:
    print(f'Unable to read CPU count, skipping test: {e}')

#@<> check cancel {not __dbug_off and not __replaying}

shell.options.useWizards = True;
testutil.dbug_set("+d,execute_cancel")

result = EXPECT_NO_THROWS(lambda: cluster.execute("SELECT sleep(10);", "all"))

testutil.dbug_set("")
shell.options.useWizards = False;

#@<> cleanup
cluster.disconnect()
session.close()

testutil.destroy_sandbox(__mysql_sandbox_port1)
testutil.destroy_sandbox(__mysql_sandbox_port2)
testutil.destroy_sandbox(__mysql_sandbox_port3)
testutil.destroy_sandbox(__mysql_sandbox_port4)
testutil.destroy_sandbox(__mysql_sandbox_port5)
