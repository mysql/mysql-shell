//@ create cluster admin
|Cluster admin user 'ca'@'%' created.|

//@<OUT> check global privileges of cluster admin {VER(<8.0.11)}
+--------------------+--------------+
| PRIVILEGE_TYPE     | IS_GRANTABLE |
+--------------------+--------------+
| CREATE USER        | YES          |
| FILE               | YES          |
| PROCESS            | YES          |
| RELOAD             | YES          |
| REPLICATION CLIENT | YES          |
| REPLICATION SLAVE  | YES          |
| SELECT             | YES          |
| SHUTDOWN           | YES          |
| SUPER              | YES          |
+--------------------+--------------+

//@<OUT> check global privileges of cluster admin {VER(>=8.0.11)}
+----------------------------+--------------+
| PRIVILEGE_TYPE             | IS_GRANTABLE |
+----------------------------+--------------+
| BACKUP_ADMIN               | YES          |
| CREATE USER                | YES          |
| FILE                       | YES          |
| PERSIST_RO_VARIABLES_ADMIN | YES          |
| PROCESS                    | YES          |
| RELOAD                     | YES          |
| REPLICATION CLIENT         | YES          |
| REPLICATION SLAVE          | YES          |
| SELECT                     | YES          |
| SHUTDOWN                   | YES          |
| SUPER                      | YES          |
| SYSTEM_VARIABLES_ADMIN     | YES          |
+----------------------------+--------------+

//@<OUT> check schema privileges of cluster admin
+-------------------------+--------------+----------------------------------------+
| PRIVILEGE_TYPE          | IS_GRANTABLE | TABLE_SCHEMA                           |
+-------------------------+--------------+----------------------------------------+
| DELETE                  | YES          | mysql                                  |
| INSERT                  | YES          | mysql                                  |
| UPDATE                  | YES          | mysql                                  |
| ALTER                   | YES          | mysql_innodb_cluster_metadata          |
| ALTER ROUTINE           | YES          | mysql_innodb_cluster_metadata          |
| CREATE                  | YES          | mysql_innodb_cluster_metadata          |
| CREATE ROUTINE          | YES          | mysql_innodb_cluster_metadata          |
| CREATE TEMPORARY TABLES | YES          | mysql_innodb_cluster_metadata          |
| CREATE VIEW             | YES          | mysql_innodb_cluster_metadata          |
| DELETE                  | YES          | mysql_innodb_cluster_metadata          |
| DROP                    | YES          | mysql_innodb_cluster_metadata          |
| EVENT                   | YES          | mysql_innodb_cluster_metadata          |
| EXECUTE                 | YES          | mysql_innodb_cluster_metadata          |
| INDEX                   | YES          | mysql_innodb_cluster_metadata          |
| INSERT                  | YES          | mysql_innodb_cluster_metadata          |
| LOCK TABLES             | YES          | mysql_innodb_cluster_metadata          |
| REFERENCES              | YES          | mysql_innodb_cluster_metadata          |
| SHOW VIEW               | YES          | mysql_innodb_cluster_metadata          |
| TRIGGER                 | YES          | mysql_innodb_cluster_metadata          |
| UPDATE                  | YES          | mysql_innodb_cluster_metadata          |
| ALTER                   | YES          | mysql_innodb_cluster_metadata_bkp      |
| ALTER ROUTINE           | YES          | mysql_innodb_cluster_metadata_bkp      |
| CREATE                  | YES          | mysql_innodb_cluster_metadata_bkp      |
| CREATE ROUTINE          | YES          | mysql_innodb_cluster_metadata_bkp      |
| CREATE TEMPORARY TABLES | YES          | mysql_innodb_cluster_metadata_bkp      |
| CREATE VIEW             | YES          | mysql_innodb_cluster_metadata_bkp      |
| DELETE                  | YES          | mysql_innodb_cluster_metadata_bkp      |
| DROP                    | YES          | mysql_innodb_cluster_metadata_bkp      |
| EVENT                   | YES          | mysql_innodb_cluster_metadata_bkp      |
| EXECUTE                 | YES          | mysql_innodb_cluster_metadata_bkp      |
| INDEX                   | YES          | mysql_innodb_cluster_metadata_bkp      |
| INSERT                  | YES          | mysql_innodb_cluster_metadata_bkp      |
| LOCK TABLES             | YES          | mysql_innodb_cluster_metadata_bkp      |
| REFERENCES              | YES          | mysql_innodb_cluster_metadata_bkp      |
| SHOW VIEW               | YES          | mysql_innodb_cluster_metadata_bkp      |
| TRIGGER                 | YES          | mysql_innodb_cluster_metadata_bkp      |
| UPDATE                  | YES          | mysql_innodb_cluster_metadata_bkp      |
| ALTER                   | YES          | mysql_innodb_cluster_metadata_previous |
| ALTER ROUTINE           | YES          | mysql_innodb_cluster_metadata_previous |
| CREATE                  | YES          | mysql_innodb_cluster_metadata_previous |
| CREATE ROUTINE          | YES          | mysql_innodb_cluster_metadata_previous |
| CREATE TEMPORARY TABLES | YES          | mysql_innodb_cluster_metadata_previous |
| CREATE VIEW             | YES          | mysql_innodb_cluster_metadata_previous |
| DELETE                  | YES          | mysql_innodb_cluster_metadata_previous |
| DROP                    | YES          | mysql_innodb_cluster_metadata_previous |
| EVENT                   | YES          | mysql_innodb_cluster_metadata_previous |
| EXECUTE                 | YES          | mysql_innodb_cluster_metadata_previous |
| INDEX                   | YES          | mysql_innodb_cluster_metadata_previous |
| INSERT                  | YES          | mysql_innodb_cluster_metadata_previous |
| LOCK TABLES             | YES          | mysql_innodb_cluster_metadata_previous |
| REFERENCES              | YES          | mysql_innodb_cluster_metadata_previous |
| SHOW VIEW               | YES          | mysql_innodb_cluster_metadata_previous |
| TRIGGER                 | YES          | mysql_innodb_cluster_metadata_previous |
| UPDATE                  | YES          | mysql_innodb_cluster_metadata_previous |
+-------------------------+--------------+----------------------------------------+


//@<OUT> check table privileges of cluster admin
Empty set ([[*]])

//@ cluster admin should be able to create another cluster admin
|Cluster admin user 'ca2'@'%' created.|
