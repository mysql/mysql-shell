//@<> Initialization
var script_path = os.path.join(__tmp_dir, "sample.js")

function callMysqlshDebug(command_line_args) {
    //"--js-debug-port", "3939",
    default_args = ["--js",  "-f", script_path]
    args = [...default_args, ...command_line_args]
    testutil.callMysqlsh(args, "", ["MYSQLSH_TERM_COLOR_MODE=nocolor"])
}

//@<> Verify existence of global objects
testutil.createFile(script_path, `
if (dir(shell).indexOf("connect") != -1) {
    println("shell object is present!")
}

if (dir(dba).indexOf("getCluster") != -1) {
    println("dba object is present!")
}

if (dir(util).indexOf("checkForServerUpgrade") != -1) {
    println("util object is present!")
}

if (dir(mysql).indexOf("getSession") != -1) {
    println("mysql module is present!")
}

if (dir(mysqlx).indexOf("getSession") != -1) {
    println("mysqlx module is present!")
}
`);

callMysqlshDebug([])
EXPECT_OUTPUT_CONTAINS("shell object is present!");
EXPECT_OUTPUT_CONTAINS("dba object is present!");
EXPECT_OUTPUT_CONTAINS("util object is present!");
EXPECT_OUTPUT_CONTAINS("mysql module is present!");
EXPECT_OUTPUT_CONTAINS("mysqlx module is present!");
WIPE_OUTPUT()

//@<> Verify access to plugins
testutil.createFile(script_path, `plugins.info()`)

callMysqlshDebug([])
EXPECT_OUTPUT_CONTAINS("MySQL Shell Plugin Manager Version");
WIPE_OUTPUT()

//@<> Verify access to global objects
testutil.createFile(script_path, `
shell.connect("${__mysqluripwd}");

if (session.isOpen()) {
  println("Global session connected!")
}

session.close();
`);


callMysqlshDebug([])
EXPECT_OUTPUT_CONTAINS("Global session connected!");
WIPE_OUTPUT()
