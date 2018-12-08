// Use the method shell.enablePager() in all the scripting modes. Validate that the command set to the shell.options["pager"] option is in use to print the output.

//@ set pager to an external command
|<<<__pager.cmd>>> --append|

//@ run command before pager is enabled in JS mode
|1111|

//@ run command while pager is enabled in JS mode
||

//@ run command after pager has been disabled in JS mode
|3333|

//@ switch to Python mode
|Switching to Python mode...|

//@ run command before pager is enabled in Python mode
|4444|

//@ run command while pager is enabled in Python mode
||

//@ run command after pager has been disabled in Python mode
|6666|

//@ switch back to JS mode
|Switching to JavaScript mode...|

//@<OUT> verify if external command received output when pager was enabled
Running with arguments: <<<__pager.cmd>>> --append

2222
Running with arguments: <<<__pager.cmd>>> --append

5555

done
