// Display the help for shell.options and verify that the pager option is present.

//@ set pager to an external command
shell.options.pager = __pager.cmd;

//@ invoke \help shell.options, there should be no output here
\help shell.options

//@ help should contain information about pager
EXPECT_CONTAINS('- pager: string which specifies the external command which is going to be', os.load_text_file(__pager.file));
