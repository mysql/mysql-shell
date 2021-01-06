var schema = "wl12134";
shell.connect(__uripwd);
session.dropSchema(schema);
session.createSchema(schema);
var fname = __import_data_path + "/bson_types.json";

var import_docs = function(newParams) {
  var params = [__uripwd + "/" + schema, "--", "util", "importJson", fname, "{", "}"];
  var delta = params.splice.apply(params, [6,0].concat(newParams));
  testutil.callMysqlsh(params, "", ["MYSQLSH_TERM_COLOR_MODE=nocolor"]);
}

var import_doc = function(document, customParams) {
  if (!customParams)
    customParams = ["--convertBsonTypes", "--ignoreRegexOptions=false"];

  testutil.createFile("single_doc.json", JSON.stringify(document));
  var params = [__uripwd + "/" + schema, "--", "util", "importJson", "single_doc.json", "{", "}"];

  var delta = params.splice.apply(params, [6,0].concat(customParams));

  testutil.callMysqlsh(params, "", ["MYSQLSH_TERM_COLOR_MODE=nocolor"]);
  testutil.rmfile("single_doc.json");
}

var print_docs = function() {
  var params = [__uripwd + "/" + schema, "--sql", "-e", "select doc from bson_types"];
  testutil.callMysqlsh(params);
}

var delete_docs = function() {
  var params = [__uripwd + "/" + schema, "--sql", "-e", "delete from bson_types"];
  testutil.callMysqlsh(params);
}

var print_doc = function() {
  var params = [__uripwd + "/" + schema, "--sql", "-e", "select doc from single_doc"];
  testutil.callMysqlsh(params);
}

var delete_doc = function() {
  var params = [__uripwd + "/" + schema, "--sql", "-e", "delete from single_doc"];
  testutil.callMysqlsh(params);
}

// ============== BSON Option Validation Tests ================
//@<> Invalid option if convertBsonOid is disabled
import_docs(["--extractOidTime=idTime"]);
EXPECT_STDOUT_CONTAINS("ERROR: Argument options: The 'extractOidTime' option can not be used if 'convertBsonOid' is disabled.");

//@<> Invalid options when convertBsonTypes is not enabled
var invalids = ["ignoreDate", "ignoreTimestamp", "ignoreBinary", "ignoreRegex", "ignoreRegexOptions", "decimalAsDouble"];

for(index in invalids) {
  import_docs(["--" + invalids[index]]);
  EXPECT_STDOUT_CONTAINS("The following option can not be used if 'convertBsonTypes' is disabled: '" + invalids[index] + "'");
}

//@<> extractOidTime is Invalid options when convertBsonOid is OFF
import_docs(["--convertBsonTypes", "--convertBsonOid=false", "--extractOidTime=idTime"]);
EXPECT_STDOUT_CONTAINS("ERROR: Argument options: The 'extractOidTime' option can not be used if 'convertBsonOid' is disabled.");

//@<> ignoreRegexOptions is Invalid options when ignoreRegex is ON
import_docs(["--convertBsonTypes", "--ignoreRegex", "--ignoreRegexOptions"]);
EXPECT_STDOUT_CONTAINS("ERROR: Argument options: The 'ignoreRegex' and 'ignoreRegexOptions' options can't both be enabled");

// ============== BSON Option Tests ================
//@ Enabling ObjectID conversion
import_docs(["--convertBsonOid"]);
EXPECT_STDOUT_CONTAINS("Total successfully imported documents 3");
print_docs()
delete_docs()

//@ Enabling extractOidTime
import_docs(["--convertBsonOid", "--extractOidTime=idTime"]);
EXPECT_STDOUT_CONTAINS("Total successfully imported documents 3");
print_docs()
delete_docs()

//@<> Enabling extractOidTime with empty value
import_docs(["--convertBsonOid", "--extractOidTime="]);
EXPECT_STDOUT_CONTAINS("ERROR: Argument options: Option 'extractOidTime' can not be empty.");

//@<> Enabling extractOidTime with no value
import_docs(["--convertBsonOid", "--extractOidTime"]);
EXPECT_STDOUT_CONTAINS("ERROR: Argument error at '--extractOidTime': String expected, but value is Bool");

//@<> Importing document, extractOidTime enabled but _id is not ObjectID
var document = {_id:"01234567890123456789", negativeInteger:-450};
import_doc(document, ["--convertBsonTypes", "--extractOidTime=myUnexistingTime"])
print_doc();
delete_doc();
EXPECT_OUTPUT_NOT_CONTAINS("myUnexistingTime");
EXPECT_STDOUT_CONTAINS('{"_id": "01234567890123456789", "negativeInteger": -450}');

//@<> Importing document, extractOidTime enabled but there's no _id {VER(>=8.0.11)}
var document = {negativeInteger:-450};
import_doc(document, ["--convertBsonTypes", "--extractOidTime=myUnexistingTime"])
print_doc();
delete_doc();
EXPECT_OUTPUT_NOT_CONTAINS("myUnexistingTime");
EXPECT_STDOUT_CONTAINS('{"_id":');
EXPECT_STDOUT_CONTAINS('"negativeInteger": -450}');


//@ Enabling full BSON type conversion
import_docs(["--convertBsonTypes"]);
EXPECT_STDOUT_CONTAINS("WARNING: The regular expression for regExpression contains options being ignored: i.")
EXPECT_STDOUT_CONTAINS("Total successfully imported documents 3");
print_docs()
delete_docs()

//@ Enabling full BSON type conversion except for Date
import_docs(["--convertBsonTypes", "--ignoreDate"]);
EXPECT_STDOUT_CONTAINS("Total successfully imported documents 3");
print_docs()
delete_docs()

//@ Enabling full BSON type conversion except for timestamp
import_docs(["--convertBsonTypes", "--ignoreTimestamp"]);
EXPECT_STDOUT_CONTAINS("Total successfully imported documents 3");
print_docs()
delete_docs()

//@ Enabling full BSON type conversion except for binary
import_docs(["--convertBsonTypes", "--ignoreBinary"]);
EXPECT_STDOUT_CONTAINS("Total successfully imported documents 3");
print_docs()
delete_docs()

//@ Enabling full BSON type conversion except for regex
import_docs(["--convertBsonTypes", "--ignoreRegex"]);
EXPECT_STDOUT_CONTAINS("Total successfully imported documents 3");
print_docs()
delete_docs()

//@ Enabling decimalAsDouble
import_docs(["--convertBsonTypes", "--decimalAsDouble"]);
EXPECT_STDOUT_CONTAINS("Total successfully imported documents 3");
print_docs()
delete_docs()

//@ Disabling ignoreRegexOptions
import_docs(["--convertBsonTypes", "--ignoreRegexOptions=false"]);
EXPECT_OUTPUT_NOT_CONTAINS("WARNING: The regular expression for regExpression contains options being ignored: i.");
EXPECT_STDOUT_CONTAINS("Total successfully imported documents 3");
print_docs()
delete_docs()

// ============== Additional Validations ================
//@<> $oid with invalid value: numeric
import_doc({_id:{$oid:01234567012345678901234},name:"Some Name"});
EXPECT_STDOUT_CONTAINS("ERROR: Unexpected data, expected to find a string with 24 hexadecimal digits processing extended JSON for $oid at offset 16");

//@<> $oid with invalid value: wrong length
import_doc({_id:{$oid:"5bfe3d5ff6d2f36d067c6f"},name:"Some Name"});
EXPECT_STDOUT_CONTAINS("ERROR: Unexpected data, expected to find a string with 24 hexadecimal digits processing extended JSON for $oid at offset 16");

//@<> $oid with invalid value: empty value
import_doc({_id:{$oid:""},name:"Some Name"});
EXPECT_STDOUT_CONTAINS("ERROR: Unexpected data, expected to find a string with 24 hexadecimal digits processing extended JSON for $oid at offset 16");


//@<> $oid with invalid value: non hexadecimal digits
import_doc({_id:{$oid:"5bfe3d5ff6d2f36d067c6fha"},name:"Some Name"});
EXPECT_STDOUT_CONTAINS("ERROR: Unexpected data, expected to find a string with 24 hexadecimal digits processing extended JSON for $oid at offset 16");

//@<> $oid with extra field
import_doc({_id:{$oid:"5bfe3d5ff6d2f36d067c6fha", extra:1},name:"Some Name"});
EXPECT_STDOUT_CONTAINS("ERROR: Unexpected data, expected to find a string with 24 hexadecimal digits processing extended JSON for $oid at offset 16");


//@<> incomplete $timestamp
import_doc({_id:{$oid:"5bfe3d5ff6d2f36d067c6fea"},timestamp:{$timestamp:{t:1543388511}}});
EXPECT_STDOUT_CONTAINS("ERROR: Unexpected data, expected to find ',' processing extended JSON for $timestamp at offset 85");

//@<> invalid field in $timestamp definition: invalid field instead of t
import_doc({_id:{$oid:"5bfe3d5ff6d2f36d067c6fea"},timestamp:{$timestamp:{some:1543388511, i:1}}});
EXPECT_STDOUT_CONTAINS("ERROR: Unexpected data, expected to find \"t\" processing extended JSON for $timestamp at offset 71");

//@<> invalid field in $timestamp definition: invalid field instead of i
import_doc({_id:{$oid:"5bfe3d5ff6d2f36d067c6fea"},timestamp:{$timestamp:{t:1543388511, other:1}}});
EXPECT_STDOUT_CONTAINS("ERROR: Unexpected data, expected to find \"i\" processing extended JSON for $timestamp at offset 86");

//@<> $timestamp with non numeric at t
import_doc({_id:{$oid:"5bfe3d5ff6d2f36d067c6fea"},timestamp:{$timestamp:{t:"1543388511", i:1}}});
EXPECT_STDOUT_CONTAINS("ERROR: Unexpected data, expected to find a number processing extended JSON for $timestamp at offset 75");

//@<> $timestamp with non numeric at i
import_doc({_id:{$oid:"5bfe3d5ff6d2f36d067c6fea"},timestamp:{$timestamp:{t:1543388511, i:"1"}}});
EXPECT_STDOUT_CONTAINS("ERROR: Unexpected data, expected to find a number processing extended JSON for $timestamp at offset 90");

//@<> extra invalid field in $timestamp definition
import_doc({_id:{$oid:"5bfe3d5ff6d2f36d067c6fea"},timestamp:{$timestamp:{t:1543388511, i:1, extra:1}}});
EXPECT_STDOUT_CONTAINS("ERROR: Unexpected data, expected to find '}' processing extended JSON for $timestamp at offset 91");

//@<> $numberLong with non string value
import_doc({_id:{$oid:"5bfe3d5ff6d2f36d067c6fea"},long:{$numberLong:4522}});
EXPECT_STDOUT_CONTAINS("ERROR: Unexpected data, expected to find an integer string processing extended JSON for $numberLong at offset 66");

//@<> $numberLong with invalid integer
import_doc({_id:{$oid:"5bfe3d5ff6d2f36d067c6fea"},long:{$numberLong:"4522.35"}});
EXPECT_STDOUT_CONTAINS("ERROR: Unexpected data, expected to find an integer string processing extended JSON for $numberLong at offset 66");


//@<> $numberLong with extra field
import_doc({_id:{$oid:"5bfe3d5ff6d2f36d067c6fea"},long:{$numberLong:"4522", extra:1}});
EXPECT_STDOUT_CONTAINS("ERROR: Unexpected data, expected to find '}' processing extended JSON for $numberLong at offset 72");

//@<> $numberInt with non string value
import_doc({_id:{$oid:"5bfe3d5ff6d2f36d067c6fea"},int:{$numberInt:4522}});
EXPECT_STDOUT_CONTAINS("ERROR: Unexpected data, expected to find an integer string processing extended JSON for $numberInt at offset 64");

//@<> $numberInt with invalid integer
import_doc({_id:{$oid:"5bfe3d5ff6d2f36d067c6fea"},int:{$numberInt:"4522.35"}});
EXPECT_STDOUT_CONTAINS("ERROR: Unexpected data, expected to find an integer string processing extended JSON for $numberInt at offset 64");

//@<> $numberInt with extra field
import_doc({_id:{$oid:"5bfe3d5ff6d2f36d067c6fea"},int:{$numberInt:"4522", extra:1}});
EXPECT_STDOUT_CONTAINS("ERROR: Unexpected data, expected to find '}' processing extended JSON for $numberInt at offset 70");

//@<> $numberInt with empty value
import_doc({_id:{$oid:"5bfe3d5ff6d2f36d067c6fea"},int:{$numberInt:""}});
EXPECT_STDOUT_CONTAINS("ERROR: Unexpected data, expected to find an integer string processing extended JSON for $numberInt at offset 64");


//@<> $numberDecimal with non string value
import_doc({_id:{$oid:"5bfe3d5ff6d2f36d067c6fea"},decimal:{$numberDecimal:15.4}});
EXPECT_STDOUT_CONTAINS("ERROR: Unexpected data, expected to find a numeric string processing extended JSON for $numberDecimal at offset 72");

//@<> $numberDecimal with non decimal value
import_doc({_id:{$oid:"5bfe3d5ff6d2f36d067c6fea"},decimal:{$numberDecimal:"15.4a"}});
EXPECT_STDOUT_CONTAINS("ERROR: Unexpected data, expected to find a numeric string processing extended JSON for $numberDecimal at offset 72");

//@<> $numberDecimal with extra field
import_doc({_id:{$oid:"5bfe3d5ff6d2f36d067c6fea"},decimal:{$numberDecimal:"15.4", extra:1}});
EXPECT_STDOUT_CONTAINS("ERROR: Unexpected data, expected to find '}' processing extended JSON for $numberDecimal at offset 78");

//@<> $numberDecimal with empty value
import_doc({_id:{$oid:"5bfe3d5ff6d2f36d067c6fea"},decimal:{$numberDecimal:""}});
EXPECT_STDOUT_CONTAINS("ERROR: Unexpected data, expected to find a numeric string processing extended JSON for $numberDecimal at offset 72");

//@<> $binary with empty $type
import_doc({_id:{$oid:"5bfe3d5ff6d2f36d067c6fea"},"binaryData":{$binary:"SGVsbG8gV29ybGQh",$type:""}})
EXPECT_STDOUT_CONTAINS("ERROR: Unexpected data, expected to find a string with 2 hexadecimal digits processing extended JSON for $binary at offset 95");

//@<> $binary with non string $type
import_doc({_id:{$oid:"5bfe3d5ff6d2f36d067c6fea"},"binaryData":{$binary:"SGVsbG8gV29ybGQh",$type:45}})
EXPECT_STDOUT_CONTAINS("ERROR: Unexpected data, expected to find a string with 2 hexadecimal digits processing extended JSON for $binary at offset 95");

//@<> $binary with single hexadecimal digit at $type
import_doc({_id:{$oid:"5bfe3d5ff6d2f36d067c6fea"},"binaryData":{$binary:"SGVsbG8gV29ybGQh",$type:"3"}})
EXPECT_STDOUT_CONTAINS("ERROR: Unexpected data, expected to find a string with 2 hexadecimal digits processing extended JSON for $binary at offset 95");

//@<> $binary with two digits at $type but not hexadecimal
import_doc({_id:{$oid:"5bfe3d5ff6d2f36d067c6fea"},"binaryData":{$binary:"SGVsbG8gV29ybGQh",$type:"AG"}})
EXPECT_STDOUT_CONTAINS("ERROR: Unexpected data, expected to find a string with 2 hexadecimal digits processing extended JSON for $binary at offset 95");

//@<> $binary with no $type
import_doc({_id:{$oid:"5bfe3d5ff6d2f36d067c6fea"},"binaryData":{$binary:"SGVsbG8gV29ybGQh"}})
EXPECT_STDOUT_CONTAINS("ERROR: Unexpected data, expected to find ',' processing extended JSON for $binary at offset 86");

//@<> $binary with non string $binary
import_doc({_id:{$oid:"5bfe3d5ff6d2f36d067c6fea"},"binaryData":{$binary:12345,$type:"00"}})
EXPECT_STDOUT_CONTAINS("ERROR: Unexpected data, expected to find a string processing extended JSON for $binary at offset 68");

//@<> $regex with invalid $options
import_doc({_id:{$oid:"5bfe3d5ff6d2f36d067c6fea"},"regExpression":{$regex:"^some*",$options:"a"}})
EXPECT_STDOUT_CONTAINS("ERROR: Unexpected data, invalid options found processing extended JSON for $regex at offset 68");

//@<> $regex with invalid $options: not string
import_doc({_id:{$oid:"5bfe3d5ff6d2f36d067c6fea"},"regExpression":{$regex:"^some*",$options:450}})
EXPECT_STDOUT_CONTAINS("ERROR: Unexpected data, expected to find a string processing extended JSON for $regex at offset 90");

//@<> $regex with no $options
import_doc({_id:{$oid:"5bfe3d5ff6d2f36d067c6fea"},"regExpression":{$regex:"^some*"}})
EXPECT_STDOUT_CONTAINS("ERROR: Unexpected data, expected to find ',' processing extended JSON for $regex at offset 78");

//@<> $regex with extra field
import_doc({_id:{$oid:"5bfe3d5ff6d2f36d067c6fea"},"regExpression":{$regex:"^some*",$options:"i",extra:1}})
EXPECT_STDOUT_CONTAINS("ERROR: Unexpected data, expected to find '}' processing extended JSON for $regex at offset 93");

//@<> $regex with field different than $options
import_doc({_id:{$oid:"5bfe3d5ff6d2f36d067c6fea"},"regExpression":{$regex:"^some*",$weird:"i"}})
EXPECT_STDOUT_CONTAINS("ERROR: Unexpected data, expected to find \"$options\" processing extended JSON for $regex at offset 79");

//@<> $regex with invalid $regex: not string
import_doc({_id:{$oid:"5bfe3d5ff6d2f36d067c6fea"},"regExpression":{$regex:456,$options:"i"}})
EXPECT_STDOUT_CONTAINS("ERROR: Unexpected data, expected to find a string processing extended JSON for $regex at offset 70");

//@<> Importing timestamp with value out of range
import_doc({_id:{$oid:"5c014aa8087579bdb2e6afef"},aTime:{$timestamp:{t:+1.3e+20,i:1}}})
print_doc();
EXPECT_STDOUT_CONTAINS("ERROR: Invalid timestamp value found processing extended JSON for $timestamp. at offset 99");

//@<> Importing same document but epoch time
import_doc({_id:{$oid:"5c014aa8087579bdb2e6afef"},aTime:{$timestamp:{t:0,i:1}}})
print_doc();
delete_doc();
EXPECT_STDOUT_CONTAINS('{"_id": "5c014aa8087579bdb2e6afef", "aTime": "1970-01-01 00:00:00"}');

//@<> Importing same document but negative time
import_doc({_id:{$oid:"5c014aa8087579bdb2e6afef"},aTime:{$timestamp:{t:-1,i:1}}})
print_doc();
delete_doc();
EXPECT_STDOUT_CONTAINS('{"_id": "5c014aa8087579bdb2e6afef", "aTime": "1969-12-31 23:59:59"}');

//@<> Importing document, with $oid conversion disabled
var document = {_id:"01234567890123456789", objectId:{$oid:"5c014aa8087579bdb2e6afef"},negativeInteger:-450};
import_doc(document, ["--convertBsonTypes", "--convertBsonOid=false"])
print_doc();
delete_doc();
EXPECT_STDOUT_CONTAINS('{"_id": "01234567890123456789", "objectId": {"$oid": "5c014aa8087579bdb2e6afef"}, "negativeInteger": -450}');
