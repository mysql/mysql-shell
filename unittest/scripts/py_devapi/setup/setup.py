import os.path

def validate_crud_functions(crud, expected):
	actual = crud.__members__

	# Ensures expected functions are on the actual list
	missing = []
	for exp_funct in expected:
		try:
			pos = actual.index(exp_funct)
			actual.remove(exp_funct)
		except:
			missing.append(exp_funct)

	if len(missing) == 0:
		print ("All expected functions are available\n")
	else:
		print "Missing Functions:", missing

	if len(actual) == 0:
		print "No additional functions are available\n"
	else:
		print "Extra Functions:", actual

def ensure_schema_does_not_exist(session, name):
	try:
		schema = session.get_schema(name)
		session.drop_schema(name)
	except:
		# Nothing happens, it means the schema did not exist
		pass

def validateMember(memberList, member):
	index = -1
	try:
		index = memberList.index(member)
	except:
		pass

	if index != -1:
		print member + ": OK\n"
	else:
		print member + ": Missing\n"

def validateNotMember(memberList, member):
	index = -1
	try:
		index = memberList.index(member)
	except:
		pass

	if index != -1:
		print member + ": Unexpected\n"
	else:
		print member + ": OK\n"

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
def retry(attempts, wait_interval, callback, cb_params = None):
  attempt = 1

  res = callback(cb_params)

  while not res and attempt < attempts:
    print "Attempt %s of %s failed, retrying..." % (attempt + 1, attempts)
    time.sleep(wait_interval)
    attempt = attempt + 1
    res = callback(cb_params)

  return res

ro_session = None;
from mysqlsh import mysql as ro_module;
def wait_super_read_only_done():
  global ro_session

  super_read_only = ro_session.run_sql('select @@super_read_only').fetch_one()[0]

  print "---> Super Read Only = %s" % super_read_only

  return super_read_only == "0"

def check_super_read_only_done(connection):
  global ro_session

  ro_session = ro_module.get_classic_session(connection)
  wait(60, 1, wait_super_read_only_done)
  ro_session.close()

recov_cluster = None
recov_master_uri = None
recov_slave_uri = None
recov_state_list = None;

def _check_slave_state():
  global recov_cluster
  global recov_slave_uri
  global recov_state_list

  full_status = recov_cluster.status()
  slave_status = full_status.defaultReplicaSet.topology[recov_slave_uri].status

  print "--->%s: %s" % (recov_slave_uri, slave_status)

  ret_val = False
  for state in recov_state_list:
    if state == slave_status:
      ret_val = True
      print "Done!"
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

  print "WAITING for %s to be in one of these states: %s" % (slave_uri, states)

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
  sandbox_has_metadata = session.run_sql("SELECT count(*) FROM information_schema.TABLES WHERE (TABLE_SCHEMA = 'mysql_innodb_cluster_metadata') AND (TABLE_NAME = 'instances')").fetch_one()[0]

  print "---> count(*) sandbox has metadata = %s" % sandbox_has_metadata

  return sandbox_has_metadata == "1"

def check_sandbox_in_metadata(instance_port):
  sandbox_count_metadata = session.run_sql("select count(*) from mysql_innodb_cluster_metadata.instances where instance_name = 'localhost:{0}'".format(instance_port)).fetch_one()[0]

  print "---> count(*) sandbox in metadata = %s" % sandbox_count_metadata

  return sandbox_count_metadata == "1"

def wait_sandbox_in_metadata(instance_port):
  connected = connect_to_sandbox([instance_port]);
  if (connected):
    wait(120, 1, check_sandbox_has_metadata)
    wait_sandbox(120, 1, check_sandbox_in_metadata, instance_port)
    session.close()

# Smart deployment routines

def connect_to_sandbox(params):
  port = params[0]
  connected = False
  try:
    shell.connect({'scheme': 'mysql', 'user':'root', 'password':'root', 'host':'localhost', 'port':port})
    print 'connected to sandbox at %s' % port
    connected = True
  except Exception, err:
    print 'failed connecting to sandbox at %s : %s' % (port, err.args[0])

  return connected;

def start_sandbox(params):
  port = params[0]
  started = False

  options = {}
  if __sandbox_dir != '':
    options['sandboxDir'] = __sandbox_dir

  try:
    dba.start_sandbox_instance(port, options)
    started = True
    print 'started sandbox at %s' % port
  except Exception, err:
    started = False
    print 'failed starting sandbox at %s : %s' % (port, err.args[0])

    if err.args[0].find("Cannot start MySQL sandbox for the given port because it does not exist.") != -1:
      raise;


  return started;

def cleanup_sandbox(port):
    print 'Stopping the sandbox at %s to delete it...' % port
    try:
      stop_options = {}
      stop_options['password'] = 'root'
      if __sandbox_dir != '':
        stop_options['sandboxDir'] = __sandbox_dir

      dba.stop_sandbox_instance(port, stop_options)
    except Exception, err:
      print err.args[0]
      pass

      options = {}
      if __sandbox_dir != '':
        options['sandboxDir'] = __sandbox_dir

      print 'Try deleting sandbox at: %s' % port
      def try_delete():
        try:
          dba.delete_sandbox_instance(port, options)
          print "succeeded"
          return True
        except Exception, err:
          print "failed: %s" % str(err)
          return False

      if wait(10, 1, try_delete):
        print 'Delete succeeded at: %s' % port
      else:
        print 'Delete failed at: %s' % port

def reset_or_deploy_sandbox(port):
  deployed_here = False

  # Checks if the sandbox is up and running
  connected = connect_to_sandbox([port])

  start = False
  reboot = False
  delete = False

  if (connected):
    # Verifies whether the sandbox requires to be rebooted (non standalone)
    try:
      print 'verifying for cluster existence...'
      c = dba.get_cluster()
      print 'cluster found, reboot required...'
      reboot = True
    except Exception, err:
      print "unable to get cluster from sandbox at %s: %s" % (port, err.args[0])

      # Reboot is required if it is not a standalone instance
      if err.args[0].find("This function is not available through a session to a standalone instance") == -1:
        reboot = True
  else:
    start = True
    deployed_here = True


  # If reboot is needed, stops the sandbox first
  if reboot:
    connected = False

    print 'Stopping sandbox at: %s' % port

    try:
      stop_options = {}
      stop_options['password'] = 'root'
      if __sandbox_dir != '':
        stop_options['sandboxDir'] = __sandbox_dir

      dba.stop_sandbox_instance(port, stop_options)
    except Exception, err:
      print err.args[0]
      pass

    start = True

  # Start attempt is done either for reboot or if
  # connection was unsuccessful
  if start:
    # Make sure the server has finished shutting down
    try:
      kill_options = {}
      if __sandbox_dir != '':
        kill_options['sandboxDir'] = __sandbox_dir

      dba.kill_sandbox_instance(port, kill_options)
    except Exception, err:
      print err.args[0]
      pass

    print 'Starting sandbox at: %s' % port

    try:
      started = retry(10, 2, start_sandbox, [port]);

      if started:
        print 'Connecting to sandbox at: %s' % port
        connected = retry(10, 2, connect_to_sandbox, [port])
    except Exception, err:
      print err.args[0]
      pass

    if not connected:
      delete = True

  # delete is needed if the start failed
  if delete:
    cleanup_sandbox(port)

  # If the instance is up and running, we just drop the metadata
  if connected:
    print '**Recycling instance %d\n' % port
    print 'Dropping metadata...'
    session.run_sql('SET GLOBAL super_read_only = 0'); # (BUG#26422638)
    session.run_sql('set sql_log_bin = 0')
    session.run_sql('drop schema if exists mysql_innodb_cluster_metadata')

    # Clean transactions and binary logs (must stop Group Replication first).
    # NOTE: Group Replication is not started again after clean-up.
    try:
        print 'Stopping Group Replication...'
        session.run_sql('STOP GROUP_REPLICATION')
    except Exception, err:
        print 'Error stopping Group Replication at %s: %s' % (port, err.args[0])

    print 'Remove binary logs and clean GTIDs sets (RESET MASTER)...'
    session.run_sql('RESET MASTER')

    session.run_sql('set sql_log_bin = 1')
    session.close()

  # Otherwise a full deployment is done
  else:
    print '**Deploying instance %d\n' % port
    print 'Deploying instance'

    options = {}
    if __sandbox_dir != '':
      options['sandboxDir'] = __sandbox_dir

    options['password'] = 'root'
    options['allowRootFrom'] = '%'
    options['mysqldOptions'] = ["innodb_log_file_size=4M"]
    dba.deploy_sandbox_instance(port, options)

  print '**End of sandbox handling\n'

  if os.path.exists(__sandbox_share):
    return False
  else:
    return deployed_here

def reset_or_deploy_sandboxes():
  deploy1 = reset_or_deploy_sandbox(__mysql_sandbox_port1)
  deploy2 = reset_or_deploy_sandbox(__mysql_sandbox_port2)
  deploy3 = reset_or_deploy_sandbox(__mysql_sandbox_port3)

  return deploy1 or deploy2 or deploy3

def cleanup_sandboxes(deployed_here):
  if deployed_here:
    cleanup_sandbox(__mysql_sandbox_port1)
    cleanup_sandbox(__mysql_sandbox_port2)
    cleanup_sandbox(__mysql_sandbox_port3)

# Operation that retries adding an instance to a cluster
# 3 retries are done on each case, expectation is that the addition
# is done on the first attempt, however, we have detected some OS
# delays that cause it to fail, that's why the retry logic
def add_instance_to_cluster(cluster, port, label = None):
  global add_instance_options
  add_instance_options['port'] = port

  labeled = False
  if not label is None:
    add_instance_extra_opts['label'] = label
    labeled = True

  attempt = 0
  success = False
  while attempt < 3 and not success:
    try:
      cluster.add_instance(add_instance_options, add_instance_extra_opts)
      print "Instance added successfully..."
      success = True
    except Exception, err:
      attempt = attempt + 1
      print "Failed adding instance on attempt %s" % attempt
      print str(err)
      print "Waiting 5 seconds for next attempt"
      time.sleep(5)

  if labeled:
    del add_instance_extra_opts['label']

  if not success:
    raise Exception('Failed adding instance : %s, %s' % add_instance_options, add_instance_extra_opts)


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
    print 'Try starting sandbox at: %s' % port
    def try_start():
        try:
            dba.start_sandbox_instance(port, options)

            # Try to establish a connection to the instance to make sure
            # it's up and running.
            shell.connect({'host': localhost, 'port': port, 'user': 'root', 'password': 'root'})
            session.close();

            print "succeeded"
            return True
        except Exception, err:
            print "failed: %s" % str(err)
            return False

    if wait(10, 1, try_start):
        print 'Restart succeeded at: %s' % port
    else:
        print 'Restart failed at: %s' % port


# Function to delete sandbox (only succeed after full server shutdown).
def try_delete_sandbox(port, sandbox_dir):
  options = {}
  if sandbox_dir:
    options['sandboxDir'] = sandbox_dir

  # Restart sandbox instance to use the new option.
  print 'Try deleting sandbox at: %s' % port
  def try_delete():
    try:
      dba.delete_sandbox_instance(port, options)
      print "succeeded"
      return True
    except Exception, err:
      print "failed: %s" % str(err)
      return False

  if wait(10, 1, try_delete):
    print 'Delete succeeded at: %s' % port
  else:
    print 'Delete failed at: %s' % port

# Check if the instance (sandbox) on the given port has SSL enabled and update
# the global SSL values used by the test scripts accordingly.
def update_have_ssl(port):
    # Check the instance SSL support
    res = False
    connected = connect_to_sandbox([port])
    if (connected):
        result = session.run_sql("SELECT @@GLOBAL.have_ssl")
        row = result.fetch_one()
        value = row[0]
        if (value == 'YES'):
            res = True
        session.close()
    print("Value of 'have_ssl' for instance on " + str(port) + ": " + value)
    # Update SSL global values used by test scripts
    global __have_ssl
    global add_instance_extra_opts
    global __ssl_mode
    __have_ssl = res
    if __have_ssl:
        add_instance_extra_opts = {'memberSslMode': 'REQUIRED'}
        __ssl_mode = 'REQUIRED'
    else:
        add_instance_extra_opts = {'memberSslMode': 'DISABLED'}
        __ssl_mode = 'DISABLED'
