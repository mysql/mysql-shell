//@# Testing require failures
||Module fail_wont_compile was not found
||my_object is not defined
||Module fail_empty is empty

//@ Testing require success
|Defined Members: 1|
|module imported ok|
|undefined|
|This is here because of JS|


//@# Stored sessions add errors
||ArgumentError: Invalid number of arguments in ShellRegistry.add, expected 2 to 3 but got 0
||ArgumentError: ShellRegistry.add: Argument #1 expected to be a string
||ArgumentError: ShellRegistry.add: Argument #2 expected to be either a URI or a connection data map
||ArgumentError: ShellRegistry.add: The name 'my sample' is not a valid identifier
||ArgumentError: ShellRegistry.add: The connection option host is mandatory
||ArgumentError: ShellRegistry.add: The connection option dbUser is mandatory
||ArgumentError: ShellRegistry.add: Argument #3 expected to be boolean


//@ Adding entry to the Stored sessions
|Added: true|
|Host: samplehost|
|Port: 44000|
|User: root|
|Password: pwd|
|Schema: sakila|


//@ Attempt to override connection without override
||ArgumentError: ShellRegistry.add: The name 'my_sample' already exists

//@ Attempt to override connection with override=false
||ArgumentError: ShellRegistry.add: The name 'my_sample' already exists

//@ Attempt to override connection with override=true
|Added: true|
|Host: localhost|
|User: admin|

//@# Stored sessions update errors
||ArgumentError: Invalid number of arguments in ShellRegistry.update, expected 2 but got 0
||ArgumentError: ShellRegistry.update: Argument #1 expected to be a string
||ArgumentError: ShellRegistry.update: Argument #2 expected to be either a URI or a connection data map
||ArgumentError: ShellRegistry.update: The name 'my sample' does not exist
||ArgumentError: ShellRegistry.update: The connection option host is mandatory
||ArgumentError: ShellRegistry.update: The connection option dbUser is mandatory

//@ Updates a connection
|Updated: true|
|User: guest|

//@# Stored sessions remove errors
||ArgumentError: Invalid number of arguments in ShellRegistry.remove, expected 1 but got 0
||ArgumentError: ShellRegistry.remove: Argument #1 expected to be a string
||ArgumentError: ShellRegistry.remove: The name 'my sample' does not exist

//@ Remove connection
|Removed: true|
