// Verify that invalid credentials removes an old entry from the credentials saved

//@<> Initialization
shell.options["credentialStore.helper"] = "plaintext";

//@<> [SR6]Set the shell.options["credentialStore.savePasswords"] to 'prompt'.
shell.options["credentialStore.savePasswords"] = "prompt";

function test(uri) {
  // Create a Session to a server using valid credentials but not passing the password
  // Type the password when prompted
  testutil.expectPassword("Please provide the password for '" + uri + "':", __cred.pwd);
  // Answer 'yes' to the prompt that asks if you want to save the credentials
  testutil.expectPrompt("Save password for '" + uri + "'? [Y]es/[N]o/Ne[v]er (default No):", "yes");
  shell.connect(uri);

  // Update the password for the user used in the session created
  var new_pwd = __cred.pwd + __cred.pwd;
  session.runSql("ALTER USER ?@? IDENTIFIED BY ?", [__cred.user, __cred.host, new_pwd]);

  // Create a Session to a server using the previous credentials given and not passing the password
  // A prompt to type the password must be displayed, type the new password
  testutil.expectPassword("Please provide the password for '" + uri + "':", new_pwd);
  // Answer 'no' to the prompt that asks if you want to save the credentials
  testutil.expectPrompt("Save password for '" + uri + "'? [Y]es/[N]o/Ne[v]er (default No):", "no");
  // The Session must be created successfully
  WIPE_SHELL_LOG();
  shell.connect(uri);

  // A message about the invalid credentials must be printed
  EXPECT_SHELL_LOG_CONTAINS("Connection to \"" + uri + "\" could not be established using the stored password: MySQL Error 1045");
  EXPECT_SHELL_LOG_CONTAINS("Invalid password has been erased.");

  // Verify that the credentials are not stored using the function shell.listCredentials()
  EXPECT_ARRAY_NOT_CONTAINS(uri, shell.listCredentials());

  // Restore old password
  session.runSql("ALTER USER ?@? IDENTIFIED BY ?", [__cred.user, __cred.host, __cred.pwd]);

  session.close();
}

//@<> test with classic connection
test(__cred.mysql.uri);

//@<> test with X connection
test(__cred.x.uri);
