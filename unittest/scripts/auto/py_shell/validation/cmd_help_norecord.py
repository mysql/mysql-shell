#@<OUT> Generic Help
The Shell Help is organized in categories and topics. To get help for a
specific category or topic use: \? <pattern>

The <pattern> argument should be the name of a category or a topic.

The pattern is a filter to identify topics for which help is required, it can
use the following wildcards:

- ? matches any single character.
- * matches any character sequence.

The following are the main help categories:

 - AdminAPI       The AdminAPI is an API that enables configuring and managing
                  InnoDB Clusters, ReplicaSets, ClusterSets, among other
                  things.
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
 - \disconnect         Disconnects the global session.
 - \edit       (\e)    Launch a system editor to edit a command to be executed.
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
 - \system     (\!)    Execute a system shell command.
 - \use        (\u)    Sets the active schema.
 - \warnings   (\W)    Show warnings after every statement.
 - \watch              Executes the given report with provided options and
                       arguments in a loop.

GLOBAL OBJECTS

The following modules and objects are ready for use when the shell starts:

 - dba      Used for InnoDB Cluster, ReplicaSet, and ClusterSet administration.
 - mysql    Support for connecting to MySQL servers using the classic MySQL
            protocol.
 - mysqlx   Used to work with X Protocol sessions using the MySQL X DevAPI.
 - plugins  Plugin to manage MySQL Shell plugins
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

- ? matches any single character.
- * matches any character sequence.

The following are the main help categories:

 - AdminAPI       The AdminAPI is an API that enables configuring and managing
                  InnoDB Clusters, ReplicaSets, ClusterSets, among other
                  things.
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

- ? matches any single character.
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
 - \disconnect         Disconnects the global session.
 - \edit       (\e)    Launch a system editor to edit a command to be executed.
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
 - \system     (\!)    Execute a system shell command.
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

- ? matches any single character.
- * matches any character sequence.

The following are the main help categories:

 - Shell Commands Provides details about the available built-in shell commands.
 - SQL Syntax     Entry point to retrieve syntax help on SQL statements.

Use \? \help for additional details.
Switching to Python mode...

#@<OUT> Help on Admin API Category
The AdminAPI can be used interactively from the MySQL Shell prompt and
non-interactively from JavaScript and Python scripts and directly from the
command line.

For more information about the dba object use: \? dba

In the AdminAPI, an InnoDB Cluster is represented as an instance of the Cluster
class, while ReplicaSets are represented as an instance of the ReplicaSet
class, and ClusterSets are represented as an instance of the ClusterSet class.

For more information about the Cluster class use: \? Cluster

For more information about the ClusterSet class use: \? ClusterSet

For more information about the ReplicaSet class use: \? ReplicaSet

Scripting

Through the JavaScript and Python bindings of the MySQL Shell, the AdminAPI can
be used from scripts, which can in turn be used interactively or
non-interactively. To execute a script, use the -f command line option,
followed by the script file path. Options that follow the path are passed
directly to the script being executed, which can access them from sys.argv
    mysqlsh root@localhost -f myscript.py arg1 arg2

If the script finishes successfully, the Shell will exit with code 0, while
uncaught exceptions/errors cause it to exist with a non-0 code.

By default, the AdminAPI enables interactivity, which will cause functions to
prompt for missing passwords, confirmations and bits of information that cannot
be obtained automatically.

Prompts can be completely disabled with the --no-wizard command line option or
using the "interactive" boolean option available in some of functions. If
interactivity is disabled and some information is missing (e.g. a password), an
error will be raised instead of being prompted.

Secure Password Handling

Passwords can be safely stored locally, using the system's native secrets
storage functionality (or login-paths in Linux). Whenever the Shell needs a
password, it will first query for the password in the system, before prompting
for it.

Passwords can be stored during interactive use, by confirming in the Store
Password prompt. They can also be stored programmatically, using the
shell.store_credential() function.

You can also use environment variables to pass information to your scripts. In
JavaScript, the os.getenv() function can be used to access them.

Command Line Interface

In addition to the scripting interface, the MySQL Shell supports generic
command line integration, which allows calling AdminAPI functions directly from
the system shell (e.g. bash). Examples:
    $ mysqlsh -- dba configure-instance root@localhost

    is equivalent to:

    > dba.configureInstance("root@localhost")

    $ mysqlsh root@localhost -- cluster status --extended

    is equivalent to:

    > dba.getCluster().status({extended:true})

The mapping from AdminAPI function signatures works as follows:

- The first argument after a -- can be a shell global object, such as dba. As a
  special case, cluster and rs are also accepted.
- The second argument is the name of the function of the object to be called.
  The naming convention is automatically converted from camelCase/snake_case to
  lower case separated by dashes.
- The rest of the arguments are used in the same order as their JS/Python
  counterparts. Instances can be given as URIs. Option dictionaries can be
  passed as --options, where the option name is the same as in JS/Python.

OBJECTS
 - dba InnoDB Cluster, ReplicaSet, and ClusterSet management functions.

CLASSES
 - Cluster    Represents an InnoDB Cluster.
 - ClusterSet Represents an InnoDB ClusterSet.
 - ReplicaSet Represents an InnoDB ReplicaSet.

#@<OUT> Help on shell commands
The shell commands allow executing specific operations including updating the
shell configuration.

The following shell commands are available:

 - \                   Start multi-line input when in SQL mode.
 - \connect    (\c)    Connects the shell to a MySQL server and assigns the
                       global session.
 - \disconnect         Disconnects the global session.
 - \edit       (\e)    Launch a system editor to edit a command to be executed.
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
 - \system     (\!)    Execute a system shell command.
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

The following topics were found at the AdminAPI category:

- dba.session

The following topics were found at the X DevAPI category:

- mysqlx.Collection.session
- mysqlx.DatabaseObject.session
- mysqlx.Schema.session
- mysqlx.Session
- mysqlx.Table.session

For help on a specific topic use: \? <topic>

e.g.: \? dba.session

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

  mysqlsh [options] -- <object> <operation> [arguments]

WHERE:

- object - a string identifying shell object to be called.
- operation - a string identifying method to be called on the object.
- arguments - arguments passed to the called method.

DETAILS:

The following objects can be called using this format:

- dba - dba global object.
- cluster - cluster returned by calling dba.getCluster().
- shell - shell global object.
- shell.options - shell.options object.
- util - util global object

The operation name can be given in the following naming styles:

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

$ mysqlsh -- util check-for-server-upgrade --user=root --host=localhost
--password=
      mysql-js> util.checkForServerUpgrade({user:"root", host:"localhost"},
      {password:""})

