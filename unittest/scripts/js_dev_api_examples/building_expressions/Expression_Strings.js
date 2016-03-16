
// Using a string expression to get all documents that
// have the name field starting with 'S'
var myDocs = myColl.find('name like :name').bind('name', 'S%').execute();
