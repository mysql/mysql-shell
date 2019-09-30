//@<OUT> help on path.basename
NAME
      basename - Provides the base name from the given path, by stripping the
                 components before the last path-separator character.

SYNTAX
      os.path.basename(path)

WHERE
      path: A file-system path.

RETURNS
      The base name of the given path.

//@<OUT> help on path.dirname
NAME
      dirname - Provides the directory name from the given path, by stripping
                the component after the last path-separator character.

SYNTAX
      os.path.dirname(path)

WHERE
      path: A file-system path.

RETURNS
      The directory name of the given path.

//@<OUT> help on path.isabs
NAME
      isabs - Checks if the given path is absolute.

SYNTAX
      os.path.isabs(path)

WHERE
      path: A file-system path.

RETURNS
      true if the given path is absolute.

//@<OUT> help on path.isdir
NAME
      isdir - Checks if the given path exists and is a directory.

SYNTAX
      os.path.isdir(path)

WHERE
      path: A file-system path.

RETURNS
      true if path points to an existing directory.

//@<OUT> help on path.isfile
NAME
      isfile - Checks if the given path exists and is a file.

SYNTAX
      os.path.isfile(path)

WHERE
      path: A file-system path.

RETURNS
      true if path points to an existing file.

//@<OUT> help on path.join
NAME
      join - Joins the path components using the path-separator character.

SYNTAX
      os.path.join(pathA, pathB[, pathC][, pathD])

WHERE
      pathA: A file-system path component.
      pathB: A file-system path component.
      pathC: A file-system path component.
      pathD: A file-system path component.

RETURNS
      The joined path.

DESCRIPTION
      If a component is an absolute path, the current result is discarded and
      replaced with the current component.

//@<OUT> help on path.normpath
NAME
      normpath - Normalizes the given path, collapsing redundant path-separator
                 characters and relative references.

SYNTAX
      os.path.normpath(path)

WHERE
      path: A file-system path component.

RETURNS
      The normalized path.
