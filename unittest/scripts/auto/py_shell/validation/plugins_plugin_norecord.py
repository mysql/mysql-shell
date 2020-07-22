#@<OUT> Install the test repository
[[*]]> plugins.repositories.add('http://127.0.0.1:<<<PORT>>>/mysql-shell-plugins-manifest.json')

WARNING:
You are about to add an external MySQL Shell plugin repository.
External plugin repositories and their plugins complement
the functionality of MySQL Shell and can contain system
level software that could be potentially harmful to your
system. Please review the description below and only proceed
if you have obtained the external plugin repository URL from
a trusted source.

Oracle and its affiliates cannot be held responsible for
any potential harm caused by using plugins from external sources.

Repository : Testing MySQL Shell Plugin Repository
Description: A testing repository to be used while testing the plugins builtin plugin.
URL: http://127.0.0.1:<<<PORT>>>/mysql-shell-plugins-manifest.json

The repository contains the following plugins:

  - Repo Testing Plugin

Fetching current user repositories...
Adding repository 'Testing MySQL Shell Plugin Repository'...
Repository 'Testing MySQL Shell Plugin Repository' successfully added.

#@<OUT> Help on built-in plugins plugin
[[*]]> \? plugins
NAME
      plugins - Plugin to manage MySQL Shell plugins

DESCRIPTION
      This global object exposes a list of shell extensions to manage MySQL
      Shell plugins

      Use plugins.about() to get more information about writing MySQL Shell
      plugins.

PROPERTIES
      repositories
            Manages the registry of plugin repositories.

FUNCTIONS
      about()
            Prints detailed information about the MySQL Shell plugin support.

      details([name][, kwargs])
            Gives detailed information about a MySQL Shell plugin.

      help([member])
            Provides help about this object and it's members

      info()
            Prints basic information about the plugin manager.

      install([name][, kwargs])
            Installs a MySQL Shell plugin.

      list([kwargs])
            Lists all available MySQL Shell plugins.

      uninstall([name][, kwargs])
            Uninstalls a MySQL Shell plugin.

      update([name][, kwargs])
            Updates MySQL Shell plugins.

      version()
            Returns the version number of the plugin manager.

#@<OUT> Tests the about function
[[*]]> plugins.about()

The MySQL Shell allows extending its base functionality through the creation
of plugins.

A plugin is a folder containing the code that provides the functionality to
be made available on the MySQL Shell.

User defined plugins should be located at plugins folder at the following
paths:

- Windows: %AppData%\MySQL\mysqlsh\plugins
- Others: ~/.mysqlsh/plugins

A plugin must contain an init file which is the entry point to load the
extension:

- init.js for plugins written in JavaScript.
- init.py for plugins written in Python.

On startup, the shell traverses the folders inside of the *plugins* folder
searching for the plugin init file. The init file will be loaded on the
corresponding context (JavaScript or Python).

Use Cases

The main use cases for MySQL Shell plugins include:

- Definition of shell reports to be used with the \show and \watch Shell
Commands.
- Definition of new Global Objects with user defined functionality.

For additional information on shell reports execute: \? reports

For additional information on extension objects execute: \? extension objects


#@<OUT> Help on built-in plugins.repositories object
[[*]]> \? plugins.repositories
NAME
      repositories - Manages the registry of plugin repositories.

SYNTAX
      plugins.repositories

DESCRIPTION
      Manages the registry of plugin repositories.

FUNCTIONS
      add([url][, kwargs])
            Adds a new plugin repository

      help([member])
            Provides help about this object and it's members

      list([kwargs])
            Lists all registered plugin repositories

      remove([kwargs])
            Removes a registered plugin repository

#@<OUT> Tests the info function
[[*]]> plugins.info()
MySQL Shell Plugin Manager Version 0.0.1

#@<OUT> Tests the details function without name specified
[[*]]> plugins.details()
Fetching list of all plugins...
${*}
   # Name                 Caption                            Version          Installed       
---- -------------------- ---------------------------------- ---------------- ----------------
${*}
Testing MySQL Shell Plugin Repository
==============================================================================================
[[*]] repo                 Repo Testing Plugin                0.0.1 PREVIEW    No              


Repo Testing Plugin
-------------------
        Plugin Name: repo
     Latest Version: 0.0.1
          Dev Stage: PREVIEW
        Description: Plugin to test the plugins builtin plugin.
 Available Versions: 1
     0.0.1 PREVIEW - Initial Version

#@<OUT> Tests the details function with a specific plugin
[[*]]> plugins.details('repo')
Fetching list of all plugins...
${*}
Repo Testing Plugin
-------------------
        Plugin Name: repo
     Latest Version: 0.0.1
          Dev Stage: PREVIEW
        Description: Plugin to test the plugins builtin plugin.
 Available Versions: 1
     0.0.1 PREVIEW - Initial Version

#@<OUT> Lists the plugin repositories
[[*]]> plugins.repositories.list()
Registered MySQL Shell Plugin Repositories.

   1 Official MySQL Shell Plugin Repository
     The official MySQL Shell Plugin Repository maintained by the MySQL Team at Oracle.
     https://cdn.mysql.com/windows/installer/manifest.zip

   2 Testing MySQL Shell Plugin Repository
     A testing repository to be used while testing the plugins builtin plugin.
     http://127.0.0.1:<<<PORT>>>/mysql-shell-plugins-manifest.json

Total of 2 repositories.

#@<OUT> Lists the plugins
[[*]]> plugins.list()
Fetching list of all plugins...
${*}
   # Name                 Caption                            Version          Installed       
---- -------------------- ---------------------------------- ---------------- ----------------
${*}
Testing MySQL Shell Plugin Repository
==============================================================================================
   [[*]] repo                 Repo Testing Plugin                0.0.1 PREVIEW    No              
     Plugin to test the plugins builtin plugin.

[[*]] plugin[[*]] total.

Use plugins.details() to get more information about a specific plugin.

#@<OUT> Install the test plugin
[[*]]> plugins.install('repo')
Fetching list of all plugins...
${*}
Installing Repo Testing Plugin ...
Repo Testing Plugin has been installed successfully.

Please restart the shell to load the plugin. To get help type  '\? repo' after restart.
${*}
[[*]]> print('Test Plugin Version: ' + repo.version())
Test Plugin Version: 0.0.1

#@<OUT> Lists installed plugins in a new shell session
[[*]]> plugins.list()
Fetching list of all plugins...
${*}
   # Name                 Caption                            Version          Installed       
---- -------------------- ---------------------------------- ---------------- ----------------
${*}
Testing MySQL Shell Plugin Repository
==============================================================================================
*  [[*]] repo                 Repo Testing Plugin                0.0.1 PREVIEW    0.0.1 PREVIEW   
     Plugin to test the plugins builtin plugin.

* 1 plugin installed, [[*]] plugin[[*]] total.

Use plugins.details() to get more information about a specific plugin.

#@<OUT> Attempts reinstalling a plugin
[[*]]> plugins.install('repo')
Fetching list of all plugins...
${*}
The plugin 'repo' is already installed. Use the force_install parameter to re-install it anyway.

#@<OUT> Attempts forces reinstalling a plugin
[[*]]> plugins.install('repo', force_install=True)
Fetching list of all plugins...
${*}
Installing Repo Testing Plugin ...
Repo Testing Plugin has been installed successfully.

Please restart the shell to load the plugin. To get help type  '\? repo' after restart.

#@<OUT> Upgrades the test plugin version in the repository
[[*]]> plugins.list()
Fetching list of all plugins...
${*}
   # Name                 Caption                            Version          Installed       
---- -------------------- ---------------------------------- ---------------- ----------------
${*}
Testing MySQL Shell Plugin Repository
==============================================================================================
*  [[*]] repo                 Repo Testing Plugin                0.0.2 PREVIEW    0.0.1 PREVIEW^  
     Plugin to test the plugins builtin plugin.

* 1 plugin installed, [[*]] plugin[[*]] total.
^ One update is available. Use plugins.update() to install the update.

Use plugins.details() to get more information about a specific plugin.

#@<OUT> Upgrades the test plugin locally
[[*]]> plugins.update()
Fetching list of updatable plugins...
${*}
   # Name                 Caption                            Version          Installed       
---- -------------------- ---------------------------------- ---------------- ----------------
Testing MySQL Shell Plugin Repository
==============================================================================================
*  [[*]] repo                 Repo Testing Plugin                0.0.2 PREVIEW    0.0.1 PREVIEW^  


Updating Repo Testing Plugin ...
${*}
The update has been completed successfully.

Please restart the shell to reload the plugin.
${*}
[[*]]> print('Test Plugin Version: ' + repo.version())
Test Plugin Version: 0.0.2

#@<OUT> Downgrades the test plugin to the initial version
[[*]]> plugins.install('repo', version='0.0.1', force_install=True)
Fetching list of all plugins...
${*}
Installing Repo Testing Plugin ...
Repo Testing Plugin has been installed successfully.

Please restart the shell to load the plugin. To get help type  '\? repo' after restart.
${*}
[[*]]> print('Test Plugin Version: ' + repo.version())
Test Plugin Version: 0.0.1

#@<OUT> Upgrades the test plugin again, using name
[[*]]> plugins.list()
Fetching list of all plugins...
${*}
   # Name                 Caption                            Version          Installed       
---- -------------------- ---------------------------------- ---------------- ----------------
${*}
Testing MySQL Shell Plugin Repository
==============================================================================================
*  [[*]] repo                 Repo Testing Plugin                0.0.2 PREVIEW    0.0.1 PREVIEW^  
     Plugin to test the plugins builtin plugin.

* 1 plugin installed, [[*]] plugin[[*]] total.
^ One update is available. Use plugins.update() to install the update.

Use plugins.details() to get more information about a specific plugin.
${*}
[[*]]> plugins.update('repo')
Fetching list of updatable plugins...
${*}
Updating Repo Testing Plugin ...
${*}
The update has been completed successfully.

Please restart the shell to reload the plugin.
${*}
[[*]]> print('Test Plugin Version: ' + repo.version())
Test Plugin Version: 0.0.2

#@<OUT> Uninstalls the test plugin
[[*]]> plugins.uninstall()
Fetching list of installed plugins...
${*}
   # Name                 Caption                            Version          Installed       
---- -------------------- ---------------------------------- ---------------- ----------------
Testing MySQL Shell Plugin Repository
==============================================================================================
*  [[*]] repo                 Repo Testing Plugin                0.0.2 PREVIEW    0.0.2 PREVIEW   



Uninstalling Repo Testing Plugin ...
Repo Testing Plugin has been uninstalled successfully.

Please restart the shell to unload the plugin.
${*}
[[*]]> print('Test Plugin Version: ' + repo.version())
Traceback (most recent call last):
  File "<string>", line 1, in <module>
NameError: name 'repo' is not defined

#@<OUT> Removes the test plugin repository
[[*]]> plugins.repositories.remove(url='http://127.0.0.1:<<<PORT>>>/mysql-shell-plugins-manifest.json')

Removing repository 'http://127.0.0.1:<<<PORT>>>/mysql-shell-plugins-manifest.json'...
Repository successfully removed.

#@<OUT> Lists the plugin repositories again
[[*]]> plugins.repositories.list()
Registered MySQL Shell Plugin Repositories.

   1 Official MySQL Shell Plugin Repository
     The official MySQL Shell Plugin Repository maintained by the MySQL Team at Oracle.
     https://cdn.mysql.com/windows/installer/manifest.zip

Total of 1 repository.

