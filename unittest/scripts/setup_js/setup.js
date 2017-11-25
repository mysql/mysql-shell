
function create_root_from_anywhere(session) {
  session.runSql("SET SQL_LOG_BIN=0");
  session.runSql("CREATE USER root@'%' IDENTIFIED BY 'root'");
  session.runSql("GRANT ALL ON *.* to root@'%' WITH GRANT OPTION");
  session.runSql("SET SQL_LOG_BIN=1");
}


function EXPECT_EQ(expected, actual) {
  if (expected != actual) {
    var context = "Tested values don't match as expected:\n\tActual: " + actual + "\n\tExpected: " + expected;
    testutil.fail(context);
  }
}

function EXPECT_TRUE(value) {
  if (!value) {
    var context = "Tested value expected to be true but is false";
    testutil.fail(context);
  }
}

function EXPECT_FALSE(value) {
  if (value) {
    var context = "Tested value expected to be false but is true";
    testutil.fail(context);
  }
}
