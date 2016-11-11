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
  while not condition() and waiting < timeout:
    time.sleep(wait_interval)
    waiting = waiting + 1


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

def wait_slave_online():
  global recov_cluster
  global recov_master_uri
  global recov_slave_uri
  
  full_status = recov_cluster.status()
  slave_status = full_status.defaultReplicaSet.topology[recov_master_uri].leaves[recov_slave_uri].status
  
  print "---> %s: %s" % (recov_slave_uri, slave_status)
  return slave_status == "ONLINE"

def check_slave_online(cluster, master_uri, slave_uri):
  global recov_cluster
  global recov_master_uri
  global recov_slave_uri
  
  recov_cluster = cluster
  recov_master_uri = master_uri
  recov_slave_uri = slave_uri
  
  wait(60, 1, wait_slave_online)
  
  recov_cluster = None
