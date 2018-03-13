// Verify that the function shell.deleteAllCredentials() delete all the credentials previously saved

//@ Initialization
shell.options["credentialStore.helper"] = "plaintext";

//@ [SR6]Verify that shell.options["credentialStore.savePasswords"] is enabled, if not then enable it.
shell.options["credentialStore.savePasswords"] = "always";

//@ Create a Session to a server using valid credentials including the password
// The Session must be created successfully
shell.connect(__cred.x.uri_pwd);

//@ Create another Session to a server using different and valid credentials including the password
// The Session must be created successfully
shell.connect(__cred.second.uri_pwd);

//@ Create another Session to a server using third credentials including the password
// The Session must be created successfully
shell.connect(__cred.third.uri_pwd);

//@ [SR12]Call the function shell.listCredentials()
var credentials = shell.listCredentials();

//@ All the credentials previously saved must be displayed
EXPECT_ARRAY_CONTAINS(__cred.x.uri, credentials);
EXPECT_ARRAY_CONTAINS(__cred.second.uri, credentials);
EXPECT_ARRAY_CONTAINS(__cred.third.uri, credentials);

//@ Call the function shell.deleteAllCredentials(), all the credentials previously saved must be deleted
var credentials = shell.deleteAllCredentials();

//@ [SR12]Call the function shell.listCredentials() to confirm
credentials = shell.listCredentials();

//@ No credentials must be displayed
EXPECT_TRUE(0 === credentials.length);
