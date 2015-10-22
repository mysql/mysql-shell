//@# Testing require failures
var mymodule = require('fail_wont_compile');
shell.js.module_paths[shell.js.module_paths.length] = os.get_binary_folder() + '/js';
var mymodule = require('fail_wont_compile');
var empty_module = require('fail_empty');

//@ Testing require success
var my_module = require('success');
print('Defined Members:', Object.keys(my_module).length);
print(my_module.ok_message);
print(typeof unexisting);
print(in_global_context);

//@# Shell registry add errors
shell.registry.add();
shell.registry.add(56, 'test@uri');
shell.registry.add('my sample', 5);
shell.registry.add('my sample', 'test@uri');
shell.registry.add('my_sample', {});
shell.registry.add('my_sample', {host:''});
shell.registry.add('my_sample', 'user@host', 5);


//@ Adding entry to the shell registry
var done = shell.registry.add('my_sample', 'root:pwd@samplehost:44000/sakila');
print('Added:', done, '\n')
print('Host:', shell.registry.my_sample.host, '\n')
print('Port:', shell.registry.my_sample.port, '\n')
print('User:', shell.registry.my_sample.dbUser, '\n')
print('Password:', shell.registry.my_sample.dbPassword, '\n')
print('Schema:', shell.registry.my_sample.schema, '\n')

//@ Attempt to override connection without override
done = shell.registry.add('my_sample', 'admin@localhost');

//@ Attempt to override connection with override=false
done = shell.registry.add('my_sample', 'admin@localhost', false);

//@ Attempt to override connection with override=true
done = false;
done = shell.registry.add('my_sample', 'admin@localhost', true);
print('Added:', done, '\n')
print('Host:', shell.registry.my_sample.host, '\n')
print('User:', shell.registry.my_sample.dbUser, '\n')

//@# Shell registry update errors
shell.registry.update();
shell.registry.update(56, 'test@uri');
shell.registry.update('my sample', 5);
shell.registry.update('my sample', 'test@uri');
shell.registry.update('my_sample', {});
shell.registry.update('my_sample', {host:''});


//@ Updates a connection
done = false;
done = shell.registry.update('my_sample', 'guest@localhost');
print('Updated:', done, '\n')
print('User:', shell.registry.my_sample.dbUser, '\n')

//@# Shell registry remove errors
shell.registry.remove();
shell.registry.remove(56);
shell.registry.remove('my sample');

//@ Remove connection
done = false;
done = shell.registry.remove('my_sample');
print('Removed:', done, '\n')

