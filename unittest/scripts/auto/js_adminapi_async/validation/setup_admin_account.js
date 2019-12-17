//@<ERR> WL#13536 TSFR3_1 An error is thrown if a non supported format is passed to the user parameter
ReplicaSet.setupAdminAccount: Invalid user syntax: Invalid syntax in string literal (ArgumentError)

//@<ERR> WL#13536 BUG#30645140 An error is thrown if the username contains the @ symbol
ReplicaSet.setupAdminAccount: Invalid user syntax: Invalid user name: foo@bar (ArgumentError)

//@ WL#13536 BUG#30645140 but no error is thrown if the @symbol on the username is surrounded by quotes
||

//@ WL#13536 BUG#30648813 Empty usernames are not supported
||ReplicaSet.setupAdminAccount: Invalid user syntax: User name must not be empty. (ArgumentError)
||ReplicaSet.setupAdminAccount: Invalid user syntax: User name must not be empty. (ArgumentError)
||ReplicaSet.setupAdminAccount: Invalid user syntax: User name must not be empty. (ArgumentError)
||ReplicaSet.setupAdminAccount: Invalid user syntax: User name must not be empty. (ArgumentError)
||ReplicaSet.setupAdminAccount: Invalid user syntax: User name must not be empty. (ArgumentError)
||ReplicaSet.setupAdminAccount: Invalid user syntax: User name must not be empty. (ArgumentError)

//@<OUT> WL#13536 TSFR3_2 check global privileges of created user {VER(>=8.0.11)}
+----------------------------+--------------+
| PRIVILEGE_TYPE             | IS_GRANTABLE |
+----------------------------+--------------+<<<(__version_num>=80017) ?  "\n| BACKUP_ADMIN               | YES          |\n| CLONE_ADMIN                | YES          |":"">>>
| CREATE USER                | YES          |<<<(__version_num>=80017) ?  "\n| EXECUTE                    | YES          |":"">>>
| FILE                       | YES          |
| PERSIST_RO_VARIABLES_ADMIN | YES          |
| PROCESS                    | YES          |
| RELOAD                     | YES          |
| REPLICATION CLIENT         | YES          |
| REPLICATION SLAVE          | YES          |<<<(__version_num>=80018) ?  "\n| REPLICATION_APPLIER        | YES          |":"">>>
| SELECT                     | YES          |
| SHUTDOWN                   | YES          |
| SUPER                      | YES          |
| SYSTEM_VARIABLES_ADMIN     | YES          |
+----------------------------+--------------+

//@<OUT> WL#13536 TSFR3_2 check schema privileges of created user
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

//@<OUT> WL#13536 TSFR3_2 check table privileges of created user
Empty set ([[*]])

//@<ERR> WL#13536 TSFR3_4 An error is thrown if user exists but update option is false
ReplicaSet.setupAdminAccount: Could not proceed with the operation because account specific_host@198.51.100.0/255.255.255.0 already exists. Enable the 'update' option to update the existing account's privileges. (RuntimeError)


//@<ERR> WL#13536 TSFR3_4 An error is thrown if user exists but update option is not specified
ReplicaSet.setupAdminAccount: Could not proceed with the operation because account specific_host@198.51.100.0/255.255.255.0 already exists. Enable the 'update' option to update the existing account's privileges. (RuntimeError)

//@ WL#13536 TSFR5_1 Validate upon creating a new account with dryRun the list of privileges is shown but the account is not created
|NOTE: dryRun option was specified. Validations will be executed, but no changes will be applied.|
|Creating user dryruntest@%.|
|GRANT DELETE, INSERT, UPDATE ON mysql.* TO 'dryruntest'@'%' WITH GRANT OPTION|
|dryRun finished.|

//@<ERR> WL#13536 TSFR5_2 Validate that trying to upgrade a non existing account fails even with dryRun enabled
ReplicaSet.setupAdminAccount: Could not proceed with the operation because account dryruntest@% does not exist and the 'update' option is enabled (RuntimeError)

//@ WL#13536 TSFR5_3 Validate upon updating an existing account with dryRun the list of privileges is shown but none is restored
|NOTE: dryRun option was specified. Validations will be executed, but no changes will be applied.|
|Updating user dryruntest@%.|
|GRANT DELETE, INSERT, UPDATE ON mysql.* TO 'dryruntest'@'%' WITH GRANT OPTION|
|dryRun finished.|

//@<ERR> WL#13536 TSFR5_4 Validate that upgrading an existing account fails if upgrade is false even with dryRun enabled
ReplicaSet.setupAdminAccount: Could not proceed with the operation because account dryruntest@% already exists. Enable the 'update' option to update the existing account's privileges. (RuntimeError)

//@<ERR> WL#13536 TSFR6_4 Creating new account fails if password not provided and interactive mode is disabled
ReplicaSet.setupAdminAccount: Could not proceed with the operation because no password was specified to create account interactive_test_2@%. Provide one using the 'password' option. (RuntimeError)

//@<ERR> WL#13536 TSET_6 Validate operation fails if user doesn't have enough privileges to create/upgrade account
ReplicaSet.setupAdminAccount: Account currently in use ('interactive_test'@'%') does not have enough privileges to execute the operation. (RuntimeError)
