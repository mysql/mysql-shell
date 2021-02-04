//@ Initialization
||

//@ Interactive_dba_drop_metadata_schemaread_only_no_prompts
||

//@ prepare Interactive_dba_drop_metadata_schemaread_only_no_flag_prompt_yes
||

//@<OUT> Interactive_dba_drop_metadata_schemaread_only_no_flag_prompt_yes
ERROR: The MySQL instance at 'localhost:<<<__mysql_sandbox_port1>>>' currently has the super_read_only system variable set to protect it from inadvertent updates from applications.
You must first unset it to be able to perform any changes to this instance.
For more information see:
https://dev.mysql.com/doc/refman/en/server-system-variables.html#sysvar_super_read_only.


//@<OUT> Interactive_dba_drop_metadata_schemaread_only_no_flag_prompt_yes

Do you want to disable super_read_only and continue? [y/N]:

//@ prepare Interactive_dba_drop_metadata_schemaread_only_no_flag_prompt_no
||

//@# Interactive_dba_drop_metadata_schemaread_only_no_flag_prompt_no
|The MySQL instance at 'localhost:<<<__mysql_sandbox_port1>>>' currently has the super_read_only system|
|variable set to protect it from inadvertent updates from applications.|
|You must first unset it to be able to perform any changes to this instance.|
|For more information see:|
|https://dev.mysql.com/doc/refman/en/server-system-variables.html#sysvar_super_read_only.|
|Do you want to disable super_read_only and continue? [y/N]:|
||Server in SUPER_READ_ONLY mode (RuntimeError)

//@ prepare Interactive_dba_drop_metadata_schemaread_only_invalid_flag_value
||

//@# Interactive_dba_drop_metadata_schemaread_only_invalid_flag_value
||Option 'clearReadOnly' Bool expected, but value is String (TypeError)

//@ prepare Interactive_dba_drop_metadata_schemaread_only_flag_true
||

//@ Interactive_dba_drop_metadata_schemaread_only_flag_true
||

//@ prepare Interactive_dba_drop_metadata_schemaread_only_flag_false
||

//@# Interactive_dba_drop_metadata_schemaread_only_flag_false
||Server in SUPER_READ_ONLY mode

//@ Cleanup
||
