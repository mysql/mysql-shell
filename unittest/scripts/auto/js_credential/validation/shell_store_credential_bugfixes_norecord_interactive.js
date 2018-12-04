//@ Initialization
|plaintext|

//@ BUG#28597766 store a password - line feed character
||Shell.storeCredential: Failed to save the credential: Unable to store password with a newline character (RuntimeError)
