// Assumptions: test_validation of collection requires MySQL 8.0.19
// Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
var mysqlx = require('mysqlx');

var mySession = mysqlx.getSession(__uripwd);

ensure_schema_does_not_exist(mySession, 'js_shell_test');

schema = mySession.createSchema('js_shell_test');

//@ create collection with wrong option
var coll = schema.createCollection("longlang", {
  vals : "whatever"
});

//@ create collection with wrong validation type
var coll = schema.createCollection("longlang", {
  validation : "longitude"
});

//@ create collection with malformed validation object {VER(>= 8.0.19)}
var coll = schema.createCollection('test', {'validation':{'whatever':{}}});

//@ create collection with incorrect validation schema {VER(>= 8.0.19)}
schema.createCollection("test", {'validation':{'schema':{'name':'some'}}})

//@ create collection unsupported server {VER(< 8.0.19)}
var coll = schema.createCollection("longlang", {
  validation : {
    level : "strict",
    schema : {
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

var coll = schema.createCollection("longlang", {"reuseExistingObject": true});

//@ create collection with validation block {VER(>= 8.0.19)}

// first completely empty validation
var empty_val = schema.createCollection("empty_validation", {"validation" : {}})

var coll = schema.createCollection("longlang", {
  validation : {
    level : "strict",
    schema : {
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

var c = schema.createCollection("col", {
  validation : {
    level : "strict",
    schema : {
      type : "object",
      properties : {
        i : {
          "type" : "number"
        }
      },
      required : [ "i" ]
    }
  }
})

c.count()

//@ validation negative case {VER(>= 8.0.19)}
c.add({j: 12});
c.add({i: "text"});

//@ validation positive case {VER(>= 8.0.19)}
c.add({i: 12});
c.count();

//@ reuseExistingObject {VER(>= 8.0.19)}
var c = schema.createCollection("col", {"reuseExistingObject": true});

// different validation ignored
var c = schema.createCollection("col", {"reuseExistingObject": true, 
	validation: {
		level:"off",
		schema : {
	      type : "object",
	      properties : {
	        j : {
	          "type" : "string"
	        }
	      },
	      required : [ "j" ]
	    }
}});

c.add({j: "text"});

//@ modify collection with empty options
schema.modifyCollection("longlang", {})

//@ modify collection with incorrect schema {VER(>= 8.0.19)}
schema.modifyCollection("longlang", {'validation':{'schema':{'name':'some'}}})

//@ modify collection with incorrect validation object {VER(>= 8.0.19)}
schema.modifyCollection("longlang", {'validation':{'whatever':'some'}});

//@ modify collection validation only schema {VER(>= 8.0.19)}
schema.modifyCollection("col", {
  validation : {
    schema : {
      type : "object",
      properties : {
        i : {
          "type" : "number"
        },
        j : {
          "type" : "string"
        }
      },
      required : [ "i" ]
    }
  }
});

c.add({i: 1, j: "text"});
c.count();

//@ schema modification in effect {VER(>= 8.0.19)}
c.add({i: 1, j: 1});

//@ modify collection validation level only {VER(>= 8.0.19)}
schema.modifyCollection("col", {
  validation : {
    level : "off"
  }
});

c.add({i: "text", j:1});
c.count();

//@ modify collection validation block fails because of data inconsistencies
schema.modifyCollection("col", {
  validation : {
    level: "strict",
    schema : {
      type : "object",
      properties : {
        i : {
          "type" : "number"
        },
        j : {
          "type" : "string"
        }
      },
      required : ["i"]
    }
  }
});

// Closes the session
mySession.dropSchema('js_shell_test');
mySession.close();