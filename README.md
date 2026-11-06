# MySQL Shell

Copyright (c) 2016, 2026, Oracle and/or its affiliates.

This is a release of MySQL Shell, an advanced client and code editor for MySQL.

License information can be found in the [LICENSE](LICENSE) file.
This distribution may include materials developed by third parties. For license
and attribution notices for these materials, please refer to the [LICENSE](LICENSE) file.

For more information on MySQL Shell visit https://dev.mysql.com/doc/mysql-shell/en/
For additional downloads and the source of MySQL Shell visit https://dev.mysql.com/downloads/shell/

Contributing information for code and non-code submissions can be found in the [CONTRIBUTING.md](CONTRIBUTING.md) file.

MySQL Shell is brought to you by the MySQL team at Oracle.

MySQL Shell is part of MySQL Server and provides an interactive console for **JavaScript**, **Python**, and **SQL**. It supports MySQL development and administration, and includes utilities for dump/load, upgrade readiness checks, and management of high-availability MySQL topologies.

## Key Features

### Dump and Load Utilities

MySQL Shell includes full-featured dump and load utilities to export MySQL instances and schemas to files and load them back efficiently.

Supported features include:

* Create and restore logical backups for MySQL
* Dump, load and copy entire MySQL instances, schemas and tables including users, stored procedures and functions
  * Multi-threaded and consistent
  * Include and exclude individual objects
  * Built-in support for compression and cloud storage in all major cloud providers
  * Optional checksums
* Migrate databases to the OCI MySQL HeatWave Service
* Dump and restore MySQL binary logs
* And more!

Use the built-in help for details on specific commands and options (see [Documentation](#documentation)).

### MySQL Upgrade Checker

MySQL Shell includes an upgrade readiness checker to help assess compatibility when upgrading MySQL Server versions.

### MySQL InnoDB Clusters and AdminAPI

MySQL Shell offers a management interface for MySQL clustering solutions through the **AdminAPI**, including:

- **MySQL InnoDB Cluster**
- **MySQL InnoDB ReplicaSet**
- **MySQL InnoDB ClusterSet**

MySQL InnoDB Cluster is a turnkey, integrated solution for high availability and horizontal scaling, built on MySQL replication technologies.
MySQL InnoDB ClusterSet is an integrated solution for disaster recovery, built on InnoDB Cluster.
MySQL InnoDB ReplicaSet is an integrated solution for replication management, providing simplified administration of asynchronous replication between MySQL instances.

MySQL Group Replication and Asynchronous Replication can be combined with **MySQL Router** to provide an end-to-end solution for distributed MySQL deployments.

The AdminAPI in MySQL Shell enables you to work with MySQL InnoDB Cluster, InnoDB ReplicaSet, and InnoDB ClusterSet, providing an integrated solution for high availability and scalability using InnoDB-based MySQL databases (differently from NDB Cluster) without requiring advanced MySQL expertise.

For full documentation on supported topologies/solutions:

- MySQL InnoDB Cluster:
  https://dev.mysql.com/doc/mysql-shell/en/mysql-innodb-cluster.html

- MySQL InnoDB ReplicaSets:
  https://dev.mysql.com/doc/mysql-shell/en/mysql-innodb-replicaset.html

- MySQL InnoDB ClusterSet:
  https://dev.mysql.com/doc/mysql-shell/en/innodb-clusterset.html

- MySQL Shell AdminAPI:
  https://dev.mysql.com/doc/mysql-shell/en/admin-api-overview.html

### MySQL REST Service Management

MySQL Shell provides a SQL based management interface for MRS, the **MySQL REST Service**.
MRS allows you to create zero-code REST endpoints for MySQL databases, served through a MySQL Router plugin.
For more information about MRS, see: https://dev.mysql.com/doc/dev/mysql-rest-service/latest/#what-is-the-mysql-rest-service

### MySQL Shell for Visual Studio Code

A GUI frontend for MySQL Shell is also available via the **MySQL Shell for VS Code** extension, which integrates MySQL Shell capabilities into the VS Code environment.

You may download it directly from VS Code by searching for "_MySQL Shell_" or from
https://marketplace.visualstudio.com/items?itemName=Oracle.mysql-shell-for-vs-code

## MySQL Server Version Compatibility

All MySQL users are strongly encouraged to always upgrade to the latest version of MySQL Shell, regardless of the MySQL Server version they're using.

The latest version of MySQL Shell is fully backwards compatible with all supported versions of MySQL Server (**26.x**, **9.x** and **8.4**).

MySQL Shell also supports older versions of MySQL (**8.0** and older) for a variety of specific purposes in a best-effort basis:

* Basic SQL query functionality (MySQL 5.7 and 8.0)
* Upgrade Checker (MySQL 5.7 and 8.0)
* Dump and load utilities (MySQL 5.6, 5.7 and 8.0)
  * Note: downgrades (loading dumps into older MySQL servers than the original) are not supported, although they might work in some cases.
* AdminAPI (MySQL 8.0)

## Downloads

Pre-compiled MySQL Shell binaries (and source packages) are available from:

https://dev.mysql.com/downloads/shell/

## Compiling from Source

Instructions for compiling MySQL Shell from source are available in:

- `INSTALL.md`

## Documentation

Full documentation for MySQL Shell:

https://dev.mysql.com/doc/mysql-shell/en/

API references:
- JavaScript API documentation:
  https://dev.mysql.com/doc/dev/mysqlsh-api-javascript/
- Python API documentation:
  https://dev.mysql.com/doc/dev/mysqlsh-api-python/
- MySQL Server documentation (reference manual):
  https://dev.mysql.com/doc/refman/en/

### Built-in Help

MySQL Shell includes a built-in help system. Reference documentation for most commands can be obtained with the `\h` or `\?` built-in command.

Example:

```text
MySQL JS > \h util.dumpInstance
NAME
      dumpInstance - Dumps the whole database to files in the output directory.

SYNTAX
      util.dumpInstance(outputUrl[, options])

WHERE
      outputUrl: Target directory to store the dump files.
      options: Dictionary with the dump options.
...
```

## Reporting Bugs

Please file bugs and feature requests at https://bugs.mysql.com
