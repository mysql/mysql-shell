if (__mysql_port !== 3306) {
  testutil.skip("This test requires server to be listening on 3306 port");
}

//@ use plaintext helper
shell.options["credentialStore.helper"] = "plaintext";

//@ prompt to save passwords
shell.options["credentialStore.savePasswords"] = "prompt";

var credential = __cred.user + "@" + hostname;
var classic_uri = "mysql://" + credential;

//@ Create a Class Session to a server using valid credentials without the password
// Type the password when prompted
testutil.expectPassword("Please provide the password for '" + credential + "':", __cred.pwd);
// Verify that a prompt asking if you want to save the credentials is displayed, answer 'yes'
testutil.expectPrompt("Save password for '" + credential + "'? [Y]es/[N]o/Ne[v]er (default No):", "yes");
shell.connect(classic_uri);

//@ Create another Classic Session using the same credentials, stored password should be used
shell.connect(classic_uri);

//@ Check if stored credential matches the expected one
shell.listCredentials();
