//@# creation should fail without segmentation fault
|ERROR: The account 'ic'@'localhost' is missing privileges required to manage an InnoDB cluster:|
|Missing privileges on schema 'mysql_innodb_cluster_metadata': ALTER, ALTER ROUTINE, CREATE, CREATE ROUTINE, CREATE TEMPORARY TABLES, CREATE VIEW, DELETE, DROP, EVENT, EXECUTE, INDEX, INSERT, LOCK TABLES, REFERENCES, SHOW VIEW, TRIGGER, UPDATE.|
|For more information, see the online documentation.|
||Dba.createCluster: The account 'ic'@'localhost' is missing privileges required to manage an InnoDB cluster. (RuntimeError)
