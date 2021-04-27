//@<OUT> setup
Cache cleared.
Cache backed up.

//@ wrong type of the argument - undefined
||TypeError: The 'module_name_or_path' parameter is expected to be a string.

//@ WL13119-TSFR1_1: Call the require() function with empty module name/path, validate that an exception is thrown.
||Error: The path must contain at least one character.

//@ WL13119-TSFR1_2
||Error: The '\' character is disallowed.

//@ WL13119-TSFR2_1: Call the require() function with an absolute path to a module, validate that an exception is thrown
||Error: The absolute path is disallowed.

//@ WL13119-TSFR3_1: clear the cache
|Cache cleared.|

//@ WL13119-TSFR3_1: get the mysql module
||

//@<OUT> WL13119-TSFR3_1: check the mysql module
NAME
      mysql - Encloses the functions and classes available to interact with a
              MySQL Server using the traditional MySQL Protocol.

//@ WL13119-TSFR3_1: get the mysqlx module
||

//@<OUT> WL13119-TSFR3_1: check the mysqlx module
NAME
      mysqlx - Encloses the functions and classes available to interact with an
               X Protocol enabled MySQL Product.

//@ WL13119-TSFR3_1: get the mysql module once again, make sure it's cached
|true|

//@ WL13119-TSFR3_1: get the mysqlx module once again, make sure it's cached
|true|

//@ WL13119-TSFR3_1: restore the cache
|Cache restored.|

//@ create modules in the current working directory
|Module has been created.|

//@<OUT> load modules from a path relative to the current working directory using './' prefix
Testing module with an extension.
Loading: <<<modules.module.name>>>
Loading: <<<modules.init.name>>>
Loading: <<<modules.cycle_a.name>>>
Directory: <<<modules.cycle_a.dir>>>
File: <<<modules.cycle_a.path>>>
Loading: <<<modules.cycle_b.name>>>
Directory: <<<modules.cycle_b.dir>>>
File: <<<modules.cycle_b.path>>>
Loaded: <<<modules.cycle_b.name>>>
Loaded: <<<modules.cycle_a.name>>>
Loaded: <<<modules.init.name>>>
Loaded: <<<modules.module.name>>>
This is fun()!
Cache restored.
Testing module without an extension.
Loading: <<<modules.module.name>>>
Loading: <<<modules.init.name>>>
Loading: <<<modules.cycle_a.name>>>
Directory: <<<modules.cycle_a.dir>>>
File: <<<modules.cycle_a.path>>>
Loading: <<<modules.cycle_b.name>>>
Directory: <<<modules.cycle_b.dir>>>
File: <<<modules.cycle_b.path>>>
Loaded: <<<modules.cycle_b.name>>>
Loaded: <<<modules.cycle_a.name>>>
Loaded: <<<modules.init.name>>>
Loaded: <<<modules.module.name>>>
This is fun()!
Cache restored.
Testing plugin.
Loading: <<<modules.init.name>>>
Loading: <<<modules.cycle_a.name>>>
Directory: <<<modules.cycle_a.dir>>>
File: <<<modules.cycle_a.path>>>
Loading: <<<modules.cycle_b.name>>>
Directory: <<<modules.cycle_b.dir>>>
File: <<<modules.cycle_b.path>>>
Loaded: <<<modules.cycle_b.name>>>
Loaded: <<<modules.cycle_a.name>>>
Loaded: <<<modules.init.name>>>
This module exports only a function.
Calling second() from <<<modules.cycle_a.name>>>.
This is function second() defined by the module <<<modules.cycle_a.name>>>.
This is function first() defined by the module <<<modules.cycle_b.name>>>.
Calling second() from <<<modules.cycle_b.name>>>.
This is function second() defined by the module <<<modules.cycle_b.name>>>.
This is function first() defined by the module <<<modules.cycle_a.name>>>.
Cache restored.
Testing done.

//@ create the script file to be executed
||

//@<OUT> load the module using './' prefix from the script file executed in shell
Running the file
Loading: <<<modules.module.name>>>
Loading: <<<modules.init.name>>>
Loading: <<<modules.cycle_a.name>>>
Directory: <<<modules.cycle_a.dir>>>
File: <<<modules.cycle_a.path>>>
Loading: <<<modules.cycle_b.name>>>
Directory: <<<modules.cycle_b.dir>>>
File: <<<modules.cycle_b.path>>>
Loaded: <<<modules.cycle_b.name>>>
Loaded: <<<modules.cycle_a.name>>>
Loaded: <<<modules.init.name>>>
Loaded: <<<modules.module.name>>>
This is fun()!
Done
0

//@ delete the script file
||

//@<OUT> WL13119-TSFR3_2: load the module for the first time
Loading...
Loading: <<<modules.module.name>>>
Loading: <<<modules.init.name>>>
Loading: <<<modules.cycle_a.name>>>
Directory: <<<modules.cycle_a.dir>>>
File: <<<modules.cycle_a.path>>>
Loading: <<<modules.cycle_b.name>>>
Directory: <<<modules.cycle_b.dir>>>
File: <<<modules.cycle_b.path>>>
Loaded: <<<modules.cycle_b.name>>>
Loaded: <<<modules.cycle_a.name>>>
Loaded: <<<modules.init.name>>>
Loaded: <<<modules.module.name>>>
This is fun()!
Done

//@<OUT> WL13119-TSFR3_2: load the module for the second time
Loading...
This is fun()!
Done

//@ WL13119-TSFR3_2: check if module was cached
|true|

//@<ERR> load the module which throws an exception
Error: Exception!!! at <<<exe_file>>>:2:7
in throw new Error('Exception!!!');
         ^
Error: Exception!!!
    at <<<exe_file>>>:2:7
    at <<<exe_file>>>:4:3

//@ delete modules in the current working directory
|Module has been removed.|

//@ WL13119-TSFR4_6: If Shell can't find the file or module specified, validate that an exception is thrown.
||Error: Could not find module 'invalid_module'.

//@<OUT> WL13119-TSFR7_1
<<<sys.path[0]>>>
/tmp
/usr/share
mod
0

//@<OUT> WL13119-TSFR7_1: no environment variable
<<<sys.path[0]>>>
0

//@ cleanup
|Cache restored.|
