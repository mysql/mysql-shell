// Assumptions: ensure_schema_does_not_exist is available

//@# Dba: kill instance
if (__sandbox_dir)
  dba.killSandboxInstance(__mysql_sandbox_port1, {sandboxDir:__sandbox_dir});
else
  dba.killSandboxInstance(__mysql_sandbox_port1);
