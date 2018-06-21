// Verify that when the shell.options["credentialStore.savePasswords"] option is set to 'prompt', a prompt asks the user if he want to save the credentials

//@ Initialization
shell.options["credentialStore.helper"] = "plaintext";

//@ Set the shell.options["credentialStore.savePasswords"] to 'prompt'
shell.options["credentialStore.savePasswords"] = "prompt";

//@ Create a Session to a server using valid credentials without the password
// Type the password when prompted
testutil.expectPassword("Please provide the password for '" + __cred.x.uri + "':", __cred.pwd);
// Verify that a prompt asking if you want to save the credentials is displayed, answer 'yes'
testutil.expectPrompt("Save password for '" + __cred.x.uri + "'? [Y]es/[N]o/Ne[v]er (default No):", "yes");
// The Session must be created successfully
shell.connect(__cred.x.uri);

//@ [SR12]Verify that the credentials are stored using the function shell.listCredentials()
EXPECT_ARRAY_CONTAINS(__cred.x.uri, shell.listCredentials());

//@ Create another Session to a server using different credentials without the password
// Type the password when prompted
testutil.expectPassword("Please provide the password for '" + __cred.second.uri + "':", __cred.second.pwd);
// Verify that a prompt asking if you want to save the credentials is displayed, answer 'no'
testutil.expectPrompt("Save password for '" + __cred.second.uri + "'? [Y]es/[N]o/Ne[v]er (default No):", "no");
// The Session must be created successfully
shell.connect(__cred.second.uri);

//@ [SR12]Verify that the credentials are not stored using the function shell.listCredentials()
EXPECT_ARRAY_NOT_CONTAINS(__cred.second.uri, shell.listCredentials());

//@ Create another Session to a server using third credentials without the password
// Type the password when prompted
testutil.expectPassword("Please provide the password for '" + __cred.third.uri + "':", __cred.third.pwd);
// [SR7]Verify that a prompt asking if you want to save the credentials is displayed, answer 'never'
testutil.expectPrompt("Save password for '" + __cred.third.uri + "'? [Y]es/[N]o/Ne[v]er (default No):", "never");
// The Session must be created successfully
shell.connect(__cred.third.uri);

//@ [SR12]Verify that the third credentials are not stored using the function shell.listCredentials()
EXPECT_ARRAY_NOT_CONTAINS(__cred.third.uri, shell.listCredentials());

//@ Verify that the URI used to create the session is listed in the shell.options["credentialStore.excludeFilters"] option
EXPECT_ARRAY_CONTAINS(__cred.third.uri, shell.options["credentialStore.excludeFilters"]);
