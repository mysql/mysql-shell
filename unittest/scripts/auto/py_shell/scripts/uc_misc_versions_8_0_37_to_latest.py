#@<> {DEF(MYSQLD8037_PATH)}

#@<> Setup
testutil.deploy_sandbox(__mysql_sandbox_port1, "root", {}, { "mysqldPath": MYSQLD8037_PATH })

#@<> Run the invalidPrivileges Upgrade Check In Interactive Mode
testutil.call_mysqlsh([__sandbox_uri1, "--", "util", "check-for-server-upgrade", "--include=invalidPrivileges"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor"])

EXPECT_STDOUT_CONTAINS_MULTILINE("""
1) Checks for user privileges that will be removed (invalidPrivileges)
   Verifies for users containing grants to be removed as part of the upgrade
   process.

   Notice: The following users have the SET_USER_ID privilege which will be
   removed as part of the upgrade process:
   - 'root'@'%'
   - 'root'@'localhost'

   Solution:
   - If the privileges are not being used, no action is required, otherwise,
      ensure they stop being used before the upgrade as they will be lost.


Errors:   0
Warnings: 0
Notices:  2
""")


#@<> Run the invalidPrivileges Upgrade Check In JSON Mode
testutil.call_mysqlsh([__sandbox_uri1, "--", "util", "check-for-server-upgrade", "--include=invalidPrivileges", "--output-format=JSON"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor"])

EXPECT_STDOUT_CONTAINS_MULTILINE("""
{
    "serverAddress": "localhost:[[*]]",
    "serverVersion": "[[*]]",
    "targetVersion": "[[*]]",
    "errorCount": 0,
    "warningCount": 0,
    "noticeCount": 2,
    "summary": "No fatal errors were found that would prevent an upgrade, but some potential issues were detected. Please ensure that the reported issues are not significant before upgrading.",
    "checksPerformed": [
        {
            "id": "invalidPrivileges",
            "title": "Checks for user privileges that will be removed",
            "status": "OK",
            "description": "Verifies for users containing grants to be removed as part of the upgrade process.",
            "detectedProblems": [
                {
                    "level": "Notice",
                    "dbObject": "'root'@'%'",
                    "description": "The user 'root'@'%' has the following privileges that will be removed as part of the upgrade process: SET_USER_ID",
                    "dbObjectType": "User"
                },
                {
                    "level": "Notice",
                    "dbObject": "'root'@'localhost'",
                    "description": "The user 'root'@'localhost' has the following privileges that will be removed as part of the upgrade process: SET_USER_ID",
                    "dbObjectType": "User"
                }
            ],
            "solutions": [
                "If the privileges are not being used, no action is required, otherwise, ensure they stop being used before the upgrade as they will be lost."
            ]
        }
    ],
    "manualChecks": []
}
""")



#@<> Run the authMethodUsage Upgrade Check In Interactive Mode
shell.connect(__sandbox_uri1)
session.run_sql("create user sample1@localhost identified with mysql_native_password by 'password'")
session.run_sql("create user sample2@localhost identified with mysql_native_password by 'password'")
session.run_sql("create user sample3@localhost identified with sha256_password by 'password'")
session.run_sql("create user sample4@localhost identified with sha256_password by 'password'")
session.close()
testutil.call_mysqlsh([__sandbox_uri1, "--", "util", "check-for-server-upgrade", "--include=authMethodUsage"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor"])

EXPECT_STDOUT_CONTAINS_MULTILINE("""
1) Check for deprecated or invalid user authentication methods.
(authMethodUsage)
   Some users are using authentication methods that may be deprecated or
   removed, please review the details below.

   Warning: The following users are using the 'mysql_native_password'
   authentication method which is deprecated as of MySQL 8.0.0 and will be
   removed in a future release.
   Consider switching the users to a different authentication method (i.e.
   caching_sha2_password).
   - sample1@localhost
   - sample2@localhost

   More information:
     https://dev.mysql.com/doc/refman/8.0/en/caching-sha2-pluggable-authentication.html

   Warning: The following users are using the 'sha256_password' authentication
   method which is deprecated as of MySQL 8.0.0 and will be removed in a future
   release.
   Consider switching the users to a different authentication method (i.e.
   caching_sha2_password).
   - sample3@localhost
   - sample4@localhost

   More information:
     https://dev.mysql.com/doc/refman/8.0/en/caching-sha2-pluggable-authentication.html


Errors:   0
Warnings: 4
Notices:  0
""")

#@<> Run the authMethodUsage Upgrade Check In JSON Mode
testutil.call_mysqlsh([__sandbox_uri1, "--", "util", "check-for-server-upgrade", "--include=authMethodUsage", "--output-format=JSON"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor"])

EXPECT_STDOUT_CONTAINS_MULTILINE("""
{
    "serverAddress": "localhost:[[*]]",
    "serverVersion": "[[*]]",
    "targetVersion": "[[*]]",
    "errorCount": 0,
    "warningCount": 4,
    "noticeCount": 0,
    "summary": "No fatal errors were found that would prevent an upgrade, but some potential issues were detected. Please ensure that the reported issues are not significant before upgrading.",
    "checksPerformed": [
        {
            "id": "authMethodUsage",
            "title": "Check for deprecated or invalid user authentication methods.",
            "status": "OK",
            "description": "Some users are using authentication methods that may be deprecated or removed, please review the details below.",
            "detectedProblems": [
                {
                    "level": "Warning",
                    "dbObject": "sample1@localhost",
                    "description": "The following users are using the 'mysql_native_password' authentication method which is deprecated as of MySQL 8.0.0 and will be removed in a future release.\\nConsider switching the users to a different authentication method (i.e. caching_sha2_password).",
                    "documentationLink": "https://dev.mysql.com/doc/refman/8.0/en/caching-sha2-pluggable-authentication.html",
                    "dbObjectType": "User"
                },
                {
                    "level": "Warning",
                    "dbObject": "sample2@localhost",
                    "description": "The following users are using the 'mysql_native_password' authentication method which is deprecated as of MySQL 8.0.0 and will be removed in a future release.\\nConsider switching the users to a different authentication method (i.e. caching_sha2_password).",
                    "documentationLink": "https://dev.mysql.com/doc/refman/8.0/en/caching-sha2-pluggable-authentication.html",
                    "dbObjectType": "User"
                },
                {
                    "level": "Warning",
                    "dbObject": "sample3@localhost",
                    "description": "The following users are using the 'sha256_password' authentication method which is deprecated as of MySQL 8.0.0 and will be removed in a future release.\\nConsider switching the users to a different authentication method (i.e. caching_sha2_password).",
                    "documentationLink": "https://dev.mysql.com/doc/refman/8.0/en/caching-sha2-pluggable-authentication.html",
                    "dbObjectType": "User"
                },
                {
                    "level": "Warning",
                    "dbObject": "sample4@localhost",
                    "description": "The following users are using the 'sha256_password' authentication method which is deprecated as of MySQL 8.0.0 and will be removed in a future release.\\nConsider switching the users to a different authentication method (i.e. caching_sha2_password).",
                    "documentationLink": "https://dev.mysql.com/doc/refman/8.0/en/caching-sha2-pluggable-authentication.html",
                    "dbObjectType": "User"
                }
            ],
            "solutions": []
        }
    ],
    "manualChecks": []
}
""")

#@<> Cleanup
testutil.destroy_sandbox(__mysql_sandbox_port1)