//@# INCLUDE async_utils.inc
||

//@<OUT> master-slave from master
* Discovering async replication topology starting with <<<__address1>>>
Discovered topology:
- 127.0.0.1:<<<__mysql_sandbox_port1>>>: uuid=[[*]] read_only=no
- 127.0.0.1:<<<__mysql_sandbox_port2>>>: uuid=[[*]] read_only=no
    - replicates from 127.0.0.1:<<<__mysql_sandbox_port1>>>
	source="localhost:<<<__mysql_sandbox_port1>>>" channel= status=ON receiver=ON applier=ON

//@<OUT> master-slave from slave
* Discovering async replication topology starting with <<<__address2>>>
Discovered topology:
- 127.0.0.1:<<<__mysql_sandbox_port2>>>: uuid=[[*]] read_only=no
    - replicates from 127.0.0.1:<<<__mysql_sandbox_port1>>>
	source="localhost:<<<__mysql_sandbox_port1>>>" channel= status=ON receiver=ON applier=ON
- 127.0.0.1:<<<__mysql_sandbox_port1>>>: uuid=[[*]] read_only=no

//@<ERR> master-slave from slave
Dba.createReplicaSet: Target instance is not the PRIMARY (MYSQLSH 51313)

//@<OUT> master-(slave1,slave2) from master
* Discovering async replication topology starting with <<<__address1>>>
Discovered topology:
- 127.0.0.1:<<<__mysql_sandbox_port1>>>: uuid=[[*]] read_only=no
- 127.0.0.1:<<<__mysql_sandbox_port2>>>: uuid=[[*]] read_only=no
    - replicates from 127.0.0.1:<<<__mysql_sandbox_port1>>>
	source="localhost:<<<__mysql_sandbox_port1>>>" channel= status=ON receiver=ON applier=ON
- 127.0.0.1:<<<__mysql_sandbox_port3>>>: uuid=[[*]] read_only=no
    - replicates from 127.0.0.1:<<<__mysql_sandbox_port1>>>
	source="localhost:<<<__mysql_sandbox_port1>>>" channel= status=ON receiver=ON applier=ON

//@<OUT> master-(slave1,slave2) from slave2
* Discovering async replication topology starting with <<<__address3>>>
Discovered topology:
- 127.0.0.1:<<<__mysql_sandbox_port3>>>: uuid=[[*]] read_only=no
    - replicates from 127.0.0.1:<<<__mysql_sandbox_port1>>>
	source="localhost:<<<__mysql_sandbox_port1>>>" channel= status=ON receiver=ON applier=ON
- 127.0.0.1:<<<__mysql_sandbox_port1>>>: uuid=[[*]] read_only=no
- 127.0.0.1:<<<__mysql_sandbox_port2>>>: uuid=[[*]] read_only=no
    - replicates from 127.0.0.1:<<<__mysql_sandbox_port1>>>
	source="localhost:<<<__mysql_sandbox_port1>>>" channel= status=ON receiver=ON applier=ON

//@<ERR> master-(slave1,slave2) from slave2
Dba.createReplicaSet: Target instance is not the PRIMARY (MYSQLSH 51313)

//@<OUT> master-(slave1-slave11,slave2) from master
* Discovering async replication topology starting with <<<__address1>>>
Discovered topology:
- 127.0.0.1:<<<__mysql_sandbox_port1>>>: uuid=[[*]] read_only=no
- 127.0.0.1:<<<__mysql_sandbox_port2>>>: uuid=[[*]] read_only=no
    - replicates from 127.0.0.1:<<<__mysql_sandbox_port1>>>
	source="localhost:<<<__mysql_sandbox_port1>>>" channel= status=ON receiver=ON applier=ON
- 127.0.0.1:<<<__mysql_sandbox_port4>>>: uuid=[[*]] read_only=no
    - replicates from 127.0.0.1:<<<__mysql_sandbox_port2>>>
	source="localhost:<<<__mysql_sandbox_port2>>>" channel= status=ON receiver=ON applier=ON
- 127.0.0.1:<<<__mysql_sandbox_port3>>>: uuid=[[*]] read_only=no
    - replicates from 127.0.0.1:<<<__mysql_sandbox_port1>>>
	source="localhost:<<<__mysql_sandbox_port1>>>" channel= status=ON receiver=ON applier=ON

//@<ERR> master-(slave1-slave11,slave2) from master
Dba.createReplicaSet: Unsupported replication topology (MYSQLSH 51151)

//@<OUT> master-(slave1-slave11,slave2) from slave2
* Discovering async replication topology starting with <<<__address3>>>
Discovered topology:
- 127.0.0.1:<<<__mysql_sandbox_port3>>>: uuid=[[*]] read_only=no
    - replicates from 127.0.0.1:<<<__mysql_sandbox_port1>>>
	source="localhost:<<<__mysql_sandbox_port1>>>" channel= status=ON receiver=ON applier=ON
- 127.0.0.1:<<<__mysql_sandbox_port1>>>: uuid=[[*]] read_only=no
- 127.0.0.1:<<<__mysql_sandbox_port2>>>: uuid=[[*]] read_only=no
    - replicates from 127.0.0.1:<<<__mysql_sandbox_port1>>>
	source="localhost:<<<__mysql_sandbox_port1>>>" channel= status=ON receiver=ON applier=ON
- 127.0.0.1:<<<__mysql_sandbox_port4>>>: uuid=[[*]] read_only=no
    - replicates from 127.0.0.1:<<<__mysql_sandbox_port2>>>
	source="localhost:<<<__mysql_sandbox_port2>>>" channel= status=ON receiver=ON applier=ON

//@<ERR> master-(slave1-slave11,slave2) from slave2
Dba.createReplicaSet: Unsupported replication topology (MYSQLSH 51151)

//@<OUT> master-(slave1-slave11,slave2) from slave11
* Discovering async replication topology starting with <<<__address4>>>
Discovered topology:
- 127.0.0.1:<<<__mysql_sandbox_port4>>>: uuid=[[*]] read_only=no
    - replicates from 127.0.0.1:<<<__mysql_sandbox_port2>>>
	source="localhost:<<<__mysql_sandbox_port2>>>" channel= status=ON receiver=ON applier=ON
- 127.0.0.1:<<<__mysql_sandbox_port2>>>: uuid=[[*]] read_only=no
    - replicates from 127.0.0.1:<<<__mysql_sandbox_port1>>>
	source="localhost:<<<__mysql_sandbox_port1>>>" channel= status=ON receiver=ON applier=ON
- 127.0.0.1:<<<__mysql_sandbox_port1>>>: uuid=[[*]] read_only=no
- 127.0.0.1:<<<__mysql_sandbox_port3>>>: uuid=[[*]] read_only=no
    - replicates from 127.0.0.1:<<<__mysql_sandbox_port1>>>
	source="localhost:<<<__mysql_sandbox_port1>>>" channel= status=ON receiver=ON applier=ON

//@<ERR> master-(slave1-slave11,slave2) from slave11
Dba.createReplicaSet: Unsupported replication topology (MYSQLSH 51151)

//@<OUT> master1-master2 from master1
* Discovering async replication topology starting with <<<__address1>>>
Discovered topology:
- 127.0.0.1:<<<__mysql_sandbox_port1>>>: uuid=[[*]] read_only=no
    - replicates from 127.0.0.1:<<<__mysql_sandbox_port2>>>
	source="localhost:<<<__mysql_sandbox_port2>>>" channel= status=ON receiver=ON applier=ON
- 127.0.0.1:<<<__mysql_sandbox_port2>>>: uuid=[[*]] read_only=no
    - replicates from 127.0.0.1:<<<__mysql_sandbox_port1>>>
	source="localhost:<<<__mysql_sandbox_port1>>>" channel= status=ON receiver=ON applier=ON

//@<ERR> master1-master2 from master1
Dba.createReplicaSet: Unsupported replication topology (MYSQLSH 51151)

//@<OUT> master-master-master
* Discovering async replication topology starting with <<<__address1>>>
Discovered topology:
- 127.0.0.1:<<<__mysql_sandbox_port1>>>: uuid=[[*]] read_only=no
    - replicates from 127.0.0.1:<<<__mysql_sandbox_port3>>>
	source="localhost:<<<__mysql_sandbox_port3>>>" channel= status=ON receiver=ON applier=ON
- 127.0.0.1:<<<__mysql_sandbox_port3>>>: uuid=[[*]] read_only=no
    - replicates from 127.0.0.1:<<<__mysql_sandbox_port2>>>
	source="localhost:<<<__mysql_sandbox_port2>>>" channel= status=ON receiver=ON applier=ON
- 127.0.0.1:<<<__mysql_sandbox_port2>>>: uuid=[[*]] read_only=no
    - replicates from 127.0.0.1:<<<__mysql_sandbox_port1>>>
	source="localhost:<<<__mysql_sandbox_port1>>>" channel= status=ON receiver=ON applier=ON

//@<ERR> master-master-master
Dba.createReplicaSet: Unsupported replication topology (MYSQLSH 51151)

//@<OUT> masterA-slaveA from master
* Discovering async replication topology starting with <<<__address1>>>
Discovered topology:
- 127.0.0.1:<<<__mysql_sandbox_port1>>>: uuid=[[*]] read_only=no
    - has a replica 127.0.0.1:<<<__mysql_sandbox_port2>>> through an unsupported replication channel

//@<ERR> masterA-slaveA from master
Dba.createReplicaSet: Unsupported replication topology (MYSQLSH 51151)

//@<OUT> masterA-slaveA from slave
* Discovering async replication topology starting with <<<__address2>>>
Discovered topology:
- 127.0.0.1:<<<__mysql_sandbox_port2>>>: uuid=[[*]] read_only=no
    - replicates from localhost:<<<__mysql_sandbox_port1>>> through unsupported channel 'chan'

//@<ERR> masterA-slaveA from slave
Dba.createReplicaSet: Unsupported replication topology (MYSQLSH 51151)

//@<OUT> master-(slave,slaveB) from master
* Discovering async replication topology starting with <<<__address1>>>
Discovered topology:
- 127.0.0.1:<<<__mysql_sandbox_port1>>>: uuid=[[*]] read_only=no
    - has a replica 127.0.0.1:<<<__mysql_sandbox_port2>>> through an unsupported replication channel
- 127.0.0.1:<<<__mysql_sandbox_port3>>>: uuid=[[*]] read_only=no
    - replicates from 127.0.0.1:<<<__mysql_sandbox_port1>>>
	source="localhost:<<<__mysql_sandbox_port1>>>" channel= status=ON receiver=ON applier=ON

//@<ERR> master-(slave,slaveB) from master
Dba.createReplicaSet: Unsupported replication topology (MYSQLSH 51151)

//@<OUT> master-(slave,slaveB) from slave
* Discovering async replication topology starting with <<<__address3>>>
Discovered topology:
- 127.0.0.1:<<<__mysql_sandbox_port3>>>: uuid=[[*]] read_only=no
    - replicates from 127.0.0.1:<<<__mysql_sandbox_port1>>>
	source="localhost:<<<__mysql_sandbox_port1>>>" channel= status=ON receiver=ON applier=ON
- 127.0.0.1:<<<__mysql_sandbox_port1>>>: uuid=[[*]] read_only=no
    - has a replica 127.0.0.1:<<<__mysql_sandbox_port2>>> through an unsupported replication channel

//@<ERR> master-(slave,slaveB) from slave
Dba.createReplicaSet: Unsupported replication topology (MYSQLSH 51151)

//@<OUT> master-(slave,slaveB) from slaveB
* Discovering async replication topology starting with <<<__address2>>>
Discovered topology:
- 127.0.0.1:<<<__mysql_sandbox_port2>>>: uuid=[[*]] read_only=no
    - replicates from localhost:<<<__mysql_sandbox_port1>>> through unsupported channel 'chan'

//@<ERR> master-(slave,slaveB) from slaveB
Dba.createReplicaSet: Unsupported replication topology (MYSQLSH 51151)

//@<OUT> masterA-slaveB-slave from slaveB
* Discovering async replication topology starting with <<<__address2>>>
Discovered topology:
- 127.0.0.1:<<<__mysql_sandbox_port2>>>: uuid=[[*]] read_only=no
    - replicates from localhost:<<<__mysql_sandbox_port1>>> through unsupported channel 'chan'
- 127.0.0.1:<<<__mysql_sandbox_port3>>>: uuid=[[*]] read_only=no
    - replicates from 127.0.0.1:<<<__mysql_sandbox_port2>>>
	source="localhost:<<<__mysql_sandbox_port2>>>" channel= status=ON receiver=ON applier=ON

//@<ERR> masterA-slaveB-slave from slaveB
Dba.createReplicaSet: Unsupported replication topology (MYSQLSH 51151)

//@<OUT> masterA-slaveB-slave from slave
* Discovering async replication topology starting with <<<__address3>>>
Discovered topology:
- 127.0.0.1:<<<__mysql_sandbox_port3>>>: uuid=[[*]] read_only=no
    - replicates from 127.0.0.1:<<<__mysql_sandbox_port2>>>
	source="localhost:<<<__mysql_sandbox_port2>>>" channel= status=ON receiver=ON applier=ON
- 127.0.0.1:<<<__mysql_sandbox_port2>>>: uuid=[[*]] read_only=no
    - replicates from localhost:<<<__mysql_sandbox_port1>>> through unsupported channel 'chan'

//@<ERR> masterA-slaveB-slave from slave
Dba.createReplicaSet: Unsupported replication topology (MYSQLSH 51151)
