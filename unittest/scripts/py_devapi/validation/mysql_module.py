#@ mysql module: exports
|Exported Items: 2|
|get_classic_session: <type 'builtin_function_or_method'>|
|help: <type 'builtin_function_or_method'>|

#@ mysql module: get_classic_session through URI
|<ClassicSession:|
|Session using right URI|

#@ mysql module: get_classic_session through URI and password
|<ClassicSession:|
|Session using right URI|

#@ mysql module: get_classic_session through data
|<ClassicSession:|
|Session using right URI|

#@ mysql module: get_classic_session through data and password
|<ClassicSession:|
|Session using right URI|

#@ mysql module: get_classic_session using URI with duplicated parameters
||Invalid URI: The SSL Connection option 'ssl-mode' is already defined as 'REQUIRED'.

#@ mysql module: get_classic_session using dictionary with duplicated parameters
||The SSL Connection option 'ssl-mode' is already defined as 'DISABLED'.

#@ mysql module: get_classic_session using SSL in URI
|Session using right SSL URI|
