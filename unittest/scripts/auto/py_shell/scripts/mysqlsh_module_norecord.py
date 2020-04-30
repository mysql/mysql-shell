
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

#@<> check that isatty exists (checking the return value depends on how the tests are ran)
sys.stdout.isatty()
sys.stdin.isatty()
sys.stderr.isatty()

#@<> Cleanup
mydba.session.close()

testutil.destroy_sandbox(__mysql_sandbox_port1)