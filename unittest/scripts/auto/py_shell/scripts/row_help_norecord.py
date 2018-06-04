shell.connect(__uripwd)
row = session.sql('select 1').execute().fetch_one()
session.close()

#@ row
row.help()

#@ global ? for Row[USE:row]
\? Row

#@ global help for Row[USE:row]
\help Row

#@ row.get_field
row.help('get_field')

#@ global ? for get_field[USE:row.get_field]
\? Row.get_field

#@ global help for get_field[USE:row.get_field]
\help Row.get_field

#@ row.get_length
row.help('get_length')

#@ global ? for get_length[USE:row.get_length]
\? Row.get_length

#@ global help for get_length[USE:row.get_length]
\help Row.get_length

#@ row.help
row.help('help')

#@ global ? for help[USE:row.help]
\? Row.help

#@ global help for help[USE:row.help]
\help Row.help

#@ row.length
row.help('length')

#@ global ? for length[USE:row.length]
\? Row.length

#@ global help for length[USE:row.length]
\help Row.length
