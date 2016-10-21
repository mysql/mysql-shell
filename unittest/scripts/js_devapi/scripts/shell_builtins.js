//@# Testing require failures
var mymodule = require('fail_wont_compile');
sys.path[sys.path.length] = __test_modules_path;
var mymodule = require('fail_wont_compile');
var empty_module = require('fail_empty');

//@ Testing require success
var my_module = require('success');
print('Defined Members:', Object.keys(my_module).length);
print(my_module.ok_message);
print(typeof unexisting);
print(in_global_context);

//@# Stored sessions add errors
shell.storedSessions.add();
shell.storedSessions.add(56, 'test@uri');
shell.storedSessions.add('my sample', 5);
shell.storedSessions.add('my sample', 'test@uri');
shell.storedSessions.add('my_sample', {});
shell.storedSessions.add('my_sample', { host: '' });
shell.storedSessions.add('my_sample', 'user@host', 5);

//@ Adding entry to the Stored sessions
var done = shell.storedSessions.add('my_sample', 'root:pwd@samplehost:44000/sakila');
print('Added:', done, '\n')
print('Host:', shell.storedSessions.my_sample.host, '\n')
print('Port:', shell.storedSessions.my_sample.port, '\n')
print('User:', shell.storedSessions.my_sample.dbUser, '\n')
print('Password:', shell.storedSessions.my_sample.dbPassword, '\n')
print('Schema:', shell.storedSessions.my_sample.schema, '\n')

//@ Attempt to override connection without override
done = shell.storedSessions.add('my_sample', 'admin@localhost');

//@ Attempt to override connection with override=false
done = shell.storedSessions.add('my_sample', 'admin@localhost', false);

//@ Attempt to override connection with override=true
done = false;
done = shell.storedSessions.add('my_sample', 'admin@localhost', true);
print('Added:', done, '\n')
print('Host:', shell.storedSessions.my_sample.host, '\n')
print('User:', shell.storedSessions.my_sample.dbUser, '\n')

//@# Stored sessions update errors
shell.storedSessions.update();
shell.storedSessions.update(56, 'test@uri');
shell.storedSessions.update('my sample', 5);
shell.storedSessions.update('my sample', 'test@uri');
shell.storedSessions.update('my_sample', {});
shell.storedSessions.update('my_sample', { host: '' });

//@ Updates a connection
done = false;
done = shell.storedSessions.update('my_sample', 'guest@localhost');
print('Updated:', done, '\n')
print('User:', shell.storedSessions.my_sample.dbUser, '\n')

//@# Stored sessions remove errors
shell.storedSessions.remove();
shell.storedSessions.remove(56);
shell.storedSessions.remove('my sample');

//@ Remove connection
done = false;
done = shell.storedSessions.remove('my_sample');
print('Removed:', done, '\n')