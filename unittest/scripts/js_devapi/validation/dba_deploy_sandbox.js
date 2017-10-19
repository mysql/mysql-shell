//@ Initialization
||

//@<OUT> Deploy sandbox
A new MySQL sandbox instance will be created on this host in
<<<__sandbox_dir>>><<<__mysql_sandbox_port1>>>

Warning: Sandbox instances are only suitable for deploying and
running on your local machine for testing purposes and are not
accessible from external networks.

Please enter a MySQL root password for the new instance: Deploying new MySQL instance...

Instance <<<localhost>>>:<<<__mysql_sandbox_port1>>> successfully deployed and started.
Use shell.connect('root@localhost:<<<__mysql_sandbox_port1>>>'); to connect to the instance.

//@ Finalization
||