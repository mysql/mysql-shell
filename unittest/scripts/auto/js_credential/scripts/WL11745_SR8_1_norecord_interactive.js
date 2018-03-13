// Verify that the function shell.listCredentialHelpers() list the available options depending on the OS that is in use

var default_helper;

switch (__os_type) {
  case "windows":
    default_helper = "windows-credential";
    break;

  case "macos":
    default_helper = "keychain";
    break;

  default:
    default_helper = "login-path";
    break;
}

//@ Call the function shell.listCredentialHelpers()
var helpers = shell.listCredentialHelpers();

//@ Verify that the list of options displayed are valid for the OS that is in use
EXPECT_ARRAY_CONTAINS("plaintext", helpers);
EXPECT_ARRAY_CONTAINS(default_helper, helpers);
