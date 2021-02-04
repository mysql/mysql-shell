//@# prepare {VER(>= 8.0.0)}
||

//@# invalid recoveryMethod (should fail) {VER(>= 8.0.0)}
||Invalid value for option recoveryMethod: foobar (ArgumentError)
||Option 'recoveryMethod' is expected to be of type String, but is Integer (TypeError)
||Invalid value for option recoveryMethod: foobar (ArgumentError)
||Option 'recoveryMethod' is expected to be of type String, but is Integer (TypeError)

//@ invalid cloneDonor (should fail) {VER(>= 8.0.0)}
||Option cloneDonor only allowed if option recoveryMethod is used and set to 'clone'. (ArgumentError)
||Option cloneDonor only allowed if option recoveryMethod is set to 'clone'. (ArgumentError)
||Invalid value for cloneDonor: Invalid address format in ':'. Must be <host>:<port> or [<ip>]:<port> for IPv6 addresses (ArgumentError)
||Invalid value for cloneDonor, string value cannot be empty. (ArgumentError)
||Invalid value for cloneDonor: Invalid address format in 'localhost:'. Must be <host>:<port> or [<ip>]:<port> for IPv6 addresses (ArgumentError)
||Invalid value for cloneDonor: Invalid address format in ':3306'. Must be <host>:<port> or [<ip>]:<port> for IPv6 addresses (ArgumentError)
||Option cloneDonor only allowed if option recoveryMethod is used and set to 'clone'. (ArgumentError)
||Option cloneDonor only allowed if option recoveryMethod is set to 'clone'. (ArgumentError)
||Invalid value for cloneDonor: Invalid address format in ':'. Must be <host>:<port> or [<ip>]:<port> for IPv6 addresses (ArgumentError)
||Invalid value for cloneDonor, string value cannot be empty. (ArgumentError)
||Invalid value for cloneDonor: Invalid address format in 'localhost:'. Must be <host>:<port> or [<ip>]:<port> for IPv6 addresses (ArgumentError)
||Invalid value for cloneDonor: Invalid address format in ':3306'. Must be <host>:<port> or [<ip>]:<port> for IPv6 addresses (ArgumentError)

//@ invalid recoveryMethod (should fail if target instance does not support it) {VER(>= 8.0.0) && VER(< 8.0.17)}
||Option 'recoveryMethod=clone' not supported on target server version: '<<<__version>>>' (RuntimeError)
||Option 'recoveryMethod=clone' not supported on target server version: '<<<__version>>>' (RuntimeError)

//@ addInstance: recoveryMethod:clone, interactive, make sure no prompts {VER(>=8.0.17)}
||debug (LogicError)

//@ rejoinInstance: recoveryMethod:clone, interactive, make sure no prompts {VER(>=8.0.17)}
||debug (LogicError)

//@ addInstance: recoveryMethod:clone, empty GTID -> clone {VER(>=8.0.17)}
|Clone based recovery selected through the recoveryMethod option|
||debug (LogicError)

//@ rejoinInstance: recoveryMethod:clone, empty GTID -> clone {VER(>=8.0.17)}
|Clone based recovery selected through the recoveryMethod option|
||debug (LogicError)


//@ addInstance: recoveryMethod:clone, empty GTIDs + gtidSetIsComplete -> clone {VER(>=8.0.17)}
|Clone based recovery selected through the recoveryMethod option|
||debug (LogicError)

//@ rejoinInstance: recoveryMethod:clone, empty GTIDs + gtidSetIsComplete -> clone {VER(>=8.0.17)}
|Clone based recovery selected through the recoveryMethod option|
||debug (LogicError)

//@ addInstance: recoveryMethod:clone, subset GTIDs -> clone {VER(>=8.0.17)}
|Clone based recovery selected through the recoveryMethod option|
||debug (LogicError)

//@ rejoinInstance: recoveryMethod:clone, subset GTIDs -> clone {VER(>=8.0.17)}
|Clone based recovery selected through the recoveryMethod option|
||debug (LogicError)

//@ addInstance: recoveryMethod:incremental, interactive, make sure no prompts {VER(>=8.0.0)}
|Adding instance to the replicaset...|
||
|* Performing validation checks|
||
|This instance reports its own address as <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>|
|<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>: Instance configuration is suitable.|
||
|* Checking async replication topology...|
||
|* Checking transaction state of the instance...|
||
|NOTE: The target instance '<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>' has not been pre-provisioned (GTID set is|
|empty). The Shell is unable to decide whether replication can completely|
|recover its state.|
||
|Incremental state recovery selected through the recoveryMethod option|

||debug (LogicError)

//@ rejoinInstance: recoveryMethod:incremental, interactive, error {VER(>=8.0.0)}
||Cannot use recoveryMethod=incremental option because the GTID state is not compatible or cannot be recovered. (RuntimeError)

//@ addInstance: recoveryMethod:incremental, empty GTIDs + gtidSetIsComplete -> incr {VER(>= 8.0.0)}
|Adding instance to the replicaset...|
||
|* Performing validation checks|
||
|This instance reports its own address as <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>|
|<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>: Instance configuration is suitable.|
||
|* Checking async replication topology...|
||
|* Checking transaction state of the instance...|
||
|NOTE: The target instance '<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>' has not been pre-provisioned (GTID set is|
|empty), but the replicaset was configured to assume that replication can|
|completely recover the state of new instances.|
||
|Incremental state recovery selected through the recoveryMethod option|
||debug (LogicError)

//@ rejoinInstance: recoveryMethod:incremental, empty GTIDs + gtidSetIsComplete -> incr {VER(>= 8.0.0)}
|* Validating instance...|
|** Checking transaction state of the instance...|
||
|NOTE: The target instance '<<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>' has not been pre-provisioned (GTID set is|
|empty), but the replicaset was configured to assume that replication can|
|completely recover the state of new instances.|
||
|Incremental state recovery selected through the recoveryMethod option|
||
|* Rejoining instance to replicaset...|
||debug (LogicError)

//@ addInstance: recoveryMethod:incremental, subset GTIDs -> incr {VER(>= 8.0.0)}
|Incremental state recovery selected through the recoveryMethod option|
||debug (LogicError)

//@ rejoinInstance: recoveryMethod:incremental, subset GTIDs -> incr {VER(>= 8.0.0)}
|Incremental state recovery selected through the recoveryMethod option|
||debug (LogicError)

//@ addInstance: recoveryMethod:incremental, errant GTIDs -> error {VER(>= 8.0.0)}
|WARNING: A GTID set check of the MySQL instance at '<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>' determined that it|
|contains transactions that do not originate from the replicaset, which must be|
|discarded before it can join the replicaset.|
||Cannot use recoveryMethod=incremental option because the GTID state is not compatible or cannot be recovered. (RuntimeError)

//@ rejoinInstance: recoveryMethod:incremental, errant GTIDs -> error {VER(>= 8.0.0)}
|WARNING: A GTID set check of the MySQL instance at '<<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>' determined that it|
|contains transactions that do not originate from the replicaset, which must be|
|discarded before it can join the replicaset.|
||Cannot use recoveryMethod=incremental option because the GTID state is not compatible or cannot be recovered. (RuntimeError)


//@ addInstance: recoveryMethod:auto, interactive, clone unavailable, empty GTID -> prompt i/a {VER(>= 8.0.0) && VER(< 8.0.17)}
|Adding instance to the replicaset...|
||
|* Performing validation checks|
||
|This instance reports its own address as <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>|
|<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>: Instance configuration is suitable.|
||
|* Checking async replication topology...|
||
|* Checking transaction state of the instance...|
||
|NOTE: The target instance '<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>' has not been pre-provisioned (GTID set is|
|empty). The Shell is unable to decide whether replication can completely|
|recover its state.|
|The safest and most convenient way to provision a new instance is through|
|automatic clone provisioning, which will completely overwrite the state of|
|'<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>' with a physical snapshot from an existing replicaset member.|
|To use this method by default, set the 'recoveryMethod' option to 'clone'.|
||
|WARNING: It should be safe to rely on replication to incrementally recover the state of|
|the new instance if you are sure all updates ever executed in the replicaset|
|were done with GTIDs enabled, there are no purged transactions and the new|
|instance contains the same GTID set as the replicaset or a subset of it. To use|
|this method by default, set the 'recoveryMethod' option to 'incremental'.|
||
||
|Please select a recovery method [I]ncremental recovery/[A]bort (default Incremental recovery):|

||Cancelled (RuntimeError)

//@ rejoinInstance: recoveryMethod:auto, interactive, clone unavailable, empty GTID -> error {VER(>= 8.0.0) && VER(< 8.0.17)}
|* Validating instance...|
|** Checking transaction state of the instance...|
||
|NOTE: The target instance '<<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>' has not been pre-provisioned (GTID set is|
|empty). The Shell is unable to decide whether replication can completely|
|recover its state.|
|The safest and most convenient way to provision a new instance is through|
|automatic clone provisioning, which will completely overwrite the state of|
|'<<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>' with a physical snapshot from an existing replicaset member.|
|To use this method by default, set the 'recoveryMethod' option to 'clone'.|
||
|WARNING: It should be safe to rely on replication to incrementally recover the state of|
|the new instance if you are sure all updates ever executed in the replicaset|
|were done with GTIDs enabled, there are no purged transactions and the new|
|instance contains the same GTID set as the replicaset or a subset of it. To use|
|this method by default, set the 'recoveryMethod' option to 'incremental'.|
||
|ERROR: The target instance must be either cloned or fully re-provisioned before it can be re-added to the target replicaset.|
|Built-in clone support is available starting with MySQL 8.0.17 and is the recommended method for provisioning instances.|
||Instance provisioning required (MYSQLSH 51153)

//@ addInstance: recoveryMethod:auto, interactive, empty GTID -> prompt c/i/a {VER(>= 8.0.19)}
|Adding instance to the replicaset...|
||
|* Performing validation checks|
||
|This instance reports its own address as <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>|
|<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>: Instance configuration is suitable.|
||
|* Checking async replication topology...|
||
|* Checking transaction state of the instance...|
||
|NOTE: The target instance '<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>' has not been pre-provisioned (GTID set is|
|empty). The Shell is unable to decide whether replication can completely|
|recover its state.|
|The safest and most convenient way to provision a new instance is through|
|automatic clone provisioning, which will completely overwrite the state of|
|'<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>' with a physical snapshot from an existing replicaset member.|
|To use this method by default, set the 'recoveryMethod' option to 'clone'.|
||
|WARNING: It should be safe to rely on replication to incrementally recover the state of|
|the new instance if you are sure all updates ever executed in the replicaset|
|were done with GTIDs enabled, there are no purged transactions and the new|
|instance contains the same GTID set as the replicaset or a subset of it. To use|
|this method by default, set the 'recoveryMethod' option to 'incremental'.|
||
||
|Please select a recovery method [C]lone/[I]ncremental recovery/[A]bort (default Clone):|

||debug (LogicError)

//@ rejoinInstance: recoveryMethod:auto, interactive, empty GTID -> prompt c/a {VER(>= 8.0.19)}
|* Validating instance...|
|** Checking transaction state of the instance...|
||
|NOTE: The target instance '<<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>' has not been pre-provisioned (GTID set is|
|empty). The Shell is unable to decide whether replication can completely|
|recover its state.|
|The safest and most convenient way to provision a new instance is through|
|automatic clone provisioning, which will completely overwrite the state of|
|'<<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>' with a physical snapshot from an existing replicaset member.|
|To use this method by default, set the 'recoveryMethod' option to 'clone'.|
||
|WARNING: It should be safe to rely on replication to incrementally recover the state of|
|the new instance if you are sure all updates ever executed in the replicaset|
|were done with GTIDs enabled, there are no purged transactions and the new|
|instance contains the same GTID set as the replicaset or a subset of it. To use|
|this method by default, set the 'recoveryMethod' option to 'incremental'.|
||
||
|Please select a recovery method [C]lone/[A]bort (default Abort):|

||debug (LogicError)

//@ addInstance: recoveryMethod:auto, interactive, empty GTIDs + gtidSetIsComplete -> incr {VER(>= 8.0.19)}
|Incremental state recovery was selected because it seems to be safely usable.|
||debug (LogicError)

//@ rejoinInstance: recoveryMethod:auto, interactive, empty GTIDs + gtidSetIsComplete -> incr {VER(>= 8.0.19)}
|Incremental state recovery was selected because it seems to be safely usable.|
||debug (LogicError)

//@ addInstance: recoveryMethod:auto, interactive, errant GTIDs -> prompt c/a {VER(>= 8.0.19)}
|Adding instance to the replicaset...|
||
|* Performing validation checks|
||
|This instance reports its own address as <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>|
|<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>: Instance configuration is suitable.|
||
|* Checking async replication topology...|
||
|* Checking transaction state of the instance...|
||
|WARNING: A GTID set check of the MySQL instance at '<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>' determined that it|
|contains transactions that do not originate from the replicaset, which must be|
|discarded before it can join the replicaset.|
||
|<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>> has the following errant GTIDs that do not exist in the replicaset:|
|00025721-1111-1111-1111-111111111111:1|
||
|WARNING: Discarding these extra GTID events can either be done manually or by completely|
|overwriting the state of <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>> with a physical snapshot from an|
|existing replicaset member. To use this method by default, set the|
|'recoveryMethod' option to 'clone'.|
||
|Having extra GTID events is not expected, and it is recommended to investigate|
|this further and ensure that the data can be removed prior to choosing the|
|clone recovery method.|
||
||
|Please select a recovery method [C]lone/[A]bort (default Abort): |
||Cancelled (RuntimeError)

//@ rejoinInstance: recoveryMethod:auto, interactive, errant GTIDs -> error {VER(>= 8.0.19)}
|* Validating instance...|
|** Checking transaction state of the instance...|
||
|WARNING: A GTID set check of the MySQL instance at '<<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>' determined that it|
|contains transactions that do not originate from the replicaset, which must be|
|discarded before it can join the replicaset.|
||
|<<<hostname_ip>>>:<<<__mysql_sandbox_port3>>> has the following errant GTIDs that do not exist in the replicaset:|
|00025721-1111-1111-1111-111111111111:1|
||
|WARNING: Discarding these extra GTID events can either be done manually or by completely|
|overwriting the state of <<<hostname_ip>>>:<<<__mysql_sandbox_port3>>> with a physical snapshot from an|
|existing replicaset member. To use this method by default, set the|
|'recoveryMethod' option to 'clone'.|
||
|Having extra GTID events is not expected, and it is recommended to investigate|
|this further and ensure that the data can be removed prior to choosing the|
|clone recovery method.|
||Instance provisioning required (MYSQLSH 51153)

//@ addInstance: recoveryMethod:auto, interactive, errant GTIDs -> error clone not supported {VER(>= 8.0.0) && VER(< 8.0.17)}
|Adding instance to the replicaset...|
||
|* Performing validation checks|
||
|This instance reports its own address as <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>|
|<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>: Instance configuration is suitable.|
||
|* Checking async replication topology...|
||
|* Checking transaction state of the instance...|
||
|WARNING: A GTID set check of the MySQL instance at '<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>' determined that it|
|contains transactions that do not originate from the replicaset, which must be|
|discarded before it can join the replicaset.|
||
|<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>> has the following errant GTIDs that do not exist in the replicaset:|
|00025721-1111-1111-1111-111111111111:1|
||
|WARNING: Discarding these extra GTID events can either be done manually or by completely|
|overwriting the state of <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>> with a physical snapshot from an|
|existing replicaset member. To use this method by default, set the|
|'recoveryMethod' option to 'clone'.|
||
|Having extra GTID events is not expected, and it is recommended to investigate|
|this further and ensure that the data can be removed prior to choosing the|
|clone recovery method.|
|ERROR: The target instance must be either cloned or fully provisioned before it can be added to the target replicaset.|
|Built-in clone support is available starting with MySQL 8.0.17 and is the recommended method for provisioning instances.|

||Instance provisioning required (MYSQLSH 51153)

//@ rejoinInstance: recoveryMethod:auto, interactive, errant GTIDs -> error clone not supported {VER(>= 8.0.0) && VER(< 8.0.17)}
||Instance provisioning required (MYSQLSH 51153)

//@ addInstance: recoveryMethod:auto, non-interactive, empty GTID -> error {VER(>= 8.0.19)}
|WARNING: It should be safe to rely on replication to incrementally recover the state of|
|the new instance if you are sure all updates ever executed in the replicaset|
|were done with GTIDs enabled, there are no purged transactions and the new|
|instance contains the same GTID set as the replicaset or a subset of it. To use|
|this method by default, set the 'recoveryMethod' option to 'incremental'.|

||'recoveryMethod' option must be set to 'clone' or 'incremental' (RuntimeError)

//@ rejoinInstance: recoveryMethod:auto, non-interactive, empty GTID -> error {VER(>= 8.0.19)}
||'recoveryMethod' option must be set to 'clone' or 'incremental' (RuntimeError)

//@ addInstance: recoveryMethod:auto, non-interactive, clone not supported, empty GTID -> error {VER(>= 8.0.0) && VER(< 8.0.17)}
|WARNING: It should be safe to rely on replication to incrementally recover the state of|
|the new instance if you are sure all updates ever executed in the replicaset|
|were done with GTIDs enabled, there are no purged transactions and the new|
|instance contains the same GTID set as the replicaset or a subset of it. To use|
|this method by default, set the 'recoveryMethod' option to 'incremental'.|

||'recoveryMethod' option must be set to 'incremental' (RuntimeError)

//@ rejoinInstance: recoveryMethod:auto, non-interactive, clone not supported, empty GTID -> error {VER(>= 8.0.0) && VER(< 8.0.17)}
||Instance provisioning required (MYSQLSH 51153)

//@ addInstance: recoveryMethod:auto, non-interactive, empty GTIDs + gtidSetIsComplete -> incr {VER(>=8.0.17)}
|Incremental state recovery was selected because it seems to be safely usable.|
||debug (LogicError)

//@ rejoinInstance: recoveryMethod:auto, non-interactive, empty GTIDs + gtidSetIsComplete -> incr {VER(>=8.0.17)}
|Incremental state recovery was selected because it seems to be safely usable.|
||debug (LogicError)


//@ addInstance: recoveryMethod:auto, non-interactive, subset GTIDs -> incr {VER(>=8.0.17)}
|Adding instance to the replicaset...|
||
|* Performing validation checks|
||
|This instance reports its own address as <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>|
|<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>: Instance configuration is suitable.|
||
|* Checking async replication topology...|
||
|* Checking transaction state of the instance...|
|The safest and most convenient way to provision a new instance is through|
|automatic clone provisioning, which will completely overwrite the state of|
|'<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>' with a physical snapshot from an existing replicaset member.|
|To use this method by default, set the 'recoveryMethod' option to 'clone'.|
||
|WARNING: It should be safe to rely on replication to incrementally recover the state of|
|the new instance if you are sure all updates ever executed in the replicaset|
|were done with GTIDs enabled, there are no purged transactions and the new|
|instance contains the same GTID set as the replicaset or a subset of it. To use|
|this method by default, set the 'recoveryMethod' option to 'incremental'.|
||
|Incremental state recovery was selected because it seems to be safely usable.|
||
||debug (LogicError)

//@ rejoinInstance: recoveryMethod:auto, non-interactive, subset GTIDs -> incr {VER(>=8.0.17)}
|Incremental state recovery was selected because it seems to be safely usable.|
||
||debug (LogicError)

//@ addInstance: recoveryMethod:auto, non-interactive, errant GTIDs -> error {VER(>=8.0.0)}
|<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>> has the following errant GTIDs that do not exist in the replicaset:|
|00025721-1111-1111-1111-111111111111:1|
||
|WARNING: Discarding these extra GTID events can either be done manually or by completely|
|overwriting the state of <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>> with a physical snapshot from an|
|existing replicaset member. To use this method by default, set the|
|'recoveryMethod' option to 'clone'.|
||
|Having extra GTID events is not expected, and it is recommended to investigate|
|this further and ensure that the data can be removed prior to choosing the|
|clone recovery method.|
|ERROR: The target instance must be either cloned or fully provisioned before it can be added to the target replicaset.|

//@ addInstance: recoveryMethod:auto, non-interactive, errant GTIDs -> error {VER(>=8.0.0) && VER(<8.0.17)}
|Built-in clone support is available starting with MySQL 8.0.17 and is the recommended method for provisioning instances.|

//@ addInstance: recoveryMethod:auto, non-interactive, errant GTIDs -> error {VER(>=8.0.0)}
||Instance provisioning required (MYSQLSH 51153)

//@ rejoinInstance: recoveryMethod:auto, non-interactive, errant GTIDs -> error {VER(>=8.0.0)}
|<<<hostname_ip>>>:<<<__mysql_sandbox_port3>>> has the following errant GTIDs that do not exist in the replicaset:|
|00025721-1111-1111-1111-111111111111:1|
||
|WARNING: Discarding these extra GTID events can either be done manually or by completely|
|overwriting the state of <<<hostname_ip>>>:<<<__mysql_sandbox_port3>>> with a physical snapshot from an|
|existing replicaset member. To use this method by default, set the|
|'recoveryMethod' option to 'clone'.|
||
|Having extra GTID events is not expected, and it is recommended to investigate|
|this further and ensure that the data can be removed prior to choosing the|
|clone recovery method.|
|ERROR: The target instance must be either cloned or fully re-provisioned before it can be re-added to the target replicaset.|

//@ rejoinInstance: recoveryMethod:auto, non-interactive, errant GTIDs -> error {VER(>=8.0.0) && VER(<8.0.17)}
|Built-in clone support is available starting with MySQL 8.0.17 and is the recommended method for provisioning instances.|

//@ rejoinInstance: recoveryMethod:auto, non-interactive, errant GTIDs -> error {VER(>=8.0.0)}
||Instance provisioning required (MYSQLSH 51153)

//@ addInstance: recoveryMethod:auto, interactive, purged GTID -> prompt c/a {VER(>= 8.0.0)}
|NOTE: A GTID set check of the MySQL instance at '<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>' determined that it|
|is missing transactions that were purged from all replicaset members.|
|NOTE: The target instance '<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>' has not been pre-provisioned (GTID set is|
|empty). The Shell is unable to decide whether clone based recovery is safe to use.|
//@ addInstance: recoveryMethod:auto, interactive, purged GTID -> prompt c/a {VER(>=8.0.17)}
||Cancelled (RuntimeError)
//@ addInstance: recoveryMethod:auto, interactive, purged GTID -> prompt c/a {VER(>= 8.0.0) && VER(< 8.0.17)}
||Instance provisioning required

//@ addInstance: recoveryMethod:auto, no-interactive, purged GTID -> error {VER(>=8.0.17)}
|NOTE: A GTID set check of the MySQL instance at '<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>' determined that it|
|is missing transactions that were purged from all replicaset members.|
|NOTE: The target instance '<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>' has not been pre-provisioned (GTID set is|
|empty). The Shell is unable to decide whether clone based recovery is safe to use.|
||Instance provisioning required

//@ addInstance: recoveryMethod:auto, interactive, purged GTID, subset gtid -> clone, no prompt {VER(>=8.0.17)}
|NOTE: A GTID set check of the MySQL instance at '<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>' determined that it|
|is missing transactions that were purged from all replicaset members.|
|Clone based recovery was selected because it seems to be safely usable.|
//@ addInstance: recoveryMethod:auto, interactive, purged GTID, subset gtid -> clone, no prompt {VER(>=8.0.17)}
||debug (LogicError)
//@ addInstance: recoveryMethod:auto, interactive, purged GTID, subset gtid -> clone, no prompt {VER(>= 8.0.0) && VER(< 8.0.17)}
||Instance provisioning required

//@ addInstance: recoveryMethod:auto, no-interactive, purged GTID, subset gtid -> clone, no prompt {VER(>=8.0.17)}
|NOTE: A GTID set check of the MySQL instance at '<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>' determined that it|
|is missing transactions that were purged from all replicaset members.|
|Clone based recovery was selected because it seems to be safely usable.|
//@ addInstance: recoveryMethod:auto, no-interactive, purged GTID, subset gtid -> clone, no prompt {VER(>=8.0.17)}
||debug (LogicError)
//@ addInstance: recoveryMethod:auto, no-interactive, purged GTID, subset gtid -> clone, no prompt {VER(>= 8.0.0) && VER(< 8.0.17)}
||Instance provisioning required

//@ rejoinInstance: recoveryMethod:auto, interactive, purged GTID -> prompt c/a {VER(>= 8.0.0)}
|NOTE: A GTID set check of the MySQL instance at '<<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>' determined that it|
|is missing transactions that were purged from all replicaset members.|
//@ rejoinInstance: recoveryMethod:auto, interactive, purged GTID -> prompt c/a {VER(>=8.0.17)}
||Cancelled (RuntimeError)
//@ rejoinInstance: recoveryMethod:auto, interactive, purged GTID -> prompt c/a {VER(>= 8.0.0) && VER(< 8.0.17)}
||Instance provisioning required

//@ rejoinInstance: recoveryMethod:auto, interactive, purged GTID, subset gtid -> clone, no prompt {VER(>=8.0.17)}
|NOTE: A GTID set check of the MySQL instance at '<<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>' determined that it|
|is missing transactions that were purged from all replicaset members.|
|Clone based recovery was selected because it seems to be safely usable.|
//@ rejoinInstance: recoveryMethod:auto, interactive, purged GTID, subset gtid -> clone, no prompt {VER(>=8.0.17)}
||debug (LogicError)
//@ rejoinInstance: recoveryMethod:auto, interactive, purged GTID, subset gtid -> clone, no prompt {VER(>= 8.0.0) && VER(< 8.0.17)}
||Instance provisioning required

//@ rejoinInstance: recoveryMethod:auto, no-interactive, purged GTID, subset gtid -> clone, no prompt {VER(>=8.0.17)}
|NOTE: A GTID set check of the MySQL instance at '<<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>' determined that it|
|is missing transactions that were purged from all replicaset members.|
|Clone based recovery was selected because it seems to be safely usable.|
//@ rejoinInstance: recoveryMethod:auto, no-interactive, purged GTID, subset gtid -> clone, no prompt {VER(>=8.0.17)}
||debug (LogicError)
//@ rejoinInstance: recoveryMethod:auto, no-interactive, purged GTID, subset gtid -> clone, no prompt {VER(>= 8.0.0) && VER(< 8.0.17)}
||Instance provisioning required

//@ addInstance: recoveryMethod:auto, interactive, errant GTIDs + purged GTIDs -> prompt c/a {VER(<8.0.17)}
|WARNING: A GTID set check of the MySQL instance at '<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>' determined that it|
|contains transactions that do not originate from the replicaset, which must be|
|discarded before it can join the replicaset.|
//@ addInstance: recoveryMethod:auto, interactive, errant GTIDs + purged GTIDs -> prompt c/a {VER(>=8.0.17)}
||Cancelled (RuntimeError)
//@ addInstance: recoveryMethod:auto, interactive, errant GTIDs + purged GTIDs -> prompt c/a {VER(>= 8.0.0) && VER(< 8.0.17)}
||Instance provisioning required

//@ rejoinInstance: recoveryMethod:auto, interactive, errant GTIDs + purged GTIDs -> error {VER(<8.0.17)}
|WARNING: A GTID set check of the MySQL instance at '<<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>' determined that it|
|contains transactions that do not originate from the replicaset, which must be|
|discarded before it can join the replicaset.|
//@ rejoinInstance: recoveryMethod:auto, interactive, errant GTIDs + purged GTIDs -> error
||Instance provisioning required (MYSQLSH 51153)

//@ addInstance: recoveryMethod:incremental, purged GTID -> error
|NOTE: A GTID set check of the MySQL instance at '<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>' determined that it|
|is missing transactions that were purged from all replicaset members.|
||Cannot use recoveryMethod=incremental option because the GTID state is not compatible or cannot be recovered. (RuntimeError)

//@ rejoinInstance: recoveryMethod:incremental, purged GTID -> error
|NOTE: A GTID set check of the MySQL instance at '<<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>' determined that it|
|is missing transactions that were purged from all replicaset members.|
||Cannot use recoveryMethod=incremental option because the GTID state is not compatible or cannot be recovered. (RuntimeError)

//@ addInstance: recoveryMethod:incremental, errant GTIDs + purged GTIDs -> error {VER(>= 8.0.0)}
||Cannot use recoveryMethod=incremental option because the GTID state is not compatible or cannot be recovered. (RuntimeError)

//@ rejoinInstance: recoveryMethod:incremental, errant GTIDs + purged GTIDs -> error {VER(>= 8.0.0)}
||Cannot use recoveryMethod=incremental option because the GTID state is not compatible or cannot be recovered. (RuntimeError)

//@ addInstance: recoveryMethod:clone, purged GTID -> clone {VER(>=8.0.17)}
|Clone based recovery selected through the recoveryMethod option|
||debug (LogicError)

//@ rejoinInstance: recoveryMethod:clone, purged GTID -> clone {VER(>=8.0.17)}
|Clone based recovery selected through the recoveryMethod option|
||debug (LogicError)

//@ addInstance: recoveryMethod:clone, errant GTIDs + purged GTIDs -> clone {VER(>=8.0.17)}
|Clone based recovery selected through the recoveryMethod option|
||debug (LogicError)

//@ rejoinInstance: recoveryMethod:clone, errant GTIDs + purged GTIDs -> clone {VER(>=8.0.17)}
|Clone based recovery selected through the recoveryMethod option|
||debug (LogicError)
