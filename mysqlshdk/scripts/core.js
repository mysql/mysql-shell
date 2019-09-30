/*
 * Copyright (c) 2015, 2019, Oracle and/or its affiliates. All rights reserved.
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

'use strict';

function check_string(name, value) {
  if (typeof value !== 'string') {
    throw new TypeError(`The '${name}' parameter is expected to be a string.`);
  }
}

function Module(full_path) {
  check_string('full_path', full_path);

  this.__filename = full_path;
  this.__dirname = os.path.dirname(full_path);
  this.exports = {};
}

function ModuleHandler() { }

ModuleHandler.__native_modules = __list_native_modules();

ModuleHandler.__find_module = function (module, paths) {
  for (let path of paths) {
    if (!os.path.isabs(path)) {
      path = os.path.join(os.getcwd(), path);
    }

    const full_path = os.path.normpath(os.path.join(path, module));

    if (os.path.isfile(full_path)) {
      return full_path;
    }

    const full_path_js = full_path + '.js';

    if (os.path.isfile(full_path_js)) {
      return full_path_js;
    }

    const full_path_init = os.path.join(full_path, 'init.js');

    if (os.path.isdir(full_path) && os.path.isfile(full_path_init)) {
      return full_path_init;
    }
  }

  throw new Error(`Could not find module '${module}'.`);
};

ModuleHandler.prototype.__cache = {};

ModuleHandler.prototype.__require = function (module) {
  check_string('module_name_or_path', module);

  if (0 === module.length) {
    throw new Error('The path must contain at least one character.');
  }

  if (module.indexOf('\\') > -1) {
    throw new Error(`The '\\' character is disallowed.`);
  }

  if (os.path.isabs(module)) {
    throw new Error('The absolute path is disallowed.');
  }

  if (ModuleHandler.__native_modules.includes(module)) {
    if (!(module in this.__cache)) {
      this.__cache[module] = __load_native_module(module);
    }

    return this.__cache[module];
  }

  const paths = [];

  if (module.startsWith('./') || module.startsWith('../')) {
    paths.push(__current_module_folder());
  }

  paths.push(...sys.path);

  const full_path = ModuleHandler.__find_module(module, paths);

  if (!(full_path in this.__cache)) {
    this.__load_module(full_path);
  }

  return this.__cache[full_path].exports;
};

ModuleHandler.prototype.__load_module = function (full_path) {
  const module = new Module(full_path);

  this.__cache[full_path] = module;

  __load_module(full_path, module);
};

const mh = new ModuleHandler();

this.require = function (module) {
  return mh.__require(module);
};

this.require.__mh = mh;

// Object.keys(object) returns the enumerable properties found directly
// on the object while iterating them also includes the ones defined on
// the prototype chain.
this.dir = function (object) {
  var keys = [];
  for (k in object) {
    keys[keys.length] = k;
  }

  return keys;
}
