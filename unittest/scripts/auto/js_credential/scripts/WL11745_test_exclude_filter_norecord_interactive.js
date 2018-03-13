//@ Initialization
shell.options["credentialStore.helper"] = "plaintext";
shell.options["credentialStore.savePasswords"] = "prompt";

//@ Should not prompt to save the password - exclude everything
shell.options["credentialStore.excludeFilters"] = [ "*" ];
shell.connect(__cred.x.uri_pwd);

//@ Should not prompt to save the password - exact match
shell.options["credentialStore.excludeFilters"] = [ __cred.x.uri ];
shell.connect(__cred.x.uri_pwd);

//@ Should not prompt to save the password - wildcards - username
shell.options["credentialStore.excludeFilters"] = [ "*@" + __cred.host + ":" + __cred.x.port ];
shell.connect(__cred.x.uri_pwd);

//@ Should not prompt to save the password - wildcards - host
shell.options["credentialStore.excludeFilters"] = [ __cred.user + "@*:" + __cred.x.port ];
shell.connect(__cred.x.uri_pwd);

//@ Should not prompt to save the password - wildcards - port
shell.options["credentialStore.excludeFilters"] = [ __cred.user + "@" + __cred.host + ":*" ];
shell.connect(__cred.x.uri_pwd);

//@ Should not prompt to save the password - wildcards - multiple characters
shell.options["credentialStore.excludeFilters"] = [ "*@*:*" ];
shell.connect(__cred.x.uri_pwd);

//@ Should not prompt to save the password - wildcards - ?
shell.options["credentialStore.excludeFilters"] = [ "?" + __cred.user.substring(1) + "@" + __cred.host + ":" + __cred.x.port ];
shell.connect(__cred.x.uri_pwd);

//@ Should not prompt to save the password - wildcards - ? and *
shell.options["credentialStore.excludeFilters"] = [ "?" + __cred.user.substring(1) + "@*:" + __cred.x.port ];
shell.connect(__cred.x.uri_pwd);

//@ Should not prompt to save the password - wildcards - host partial match
shell.options["credentialStore.excludeFilters"] = [  __cred.user + "@*" + __cred.host.substring(3, 8) + "*:" + __cred.x.port ];
shell.connect(__cred.x.uri_pwd);
