#@<OUT> Sandbox Deployment
[[*]]) Check for invalid table names and schema names used in 5.7
  No issues found

#@<OUT> Invalid Name Check
[[*]]) Check for invalid table names and schema names used in 5.7
  The following tables and/or schemas have invalid names. In order to fix them
    use the mysqlcheck utility as follows:
    
      $ mysqlcheck --check-upgrade --all-databases
      $ mysqlcheck --fix-db-names --fix-table-names --all-databases
    
    OR via mysql client, for eg:
    
      ALTER DATABASE `#mysql50#lost+found` UPGRADE DATA DIRECTORY NAME;
  More information:
    https://dev.mysql.com/doc/refman/5.7/en/identifier-mapping.html
    https://dev.mysql.com/doc/refman/5.7/en/alter-database.html
    https://dev.mysql.com/doc/refman/8.0/en/mysql-nutshell.html#mysql-nutshell-removals

  #mysql50#lost+found - Schema name
  UCTEST1.#mysql50#lost+found - Table name

