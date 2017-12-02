
def create_root_from_anywhere(session):
  session.run_sql("SET SQL_LOG_BIN=0")
  session.run_sql("CREATE USER root@'%' IDENTIFIED BY 'root'")
  session.run_sql("GRANT ALL ON *.* to root@'%' WITH GRANT OPTION")
  session.run_sql("SET SQL_LOG_BIN=1")


def EXPECT_EQ(expected, actual):
  if expected != actual:
    context = "Tested values don't match as expected:\n\tActual: " + actual + "\n\tExpected: " + expected
    testutil.fail(context)

def  EXPECT_TRUE(value):
  if !value:
    context = "Tested value expected to be true but is false"
    testutil.fail(context)

def EXPECT_FALSE(value):
  if value:
    context = "Tested value expected to be false but is true"
    testutil.fail(context)
