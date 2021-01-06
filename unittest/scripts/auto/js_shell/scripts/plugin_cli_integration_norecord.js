//@<> Initialization
var user_path = testutil.getUserConfigPath()
var plugins_path = os.path.join(user_path, "plugins")
var plugin_folder_path = os.path.join(plugins_path, "cli_tester")
var plugin_path =  os.path.join(plugin_folder_path, "init.js")
testutil.mkdir(plugin_folder_path, true)

function callMysqlsh(command_line_args) {
    testutil.callMysqlsh(command_line_args, "", ["MYSQLSH_TERM_COLOR_MODE=nocolor", "MYSQLSH_USER_CONFIG_HOME=" + user_path])
}

var plugin_code = `
function showInfo(name, age, adult, hobbies, address ,family){
    println("My name is " + name);
    println("Next year I will be " + parseInt(age + 1));
    adult ? println("I am an adult") : println("I am not an adult yet");
    println("My hobbies are:");
    for(var i=0; i <= hobbies.length -1; i++){
        println(hobbies[i]);
    }
    println("My address is " + address);
    if(family){
        println("My family is:");
        println(family["wifeName"]);
        println(family["sonName"]);
        println(family["dogName"]);
    }
}

var object = shell.createExtensionObject()

shell.addExtensionObjectMember(object, "showInfo", showInfo,
                      {
                        brief:"Retrieves personal information",
                        details: ["Retrieves information about the name, age, adult, etc"],
                        cli: true,
                        parameters:
                        [
                          {
                            name: "name",
                            type: "string",
                            brief: "Name of person"
                          },
                          {
                            name: "age",
                            type: "integer",
                            brief: "Age of person"
                          },
                          {
                            name: "adult",
                            type: "bool",
                            brief: "If person is an adult"
                          },
                          {
                            name: "hobbies",
                            type: "array",
                            brief: "List of hobbies"
                          },
                          {
                            name: "address",
                            type: "string",
                            brief: "Adress location"
                          },
                          {
                            name: "family",
                            type: "dictionary",
                            brief: "List of family members",
                            options: [
                              {
                                name: "wifeName",
                                type: "string",
                                brief: "wife name"
                              },
                              {
                                name: "sonName",
                                type: "string",
                                brief: "son name"
                              },
                              {
                                name: "dogName",
                                brief: "dog name"
                              }
                            ]
                          }
                        ]
                      });

shell.registerGlobal("customObj", object, {brief:"Dummy global object",
                    details:[
                       "Global object to test WL14297",
                       "Global object to test WL14297"]})
`

testutil.createFile(plugin_path, plugin_code)

// WL14297 - TSFR_9_1_2
//@ CLI -- --help
callMysqlsh(["--", "--help"])

//@ CLI -- -h [USE:CLI -- --help]
callMysqlsh(["--", "-h"])

// WL14297 - TSFR_2_1_3
//@ CLI customObj showInfo --help
callMysqlsh(["--", "customObj", "showInfo", "--help"])

//@ CLI customObj showInfo -h [USE:CLI customObj showInfo --help]
callMysqlsh(["--", "customObj", "showInfo", "-h"])

//@ WL14297 - TSFR_2_6_1
callMysqlsh(["--", "customObj", "showInfo", "John", "30", "true", "run", "tennis", "--address=Str.123", "--wifeName=Tess", "--sonName=Jack", "--dogName=Nick"])

//@ WL14297 - TSFR_2_6_2
callMysqlsh(["--", "customObj", "showInfo", "John", "30", "true", "run", "tennis", "Str.123", "--wifeName=Tess", "--sonName=Jack", "--dogName:int=123"])
testutil.rmfile(plugin_path)

//@<> WL14297 - TSFR_3_3_3 - Plugin Definition
var plugin_code = `
function dummyFunction(param_a, param_b, param_c){
  println("param_a=" + param_a + " with data type as " + type(param_a));
  println("param_b=" + param_b + " with data type as " + type(param_b));

  if ('optionA' in param_c) {
    println("param_c.optionA=" + param_c['optionA'] + " with data type as " + type(param_c["optionA"]));
  }

  if ('optionB' in param_c) {
    if ('innerOption' in param_c['optionB']) {
      println("param_c.optionB.innerOption=" + param_c['optionB']['innerOption'] + " with data type as " + type(param_c["optionB"]["innerOption"]));
    }
  }
}

var object = shell.createExtensionObject()

shell.addExtensionObjectMember(object, "dummyFunction", dummyFunction,
                      {
                        brief:"Retrieves brief information",
                        details: ["Retrieves details information"],
                        cli: true,
                        parameters:
                        [
                          {
                            name: "param_a",
                            type: "string",
                            brief: "param_a brief"
                          },
                          {
                            name: "param_b",
                            brief: "param_b brief"
                          },
                          {
                            name: "param_c",
                            type: "dictionary",
                            brief: "param_c brief",
                            options: [
                              {
                                name: "optionA",
                                brief: "optionA brief"
                              },
                              {
                                name: "optionB",
                                type: "dictionary",
                                brief: "optionB brief",
                                options: [
                                  {
                                    name: "innerOption",
                                    brief: "Inner Option"
                                  }
                                ]
                              },
                              {
                                name: "optionC",
                                type: "bool",
                                brief: "optionC brief"
                              },
                            ]
                          }
                        ]
                      });

shell.registerGlobal("customObj", object, {brief:"Dummy global object",
                    details:[
                       "Global object to test WL14297",
                       "Global object to test WL14297"]})
`

testutil.createFile(plugin_path, plugin_code)

//@<> WL14297 - TSFR_3_3_3
// - param_a is always string as the parameter is defined like that
// - param_b and optionA data type is defined by the parsing logic
// string, number, number
callMysqlsh(["--", "customObj", "dummyFunction", "1", "2", "--optionA=123"])
EXPECT_OUTPUT_CONTAINS("param_a=1 with data type as String")
EXPECT_OUTPUT_CONTAINS("param_b=2 with data type as Integer")
EXPECT_OUTPUT_CONTAINS("param_c.optionA=123 with data type as Integer")
WIPE_OUTPUT()

// string, string, number
callMysqlsh(["--", "customObj", "dummyFunction", "1", '\"2\"', "--optionA=123"])
EXPECT_OUTPUT_CONTAINS('param_b="2" with data type as String')
WIPE_OUTPUT()

//@<> WL14297 - TSFR_7_2_1 - User Defined Type in Named Argument
// FR7.1 - Named arguments support user defined data types using the following syntax: --<key>:<type>=<value>
// optionA defined by user as string
// FR7.2 Named arguments for nested dictionaries support user defined data types using the following syntax: --<key>=<inner_key>:<type>=<value>
// FR7.3 Allowed values for user defined data types include: str, int, uint, float, bool, list, dict, json*
callMysqlsh(["--", "customObj", "dummyFunction", "1", "2", "--optionA:str=123"])
EXPECT_OUTPUT_CONTAINS("param_c.optionA=123 with data type as String")
WIPE_OUTPUT()

// optionA defined by user as boolean
callMysqlsh(["--", "customObj", "dummyFunction", "1", "2", "--optionA:bool=123"])
EXPECT_OUTPUT_CONTAINS("param_c.optionA=true with data type as Bool")
WIPE_OUTPUT()

// optionA defined by user as uint
callMysqlsh(["--", "customObj", "dummyFunction", "1", "2", "--optionA:uint=123"])
EXPECT_OUTPUT_CONTAINS("param_c.optionA=123 with data type as Integer")
WIPE_OUTPUT()

// optionA defined by user as invalid uint
callMysqlsh(["--", "customObj", "dummyFunction", "1", "2", "--optionA:uint=-123"])
EXPECT_OUTPUT_CONTAINS("Error at '--optionA:uint=-123': Argument error at '--optionA:uint=-123': UInteger indicated, but value is Integer")
WIPE_OUTPUT()

// optionA defined by user as int
callMysqlsh(["--", "customObj", "dummyFunction", "1", "2", "--optionA:int=-123"])
EXPECT_OUTPUT_CONTAINS("param_c.optionA=-123 with data type as Integer")
WIPE_OUTPUT()

// optionA defined by user as invalid int
callMysqlsh(["--", "customObj", "dummyFunction", "1", "2", "--optionA:int=-123.45"])
EXPECT_OUTPUT_CONTAINS("Error at '--optionA:int=-123.45': Argument error at '--optionA:int=-123.45': Integer indicated, but value is Float")
WIPE_OUTPUT()

// optionA defined by user as float
callMysqlsh(["--", "customObj", "dummyFunction", "1", "2", "--optionA:float=123.45"])
EXPECT_OUTPUT_CONTAINS("param_c.optionA=123.45 with data type as Number")
WIPE_OUTPUT()

// optionA interpreted as JSON array as defined by the user
callMysqlsh(["--", "customObj", "dummyFunction", "1", "2", "--optionA:json=[\"a\",\"b\"]"])
// NOTES: To be analyzed, out of the scope of WL14297
// - object Object might be result of wrong toString conversion of Array
// - m.Array is the data type for arrays defined in C++, to review why.
EXPECT_OUTPUT_CONTAINS("param_c.optionA=[object Object] with data type as m.Array")
WIPE_OUTPUT()


// optionA interpreted as list as defined by the user
callMysqlsh(["--", "customObj", "dummyFunction", "1", "2", "--optionA:list=[\"a\",\"b\"]"])
// NOTES: To be analyzed, out of the scope of WL14297
// - object Object might be result of wrong toString conversion of Array
// - m.Array is the data type for JSON Arrays, tTBD: review why.
EXPECT_OUTPUT_CONTAINS("param_c.optionA=[object Object] with data type as m.Array")
WIPE_OUTPUT()

// optionA interpreted as JSON Object as defined by the user
callMysqlsh(["--", "customObj", "dummyFunction", "1", "2", "--optionA:json={\"a\":\"b\"}"])
// NOTES: To be analyzed, out of the scope of WL14297
// - object Object might be result of wrong toString conversion of Array
// - m.Map is the data type for JSON objects in JavaScript, TBD: review why.
EXPECT_OUTPUT_CONTAINS("param_c.optionA=[object Object] with data type as m.Map")
WIPE_OUTPUT()

// optionA interpreted as JSON Object as defined by the user
callMysqlsh(["--", "customObj", "dummyFunction", "1", "2", "--optionA:dict={\"a\":\"b\"}"])
// NOTES: To be analyzed, out of the scope of WL14297
// - object Object might be result of wrong toString conversion of Array
// - m.Map is the data type for JSON objects in JavaScript, TBD: review why.
EXPECT_OUTPUT_CONTAINS("param_c.optionA=[object Object] with data type as m.Map")
WIPE_OUTPUT()

// optionA normally interpreted as boolean
callMysqlsh(["--", "customObj", "dummyFunction", "1", "2", "--optionA=true"])
EXPECT_OUTPUT_CONTAINS("param_c.optionA=true with data type as Bool")
WIPE_OUTPUT()

// FR7.1 - optionA interpreted as str as defined by the user
callMysqlsh(["--", "customObj", "dummyFunction", "1", "2", "--optionA:str=true"])
EXPECT_OUTPUT_CONTAINS("param_c.optionA=true with data type as String")
WIPE_OUTPUT()

// optionB.innerOption interpreted as number
callMysqlsh(["--", "customObj", "dummyFunction", "1", "2", "--optionA=true", "--optionB=innerOption=123"])
EXPECT_OUTPUT_CONTAINS("param_c.optionB.innerOption=123 with data type as Integer")
WIPE_OUTPUT()

// FR7.2 - optionB.innerOption interpreted as string
callMysqlsh(["--", "customObj", "dummyFunction", "1", "2", "--optionA=true", "--optionB=innerOption:str=123"])
EXPECT_OUTPUT_CONTAINS("param_c.optionB.innerOption=123 with data type as String")
WIPE_OUTPUT()

// FR7.4 User defined data type for a named argument must match the data type of the corresponding parameter/option, an error will be raised if types don't match.
callMysqlsh(["--", "customObj", "dummyFunction", "1", "2", "--optionC:int=1"])
EXPECT_OUTPUT_CONTAINS("ERROR: Argument error at '--optionC:int=1': Bool expected, but value is Integer")
WIPE_OUTPUT()


testutil.rmfile(plugin_path)

//@<> WL14297 - TSFR_5_1_3_3 - Testing list params can be passed as anonymous args, comma separated list of values or json array
var plugin_code = `
function dummyFunction(list_a){
  for(var i=0; i<= list_a.length-1;i++){
      println(list_a[i]);
  }
}

var object = shell.createExtensionObject()

shell.addExtensionObjectMember(object, "dummyFunction", dummyFunction,
                      {
                        brief:"Retrieves brief information",
                        details: ["Retrieves details information"],
                        cli: true,
                        parameters:
                        [
                          {
                            name: "list_a",
                            type: "array",
                            brief: "list_a brief"
                          }
                        ]
                      });

shell.registerGlobal("customObj", object, {brief:"Dummy global object",
                    details:[
                       "Global object to test WL14297",
                       "Global object to test WL14297"]})
`

testutil.createFile(plugin_path, plugin_code)

// Comma Separated List if values (Strings)
callMysqlsh(["--", "customObj", "dummyFunction", "a,b,c"])
// Comma Separated List of Quoted Strings (JSON Strings)
callMysqlsh(["--", "customObj", "dummyFunction", '\"a\",\"b\",\"c\"'])
// Anonymous String Arguments
callMysqlsh(["--", "customObj", "dummyFunction", "a", "b", "c"])
// Anonymous JSON String Arguments
callMysqlsh(["--", "customObj", "dummyFunction", "\"a\"","\"b\"","\"c\""])
// JSON Array
callMysqlsh(["--", "customObj", "dummyFunction", "[\"a\",\"b\",\"c\"]"])

testutil.rmfile(plugin_path)

//@<> WL14297 - TSFR_6_1_2 - Testing list option can be passed as comma separated list of values, json array or repeated list definition
var plugin_code = `
function dummyFunction(param_a, list_a){
  println(param_a);
  println(list_a);
}

var object = shell.createExtensionObject()

shell.addExtensionObjectMember(object, "dummyFunction", dummyFunction,
                      {
                        brief:"Retrieves brief information",
                        details: ["Retrieves details information"],
                        cli: true,
                        parameters:
                        [
                          {
                            name: "param_a",
                            type: "string",
                            brief: "param_a brief"
                          },
                          {
                            name: "list_a",
                            type: "dictionary",
                            brief: "list_a brief",
                            options: [
                              {
                                name: "testOption",
                                type: "array",
                                brief: "testOption brief"
                              }
                            ]
                          }
                        ]
                      });

shell.registerGlobal("customObj", object, {brief:"Dummy global object",
                    details:[
                       "Global object to test WL14297",
                       "Global object to test WL14297"]})
`

testutil.createFile(plugin_path, plugin_code)

// Comma Separated List if values (Strings)
// Options as JSON Array
callMysqlsh(["--", "customObj", "dummyFunction", "test", "--testOption=[\"a\",\"b\",\"c\"]"])

// Options as comma separated list of strings
callMysqlsh(["--", "customObj", "dummyFunction", "test", "--testOption=a,b,c"])

// Options as comma separated list of JSON strings
callMysqlsh(["--", "customObj", "dummyFunction", "test", "--testOption=\"a\",\"b\",\"c\""])

// Options as independent args of strings
callMysqlsh(["--", "customObj", "dummyFunction", "test", "--testOption=a", "--testOption=b", "--testOption=c"])

// Options as independent args of quoted strings
callMysqlsh(["--", "customObj", "dummyFunction", "test", "--testOption=\"a\"", "--testOption=\"b\"", "--testOption=\"c\""])

testutil.rmfile(plugin_path)

//@<> WL14297 - TSFR_8_2_1 - Testing dictionary with no options takes options that don't belong to other dictionary
var plugin_code = `
function dummyFunction(param_a, param_b){
  println(param_a);
  println(param_b);
}

var object = shell.createExtensionObject()

shell.addExtensionObjectMember(object, "dummyFunction", dummyFunction,
                      {
                        brief:"Retrieves brief information",
                        details: ["Retrieves details information"],
                        cli: true,
                        parameters:
                        [
                        {
                          name: "param_a",
                          type: "dictionary",
                          brief: "list_a brief"
                        },
                        {
                          name: "param_b",
                          type: "dictionary",
                          brief: "list_b brief",
                          options: [
                              {
                                name: "testOption",
                                type: "integer",
                                brief: "testOption brief"
                              }
                            ]
                        }
                        ]
                      });

shell.registerGlobal("customObj", object, {brief:"Dummy global object",
                    details:[
                       "Global object to test WL14297",
                       "Global object to test WL14297"]})
`

testutil.createFile(plugin_path, plugin_code)

// Comma Separated List if values (Strings)
// Options as JSON Array
callMysqlsh(["--", "customObj", "dummyFunction", "--testOther=23", "--testOption=45", "--yetAnother=sample"])

testutil.rmfile(plugin_path)

//@<> WL14297 - TSFR_4_1_1 - Testing JSON Escaping is honored
var plugin_code = `
function dummyFunction(param_a, param_b){
  println(param_a);
  println(param_b);
}

var object = shell.createExtensionObject()


shell.addExtensionObjectMember(object, "dummyFunction", dummyFunction,
                      {
                        brief:"Retrieves brief information",
                        details: ["Retrieves details information"],
                        cli: true,
                        parameters:
                        [
                          {
                            name: "param_a",
                            type: "string",
                            brief: "param_a brief"
                          },
                          {
                            name: "param_b",
                            type: "dictionary",
                            brief: "param_b brief",
                            options: [
                              {
                                name: "optionA",
                                type: "string",
                                brief: "optionA brief"
                              }
                            ]
                          }
                        ]
                      });

shell.registerGlobal("customObj", object, {brief:"Dummy global object",
                    details:[
                       "Global object to test WL14297",
                       "Global object to test WL14297"]})
`

testutil.createFile(plugin_path, plugin_code)

callMysqlsh(["--", "customObj", "dummyFunction", '\"test\"', "--optionA=\"testOption\""])

testutil.rmfile(plugin_path)

//@<> Finalization
testutil.rmdir(plugins_path, true)
