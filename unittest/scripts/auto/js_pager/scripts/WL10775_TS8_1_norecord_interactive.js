// Display the general help for MySQL Shell (\h), verify that the command \nopager is present.

//@ set pager to an external command
shell.options.pager = __pager.cmd;

//@ invoke \help Shell Commands, there should be no output here
\help Shell Commands

//@ help should contain information about nopager command
EXPECT_CONTAINS('- \\nopager            Disables the current pager.', os.loadTextFile(__pager.file));
