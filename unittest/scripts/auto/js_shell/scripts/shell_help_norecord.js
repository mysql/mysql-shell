//@ Shell Help
shell.help()

//@ Shell Help, \? [USE:Shell Help]
\? shell

//@ Help on Options
shell.help("options")

//@ Help on Options, \? [USE:Help on Options]
\? shell.options

//@ Help on Reports
shell.help("reports")

//@ Help on Reports, \? [USE:Help on Reports]
\? reports

//@ Help on addExtensionObjectMember
shell.help("addExtensionObjectMember")

//@ Help on addExtensionObjectMember, \? [USE:Help on addExtensionObjectMember]
\? addExtensionObjectMember

//@ Help on shell.addExtensionObjectMember, \? [USE:Help on addExtensionObjectMember]
\? shell.addExtensionObjectMember

//@ Help on autoCompleteSql
// WL13397-TSFR_2_1
shell.help("autoCompleteSql")

//@ Help on autoCompleteSql, \? [USE:Help on autoCompleteSql]
\? autoCompleteSql

//@ Help on shell.autoCompleteSql, \? [USE:Help on autoCompleteSql]
\? shell.autoCompleteSql

//@ Help on Connect
shell.help("connect")

//@ Help on Connect, \? [USE:Help on Connect]
\? connect

// WL13236-TSFR4_1: Validate that the function named shell.connectToPrimary([connectionData, password]) exists under the shell global object and receives two optional parameters: connectionData and password.
// WL13236-TSFR4_2: Validate that the help system has an entry for the function shell.connectToPrimary([connectionData, password]). Test with: \h shell.connectToPrimary, \h connectToPrimary, \h shell.help('connectToPrimary'), \h shell.

//@ Help on connectToPrimary
shell.help("connectToPrimary")

//@ Help on connectToPrimary, \? [USE: Help on connectToPrimary]
\? connectToPrimary

//@ Help on shell.connectToPrimary [USE: Help on connectToPrimary]
\? shell.connectToPrimary

//@ Help on createExtensionObject
shell.help("createExtensionObject")

//@ Help on createExtensionObject, \? [USE:Help on createExtensionObject]
\? createExtensionObject

//@ Help on shell.createExtensionObject, \? [USE:Help on createExtensionObject]
\? shell.createExtensionObject

//@ Help on createResult
shell.help("createResult")

//@ Help on createResult, \? [USE:Help on createResult]
\? createResult

//@ Help on shell.createResult, \? [USE:Help on createResult]
\? shell.createResult

//@ Help on deleteAllCredentials
shell.help("deleteAllCredentials")

//@ Help on deleteAllCredentials, \? [USE:Help on deleteAllCredentials]
\? deleteAllCredentials

//@ Help on deleteAllSecrets
shell.help("deleteAllSecrets")

//@ Help on deleteAllSecrets, \? [USE: Help on deleteAllSecrets]
\? deleteAllSecrets

//@ Help on deleteCredential
shell.help("deleteCredential")

//@ Help on deleteCredential, \? [USE:Help on deleteCredential]
\? shell.deleteCredential

//@ Help on deleteSecret
shell.help("deleteSecret")

//@ Help on deleteSecret, \? [USE: Help on deleteSecret]
\? shell.deleteSecret

//@ Help on disablePager
shell.help("disablePager")

//@ Help on disablePager, \? [USE:Help on disablePager]
\? disablePager

//@ Help on enablePager
shell.help("enablePager")

//@ Help on enablePager, \? [USE:Help on enablePager]
\? enablePager

//@ Help on getSession
shell.help("getSession")

//@ Help on getSession, \? [USE:Help on getSession]
\? shell.getSession

//@ Help on Help
shell.help("help")

//@ Help on Help, \? [USE:Help on Help]
\? shell.help

//@ Help on listCredentialHelpers
shell.help("listCredentialHelpers")

//@ Help on listCredentialHelpers, \? [USE:Help on listCredentialHelpers]
\? listCredentialHelpers

//@ Help on listCredentials
shell.help("listCredentials")

//@ Help on listCredentials, \? [USE:Help on listCredentials]
\? shell.listCredentials

//@ Help on listSecrets
shell.help("listSecrets")

//@ Help on listSecrets, \? [USE: Help on listSecrets]
\? shell.listSecrets

//@ Help on listSshConnections
shell.help("listSshConnections")

//@ Help on listSshConnections, \? [USE:Help on listSshConnections]
\? listSshConnections

//@ Help on Log
shell.help("log")

//@ Help on Log, \? [USE:Help on Log]
\? log

//@ Help on parseUri
shell.help("parseUri")

//@ Help on parseUri, \? [USE:Help on parseUri]
\? parseuri

//@ Help on Prompt
shell.help("prompt")

//@ Help on Prompt, \? [USE:Help on Prompt]
\? prompt

//@ Help on shell.disconnect
shell.help("disconnect")

//@ Help on disconnect [USE: Help on shell.disconnect]
\? shell.disconnect

//@ Help on readSecret
shell.help("readSecret")

//@ Help on readSecret, \? [USE: Help on readSecret]
\? readSecret

//@ Help on reconnect
shell.help("reconnect")

//@ Help on reconnect, \? [USE:Help on reconnect]
\? reconnect

//@ Help on registerGlobal
shell.help("registerGlobal")

//@ Help on registerGlobal, \? [USE:Help on registerGlobal]
\? registerGlobal

//@ Help on shell.registerGlobal, \? [USE:Help on registerGlobal]
\? shell.registerGlobal


//@ Help on registerReport
shell.help("registerReport")

//@ Help on registerReport, \? [USE:Help on registerReport]
\? registerReport


//@ Help on registerSQLHandler
shell.help("registerSQLHandler")

//@ Help on registerSQLHandler, \? [USE:Help on registerSQLHandler]
\? registerSQLHandler

//@ Help on listSQLHandlers
shell.help("listSQLHandlers")

//@ Help on listSQLHandlers, \? [USE:Help on listSQLHandlers]
\? listSQLHandlers

//@ Help on setCurrentSchema
shell.help("setCurrentSchema")

//@ Help on setCurrentSchema, \? [USE:Help on setCurrentSchema]
\? shell.setCurrentSchema

//@ Help on setSession
shell.help("setSession")

//@ Help on setSession, \? [USE:Help on setSession]
\? setSession

//@ Help on status
shell.help("status")

//@ Help on status, \? [USE:Help on status]
\? shell.status

//@ Help on print
shell.help("print")

//@ Help on print, \? [USE:Help on print]
\? shell.print

//@ Help on storeCredential
shell.help("storeCredential")

//@ Help on storeCredential, \? [USE:Help on storeCredential]
\? storeCredential

//@ Help on storeSecret
shell.help("storeSecret")

//@ Help on storeSecret, \? [USE: Help on storeSecret]
\? storeSecret

//@ BUG28393119 UNABLE TO GET HELP ON CONNECTION DATA, before session
\? connection

//@ BUG28393119 UNABLE TO GET HELP ON CONNECTION DATA, after session
shell.connect(__mysqluripwd)
\? connection
session.close()

//@ Help on connection attributes
\? connection attributes

//@ Help on connection compression
\? connection compression

//@ Help on connection options
\? connection options

//@ Help on connection types
\? connection types

//@ Help on extension objects
\? extension objects

//@ Help on unparseUri
\? unparseUri

//@ Help on dumpRows
\? dumpRows
