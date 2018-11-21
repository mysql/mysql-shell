# Object Registration
testutil.enable_extensible()

#@ Registration from C++, the parent class
\? testutil

#@ Registration from C++, a test module
\? sampleModulePY

#@ Registration from C++, register_module function
\? register_module

#@ Registration from C++, register_function function
\? register_function

#@ Register, function(string)
def f1(data):
  print "Python Function Definition: ", data

testutil.register_function("testutil.sampleModulePY", "stringFunction", f1,
                          {
                            "brief":"Brief description for stringFunction.",
                            "details": ["Detailed description for stringFunction"],
                            "parameters":
                            [
                              {
                                "name": "data",
                                "type": "string",
                                "brief": "Brief description for string parameter.",
                                "details": ["Detailed description for string parameter."],
                                "values": ["one", "two", "three"]
                              }
                            ]
                          });

#@ Module help, function(string)
\? sampleModulePY

#@ Help, function(string)
\? sampleModulePY.string_function

#@ Usage, function(string)
testutil.sampleModulePY.string_function(5)
testutil.sampleModulePY.string_function("whatever")
testutil.sampleModulePY.string_function("one")


#@ Register, function(dictionary)
def f2(data=None):
  if data:
    print "Function data: ", data['myOption']
  else:
    print "No function data available"

testutil.register_function("testutil.sampleModulePY", "dictFunction", f2,
                          {
                            "brief":"Brief definition for dictFunction.",
                            "details": ["Detailed description for dictFunction"],
                            "parameters":
                            [
                              {
                                "name": "data",
                                "type": "dictionary",
                                "brief": "Short description for dictionary parameter.",
                                "required": False,
                                "details": ["Detailed description for dictionary parameter."],
                                "options": [
                                  {
                                    "name": "myOption",
                                    "brief": "A sample option",
                                    "type": "string",
                                    "details": ["Details for the sample option"],
                                    "values": ["test", "value"],
                                    "required": True
                                  }
                                ]
                              }
                            ]
                          });

#@ Module help, function(dictionary)
\? sampleModulePY

#@ Help, function(dictionary)
\? sampleModulePY.dict_function

#@ Usage, function(dictionary)
testutil.sampleModulePY.dict_function({})
testutil.sampleModulePY.dict_function({"someOption": 5})
testutil.sampleModulePY.dict_function({"myOption": 5})
testutil.sampleModulePY.dict_function({"myOption": "whatever"})
testutil.sampleModulePY.dict_function()
testutil.sampleModulePY.dict_function({"myOption": "test"})

#@ Register, function(Session)
def f3(data):
  print "Active Session:", data

testutil.register_function("testutil.sampleModulePY", "objectFunction1", f3,
                          {
                            "brief":"Brief definition for objectFunction.",
                            "details": ["Detailed description for objectFunction"],
                            "parameters":
                            [
                              {
                                "name": "session",
                                "type": "object",
                                "brief": "Short description for object parameter.",
                                "details": ["Detailed description for object parameter."],
                                "class": "Session"
                              }
                            ]
                          });

#@ Module help, function(Session)
\? sampleModulePY

#@ Help, function(Session)
\? sampleModulePY.object_function1

#@ Usage, function(Session)
shell.connect(__mysqluripwd)
testutil.sampleModulePY.object_function1(session)
session.close()

shell.connect(__uripwd)
testutil.sampleModulePY.object_function1(session)
session.close()

#@ Register, function(Session and ClassicSession)
def f4(data):
  print "Active Session:", data

testutil.register_function("testutil.sampleModulePY", "objectFunction2", f4,
                          {
                            "brief":"Brief definition for objectFunction.",
                            "details": ["Detailed description for objectFunction"],
                            "parameters":
                            [
                              {
                                "name": "session",
                                "type": "object",
                                "brief": "Short description for object parameter.",
                                "details": ["Detailed description for object parameter."],
                                "classes": ["Session", "ClassicSession"]
                              }
                            ]
                          });

#@ Module help, function(Session and ClassicSession)
\? sampleModulePY

#@ Help, function(Session and ClassicSession)
\? sampleModulePY.object_function2

#@ Usage, function(Session and ClassicSession)
shell.connect(__mysqluripwd)
testutil.sampleModulePY.object_function2(session)
session.close()

shell.connect(__uripwd)
testutil.sampleModulePY.object_function2(session)
session.close()

#@ Registration errors, function definition
def f5(whatever):
  pass

testutil.register_function("testutil.unexistingModule", "function", f5,
                          {
                            "brief":"Brief definition for function.",
                            "details": ["Detailed description for function"]
                          });

testutil.register_function("testutil.sampleModulePY", "function", f5,
                          {
                            "brief":"Brief definition for function.",
                            "details": ["Detailed description for function"],
                            "extra": "This will cause a failure"
                          });

testutil.register_function("testutil.sampleModulePY", "function", f5,
                          {
                            "brief":5,
                            "details": ["Detailed description for function"]
                          });

testutil.register_function("testutil.sampleModulePY", "function", f5,
                          {
                            "brief":"Brief definition for function.",
                            "details": 45,
                          });

testutil.register_function("testutil.sampleModulePY", "function", f5,
                          {
                            "brief":"Brief definition for function.",
                            "details": ["Detailed description for function", 34],
                          });

testutil.register_function("testutil.sampleModulePY", "function", f5,
                          {
                            "brief":"Brief definition for function.",
                            "details": ["Detailed description for function"],
                            "parameters":34
                          });

testutil.register_function("testutil.sampleModulePY", "function", f5,
                          {
                            "brief":"Brief definition for function.",
                            "details": ["Detailed description for function"],
                            "parameters": [23]
                          });

#@ Registration errors, parameters
testutil.register_function("testutil.sampleModulePY", "function", f5,
                          {
                            "parameters":[{}]
                          });

testutil.register_function("testutil.sampleModulePY", "function", f5,
                          {
                            "parameters":[
                            {
                              "name": "sample",
                              "type": 5,
                            }]
                          });

#@ Registration errors, integer parameters
testutil.register_function("testutil.sampleModulePY", "function", f5,
                          {
                            "parameters":[
                            {
                              "name": "sample",
                              "type": "integer",
                              "class": "unexisting",
                              "classes": [],
                              "options": [],
                              "values": [1,2,3]
                            }]
                          });

#@ Registration errors, float parameters
testutil.register_function("testutil.sampleModulePY", "function", f5,
                          {
                            "parameters":[
                            {
                              "name": "sample",
                              "type": "float",
                              "class": "unexisting",
                              "classes": [],
                              "options": [],
                              "values": [1,2,3]
                            }]
                          });

#@ Registration errors, bool parameters
testutil.register_function("testutil.sampleModulePY", "function", f5,
                          {
                            "parameters":[
                            {
                              "name": "sample",
                              "type": "bool",
                              "class": "unexisting",
                              "classes": [],
                              "options": [],
                              "values": [1,2,3]
                            }]
                          });

#@ Registration errors, string parameters
testutil.register_function("testutil.sampleModulePY", "function", f5,
                          {
                            "parameters":[
                            {
                              "name": "sample",
                              "type": "string",
                              "class": "unexisting",
                              "classes": [],
                              "options":[]
                            }]
                          });

testutil.register_function("testutil.sampleModulePY", "function", f5,
                          {
                            "parameters":[
                            {
                              "name": "sample",
                              "type": "string",
                              "values": 5
                            }]
                          });

testutil.register_function("testutil.sampleModulePY", "function", f5,
                          {
                            "parameters":[
                            {
                              "name": "sample",
                              "type": "string",
                              "values": [5]
                            }]
                          });

#@ Registration errors, object parameters
testutil.register_function("testutil.sampleModulePY", "function", f5,
                          {
                            "parameters":[
                            {
                              "name": "sample",
                              "type": "object",
                              "options":[],
                              "values": []
                            }]
                          });

testutil.register_function("testutil.sampleModulePY", "function", f5,
                          {
                            "parameters":[
                            {
                              "name": "sample",
                              "type": "object",
                              "class":45
                            }]
                          });

testutil.register_function("testutil.sampleModulePY", "function", f5,
                          {
                            "parameters":[
                            {
                              "name": "sample",
                              "type": "object",
                              "classes":[]
                            }]
                          });

testutil.register_function("testutil.sampleModulePY", "function", f5,
                          {
                            "parameters":[
                            {
                              "name": "sample",
                              "type": "object",
                              "classes":[45]
                            }]
                          });

#@ Registration errors, dictionary parameters
testutil.register_function("testutil.sampleModulePY", "function", f5,
                          {
                            "parameters":[
                            {
                              "name": "sample",
                              "type": "dictionary",
                              "class": "unexisting",
                              "classes": [],
                              "values": []
                            }]
                          });

testutil.register_function("testutil.sampleModulePY", "function", f5,
                          {
                            "parameters":[
                            {
                              "name": "sample",
                              "type": "dictionary",
                              "options": []
                            }]
                          });

testutil.register_function("testutil.sampleModulePY", "function", f5,
                          {
                            "parameters":[
                            {
                              "name": "sample",
                              "type": "dictionary",
                              "options": [45]
                            }]
                          });

testutil.register_function("testutil.sampleModulePY", "function", f5,
                          {
                            "parameters":[
                            {
                              "name": "sample",
                              "type": "dictionary",
                              "options": [{}]
                            }]
                          });

#@ Registration errors, invalid identifiers
testutil.register_module("testutil", "my module")

testutil.register_function("testutil.sampleModulePY", "my function", f5);

testutil.register_function("testutil.sampleModulePY", "function", f5,
                          {
                            "parameters":[
                            {
                              "name": "a sample",
                              "type": "string",
                            }]
                          });

testutil.register_function("testutil.sampleModulePY", "function", f5,
                          {
                            "parameters":[
                            {
                              "name": "sample",
                              "type": "dictionary",
                              "options": [{
                                "name":'an invalid name',
                                "type":'string'
                              }]
                            }]
                          });


def f6(data):
  print "Some random data:", data

#@ Function with custom names, registration error
testutil.register_function("testutil.sampleModulePY", "js_function|py_function|another", f6);

#@ Function with custom names, registration ok
testutil.register_function("testutil.sampleModulePY", "js_function|py_function", f6,
                          {
                            "brief":"Brief description for stringFunction.",
                            "details": ["Detailed description for stringFunction"],
                            "parameters":
                            [
                              {
                                "name": "data",
                                "type": "string",
                                "brief": "Brief description for string parameter.",
                                "details": ["Detailed description for string parameter."],
                              }
                            ]
                          });

#@ Function with custom names, help on module
\? sampleModulePY

#@ Function with custom names, help on function
\? sampleModulePY.py_function

#@ Function with custom names, usage
testutil.sampleModulePY.py_function("some data")
