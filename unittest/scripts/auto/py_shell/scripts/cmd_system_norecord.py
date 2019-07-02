def load_text_file(path):
    with open(path) as f:
        print(f.read())

#@ initialization
import os
import os.path

shell = os.path.join(__bin_dir, "mysqlsh")
helper = os.path.join(__data_path, "py", "system_command.py")
verification = os.path.join(__tmp_dir, "system.txt")
command = "{0} --file {1} {2}".format(shell, helper, verification)

# WL12763-TSFR1_1 - Validate that the command `\system` is supported by the Shell and the command `\!` it's the short version of it.

#@ \system command with parameter
\system <<<command>>> system

#@ \system command with parameter - verification
load_text_file(verification)

#@ \! command with parameter
\! <<<command>>> !

#@ \! command with parameter - verification
load_text_file(verification)

#@ \! command with quotes
\! <<<command>>> one "two three" four "five \"six\" seven"

#@ \! command with quotes - verification
load_text_file(verification)

#@ \! command with redirection
redirected = os.path.join(__tmp_dir, "redirected.txt")
\! <<<command>>> redirection > <<<redirected>>>

#@ \! command with redirection - verification
load_text_file(verification)

#@ \! command with redirection - redirected [USE: \! command with redirection - verification]
load_text_file(redirected)

# WL12763-TSFR1_3 - When calling the `\system` command with no arguments, validate that the help for the command is displayed to the user.

#@ \system command with no arguments prints help
\system

#@ \! command with no arguments prints help [USE: \system command with no arguments prints help]
\!

# WL12763-TSFR1_5 - When calling the `\system` command with a valid argument that returns an error, validate that the output is displayed properly by the Shell.

#@ \system command with an error
\system unknown_shell_command

#@ \! command with an error [USE: \system command with an error]
\! unknown_shell_command

#@ cleanup
for f in (verification, redirected):
  if os.path.exists(f):
    os.remove(f)
