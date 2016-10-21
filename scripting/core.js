/*
 * Copyright (c) 2015, 2016, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */
function ModuleHandler()
{
  this._module_cache = {}

  this._find_module = function(name)
  {
    var index = 0;
    var module_path = '';
    while(index < sys.path.length && module_path == '')
    {
      var tmp_path = sys.path[index];
      var last_char = tmp_path.slice(-1);
      if (last_char != '/' && last_char != '\\')
        tmp_path += '/';

      tmp_path += name + '.js';

      if (os.file_exists(tmp_path))
        module_path = tmp_path;
      else
        index++;
    }

    return module_path;
  }

  this._load_module = function(name)
  {
    var module = __require(name);
    if (module)
      this._module_cache[name] = module;
    else
    {
      var path = this._find_module(name);

      if (path)
      {
        var source = os.load_text_file(path);
        if (source)
        {
          var wrapped_source = '(function (exports){' + source + '});';
          var module_definition = __build_module(path, wrapped_source);

          var module = {}
          module_definition(module)

          this._module_cache[name] = module;
        }
        else
          throw ('Module ' + name + ' is empty.');
      }
    }
  }

  this.get_module = function(name, reload)
  {
    if (typeof reload !== 'undefined' && reload)
      delete this._module_cache[name];

    // Loads the module if not done already.
    if (typeof this._module_cache[name] === 'undefined')
    {
      // Retrieves the module path and content
      this._load_module(name);
    }

    // If the module is on the cache then returns it
    if (typeof this._module_cache[name] !== 'undefined')
      return this._module_cache[name]
    else
      throw ('Module ' + name + ' was not found.');
  }
}

this.sys = {}

Object.defineProperties(this.sys, {
  'path': {
    value: [],
    writable: true,
    enumerable: true
  },
  '_module_handler': {
    value: new ModuleHandler(),
    writable: false,
    enumerable: false
  }
});

// Searches for MYSQLX_HOME
var path = os.get_mysqlx_home_path();
if (path)
  this.sys.path[this.sys.path.length] = path + '/share/mysqlsh/modules/js';

// If MYSQLX_HOME not found, sets the current directory as a valid module path
else
{
  path = os.get_binary_folder();
  if (path)
    this.sys.path[this.sys.path.length] = path + '/modules/js';
  else
    this.sys.path[this.sys.path.length] = './modules/js';
}

// Finally sees if there are additional configured paths
path = os.getenv('MYSQLSH_JS_MODULE_PATH');
if (path)
{
  var paths = path.split(';');
  this.sys.path = this.sys.path.concat(paths);
}

this.require = function(module_name, reload)
{
  return this.sys._module_handler.get_module(module_name, reload);
}

// Object.keys(object) returns the enumerable properties found directly
// on the object while iterating them also includes the ones defined on
// the prototype chain.
this.dir = function(object)
{
  var keys = []
  for(k in object)
    keys[keys.length] = k;

  return keys;
}

