//@<OUT> help on os
NAME
      os - Gives access to functions which allow to interact with the operating
           system.

DESCRIPTION
      Gives access to functions which allow to interact with the operating
      system.

PROPERTIES
      path
            Gives access to path-related functions.

FUNCTIONS
      getcwd()
            Retrieves the absolute path of the current working directory.

      getenv(name)
            Retrieves the value of the specified environment variable.

      help([member])
            Provides help about this object and it's members

      loadTextFile(path)
            Reads the contents of a text file.

      sleep(seconds)
            Stops the execution for the given number of seconds.

//@<OUT> help on os.path
NAME
      path - Gives access to path-related functions.

SYNTAX
      os.path

DESCRIPTION
      Gives access to path-related functions.

FUNCTIONS
      basename(path)
            Provides the base name from the given path, by stripping the
            components before the last path-separator character.

      dirname(path)
            Provides the directory name from the given path, by stripping the
            component after the last path-separator character.

      help([member])
            Provides help about this object and it's members

      isabs(path)
            Checks if the given path is absolute.

      isdir(path)
            Checks if the given path exists and is a directory.

      isfile(path)
            Checks if the given path exists and is a file.

      join(pathA, pathB[, pathC][, pathD])
            Joins the path components using the path-separator character.

      normpath(path)
            Normalizes the given path, collapsing redundant path-separator
            characters and relative references.

//@<OUT> help on os.getcwd
NAME
      getcwd - Retrieves the absolute path of the current working directory.

SYNTAX
      os.getcwd()

RETURNS
      The absolute path of the current working directory.

//@<OUT> help on os.getenv
NAME
      getenv - Retrieves the value of the specified environment variable.

SYNTAX
      os.getenv(name)

WHERE
      name: The name of the environment variable.

RETURNS
      The value of environment variable, or null if given variable does not
      exist.

//@<OUT> help on os.loadTextFile
NAME
      loadTextFile - Reads the contents of a text file.

SYNTAX
      os.loadTextFile(path)

WHERE
      path: The path to the file to be read.

RETURNS
      The contents of the text file.

EXCEPTIONS
      RuntimeError If the specified file does not exist.

//@<OUT> help on os.sleep
NAME
      sleep - Stops the execution for the given number of seconds.

SYNTAX
      os.sleep(seconds)

WHERE
      seconds: The number of seconds to sleep for.
