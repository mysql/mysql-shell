//@<> Initialization
shell.options["credentialStore.helper"] = "plaintext";
shell.deleteAllCredentials();

const credential_url = "mfa@host";
const credential_file = os.path.join(testutil.getUserConfigPath(), ".mysql-secret-store-plaintext.json");

//@<> Store prompted second and third factor passwords
testutil.expectPassword(
    "Please provide the password2 for 'mfa@host':", "password2");
testutil.expectPassword(
    "Please provide the password3 for 'mfa@host':", "password3");
shell.storeCredential(credential_url, {
  password: "password1",
  password2: null,
  password3: null
});
let stored = JSON.parse(testutil.catFile(credential_file));
EXPECT_EQ("password1", stored.find(secret => secret.SecretType === "password" && secret.SecretID === credential_url).Secret);
EXPECT_EQ("password2", stored.find(secret => secret.SecretType === "generic" && secret.SecretID === "mysqlsh:mfa-password:2:mfa@host").Secret);
EXPECT_EQ("password3", stored.find(secret => secret.SecretType === "generic" && secret.SecretID === "mysqlsh:mfa-password:3:mfa@host").Secret);

//@<> Delete all stored MFA factors
shell.deleteCredential(credential_url);
stored = JSON.parse(testutil.catFile(credential_file));
EXPECT_EQ(0, stored.filter(secret => secret.SecretID === credential_url || secret.SecretID.endsWith(":" + credential_url)).length);

//@<> Normalize MFA credential URLs
const unnormalized_credential_url = "mfa@host:0003306";
const normalized_credential_url = "mfa@host:3306";
shell.storeCredential(unnormalized_credential_url, {
  password: "password1",
  password2: "password2"
});
stored = JSON.parse(testutil.catFile(credential_file));
EXPECT_EQ("password2", stored.find(secret => secret.SecretType === "generic" && secret.SecretID === "mysqlsh:mfa-password:2:" + normalized_credential_url).Secret);
shell.deleteCredential(unnormalized_credential_url);
stored = JSON.parse(testutil.catFile(credential_file));
EXPECT_EQ(0, stored.filter(secret => secret.SecretID === normalized_credential_url || secret.SecretID.endsWith(":" + normalized_credential_url)).length);

//@<> Invalid credential URL is rejected before listing credentials
EXPECT_THROWS(() => shell.deleteCredential("mfa@host?ssl-mode=REQUIRED"),
              "ArgumentError: Invalid URL: URL should not have options: mfa@host?ssl-mode=REQUIRED");

//@<> password1 takes precedence over password
shell.deleteAllCredentials();
shell.storeCredential(credential_url, {
  password: "wrong-password",
  password1: "password1"
});
stored = JSON.parse(testutil.catFile(credential_file));
EXPECT_EQ("password1", stored.find(secret => secret.SecretType === "password" && secret.SecretID === credential_url).Secret);
EXPECT_EQ(1, stored.filter(secret => secret.SecretID === credential_url || secret.SecretID.endsWith(":" + credential_url)).length);

//@<> Preserve the SSH scheme in MFA credential keys
shell.deleteAllCredentials();
const ssh_credential_url = "ssh://mfa@host";
shell.storeCredential(credential_url, {password: "mysql-password"});
shell.storeCredential(ssh_credential_url, {password: "ssh-password"});
stored = JSON.parse(testutil.catFile(credential_file));
EXPECT_EQ("mysql-password", stored.find(secret => secret.SecretType === "password" && secret.SecretID === credential_url).Secret);
EXPECT_EQ("ssh-password", stored.find(secret => secret.SecretType === "password" && secret.SecretID === ssh_credential_url).Secret);

//@<> Roll back failed MFA credential storage {shell.listCredentialHelpers().includes("login-path")}
shell.options["credentialStore.helper"] = "login-path";
const rollback_url = "mfa-rollback@host";
shell.deleteAllCredentials();
EXPECT_THROWS(() => shell.storeCredential(rollback_url, {password: "password1", password2: "x".repeat(79)}), "RuntimeError: Failed to save the credential: The login-path helper cannot store secrets longer than 78 characters.");
EXPECT_FALSE(shell.listCredentials().includes(rollback_url));

//@<> Cleanup
shell.deleteAllCredentials();
