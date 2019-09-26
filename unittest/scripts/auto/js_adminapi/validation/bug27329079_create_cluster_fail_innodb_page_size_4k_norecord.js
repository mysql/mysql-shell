//@ Deploy instances (with specific innodb_page_size).
||

//@ checkInstanceConfiguration error with innodb_page_size=4k.
|ERROR: Instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' is using a non-supported InnoDB page size (innodb_page_size=4096). Only instances with innodb_page_size greater than 4k (4096) can be used with InnoDB Cluster.|Dba.checkInstanceConfiguration: Unsupported innodb_page_size value: 4096 (RuntimeError)

//@ configureLocalInstance error with innodb_page_size=4k.
|ERROR: Instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' is using a non-supported InnoDB page size (innodb_page_size=4096). Only instances with innodb_page_size greater than 4k (4096) can be used with InnoDB Cluster.|Dba.configureLocalInstance: Unsupported innodb_page_size value: 4096 (RuntimeError)

//@ create cluster fails with nice error if innodb_page_size=4k
|ERROR: Instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' is using a non-supported InnoDB page size (innodb_page_size=4096). Only instances with innodb_page_size greater than 4k (4096) can be used with InnoDB Cluster.|Dba.createCluster: Unsupported innodb_page_size value: 4096 (RuntimeError)

//@ create cluster works with innodb_page_size=8k (> 4k)
||

//@ Clean-up deployed instances.
||

