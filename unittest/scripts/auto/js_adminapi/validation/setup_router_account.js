//@<ERR> WL#13536 TSFR3_7 An error is thrown if a non supported format is passed to the user parameter
Cluster.setupRouterAccount: Invalid user syntax: Invalid syntax in string literal (ArgumentError)

//@<ERR> WL#13536 BUG#30645140 An error is thrown if the username contains the @ symbol
Cluster.setupRouterAccount: Invalid user syntax: Invalid user name: foo@bar (ArgumentError)

//@ WL#13536 BUG#30645140 but no error is thrown if the @symbol on the username is surrounded by quotes
||

//@ WL#13536 BUG#30648813 Empty usernames are not supported
||Invalid user syntax: User name must not be empty. (ArgumentError)
||Invalid user syntax: User name must not be empty. (ArgumentError)
||Invalid user syntax: User name must not be empty. (ArgumentError)
||Invalid user syntax: User name must not be empty. (ArgumentError)
||Invalid user syntax: User name must not be empty. (ArgumentError)
||Invalid user syntax: User name must not be empty. (ArgumentError)

//@<OUT> WL#13536 TSFR3_8 check global privileges of created user
| PRIVILEGE_TYPE | IS_GRANTABLE |
+----------------+--------------+
| USAGE          | NO           |
+----------------+--------------+

//@<OUT> WL#13536 TSFR3_8 check schema privileges of created user
+----------------+--------------+-------------------------------+
| PRIVILEGE_TYPE | IS_GRANTABLE | TABLE_SCHEMA                  |
+----------------+--------------+-------------------------------+
| EXECUTE        | NO           | mysql_innodb_cluster_metadata |
| SELECT         | NO           | mysql_innodb_cluster_metadata |
+----------------+--------------+-------------------------------+


//@<OUT> WL#13536 TSFR3_8 check table privileges of created user
+----------------+--------------+-------------------------------+--------------------------------+
| PRIVILEGE_TYPE | IS_GRANTABLE | TABLE_SCHEMA                  | TABLE_NAME                     |
+----------------+--------------+-------------------------------+--------------------------------+
| DELETE         | NO           | mysql_innodb_cluster_metadata | routers                        |
| INSERT         | NO           | mysql_innodb_cluster_metadata | routers                        |
| UPDATE         | NO           | mysql_innodb_cluster_metadata | routers                        |
| DELETE         | NO           | mysql_innodb_cluster_metadata | v2_routers                     |
| INSERT         | NO           | mysql_innodb_cluster_metadata | v2_routers                     |
| UPDATE         | NO           | mysql_innodb_cluster_metadata | v2_routers                     |
| SELECT         | NO           | performance_schema            | global_variables               |
| SELECT         | NO           | performance_schema            | replication_group_members      |
| SELECT         | NO           | performance_schema            | replication_group_member_stats |
+----------------+--------------+-------------------------------+--------------------------------+

//@<ERR> WL#13536 TSFR3_10 An error is thrown if user exists but update option is false
Cluster.setupRouterAccount: Could not proceed with the operation because account specific_host@198.51.100.0/255.255.255.0 already exists. Enable the 'update' option to update the existing account's privileges. (RuntimeError)

//@<ERR> WL#13536 TSFR3_10 An error is thrown if user exists but update option is not specified
Cluster.setupRouterAccount: Could not proceed with the operation because account specific_host@198.51.100.0/255.255.255.0 already exists. Enable the 'update' option to update the existing account's privileges. (RuntimeError)

//@ WL#13536 TSFR5_5 Validate upon creating a new account with dryRun the list of privileges is shown but the account is not created
|NOTE: dryRun option was specified. Validations will be executed, but no changes will be applied.|
|Creating user dryruntest@%.|
|The following grants would be executed:|
|GRANT EXECUTE, SELECT ON mysql_innodb_cluster_metadata.* TO 'dryruntest'@'%'|
|GRANT DELETE, INSERT, UPDATE ON mysql_innodb_cluster_metadata.routers TO 'dryruntest'@'%'|
|GRANT DELETE, INSERT, UPDATE ON mysql_innodb_cluster_metadata.v2_routers TO 'dryruntest'@'%'|
|GRANT SELECT ON performance_schema.global_variables TO 'dryruntest'@'%'|
|GRANT SELECT ON performance_schema.replication_group_member_stats TO 'dryruntest'@'%'|
|GRANT SELECT ON performance_schema.replication_group_members TO 'dryruntest'@'%'|
|Account dryruntest@% was successfully created.|
|dryRun finished.|

//@<ERR> WL#13536 TSFR5_6 Validate that trying to upgrade a non existing account fails even with dryRun enabled
Cluster.setupRouterAccount: Could not proceed with the operation because account dryruntest@% does not exist and the 'update' option is enabled (RuntimeError)

//@ WL#13536 TSFR5_7 Validate upon updating an existing account with dryRun the list of privileges is shown but none is restored
|NOTE: dryRun option was specified. Validations will be executed, but no changes will be applied.|
|Updating user dryruntest@%.|
|GRANT SELECT ON performance_schema.replication_group_members TO 'dryruntest'@'%'|
|dryRun finished.|

//@<ERR> WL#13536 TSFR5_8 Validate that upgrading an existing account fails if upgrade is false even with dryRun enabled
Cluster.setupRouterAccount: Could not proceed with the operation because account dryruntest@% already exists. Enable the 'update' option to update the existing account's privileges. (RuntimeError)

//@<ERR> WL#13536 TSFR6_8 Creating new account fails if password not provided and interactive mode is disabled
Cluster.setupRouterAccount: Could not proceed with the operation because no password was specified to create account interactive_test_2@%. Provide one using the 'password' option. (RuntimeError)

//@<ERR> WL#13536 TSET_6 Validate operation fails if user doesn't have enough privileges to create/upgrade account
Cluster.setupRouterAccount: Account currently in use ('almost_admin_user'@'%') does not have enough privileges to execute the operation. (RuntimeError)
