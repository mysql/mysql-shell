#@ {VER(>=8.0.0)}

#@<> Setup
testutil.deploy_sandbox(__mysql_sandbox_port1, "root")

#@<> Setup cluster 
import mysqlsh

mydba = mysqlsh.connect_dba(__sandbox_uri1)

cluster = mydba.create_cluster("mycluster")
cluster.disconnect()

#@<> Catch error through mysqlsh.Error
try:
    mydba.get_cluster("badcluster")
    testutil.fail("<red>Function didn't throw exception as expected</red>")
except mysqlsh.Error as e:
    EXPECT_EQ(51101, e.code)
except:
    testutil.fail("<red>Function threw wrong exception</red>")

#@<> dba.session
mydba.session.run_sql("select 1")

#@<> DbError should be a subclass of Error

try:
    mydba.session.run_sql("badquery")
    testutil.fail("<red>Function didn't throw exception as expected</red>")
except mysqlsh.DBError as e:
    EXPECT_EQ(mysql.ErrorCode.ER_PARSE_ERROR, e.code)
except:
    testutil.fail("<red>Function threw wrong exception</red>")

try:
    mydba.session.run_sql("badquery")
    testutil.fail("<red>Function didn't throw exception as expected</red>")
except mysqlsh.Error as e:
    EXPECT_EQ(mysql.ErrorCode.ER_PARSE_ERROR, e.code)
except:
    testutil.fail("<red>Function threw wrong exception</red>")

#@<> Check for __qualname__ and __name__ in wrapped methods

EXPECT_EQ("Testutils.deploy_sandbox", testutil.deploy_sandbox.__qualname__)
EXPECT_EQ("Dba.create_cluster", dba.create_cluster.__qualname__)

EXPECT_EQ("deploy_sandbox", testutil.deploy_sandbox.__name__)
EXPECT_EQ("create_cluster", dba.create_cluster.__name__)

#@<> check that isatty exists
sys.stdout.isatty()
sys.stdin.isatty()
sys.stderr.isatty()

#@<> check if fileno throws an exception
EXPECT_THROWS(lambda: sys.stdout.fileno(), "UnsupportedOperation: Method not supported.")
EXPECT_THROWS(lambda: sys.stderr.fileno(), "UnsupportedOperation: Method not supported.")

#@<> thread_start and end

script=f"""
import mysqlsh
def test_thread():
    mysqlsh.thread_init()
    session = mysqlsh.globals.shell.open_session("{__sandbox_uri1}")
    session.close()
    mysqlsh.thread_end()
    print("THREAD DONE")

import threading
thd = threading.Thread(target=test_thread)
thd.start()
thd.join()
"""
with open("testscript.py", "w+") as f:
    f.write(script)
testutil.call_mysqlsh(["-f", "testscript.py"])

EXPECT_STDOUT_CONTAINS("THREAD DONE")
EXPECT_STDOUT_NOT_CONTAINS("Error in my_thread_global_end()")

#@<> Cleanup
mydba.session.close()

testutil.destroy_sandbox(__mysql_sandbox_port1)
