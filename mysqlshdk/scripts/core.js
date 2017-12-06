/*
 * Copyright (c) 2015, 2016, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
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

this._module_handler = new ModuleHandler();


this.require = function(module_name, reload)
{
  return this._module_handler.get_module(module_name, reload);
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

