#@ shell
shell.help()

#@ global ? for Shell[USE:shell]
\? Shell

#@ global help for Shell[USE:shell]
\help Shell

#@ shell.add_extension_object_member
shell.help('add_extension_object_member')

#@ global ? for add_extension_object_member[USE:shell.add_extension_object_member]
\? Shell.add_extension_object_member

#@ global help for add_extension_object_member[USE:shell.add_extension_object_member]
\help Shell.add_extension_object_member

#@ shell.connect
shell.help('connect')

#@ global ? for connect[USE:shell.connect]
\? Shell.connect

#@ global help for connect[USE:shell.connect]
\help Shell.connect

#@ shell.create_extension_object
shell.help('create_extension_object')

#@ global ? for create_extension_object[USE:shell.create_extension_object]
\? Shell.create_extension_object

#@ global help for create_extension_object[USE:shell.create_extension_object]
\help Shell.create_extension_object

#@ shell.delete_all_credentials
shell.help('delete_all_credentials')

#@ global ? for delete_all_credentials [USE:shell.delete_all_credentials]
\? Shell.delete_all_credentials

#@ global help for delete_all_credentials [USE:shell.delete_all_credentials]
\help Shell.delete_all_credentials

#@ shell.delete_credential
shell.help('delete_credential')

#@ global ? for delete_credential [USE:shell.delete_credential]
\? Shell.delete_credential

#@ global help for delete_credential [USE:shell.delete_credential]
\help Shell.delete_credential

#@ shell.disable_pager
shell.help('disable_pager')

#@ global ? for disable_pager [USE:shell.disable_pager]
\? Shell.disable_pager

#@ global help for disable_pager [USE:shell.disable_pager]
\help Shell.disable_pager

#@ shell.enable_pager
shell.help('enable_pager')

#@ global ? for enable_pager [USE:shell.enable_pager]
\? Shell.enable_pager

#@ global help for enable_pager [USE:shell.enable_pager]
\help Shell.enable_pager

#@ shell.get_session
shell.help('get_session')

#@ global ? for get_session[USE:shell.get_session]
\? Shell.get_session

#@ global help for get_session[USE:shell.get_session]
\help Shell.get_session

#@ shell.help
shell.help('help')

#@ global ? for help[USE:shell.help]
\? Shell.help

#@ global help for help[USE:shell.help]
\help Shell.help

#@ shell.list_credential_helpers
shell.help('list_credential_helpers')

#@ global ? for list_credential_helpers [USE:shell.list_credential_helpers]
\? Shell.list_credential_helpers

#@ global help for list_credential_helpers [USE:shell.list_credential_helpers]
\help Shell.list_credential_helpers

#@ shell.list_credentials
shell.help('list_credentials')

#@ global ? for list_credentials [USE:shell.list_credentials]
\? Shell.list_credentials

#@ global help for list_credentials [USE:shell.list_credentials]
\help Shell.list_credentials

#@ shell.log
shell.help('log')

#@ global ? for log[USE:shell.log]
\? Shell.log

#@ global help for log[USE:shell.log]
\help Shell.log

#@ shell.options
shell.help('options')

#@ global ? for options[USE:shell.options]
\? Shell.options

#@ global help for options[USE:shell.options]
\help Shell.options

#@ shell.parse_uri
shell.help('parse_uri')

#@ global ? for parse_uri[USE:shell.parse_uri]
\? Shell.parse_uri

#@ global help for parse_uri[USE:shell.parse_uri]
\help Shell.parse_uri

#@ shell.prompt
shell.help('prompt')

#@ global ? for prompt[USE:shell.prompt]
\? Shell.prompt

#@ global help for prompt[USE:shell.prompt]
\help Shell.prompt

#@ shell.reconnect
shell.help('reconnect')

#@ global ? for reconnect[USE:shell.reconnect]
\? Shell.reconnect

#@ global help for reconnect[USE:shell.reconnect]
\help Shell.reconnect

#@ shell.reports
shell.help('reports')

#@ global ? for reports [USE:shell.reports]
\? Shell.reports

#@ global help for reports [USE:shell.reports]
\help Shell.reports

#@ shell.register_global
shell.help('register_global')

#@ global ? for register_global[USE:shell.register_global]
\? Shell.register_global

#@ global help for register_global[USE:shell.register_global]
\help Shell.register_global

#@ shell.register_report
shell.help('register_report')

#@ global ? for register_report [USE:shell.register_report]
\? Shell.register_report

#@ global help for register_report [USE:shell.register_report]
\help Shell.register_report

#@ shell.set_current_schema
shell.help('set_current_schema')

#@ global ? for set_current_schema[USE:shell.set_current_schema]
\? Shell.set_current_schema

#@ global help for set_current_schema[USE:shell.set_current_schema]
\help Shell.set_current_schema

#@ shell.set_session
shell.help('set_session')

#@ global ? for set_session[USE:shell.set_session]
\? Shell.set_session

#@ global help for set_session[USE:shell.set_session]
\help Shell.set_session

#@ shell.status
shell.help('status')

#@ global ? for status[USE:shell.status]
\? Shell.status

#@ global help for status[USE:shell.status]
\help Shell.status

#@ shell.store_credential
shell.help('store_credential')

#@ global ? for store_credential [USE:shell.store_credential]
\? Shell.store_credential

#@ global help for store_credential [USE:shell.store_credential]
\help Shell.store_credential

#@ BUG28393119 UNABLE TO GET HELP ON CONNECTION DATA, before session
\? connection

#@ BUG28393119 UNABLE TO GET HELP ON CONNECTION DATA, after session
shell.connect(__mysqluripwd)
\? connection
session.close()

#@ Help on plugins
\? plugins

#@ Help on extension objects
\? extension objects

#@ unparse_uri
\? unparse_uri

#@ dump_rows
\? dump_rows

