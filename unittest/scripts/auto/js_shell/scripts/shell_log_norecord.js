//@<> helpers
function log_message(level, msg) {
  return (level.charAt(0).toUpperCase() + level.slice(1).toLowerCase()) + ": " + msg;
}

//@<> set log level, do not log to stderr
WIPE_SHELL_LOG();

const debug_message = "This is a debug message!";
const debug2_message = "This is a debug TWO message!";

// set log level to ERROR + stderr, change it to DEBUG, do some logging
EXPECT_EQ(0, testutil.callMysqlsh([ "--js", "--log-level=@3", "-e", `shell.options.logLevel = "debug"; shell.log("debug", "${debug_message}"); shell.log("debug2", "${debug2_message}");` ]))

// first message is logged
EXPECT_SHELL_LOG_CONTAINS(debug_message);
EXPECT_SHELL_LOG_NOT_CONTAINS(debug2_message);

if (__os_type != "windows") {
  // no output to stderr
  EXPECT_STDOUT_NOT_CONTAINS(log_message("debug", debug_message));
  EXPECT_STDOUT_NOT_CONTAINS(log_message("debug2", debug2_message));
}

//@<> set log level, log to stderr
WIPE_SHELL_LOG();

const another_debug2_message = "This is another debug TWO message!";
const debug3_message = "This is a debug THREE message!";

// set log level to ERROR, change it to DEBUG + stderr, do some logging
EXPECT_EQ(0, testutil.callMysqlsh([ "--js", "--log-level=3", "-e", `shell.options.logLevel = "@debug2"; shell.log("debug2", "${another_debug2_message}"); shell.log("debug3", "${debug3_message}");` ]))

// first message is logged
EXPECT_SHELL_LOG_CONTAINS(another_debug2_message);
EXPECT_SHELL_LOG_NOT_CONTAINS(debug3_message);

// on Windows output is logged using OutputDebugString(), it does not appear in stderr
if (__os_type != "windows") {
  // first message is outputted to stderr
  EXPECT_STDOUT_CONTAINS(log_message("debug2", another_debug2_message));
  EXPECT_STDOUT_NOT_CONTAINS(log_message("debug3", debug3_message));
}
