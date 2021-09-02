//@<> BUG#33310170 - Wrong JSON formatting when printing strings in JavaScript mode with --json
testutil.callMysqlsh(["--json=raw", "-e", "print('sample')"]);
EXPECT_OUTPUT_CONTAINS('{"info":"sample"}');
WIPE_OUTPUT();


//@<> Ensuring the print function for multiple values reports the correct output using --json
testutil.callMysqlsh(["--json=raw", "-e", "print('sample', 5, 'other', 3.5)"]);
EXPECT_OUTPUT_CONTAINS('{"info":"sample 5 other 3.5"}');
WIPE_OUTPUT();
