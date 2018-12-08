// Use the method shell.enablePager() in all the scripting modes. Validate that the command set to the shell.options["pager"] option is in use to print the output.

//@ set pager to an external command
shell.options.pager = __pager.cmd + ' --append';

//@ run command before pager is enabled in JS mode
1111

//@ run command while pager is enabled in JS mode
shell.enablePager();
2222

//@ run command after pager has been disabled in JS mode
shell.disablePager();
3333

//@ switch to Python mode
\py

//@ run command before pager is enabled in Python mode
4444

//@ run command while pager is enabled in Python mode
shell.enable_pager();
5555

//@ run command after pager has been disabled in Python mode
shell.disable_pager();
6666

//@ switch back to JS mode
\js

//@ verify if external command received output when pager was enabled
os.load_text_file(__pager.file);
println('done');
