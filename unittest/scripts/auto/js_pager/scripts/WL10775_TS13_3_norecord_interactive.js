// Use the method shell.enablePager(). Then change the script mode, validate that the command set to the shell.options["pager"] option is not in use to print the output anymore.

//@ set pager to an external command
shell.options.pager = __pager.cmd + ' --append';

//@ run command before pager is enabled in JS mode
1111

//@ call enablePager()
shell.enablePager();

//@ run command while pager is enabled in JS mode
2222

//@ switch to Python mode
\py

//@ run command in Python mode, pager should be automatically disabled when switching scripting modes
3333

//@ switch back to JS mode
\js

//@ verify if external command received output when pager was enabled
os.load_text_file(__pager.file);
println('done');
