#@ {__os_type != "macos" or __machine_type != "aarch64"}

# Test is disabled on aarch64, because timers behave wierd there.
# It is hard to pinpoint why this happen, and it is also hard to create/find
# server which will trigger connection timeout scenario.
#
# It's impossible to write this test deterministicly.
#
# See notes below.
#
# Two solution were tested.
#
# 1/ Spawn "sink server". Such server accept tcp connections but does not
# send back any data. Two more scenarios were tested here:
#   1.1/ Read data from client, but do not send anything back
#   1.2/ Do not read any data from client (on layer 7)
#
# Such test setup works for MySQL classic protocol, but not for X Protocol.
# It seems that MySQL classic protocol reset connect timer when receive data
# from server, but it seems that X Protocol reset connection timer when
# 3-way handshake succeed.
#
# 2/ Override connect(2)[0] syscall (shared library approach). Impl. connection
# delay and return ETIMEDOUT error. LD_PRELOAD that library when running
# run_unit_tests. It works fine, but doesn't test connect-timeout feature at
# all, because timeout is not returned from kernel socket timeout.
#
# There are to more ideas, but non of them are feasible to impl.
#
# 3/ Prepare box with modified kernel tcp stack that delays accept() connection
# from client.
#
# 4/ Impl. Server using L2 raw sockets, but running such server requires root
# priviledges, or at least CAP_NET_RAW capability for user. And also we need
# to impl. part of TCP stack.
#
#
# [0]
# int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);

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
if __version_num < 80027:
  cluster = dba.create_cluster("test", {"ipAllowlist": "127.0.0.1," + hostname_ip})
else:
  cluster = dba.create_cluster("test")

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
