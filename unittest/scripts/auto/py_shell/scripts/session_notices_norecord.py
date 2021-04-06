#@{__version_num > 80000}
# Tests:
# _get_socket_fd()
# _enable_notices()
# _fetch_notices()

import time
import select

#@<> Test _get_socket_fd() in a classic session

session = mysql.get_session(__mysqluripwd)

EXPECT_LE(0, session._get_socket_fd())

session.close()

#@<> _enable_notices doesn't exist in classic

EXPECT_THROWS(lambda: session._enable_notices(["GRViewChanged"]), "unknown attribute: _enable_notices")
EXPECT_THROWS(lambda: session._fetch_notice(), "unknown attribute: _fetch_notice")


#@<> Test _get_socket_fd() in an X protocol session

session = mysqlx.get_session(__uripwd)

EXPECT_LE(0, session._get_socket_fd())

session.close()


#@<> Test _enable_notices() and _fetch_notices() in a GR group with 2 members

testutil.deploy_sandbox(__mysql_sandbox_port1, "root")
testutil.deploy_sandbox(__mysql_sandbox_port2, "root")

shell.connect(f"mysqlx://root:root@localhost:{__mysql_sandbox_port1}0")

session._enable_notices(["GRViewChanged"])

r, _, _ = select.select([session._get_socket_fd()], [], [], 0.1)
EXPECT_FALSE(r)

# nothing expected
n = session._fetch_notice()
EXPECT_EQ(None, n)

c = dba.create_cluster("cluster", {"gtidSetIsComplete":True})

# wait for notice to arrive
r, _, _ = select.select([session._get_socket_fd()], [], [], 10.0)
EXPECT_TRUE(session._get_socket_fd() in r)

# fetch all notices
notices = []
while True:
    session.run_sql("select 1") # hack to force libmysqlx to read notices
    n = session._fetch_notice()
    if n != None:
      notices.append(n)
    else:
      if notices:
        break
EXPECT_NE([], notices)

print(notices)
for n in notices:
  EXPECT_EQ("GRViewChanged", n["type"])

# addInstance and check again
c.add_instance(__sandbox_uri2)

# wait for notice to arrive
r, _, _ = select.select([session._get_socket_fd()], [], [], 10.0)
EXPECT_TRUE(session._get_socket_fd() in r)

# fetch all notices
notices = []
while True:
    session.run_sql("select 1") # hack to force libmysqlx to read notices
    n = session._fetch_notice()
    if n != None:
      notices.append(n)
    else:
      if notices:
        break
EXPECT_NE([], notices)

print(notices)
for n in notices:
  EXPECT_EQ("GRViewChanged", n["type"])

# removeInstance and check again
c.remove_instance(__sandbox_uri2)

# wait for notice to arrive
r, _, _ = select.select([session._get_socket_fd()], [], [], 10.0)
EXPECT_TRUE(session._get_socket_fd() in r)

# fetch all notices
notices = []
while True:
    session.run_sql("select 1") # hack to force libmysqlx to read notices
    n = session._fetch_notice()
    if n != None:
      notices.append(n)
    else:
      if notices:
        break
EXPECT_NE([], notices)

print(notices)
for n in notices:
  EXPECT_EQ("GRViewChanged", n["type"])

#@<> cleanup
testutil.destroy_sandbox(__mysql_sandbox_port1)
testutil.destroy_sandbox(__mysql_sandbox_port2)
