// WL12763-TSFR1_4 - When calling the `\system` command with a valid argument, validate that the output is displayed to the user and is not affected by the active pager.

//@ set pager to an external command which will capture the output to a file
shell.options.pager = __pager.cmd + ' --append';

//@ enable the pager
shell.enablePager()

//@ run \system command, output should not be captured by pager
\system echo zaq12WSX

//@ disable the pager
shell.disablePager()

//@ check if pager didn't get the output from the system command
EXPECT_NOT_CONTAINS("zaq12WSX", os.loadTextFile(__pager.file));
