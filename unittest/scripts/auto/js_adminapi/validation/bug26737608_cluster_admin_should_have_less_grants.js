//@ create cluster admin
|"status": "ok"|

//@<OUT> check global privileges of cluster admin
+--------------------+--------------+
| PRIVILEGE_TYPE     | IS_GRANTABLE |
+--------------------+--------------+
| CREATE USER        | YES          |
| FILE               | YES          |
| PROCESS            | YES          |
| RELOAD             | YES          |
| REPLICATION CLIENT | YES          |
| REPLICATION SLAVE  | YES          |
| SHUTDOWN           | YES          |
| SUPER              | YES          |
+--------------------+--------------+

//@<OUT> check schema privileges of cluster admin
+-------------------------+--------------+-------------------------------+
| PRIVILEGE_TYPE          | IS_GRANTABLE | TABLE_SCHEMA                  |
+-------------------------+--------------+-------------------------------+
| DELETE                  | YES          | mysql                         |
| INSERT                  | YES          | mysql                         |
| SELECT                  | YES          | mysql                         |
| UPDATE                  | YES          | mysql                         |
| ALTER                   | YES          | mysql_innodb_cluster_metadata |
| ALTER ROUTINE           | YES          | mysql_innodb_cluster_metadata |
| CREATE                  | YES          | mysql_innodb_cluster_metadata |
| CREATE ROUTINE          | YES          | mysql_innodb_cluster_metadata |
| CREATE TEMPORARY TABLES | YES          | mysql_innodb_cluster_metadata |
| CREATE VIEW             | YES          | mysql_innodb_cluster_metadata |
| DELETE                  | YES          | mysql_innodb_cluster_metadata |
| DROP                    | YES          | mysql_innodb_cluster_metadata |
| EVENT                   | YES          | mysql_innodb_cluster_metadata |
| EXECUTE                 | YES          | mysql_innodb_cluster_metadata |
| INDEX                   | YES          | mysql_innodb_cluster_metadata |
| INSERT                  | YES          | mysql_innodb_cluster_metadata |
| LOCK TABLES             | YES          | mysql_innodb_cluster_metadata |
| REFERENCES              | YES          | mysql_innodb_cluster_metadata |
| SELECT                  | YES          | mysql_innodb_cluster_metadata |
| SHOW VIEW               | YES          | mysql_innodb_cluster_metadata |
| TRIGGER                 | YES          | mysql_innodb_cluster_metadata |
| UPDATE                  | YES          | mysql_innodb_cluster_metadata |
| SELECT                  | YES          | sys                           |
+-------------------------+--------------+-------------------------------+

//@<OUT> check table privileges of cluster admin
+----------------+--------------+--------------------+-------------------------------------------+
| PRIVILEGE_TYPE | IS_GRANTABLE | TABLE_SCHEMA       | TABLE_NAME                                |
+----------------+--------------+--------------------+-------------------------------------------+
| SELECT         | YES          | performance_schema | replication_applier_configuration         |
| SELECT         | YES          | performance_schema | replication_applier_status                |
| SELECT         | YES          | performance_schema | replication_applier_status_by_coordinator |
| SELECT         | YES          | performance_schema | replication_applier_status_by_worker      |
| SELECT         | YES          | performance_schema | replication_connection_configuration      |
| SELECT         | YES          | performance_schema | replication_connection_status             |
| SELECT         | YES          | performance_schema | replication_group_members                 |
| SELECT         | YES          | performance_schema | replication_group_member_stats            |
| SELECT         | YES          | performance_schema | threads                                   |
+----------------+--------------+--------------------+-------------------------------------------+

//@ cluster admin should be able to create another cluster admin
|"status": "ok"|
