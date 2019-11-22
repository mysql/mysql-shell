# Assumptions: test_validation of collection requires MySQL 8.0.19
# Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
from __future__ import print_function
from mysqlsh import mysqlx

mySession = mysqlx.get_session(__uripwd)

ensure_schema_does_not_exist(mySession, 'py_shell_test')

schema = mySession.create_schema('py_shell_test')

#@ create collection with wrong option
coll = schema.create_collection("longlang", {
  "vals" : "whatever"
});

#@ create collection with wrong validation type
coll = schema.create_collection("longlang", {
  "validation" : "longitude"
});

#@ create collection with malformed validation object {VER(>= 8.0.19)}
coll = schema.create_collection('test', {'validation':{'whatever':{}}});

#@ create collection with incorrect validation schema {VER(>= 8.0.19)}
schema.create_collection("test", {'validation':{'schema':{'name':'some'}}})

#@ create collection unsupported server {VER(< 8.0.19)}
coll = schema.create_collection("longlang", {
  "validation" : {
    "level" : "strict",
    "schema" : {
      "id" : "http://json-schema.org/geo",
      "$schema" : "http://json-schema.org/draft-06/schema#",
      "description" : "A geographical coordinate",
      "type" : "object",
      "properties" : {
        "latitude" : {
          "type" : "number"
        },
        "longitude" : {
          "type" : "number"
        }
      },
      "required" : [ "latitude", "longitude" ]
    }
  }
})

coll = schema.create_collection("longlang", {"reuseExistingObject": True});

#@ create collection with validation block {VER(>= 8.0.19)}

# first completely empty validation
coll = schema.create_collection("empty_validation", {"validation" : {}})

coll = schema.create_collection("longlang", {
  "validation" : {
    "level" : "strict",
    "schema" : {
      "id" : "http://json-schema.org/geo",
      "$schema" : "http://json-schema.org/draft-06/schema#",
      "description" : "A geographical coordinate",
      "type" : "object",
      "properties" : {
        "latitude" : {
          "type" : "number"
        },
        "longitude" : {
          "type" : "number"
        }
      },
      "required" : [ "latitude", "longitude" ]
    }
  }
})

coll.count();

c = schema.create_collection("col", {
  "validation" : {
    "level" : "strict",
    "schema" : {
      "type" : "object",
      "properties" : {
        "i" : {
          "type" : "number"
        }
      },
      "required" : [ "i" ]
    }
  }
})

c.count()

#@ validation negative case {VER(>= 8.0.19)}
c.add({"j": 12});
c.add({"i": "text"});

#@ validation positive case {VER(>= 8.0.19)}
c.add({"i": 12});
c.count();

#@ reuseExistingObject {VER(>= 8.0.19)}
c = schema.create_collection("col", {"reuseExistingObject": True})

# different validation ignored
c = schema.create_collection("col", {"reuseExistingObject": True, 
  "validation" : {
    "level" : "strict",
    "schema" : {
      "type" : "object",
      "properties" : {
        "j" : {
          "type" : "string"
        }
      },
      "required" : [ "j" ]
    }
  }
})

c.add({"j": "text"});


#@ modify collection with empty options
schema.modify_collection("longlang", {})

#@ modify collection with incorrect schema {VER(>= 8.0.19)}
schema.modify_collection("longlang", {'validation':{'schema':{'name':'some'}}})

#@ modify collection with incorrect validation object {VER(>= 8.0.19)}
schema.modify_collection("longlang", {'validation':{'whatever':'some'}});

#@ modify collection validation only schema {VER(>= 8.0.19)}
schema.modify_collection("col", {
  "validation" : {
    "schema" : {
      "type" : "object",
      "properties" : {
        "i" : {
          "type" : "number"
        },
        "j" : {
          "type" : "string"
        }
      },
      "required" : [ "i" ]
    }
  }
});

c.add({"i": 1, "j": "text"});
c.count();

#@ schema modification in effect {VER(>= 8.0.19)}
c.add({"i": 1, "j": 1});

#@ modify collection validation level only {VER(>= 8.0.19)}
schema.modify_collection("col", {
  "validation" : {
    "level" : "off"
  }
});

c.add({"i": "text", "j":1});
c.count();

#@ modify collection validation block fails because of data inconsistencies
schema.modify_collection("col", {
  "validation" : {
    "level": "strict",
    "schema" : {
      "type" : "object",
      "properties" : {
        "i" : {
          "type" : "number"
        },
        "j" : {
          "type" : "string"
        }
      },
      "required" : ["i"]
    }
  }
});

# Closes the session
mySession.drop_schema('py_shell_test')
mySession.close()
