# imports
import json
import os
import os.path
import re

# constants

# we're using mysqlshrec in order to trace syslog messages
mysqlshrec = "mysqlshrec"
if __os_type == "windows":
    mysqlshrec = mysqlshrec + ".exe"
mysqlshrec = os.path.join(__bin_dir, mysqlshrec)

syslog_trace_file = os.path.abspath("syslog_trace.log")

uri = "mysql://" + __mysqluripwd
xuri = "mysqlx://" + __uripwd

connection_id_regex = re.compile(r"^Your MySQL connection id is ([0-9]+)", re.MULTILINE)

sql_script_filename = "script.sql"
sql_script = os.path.abspath(sql_script_filename)

# helpers

def remove_trace_file():
    try:
        os.remove(syslog_trace_file)
    except OSError:
        pass

def read_trace_file():
    if os.path.isfile(syslog_trace_file):
        with open(syslog_trace_file, encoding='utf-8') as f:
            # split by lines, remove empty ones
            return [s for s in f.read().split("\n") if s]
    else:
        return []

def persist_syslog_option(value):
    EXPECT_EQ(0, testutil.call_mysqlsh([ "--py", "-e", "shell.options.set_persist('history.sql.syslog', {0})".format(value) ], "", [], mysqlshrec))

def unpersist_syslog_option():
    EXPECT_EQ(0, testutil.call_mysqlsh([ "--py", "-e", "shell.options.unset_persist('history.sql.syslog')" ], "", [], mysqlshrec))

def mysqlsh_trace_syslog(argv, uri=uri, stdin=""):
    WIPE_STDOUT()
    remove_trace_file()
    EXPECT_EQ(0, testutil.call_mysqlsh([ uri ] + argv, stdin, [ "MYSQLSH_TRACE_SYSLOG=" + syslog_trace_file ], mysqlshrec))

def make_kvp(key, value):
    def should_quote(s):
        allowed = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_.$@"
        return any(c not in allowed for c in s)
    def quote_string(s):
        return "'" + s.replace("\\", "\\\\").replace("'", "\\'") + "'"
    def quote(s):
        return quote_string(s) if should_quote(s) else s
    return quote(key) + "=" + quote(value)

def glob_to_regexp(g):
    return re.compile(g.replace("?", ".").replace("*", ".*"))

def PREPARE_SYSLOG(syslog, uri=uri):
    remove_trace_file()
    testutil.trace_syslog(syslog_trace_file)
    shell.options["history.sql.syslog"] = syslog
    shell.connect(uri)

def CLEANUP_SYSLOG():
    shell.options["history.sql.syslog"] = False
    testutil.stop_tracing_syslog()

def EXPECT_NO_SYSLOG():
    EXPECT_EQ(0, len(read_trace_file()))

def EXPECT_SYSLOG(statements, uri=uri):
    # read the trace file
    contents = read_trace_file()
    # parse connection data from URI
    connection = shell.parse_uri(uri)
    def get_data(key):
        try:
            return connection[key]
        except Exception:
            return "--"
    # construct log message prefix
    prefix = " ".join([ make_kvp(k, v) for k, v in {
        "SYSTEM_USER": __system_user,
        "MYSQL_USER": get_data("user"),
        "CONNECTION_ID": connection_id_regex.search(testutil.fetch_captured_stdout(False)).group(1),
        "DB_SERVER": get_data("host"),
        "DB": get_data("schema")
    }.items() ])
    # verify if trace matches against what's expected
    # WL14358-TSFR_3_1
    EXPECT_EQ(len(statements), len(contents), "Number of statements in the log file does not match the number of executed and not filtered out statements")
    for i in range(len(contents)):
        log = json.loads(contents[i])
        EXPECT_EQ("information", log["level"], "Log level should be information")
        EXPECT_EQ(prefix + " " + make_kvp("QUERY", statements[i]), log["msg"], "Log message does not match")

def EXPECT_MYSQLSH_NO_SYSLOG(argv, uri=uri, stdin=""):
    mysqlsh_trace_syslog(argv, uri, stdin)
    EXPECT_NO_SYSLOG()

def EXPECT_MYSQLSH_SYSLOG(argv, uri=uri, stdin=""):
    # get the SQL statements that will be executed
    statements = ""
    # --execute
    for cmd in [ "-e", "--execute" ]:
        if cmd in argv:
            statements = argv[argv.index(cmd) + 1]
    # --file
    for cmd in [ "-f", "--file" ]:
        if cmd in argv:
            with open(argv[argv.index(cmd) + 1], encoding='utf-8') as f:
                statements = f.read()
    # stdin should only be given if there are no options which execute code
    if statements and stdin:
        raise Exception("stdin should not be specified when --execute or --file is given")
    if stdin:
        statements = stdin
    if not statements:
        raise Exception("No statements to execute")
    # split statements into array, trim white-space, remove empty strings, strip "\sql " prefix
    statements = [s[len("\sql "):] if s.startswith("\sql ") else s for s in [s for s in [s.strip() for s in statements.split("\n")] if s]]
    # handle --histignore
    if "--histignore" in argv:
        filters = [glob_to_regexp(s) for s in argv[argv.index("--histignore") + 1].split(":")]
        # remove statements which match the filters
        statements = [s for s in statements if not any(f.search(s) for f in filters)]
    # run mysqlsh
    mysqlsh_trace_syslog(argv, uri, stdin)
    # verify if trace file contains the filtered statements
    EXPECT_SYSLOG(statements, uri)

#@<> setup

# create the SQL script
with open(sql_script, "w", encoding='utf-8') as f:
    f.write("""
CREATE USER IF NOT EXISTS 'wl14358'@'{0}';
GRANT TRIGGER ON *.* TO 'wl14358'@'{0}';
DROP USER IF EXISTS 'wl14358'@'{0}';
""".format(__host))

# tests

#@<> WL14358-TSFR_1_1
EXPECT_EQ(0, testutil.call_mysqlsh(["--help"]))

EXPECT_STDOUT_CONTAINS("""
  --syslog                        Log filtered interactive commands to the
                                  system log. Filtering of commands depends on
                                  the patterns supplied via histignore option.
""")

#@<> WL14358-TSFR_2_1 - 1
\option -l

EXPECT_STDOUT_CONTAINS(" history.sql.syslog              false")

#@<> WL14358-TSFR_2_1 - 2
\option -h history.sql.syslog

EXPECT_STDOUT_CONTAINS(""" history.sql.syslog  Log filtered interactive commands to the system log.
                     Filtering of commands depends on the patterns supplied via
                     histignore option.""")

#@<> WL14358-TSFR_2_1 - 3
shell.options

EXPECT_STDOUT_CONTAINS("""    "history.sql.syslog": false,""")

#@<> WL14358-TSFR_2_1 - 4
shell.help('options')

EXPECT_STDOUT_CONTAINS("""
      - history.sql.syslog: true to log filtered interactive commands to the
        system log, filtering of commands depends on the value of
        history.sql.ignorePattern
""")

#@<> WL14358-TSFR_2_2 - 1
EXPECT_MYSQLSH_NO_SYSLOG([ "-ifull", "--sql" ], stdin="SELECT 'TSFR_2_2 - 1';")

#@<> WL14358-TSFR_2_2 - 2
EXPECT_MYSQLSH_NO_SYSLOG([ "-ifull", "--sql" ], stdin="\source {0}".format(sql_script))
EXPECT_MYSQLSH_NO_SYSLOG([ "-ifull", "--sql" ], stdin="\. {0}".format(sql_script))

#@<> WL14358-TSFR_2_2 - 3
EXPECT_MYSQLSH_NO_SYSLOG([ "-ifull", "--py" ], stdin="\sql SELECT 'TSFR_2_2 - 3';")

#@<> WL14358-TSFR_2_3 - 1
EXPECT_MYSQLSH_SYSLOG([ "-ifull", "--syslog", "--sql" ], stdin="SELECT 'TSFR_2_3 - 1';")

#@<> WL14358-TSFR_2_3 - 2
EXPECT_MYSQLSH_SYSLOG([ "-ifull", "--syslog", "--sql" ], stdin="\source {0}".format(sql_script))
EXPECT_MYSQLSH_SYSLOG([ "-ifull", "--syslog", "--sql" ], stdin="\. {0}".format(sql_script))

#@<> other commands executed in SQL mode should not be logged
EXPECT_MYSQLSH_NO_SYSLOG([ "-ifull", "--syslog", "--sql" ], stdin="""
\connect
\c
\disconnect
\help
\?
\h
\history
\js
\sql
\\nopager
\\nowarnings
\w
\option
\pager
\P
\py
\sql
\\reconnect
\\rehash
\show
\sql
\status
\s
\system
\!
\\use
\\u
\warnings
\W
\watch
\q
""")

#@<> WL14358-TSFR_2_3 - 3
EXPECT_MYSQLSH_SYSLOG([ "-ifull", "--syslog", "--py" ], stdin="\sql SELECT 'TSFR_2_3 - 3';")

#@<> WL14358-TSFR_2_4 - 1
persist_syslog_option(True)
EXPECT_MYSQLSH_SYSLOG([ "-i", "--sql" ], stdin="SELECT 'TSFR_2_4 - 1';")
unpersist_syslog_option()

#@<> WL14358-TSFR_2_4 - 2
persist_syslog_option(True)
EXPECT_MYSQLSH_SYSLOG([ "-i", "--sql" ], stdin="\source {0}".format(sql_script))
EXPECT_MYSQLSH_SYSLOG([ "-i", "--sql" ], stdin="\. {0}".format(sql_script))
unpersist_syslog_option()

#@<> WL14358-TSFR_2_4 - 3
persist_syslog_option(True)
EXPECT_MYSQLSH_SYSLOG([ "-i", "--py" ], stdin="\sql SELECT 'TSFR_2_4 - 3';")
unpersist_syslog_option()

#@<> WL14358-TSFR_2_5 - 1
persist_syslog_option(False)
EXPECT_MYSQLSH_NO_SYSLOG([ "-ifull", "--sql" ], stdin="SELECT 'TSFR_2_5 - 1';")
unpersist_syslog_option()

#@<> WL14358-TSFR_2_5 - 2
persist_syslog_option(False)
EXPECT_MYSQLSH_NO_SYSLOG([ "-ifull", "--sql" ], stdin="\source {0}".format(sql_script))
EXPECT_MYSQLSH_NO_SYSLOG([ "-ifull", "--sql" ], stdin="\. {0}".format(sql_script))
unpersist_syslog_option()

#@<> WL14358-TSFR_2_5 - 3
persist_syslog_option(False)
EXPECT_MYSQLSH_NO_SYSLOG([ "-ifull", "--py" ], stdin="\sql SELECT 'TSFR_2_5 - 3';")
unpersist_syslog_option()

#@<> WL14358-TSFR_2_6 - 1
persist_syslog_option(False)
EXPECT_MYSQLSH_SYSLOG([ "-ifull", "--syslog", "--sql" ], stdin="SELECT 'TSFR_2_6 - 1';")
unpersist_syslog_option()

#@<> WL14358-TSFR_2_6 - 2
persist_syslog_option(False)
EXPECT_MYSQLSH_SYSLOG([ "-ifull", "--syslog", "--sql" ], stdin="\source {0}".format(sql_script))
EXPECT_MYSQLSH_SYSLOG([ "-ifull", "--syslog", "--sql" ], stdin="\. {0}".format(sql_script))
unpersist_syslog_option()

#@<> WL14358-TSFR_2_6 - 3
persist_syslog_option(False)
EXPECT_MYSQLSH_SYSLOG([ "-ifull", "--syslog", "--py" ], stdin="\sql SELECT 'TSFR_2_6 - 3';")
unpersist_syslog_option()

#@<> WL14358-TSFR_2_7
EXPECT_MYSQLSH_NO_SYSLOG([ "-i", "--sql", "-e", "SELECT 'TSFR_2_7';" ])

#@<> WL14358-TSFR_2_8
EXPECT_MYSQLSH_SYSLOG([ "-i", "--syslog", "--sql", "-e", "SELECT 'TSFR_2_8';" ])

#@<> WL14358-TSFR_2_9
EXPECT_MYSQLSH_NO_SYSLOG([ "-i", "--sql", "-f", sql_script ])

#@<> WL14358-TSFR_2_9_1
EXPECT_MYSQLSH_SYSLOG([ "-i", "--syslog", "--sql", "-f", sql_script ])

#@<> WL14358-TSFR_2_10
persist_syslog_option(True)
EXPECT_MYSQLSH_SYSLOG([ "-i", "--sql", "-e", "SELECT 'TSFR_2_10';" ])
unpersist_syslog_option()

#@<> WL14358-TSFR_2_11
persist_syslog_option(True)
EXPECT_MYSQLSH_SYSLOG([ "-i", "--sql", "-f", sql_script ])
unpersist_syslog_option()

#@<> WL14358-TSFR_2_12
persist_syslog_option(False)
EXPECT_MYSQLSH_NO_SYSLOG([ "-i", "--sql", "-e", "SELECT 'TSFR_2_12';" ])
unpersist_syslog_option()

#@<> WL14358-TSFR_2_13
persist_syslog_option(False)
EXPECT_MYSQLSH_NO_SYSLOG([ "-i", "--sql", "-f", sql_script ])
unpersist_syslog_option()

#@<> WL14358-TSFR_2_14
persist_syslog_option(False)
EXPECT_MYSQLSH_SYSLOG([ "-i", "--syslog", "--sql", "-e", "SELECT 'TSFR_2_12';" ])
unpersist_syslog_option()

#@<> WL14358-TSFR_2_15
persist_syslog_option(False)
EXPECT_MYSQLSH_SYSLOG([ "-i", "--syslog", "--sql", "-f", sql_script ])
unpersist_syslog_option()

#@<> WL14358-TSFR_2_16 - 1
EXPECT_MYSQLSH_NO_SYSLOG([ "-ifull", "--syslog", "--sql", "--histignore", "*IGNORE*" ], stdin="SELECT 'IGNORE';")

EXPECT_MYSQLSH_SYSLOG([ "-ifull", "--syslog", "--sql", "--histignore", "*IGNORE*" ], stdin="SELECT 'TSFR_2_16 - 1';")

#@<> WL14358-TSFR_2_16 - 2
EXPECT_MYSQLSH_NO_SYSLOG([ "-ifull", "--syslog", "--sql", "--histignore", "*" + sql_script_filename + "*" ], stdin="\source {0}".format(sql_script))
EXPECT_MYSQLSH_NO_SYSLOG([ "-ifull", "--syslog", "--sql", "--histignore", "*" + sql_script_filename + "*" ], stdin="\. {0}".format(sql_script))

EXPECT_MYSQLSH_SYSLOG([ "-ifull", "--syslog", "--sql", "--histignore", "*IGNORE*" ], stdin="\source {0}".format(sql_script))
EXPECT_MYSQLSH_SYSLOG([ "-ifull", "--syslog", "--sql", "--histignore", "*IGNORE*" ], stdin="\. {0}".format(sql_script))

#@<> WL14358-TSFR_2_16 - 3
EXPECT_MYSQLSH_NO_SYSLOG([ "-ifull", "--syslog", "--py", "--histignore", "*IGNORE*" ], stdin="\sql SELECT 'IGNORE';")

EXPECT_MYSQLSH_SYSLOG([ "-ifull", "--syslog", "--py", "--histignore", "*IGNORE*" ], stdin="\sql SELECT 'TSFR_2_16 - 3';")

#@<> WL14358-TSFR_2_17
EXPECT_MYSQLSH_NO_SYSLOG([ "-i", "--syslog", "--sql", "--histignore", "*IGNORE*", "-e", "SELECT 'IGNORE';" ])

#@<> WL14358-TSFR_2_18
EXPECT_MYSQLSH_SYSLOG([ "-i", "--syslog", "--sql", "--histignore", "*GRANT*", "-f", sql_script ])

#@<> WL14358-ET_1 - 1 - classic connection, syslog is off
PREPARE_SYSLOG(False)

\sql SELECT 'ET_1 - 1'

CLEANUP_SYSLOG()

EXPECT_NO_SYSLOG()

#@<> WL14358-ET_1 - 2 - classic connection, syslog is on
PREPARE_SYSLOG(True)

\sql SELECT 'ET_1 - 2'

CLEANUP_SYSLOG()

EXPECT_SYSLOG([ "SELECT 'ET_1 - 2'" ])

#@<> WL14358-ET_1 - 3 - X connection, syslog is off
PREPARE_SYSLOG(False, xuri)

\sql SELECT 'ET_1 - 3'

CLEANUP_SYSLOG()

EXPECT_NO_SYSLOG()

#@<> WL14358-ET_1 - 4 - X connection, syslog is on
PREPARE_SYSLOG(True, xuri)

\sql SELECT 'ET_1 - 4'

CLEANUP_SYSLOG()

EXPECT_SYSLOG([ "SELECT 'ET_1 - 4'" ], xuri)

#@<> cleanup
remove_trace_file()
os.remove(sql_script)
