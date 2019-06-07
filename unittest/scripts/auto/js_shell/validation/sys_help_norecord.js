//@<OUT> Help on sys
NAME
      sys - Gives access to system specific parameters.

DESCRIPTION
      Gives access to system specific parameters.

PROPERTIES
      argv
            Contains the list of arguments available during a script
            processing.

      path
            Lists the search paths to load JavaScript modules.

FUNCTIONS
      help([member])
            Provides help about this object and it's members

//@<OUT> Help on argv
NAME
      argv - Contains the list of arguments available during a script
             processing.

SYNTAX
      sys.argv

DESCRIPTION
      When the shell is used to execute a script, the arguments passed to the
      shell after the script name will be considered script arguments and will
      be available during the script execution at sys.argv which will contain:

      - The first element is the path to the script being executed.
      - Each script argument will be addded as an element of the array.

      For example, given the following call:
      $ mysqlsh root@localhost/sakila -f test.js first second 3

      The sys.argv array will contain:

      - sys.argv[0]: "test.js"
      - sys.argv[1]: "first"
      - sys.argv[2]: "second"
      - sys.argv[3]: "3"

//@<OUT> Help on path
NAME
      path - Lists the search paths to load JavaScript modules.

SYNTAX
      sys.path

