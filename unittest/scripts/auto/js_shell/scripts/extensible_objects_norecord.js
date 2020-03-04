// Object Registration
testutil.enableExtensible()

//@ Registration from C++, the parent class
\? testutil

//@ Registration from C++, a test module
\? sampleModuleJS

//@ Registration from C++, registerModule function
\? registerModule

//@ Registration from C++, registerFunction function
\? registerFunction

//@ Register, function(string)
var f1 = function(data) {
  println("JavaScript Function Definition: " + data);
}

shell.addExtensionObjectMember(testutil.sampleModuleJS, "stringFunction", f1,
                          {
                            brief:"Brief description for stringFunction.",
                            details: ["Detailed description for stringFunction"],
                            parameters:
                            [
                              {
                                name: "data",
                                type: "string",
                                brief: "Brief description for string parameter.",
                                details: ["Detailed description for string parameter."],
                                values: ["one", "two", "three"]
                              }
                            ]
                          });

//@ Module help, function(string)
\? sampleModuleJS

//@ Help, function(string)
\? sampleModuleJS.stringFunction

//@ Usage, function(string)
testutil.sampleModuleJS.stringFunction(5)
testutil.sampleModuleJS.stringFunction("whatever")
testutil.sampleModuleJS.stringFunction("one")


//@ Register, function(dictionary)
var f2 = function(data) {
  if (data) {
    if (data.myOption)
      println("Function data: " + data.myOption);
    else
      println ("Full data:" + data)
  } else {
    println("No function data available");
  }
}

shell.addExtensionObjectMember(testutil.sampleModuleJS, "dictFunction", f2,
                          {
                            brief:"Brief definition for dictFunction.",
                            details: ["Detailed description for dictFunction"],
                            parameters:
                            [
                              {
                                name: "data",
                                type: "dictionary",
                                brief: "Short description for dictionary parameter.",
                                required: false,
                                details: ["Detailed description for dictionary parameter."],
                                options: [
                                  {
                                    name: "myOption",
                                    brief: "A sample option",
                                    type: "string",
                                    details: ["Details for the sample option"],
                                    values: ["test", "value"],
                                    required: true
                                  }
                                ]
                              }
                            ]
                          });

//@ Module help, function(dictionary)
\? sampleModuleJS

//@ Help, function(dictionary)
\? sampleModuleJS.dictFunction

//@ Usage, function(dictionary)
testutil.sampleModuleJS.dictFunction({})
testutil.sampleModuleJS.dictFunction({"someOption": 5})
testutil.sampleModuleJS.dictFunction({"myOption": 5})
testutil.sampleModuleJS.dictFunction({"myOption": "whatever"})
testutil.sampleModuleJS.dictFunction()
testutil.sampleModuleJS.dictFunction({"myOption": "test"})

//@ Register, function(dictionary), no option validation
shell.addExtensionObjectMember(testutil.sampleModuleJS, "freeDictFunction", f2,
                          {
                            brief:"Brief definition for dictFunction.",
                            details: ["Detailed description for dictFunction"],
                            parameters:
                            [
                              {
                                name: "data",
                                type: "dictionary",
                                brief: "Dictinary containing anything.",
                                required: false,
                                details: ["Detailed description for dictionary parameter."],
                              }
                            ]
                          });

//@ Usage, function(dictionary), no option validation
testutil.sampleModuleJS.freeDictFunction({})
testutil.sampleModuleJS.freeDictFunction({"someOption": 5})
testutil.sampleModuleJS.freeDictFunction({"myOption": 5})
testutil.sampleModuleJS.freeDictFunction({"myOption": "whatever"})
testutil.sampleModuleJS.freeDictFunction()
testutil.sampleModuleJS.freeDictFunction({"myOption": "test"})


//@ Register, function(Session)
var f3 = function(data) {
  println ("Active Session:", data)
}

shell.addExtensionObjectMember(testutil.sampleModuleJS, "objectFunction1", f3,
                          {
                            brief:"Brief definition for objectFunction.",
                            details: ["Detailed description for objectFunction"],
                            parameters:
                            [
                              {
                                name: "session",
                                type: "object",
                                brief: "Short description for object parameter.",
                                details: ["Detailed description for object parameter."],
                                class: "Session"
                              }
                            ]
                          });

//@ Module help, function(Session)
\? sampleModuleJS

//@ Help, function(Session)
\? sampleModuleJS.objectFunction1

//@ Usage, function(Session)
shell.connect(__mysqluripwd)
testutil.sampleModuleJS.objectFunction1(session)
session.close()

shell.connect(__uripwd)
testutil.sampleModuleJS.objectFunction1(session)
session.close()

//@ Register, function(Session and ClassicSession)
var f4 = function(data) {
  println ("Active Session:", data)
}

shell.addExtensionObjectMember(testutil.sampleModuleJS, "objectFunction2", f4,
                          {
                            brief:"Brief definition for objectFunction.",
                            details: ["Detailed description for objectFunction"],
                            parameters:
                            [
                              {
                                name: "session",
                                type: "object",
                                brief: "Short description for object parameter.",
                                details: ["Detailed description for object parameter."],
                                classes: ["Session", "ClassicSession"]
                              }
                            ]
                          });

//@ Module help, function(Session and ClassicSession)
\? sampleModuleJS

//@ Help, function(Session and ClassicSession)
\? sampleModuleJS.objectFunction2

//@ Usage, function(Session and ClassicSession)
shell.connect(__mysqluripwd)
testutil.sampleModuleJS.objectFunction2(session)
session.close()

shell.connect(__uripwd)
testutil.sampleModuleJS.objectFunction2(session)
session.close()

//@ Registration errors, function definition
var f5 = function(whatever) {
}
shell.addExtensionObjectMember("object", "function", f5);

shell.addExtensionObjectMember(shell, "function", f5);

shell.addExtensionObjectMember(testutil.sampleModuleJS, 25, f5);

shell.addExtensionObjectMember(testutil.sampleModuleJS, "function", f5,
                          {
                            brief:"Brief definition for function.",
                            details: ["Detailed description for function"],
                            extra: "This will cause a failure"
                          });

shell.addExtensionObjectMember(testutil.sampleModuleJS, "function", f5,
                          {
                            brief:5,
                            details: ["Detailed description for function"]
                          });

shell.addExtensionObjectMember(testutil.sampleModuleJS, "function", f5,
                          {
                            brief:"Brief definition for function.",
                            details: 45,
                          });

shell.addExtensionObjectMember(testutil.sampleModuleJS, "function", f5,
                          {
                            brief:"Brief definition for function.",
                            details: ["Detailed description for function", 34],
                          });

shell.addExtensionObjectMember(testutil.sampleModuleJS, "function", f5,
                          {
                            brief:"Brief definition for function.",
                            details: ["Detailed description for function"],
                            parameters:34
                          });

shell.addExtensionObjectMember(testutil.sampleModuleJS, "function", f5,
                          {
                            brief:"Brief definition for function.",
                            details: ["Detailed description for function"],
                            parameters: [23]
                          });

//@ Registration errors, parameters
shell.addExtensionObjectMember(testutil.sampleModuleJS, "function", f5,
                          {
                            parameters:[{}]
                          });

shell.addExtensionObjectMember(testutil.sampleModuleJS, "function", f5,
                          {
                            parameters:[
                            {
                              name: "sample",
                              type: 5,
                            }]
                          });

//@ Registration errors, integer parameters
shell.addExtensionObjectMember(testutil.sampleModuleJS, "function", f5,
                          {
                            parameters:[
                            {
                              name: "sample",
                              type: "integer",
                              class: "unexisting",
                              classes: [],
                              options: [],
                              values: [1,2,3]
                            }]
                          });

//@ Registration errors, float parameters
shell.addExtensionObjectMember(testutil.sampleModuleJS, "function", f5,
                          {
                            parameters:[
                            {
                              name: "sample",
                              type: "float",
                              class: "unexisting",
                              classes: [],
                              options: [],
                              values: [1,2,3]
                            }]
                          });

//@ Registration errors, bool parameters
shell.addExtensionObjectMember(testutil.sampleModuleJS, "function", f5,
                          {
                            parameters:[
                            {
                              name: "sample",
                              type: "bool",
                              class: "unexisting",
                              classes: [],
                              options: [],
                              values: [1,2,3]
                            }]
                          });

//@ Registration errors, string parameters
shell.addExtensionObjectMember(testutil.sampleModuleJS, "function", f5,
                          {
                            parameters:[
                            {
                              name: "sample",
                              type: "string",
                              class: "unexisting",
                              classes: [],
                              options:[]
                            }]
                          });

shell.addExtensionObjectMember(testutil.sampleModuleJS, "function", f5,
                          {
                            parameters:[
                            {
                              name: "sample",
                              type: "string",
                              values: 5
                            }]
                          });

shell.addExtensionObjectMember(testutil.sampleModuleJS, "function", f5,
                          {
                            parameters:[
                            {
                              name: "sample",
                              type: "string",
                              values: [5]
                            }]
                          });

//@ Registration errors, object parameters
shell.addExtensionObjectMember(testutil.sampleModuleJS, "function", f5,
                          {
                            parameters:[
                            {
                              name: "sample",
                              type: "object",
                              options:[],
                              values: []
                            }]
                          });

shell.addExtensionObjectMember(testutil.sampleModuleJS, "function", f5,
                          {
                            parameters:[
                            {
                              name: "sample",
                              type: "object",
                              class:45
                            }]
                          });

shell.addExtensionObjectMember(testutil.sampleModuleJS, "function", f5,
                          {
                            parameters:[
                            {
                              name: "sample",
                              type: "object",
                              classes:[45]
                            }]
                          });

//@ Registration errors, dictionary parameters
shell.addExtensionObjectMember(testutil.sampleModuleJS, "function", f5,
                          {
                            parameters:[
                            {
                              name: "sample",
                              type: "dictionary",
                              class: "unexisting",
                              classes: [],
                              values: []
                            }]
                          });

shell.addExtensionObjectMember(testutil.sampleModuleJS, "function", f5,
                          {
                            parameters:[
                            {
                              name: "sample",
                              type: "dictionary",
                              options: [45]
                            }]
                          });

shell.addExtensionObjectMember(testutil.sampleModuleJS, "function", f5,
                          {
                            parameters:[
                            {
                              name: "sample",
                              type: "dictionary",
                              options: [{}]
                            }]
                          });

//@ Registration errors, invalid identifiers
testutil.registerModule("testutil", "my module")

shell.addExtensionObjectMember(testutil.sampleModuleJS, "my function", f5);

shell.addExtensionObjectMember(testutil.sampleModuleJS, "function", f5,
                          {
                            parameters:[
                            {
                              name: "a sample",
                              type: "string",
                            }]
                          });

shell.addExtensionObjectMember(testutil.sampleModuleJS, "function", f5,
                          {
                            parameters:[
                            {
                              name: "sample",
                              type: "dictionary",
                              options: [{
                                name:'an invalid name',
                                type:'string'
                              }]
                            }]
                          });


var f6 = function(data) {
  println("Some random data: " + data);
}

