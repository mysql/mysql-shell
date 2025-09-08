from __future__ import print_function
import os.path


def validate_crud_functions(crud, expected):
    actual = dir(crud)

    # Ensures expected functions are on the actual list
    missing = []
    for exp_funct in expected:
        try:
            pos = actual.index(exp_funct)
            actual.remove(exp_funct)
        except:
            missing.append(exp_funct)

    if len(missing) == 0:
        print("All expected functions are available\n")
    else:
        print("Missing Functions:", missing)

# help is ignored cuz it's always available
    if len(actual) == 0 or (len(actual) == 1 and actual[0] == "help"):
        print("No additional functions are available\n")
    else:
        print("Extra Functions:", actual)


def validate_members(object, expected_members):
  all_members = dir(object)

  # Remove the python built in members
  members = []
  for member in all_members:
    if not member.startswith('__'):
      members.append(member)

  missing = []
  for expected in expected_members:
    try:
      index = members.index(expected)
      members.remove(expected)
    except:
      missing.append(expected)

  errors = []
  error = ""
  if len(members):
    error = "Unexpected Members: %s" % ', '.join(members)
    errors.append(error)

  error = ""
  if len(missing):
    error = "Missing Members: %s" % ', '.join(missing)
    errors.append(error)

  if len(errors):
    testutil.fail(', '.join(errors))

def ensure_schema_does_not_exist(session, name):
    try:
        schema = session.get_schema(name)
        session.drop_schema(name)
    except:
        # Nothing happens, it means the schema did not exist
        pass

def getSchemaFromList(schemas, name):
    for schema in schemas:
        if schema.name == name:
            return schema

    return None


import time


def wait(timeout, wait_interval, condition):
    waiting = 0
    res = condition()
    while not res and waiting < timeout:
        time.sleep(wait_interval)
        waiting = waiting + 1
        res = condition()
    return res


# cb_params is an array with whatever parameters required by the
# called callback, this way we can pass params to that function
# right from the caller of retry
def retry(attempts, wait_interval, callback, cb_params=None):
    attempt = 1

    res = callback(cb_params)

    while not res and attempt < attempts:
        print("Attempt %s of %s failed, retrying..." % (attempt + 1, attempts))
        time.sleep(wait_interval)
        attempt = attempt + 1
        res = callback(cb_params)

    return res


ro_session = None
from mysqlsh import mysql as ro_module


def wait_super_read_only_done():
    global ro_session

    super_read_only = ro_session.run_sql(
        'select @@super_read_only').fetch_one()[0]

    print("---> Super Read Only = %s" % super_read_only)

    return super_read_only == "0"


def check_super_read_only_done(connection):
    global ro_session

    ro_session = ro_module.get_classic_session(connection)
    wait(60, 1, wait_super_read_only_done)
    ro_session.close()


recov_cluster = None
recov_master_uri = None
recov_slave_uri = None
recov_state_list = None


def _check_slave_state():
    global recov_cluster
    global recov_slave_uri
    global recov_state_list

    full_status = recov_cluster.status()
    slave_status = full_status.defaultReplicaSet.topology[
        recov_slave_uri].status

    print("--->%s: %s" % (recov_slave_uri, slave_status))

    ret_val = False
    for state in recov_state_list:
        if state == slave_status:
            ret_val = True
            print("Done!")
            break

    return ret_val


def wait_slave_state(cluster, slave_uri, states):
    global recov_cluster
    global recov_slave_uri
    global recov_state_list

    recov_cluster = cluster
    recov_slave_uri = slave_uri

    if type(states) is list:
        recov_state_list = states
    else:
        recov_state_list = [states]

    print("WAITING for %s to be in one of these states: %s" % (slave_uri,
                                                               states))

    wait(60, 1, _check_slave_state)

    recov_cluster = None


def wait_sandbox(timeout, wait_interval, condition, sandbox_port):
    waiting = 0
    res = condition([sandbox_port])
    while not res and waiting < timeout:
        time.sleep(wait_interval)
        waiting = waiting + 1
        res = condition([sandbox_port])
    return res


def check_sandbox_has_metadata():
    sandbox_has_metadata = session.run_sql(
        "SELECT count(*) FROM information_schema.TABLES WHERE (TABLE_SCHEMA = 'mysql_innodb_cluster_metadata') AND (TABLE_NAME = 'instances')"
    ).fetch_one()[0]

    print("---> count(*) sandbox has metadata = %s" % sandbox_has_metadata)

    return sandbox_has_metadata == "1"


def check_sandbox_in_metadata(instance_port):
    sandbox_count_metadata = session.run_sql(
        "select count(*) from mysql_innodb_cluster_metadata.instances where instance_name = 'localhost:{0}'"
        .format(instance_port)).fetch_one()[0]

    print("---> count(*) sandbox in metadata = %s" % sandbox_count_metadata)

    return sandbox_count_metadata == "1"


def wait_sandbox_in_metadata(instance_port):
    connected = connect_to_sandbox([instance_port])
    if (connected):
        wait(120, 1, check_sandbox_has_metadata)
        wait_sandbox(120, 1, check_sandbox_in_metadata, instance_port)
        session.close()


# Smart deployment routines


def connect_to_sandbox(params):
    port = params[0]
    connected = False
    try:
        shell.connect({
            'scheme': 'mysql',
            'user': 'root',
            'password': 'root',
            'host': 'localhost',
            'port': port
        })
        print('connected to sandbox at %s' % port)
        connected = True
    except Exception as err:
        print('failed connecting to sandbox at %s : %s' % (port, err.args[0]))

    return connected


def start_sandbox(params):
    port = params[0]
    started = False

    options = {}
    if __sandbox_dir != '':
        options['sandboxDir'] = __sandbox_dir

    try:
        dba.start_sandbox_instance(port, options)
        started = True
        print('started sandbox at %s' % port)
    except Exception as err:
        started = False
        print('failed starting sandbox at %s : %s' % (port, err.args[0]))

        if err.args[0].find(
                "Cannot start MySQL sandbox for the given port because it does not exist."
        ) != -1:
            raise

    return started

# Function to cleanup (if deployed) or reset the sandbox.
def cleanup_or_reset_sandbox(port, deployed):
    if deployed:
        cleanup_sandbox(port)
    else:
        reset_or_deploy_sandbox(port)


# Function to restart the server (after being stopped).
def try_restart_sandbox(port):
    options = {}
    if __sandbox_dir != '':
        options['sandboxDir'] = __sandbox_dir

    # Restart sandbox instance to use the new option.
    print('Try starting sandbox at: %s' % port)

    def try_start():
        try:
            dba.start_sandbox_instance(port, options)

            # Try to establish a connection to the instance to make sure
            # it's up and running.
            shell.connect({
                'host': localhost,
                'port': port,
                'user': 'root',
                'password': 'root'
            })
            session.close()

            print("succeeded")
            return True
        except Exception as err:
            print("failed: %s" % str(err))
            return False

    if wait(10, 1, try_start):
        print('Restart succeeded at: %s' % port)
    else:
        print('Restart failed at: %s' % port)


# Function to delete sandbox (only succeed after full server shutdown).
def try_delete_sandbox(port, sandbox_dir):
    options = {}
    if sandbox_dir:
        options['sandboxDir'] = sandbox_dir

    # Restart sandbox instance to use the new option.
    print('Try deleting sandbox at: %s' % port)

    def try_delete():
        try:
            dba.delete_sandbox_instance(port, options)
            print("succeeded")
            return True
        except Exception as err:
            print("failed: %s" % str(err))
            return False

    if wait(10, 1, try_delete):
        print('Delete succeeded at: %s' % port)
    else:
        print('Delete failed at: %s' % port)

def EXPECT_OUTPUT_CONTAINS(text, note=None):
    out = testutil.fetch_captured_stdout(False)
    err = testutil.fetch_captured_stderr(False)
    if out.find(text) == -1 and err.find(text) == -1:
        if not note:
            note = __caller_context()
        context = "<b>Context:</b> " + __test_context + "\n<red>Missing output:</red> " + text + \
                  "\n<yellow>Actual stdout:</yellow> " + out + \
                  "\n<yellow>Actual stderr:</yellow> " + err
        testutil.fail(context)
