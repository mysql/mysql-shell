#@ Original globals in help
\?


#@ Registering a new global object
obj = shell.create_extension_object()
shell.register_global("myGlobal", obj, {"brief": "User defined global object.",
                                        "details": ["This object is used to verify how each element help is registered on the go.",
                                                    "The reason for this is because this object was registered as global right after it's creation"]
                                        })

# New global is shown in help
\?

#@ Now we can retrieve the object's help
\? myGlobal

#@<> Adding a member
shell.add_extension_object_member(obj, "myInteger", 5, {"brief": "Simple description for integer member.",
                                              "details": ["The member myInteger should only be used for obscure purposes."]
                                              })

#@ Now we can ask about member help
\? my_integer

#@ The member data is added to the object help
\? myGlobal

#@<> Adding members of the different types
shell.add_extension_object_member(obj, "myBoolean", True, {"brief": "Simple description for boolean member."})
shell.add_extension_object_member(obj, "myFloat", 3.1416, {"brief": "Simple description for float member."})
shell.add_extension_object_member(obj, "myNull", None, {"brief": "Simple description for none member."})
shell.add_extension_object_member(obj, "myString", "Hello World!", {"brief": "Simple description for string member."})
shell.add_extension_object_member(obj, "myArray", [1,2,"sample"], {"brief": "Simple description for array member."})
shell.add_extension_object_member(obj, "myDictionary", {"one": 1, "two":2}, {"brief": "Simple description for dictionary member."})
obj_child = shell.create_extension_object()
shell.add_extension_object_member(obj_child, "myChildBoolean", True, {"brief": "Simple description for boolean member."})
shell.add_extension_object_member(obj_child, "myChildFloat", 3.1416, {"brief": "Simple description for float member."})
shell.add_extension_object_member(obj_child, "myChildNull", None, {"brief": "Simple description for none member."})
shell.add_extension_object_member(obj_child, "myChildString", "Hello World!", {"brief": "Simple description for string member."})
shell.add_extension_object_member(obj_child, "myChildArray", [1,2,"sample"], {"brief": "Simple description for array member."})
shell.add_extension_object_member(obj_child, "myChildDictionary", {"one": 1, "two":2}, {"brief": "Simple description for dictionary member."})
shell.add_extension_object_member(obj, "myChildObject", obj_child, {"brief": "Child object with it's own definitions."})

#@ The member data is updated with all the members
\? myGlobal

#@ The child object is available as well
\? my_child_object

def my_native_function(one, two, three, four):
  pass

#@<> Adding a function
shell.add_extension_object_member(obj, "myFunction", my_native_function, {
  "brief": "Simple description for a function.",
  "details": ["This function is to test how the help gets properly registered."],
  "parameters": [
    {
      "name": "one",
      "type": "string",
      "values": ["1", "2", "3"],
      "brief": "Exersices the string parameter usage",
      "details": ["This parameter must be a string.", "With one of the supported values."]
      },
    {
      "name": "two",
      "type": "integer",
      "required": True,
      "brief": "Exersices the integer parameter usage",
      "details": ["Will not provide additional details about this parameter."]
      },
    {
      "name": "three",
      "type": "float",
      "required": False,
      "brief": "Exersices the float parameter usage"
      },
    {
      "name": "four",
      "type": "bool",
      "required": False,
      "brief": "Exersices the boolean parameter usage"
      }]})

#@ The function help is available
\? my_function

#@ The function is added to the object help
\? myGlobal

#@<> Adding another function
def my_other_native_function(one, two, three, four):
  pass

shell.add_extension_object_member(obj, "mySecondFunction", my_native_function, {
  "brief": "Simple description for a function.",
  "details": ["This function is to test how the help gets properly registered."],
  "parameters": [
    {
      "name": "one",
      "type": "array",
      "brief": "Exersices the array parameter usage",
      "details": ["This parameter must be an array."]
      },
    {
      "name": "two",
      "type": "object",
      "class": "Session",
      "brief": "A session object for DB work execution."
      },
    {
      "name": "data",
      "type": "dictionary",
      "brief": "A sample dictionary with options for the function",
      "required": False,
      "options": [
        {
          "name": "myOption",
          "type": "bool",
          "brief": "This option will be used show how options look like at the function help"
          }
        ]
      }]})

#@ The function help is available on the help system
\? my_second_function

#@ The function is added to the object help again
\? myGlobal


#@<> Second object with identical data, but delayed registration
obj2 = shell.create_extension_object()
shell.add_extension_object_member(obj2, "myInteger", 5, {"brief": "Simple description for integer member.",
                                              "details": ["The member myInteger should only be used for obscure purposes."]
                                              })

shell.add_extension_object_member(obj2, "myBoolean", True, {"brief": "Simple description for boolean member."})
shell.add_extension_object_member(obj2, "myFloat", 3.1416, {"brief": "Simple description for float member."})
shell.add_extension_object_member(obj2, "myNull", None, {"brief": "Simple description for none member."})
shell.add_extension_object_member(obj2, "myString", "Hello World!", {"brief": "Simple description for string member."})
shell.add_extension_object_member(obj2, "myArray", [1,2,"sample"], {"brief": "Simple description for array member."})
shell.add_extension_object_member(obj2, "myDictionary", {"one": 1, "two":2}, {"brief": "Simple description for dictionary member."})

obj2_child = shell.create_extension_object()
shell.add_extension_object_member(obj2_child, "myChildBoolean", True, {"brief": "Simple description for boolean member."})
shell.add_extension_object_member(obj2_child, "myChildFloat", 3.1416, {"brief": "Simple description for float member."})
shell.add_extension_object_member(obj2_child, "myChildNull", None, {"brief": "Simple description for none member."})
shell.add_extension_object_member(obj2_child, "myChildString", "Hello World!", {"brief": "Simple description for string member."})
shell.add_extension_object_member(obj2_child, "myChildArray", [1,2,"sample"], {"brief": "Simple description for array member."})
shell.add_extension_object_member(obj2_child, "myChildDictionary", {"one": 1, "two":2}, {"brief": "Simple description for dictionary member."})

shell.add_extension_object_member(obj2, "myChildObject", obj2_child, {"brief": "Child object with it's own definitions."})

shell.add_extension_object_member(obj2, "myFunction", my_native_function, {
  "brief": "Simple description for a function.",
  "details": ["This function is to test how the help gets properly registered."],
  "parameters": [
    {
      "name": "one",
      "type": "string",
      "values": ["1", "2", "3"],
      "brief": "Exersices the string parameter usage",
      "details": ["This parameter must be a string.", "With one of the supported values."]
      },
    {
      "name": "two",
      "type": "integer",
      "required": True,
      "brief": "Exersices the integer parameter usage",
      "details": ["Will not provide additional details about this parameter."]
      },
    {
      "name": "three",
      "type": "float",
      "required": False,
      "brief": "Exersices the float parameter usage"
      },
    {
      "name": "four",
      "type": "bool",
      "required": False,
      "brief": "Exersices the boolean parameter usage"
      }]})

shell.add_extension_object_member(obj2, "mySecondFunction", my_native_function, {
  "brief": "Simple description for a function.",
  "details": ["This function is to test how the help gets properly registered."],
  "parameters": [
    {
      "name": "one",
      "type": "array",
      "brief": "Exersices the array parameter usage",
      "details": ["This parameter must be an array."]
      },
    {
      "name": "two",
      "type": "object",
      "class": "Session",
      "brief": "A session object for DB work execution."
      },
    {
      "name": "data",
      "type": "dictionary",
      "brief": "A sample dictionary with options for the function",
      "required": False,
      "options": [
        {
          "name": "myOption",
          "type": "bool",
          "brief": "This option will be used show how options look like at the function help"
          }
        ]
      }]})

shell.register_global("mySecondGlobal", obj2, {"brief": "User defined global object.",
                                        "details": ["This object is used to verify how each element help is registered on the go.",
                                                    "The reason for this is because this object was registered as global right after it's creation"]
                                        })

#@ Request the second object help [USE:The function is added to the object help again]
\? mySecondGlobal


#@ Finally the last global help
\?
