// Display the general help for MySQL Shell (\h), verify that the command \pager (\P) is present.

//@ set pager to an external command
shell.options.pager = __pager.cmd;

//@ invoke \help Shell Commands, there should be no output here
\help Shell Commands

//@ help should contain information about pager commands
EXPECT_CONTAINS('- \\pager      (\\P)    Sets the current pager.', os.loadTextFile(__pager.file));
