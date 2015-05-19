/*
 * Copyright (c) 2015, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

exports.mysqlx = {}
 
exports.mysqlx.openNodeSession = function(connection_data)
{
  // TODO: Add logic to enable handling the different formats of the connection data
  return new NodeSession(connection_data);
}

exports.mysqlx.openSession = function(connection_data)
{
  // TODO: Add logic to enable handling the different formats of the connection data
  return new Session(connection_data);
}
 
//--- Dev-API: Session Object
function BaseSession(connection_data)
{
  // Connection represents the protocol interface
  // Must be completely hidden from the outside
  var _conn = _F.mysqlx.Connection(connection_data);

  // Privileged function to enable accessing the connection
  // to perform operations through it
  function _exec_sql(sql)
  {
    return _conn.sql(sql);
  }
  
  Object.defineProperty(this, '_schemas', {value: {}});
  Object.defineProperty(this, '_loaded', {value: false, writable:true});
  Object.defineProperty(this, '_query_data', 
  {
    value: function(key, param1, param2)
    {
      switch(key)
      {
        // Used to load all the schemas on the database
        case 'default_schema':
          return _exec_sql('select schema()');
        case 'schema':
          return _exec_sql('show databases like "'+ param1 +'"');
        case 'schemas':
          return _exec_sql('show databases');
        case 'tables':
          if (typeof param1 === 'undefined')
            throw 'Query data request ' + key + ' is missing a parameter'
          return _exec_sql('show full tables in `' + param1 + '`')
        case 'table':
          if (typeof param1 === 'undefined' || typeof param2 === 'undefined' )
            throw 'Query data request ' + key + ' is missing a parameter'
          return _exec_sql('show full tables in `' + param1 + '` like "'+ param2 +'"')
        default:
          // Using this hack to only allow arbitrary sql execution from inside
          // an instance of NodeSession, this is needed because:
          // - _exec_sql is only visible at this scope.
          // - Have not found a way to define it as a protected member.
          // - Even _query_data is hidden, it's accessible from outside.
          //
          // TODO: Find a way to define protected members so we can remove the
          //       usage of NodeSession at this level.
          if (this instanceof NodeSession)
            return _exec_sql(key);
          else
            throw "Invalid query data operation: " + key;
      }
      
      return null;
    }
  });
  
  function _instantiate_schema(session, name, in_db, verify)
  {
    return new Schema(session, name, _conn, in_db, verify);
  }

  Object.defineProperty(this, '_new_schema', 
  {
    value: function(name, in_db, verify)
    {
      return _instantiate_schema(this, name, in_db, verify);
    }
  });
}

BaseSession.prototype.getSchemas = function()
{
  if (!this._loaded)
  {
    var res = this._query_data('schemas');
    
    if (res)
    {
      var data = res.fetch_all(true);
      
      // Creates a new schema object if not exists already
      for(index in data)
      {
        var name = data[index][0];
        
        if ( typeof this._schemas[name] === "undefined")
        {
          var schema = this._new_schema(name, true, false);
          
          // Stores the schema on the registry
          this._schemas[name] = schema;
          
          // As well as session property if does not exist already
          if (typeof this[name] === 'undefined')
            this[name] = schema;
        }
      }
      
      this._loaded = true;
    }
  }
  
  return this._schemas;
}

Object.defineProperty(BaseSession.prototype, 'schemas', 
{
  enumerable: true,
  get: BaseSession.prototype.getSchemas
})

BaseSession.prototype.toString = function()
{
  return "BaseSession";
}

BaseSession.prototype.getSchema = function(name)
{
  if ( typeof this._schemas[name] === "undefined")
  {
    var schema = this._new_schema(name, true, true);
    
    if (typeof schema != "undefined")
    {
      // Adds it to the schema registry
      this._schemas[name] = schema;
      
      // As well as session property if does not exist already
      if (typeof this[name] === 'undefined')
        this[name] = schema;
    }
  }
  
  return this._schemas[name]
}

BaseSession.prototype.getDefaultSchema = function()
{
  var schema = null;
  var res = this._query_data('default_schema');
  var data = res.fetch_one(true);
  
  if (data[0] != null)
    schema = this.getSchema(data[0])
  
  return schema;
}

Object.defineProperty(BaseSession.prototype, 'default_schema', 
{
  enumerable: true,
  get: BaseSession.prototype.getDefaultSchema
})

//--- Dev-API: Session
function Session(cd)
{
  BaseSession.call(this, cd);
}

Session.prototype = Object.create(BaseSession.prototype);
Object.defineProperty(NodeSession.prototype, 'constructor', {value: Session});

exports.mysqlx.Session = Session;

//--- Dev-API: NodeSession
function NodeSession(cd)
{
  BaseSession.call(this, cd);

  this.executeSql = function(sql) 
  {
    return this._query_data(sql);
  }
}

NodeSession.prototype = Object.create(BaseSession.prototype);
Object.defineProperty(NodeSession.prototype, 'constructor', {value: NodeSession});

exports.mysqlx.NodeSession = NodeSession;

//-------- Dev-API Database Objects
function DatabaseObject(session, schema, name, in_db)
{
  Object.defineProperty(this, 'session',  {value: session, enumerable: true});
  Object.defineProperty(this, 'schema',  {value: schema, enumerable: true});
  Object.defineProperty(this, 'name',  {value: name, enumerable: true});
  Object.defineProperty(this, 'in_database',  {value: in_db, enumerable: true});
}

DatabaseObject.prototype.getSession = function()
{
  return this.session;
}

DatabaseObject.prototype.getSchema = function()
{
  return this.schema;
}

DatabaseObject.prototype.getName = function()
{
  return this.name;
}

DatabaseObject.prototype.existsInDatabase = function()
{
  return this.in_database;
}

//--- Schema Object
function Schema(session, name, connection, in_db, verify)
{
  var _conn = connection;
  
  Object.defineProperty(this, '_tables', {value: {}});
  Object.defineProperty(this, '_collections', {value: {}});
  Object.defineProperty(this, '_views', {value: {}});
  Object.defineProperty(this, '_loaded', {value: false, writable: true});

  function _instantiate_table(schema, name, in_db, verify)
  {
    return new Table(schema, name, _conn, in_db, verify);
  }
  
  function _instantiate_view(schema, name, in_db, verify)
  {
    return new View(schema, name, _conn, in_db, verify);
  }
  
  function _instantiate_collection(schema, name, in_db, verify)
  {
    return new Collection(schema, name, _conn, in_db, verify);
  }
  
  Object.defineProperty(this, '_new_table', 
  {
    value: function(name, in_db, verify)
    {
      return _instantiate_table(this, name, in_db, verify);
    }
  });
  
  Object.defineProperty(this, '_new_view', 
  {
    value: function(name, in_db, verify)
    {
      return _instantiate_view(this, name, in_db, verify);
    }
  });
  
  Object.defineProperty(this, '_new_collection', 
  {
    value: function(name, in_db, verify)
    {
      return _instantiate_collection(this, name, in_db, verify);
    }
  });
  
  DatabaseObject.call(this, session, this, name, in_db)
  
  if (typeof verify !== "undefined" && verify)
    this._verify()
}

Schema.prototype = Object.create(DatabaseObject.prototype);
Object.defineProperty(Schema.prototype, 'constructor', {value: Schema});
Object.defineProperty(Schema.prototype, '_verify', 
{
  value: function()
  {
    var is_valid = false;
    var res = this.session._query_data('schema', this.name);
    
    if (res)
    {
      var data = res.fetch_all(true);

      if (data.length == 1 && data[0][0] == this.name)
        is_valid = true;
    }
    
    if (!is_valid)
    {
      var error = "Undefined schema " + this.name;
      throw (error)
    }
  }
});

Object.defineProperty(Schema.prototype, '_loadObjects', 
{
  value: function()
  {
    var res = this.session._query_data('tables', this.name);
    
    if (res)
    {
      var data = res.fetch_all(true);
      
      // Creates a new table/view/collection object if not exists already
      for(index in data)
      {
        var type = data[index][1];
        var name = data[index][0];
        var object;
        
        if (type == "BASE TABLE"){
          if (typeof this._tables[name] === "undefined")
          {
            object = this._new_table(name, true);
            this._tables[name] = object;
          }
        }
        
        // TODO: Add else if for the collections
        else
        {
          if (typeof this._views[name] === "undefined")
          {
            object = this._new_view(name, true);
            this._views[name] = object;
          }
        }
        
        // Creates the schema atribute if not already exists
        if (typeof object !== 'undefined' && typeof this[name] === 'undefined')
          this[name] = object;
      }
    }
    
    this._loaded = true;
  },
  enumerable: false
});

Schema.prototype.getCollections = function()
{
  if (!this._loaded)
    this._loadObjects();
  
  return this._collections;
}

Object.defineProperty(Schema.prototype, 'collections', 
{
  enumerable: true,
  get: Schema.prototype.getCollections
})


Schema.prototype.getCollection = function(name)
{
  if ( typeof this._collections[name] === "undefined")
  {
    var collection = new Collection(this, name, true, true)
    
    if (typeof collection !== 'undefined')
    {
      this._collections[name] = collection;
      
      if (typeof this[name] === 'undefined')
        this[name] = collection;
    }
  }
  
  return this._collections[name]
}


Schema.prototype.getTables = function()
{
  if (!this._loaded)
    this._loadObjects();
  
  return this._tables;
}

Object.defineProperty(Schema.prototype, 'tables', 
{
  enumerable: true,
  get: Schema.prototype.getTables
})


Schema.prototype.getTable = function(name)
{
  if ( typeof this._tables[name] === "undefined")
  {
    var table = this._new_table(name, true, true);
    
    if (typeof table !== 'undefined')
    {
      this._tables[name] = table;
      
      if (typeof this[name] === 'undefined')
        this[name] = table;
    }
  }
  
  return this._tables[name]
}

Schema.prototype.getViews = function()
{
  if (!this._loaded)
    this._loadObjects();
  
  return this._views;
}

Object.defineProperty(Schema.prototype, 'views', 
{
  enumerable: true,
  get: Schema.prototype.getTables
})

Schema.prototype.getView = function(name)
{
  if ( typeof this._views[name] === "undefined")
  {
    var view = new View(this, name, true, true)
    
    if (typeof view !== 'undefined')
    {
      this._views[name] = view;
      
      if (typeof this[name] === 'undefined')
        this[name] = view;
    }
  }
  
  return this._views[name]
}

exports.mysqlx.Schema = Schema;

//--- Table Object
function Table(schema, name, connection, in_db, verify)
{
  var _conn = connection;
  
  DatabaseObject.call(this, schema.getSession(), schema, name, in_db);
  
  if (typeof verify === "boolean" && verify)
    this._verify();
  
  function _instantiate_insert(schema, table)
  {
    return _F.mysqlx.TableInsert(_conn, schema, table);
  }
  
  Object.defineProperty(this, '_new_crud', 
  {
    value: function(type)
    {
      switch(type)
      {
        case 'insert':
          return _instantiate_insert(this.schema.name, this.name);
          break;
      }
    }
  });
}

Table.prototype = Object.create(DatabaseObject.prototype);
Object.defineProperty(Table.prototype, 'constructor', {value: Table});
Object.defineProperty(Table.prototype, '_verify', 
{
  value: function()
  {
    var is_valid = false;
    var res = this.session._query_data('table', this.schema.name, this.name);
    
    if (res)
    {
      var data = res.fetch_all(true);

      if (data.length == 1 && data[0][1] == "BASE TABLE")
        is_valid = true;
    }
    
    if (!is_valid)
    {
      var error = "Undefined table " + this.name + " on schema " + this.schema.name;
      throw (error)
    }
  }
});

Table.prototype.insert = function(data)
{
  var insert = this._new_crud('insert');
  return insert.insert(data);
}

exports.mysqlx.Table = Table;


//--- Collection Object
function Collection(schema, name, in_db, verify)
{
  DatabaseObject.call(this, schema.getSession(), schema, name, in_db);
  
  if (typeof verify === "boolean" && verify)
    this._verify()
}

Collection.prototype = Object.create(DatabaseObject.prototype);
Object.defineProperty(Collection.prototype, 'constructor', {value: Collection});

Collection.prototype._verify = function()
{
  var is_valid = false;
  var res = this.session._query_data('table', this.schema.name, this.name);
  
  if (res)
  {
    var data = res.fetch_all(true);
    
    //TODO: Define the proper type
    if (data.length == 1 && data[0][1] == "COLLECTION")
      is_valid = true;
  }
  
  if (!is_valid)
  {
    var error = "Undefined collection " + this.name + " on schema " + this.schema.name;
    throw (error);
  }
}

exports.mysqlx.Collection = Collection;


//--- View Object
function View(schema, name, in_db)
{
  DatabaseObject.call(this, schema.getSession(), schema, name, in_db);
}

View.prototype = Object.create(DatabaseObject.prototype);
Object.defineProperty(View.prototype, 'constructor', {value: View});

exports.mysqlx.View = View;
