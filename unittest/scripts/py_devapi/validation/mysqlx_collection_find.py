#@ CollectionFind: valid operations after find
|All expected functions are available|
|No additional functions are available|

#@ CollectionFind: valid operations after fields
|All expected functions are available|
|No additional functions are available|

#@ CollectionFind: valid operations after group_by
|All expected functions are available|
|No additional functions are available|

#@ CollectionFind: valid operations after having
|All expected functions are available|
|No additional functions are available|

#@ CollectionFind: valid operations after sort
|All expected functions are available|
|No additional functions are available|

#@ CollectionFind: valid operations after limit
|All expected functions are available|
|No additional functions are available|

#@ CollectionFind: valid operations after offset
|All expected functions are available|
|No additional functions are available|

#@ CollectionFind: valid operations after skip
|All expected functions are available|
|No additional functions are available|

#@ CollectionFind: valid operations after lock_shared
|All expected functions are available|
|No additional functions are available|

#@ CollectionFind: valid operations after lock_exclusive
|All expected functions are available|
|No additional functions are available|

#@ CollectionFind: valid operations after bind
|All expected functions are available|
|No additional functions are available|

#@ CollectionFind: valid operations after execute
|All expected functions are available|
|No additional functions are available|

#@ CollectionFind: valid operations after execute with limit
|All expected functions are available|
|No additional functions are available|

#@ Reusing CRUD with binding
|adam|
|alma|


#@# CollectionFind: Error conditions on find
||CollectionFind.find: Argument #1 is expected to be a string
||CollectionFind.find: Unterminated quoted string starting at position 8

#@# CollectionFind: Error conditions on fields
||CollectionFind.fields: Invalid number of arguments, expected at least 1 but got 0
||CollectionFind.fields: Argument #1 is expected to be a string, array of strings or a JSON expression
||CollectionFind.fields: Field selection criteria can not be empty
||CollectionFind.fields: Element #2 is expected to be a string
||CollectionFind.fields: Argument #1 is expected to be a JSON expression
||CollectionFind.fields: Argument #2 is expected to be a string

#@# CollectionFind: Error conditions on group_by
||CollectionFind.group_by: Invalid number of arguments, expected at least 1 but got 0
||CollectionFind.group_by: Argument #1 is expected to be a string or an array of strings
||CollectionFind.group_by: Grouping criteria can not be empty
||CollectionFind.group_by: Element #2 is expected to be a string
||CollectionFind.group_by: Argument #2 is expected to be a string

#@# CollectionFind: Error conditions on having
||CollectionFind.having: Invalid number of arguments, expected 1 but got 0
||CollectionFind.having: Argument #1 is expected to be a string

#@# CollectionFind: Error conditions on sort
||CollectionFind.sort: Invalid number of arguments, expected at least 1 but got 0
||CollectionFind.sort: Argument #1 is expected to be a string or an array of strings
||CollectionFind.sort: Sort criteria can not be empty
||CollectionFind.sort: Element #2 is expected to be a string
||CollectionFind.sort: Argument #2 is expected to be a string

#@# CollectionFind: Error conditions on limit
||CollectionFind.limit: Invalid number of arguments, expected 1 but got 0
||CollectionFind.limit: Argument #1 is expected to be an unsigned int

#@# CollectionFind: Error conditions on offset
||CollectionFind.offset: Invalid number of arguments, expected 1 but got 0
||CollectionFind.offset: Argument #1 is expected to be an unsigned int

#@# CollectionFind: Error conditions on skip
||CollectionFind.skip: Invalid number of arguments, expected 1 but got 0
||CollectionFind.skip: Argument #1 is expected to be an unsigned int

#@# CollectionFind: Error conditions on lock_shared
||CollectionFind.lock_shared: Invalid number of arguments, expected 0 to 1 but got 2
||CollectionFind.lock_shared: Argument #1 is expected to be one of DEFAULT, NOWAIT or SKIP_LOCKED

#@# CollectionFind: Error conditions on lock_exclusive
||CollectionFind.lock_exclusive: Invalid number of arguments, expected 0 to 1 but got 2
||CollectionFind.lock_exclusive: Argument #1 is expected to be one of DEFAULT, NOWAIT or SKIP_LOCKED

#@# CollectionFind: Error conditions on bind
||CollectionFind.bind: Invalid number of arguments, expected 2 but got 0
||CollectionFind.bind: Argument #1 is expected to be a string
||CollectionFind.bind: Unable to bind value for unexisting placeholder: another

#@# CollectionFind: Error conditions on execute
||CollectionFind.execute: Missing value bindings for the following placeholders: data, years
||CollectionFind.execute: Missing value bindings for the following placeholders: data



#@ Collection.Find All
|All: 7|

#@ Collection.Find Filtering
|Males: 4|
|Females: 3|
|13 Years: 1|
|14 Years: 3|
|Under 17: 6|
|Names With A: 3|

#@ Collection.Find Field Selection
|1-Metadata Length: 2|
|1-Metadata Field: name|
|1-Metadata Field: age|

|2-Metadata Length: 1|
|2-Metadata Field: age|

#@ Collection.Find Sorting
|Find Asc 0 : adam|
|Find Asc 1 : alma|
|Find Asc 2 : angel|
|Find Asc 3 : brian|
|Find Asc 4 : carol|
|Find Asc 5 : donna|
|Find Asc 6 : jack|
|Find Desc 0 : jack|
|Find Desc 1 : donna|
|Find Desc 2 : carol|
|Find Desc 3 : brian|
|Find Desc 4 : angel|
|Find Desc 5 : alma|
|Find Desc 6 : adam|

#@ Collection.Find Limit and Offset
|Limit-Offset 0 : 4|
|Limit-Offset 1 : 4|
|Limit-Offset 2 : 4|
|Limit-Offset 3 : 4|
|Limit-Offset 4 : 3|
|Limit-Offset 5 : 2|
|Limit-Offset 6 : 1|
|Limit-Offset 7 : 0|


#@ Collection.Find Parameter Binding
|Find Binding Length: 1|
|Find Binding Name: alma|

#@ Collection.Find Field Selection Using Field List
|First Name: JACK|
|Age: 17|

#@ Collection.Find Field Selection Using Field Parameters
|First Name: JACK|
|Age: 17|

#@ Collection.Find Field Selection Using Projection Expression
|First Name: JACK|
|In Three Years: 20|

#@<OUT> WL12813 Collection Test 01
{
    "_id": "4C514FF38144B714E7119BCF48B4CA01",
    "like": "foo",
    "nested": {
        "like": "bar"
    }
}
{
    "_id": "4C514FF38144B714E7119BCF48B4CA02",
    "like": "foo",
    "nested": {
        "like": "nested bar"
    }
}
2 documents in set [[*]]

#@<OUT> WL12813 Collection Test 03
{
    "_id": "4C514FF38144B714E7119BCF48B4CA05",
    "like": "bar",
    "nested": {
        "like": "foo"
    }
}
{
    "_id": "4C514FF38144B714E7119BCF48B4CA07",
    "like": "top bar",
    "nested": {
        "like": "foo"
    }
}
2 documents in set [[*]]

#@<OUT> WL12813 Collection Test 04
{
    "_id": "4C514FF38144B714E7119BCF48B4CA05",
    "like": "bar",
    "nested": {
        "like": "foo"
    }
}
{
    "_id": "4C514FF38144B714E7119BCF48B4CA06",
    "like": "bar",
    "nested": {
        "like": "nested foo"
    }
}
{
    "_id": "4C514FF38144B714E7119BCF48B4CA07",
    "like": "top bar",
    "nested": {
        "like": "foo"
    }
}
{
    "_id": "4C514FF38144B714E7119BCF48B4CA08",
    "like": "top bar",
    "nested": {
        "like": "nested foo"
    }
}
4 documents in set [[*]]

#@<OUT> WL12813 Collection Test 06
{
    "_id": "4C514FF38144B714E7119BCF48B4CA01",
    "like": "foo",
    "nested": {
        "like": "bar"
    }
}
{
    "_id": "4C514FF38144B714E7119BCF48B4CA02",
    "like": "foo",
    "nested": {
        "like": "nested bar"
    }
}
{
    "_id": "4C514FF38144B714E7119BCF48B4CA03",
    "like": "top foo",
    "nested": {
        "like": "bar"
    }
}
{
    "_id": "4C514FF38144B714E7119BCF48B4CA04",
    "like": "top foo",
    "nested": {
        "like": "nested bar"
    }
}
4 documents in set [[*]]
