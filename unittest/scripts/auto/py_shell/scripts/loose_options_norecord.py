#@<> Warnins on unknown loose options
testutil.call_mysqlsh(["--loose-whatever", "-e", "print('done')"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor"])
EXPECT_STDOUT_CONTAINS("WARNING: unknown option: --loose-whatever")
WIPE_OUTPUT()

#@<> Warnins on unknown loose options with --quiet-start=1
testutil.call_mysqlsh(["--quiet-start=1", "--loose-whatever", "-e", "print('done')"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor"])
EXPECT_STDOUT_CONTAINS("WARNING: unknown option: --loose-whatever")
WIPE_OUTPUT()

#@<> No warnins on unknown loose options with --quiet-start=2
testutil.call_mysqlsh(["--quiet-start=2", "--loose-whatever", "-e", "print('done')"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor"])
EXPECT_STDOUT_NOT_CONTAINS("WARNING: unknown option: --loose-whatever")
WIPE_OUTPUT()

#@<> Existing options can be prefixed with --loose
testutil.call_mysqlsh(["--loose-quiet-start=1", "--loose-whatever", "-e", "print('done')"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor"])
EXPECT_STDOUT_NOT_CONTAINS("WARNING: unknown option: --loose-quiet-start")
EXPECT_STDOUT_CONTAINS("WARNING: unknown option: --loose-whatever")
WIPE_OUTPUT()

#@<> Unknown loose option with URI value as --loose-option=URI ignores value
testutil.call_mysqlsh([f"--loose-whatever={__mysqluripwd}", "-e", "print(session)"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor"])
EXPECT_STDOUT_CONTAINS(f"WARNING: unknown option: --loose-whatever={__mysqluripwd}")
EXPECT_STDOUT_NOT_CONTAINS(f"<ClassicSession:{__mysql_uri}>")
EXPECT_STDOUT_CONTAINS("null")
WIPE_OUTPUT()

#@<> Unknown loose option with URI value as --loose-option URI uses value
testutil.call_mysqlsh([f"--loose-whatever", __mysqluripwd, "-e", "print(session)"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor"])
EXPECT_STDOUT_CONTAINS("WARNING: unknown option: --loose-whatever")
EXPECT_STDOUT_CONTAINS(f"<ClassicSession:{__mysql_uri}>")
EXPECT_STDOUT_NOT_CONTAINS("null")
WIPE_OUTPUT()

#@<> Warns unsupported loose options from options file
testutil.create_file("my_options.cnf", """
[client]
loose-unknown=whatever
""")
testutil.call_mysqlsh(["--defaults-file=my_options.cnf", "-e", "print('done')"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor"])
EXPECT_STDOUT_CONTAINS("WARNING: unknown option: --loose-unknown")
WIPE_OUTPUT()
testutil.rmfile("my_options.cnf")
