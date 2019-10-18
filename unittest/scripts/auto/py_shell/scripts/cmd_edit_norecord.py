#@ initialization
import os
import os.path

shell = os.path.join(__bin_dir, "mysqlsh")
helper = os.path.join(__data_path, "py", "edit_command.py")
command = "{0} --file {1}".format(shell, helper)

#@ connect to a server
\connect <<<__mysqluripwd>>>

# WL12763-TSFR2_3 - When the system variables `EDITOR` and/or `VISUAL` are set (test with one of them set and both set),
# validate that Shell choose the default editor in the system following this priority:
# - value of `EDITOR` system variable
# - value of `VISUAL` system variable

#@ set the EDITOR environment variable
os.environ["EDITOR"] = "unknown_editor"

#@ edit should try to use the editor from EDITOR environment variable
\edit

#@ set the VISUAL environment variable
os.environ["VISUAL"] = "unknown_visual_editor"

#@ edit should still try to use the editor from EDITOR environment variable
\edit

#@ remove the EDITOR environment variable
del os.environ["EDITOR"]

#@ edit should try to use the editor from VISUAL environment variable
\edit

# WL12763-TSFR2_1 - Validate that the command `\edit` is supported by the Shell and the command `\e` it's the short version of it.

#@ use the test editor
os.environ["EDITOR"] = command

#@ \edit command
\edit first

#@ \e command
\e second

#@ \edit with multiline result
\edit multiline

multiline(1)
multiline(2, True)
print('end')

#@ switch to JS
\js

#@ \edit with multiple statements - JS
\edit js_statements

println(js_one)
println(js_two)
println('end')

#@ switch to SQL
\sql

#@ \edit with multiple statements - SQL
\edit sql_statements

#@ switch back to Python
\py

#@ \edit with multiple statements - Python
\edit py_statements

print(py_one)
print(py_two)
print('end')

# WL12763-TSFR2_5 - When calling the `\edit` command, validate that:
# - A temporary file is created in the system temporary directory and contains the command to edit.
# - The temporary file is deleted when the editor is closed.

#@ \edit - get path to the temporary file
\edit temporary

#@ temporary file should not exist after it was used by the editor
EXPECT_FALSE(os.path.exists(temporary_file_path))

# WL12763-TSFR2_8 - When calling the `\edit` command with arguments, validate that the arguments are loaded into the temporary file.

#@ \edit with extra arguments
\edit extra "quoted \"arguments\""

# WL12763-TSFR2_9 - After calling the `\edit` command and closing the editor, validate that the content written to the temporary file
# is presented as the next command to be executed in the Shell.

#@ \edit - contents must be presented to the user
\edit display

#@ disconnect from a server
session.close()

#@ cleanup
for env in ("EDITOR", "VISUAL"):
  if env in os.environ:
    del os.environ[env]
