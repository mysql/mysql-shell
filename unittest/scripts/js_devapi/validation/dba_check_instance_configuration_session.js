//@ Check Instance Configuration must work without a session
||Can't connect to MySQL server on 'localhost'

//@ First Sandbox
||

//@ Check Instance Configuration ok with a session
||

//@ Error: user has no privileges to run the checkInstanceConfiguration command (BUG#26609909)
||Dba.checkInstanceConfiguration: Session account 'test_user'@'%' (used to authenticate 'test_user'@'<<<localhost>>>') does not have all the required privileges to execute this operation. For more information, see the online documentation.

//@ Check instance configuration using a non existing user that authenticates as another user that does not have enough privileges (BUG#26979375)
||Dba.checkInstanceConfiguration: Session account 'test_user'@'%' (used to authenticate 'test_user'@'<<<hostname>>>') does not have all the required privileges to execute this operation. For more information, see the online documentation.

//@ Check instance configuration using a non existing user that authenticates as another user that has all privileges (BUG#26979375)
||
