//@# creation should fail without segmentation fault
|ERROR: The account 'ic'@'localhost' is missing privileges required to manage an InnoDB Cluster:|
|GRANT ALTER, ALTER ROUTINE, CREATE, CREATE ROUTINE, CREATE TEMPORARY TABLES, CREATE VIEW, DELETE, DROP, EVENT, EXECUTE, INDEX, INSERT, LOCK TABLES, REFERENCES, SHOW VIEW, TRIGGER, UPDATE ON mysql_innodb_cluster_metadata.* TO 'ic'@'localhost' WITH GRANT OPTION;|
|For more information, see the online documentation.|
||The account 'ic'@'localhost' is missing privileges required to manage an InnoDB Cluster. (RuntimeError)
