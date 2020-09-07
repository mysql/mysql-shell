###############################################################################
#                            Errors Creating Object
###############################################################################
#@ Shell create object errors
obj = shell.create_extension_object(5)

#@ Shell create object ok
obj = shell.create_extension_object()
obj

###############################################################################
#                            Errors Adding Members
###############################################################################
# Signature is
# shell.add_extension_object_member(UserObject object,
#                                   String memberName,
#                                   ValidType member[,
#                                   Dictionary memberDefinition])
#
# ValidType is any of:
# - Function
# - Array
# - Dictionary
# - Null/None
# - Bool
# - Integer
# - UInteger
# - Float
# - String
#
# Not Allowed: Object
#
# The member definition accepts the following attributes
#
# - brief: string
# - details: array of strings
#
# If the member is a Function then an additional attribute
# is allowed:
#
# - parameters: list of parameter definition dictionaries, each entry on this
#               list defines the parameters that the function being registered
#               as a new object member accepts.
#
# A parameter definition dictionary accepts the following attributes:
#
# - name: the name of the parameter.
# - type: the data type of the parameter, allowed values include:
#         - string
#         - integer
#         - boolean
#         - float
#         - array
#         - dictionary
#         - object
# - required: a boolean value indicating whether the parameter is mandatory
#             (true) or optional (false).
# - brief a short description of the parameter.
# - details: a string list with additional details about the parameter.
#
# If a parameter type is 'object' the following attributes are also allowed on
# the parameter definition dictionary
#
# class: defines the object class allowed as parameter
# classes: a list of classes allowed as parameter
#
# If a parameter type is 'dictionary' the following attribute is also allowed
# on the parameter definition dictionary:
#
# - options: list of option definition dictionaries defining the allowed
#            options that can be passed on the dictionary parameter
#
# An option definition dictionary follows exactly the same rules as the
# parameter definition dictionary.
###############################################################################
def f5(whatever):
  pass

#@ object parameter should be an object
shell.add_extension_object_member("object", "function", f5);

#@ but not any object, an extension object
shell.add_extension_object_member(shell, "function", f5);

#@ name parameters must be a string
shell.add_extension_object_member(obj, 25, 1);

#@ name parameters must be a valid identifier (member)
shell.add_extension_object_member(obj, "my name", 1);

#@ name parameters must be a valid identifier (function)
shell.add_extension_object_member(obj, "my name", f5);

#@ member definition must be a dictionary
shell.add_extension_object_member(obj, "function", f5, 5);

#@ member definition 'brief' must be a string
shell.add_extension_object_member(obj, "function", f5,
                          {
                            "brief":5
                          });

#@ member definition 'details' must be an array
shell.add_extension_object_member(obj, "function", f5,
                          {
                            "brief":"Brief definition for function.",
                            "details": 45,
                          });

#@ member definition 'details' must be an array of strings
shell.add_extension_object_member(obj, "function", f5,
                          {
                            "brief":"Brief definition for function.",
                            "details": ["Detailed description for function", 34],
                          });

#@ function definition does not accept other attributes
shell.add_extension_object_member(obj, "function", f5,
                          {
                            "brief":"Brief definition for function.",
                            "details": ["Detailed description for function"],
                            "extra": "This will cause a failure"
                          });

#@ member definition does not accept other attributes
shell.add_extension_object_member(obj, "member", 1,
                          {
                            "brief":"Brief definition for function.",
                            "details": ["Detailed description for function"],
                            "extra": "This will cause a failure"
                          });

#@ member definition does not accept 'parameters' if member is not a function
members = [1, 2.5, "sample", True,None, [1,2], {"a":1}]
for member in members:
  shell.add_extension_object_member(obj, "member", member,
                            {
                              "brief":"Brief definition for function.",
                              "details": ["Detailed description for function"],
                              "extra": "This will cause a failure"
                            });

#@ member definition 'parameters' must be an array
shell.add_extension_object_member(obj, "function", f5,
                          {
                            "brief":"Brief definition for function.",
                            "details": ["Detailed description for function"],
                            "parameters":34
                          });

#@ member definition 'parameters' must be an array of dictionaries
shell.add_extension_object_member(obj, "function", f5,
                          {
                            "brief":"Brief definition for function.",
                            "details": ["Detailed description for function"],
                            "parameters": [23]
                          });

#@ A parameter definition requires name
shell.add_extension_object_member(obj, "function", f5,
                          {
                            "parameters":[{}]
                          });

#@ A parameter definition requires string on name
shell.add_extension_object_member(obj, "function", f5,
                          {
                            "parameters":[
                            {
                              "name": 5,
                            }]
                          });

#@ A parameter definition requires valid identifier on name
shell.add_extension_object_member(obj, "function", f5,
                          {
                            "parameters":[
                            {
                              "name": "my sample",
                              "type": "my type",
                            }]
                          });

#@ A parameter definition requires a string on type
shell.add_extension_object_member(obj, "function", f5,
                          {
                            "parameters":[
                            {
                              "name": "sample",
                              "type": 5,
                            }]
                          });

#@ A parameter definition requires a valid type on type
shell.add_extension_object_member(obj, "function", f5,
                          {
                            "parameters":[
                            {
                              "name": "sample",
                              "type": "whatever",
                            }]
                          });

#@ A parameter definition requires a boolean on required
shell.add_extension_object_member(obj, "function", f5,
                          {
                            "parameters":[
                            {
                              "name": "sample",
                              "type": "whatever",
                              "required": "something weird"
                            }]
                          });


#@ On parameters, duplicate definitions are not allowed
shell.add_extension_object_member(obj, "function", f5,
                          {
                            "parameters":[
                            {
                              "name": "myName",
                              "type": "string",
                            },
                            {
                              "name": "myname",
                              "type": "string",
                            }]
                          });

#@ On parameters, required ones can't come after optional ones
shell.add_extension_object_member(obj, "function", f5,
                          {
                            "parameters":[
                            {
                              "name": "optional",
                              "type": "string",
                              "required": False
                            },
                            {
                              "name": "required",
                              "type": "string",
                              "required": True
                            }]
                          });

#@ A parameter definition requires a string on brief
shell.add_extension_object_member(obj, "function", f5,
                          {
                            "parameters":[
                            {
                              "name": "sample",
                              "type": "whatever",
                              "brief": 5
                            }]
                          });

#@ A parameter definition requires a string on details
shell.add_extension_object_member(obj, "function", f5,
                          {
                            "parameters":[
                            {
                              "name": "sample",
                              "type": "whatever",
                              "brief": "brief parameter description",
                              "details": 5
                            }]
                          });

#@ A parameter definition requires just strings on details
shell.add_extension_object_member(obj, "function", f5,
                          {
                            "parameters":[
                            {
                              "name": "sample",
                              "type": "whatever",
                              "brief": "brief parameter description",
                              "details": ["sample", 5]
                            }]
                          });

#@ Non object parameters do not accept 'class' or 'classes' attributes
for invalid in ['class', 'classes']:
  for type in ["string", "integer", "float", "bool", "array", "dictionary", "boolean"]:
    try:
      shell.add_extension_object_member(obj, "function", f5,
                              {
                                "parameters":[
                                {
                                  "name": "sample",
                                  "type": type,
                                  invalid: "whatever"
                                }]
                              });
    except Exception as err:
      print(err)

#@ Non dictionary parameters do not accept 'options' attribute
for type in ["string", "integer", "float", "bool", "array", "object", "boolean"]:
  try:
    shell.add_extension_object_member(obj, "function", f5,
                            {
                              "parameters":[
                              {
                                "name": "sample",
                                "type": type,
                                "options": "whatever"
                              }]
                            });
  except Exception as err:
    print(err)

#@ Non string parameters do not accept 'values' attribute
for type in ["dictionary", "integer", "float", "bool", "array", "object", "boolean"]:
  try:
    shell.add_extension_object_member(obj, "function", f5,
                            {
                              "parameters":[
                              {
                                "name": "sample",
                                "type": type,
                                "values": "whatever"
                              }]
                            });
  except Exception as err:
    print(err)


#@ String parameters 'values' must be an array
shell.add_extension_object_member(obj, "function", f5,
                          {
                            "parameters":[
                            {
                              "name": "sample",
                              "type": "string",
                              "values": 5
                            }]
                          });

#@ String parameter 'values' must be an array of strings
shell.add_extension_object_member(obj, "function", f5,
                          {
                            "parameters":[
                            {
                              "name": "sample",
                              "type": "string",
                              "values": [5]
                            }]
                          });


#@ Object parameter 'class' must be a string
shell.add_extension_object_member(obj, "function", f5,
                          {
                            "parameters":[
                            {
                              "name": "sample",
                              "type": "object",
                              "class": 5
                            }]
                          });

#@ Object parameter 'class' can not be empty
shell.add_extension_object_member(obj, "function", f5,
                          {
                            "parameters":[
                            {
                              "name": "sample",
                              "type": "object",
                              "class": ""
                            }]
                          });

#@ Object parameter 'classes' must be an array
shell.add_extension_object_member(obj, "function", f5,
                          {
                            "parameters":[
                            {
                              "name": "sample",
                              "type": "object",
                              "classes": 5
                            }]
                          });

#@ Object parameter 'classes' must be an array of strings
shell.add_extension_object_member(obj, "function", f5,
                          {
                            "parameters":[
                            {
                              "name": "sample",
                              "type": "object",
                              "classes": ["one", 2]
                            }]
                          });

#@ Object parameter 'classes' can not be an empty array
shell.add_extension_object_member(obj, "function", f5,
                          {
                            "parameters":[
                            {
                              "name": "sample",
                              "type": "object",
                              "classes": []
                            }]
                          });

#@ Object parameter 'class' must hold a valid class name
shell.add_extension_object_member(obj, "function", f5,
                          {
                            "parameters":[
                            {
                              "name": "sample",
                              "type": "object",
                              "class": "unknown"
                            }]
                          });

#@ Object parameter 'classes' must hold valid class names (singular)
shell.add_extension_object_member(obj, "function", f5,
                          {
                            "parameters":[
                            {
                              "name": "sample",
                              "type": "object",
                              "classes": ["Session", "Table", "Weirdie"]
                            }]
                          });

#@ Object parameter 'classes' must hold valid class names (plural)
shell.add_extension_object_member(obj, "function", f5,
                          {
                            "parameters":[
                            {
                              "name": "sample",
                              "type": "object",
                              "classes": ["Unexisting", "Table", "Weirdie"]
                            }]
                          });

#@ Dictionary parameter 'options' should be an array
shell.add_extension_object_member(obj, "function", f5,
                          {
                            "parameters":[
                            {
                              "name": "sample",
                              "type": "dictionary",
                              "options": 5
                            }]
                          });

#@ Dictionary parameter 'options' must be an array of dictionaries
shell.add_extension_object_member(obj, "function", f5,
                          {
                            "parameters":[
                            {
                              "name": "sample",
                              "type": "dictionary",
                              "options": [45]
                            }]
                          });

#@ Parameter option definition requires type and name, missing both
shell.add_extension_object_member(obj, "function", f5,
                          {
                            "parameters":[
                            {
                              "name": "sample",
                              "type": "dictionary",
                              "options": [{}]
                            }]
                          });

#@ Parameter option definition 'required' must be boolean
shell.add_extension_object_member(obj, "function", f5,
                          {
                            "parameters":[
                            {
                              "name": "sample",
                              "type": "dictionary",
                              "options": [
                              {
                                "name": "myOption",
                                "type": "string",
                                "required": "myOption",
                              }]
                            }]
                          });

#@ Parameter option definition 'brief' must be string
shell.add_extension_object_member(obj, "function", f5,
                          {
                            "parameters":[
                            {
                              "name": "sample",
                              "type": "dictionary",
                              "options": [
                              {
                                "name": "myOption",
                                "type": "string",
                                "required": True,
                                "brief": 5,
                              }]
                            }]
                          });

#@ Parameter option definition 'details' must be array
shell.add_extension_object_member(obj, "function", f5,
                          {
                            "parameters":[
                            {
                              "name": "sample",
                              "type": "dictionary",
                              "options": [
                              {
                                "name": "myOption",
                                "type": "string",
                                "required": False,
                                "details": 5,
                              }]
                            }]
                          });

#@ Parameter option definition 'details' must be array of strings
shell.add_extension_object_member(obj, "function", f5,
                          {
                            "parameters":[
                            {
                              "name": "sample",
                              "type": "dictionary",
                              "options": [
                              {
                                "name": "myOption",
                                "type": "string",
                                "required": True,
                                "brief": "brief option description",
                                "details": ["hello", 5],
                              }]
                            }]
                          });

#@ Parameter option definition, duplicates are not allowed
shell.add_extension_object_member(obj, "function", f5,
                          {
                            "parameters":[
                            {
                              "name": "sample",
                              "type": "dictionary",
                              "options": [
                              {
                                "name": "myOption",
                                "type": "string",
                                "required": False,
                              },
                              {
                                "name": "myoption",
                                "type": "string",
                                "required": False,
                              }]
                            }]
                          });

#@ Parameter option definition, unknown attributes are not allowed
shell.add_extension_object_member(obj, "function", f5,
                          {
                            "parameters":[
                            {
                              "name": "sample",
                              "type": "dictionary",
                              "options": [
                              {
                                "name": "myOption",
                                "type": "string",
                                "required": False,
                              },
                              {
                                "name": "myoption",
                                "type": "string",
                                "other": False,
                              }]
                            }]
                          });


#@ Adding duplicate member
shell.add_extension_object_member(obj, "sample", 1);
shell.add_extension_object_member(obj, "sample", "other");

#@ Adding self as member
shell.add_extension_object_member(obj, "self", obj);

#@ Adding object that was already added
obj1 = shell.create_extension_object()
obj2 = shell.create_extension_object()
obj3 = shell.create_extension_object()
shell.add_extension_object_member(obj1, "anObject", obj3);
shell.add_extension_object_member(obj2, "anObject", obj3);

###############################################################################
#                            Errors Registering Global
###############################################################################
#@ Registering global, missing arguments
shell.register_global();
shell.register_global("someName");

#@ Registering global, invalid data for name parameter
shell.register_global(5, 5);

#@ Registering global, invalid name parameter
shell.register_global("bad name", obj);

#@ Registering global, invalid data for object parameter, not an object
shell.register_global("goodName", "notAnObject");

#@ Registering global, invalid data for object parameter, not an extension object
shell.register_global("goodName", shell);

#@ Registering global, invalid data definition
shell.register_global("goodName", obj, 1);

#@ Registering global, invalid definition, brief should be string
shell.register_global("goodName", obj, {"brief": 5});

#@ Registering global, invalid definition, details should be array
shell.register_global("goodName", obj, {"details": 5});

#@ Registering global, invalid definition, details should be array of strings
shell.register_global("goodName", obj, {"details": ["one", 5]});

#@ Registering global, invalid definition, other attributes not accepted
shell.register_global("goodName", obj, {"brief": "brief description", "other": 5});

#@<> Registering global, no definition
shell.register_global("goodName", obj);
\?

#@ Registering global using existing global names
shell.connect(__mysql_uri + "/mysql")
other = shell.create_extension_object();
for name in ["shell", "dba", "util", "mysql", "mysqlx", "session", "db", "sys", "os", "goodName"]:
  try:
    shell.register_global(name, other);
  except Exception as err:
    print(err)

#@ Adding object that was already registered
obj1 = shell.create_extension_object()
obj2 = shell.create_extension_object()
shell.register_global("sampleObject", obj2)
shell.add_extension_object_member(obj1, "anObject", obj2);

#@ Registering object that was already registered
shell.register_global("anotherObject", obj2)


###############################################################################
#                        Errors Using Unregistered Object
###############################################################################
obj = shell.create_extension_object()
shell.add_extension_object_member(obj, "myProperty", 5);
def hw():
  print("Hello World!")

shell.add_extension_object_member(obj, "mySampleFunction", hw);

#@ Attempt to get property of unregistered
obj.my_property

#@ Attempt to set property of unregistered
obj.my_property = 7

#@ Attempt to call function of unregistered
obj.my_sample_function()

shell.register_global("myRegistered", obj)

#@ Attempt to get property of registered
obj.my_property

#@ Attempt to set property of registered
obj.my_property = 7
obj.my_property

#@ Attempt to call function of registered
obj.my_sample_function()
