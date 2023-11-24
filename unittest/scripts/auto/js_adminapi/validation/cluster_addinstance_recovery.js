//@# Setup
||

//@# prepare
||

//@# bogus recoveryMethod (should fail)
||Invalid value for option recoveryMethod: foobar (ArgumentError)

//@ bogus recoveryMethod (should fail if target instance does not support it) {VER(<8.0.17)}
||Option 'recoveryMethod=clone' not supported on target server version: '<<<__version>>>' (RuntimeError)

//@ Ensure clone enabled on all cluster members on clone recovery {VER(>=8.0.17)}
||debug (LogicError)

//@ Ensure clone enabled on all cluster members on clone recovery when cluster created with old shell {VER(>=8.0.17)}
||debug (LogicError)

//@# recoveryMethod:auto, interactive, empty GTID -> prompt c/i/a {VER(>=8.0.17)}
|NOTE: The target instance '<<<__address2>>>' has not been pre-provisioned (GTID set is|
|empty). The Shell is unable to decide whether incremental state|
|recovery can correctly provision it.|
|The safest and most convenient way to provision a new instance is through|
|automatic clone provisioning, which will completely overwrite the state of|
|'<<<__address2>>>' with a physical snapshot from an existing cluster member. To|
|use this method by default, set the 'recoveryMethod' option to 'clone'.|
||
|The incremental state recovery may be safely used if you are sure|
|all updates ever executed in the cluster were done with GTIDs enabled, there|
|are no purged transactions and the new instance contains the same GTID set as|
|the cluster or a subset of it. To use this method by default, set the|
|'recoveryMethod' option to 'incremental'.|
||
||
|Please select a recovery method [C]lone/[I]ncremental recovery/[A]bort (default Clone):|

|Validating instance configuration at localhost:<<<__mysql_sandbox_port2>>>...|

||debug (LogicError)

//@# recoveryMethod:auto, interactive, cloneDisabled, empty GTID -> prompt i/a
|Please select a recovery method [I]ncremental recovery/[A]bort (default Incremental recovery): |
||Cancelled (RuntimeError)

//@# recoveryMethod:auto, interactive, empty GTIDs + gtidSetIsComplete -> incr
|Incremental state recovery was selected because it seems to be safely usable.|
||debug (LogicError)

//@# recoveryMethod:auto, interactive, subset GTIDs -> incr {VER(>=8.0.17)}
|Incremental state recovery was selected because it seems to be safely usable.|
||debug (LogicError)

//@# recoveryMethod:auto, interactive, errant GTIDs -> prompt c/a {VER(>=8.0.17)}
|WARNING: A GTID set check of the MySQL instance at '<<<__address2>>>' determined that it|
|contains transactions that do not originate from the cluster, which must be|
|discarded before it can join the cluster.|
||
|Instance '<<<__address2>>>' has the following errant GTIDs that do not exist in the cluster:|
|00025721-1111-1111-1111-111111111111:1|
||
|Discarding these extra GTID events can either be done manually or by completely|
|overwriting the state of '<<<__address2>>>' with a physical snapshot from an|
|existing cluster member. To use this method by default, set the|
|'recoveryMethod' option to 'clone'.|
||
|Having extra GTID events is not expected, and it is recommended to investigate|
|this further and ensure that the data can be removed prior to choosing the|
|clone recovery method.|
||
|Please select a recovery method [C]lone/[A]bort (default Abort): |
||Cancelled (RuntimeError)

//@ recoveryMethod:auto, interactive, errant GTIDs -> error 5.7 {VER(<8.0.17)}
|WARNING: A GTID set check of the MySQL instance at '<<<__address2>>>' determined that it|
|contains transactions that do not originate from the cluster, which must be|
|discarded before it can join the cluster.|
||
|Instance '<<<__address2>>>' has the following errant GTIDs that do not exist in the cluster:|
|00025721-1111-1111-1111-111111111111:1|
||
|Discarding these extra GTID events can either be done manually or by completely|
|overwriting the state of '<<<__address2>>>' with a physical snapshot from an|
|existing cluster member. To use this method by default, set the|
|'recoveryMethod' option to 'clone'.|
||
|Having extra GTID events is not expected, and it is recommended to investigate|
|this further and ensure that the data can be removed prior to choosing the|
|clone recovery method.|
||
|ERROR: The target instance must be either cloned or fully provisioned before it can be added to the target cluster.|
||
|Built-in clone support is available starting with MySQL 8.0.17 and is the recommended method for provisioning instances.|
||Instance provisioning required

//@# recoveryMethod:auto, interactive, cloneDisabled, errant GTIDs -> error
|ERROR: The target instance must be either cloned or fully provisioned before it can be added to the target cluster.|
||Instance provisioning required

//@# recoveryMethod:auto, non-interactive, empty GTID -> error {VER(>=8.0.17)}
||'recoveryMethod' option must be set to 'clone' or 'incremental' (MYSQLSH 51167)

//@# recoveryMethod:auto, non-interactive, cloneDisabled, empty GTID -> error
||'recoveryMethod' option must be set to 'incremental' (MYSQLSH 51168)

//@# recoveryMethod:auto, non-interactive, empty GTIDs + gtidSetIsComplete -> incr
|Incremental state recovery was selected because it seems to be safely usable.|
||debug (LogicError)

//@# recoveryMethod:auto, non-interactive, subset GTIDs -> incr
|The safest and most convenient way to provision a new instance is through|
|automatic clone provisioning, which will completely overwrite the state of|
|'<<<__address2>>>' with a physical snapshot from an existing cluster member. To|
|use this method by default, set the 'recoveryMethod' option to 'clone'.|
||
|The incremental state recovery may be safely used if you are sure|
|all updates ever executed in the cluster were done with GTIDs enabled, there|
|are no purged transactions and the new instance contains the same GTID set as|
|the cluster or a subset of it. To use this method by default, set the|
|'recoveryMethod' option to 'incremental'.|
||
|Incremental state recovery was selected because it seems to be safely usable.|
||
||debug (LogicError)

//@# recoveryMethod:auto, non-interactive, errant GTIDs -> error {VER(>=8.0.17)}
|WARNING: A GTID set check of the MySQL instance at '<<<__address2>>>' determined that it|
|contains transactions that do not originate from the cluster, which must be|
|discarded before it can join the cluster.|
||
|Instance '<<<__address2>>>' has the following errant GTIDs that do not exist in the cluster:|
|00025721-1111-1111-1111-111111111111:1|
||
|Discarding these extra GTID events can either be done manually or by completely|
|overwriting the state of '<<<__address2>>>' with a physical snapshot from an|
|existing cluster member. To use this method by default, set the|
|'recoveryMethod' option to 'clone'.|
||
|Having extra GTID events is not expected, and it is recommended to investigate|
|this further and ensure that the data can be removed prior to choosing the|
|clone recovery method.|
||
//@# recoveryMethod:auto, non-interactive, errant GTIDs -> error
||Instance provisioning required

//@# recoveryMethod:auto, non-interactive, cloneDisabled, errant GTIDs -> error
|ERROR: The target instance must be either cloned or fully provisioned before it can be added to the target cluster.|
||Instance provisioning required

//@# recoveryMethod:incremental, interactive, make sure no prompts
||debug (LogicError)

//@# recoveryMethod:incremental, empty GTID -> incr
|NOTE: The target instance '<<<__address2>>>' has not been pre-provisioned (GTID set is|
|empty).|
|Incremental state recovery selected through the recoveryMethod option|
||debug (LogicError)

//@# recoveryMethod:incremental, cloneDisabled, empty GTID -> incr
|Incremental state recovery selected through the recoveryMethod option|
||debug (LogicError)

//@# recoveryMethod:incremental, empty GTIDs + gtidSetIsComplete -> incr
|Incremental state recovery selected through the recoveryMethod option|
||debug (LogicError)

//@# recoveryMethod:incremental, subset GTIDs -> incr
|Incremental state recovery selected through the recoveryMethod option|
||debug (LogicError)

//@# recoveryMethod:incremental, errant GTIDs -> error
|WARNING: A GTID set check of the MySQL instance at '<<<__address2>>>' determined that it|
|contains transactions that do not originate from the cluster, which must be|
|discarded before it can join the cluster.|
||Cannot use recoveryMethod=incremental option because the GTID state is not compatible or cannot be recovered. (MYSQLSH 51166)

//@# recoveryMethod:incremental, cloneDisabled, errant GTIDs -> error
||Cannot use recoveryMethod=incremental option because the GTID state is not compatible or cannot be recovered. (MYSQLSH 51166)

//@# recoveryMethod:clone, interactive, make sure no prompts {VER(>=8.0.17)}
||debug (LogicError)

//@# recoveryMethod:clone, empty GTID -> clone {VER(>=8.0.17)}
|Clone based recovery selected through the recoveryMethod option|
||debug (LogicError)

//@# recoveryMethod:clone, cloneDisabled, empty GTID -> err {VER(>=8.0.17)}
||Cannot use recoveryMethod=clone option because the disableClone option was set for the cluster. (MYSQLSH 51165)

//@# recoveryMethod:clone, empty GTIDs + gtidSetIsComplete -> clone {VER(>=8.0.17)}
|Clone based recovery selected through the recoveryMethod option|
||debug (LogicError)

//@# recoveryMethod:clone, subset GTIDs -> clone {VER(>=8.0.17)}
|Clone based recovery selected through the recoveryMethod option|
||debug (LogicError)

//@# recoveryMethod:clone, errant GTIDs -> clone {VER(>=8.0.17)}
|Clone based recovery selected through the recoveryMethod option|
||debug (LogicError)

//@# recoveryMethod:clone, cloneDisabled, errant GTIDs -> error {VER(>=8.0.17)}
||Cannot use recoveryMethod=clone option because the disableClone option was set for the cluster. (MYSQLSH 51165)

//@# purge GTIDs from cluster
||

//@# recoveryMethod:auto, interactive, purged GTID, new -> prompt c/a
|NOTE: A GTID set check of the MySQL instance at '<<<__address2>>>' determined that it|
|is missing transactions that were purged from all cluster members.|
|NOTE: The target instance '<<<__address2>>>' has not been pre-provisioned (GTID set is|
|empty). The Shell is unable to determine whether the instance has pre-existing data that would be overwritten with clone based recovery.|
//@# recoveryMethod:auto, interactive, purged GTID, new -> prompt c/a {VER(>=8.0.17)}
||Cancelled (RuntimeError)
//@# recoveryMethod:auto, interactive, purged GTID, new -> prompt c/a {VER(<8.0.17)}
||Instance provisioning required

//@ recoveryMethod:auto, no-interactive, purged GTID, new -> error {VER(>=8.0.17)}
|NOTE: A GTID set check of the MySQL instance at '<<<__address2>>>' determined that it|
|is missing transactions that were purged from all cluster members.|
|NOTE: The target instance '<<<__address2>>>' has not been pre-provisioned (GTID set is|
|empty). The Shell is unable to determine whether the instance has pre-existing data that would be overwritten with clone based recovery.|
//@ recoveryMethod:auto, no-interactive, purged GTID, new -> error {VER(>=8.0.17)}
||Instance provisioning required

// BUG#30884590: ADDING AN INSTANCE WITH COMPATIBLE GTID SET SHOULDN'T PROMPT FOR CLONE
//@# recoveryMethod:auto, interactive, purged GTID, subset gtid -> clone, no prompt
|NOTE: A GTID set check of the MySQL instance at '<<<__address2>>>' determined that it|
|is missing transactions that were purged from all cluster members.|
|Clone based recovery was selected because it seems to be safely usable.|
//@# recoveryMethod:auto, interactive, purged GTID, subset gtid -> clone, no prompt {VER(>=8.0.17)}
||debug (LogicError)
//@# recoveryMethod:auto, interactive, purged GTID, subset gtid -> clone, no prompt {VER(<8.0.17)}
||Instance provisioning required

// BUG#30884590: ADDING AN INSTANCE WITH COMPATIBLE GTID SET SHOULDN'T PROMPT FOR CLONE
//@# recoveryMethod:auto, no-interactive, purged GTID, subset gtid -> clone, no prompt
|NOTE: A GTID set check of the MySQL instance at '<<<__address2>>>' determined that it|
|is missing transactions that were purged from all cluster members.|
|Clone based recovery was selected because it seems to be safely usable.|
//@# recoveryMethod:auto, no-interactive, purged GTID, subset gtid -> clone, no prompt {VER(>=8.0.17)}
||debug (LogicError)
//@# recoveryMethod:auto, no-interactive, purged GTID, subset gtid -> clone, no prompt {VER(<8.0.17)}
||Instance provisioning required

//@# recoveryMethod:auto, interactive, cloneDisabled, purged GTID -> error
|NOTE: A GTID set check of the MySQL instance at '<<<__address2>>>' determined that it|
|is missing transactions that were purged from all cluster members.|
|ERROR: The target instance must be either cloned or fully provisioned before it can be added to the target cluster.|
||Instance provisioning required

//@# recoveryMethod:auto, interactive, errant GTIDs + purged GTIDs -> prompt c/a
|WARNING: A GTID set check of the MySQL instance at '<<<__address2>>>' determined that it|
|contains transactions that do not originate from the cluster, which must be|
|discarded before it can join the cluster.|
//@# recoveryMethod:auto, interactive, errant GTIDs + purged GTIDs -> prompt c/a {VER(>=8.0.17)}
||Cancelled (RuntimeError)
//@# recoveryMethod:auto, interactive, errant GTIDs + purged GTIDs -> prompt c/a {VER(<8.0.17)}
||Instance provisioning required


//@# recoveryMethod:auto, interactive, cloneDisabled, errant GTIDs + purged GTIDs -> error
|ERROR: The target instance must be either cloned or fully provisioned before it can be added to the target cluster.|
||Instance provisioning required

//@# recoveryMethod:incremental, purged GTID -> error
|NOTE: A GTID set check of the MySQL instance at '<<<__address2>>>' determined that it|
|is missing transactions that were purged from all cluster members.|
||Cannot use recoveryMethod=incremental option because the GTID state is not compatible or cannot be recovered. (MYSQLSH 51166)

//@# recoveryMethod:incremental, cloneDisabled, purged GTID -> error
||Cannot use recoveryMethod=incremental option because the GTID state is not compatible or cannot be recovered. (MYSQLSH 51166)

//@# recoveryMethod:incremental, errant GTIDs + purged GTIDs -> error
||Cannot use recoveryMethod=incremental option because the GTID state is not compatible or cannot be recovered. (MYSQLSH 51166)

//@# recoveryMethod:incremental, cloneDisabled, errant GTIDs + purged GTIDs -> error
||Cannot use recoveryMethod=incremental option because the GTID state is not compatible or cannot be recovered. (MYSQLSH 51166)

//@# recoveryMethod:clone, purged GTID -> clone {VER(>=8.0.17)}
|Clone based recovery selected through the recoveryMethod option|
||debug (LogicError)

//@# recoveryMethod:clone, cloneDisabled, purged GTID -> err {VER(>=8.0.17)}
||Cannot use recoveryMethod=clone option because the disableClone option was set for the cluster. (MYSQLSH 51165)

//@# recoveryMethod:clone, errant GTIDs + purged GTIDs -> clone {VER(>=8.0.17)}
|Clone based recovery selected through the recoveryMethod option|
||debug (LogicError)

//@# recoveryMethod:clone, cloneDisabled, errant GTIDs + purged GTIDs -> error {VER(>=8.0.17)}
||Cannot use recoveryMethod=clone option because the disableClone option was set for the cluster. (MYSQLSH 51165)

//@# Cleanup
||
