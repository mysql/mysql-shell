//@ Initialization
shell.options["credentialStore.helper"] = "plaintext";

// BUG#28597766 NEWLINES PRODUCE ERROR IN PASSWORD HELPER

//@ BUG#28597766 store a password - line feed character
shell.storeCredential(__cred.x.uri, "\n");

//@ BUG#28597766 store a password - carriage return character [USE: BUG#28597766 store a password - line feed character]
shell.storeCredential(__cred.x.uri, "\r");

//@ BUG#28597766 store a password - both characters [USE: BUG#28597766 store a password - line feed character]
shell.storeCredential(__cred.x.uri, "\r\n");

//@ BUG#28597766 store a password - both characters - reversed [USE: BUG#28597766 store a password - line feed character]
shell.storeCredential(__cred.x.uri, "\n\r");

//@ BUG#28597766 store a password - line feed is first [USE: BUG#28597766 store a password - line feed character]
shell.storeCredential(__cred.x.uri, "\nabc");

//@ BUG#28597766 store a password - line feed is in the middle [USE: BUG#28597766 store a password - line feed character]
shell.storeCredential(__cred.x.uri, "ab\nc");

//@ BUG#28597766 store a password - line feed is last [USE: BUG#28597766 store a password - line feed character]
shell.storeCredential(__cred.x.uri, "abc\n");

//@ BUG#28597766 store a password - carriage return is first [USE: BUG#28597766 store a password - line feed character]
shell.storeCredential(__cred.x.uri, "\rabc");

//@ BUG#28597766 store a password - carriage return is in the middle [USE: BUG#28597766 store a password - line feed character]
shell.storeCredential(__cred.x.uri, "ab\rc");

//@ BUG#28597766 store a password - carriage return is last [USE: BUG#28597766 store a password - line feed character]
shell.storeCredential(__cred.x.uri, "abc\r");

//@ BUG#28597766 store a password - both characters in the middle [USE: BUG#28597766 store a password - line feed character]
shell.storeCredential(__cred.x.uri, "a\rb\nc");

//@ BUG#28597766 store a password - both characters in the middle - reversed [USE: BUG#28597766 store a password - line feed character]
shell.storeCredential(__cred.x.uri, "a\nb\rc");
