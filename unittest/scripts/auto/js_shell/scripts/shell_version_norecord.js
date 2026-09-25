//@<> Full versions are available without a session
// WL17369-FR2-01: JavaScript property name and version description.
// WL17369-FR2-03: Offline availability only.
// WL17369-FR3-01: Format of the configured build.
var full = shell.versionWithUpdate;
EXPECT_EQ("string", typeof full);
EXPECT_TRUE(/^Ver \d+\.\d+\.\d+\.\d+(?:-[^ ]+)? for .+ - for MySQL \d+\.\d+\.\d+\.\d+(?:-[^ ]+)? \(.+\)$/.test(full));
// WL17369-NF2-01: Legacy scripting version.
// WL17369-FR3-03: Suffix consistency with the legacy description.
EXPECT_EQ(shell.version, full.replace(/(\d+\.\d+\.\d+)\.\d+/g, "$1"));

//@<> Existing CLI versions retain their representation
// WL17369-NF2-01: Legacy CLI versions.
for (var flags of [["--version"], ["-V"], ["--version", "-V"]]) {
    WIPE_OUTPUT();
    EXPECT_EQ(0, testutil.callMysqlsh(flags));
    EXPECT_OUTPUT_CONTAINS(shell.version);
}

//@<> CLI selects full versions regardless of option order
// WL17369-FR1-01: Successful exit and consistency with the scripting property.
// WL17369-HLS-01: Full-version selection for mixed and repeated options.
for (var flags of [["--version-with-update"],
                   ["--version", "--version-with-update"],
                   ["--version-with-update", "--version"],
                   ["--version-with-update", "-V"],
                   ["--version-with-update", "--version-with-update"]]) {
    WIPE_OUTPUT();
    EXPECT_EQ(0, testutil.callMysqlsh(flags));
    EXPECT_OUTPUT_CONTAINS(full);
}

//@<> The full version option rejects arguments
// WL17369-FR1-03: --version-with-update rejects an attached argument.
EXPECT_NE(0, testutil.callMysqlsh(["--version-with-update=1"]));

//@<> The property is read-only
// WL17369-FR2-02: Assignment fails and preserves the property value.
EXPECT_THROWS(function () { shell.versionWithUpdate = "changed"; }, "Can't set object member versionWithUpdate");
EXPECT_EQ(full, shell.versionWithUpdate);

//@<> Version properties identify the linked library before and after connections
// WL17369-FR2-03: Session independence with Classic and X Protocol.
var legacy = shell.version;
for (var uri of [__mysqluripwd, __uripwd]) {
    try {
        shell.connect(uri);
        EXPECT_EQ(full, shell.versionWithUpdate);
        EXPECT_EQ(legacy, shell.version);

        // WL17369-NF2-01: Status retains the legacy Shell version.
        WIPE_OUTPUT();
        shell.status();
        EXPECT_OUTPUT_CONTAINS("MySQL Shell version " + __mysh_version_full + "\n");
    } finally {
        shell.disconnect();
    }
    EXPECT_EQ(full, shell.versionWithUpdate);
    EXPECT_EQ(legacy, shell.version);
}

//@<> Full-version help
// WL17369-HLS-02: Property help describes the update component.
WIPE_OUTPUT();
shell.help("versionWithUpdate");
EXPECT_OUTPUT_MATCHES(/update\s+component/);
