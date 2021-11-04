//@<OUT> Generic Help
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
 - os       Gives access to functions which allow to interact with the
            operating system.
 - shell    Gives access to general purpose functions and properties.
 - sys      Gives access to system specific parameters.
 - testutil
 - util     Global object that groups miscellaneous tools like upgrade checker
            and JSON import.

For additional information on these global objects use: <object>.help()

EXAMPLES
\? AdminAPI
      Displays information about the AdminAPI.

\? \connect
      Displays usage details for the \connect command.

\? checkInstanceConfiguration
      Displays usage details for the dba.checkInstanceConfiguration function.

\? sql syntax
      Displays the main SQL help categories.

//@<OUT> Help Contents
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

//@<OUT> Generic Help in SQL mode
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

//@<OUT> Help Contents in SQL mode
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
Switching to JavaScript mode...

//@<OUT> Help on Admin API Category
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
shell.storeCredential() function.

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

//@<OUT> Help on shell commands
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

//@<OUT> Help on ShellAPI Category
Contains information about the shell and util global objects as well as the
mysql module that enables executing SQL on MySQL Servers.

OBJECTS
 - os    Gives access to functions which allow to interact with the operating
         system.
 - shell Gives access to general purpose functions and properties.
 - sys   Gives access to system specific parameters.
 - util  Global object that groups miscellaneous tools like upgrade checker and
         JSON import.

FUNCTIONS
      dir(object)
            Returns a list of enumerable properties on the target object.

      require(module_name_or_path)
            Loads the specified JavaScript module.

CLASSES
 - Column Represents the metadata for a column in a result.
 - Row    Represents the a Row in a Result.

MODULES
 - mysql Encloses the functions and classes available to interact with a MySQL
         Server using the traditional MySQL Protocol.

//@<OUT> Help on dir
NAME
      dir - Returns a list of enumerable properties on the target object.

SYNTAX
      dir(object)

WHERE
      object: The object whose properties will be listed.

RETURNS
      The list of enumerable properties on the object.

DESCRIPTION
      Traverses the object retrieving its enumerable properties. The content of
      the returned list will depend on the object:

      - For a dictionary, returns the list of keys.
      - For an API object, returns the list of members.

      Behavior of the function passing other types of objects is undefined and
      also unsupported.

//@<OUT> WL13119-TSFR10_1: Validate that the help system contains an entry for the require() function.
NAME
      require - Loads the specified JavaScript module.

SYNTAX
      require(module_name_or_path)

WHERE
      module_name_or_path: The name or a path to the module to be loaded.

RETURNS
      The exported content of the loaded module.

DESCRIPTION
      The module_name_or_path parameter can be either a name of the built-in
      module (i.e. mysql or mysqlx) or a path to the JavaScript module on the
      local file system. The local module is searched for in the following
      folders:

      - if module_name_or_path begins with either './' or '../' characters, use
        the folder which contains the JavaScipt file or module which is
        currently being executed or the current working directory if there is
        no such file or module (i.e. shell is running in interactive mode),
      - folders listed in the sys.path variable.

      The file containing the module to be loaded is located by iterating
      through these folders, for each folder path:

      - append module_name_or_path, if such file exists, use it,
      - append module_name_or_path, append '.js' extension, if such file
        exists, use it,
      - append module_name_or_path, if such folder exists, append 'init.js'
        file name, if such file exists, use it.

      The loaded module has access to the following variables:

      - exports - an empty object, module should use it to export its
        functionalities; this is the value returned from the require()
        function,
      - module - a module object, contains the exports object described above;
        can be used to i.e. change the type of exports or store module-specific
        data,
      - __filename - absolute path to the module file,
      - __dirname - absolute path to the directory containing the module file.

      Each module is loaded only once, any subsequent call to require() which
      would use the same module will return a cached value instead.

      If two modules form a cycle (try to load each other using the require()
      function), one of them is going to receive an unfinished copy of the
      other ones exports object.

      Here is a sample module called test.js which stores some data in the
      module object and exports a function callme():
        module.counter = 0;

        exports.callme = function() {
          const plural = ++module.counter > 1 ? 's' : '';
          println(`I was called ${module.counter} time${plural}.`);
        };

      If placed in the current working directory, it can be used in shell as
      follows:
        mysql-js> var test = require('./test');
        mysql-js> test.callme();
        I was called 1 time.
        mysql-js> test.callme();
        I was called 2 times.
        mysql-js> test.callme();
        I was called 3 times.

EXCEPTIONS
      TypeError in the following scenarios:

      - if module_name_or_path is not a string.

      Error in the following scenarios:

      - if module_name_or_path is empty,
      - if module_name_or_path contains a backslash character,
      - if module_name_or_path is an absolute path,
      - if local module could not be found,
      - if local module could not be loaded.

//@<OUT> Help on X DevAPI Category
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
mysqlx.getSession(...).

For more details about the mysqlx module use: \? mysqlx

For more details about how to create a session with the X DevAPI use: \?
mysqlx.getSession

//@<OUT> Help on unknown topic
No help items found matching 'unknown'

//@<OUT> Help on topic with several matches
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

//@<OUT> Help for sandbox operations
Found several entries matching *sandbox*

The following topics were found at the AdminAPI category:

- dba.deleteSandboxInstance
- dba.deploySandboxInstance
- dba.killSandboxInstance
- dba.startSandboxInstance
- dba.stopSandboxInstance

For help on a specific topic use: \? <topic>

e.g.: \? dba.deleteSandboxInstance

//@<OUT> Help for SQL, with classic session, multiple matches
Found several entries matching select

The following topics were found at the SQL Syntax category:

- SQL Syntax/SELECT

The following topics were found at the X DevAPI category:

- mysqlx.Table.select
- mysqlx.TableSelect.select

For help on a specific topic use: \? <topic>

e.g.: \? SQL Syntax/SELECT

//@ Switching to SQL mode, same test gives results
|Syntax:|
|SELECT is used to retrieve rows selected from one or more tables|
|The most commonly used clauses of SELECT statements are these:|


//@ Switching back to JS, help for SQL Syntax
|The following topics were found at the SQL Syntax category:|
|- Account Management|
|For help on a specific topic use: \? <topic>|
|e.g.: \? Account Management|

//@ Help for SQL Syntax, with x session
|The following topics were found at the SQL Syntax category:|
|- Account Management|
|For help on a specific topic use: \? <topic>|
|e.g.: \? Account Management|

//@<OUT> Help for SQL Syntax, no connection
SQL help requires the Shell to be connected to a MySQL server.

//@<OUT> Help for API Command Line Integration
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

//@<OUT> Help using wildcard prefix prints single element found
NAME
      createClusterSet - Creates a MySQL InnoDB ClusterSet from an existing
                         standalone InnoDB Cluster.

SYNTAX
      <Cluster>.createClusterSet(domainName[, options])

WHERE
      domainName: An identifier for the ClusterSet's logical dataset.
      options: Dictionary with additional parameters described below.

RETURNS
      The created ClusterSet object.

DESCRIPTION
      Creates a ClusterSet object from an existing cluster, with the given data
      domain name.

      Several checks and validations are performed to ensure that the target
      Cluster complies with the requirements for ClusterSets and if so, the
      Metadata schema will be updated to create the new ClusterSet and the
      target Cluster becomes the PRIMARY cluster of the ClusterSet.

      InnoDB ClusterSet

      A ClusterSet is composed of a single PRIMARY InnoDB Cluster that can have
      one or more replica InnoDB Clusters that replicate from the PRIMARY using
      asynchronous replication.

      ClusterSets allow InnoDB Cluster deployments to achieve fault-tolerance
      at a whole Data Center / region or geographic location, by creating
      REPLICA clusters in different locations (Data Centers), ensuring Disaster
      Recovery is possible.

      If the PRIMARY InnoDB Cluster becomes completely unavailable, it's
      possible to promote a REPLICA of that cluster to take over its duties
      with minimal downtime or data loss.

      All Cluster operations are available at each individual member (cluster)
      of the ClusterSet. The AdminAPI ensures all updates are performed at the
      PRIMARY and controls the command availability depending on the individual
      status of each Cluster.

      Please note that InnoDB ClusterSets don't have the same consistency and
      data loss guarantees as InnoDB Clusters. To read more about ClusterSets,
      see \? ClusterSet or refer to the MySQL manual.

      Pre-requisites

      The following is a non-exhaustive list of requirements to create a
      ClusterSet:

      - The target cluster must not already be part of a ClusterSet.
      - MySQL 8.0.27 or newer.
      - The target cluster's Metadata schema version is 2.1.0 or newer.
      - Unmanaged replication channels are not allowed.

      Options

      The options dictionary can contain the following values:

      - dryRun: boolean if true, all validations and steps for creating a
        ClusterSet are executed, but no changes are actually made. An exception
        will be thrown when finished.
      - clusterSetReplicationSslMode: SSL mode for the ClusterSet replication
        channels.
      - replicationAllowedHost: string value to use as the host name part of
        internal replication accounts (i.e. 'mysql_innodb_cs_###'@'hostname').
        Default is %. It must be possible for any member of the ClusterSet to
        connect to any other member using accounts with this hostname value.

      The clusterSetReplicationSslMode option supports the following values:

      - REQUIRED: if used, SSL (encryption) will be enabled for the ClusterSet
        replication channels.
      - DISABLED: if used, SSL (encryption) will be disabled for the ClusterSet
        replication channels.
      - AUTO: if used, SSL (encryption) will be enabled if supported by the
        instance, otherwise disabled.

      If clusterSetReplicationSslMode is not specified AUTO will be used by
      default.

