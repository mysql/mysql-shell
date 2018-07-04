// Use the method shell.enablePager() in all the scripting modes. Validate that the command set to the shell.options["pager"] option is in use to print the output.

//@ set pager to an external command
shell.options.pager = __pager.cmd + ' --append';

//@ run command before pager is enabled in JS mode
1111

//@ call enablePager()
shell.enablePager();

//@ run command while pager is enabled in JS mode
2222

//@ call disablePager()
shell.disablePager();

//@ run command after pager has been disabled in JS mode
3333

//@ switch to Python mode
\py

//@ run command before pager is enabled in Python mode
4444

//@ call enable_pager()
shell.enable_pager();

//@ run command while pager is enabled in Python mode
5555

//@ call disable_pager()
shell.disable_pager();

//@ run command after pager has been disabled in Python mode
6666

//@ switch back to JS mode
\js

//@ verify if external command received output when pager was enabled
os.load_text_file(__pager.file);
println('done');
