// Verify that invalid credentials removes an old entry from the credentials saved

//@ Initialization
shell.options["credentialStore.helper"] = "plaintext";

//@ [SR6]Set the shell.options["credentialStore.savePasswords"] to 'prompt'.
shell.options["credentialStore.savePasswords"] = "prompt";

//@ Create a Session to a server using valid credentials but not passing the password
// Type the password when prompted
testutil.expectPassword("Please provide the password for '" + __cred.x.uri + "':", __cred.pwd);
// Answer 'yes' to the prompt that asks if you want to save the credentials
testutil.expectPrompt("Save password for '" + __cred.x.uri + "'? [Y]es/[N]o/Ne[v]er (default No):", "yes");
shell.connect(__cred.x.uri);

//@ Update the password for the user used in the session created
var new_pwd = __cred.pwd + __cred.pwd;
session.sql("ALTER USER ?@? IDENTIFIED BY ?").bind([__cred.user, __cred.host, new_pwd]).execute();

//@ Create a Session to a server using the previous credentials given and not passing the password
// A prompt to type the password must be displayed, type the new password
testutil.expectPassword("Please provide the password for '" + __cred.x.uri + "':", new_pwd);
// Answer 'no' to the prompt that asks if you want to save the credentials
testutil.expectPrompt("Save password for '" + __cred.x.uri + "'? [Y]es/[N]o/Ne[v]er (default No):", "no");
// The Session must be created successfully
WIPE_SHELL_LOG();
shell.connect(__cred.x.uri);
//@ A message about the invalid credentials must be printed {VER(>=8.0.12)}
EXPECT_SHELL_LOG_CONTAINS("Connection to \"" + __cred.x.uri + "\" could not be established using the stored password: MySQL Error 1045: Access denied for user '" + __cred.user + "'@'" + __cred.host + "' (using password: YES)");
EXPECT_SHELL_LOG_CONTAINS("Invalid password has been erased.");

//@ A message about the invalid credentials must be printed - old message {VER(<8.0.12)}
EXPECT_SHELL_LOG_CONTAINS("Connection to \"" + __cred.x.uri + "\" could not be established using the stored password: MySQL Error 1045: Invalid user or password.");
EXPECT_SHELL_LOG_CONTAINS("Invalid password has been erased.");

//@ [SR12]Verify that the credentials are not stored using the function shell.listCredentials()
EXPECT_ARRAY_NOT_CONTAINS(__cred.x.uri, shell.listCredentials());
