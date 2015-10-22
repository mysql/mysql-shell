#@# Shell registry add errors
shell.registry.add()
shell.registry.add(56, 'test@uri')
shell.registry.add('my sample', 5)
shell.registry.add('my sample', 'test@uri')
shell.registry.add('my_sample', {})
shell.registry.add('my_sample', {'host':''})
shell.registry.add('my_sample', 'user@host', 5)


#@ Adding entry to the shell registry
done = shell.registry.add('my_sample', 'root:pwd@samplehost:44000/sakila')
print 'Added: %s\n' % done, '\n' 
print 'Host: %s\n' % shell.registry.my_sample.host
print 'Port: %s\n' % shell.registry.my_sample.port
print 'User: %s\n' % shell.registry.my_sample.dbUser
print 'Password: %s\n' % shell.registry.my_sample.dbPassword
print 'Schema: %s\n' % shell.registry.my_sample.schema

#@ Attempt to override connection without override
done = shell.registry.add('my_sample', 'admin@localhost')

#@ Attempt to override connection with override=False
done = shell.registry.add('my_sample', 'admin@localhost', False)

#@ Attempt to override connection with override=True
done = False
done = shell.registry.add('my_sample', 'admin@localhost', True)
print 'Added: %s\n' % done
print 'Host: %s\n' % shell.registry.my_sample.host
print 'User: %s\n' % shell.registry.my_sample.dbUser

#@# Shell registry update errors
shell.registry.update()
shell.registry.update(56, 'test@uri')
shell.registry.update('my sample', 5)
shell.registry.update('my sample', 'test@uri')
shell.registry.update('my_sample', {})
shell.registry.update('my_sample', {'host':''})


#@ Updates a connection
done = False
done = shell.registry.update('my_sample', 'guest@localhost')
print 'Updated: %s\n' % done
print 'User: %s\n' % shell.registry.my_sample.dbUser

#@# Shell registry remove errors
shell.registry.remove()
shell.registry.remove(56)
shell.registry.remove('my sample')

#@ Remove connection
done = False
done = shell.registry.remove('my_sample')
print 'Removed: %s\n' % done

