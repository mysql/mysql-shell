#@ {__os_type != "macos"}

from timeit import default_timer as timer

#@<> helpers & constants
uri = "root:root@192.168.255.255:4444"
mysql_uri = f"mysql://{uri}"
mysqlx_uri = f"mysqlx://{uri}"

def execution_time(fun):
  start = timer()
  EXPECT_THROWS(fun, "connect")
  end = timer()
  return end - start

def EXPECT_EXECUTION_TIME(time, fun):
  EXPECT_DELTA(time, 0.1, execution_time(fun))

#@<> WL14698-ET_1 - classic protocol
# default value of the "connectTimeout" is 10 seconds
EXPECT_EXECUTION_TIME(10, lambda: shell.connect(mysql_uri))

# integer value
shell.options["connectTimeout"] = 2
EXPECT_EXECUTION_TIME(2, lambda: shell.connect(mysql_uri))

# fractional values
shell.options["connectTimeout"] = 0.5
EXPECT_EXECUTION_TIME(1, lambda: shell.connect(mysql_uri))

shell.options["connectTimeout"] = 1.1
EXPECT_EXECUTION_TIME(2, lambda: shell.connect(mysql_uri))

# connect-timeout (ms) in URI should overwrite the global option
shell.options["connectTimeout"] = 2
EXPECT_EXECUTION_TIME(1, lambda: shell.connect(f"{mysql_uri}?connect-timeout=1000"))

# reset the value
\option --unset connectTimeout

#@<> WL14698-ET_1 - X protocol
# default value of the "connectTimeout" is 10 seconds
EXPECT_EXECUTION_TIME(10, lambda: shell.connect(mysqlx_uri))

# integer value
shell.options["connectTimeout"] = 2
EXPECT_EXECUTION_TIME(2, lambda: shell.connect(mysqlx_uri))

# fractional values
shell.options["connectTimeout"] = 0.5
EXPECT_EXECUTION_TIME(0.5, lambda: shell.connect(mysqlx_uri))

shell.options["connectTimeout"] = 1.1
EXPECT_EXECUTION_TIME(1.1, lambda: shell.connect(mysqlx_uri))

# connect-timeout (ms) in URI should overwrite the global option
shell.options["connectTimeout"] = 2
EXPECT_EXECUTION_TIME(1, lambda: shell.connect(f"{mysqlx_uri}?connect-timeout=1000"))

# reset the value
\option --unset connectTimeout

#@<> AdminAPI tests - setup
testutil.deploy_sandbox(__mysql_sandbox_port1, 'root', {'report_host': hostname})

shell.connect(__sandbox_uri1)
cluster = dba.create_cluster("test", {"ipAllowlist": "127.0.0.1," + hostname_ip})

#@<> WL14698-ET_2 - classic protocol
# default value of the "dba.connectTimeout" is 5 seconds, set "connectTimeout" to something else to make sure it's not used
shell.options["connectTimeout"] = 6
EXPECT_EXECUTION_TIME(5, lambda: cluster.add_instance(mysql_uri))

# integer value
shell.options["dba.connectTimeout"] = 2
EXPECT_EXECUTION_TIME(2, lambda: cluster.add_instance(mysql_uri))

# fractional values
shell.options["dba.connectTimeout"] = 0.5
EXPECT_EXECUTION_TIME(1, lambda: cluster.add_instance(mysql_uri))

shell.options["dba.connectTimeout"] = 1.1
EXPECT_EXECUTION_TIME(2, lambda: cluster.add_instance(mysql_uri))

# connect-timeout (ms) in URI should overwrite the global option
shell.options["dba.connectTimeout"] = 2
EXPECT_EXECUTION_TIME(1, lambda: cluster.add_instance(f"{mysql_uri}?connect-timeout=1000"))

# reset the values
\option --unset connectTimeout
\option --unset dba.connectTimeout

#@<> WL14698-ET_2 - X protocol - not supported by AAPI

#@<> AdminAPI tests - cleanup
session.close()
testutil.destroy_sandbox(__mysql_sandbox_port1)

#@<> WL14698-ET_3 - classic protocol
# default value of the "connectTimeout" is 10 seconds
EXPECT_EXECUTION_TIME(10, lambda: util.check_for_server_upgrade(mysql_uri))

# integer value
shell.options["connectTimeout"] = 2
EXPECT_EXECUTION_TIME(2, lambda: util.check_for_server_upgrade(mysql_uri))

# fractional values
shell.options["connectTimeout"] = 0.5
EXPECT_EXECUTION_TIME(1, lambda: util.check_for_server_upgrade(mysql_uri))

shell.options["connectTimeout"] = 1.1
EXPECT_EXECUTION_TIME(2, lambda: util.check_for_server_upgrade(mysql_uri))

# connect-timeout (ms) in URI should overwrite the global option
shell.options["connectTimeout"] = 2
EXPECT_EXECUTION_TIME(1, lambda: util.check_for_server_upgrade(f"{mysql_uri}?connect-timeout=1000"))

# reset the value
\option --unset connectTimeout

#@<> WL14698-ET_3 - X protocol
# default value of the "connectTimeout" is 10 seconds
EXPECT_EXECUTION_TIME(10, lambda: util.check_for_server_upgrade(mysqlx_uri))

# integer value
shell.options["connectTimeout"] = 2
EXPECT_EXECUTION_TIME(2, lambda: util.check_for_server_upgrade(mysqlx_uri))

# fractional values
shell.options["connectTimeout"] = 0.5
EXPECT_EXECUTION_TIME(0.5, lambda: util.check_for_server_upgrade(mysqlx_uri))

shell.options["connectTimeout"] = 1.1
EXPECT_EXECUTION_TIME(1.1, lambda: util.check_for_server_upgrade(mysqlx_uri))

# connect-timeout (ms) in URI should overwrite the global option
shell.options["connectTimeout"] = 2
EXPECT_EXECUTION_TIME(1, lambda: util.check_for_server_upgrade(f"{mysqlx_uri}?connect-timeout=1000"))

# reset the value
\option --unset connectTimeout
