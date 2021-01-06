#@<OUT> CLI --help
The following objects provide command line operations:

   cli_tester
      CLI Integration Testing Plugin

   cluster
      Represents an InnoDB cluster.

   dba
      InnoDB cluster and replicaset management functions.

   rs
      Represents an InnoDB ReplicaSet.

   shell
      Gives access to general purpose functions and properties.

   util
      Global object that groups miscellaneous tools like upgrade checker and
      JSON import.

#@<OUT> CLI plugin --help
The following object provides command line operations at 'cli_tester':

   emptyChild
      Empty object, exposes no functions but child does.

The following operations are available at 'cli_tester':

   test
      Testing cmd line args.

#@<OUT> CLI plugin function --help
NAME
      test - Testing cmd line args.

SYNTAX
      cli_tester test <stritem> <strlist> --namedstr=<str> <options>

WHERE
      stritem: String - string parameter.
      strlist: Array - string list parameter.

OPTIONS
--namedstr=<str>
            String named parameter (for being after a list parameter).

--str=<str>
            String option.

--strlist=<str list>
            String list option.

#@<OUT> Interactive plugin function --help
ERROR: Invalid operation for cli_tester object: test-interactive

#@<OUT> Interactive plugin function help
NAME
      testInteractive - Testing interactive function.

SYNTAX
      cli_tester.testInteractive()

#@<OUT> CLI plugin nested child --help
The following object provides command line operations at 'cli_tester emptyChild':

   grandChild
      Grand child object exposing a function.

#@<OUT> CLI plugin nested grand child --help
The following operations are available at 'cli_tester emptyChild grandChild':

   grand-child-function
      Testing cmd line args in nested objects.

#@<OUT> CLI plugin nested grand child function --help
The following operations are available at 'cli_tester emptyChild grandChild':

   grand-child-function
      Testing cmd line args in nested objects.

#@<OUT> Test string list handling
String Parameter: 1,2,3,4,5
String List Parameter Item: 1
String List Parameter Item: 2
String List Parameter Item: 3
String List Parameter Item: 4
String List Parameter Item: 5
String Named Item: 1,2,3,4,5
String Option: 1,2,3,4,5
String List Option Item: 1
String List Option Item: 2
String List Option Item: 3
String List Option Item: 4
String List Option Item: 5

#@<OUT> Test string list quoted args for interpreter
String Parameter: 1,2,3,4,5
String List Parameter Item: 1,2
String List Parameter Item: 3
String List Parameter Item: 4,5
String Named Item: 1,2,3,4,5
String Option: 1,2,3,4,5
String List Option Item: 1,2
String List Option Item: 3
String List Option Item: 4,5

#@<OUT> Escaped comma in list parsing
String Parameter: 1,2,3,4,5
String List Parameter Item: 1,2
String List Parameter Item: 3
String List Parameter Item: 4,5
String Named Item: 1,2,3,4,5
String Option: 1,2,3,4,5
String List Option Item: 1,2
String List Option Item: 3
String List Option Item: 4,5

#@<OUT> Escaped quoting: \", \'
String Parameter: "1",2,3,4,5
String List Parameter Item: 1
String List Parameter Item: 2
String List Parameter Item: 3
String List Parameter Item: 4
String List Parameter Item: 5
String Named Item: 1,2,"3",4,5
String Option: 1,2,"3",4,5
String List Option Item: 1
String List Option Item: 2
String List Option Item: 3
String List Option Item: 4
String List Option Item: 5
String Parameter: '1',2,3,4,5
String List Parameter Item: 1
String List Parameter Item: 2
String List Parameter Item: 3
String List Parameter Item: 4
String List Parameter Item: 5
String Named Item: 1,2,'3',4,5
String Option: 1,2,'3',4,5
String List Option Item: 1
String List Option Item: 2
String List Option Item: 3
String List Option Item: 4
String List Option Item: 5

#@<OUT> Escaped equal: \=
String Parameter: =1,2,3,4,5
String List Parameter Item: 1=2
String List Parameter Item: 3
String List Parameter Item: 4=5
String Named Item: =1,2,3,4=5
String Option: =1,2,3,4=5
String List Option Item: 1=2
String List Option Item: 3
String List Option Item: 4=5

#@<OUT> CLI calling plugin nested grand child function
Unique Parameter: Success!!

