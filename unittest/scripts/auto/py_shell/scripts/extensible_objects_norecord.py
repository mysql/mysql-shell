from __future__ import print_function

# Object Registration
testutil.enable_extensible()

#@ Registration from C++, the parent class
\? testutil

#@ Registration from C++, a test module
\? sample_module_p_y

#@ Registration from C++, register_module function
\? register_module

#@ Registration from C++, register_function function
\? register_function

#@ Register, function(string)
def f1(data):
  print("Python Function Definition: ", data)

shell.add_extension_object_member(testutil.sample_module_p_y, "stringFunction", f1,
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
\? sample_module_p_y

#@ Help, function(string)
\? sample_module_p_y.string_function

#@ Usage, function(string)
testutil.sample_module_p_y.string_function(5)
testutil.sample_module_p_y.string_function("whatever")
testutil.sample_module_p_y.string_function("one")


#@ Register, function(dictionary)
def f2(data=None):
  if data:
    try:
      print("Function data: ", data.myOption)
    except AttributeError:
      print("Full data:", data)
  else:
    print("No function data available")

shell.add_extension_object_member(testutil.sample_module_p_y, "dictFunction", f2,
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
\? sample_module_p_y

#@ Help, function(dictionary)
\? sample_module_p_y.dict_function

#@ Usage, function(dictionary)
testutil.sample_module_p_y.dict_function({})
testutil.sample_module_p_y.dict_function({"someOption": 5})
testutil.sample_module_p_y.dict_function({"myOption": 5})
testutil.sample_module_p_y.dict_function({"myOption": "whatever"})
testutil.sample_module_p_y.dict_function()
testutil.sample_module_p_y.dict_function({"myOption": "test"})

#@ Register, function(dictionary), no option validation
shell.add_extension_object_member(testutil.sample_module_p_y, "freeDictFunction", f2,
                          {
                            "brief":"Brief definition for dictFunction.",
                            "details": ["Detailed description for dictFunction"],
                            "parameters":
                            [
                              {
                                "name": "data",
                                "type": "dictionary",
                                "brief": "Dictinary containing anything.",
                                "required": False,
                                "details": ["Detailed description for dictionary parameter."],
                              }
                            ]
                          });

#@ Usage, function(dictionary), no option validation
testutil.sample_module_p_y.free_dict_function({})
testutil.sample_module_p_y.free_dict_function({"someOption": 5})
testutil.sample_module_p_y.free_dict_function({"myOption": 5})
testutil.sample_module_p_y.free_dict_function({"myOption": "whatever"})
testutil.sample_module_p_y.free_dict_function()
testutil.sample_module_p_y.free_dict_function({"myOption": "test"})


#@ Register, function(Session)
def f3(data):
  print("Active Session:", data)

shell.add_extension_object_member(testutil.sample_module_p_y, "objectFunction1", f3,
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
\? sample_module_p_y

#@ Help, function(Session)
\? sample_module_p_y.object_function1

#@ Usage, function(Session)
shell.connect(__mysqluripwd)
testutil.sample_module_p_y.object_function1(session)
session.close()

shell.connect(__uripwd)
testutil.sample_module_p_y.object_function1(session)
session.close()

#@ Register, function(Session and ClassicSession)
def f4(data):
  print("Active Session:", data)

shell.add_extension_object_member(testutil.sample_module_p_y, "objectFunction2", f4,
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
\? sample_module_p_y

#@ Help, function(Session and ClassicSession)
\? sample_module_p_y.object_function2

#@ Usage, function(Session and ClassicSession)
shell.connect(__mysqluripwd)
testutil.sample_module_p_y.object_function2(session)
session.close()

shell.connect(__uripwd)
testutil.sample_module_p_y.object_function2(session)
session.close()

#@ Registration errors, function definition
def f5(whatever):
  pass

shell.add_extension_object_member("object", "function", f5);
shell.add_extension_object_member(shell, "function", f5);
shell.add_extension_object_member(testutil.sample_module_p_y, 25, f5);
shell.add_extension_object_member(testutil.sample_module_p_y, "function", f5,
                          {
                            "brief":"Brief definition for function.",
                            "details": ["Detailed description for function"],
                            "extra": "This will cause a failure"
                          });

shell.add_extension_object_member(testutil.sample_module_p_y, "function", f5,
                          {
                            "brief":5,
                            "details": ["Detailed description for function"]
                          });

shell.add_extension_object_member(testutil.sample_module_p_y, "function", f5,
                          {
                            "brief":"Brief definition for function.",
                            "details": 45,
                          });

shell.add_extension_object_member(testutil.sample_module_p_y, "function", f5,
                          {
                            "brief":"Brief definition for function.",
                            "details": ["Detailed description for function", 34],
                          });

shell.add_extension_object_member(testutil.sample_module_p_y, "function", f5,
                          {
                            "brief":"Brief definition for function.",
                            "details": ["Detailed description for function"],
                            "parameters":34
                          });

shell.add_extension_object_member(testutil.sample_module_p_y, "function", f5,
                          {
                            "brief":"Brief definition for function.",
                            "details": ["Detailed description for function"],
                            "parameters": [23]
                          });

#@ Registration errors, parameters
shell.add_extension_object_member(testutil.sample_module_p_y, "function", f5,
                          {
                            "parameters":[{}]
                          });

shell.add_extension_object_member(testutil.sample_module_p_y, "function", f5,
                          {
                            "parameters":[
                            {
                              "name": "sample",
                              "type": 5,
                            }]
                          });

#@ Registration errors, integer parameters
shell.add_extension_object_member(testutil.sample_module_p_y, "function", f5,
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
shell.add_extension_object_member(testutil.sample_module_p_y, "function", f5,
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
shell.add_extension_object_member(testutil.sample_module_p_y, "function", f5,
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
shell.add_extension_object_member(testutil.sample_module_p_y, "function", f5,
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

shell.add_extension_object_member(testutil.sample_module_p_y, "function", f5,
                          {
                            "parameters":[
                            {
                              "name": "sample",
                              "type": "string",
                              "values": 5
                            }]
                          });

shell.add_extension_object_member(testutil.sample_module_p_y, "function", f5,
                          {
                            "parameters":[
                            {
                              "name": "sample",
                              "type": "string",
                              "values": [5]
                            }]
                          });

#@ Registration errors, dictionary parameters
shell.add_extension_object_member(testutil.sample_module_p_y, "function", f5,
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

shell.add_extension_object_member(testutil.sample_module_p_y, "function", f5,
                          {
                            "parameters":[
                            {
                              "name": "sample",
                              "type": "dictionary",
                              "options": [45]
                            }]
                          });

shell.add_extension_object_member(testutil.sample_module_p_y, "function", f5,
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

shell.add_extension_object_member(testutil.sample_module_p_y, "my function", f5);

shell.add_extension_object_member(testutil.sample_module_p_y, "function", f5,
                          {
                            "parameters":[
                            {
                              "name": "a sample",
                              "type": "string",
                            }]
                          });

shell.add_extension_object_member(testutil.sample_module_p_y, "function", f5,
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
  print("Some random data:", data)

