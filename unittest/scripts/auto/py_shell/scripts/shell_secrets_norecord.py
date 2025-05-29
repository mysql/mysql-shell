#@<> entry point
# imports
from contextlib import ExitStack

# constants
current_helper = shell.options["credentialStore.helper"]
has_login_path = "login-path" in shell.list_credential_helpers()
used_helpers = ["plaintext", "login-path"]

test_key = "my-key"
test_secret = "my-secret"
actual_secret = ""
actual_all_secrets = []

# helpers
def set_helper(helper):
    shell.options["credentialStore.helper"] = helper

def wipe_all_secrets():
    for helper in shell.list_credential_helpers():
        if helper in used_helpers:
            print("--> removing all secrets from:", helper)
            set_helper(helper)
            shell.delete_all_credentials()
            shell.delete_all_secrets()

def read_secret(key):
    global actual_secret
    actual_secret = shell.read_secret(key)

def list_all_secrets():
    global actual_all_secrets
    actual_all_secrets = shell.list_secrets()

def TEST(helper):
    global actual_secret
    actual_secret = None
    global actual_all_secrets
    actual_all_secrets = None
    EXPECT_NO_THROWS(lambda: set_helper(helper))
    stack = ExitStack()
    stack.callback(lambda: EXPECT_NO_THROWS(lambda: wipe_all_secrets()))
    return stack

#@<> Setup
wipe_all_secrets()

#@<> WL16958-TSFR_1_2_1 - valid key
with TEST("plaintext"):
    # store
    EXPECT_NO_THROWS(lambda: shell.store_secret(test_key, test_secret))
    # read
    EXPECT_NO_THROWS(lambda: read_secret(test_key))
    # test
    EXPECT_EQ(test_secret, actual_secret)
    EXPECT_EQ("str", type(actual_secret).__name__)

#@<> WL16958-TSFR_1_2_2 - login-path limitations {has_login_path}
with TEST("login-path"):
    EXPECT_THROWS(lambda: shell.store_secret(test_key, "x" * 100), "RuntimeError: Failed to save the secret: The login-path helper cannot store secrets longer than 78 characters.")

#@<> WL16958-TSFR_1_2_8 - there are no limitations on secret's value {has_login_path}
with TEST("login-path"):
    # store
    EXPECT_NO_THROWS(lambda: shell.store_secret(test_key, "\r\n"))
    # read
    EXPECT_NO_THROWS(lambda: read_secret(test_key))
    # test
    EXPECT_EQ("\r\n", actual_secret)

#@<> WL16958-TSFR_1_2_3 - key - invalid type
with TEST("plaintext"):
    EXPECT_THROWS(lambda: shell.store_secret({}, test_secret), "TypeError: Argument #1 is expected to be a string")

#@<> WL16958-TSFR_1_2_4 - value - invalid type
with TEST("plaintext"):
    EXPECT_THROWS(lambda: shell.store_secret(test_key, {}), "TypeError: Argument #2 is expected to be a string")

#@<> WL16958-TSFR_1_2_5 - empty key
with TEST("plaintext"):
    EXPECT_THROWS(lambda: shell.store_secret("", test_secret), "ValueError: Key cannot be empty.")

#@<> WL16958-TSFR_1_2_6 - prompt for empty value
with TEST("plaintext"):
    # store
    testutil.expect_password("Please provide the secret to store: ", "")
    EXPECT_NO_THROWS(lambda: shell.store_secret(test_key))
    # read
    EXPECT_NO_THROWS(lambda: read_secret(test_key))
    # test
    EXPECT_EQ("", actual_secret)

#@<> WL16958-TSFR_1_2_7 - prompt for non-empty value
with TEST("plaintext"):
    # store
    testutil.expect_password("Please provide the secret to store: ", test_secret)
    EXPECT_NO_THROWS(lambda: shell.store_secret(test_key))
    # read
    EXPECT_NO_THROWS(lambda: read_secret(test_key))
    # test
    EXPECT_EQ(test_secret, actual_secret)

#@<> WL16958-TSFR_1_3_1 - overwrite secret
with TEST("plaintext"):
    # store then overwrite
    EXPECT_NO_THROWS(lambda: shell.store_secret(test_key, test_secret))
    EXPECT_NO_THROWS(lambda: shell.store_secret(test_key, 2 * test_secret))
    # read
    EXPECT_NO_THROWS(lambda: read_secret(test_key))
    # test
    EXPECT_EQ(2 * test_secret, actual_secret)

#@<> WL16958-TSFR_2_1_1 - invalid key
with TEST("plaintext"):
    EXPECT_THROWS(lambda: shell.read_secret({}), "TypeError: Argument #1 is expected to be a string")
    EXPECT_THROWS(lambda: shell.read_secret(""), "ValueError: Key cannot be empty.")

#@<> WL16958-TSFR_2_2_1 - unknown key
with TEST("plaintext"):
    EXPECT_THROWS(lambda: shell.read_secret(test_key), "RuntimeError: Failed to read the secret: Could not find the secret")

#@<> WL16958-TSFR_3_1_1 - invalid key
with TEST("plaintext"):
    EXPECT_THROWS(lambda: shell.delete_secret({}), "TypeError: Argument #1 is expected to be a string")
    EXPECT_THROWS(lambda: shell.delete_secret(""), "ValueError: Key cannot be empty.")

#@<> WL16958-TSFR_3_1_2 - read deleted secret
with TEST("plaintext"):
    # store then delete
    EXPECT_NO_THROWS(lambda: shell.store_secret(test_key, test_secret))
    EXPECT_NO_THROWS(lambda: shell.delete_secret(test_key))
    # read fails
    EXPECT_THROWS(lambda: shell.read_secret(test_key), "RuntimeError: Failed to read the secret: Could not find the secret")

#@<> WL16958-TSFR_3_2_1 - unknown key
with TEST("plaintext"):
    EXPECT_THROWS(lambda: shell.delete_secret(test_key), "RuntimeError: Failed to delete the secret: Could not find the secret")

#@<> WL16958-TSFR_4_1 - delete all secrets with no secrets stored
with TEST("plaintext"):
    EXPECT_NO_THROWS(lambda: shell.delete_all_secrets())

#@<> WL16958-TSFR_4_2 - store some secrets, then delete all of them
with TEST("plaintext"):
    # store
    EXPECT_NO_THROWS(lambda: shell.store_secret(test_key, test_secret))
    EXPECT_NO_THROWS(lambda: shell.store_secret(2 * test_key, 2 * test_secret))
    # delete
    EXPECT_NO_THROWS(lambda: shell.delete_all_secrets())
    # test
    EXPECT_NO_THROWS(lambda: list_all_secrets())
    EXPECT_EQ([], actual_all_secrets)
    EXPECT_THROWS(lambda: shell.read_secret(test_key), "RuntimeError: Failed to read the secret: Could not find the secret")

#@<> WL16958-TSFR_5_1 - store some secrets, then list them
with TEST("plaintext"):
    # store
    EXPECT_NO_THROWS(lambda: shell.store_secret(test_key, test_secret))
    EXPECT_NO_THROWS(lambda: shell.store_secret(2 * test_key, 2 * test_secret))
    # list
    EXPECT_NO_THROWS(lambda: list_all_secrets())
    # test
    EXPECT_EQ(2, len(actual_all_secrets))
    EXPECT_EQ({test_key, 2 * test_key}, set(actual_all_secrets))

#@<> WL16958-TSFR_5_2 - store some secrets, store a credential, then list secrets
with TEST("plaintext"):
    # store
    EXPECT_NO_THROWS(lambda: shell.store_secret(test_key, test_secret))
    EXPECT_NO_THROWS(lambda: shell.store_secret(2 * test_key, 2 * test_secret))
    EXPECT_NO_THROWS(lambda: shell.store_credential("user@host", "pass"))
    # list
    EXPECT_NO_THROWS(lambda: list_all_secrets())
    # test
    EXPECT_EQ(2, len(actual_all_secrets))
    EXPECT_EQ({test_key, 2 * test_key}, set(actual_all_secrets))

#@<> WL16958-TSFR_6_1 - store a secret and a credential using the same ID
with TEST("plaintext"):
    key = "user@host"
    # store
    EXPECT_NO_THROWS(lambda: shell.store_secret(key, test_secret))
    EXPECT_NO_THROWS(lambda: shell.store_credential(key, "pass"))
    # list + read
    EXPECT_NO_THROWS(lambda: list_all_secrets())
    EXPECT_NO_THROWS(lambda: read_secret(key))
    # test
    EXPECT_EQ([key], actual_all_secrets)
    EXPECT_EQ(test_secret, actual_secret)

#@<> WL16958-ET_1 - multiple helpers
with TEST("plaintext"):
    # store
    EXPECT_NO_THROWS(lambda: shell.store_secret(test_key, test_secret))
    # switch to a different helper
    alternative_helper = "windows-credential" if "windows" == __os_type else "login-path"
    EXPECT_NO_THROWS(lambda: set_helper(alternative_helper))
    # read fails
    EXPECT_THROWS(lambda: shell.read_secret(test_key), "RuntimeError: Failed to read the secret: Could not find the secret")
    # switch back
    EXPECT_NO_THROWS(lambda: set_helper("plaintext"))
    # read succeeds
    EXPECT_NO_THROWS(lambda: read_secret(test_key))
    # test
    EXPECT_EQ(test_secret, actual_secret)

#@<> Cleanup
wipe_all_secrets()
set_helper(current_helper)
