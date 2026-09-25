#@<> Full versions are available without a session
import re

# WL17369-FR2-01: Python property name and version description.
# WL17369-FR2-03: Offline availability only.
# WL17369-FR3-01: Format of the configured build.
full = shell.version_with_update
EXPECT_EQ(str, type(full))
EXPECT_TRUE(re.fullmatch(r"Ver \d+\.\d+\.\d+\.\d+(?:-[^ ]+)? for .+ - for MySQL \d+\.\d+\.\d+\.\d+(?:-[^ ]+)? \(.+\)", full) is not None)
# WL17369-NF2-01: Legacy scripting version.
# WL17369-FR3-03: Suffix consistency with the legacy description.
EXPECT_EQ(shell.version, re.sub(r"(\d+\.\d+\.\d+)\.\d+", r"\1", full))

#@<> CLI and Python expose the same full versions
# WL17369-FR1-01: Successful exit and consistency with the scripting property.
EXPECT_EQ(0, testutil.call_mysqlsh(["--version-with-update"]))
EXPECT_OUTPUT_CONTAINS(full)

#@<> The property is read-only
# WL17369-FR2-02: Assignment fails and preserves the property value.
EXPECT_THROWS(lambda: setattr(shell, "version_with_update", "changed"), "Can't set object member versionWithUpdate")
EXPECT_EQ(full, shell.version_with_update)

#@<> Version properties identify the linked library before and after connections
# WL17369-FR2-03: Session independence with Classic and X Protocol.
legacy = shell.version
for uri in [__mysqluripwd, __uripwd]:
    try:
        shell.connect(uri)
        EXPECT_EQ(full, shell.version_with_update)
        EXPECT_EQ(legacy, shell.version)
        # WL17369-NF2-01: Status retains the legacy Shell version.
        WIPE_OUTPUT()
        shell.status()
        EXPECT_OUTPUT_CONTAINS("MySQL Shell version " + __mysh_version_full + "\n")
    finally:
        shell.disconnect()
    EXPECT_EQ(full, shell.version_with_update)
    EXPECT_EQ(legacy, shell.version)

#@<> Full-version help
# WL17369-HLS-02: Property help describes the update component.
WIPE_OUTPUT()
shell.help("version_with_update")
EXPECT_OUTPUT_CONTAINS("update component")
