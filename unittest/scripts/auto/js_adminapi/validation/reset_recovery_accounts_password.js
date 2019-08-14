//@ Validate resetRecoveryAccountsPassword changes the passwords of the recovery_accounts when all instances online
|The recovery account passwords of all the cluster instances' were successfully reset.|

//@WL#12776 An error is thrown if instance not online and the force option is not used and we are not in interactive mode
|ERROR: The recovery password of instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' cannot be reset because it is on a '(MISSING)' state. Please bring the instance back ONLINE and try the <Cluster>.resetRecoveryAccountsPassword() operation again. You can choose to run the <Cluster>.resetRecoveryAccountsPassword() with the force option enabled to ignore resetting the password of instances that are not ONLINE.|Cluster.resetRecoveryAccountsPassword: The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' is '(MISSING)' (it must be ONLINE). (RuntimeError)

//@WL#12776 An error is thrown if instance not online and the force option is not used and we we reply no to the interactive prompt
|ERROR: The recovery password of instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' cannot be reset because it is on a '(MISSING)' state. Please bring the instance back ONLINE and try the <Cluster>.resetRecoveryAccountsPassword() operation again. You can choose to proceed with the operation and skip resetting the instances' recovery account password.|Cluster.resetRecoveryAccountsPassword: The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' is '(MISSING)' (it must be ONLINE). (RuntimeError)

//@WL#12776 An error is thrown if instance not online and the force option is false (no prompts are shown in interactive mode because force option was already set).
|ERROR: The recovery password of instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' cannot be reset because it is on a '(MISSING)' state. Please bring the instance back ONLINE and try the <Cluster>.resetRecoveryAccountsPassword() operation again. You can choose to run the <Cluster>.resetRecoveryAccountsPassword() with the force option enabled to ignore resetting the password of instances that are not ONLINE.|Cluster.resetRecoveryAccountsPassword: The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' is '(MISSING)' (it must be ONLINE). (RuntimeError)

//@<OUT>WL#12776 Warning is printed for all instances not online but no error is thrown if force option is used (no prompts are shown even in interactive mode because the force option was already set)
NOTE: Skipping reset of the recovery account password for instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' because it is '(MISSING)'. To reset the recovery password, bring the instance back ONLINE and run the <Cluster>.resetRecoveryAccountsPassword() again.

NOTE: Skipping reset of the recovery account password for instance '<<<hostname>>>:<<<__mysql_sandbox_port3>>>' because it is '(MISSING)'. To reset the recovery password, bring the instance back ONLINE and run the <Cluster>.resetRecoveryAccountsPassword() again.

WARNING: Not all recovery account passwords were successfully reset, the following instances were skipped: '<<<hostname>>>:<<<__mysql_sandbox_port2>>>', '<<<hostname>>>:<<<__mysql_sandbox_port3>>>'. Bring these instances back online and run the <Cluster>.resetRecoveryAccountsPassword() operation again if you want to reset their recovery account passwords.

//@<OUT>WL#12776 Warning is printed for the instance not online but no error is thrown if force option is used (no prompts are shown even in interactive mode because the force option was already set)
NOTE: Skipping reset of the recovery account password for instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' because it is '(MISSING)'. To reset the recovery password, bring the instance back ONLINE and run the <Cluster>.resetRecoveryAccountsPassword() again.

WARNING: Not all recovery account passwords were successfully reset, the following instance was skipped: '<<<hostname>>>:<<<__mysql_sandbox_port2>>>'. Bring this instance back online and run the <Cluster>.resetRecoveryAccountsPassword() operation again if you want to reset its recovery account password.

//@WL#12776 An error is thrown if the any of the instances' recovery user was not created by InnoDB cluster.
|ERROR: The recovery user name for instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' does not match the expected format for users created automatically by InnoDB Cluster. Aborting password reset operation.|Cluster.resetRecoveryAccountsPassword: Recovery user 'nonstandart' not created by InnoDB Cluster (RuntimeError)
