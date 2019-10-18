#@ initialization
||

#@ connect to a server
|Creating a session to '<<<__mysql_uri>>>'|

#@ set the EDITOR environment variable
||

#@ edit should try to use the editor from EDITOR environment variable
|ERROR: Editor "unknown_editor" returned exit code: |

#@ set the VISUAL environment variable
||

#@ edit should still try to use the editor from EDITOR environment variable
|ERROR: Editor "unknown_editor" returned exit code: |

#@ remove the EDITOR environment variable
||

#@ edit should try to use the editor from VISUAL environment variable
|ERROR: Editor "unknown_visual_editor" returned exit code: |

#@ use the test editor
||

#@<OUT> \edit command
mysql-py []>
           > print('first test')
first test

#@<OUT> \e command
mysql-py []>
           > print('second test')
second test

#@<OUT> \edit with multiline result
mysql-py []> def multiline(req, opt = False):
           >   if opt:
           >     print('multiline -> optional')
           >
           >   print('multiline test: {0}'.format(req))
multiline test: 1
multiline -> optional
multiline test: 2
end

#@ switch to JS
|Switching to JavaScript mode...|

#@<OUT> \edit with multiple statements - JS
mysql-js []> js_one = 'first JS'
           > js_two = 'second JS'
second JS
first JS
second JS
end

#@ switch to SQL
|Switching to SQL mode... Commands end with ;|

#@<OUT> \edit with multiple statements - SQL
mysql-sql []> SELECT 'first SQL';
            > SELECT 'second SQL';
+-----------+
| first SQL |
+-----------+
| first SQL |
+-----------+
1 row in set ([[*]] sec)
+------------+
| second SQL |
+------------+
| second SQL |
+------------+
1 row in set ([[*]] sec)

#@ switch back to Python
|Switching to Python mode...|

#@<OUT> \edit with multiple statements - Python
mysql-py []> py_one = 'first Python'
           > py_two = 'second Python'
first Python
second Python
end

#@<OUT> \edit - get path to the temporary file
mysql-py []>
           > temporary_file_path = r'[[*]]'

#@ temporary file should not exist after it was used by the editor
||

#@<OUT> \edit with extra arguments
mysql-py []>
           > print('test with extra additional quoted arguments')
test with extra additional quoted arguments

#@<OUT> \edit - contents must be presented to the user
mysql-py []> def display():
           >   print('display test')

#@ disconnect from a server
||

#@ cleanup
||
