//@ Check Instance Configuration must work without a session
||Dba.checkInstanceConfiguration: Can't connect to MySQL server on 'localhost'

//@ First Sandbox
||

//@ Check Instance Configuration ok with a session
||

//@ Error: user has no privileges to run the checkInstanceConfiguration command (BUG#26609909)
||Dba.checkInstanceConfiguration: Account 'test_user'@'localhost' does not have all the required privileges to execute this operation. For more information, see the online documentation.
