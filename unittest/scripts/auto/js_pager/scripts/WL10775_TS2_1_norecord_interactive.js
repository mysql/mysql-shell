// Call the help command for different functions/methods/objects, verify that the output is forwarded to the pager specified by shell.options["pager"] option.

//@ set pager to an external command
shell.options.pager = __pager.cmd;

//@ invoke \help mysql, there should be no output here
\help mysql

//@ check if pager got all the output from \help mysql
os.load_text_file(__pager.file);

//@ invoke \help ClassicSession, there should be no output here
\help ClassicSession

//@ check if pager got all the output from \help ClassicSession
os.load_text_file(__pager.file);

//@ invoke \help ClassicSession.rollback, there should be no output here
\help ClassicSession.rollback

//@ check if pager got all the output from \help ClassicSession.rollback
os.load_text_file(__pager.file);
