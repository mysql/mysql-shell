//@{VER(>=8.0.23)}
//@<> Setup
testutil.deployRawSandbox(__mysql_sandbox_port1, "root");
shell.connect(__sandbox_uri1+"/mysql");
\sql

//@ Setting query attributes without the component installed
\query_attributes at1 val1 at2 val2 at3 val3 at4 val4

//@<> Query attributes without the component installed
select mysql_query_attribute_string("at1") as at1, mysql_query_attribute_string("at2") as at2, mysql_query_attribute_string("at3") as at3, mysql_query_attribute_string("at4") as at4, mysql_query_attribute_string("at5") as at5;

//@<> Installing the query attributes component
INSTALL COMPONENT "file://component_query_attributes";

//@<> Basic query attribute definition
\query_attributes at1 val1 at2 val2 at3 val3 at4 val4
select mysql_query_attribute_string("at1") as at1, mysql_query_attribute_string("at2") as at2, mysql_query_attribute_string("at3") as at3, mysql_query_attribute_string("at4") as at4, mysql_query_attribute_string("at5") as at5;

//@<> Query attributes belong only to the next query
select mysql_query_attribute_string("at1") as at1, mysql_query_attribute_string("at2") as at2, mysql_query_attribute_string("at3") as at3, mysql_query_attribute_string("at4") as at4, mysql_query_attribute_string("at5") as at5;

//@<> Calling \query_attributes multiple times, last attribute set overrides initial
\query_attributes at1 val1 at2 val2 at3 val3 at4 val4
\query_attributes at1 v1 at2 v2 at3 v3
select mysql_query_attribute_string("at1") as at1, mysql_query_attribute_string("at2") as at2, mysql_query_attribute_string("at3") as at3, mysql_query_attribute_string("at4") as at4, mysql_query_attribute_string("at5") as at5;

//@<> Calling \query_attributes without arguments cleans previouly defined attributes
\query_attributes at1 val1 at2 val2 at3 val3 at4 val4
\query_attributes
select mysql_query_attribute_string("at1") as at1, mysql_query_attribute_string("at2") as at2, mysql_query_attribute_string("at3") as at3, mysql_query_attribute_string("at4") as at4, mysql_query_attribute_string("at5") as at5;

//@<> Trying quoted attribute/values with "
\query_attributes "some attribute" "some value"
select mysql_query_attribute_string("some attribute") as at1;

//@<> Trying quoted attribute/values with '
\query_attributes 'some attribute' 'some value'
select mysql_query_attribute_string("some attribute") as at1;

//@<> Trying quoted attribute/values with `
\query_attributes `some attribute` `some value`
select mysql_query_attribute_string("some attribute") as at1;

//@<> Multiple <attributes><space><value> with multiple spaces and tabs as separator
\query_attributes at1 val1   at2 val2			at3 val3	  	at4 val4
select mysql_query_attribute_string("at1") as at1, mysql_query_attribute_string("at2") as at2, mysql_query_attribute_string("at3") as at3, mysql_query_attribute_string("at4") as at4, mysql_query_attribute_string("at5") as at5;


//@<> Query attributes with multiple spaces and tabs as separator
\query_attributes  at1  val1		at2		val2 	 at3 	 val3 at4 val4
select mysql_query_attribute_string("at1") as at1, mysql_query_attribute_string("at2") as at2, mysql_query_attribute_string("at3") as at3, mysql_query_attribute_string("at4") as at4, mysql_query_attribute_string("at5") as at5;


//@<> Query attributes with space as key
\query_attributes  at1  val1		at2		val2 	 at3 	 val3 at4 val4
select mysql_query_attribute_string("at1") as at1, mysql_query_attribute_string("at2") as at2, mysql_query_attribute_string("at3") as at3, mysql_query_attribute_string("at4") as at4, mysql_query_attribute_string("at5") as at5;

//@<> Query attributes edge cases: space key, spaces key, empty value
\query_attributes " " val1 att2 "" "" val3
select mysql_query_attribute_string(" ") as SPACE, mysql_query_attribute_string("att2") as EMPTY_VALUE, mysql_query_attribute_string("") as NULL_EXPECTED;

//@<> Query attributes with using key<space>
\query_attributes at1  

//@<> Query attributes with using key<space>value <other>
\query_attributes at1 val1 at2

//@<> Query attributes with >32 key value pairs
\query_attributes 1 1 2 2 3 3 4 4 5 5 6 6 7 7 8 8 9 9 10 10 11 11 12 12 13 13 14 14 15 15 16 16 17 17 18 18 19 19 20 20 21 21 22 22 23 23 24 24 25 25 26 26 27 27 28 28 29 29 30 30 31 31 32 32 33 33
select mysql_query_attribute_string("32"), mysql_query_attribute_string("33");


//@<> Query attribute, long attribute name (1025 chars)
\query_attributes at1 val1 01234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234 val2 at3 val3
select mysql_query_attribute_string("at1"), mysql_query_attribute_string("01234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234"), mysql_query_attribute_string("at3");

//@<> Query attribute, long value (1025 chars)
\query_attributes at1 val1 at2 01234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234 at3 val3
select mysql_query_attribute_string("at1"), mysql_query_attribute_string("at2"), mysql_query_attribute_string("at3");


//@<> Query attributes, after reconnect 1
\js
var global_id = session.runSql("select connection_id()").fetchOne()[0];
var other_session = mysql.getSession(__sandbox_uri1);
shell.options.useWizards = true
other_session.runSql(`kill ${global_id}`);
other_session.close();
\sql
\query_attributes at1 val1 at2 val2
\reconnect
select mysql_query_attribute_string("at2"), mysql_query_attribute_string("at2");

//@<> Query attributes, after reconnect 2
\js
var global_id = session.runSql("select connection_id()").fetchOne()[0];

\sql
\query_attributes at1 val1 at2 val2

\js
var other_session = mysql.getSession(__sandbox_uri1);
shell.options.useWizards = true
other_session.runSql(`kill ${global_id}`);
other_session.close();

\sql
\reconnect
select mysql_query_attribute_string("at2"), mysql_query_attribute_string("at2");


//@<> Defining query attributes before restart
\query_attributes at1 val1 at2 val2

\js
testutil.stopSandbox(__mysql_sandbox_port1, {wait:1})
testutil.startSandbox(__mysql_sandbox_port1)

//@<> Reconnection will be triggered here
\sql select mysql_query_attribute_string("at2"), mysql_query_attribute_string("at2");

//@<> Retrieving query attributes after restart
\sql select mysql_query_attribute_string("at2"), mysql_query_attribute_string("at2");

//@<> Query attributes with no global session
\sql
\disconnect
\query_attributes at1 val1 at2 val2 at3 val3 at4 val4
\js

//@<> Query attributes with session through X protocol
shell.connect(__uripwd);
\sql
\query_attributes at1 val1 at2 val2 at3 val3 at4 val4
\disconnect


//@<> \query_attributes in Python
\py
\query_attributes at1 

//@<> \query_attributes in JavaScript
\js
\query_attributes at1 

//@<> Cleanup
testutil.stopSandbox(__mysql_sandbox_port1, {wait:1})
testutil.destroySandbox(__mysql_sandbox_port1)
