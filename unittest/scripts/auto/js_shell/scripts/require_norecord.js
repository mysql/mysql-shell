// helper functions
function clone(obj) {
  return Object.assign({}, obj);
}

function strip_ext(path) {
  const idx = path.indexOf('.');
  return idx > -1 ? path.substring(0, idx) : path;
}

function Cache() {
  this.clear();
  const c = require('mysql');
  const x = require('mysqlx');
  this.__backup();
}

Cache.prototype.clear = function () {
  require.__mh.__cache = {};
  println('Cache cleared.');
};

Cache.prototype.restore = function () {
  require.__mh.__cache = clone(this.__cache);
  println('Cache restored.');
};

Cache.prototype.__backup = function () {
  this.__cache = clone(require.__mh.__cache);
  println('Cache backed up.');
};

function Module(folder, name) {
  this.dir = folder;
  this.name = name;
  this.name_no_ext = strip_ext(name);
  this.path = os.path.join(folder, name);
}

Module.prototype.write = function (contents) {
  testutil.createFile(this.path, `println('Loading: ${this.name}');
${contents}
println('Loaded: ${this.name}');
`);
}

function Modules(folder, cache) {
  this.__folder = folder;
  this.__cache = cache;

  this.__plugin = new Module(this.__folder, 'plugin');
  this.module = new Module(this.__folder, 'module.js');
  this.init = new Module(this.__plugin.path, 'init.js');
  this.cycle_a = new Module(this.__plugin.path, 'cycle-a.js');
  this.cycle_b = new Module(this.__plugin.path, 'cycle-b.js');

  // WL13119-TSFR6_1: When writing a new module validate that the module has access to the following variables: exports, module, __filename, __dirname.
  // 'module.js' uses `exports`, 'init.js' uses `module`, both 'cycle-a.js' and 'cycle-b.js' use `__filename` and `__dirname`

  this.module.write(`exports.fun = function () {
  println('This is fun()!');
};
require('./${this.__plugin.name}');
`);

  // WL13119-TSFR5_1: Request a module from different modules. Validate that there is no issue/conflict loading the module.
  // both 'init.js' and 'cycle-b.js' load module 'cycle-a.js'

  this.init.write(`const a = require('./${this.cycle_a.name_no_ext}');
const b = require('./${this.cycle_b.name_no_ext}');
module.exports = function () {
  println('This module exports only a function.');
  println('Calling second() from ${this.cycle_a.name}.');
  a.second();
  println('Calling second() from ${this.cycle_b.name}.');
  b.second();
};
`);

  // WL13119-TSFR9_1: Create a module that built its functionality in a cycle. Request the module and validate that the not loaded functionalities are available in the module being executed when the cycle completes.
  // modules 'cycle-a.js' and 'cycle-b.js' form a cycle and use each others exports

  this.cycle_a.write(`println('Directory: ' + __dirname);
println('File: ' + __filename);
const b = require('./${this.cycle_b.name}');
exports.first = function() {
  println('This is function first() defined by the module ${this.cycle_a.name}.');
};
exports.second = function() {
  println('This is function second() defined by the module ${this.cycle_a.name}.');
  b.first();
};
`);

  this.cycle_b.write(`println('Directory: ' + __dirname);
println('File: ' + __filename);
const a = require('./${this.cycle_a.name_no_ext}');
exports.first = function() {
  println('This is function first() defined by the module ${this.cycle_b.name}.');
};
exports.second = function() {
  println('This is function second() defined by the module ${this.cycle_b.name}.');
  a.first();
};
`);

  println('Module has been created.');
}

Modules.prototype.remove = function () {
  testutil.rmdir(this.__folder, true);
  println('Module has been removed.');
}

Modules.prototype.test = function (prefix) {
  // FR1.4.1.1, FR1.4.2.1
  println('Testing module with an extension.');
  this.__test_module(prefix, true);
  // FR1.4.1.2, FR1.4.2.2
  println('Testing module without an extension.');
  this.__test_module(prefix, false);
  // FR1.4.1.3, FR1.4.2.3
  println('Testing plugin.');
  this.__test_plugin(prefix);
  println('Testing done.');
}

Modules.prototype.__test_module = function (prefix, use_ext) {
  const m = require(`${prefix}${use_ext ? this.module.name : this.module.name_no_ext}`);
  // WL13119-TSFR6_2: Use the module requested to validate that the module functionality can be used.
  m.fun();
  this.__cache.restore();
}

Modules.prototype.__test_plugin = function (prefix) {
  const p = require(`${prefix}${this.__plugin.name}`);
  // WL13119-TSFR6_2: Use the module requested to validate that the module functionality can be used.
  p();
  this.__cache.restore();
}

//@ setup
const cache = new Cache();

////////////////////////////////////////////////////////////////////////////////

//@ wrong type of the argument - undefined
require(undefined);

//@ wrong type of the argument - null [USE: wrong type of the argument - undefined]
require(null);

//@ wrong type of the argument - number [USE: wrong type of the argument - undefined]
require(1);

//@ wrong type of the argument - array [USE: wrong type of the argument - undefined]
require([]);

//@ wrong type of the argument - dictionary [USE: wrong type of the argument - undefined]
require({});

//@ wrong type of the argument - object [USE: wrong type of the argument - undefined]
require(shell);

////////////////////////////////////////////////////////////////////////////////

//@ WL13119-TSFR1_1: Call the require() function with empty module name/path, validate that an exception is thrown.
require('');

////////////////////////////////////////////////////////////////////////////////

// WL13119-TSFR1_2: Call the require() function with a module name/path that has a backslash (\) character in the name/path, validate that an exception is thrown.

//@ WL13119-TSFR1_2
require('\\');

//@ WL13119-TSFR1_2 - begin [USE: WL13119-TSFR1_2]
require('\\abc');

//@ WL13119-TSFR1_2 - middle [USE: WL13119-TSFR1_2]
require('ab\\c');

//@ WL13119-TSFR1_2 - end [USE: WL13119-TSFR1_2]
require('abc\\');

////////////////////////////////////////////////////////////////////////////////

//@ WL13119-TSFR2_1: Call the require() function with an absolute path to a module, validate that an exception is thrown
require(os.getcwd().replace(/\\/g, '/'));

////////////////////////////////////////////////////////////////////////////////

// WL13119-TSFR3_1: Call the require('built_in_module') function to load a built-in module. Validate that the module is loaded and can be used.

//@ WL13119-TSFR3_1: clear the cache
cache.clear();

//@ WL13119-TSFR3_1: get the mysql module
const c = require('mysql');

//@ WL13119-TSFR3_1: check the mysql module
c.help();

//@ WL13119-TSFR3_1: get the mysqlx module
const x = require('mysqlx');

//@ WL13119-TSFR3_1: check the mysqlx module
x.help();

//@ WL13119-TSFR3_1: get the mysql module once again, make sure it's cached
c === require('mysql');

//@ WL13119-TSFR3_1: get the mysqlx module once again, make sure it's cached
x === require('mysqlx');

//@ WL13119-TSFR3_1: restore the cache
cache.restore();

////////////////////////////////////////////////////////////////////////////////

const modules_subfolder = 'dir';
const modules_subfolder_abs = os.path.join(os.getcwd(), modules_subfolder);

//@ create modules in the current working directory
let modules = new Modules(modules_subfolder_abs, cache);

////////////////////////////////////////////////////////////////////////////////

// WL13119-TSFR4_1: Having a file placed in the directory of the module currently being executed, use the require('./mymodule.js') function to load it starting the path with ./ and using the whole file name including the file extension. Validate that the module is loaded and can be used.
// WL13119-TSFR4_2: Having a file placed in the directory of the module currently being executed, use the require('./mymodule') function to load it starting the path with ./ without including the file extension. Validate that the module is loaded and can be used.
// WL13119-TSFR4_3: Having a module directory placed in the directory of the module currently being executed, use the require('./mymodule') function to load it starting the path with ./ without including the init.js file in the path. Validate that the module is loaded and can be used.

//@ load modules from a path relative to the current working directory using './' prefix
// note: this method loads a module file with and without an extension, as well as a module directory
modules.test(`./${modules_subfolder}/`);

//@ load modules from a path relative to the current working directory using '../' prefix [USE: load modules from a path relative to the current working directory using './' prefix]
modules.test(`../${os.path.basename(os.getcwd())}/${modules_subfolder}/`);

////////////////////////////////////////////////////////////////////////////////

// load a module from a path relative to the script which is currently being executed

const exe_file = os.path.join(modules_subfolder_abs, 'exe.js');

//@ create the script file to be executed
testutil.createFile(exe_file, `println('Running the file');
const mod = require('./${modules.module.name}');
mod.fun();
println('Done');
`);

//@ load the module using './' prefix from the script file executed in shell
testutil.callMysqlsh(['--js', '--file', exe_file]);

//@ delete the script file
testutil.rmfile(exe_file);

////////////////////////////////////////////////////////////////////////////////

// WL13119-TSFR3_2: Call the require('module_name_or_path') function to load a module from a path. Validate that the module is loaded and can be used.

//@ WL13119-TSFR3_2: load the module for the first time
println('Loading...');
const first_time = require(`./${modules_subfolder}/${modules.module.name}`);
first_time.fun();
println('Done');

//@ WL13119-TSFR3_2: load the module for the second time
println('Loading...');
const second_time = require(`./${modules_subfolder}/${modules.module.name}`);
second_time.fun();
println('Done');

//@ WL13119-TSFR3_2: check if module was cached
first_time === second_time;

//@ WL13119-TSFR3_2: restore the cache [USE: WL13119-TSFR3_1: restore the cache]
cache.restore();

////////////////////////////////////////////////////////////////////////////////

// load a module which throws an exception

//@ create a module which throws an exception [USE: create the script file to be executed]
testutil.createFile(exe_file, `println('Running the file');
throw new Error('Exception!!!');
println('Done');
`);

//@ load the module which throws an exception
require(`./${modules_subfolder}/exe.js`);

//@ delete the module file which throws an exception [USE: delete the script file]
testutil.rmfile(exe_file);

////////////////////////////////////////////////////////////////////////////////

//@ delete modules in the current working directory
modules.remove();

//@ create modules in the absolute directory present in the sys.path variable [USE: create modules in the current working directory]
modules = new Modules(os.path.join(sys.path[0], modules_subfolder), cache);

////////////////////////////////////////////////////////////////////////////////

// WL13119-TSFR4_4: Repeat TSFR4_1 - TSFR4_3 but using the Shell home directory.

//@ WL13119-TSFR4_4: load the modules from the Shell home directory [USE: load modules from a path relative to the current working directory using './' prefix]
modules.test(`${modules_subfolder}/`);

//@ WL13119-TSFR4_4: load the modules from the Shell home directory using './' prefix [USE: load modules from a path relative to the current working directory using './' prefix]
modules.test(`./${modules_subfolder}/`);

////////////////////////////////////////////////////////////////////////////////

//@ delete modules in the absolute directory present in the sys.path variable [USE: delete modules in the current working directory]
modules.remove();

////////////////////////////////////////////////////////////////////////////////

// WL13119-TSFR7_2: Modify the value of the sys.path variable at runtime to a valid directory that contains valid modules to load. Validate that the require() function is updated to load the modules in the new directory specified.
// WL13119-TSFR7_3: If a relative path is set to the sys.var, validate that the absolute path is well resolved as relative to the current working directory.

// add a relative path to sys.path
const subfolder_in_path = 'tmp';
sys.path = [...sys.path, subfolder_in_path];
const subfolder_in_path_abs = os.path.join(os.getcwd(), subfolder_in_path);

//@ create modules in the relative directory present in the sys.path variable [USE: create modules in the current working directory]
modules = new Modules(subfolder_in_path_abs, cache);

//@ load the modules from the relative directory present in the sys.path variable [USE: load modules from a path relative to the current working directory using './' prefix]
modules.test('');

//@ load the modules from the relative directory present in the sys.path variable using './' prefix [USE: load modules from a path relative to the current working directory using './' prefix]
modules.test('./');

////////////////////////////////////////////////////////////////////////////////

// WL13119-TSFR4_5: Repeat TSFR4_1 - TSFR4_3 but using the directories specified in the MYSQLSH_JS_MODULE_PATH environment variable.

//@ create a script file in a folder different than the one in sys.path [USE: create the script file to be executed]
testutil.createFile(exe_file, `println('Running the file');
const mod = require('${modules.module.name}');
mod.fun();
println('Done');
`);

//@ run the script file, specifying the module folder via the environment variable [USE: load the module using './' prefix from the script file executed in shell]
testutil.callMysqlsh(['--js', '--file', exe_file], '', [`MYSQLSH_JS_MODULE_PATH=${subfolder_in_path_abs}`]);

//@ remove the whole folder containing the script file [USE: delete the script file]
testutil.rmdir(modules_subfolder_abs, true);

////////////////////////////////////////////////////////////////////////////////

//@ delete modules in the relative directory present in the sys.path variable [USE: delete modules in the current working directory]
modules.remove();

////////////////////////////////////////////////////////////////////////////////

//@ WL13119-TSFR4_6: If Shell can't find the file or module specified, validate that an exception is thrown.
require('invalid_module');

////////////////////////////////////////////////////////////////////////////////

// WL13119-TSFR7_1: When starting the Shell, validate that the value of the sys.path variable contains the Shell home directory and the directories specified in the MYSQLSH_JS_MODULE_PATH environment variable.

//@ WL13119-TSFR7_1
testutil.callMysqlsh(['--js', '--execute', 'for (var p of sys.path) println(p);'], '', ['MYSQLSH_JS_MODULE_PATH=/tmp;/usr/share;mod']);

//@ WL13119-TSFR7_1: no environment variable
testutil.callMysqlsh(['--js', '--execute', 'for (var p of sys.path) println(p);']);

//@ WL13119-TSFR7_1: empty environment variable [USE: WL13119-TSFR7_1: no environment variable]
testutil.callMysqlsh(['--js', '--execute', 'for (var p of sys.path) println(p);'], '', ['MYSQLSH_JS_MODULE_PATH=']);

////////////////////////////////////////////////////////////////////////////////

//@ cleanup
cache.restore();
