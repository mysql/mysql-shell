#@<OUT> Generic Help
The Shell Help is organized in categories and topics. To get help for a
specific category or topic use: \? <pattern>

The <pattern> argument should be the name of a category or a topic.

The pattern is a filter to identify topics for which help is required, it can
use the following wildcards:

- ? matches any single charecter.
- * matches any character sequence.

The following are the main help categories:

 - AdminAPI       Introduces to the dba global object and the InnoDB cluster
                  administration API.
 - Shell Commands Provides details about the available built-in shell commands.
 - ShellAPI       Contains information about the shell and util global objects
                  as well as the mysql module that enables executing SQL on
                  MySQL Servers.
 - SQL Syntax     Entry point to retrieve syntax help on SQL statements.
 - X DevAPI       Details the mysqlx module as well as the capabilities of the
                  X DevAPI which enable working with MySQL as a Document Store

The available topics include:

- The dba global object and the classes available at the AdminAPI.
- The mysqlx module and the classes available at the X DevAPI.
- The mysql module and the global objects and classes available at the
  ShellAPI.
- The functions and properties of the classes exposed by the APIs.
- The available shell commands.
- Any word that is part of an SQL statement.
- Command Line - invoking built-in shell functions without entering interactive
  mode.

SHELL COMMANDS

The shell commands allow executing specific operations including updating the
shell configuration.

The following shell commands are available:

 - \                   Start multi-line input when in SQL mode.
 - \connect    (\c)    Connects the shell to a MySQL server and assigns the
                       global session.
 - \exit               Exits the MySQL Shell, same as \quit.
 - \help       (\?,\h) Prints help information about a specific topic.
 - \history            View and edit command line history.
 - \js                 Switches to JavaScript processing mode.
 - \nopager            Disables the current pager.
 - \nowarnings (\w)    Don't show warnings after every statement.
 - \option             Allows working with the available shell options.
 - \pager      (\P)    Sets the current pager.
 - \py                 Switches to Python processing mode.
 - \quit       (\q)    Exits the MySQL Shell.
 - \reconnect          Reconnects the global session.
 - \rehash             Refresh the autocompletion cache.
 - \show               Executes the given report with provided options and
                       arguments.
 - \source     (\.)    Loads and executes a script from a file.
 - \sql                Executes SQL statement or switches to SQL processing
                       mode when no statement is given.
 - \status     (\s)    Print information about the current global session.
 - \use        (\u)    Sets the active schema.
 - \warnings   (\W)    Show warnings after every statement.
 - \watch              Executes the given report with provided options and
                       arguments in a loop.

GLOBAL OBJECTS

The following modules and objects are ready for use when the shell starts:

 - dba      Used for InnoDB cluster administration.
 - mysql    Support for connecting to MySQL servers using the classic MySQL
            protocol.
 - mysqlx   Used to work with X Protocol sessions using the MySQL X DevAPI.
 - shell    Gives access to general purpose functions and properties.
 - testutil
 - util     Global object that groups miscellaneous tools like upgrade checker
            and JSON import.

For additional information on these global objects use: <object>.help()

EXAMPLES
\? AdminAPI
      Displays information about the AdminAPI.

\? \connect
      Displays usage details for the \connect command.

\? check_instance_configuration
      Displays usage details for the dba.check_instance_configuration function.

\? sql syntax
      Displays the main SQL help categories.

#@<OUT> Help Contents
The Shell Help is organized in categories and topics. To get help for a
specific category or topic use: \? <pattern>

The <pattern> argument should be the name of a category or a topic.

The pattern is a filter to identify topics for which help is required, it can
use the following wildcards:

- ? matches any single charecter.
- * matches any character sequence.

The following are the main help categories:

 - AdminAPI       Introduces to the dba global object and the InnoDB cluster
                  administration API.
 - Shell Commands Provides details about the available built-in shell commands.
 - ShellAPI       Contains information about the shell and util global objects
                  as well as the mysql module that enables executing SQL on
                  MySQL Servers.
 - SQL Syntax     Entry point to retrieve syntax help on SQL statements.
 - X DevAPI       Details the mysqlx module as well as the capabilities of the
                  X DevAPI which enable working with MySQL as a Document Store

Use \? \help for additional details.

#@<OUT> Generic Help in SQL mode
Switching to SQL mode... Commands end with ;
The Shell Help is organized in categories and topics. To get help for a
specific category or topic use: \? <pattern>

The <pattern> argument should be the name of a category or a topic.

The pattern is a filter to identify topics for which help is required, it can
use the following wildcards:

- ? matches any single charecter.
- * matches any character sequence.

The following are the main help categories:

 - Shell Commands Provides details about the available built-in shell commands.
 - SQL Syntax     Entry point to retrieve syntax help on SQL statements.

The available topics include:

- The available shell commands.
- Any word that is part of an SQL statement.
- Command Line - invoking built-in shell functions without entering interactive
  mode.

SHELL COMMANDS

The shell commands allow executing specific operations including updating the
shell configuration.

The following shell commands are available:

 - \                   Start multi-line input when in SQL mode.
 - \connect    (\c)    Connects the shell to a MySQL server and assigns the
                       global session.
 - \exit               Exits the MySQL Shell, same as \quit.
 - \G                  Send command to mysql server, display result vertically.
 - \g                  Send command to mysql server.
 - \help       (\?,\h) Prints help information about a specific topic.
 - \history            View and edit command line history.
 - \js                 Switches to JavaScript processing mode.
 - \nopager            Disables the current pager.
 - \nowarnings (\w)    Don't show warnings after every statement.
 - \option             Allows working with the available shell options.
 - \pager      (\P)    Sets the current pager.
 - \py                 Switches to Python processing mode.
 - \quit       (\q)    Exits the MySQL Shell.
 - \reconnect          Reconnects the global session.
 - \rehash             Refresh the autocompletion cache.
 - \show               Executes the given report with provided options and
                       arguments.
 - \source     (\.)    Loads and executes a script from a file.
 - \sql                Executes SQL statement or switches to SQL processing
                       mode when no statement is given.
 - \status     (\s)    Print information about the current global session.
 - \use        (\u)    Sets the active schema.
 - \warnings   (\W)    Show warnings after every statement.
 - \watch              Executes the given report with provided options and
                       arguments in a loop.

EXAMPLES
\? sql syntax
      Displays the main SQL help categories.

\? select
      Displays information about the SELECT SQL statement.

#@<OUT> Help Contents in SQL mode
The Shell Help is organized in categories and topics. To get help for a
specific category or topic use: \? <pattern>

The <pattern> argument should be the name of a category or a topic.

The pattern is a filter to identify topics for which help is required, it can
use the following wildcards:

- ? matches any single charecter.
- * matches any character sequence.

The following are the main help categories:

 - Shell Commands Provides details about the available built-in shell commands.
 - SQL Syntax     Entry point to retrieve syntax help on SQL statements.

Use \? \help for additional details.
Switching to Python mode...

#@<OUT> Help on Admin API Category
MySQL InnoDB cluster provides a complete high availability solution for MySQL.

The AdminAPI is an interactive API that enables configuring and administering
InnoDB clusters.

Use the dba global object to:

- Verify if a MySQL server is suitable for InnoDB cluster.
- Configure a MySQL server to be used as an InnoDB cluster instance.
- Create an InnoDB cluster.
- Get a handle for performing operations on an InnoDB cluster.
- Other InnoDB cluster maintenance tasks.

In the AdminAPI, an InnoDB cluster is represented as an instance of the Cluster
class.

For more information about the dba object use: \? dba

For more information about the Cluster class use: \? Cluster

#@<OUT> Help on shell commands
The shell commands allow executing specific operations including updating the
shell configuration.

The following shell commands are available:

 - \                   Start multi-line input when in SQL mode.
 - \connect    (\c)    Connects the shell to a MySQL server and assigns the
                       global session.
 - \exit               Exits the MySQL Shell, same as \quit.
 - \help       (\?,\h) Prints help information about a specific topic.
 - \history            View and edit command line history.
 - \js                 Switches to JavaScript processing mode.
 - \nopager            Disables the current pager.
 - \nowarnings (\w)    Don't show warnings after every statement.
 - \option             Allows working with the available shell options.
 - \pager      (\P)    Sets the current pager.
 - \py                 Switches to Python processing mode.
 - \quit       (\q)    Exits the MySQL Shell.
 - \reconnect          Reconnects the global session.
 - \rehash             Refresh the autocompletion cache.
 - \show               Executes the given report with provided options and
                       arguments.
 - \source     (\.)    Loads and executes a script from a file.
 - \sql                Executes SQL statement or switches to SQL processing
                       mode when no statement is given.
 - \status     (\s)    Print information about the current global session.
 - \use        (\u)    Sets the active schema.
 - \warnings   (\W)    Show warnings after every statement.
 - \watch              Executes the given report with provided options and
                       arguments in a loop.

For help on a specific command use \? <command>

EXAMPLE
\? \connect
      Displays information about the \connect command.

#@<OUT> Help on ShellAPI Category
Contains information about the shell and util global objects as well as the
mysql module that enables executing SQL on MySQL Servers.

OBJECTS
 - shell Gives access to general purpose functions and properties.
 - util  Global object that groups miscellaneous tools like upgrade checker and
         JSON import.

CLASSES
 - Column Represents the metadata for a column in a result.
 - Row    Represents the a Row in a Result.

MODULES
 - mysql Encloses the functions and classes available to interact with a MySQL
         Server using the traditional MySQL Protocol.

#@<OUT> Help on X DevAPI Category
The X DevAPI allows working with MySQL databases through a modern and fluent
API. It specially simplifies using MySQL as a Document Store.

The variety of operations available through this API includes:

- Creation of a session to an X Protocol enabled MySQL Server.
- Schema management.
- Collection management.
- CRUD operations on Collections and Tables.

The X DevAPI is a collection of functions and classes contained on the mysqlx
module which is automatically loaded when the shell starts.

To work on a MySQL Server with the X DevAPI, start by creating a session using:
mysqlx.get_session(...).

For more details about the mysqlx module use: \? mysqlx

For more details about how to create a session with the X DevAPI use: \?
mysqlx.get_session

#@<OUT> Help on unknown topic
No help items found matching 'unknown'

#@<OUT> Help on topic with several matches
Found several entries matching session

The following topics were found at the X DevAPI category:

- mysqlx.Collection.session
- mysqlx.DatabaseObject.session
- mysqlx.Schema.session
- mysqlx.Session
- mysqlx.Table.session

For help on a specific topic use: \? <topic>

e.g.: \? mysqlx.Collection.session

#@<OUT> Help for sandbox operations
Found several entries matching *sandbox*

The following topics were found at the AdminAPI category:

- dba.delete_sandbox_instance
- dba.deploy_sandbox_instance
- dba.kill_sandbox_instance
- dba.start_sandbox_instance
- dba.stop_sandbox_instance

For help on a specific topic use: \? <topic>

e.g.: \? dba.delete_sandbox_instance

#@<OUT> Help for SQL, with classic session, multiple matches
Found several entries matching select

The following topics were found at the SQL Syntax category:

- SQL Syntax/SELECT

The following topics were found at the X DevAPI category:

- mysqlx.Table.select
- mysqlx.TableSelect.select

For help on a specific topic use: \? <topic>

e.g.: \? SQL Syntax/SELECT

#@ Switching to SQL mode, same test gives results
|Syntax:|
|SELECT is used to retrieve rows selected from one or more tables|
|The most commonly used clauses of SELECT statements are these:|

#@ Switching back to Python, help for SQL Syntax
|The following topics were found at the SQL Syntax category:|
|- Account Management|
|For help on a specific topic use: \? <topic>|
|e.g.: \? Account Management|

#@ Help for SQL Syntax, with x session
|The following topics were found at the SQL Syntax category:|
|- Account Management|
|For help on a specific topic use: \? <topic>|
|e.g.: \? Account Management|

#@<OUT> Help for SQL Syntax, no connection
SQL help requires the Shell to be connected to a MySQL server.


#@<OUT> Help for API Command Line Integration
The MySQL Shell functionality is generally available as API calls, this is
objects containing methods to perform specific tasks, they can be called either
in JavaScript or Python modes.

The most important functionality (available on the shell global objects) has
been integrated in a way that it is possible to call it directly from the
command line, by following a specific syntax.

SYNTAX:

  mysqlsh [options] -- <object> <method> [arguments]

WHERE:

- object - a string identifying shell object to be called.
- method - a string identifying method to be called on the object.
- arguments - arguments passed to the called method.

DETAILS:

The following objects can be called using this format:

- dba - dba global object.
- cluster - cluster returned by calling dba.getCluster().
- shell - shell global object.
- shell.options - shell.options object.
- util - util global object

The method name can be given in the following naming styles:

- Camel case: (e.g. createCluster, checkForServerUpgrade)
- Kebab case: (e.g. create-cluster, check-for-server-upgrade)

The arguments syntax allows mixing positional and named arguments (similar to
args and *kwargs in Python):

ARGUMENT SYNTAX:

  [ positional_argument ]* [ { named_argument* } ]* [ named_argument ]*

WHERE:

- positional_argument - is a single string representing a value.
- named_argument - a string in the form of --argument-name[=value]

A positional_argument is defined by the following rules:

- positional_argument ::= value
- value   ::= string | integer | float | boolean | null
- string  ::= plain string, quoted if contains whitespace characters
- integer ::= plain integer
- float   ::= plain float
- boolean ::= 1 | 0 | true | false
- null    ::= "-"

Positional arguments are passed to the function call in the order they are
defined.

Named arguments represent a dictionary option that the method is expecting. It
is created by prepending the option name with double dash and can also be
specified either using camelCaseNaming or kebab-case-naming, examples:

--anOptionName[=value]
--an-option-name[=value]

If the value is not provided the option name will be handled as a boolean
option with a default value of TRUE.

Grouping Named Arguments

It is possible to create a group of named arguments by enclosing them between
curly braces. This group would be handled as a positional argument at the
position where the opening curly brace was found, it will be passed to the
function as a dictionary containing all the named arguments defined on the
group.

Named arguments that are not placed inside curly braces (independently of the
position on the command line), are packed a single dictionary, passed as a last
parameter on the method call.

Following are some examples of command line calls as well as how they are
mapped to the API method call.

EXAMPLES
$ mysqlsh -- dba deploy-sandbox-instance 1234 --password=secret
      mysql-js> dba.deploySandboxInstance(1234, {password: secret})

$ mysqlsh root@localhost:1234 -- dba create-cluster mycluster
      mysql-js> dba.createCluster("mycluster")

$ mysqlsh root@localhost:1234 -- cluster status
      mysql-js> cluster.status()

$ mysqlsh -- shell.options set-persist history.autoSave true
      mysql-js> shell.options.setPersist("history.autoSave", true)

$ mysqlsh -- util checkForServerUpgrade root@localhost --outputFormat=JSON
      mysql-js> util.checkForServerUpgrade("root@localhost",{outputFormat:
      "JSON"})

$ mysqlsh -- util check-for-server-upgrade { --user=root --host=localhost }
--password=
      mysql-js> util.checkForServerUpgrade({user:"root", host:"localhost"},
      {password:""})

