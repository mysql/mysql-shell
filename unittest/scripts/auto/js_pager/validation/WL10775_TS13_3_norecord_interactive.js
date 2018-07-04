// Use the method shell.enablePager(). Then change the script mode, validate that the command set to the shell.options["pager"] option is not in use to print the output anymore.

//@ set pager to an external command
|<<<__pager.cmd>>> --append|

//@ run command before pager is enabled in JS mode
|1111|

//@ call enablePager()
||

//@ run command while pager is enabled in JS mode
||

//@ switch to Python mode
|Switching to Python mode...|

//@ run command in Python mode, pager should be automatically disabled when switching scripting modes
|3333|

//@ switch back to JS mode
|Switching to JavaScript mode...|

//@<OUT> verify if external command received output when pager was enabled
Running with arguments: <<<__pager.cmd>>> --append

2222

done
