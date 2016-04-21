function validate_crud_functions(crud, expected)
{
    var actual = dir(crud);
    
    // Ensures expected functions are on the actual list
    var missing = [];
    for(exp_funct of expected){
        var pos = actual.indexOf(exp_funct);
        if(pos == -1){
            missing.push(exp_funct);
        }
        else{
            actual.splice(pos, 1);
        }
    }
    
    if(missing.length == 0){
        print ("All expected functions are available\n");
    }
    else{
        print("Missing Functions:", missing);
    }
    
    if (actual.length == 0){
        print("No additional functions are available\n")
    }
    else{
        print("Extra Functions:", actual);
    }
}

function ensure_schema_does_not_exist(session, name){
	try{
		var schema = session.getSchema(name);
		session.dropSchema(name);
	}
	catch(err){
		// Nothing happens, it means the schema did not exist
	}
}

function validateMember(memberList, member){
	if (memberList.indexOf(member) != -1){
		print(member + ": OK\n");
	}
	else{
		print(member + ": Missing\n");
	}
}

function validateNotMember(memberList, member){
	if (memberList.indexOf(member) != -1){
		print(member + ": Unexpected\n");
	}
	else{
		print(member + ": OK\n");
	}
}

function getSchemaFromList(schemas, name){
  for(index in schemas){
    if(schemas[index].name == name)
      return schemas[index];
  }
  
  return null;
}
