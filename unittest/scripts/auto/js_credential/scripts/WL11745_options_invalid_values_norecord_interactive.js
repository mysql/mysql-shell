//@ Invalid helper name
shell.options["credentialStore.helper"] = "unknown";

//@ Invalid savePasswords value (allowed values are 'always', 'never' and 'prompt')
shell.options["credentialStore.savePasswords"] = "unknown";

//@ Invalid exclude filter (must be array of strings)
shell.options["credentialStore.excludeFilters"] = "user@host:port";
