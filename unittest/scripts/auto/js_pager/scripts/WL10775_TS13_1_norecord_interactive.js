// Display the help for the shell object, verify the methods enablePager() and disablePager() are present.

//@ set pager to an external command
shell.options.pager = __pager.cmd;

//@ invoke \help shell, there should be no output here
\help shell

//@ load output from pager
var output = os.load_text_file(__pager.file);

//@ help should contain information about enablePager() method
EXPECT_CONTAINS('enablePager()', output);

//@ help should contain information about disablePager() method
EXPECT_CONTAINS('disablePager()', output);
