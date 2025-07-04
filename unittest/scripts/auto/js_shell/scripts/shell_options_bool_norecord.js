

//@<> bool option handling - general check - default value true
testutil.callMysqlsh(["--js", "-e", "print(\"showWarnings: \" + shell.options.showWarnings);"]);
EXPECT_OUTPUT_CONTAINS("showWarnings: true")

//@<> bool option handling - general check - default value true - set true
testutil.callMysqlsh(["--js","--show-warnings=true", "-e", "print(\"showWarnings: \" + shell.options.showWarnings);"]);
EXPECT_OUTPUT_CONTAINS("showWarnings: true")

//@<> bool option handling - general check - default value true - set false
testutil.callMysqlsh(["--js","--show-warnings=false", "-e", "print(\"showWarnings: \" + shell.options.showWarnings);"]);
EXPECT_OUTPUT_CONTAINS("showWarnings: false")

//@<> bool option handling - general check - default value true - no prefix
testutil.callMysqlsh(["--js","--show-warnings", "-e", "print(\"showWarnings: \" + shell.options.showWarnings);"]);
EXPECT_OUTPUT_CONTAINS("showWarnings: true")

//@<> bool option handling - general check - default value true - prefix enable
testutil.callMysqlsh(["--js","--enable-show-warnings", "-e", "print(\"showWarnings: \" + shell.options.showWarnings);"]);
EXPECT_OUTPUT_CONTAINS("showWarnings: true")

//@<> bool option handling - general check - default value true - prefix disable
testutil.callMysqlsh(["--js","--disable-show-warnings", "-e", "print(\"showWarnings: \" + shell.options.showWarnings);"]);
EXPECT_OUTPUT_CONTAINS("showWarnings: false")

//@<> bool option handling - general check - default value true - prefix skip
testutil.callMysqlsh(["--js","--skip-show-warnings", "-e", "print(\"showWarnings: \" + shell.options.showWarnings);"]);
EXPECT_OUTPUT_CONTAINS("showWarnings: false")

//@<> bool option handling - general check - default value false
testutil.callMysqlsh(["--js", "-e", "print(\"sysLog: \" + shell.options[\"history.sql.syslog\"]);"]);
EXPECT_OUTPUT_CONTAINS("sysLog: false")

//@<> bool option handling - general check - default value false - set true
testutil.callMysqlsh(["--js","--syslog=true", "-e", "print(\"sysLog: \" + shell.options[\"history.sql.syslog\"]);"]);
EXPECT_OUTPUT_CONTAINS("sysLog: true")

//@<> bool option handling - general check - default value false - set false
testutil.callMysqlsh(["--js","--syslog=false", "-e", "print(\"sysLog: \" + shell.options[\"history.sql.syslog\"]);"]);
EXPECT_OUTPUT_CONTAINS("sysLog: false")

//@<> bool option handling - general check - default value false - no prefix
testutil.callMysqlsh(["--js","--syslog", "-e", "print(\"sysLog: \" + shell.options[\"history.sql.syslog\"]);"]);
EXPECT_OUTPUT_CONTAINS("sysLog: true")

//@<> bool option handling - general check - default value false - prefix enable
testutil.callMysqlsh(["--js","--enable-syslog", "-e", "print(\"sysLog: \" + shell.options[\"history.sql.syslog\"]);"]);
EXPECT_OUTPUT_CONTAINS("sysLog: true")

//@<> bool option handling - general check - default value false - prefix disable
testutil.callMysqlsh(["--js","--disable-syslog", "-e", "print(\"sysLog: \" + shell.options[\"history.sql.syslog\"]);"]);
EXPECT_OUTPUT_CONTAINS("sysLog: false")

//@<> bool option handling - general check - default value false - prefix skip
testutil.callMysqlsh(["--js","--skip-syslog", "-e", "print(\"sysLog: \" + shell.options[\"history.sql.syslog\"]);"]);
EXPECT_OUTPUT_CONTAINS("sysLog: false")

//@<> bool option handling - proxy option with handler
testutil.callMysqlsh(["--js", "-e", "print(\"name-cache: \" + shell.options[\"autocomplete.nameCache\"]);"]);
// by default name-cache is true, but in non-interactive or non tty terminal, it's set to false
EXPECT_OUTPUT_CONTAINS("name-cache: false")

//@<> bool option handling - proxy option with handler - set true
testutil.callMysqlsh(["--js", "--name-cache=true", "-e", "print(\"name-cache: \" + shell.options[\"autocomplete.nameCache\"]);"]);
EXPECT_OUTPUT_CONTAINS("name-cache: true")

//@<> bool option handling - proxy option with handler - set false
testutil.callMysqlsh(["--js", "--name-cache=false", "-e", "print(\"name-cache: \" + shell.options[\"autocomplete.nameCache\"]);"]);
EXPECT_OUTPUT_CONTAINS("name-cache: false")

//@<> bool option handling - proxy option with handler - no prefix
testutil.callMysqlsh(["--js", "--name-cache", "-e", "print(\"name-cache: \" + shell.options[\"autocomplete.nameCache\"]);"]);
EXPECT_OUTPUT_CONTAINS("name-cache: true")

//@<> bool option handling - proxy option with handler - prefix enable
testutil.callMysqlsh(["--js", "--enable-name-cache", "-e", "print(\"name-cache: \" + shell.options[\"autocomplete.nameCache\"]);"]);
EXPECT_OUTPUT_CONTAINS("name-cache: true")

//@<> bool option handling - proxy option with handler - prefix disable
testutil.callMysqlsh(["--js", "--disable-name-cache", "-e", "print(\"name-cache: \" + shell.options[\"autocomplete.nameCache\"]);"]);
EXPECT_OUTPUT_CONTAINS("name-cache: false")

//@<> bool option handling - proxy option with handler - prefix skip
testutil.callMysqlsh(["--js", "--skip-name-cache", "-e", "print(\"name-cache: \" + shell.options[\"autocomplete.nameCache\"]);"]);
EXPECT_OUTPUT_CONTAINS("name-cache: false")

//@<> bool option handling - alternative value - default value false - set 1
testutil.callMysqlsh(["--js","--syslog=1", "-e", "print(\"sysLog: \" + shell.options[\"history.sql.syslog\"]);"]);
EXPECT_OUTPUT_CONTAINS("sysLog: true")

//@<> bool option handling - alternative value - default value false - set on
testutil.callMysqlsh(["--js","--syslog=on", "-e", "print(\"sysLog: \" + shell.options[\"history.sql.syslog\"]);"]);
EXPECT_OUTPUT_CONTAINS("sysLog: true")

//@<> bool option handling - alternative value - default value false - set ON
testutil.callMysqlsh(["--js","--syslog=ON", "-e", "print(\"sysLog: \" + shell.options[\"history.sql.syslog\"]);"]);
EXPECT_OUTPUT_CONTAINS("sysLog: true")

//@<> bool option handling - alternative value - default value true - set 0
testutil.callMysqlsh(["--js","--show-warnings=0", "-e", "print(\"showWarnings: \" + shell.options.showWarnings);"]);
EXPECT_OUTPUT_CONTAINS("showWarnings: false")

//@<> bool option handling - alternative value - default value true - set off
testutil.callMysqlsh(["--js","--show-warnings=off", "-e", "print(\"showWarnings: \" + shell.options.showWarnings);"]);
EXPECT_OUTPUT_CONTAINS("showWarnings: false")

//@<> bool option handling - alternative value - default value true - set OFF
testutil.callMysqlsh(["--js","--show-warnings=OFF", "-e", "print(\"showWarnings: \" + shell.options.showWarnings);"]);
EXPECT_OUTPUT_CONTAINS("showWarnings: false")
