//@<OUT> Normal shell call
Oracle is a registered trademark of Oracle Corporation and/or its affiliates.
Other names may be trademarks of their respective owners.

Type '\help' or '\?' for help; '\quit' to exit.
WARNING: Using a password on the command line interface can be insecure.
Creating a session to '<<<__uri>>>/mysql'
Your MySQL connection id is [[*]] (X protocol)
Server version: [[*]]
Default schema `mysql` accessible through db.
1

//@<OUT> Using --quiet-start
WARNING: Using a password on the command line interface can be insecure.
Creating a session to '<<<__uri>>>/mysql'
Your MySQL connection id is [[*]] (X protocol)
Server version: [[*]]
Default schema `mysql` accessible through db.
1

//@ Using --quiet-start
|~Oracle is a registered trademark of Oracle Corporation and/or its affiliates.|

//@<OUT> Using --quiet-start=1
WARNING: Using a password on the command line interface can be insecure.
Creating a session to '<<<__uri>>>/mysql'
Your MySQL connection id is [[*]] (X protocol)
Server version: [[*]]
Default schema `mysql` accessible through db.
1

//@ Using --quiet-start=1
|~Oracle is a registered trademark of Oracle Corporation and/or its affiliates.|

//@<OUT> Using --quiet-start=2
1

//@ Using --quiet-start=2
|~Oracle is a registered trademark of Oracle Corporation and/or its affiliates.|
|~WARNING: Using a password on the command line interface can be insecure.|
|~Creating a session to '<<<__uri>>>/mysql'|
|~Default schema `mysql` accessible through db.|

//@<OUT> Normal shell call with error
Oracle is a registered trademark of Oracle Corporation and/or its affiliates.
Other names may be trademarks of their respective owners.

Type '\help' or '\?' for help; '\quit' to exit.
WARNING: Using a password on the command line interface can be insecure.
Creating a session to '<<<__uri>>>/unexisting'
MySQL Error {{1049|1045}}: Unknown database 'unexisting'

//@<OUT> Using --quiet-start with error
WARNING: Using a password on the command line interface can be insecure.
Creating a session to '<<<__uri>>>/unexisting'
MySQL Error {{1049|1045}}: Unknown database 'unexisting'

//@ Using --quiet-start with error
|~Oracle is a registered trademark of Oracle Corporation and/or its affiliates.|

//@<OUT> Using --quiet-start=1 with error
WARNING: Using a password on the command line interface can be insecure.
Creating a session to '<<<__uri>>>/unexisting'
MySQL Error {{1049|1045}}: Unknown database 'unexisting'

//@ Using --quiet-start=1 with error
|~Oracle is a registered trademark of Oracle Corporation and/or its affiliates.|


//@<OUT> Using --quiet-start=2 with error
MySQL Error {{1049|1045}}: Unknown database 'unexisting'

//@ Using --quiet-start=2 with error
|~Oracle is a registered trademark of Oracle Corporation and/or its affiliates.|
|~WARNING: Using a password on the command line interface can be insecure.|
|~Creating a session to '<<<__uri>>>/mysql'|
|~Default schema `mysql` accessible through db.|


