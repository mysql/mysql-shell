//@ Test Schema Creation
shell.connect(__uripwd);
session.dropSchema('resultset_dumper');
var schema = session.createSchema('resultset_dumper');
session.sql('create table resultset_dumper.bindata(data BINARY(5))');
session.sql('insert into resultset_dumper.bindata values ("ab\0cd"),("ab\tcd"),("ab\ncd")');

//---------- TEST FOR BINARY DATA ----------
//@ X Table Format
shell.options.resultFormat = 'table';
session.sql('select * from resultset_dumper.bindata');

//@ X Vertical Format
shell.options.resultFormat = 'vertical';
session.sql('select * from resultset_dumper.bindata');

//@ X Tabbed Format
shell.options.resultFormat = 'tabbed';
session.sql('select * from resultset_dumper.bindata');

//@ X Json Format
shell.options.resultFormat = 'json';
var result = session.sql('select * from resultset_dumper.bindata').execute();
result
shell.options.resultFormat = 'table';
session.close();

//@ Classic Table Format
shell.connect(__mysqluripwd);
shell.options.resultFormat = 'table';
session.runSql('select * from resultset_dumper.bindata');

//@ Classic Vertical Format
shell.options.resultFormat = 'vertical';
session.runSql('select * from resultset_dumper.bindata');

//@ Classic Tabbed Format
shell.options.resultFormat = 'tabbed';
session.runSql('select * from resultset_dumper.bindata');

//@ Classic Json Format
shell.options.resultFormat = 'json';
var result = session.runSql('select * from resultset_dumper.bindata');
result
shell.options.resultFormat = 'table';
session.runSql('drop schema resultset_dumper');
session.close();
//------------------------------------------


//@<>---------- TEST FOR MULTIBYTE CHARACTER FORMATTING ---------- {!__os_type != 'windows'}
shell.connect(__uripwd);
var schema = session.createSchema('resultset_dumper');
session.sql('create table resultset_dumper.mbtable(data VARCHAR(100))');
var table = schema.getTable('mbtable');
table.insert('data').
      values('生活是美好的\0生活是美好的').
      values('辛德勒的名单\0辛德勒的名单').
      values('指環王\0指環王').
      values('尋找尼莫\0尋找尼莫').
      values('😁😍😠\0😭🙅🙉').
      values('✅✨✋\0✈❄❔➗').
      values('🚀🚑\0🚙🚬🚻🛀').
      values('🇯🇵🈳🆕🆒').
      values('®7⃣⏰☕♒♣\0⛽🌄🌠🎨🐍🐾');

//@ table in table format {!__os_type != 'windows'}
shell.options.resultFormat = 'table';
table.select();

//@ table in tabbed format {!__os_type != 'windows'}
shell.options.resultFormat="tabbed";
table.select();

//@ table in vertical format {!__os_type != 'windows'}
shell.options.resultFormat="vertical";
table.select();

var collection = schema.createCollection('mbcollection');
collection.add({_id:'1', name:'生活是美好的', year: 1997});
collection.add({_id:'2', name:'辛德勒的名单', year: 1993});
collection.add({_id:'3', name:'指環王', year: 2001});
collection.add({_id:'4', name:'尋找尼莫', year: 2003});
collection.add({_id:'5', name:'الجنة الآن', year: 2003});
collection.add({_id:'6', name:'😁😍😠😭🙅🙉', year: 2004});
collection.add({_id:'7', name:'✅✨✋✈❄❔➗', year: 2004});
collection.add({_id:'8', name:'🚀🚑🚙🚬🚻🛀', year: 2004});
collection.add({_id:'9', name:'🇯🇵🈳🆕🆒', year: 2004});
collection.add({_id:'10', name:'®7⃣⏰☕♒♣⛽🌄🌠🎨🐍🐾', year: 2004});

//@ Pulling as collection in JSON format {!__os_type != 'windows'}
collection.find();

//@ pulling as table in table format {!__os_type != 'windows'}
shell.options.resultFormat="table";
var table = schema.getCollectionAsTable('mbcollection');
table.select();

//@ pulling as table in tabbed format {!__os_type != 'windows'}
shell.options.resultFormat="tabbed";
table.select();

//@ pulling as table in vertical format {!__os_type != 'windows'}
shell.options.resultFormat="vertical";
table.select();

session.dropSchema('resultset_dumper');
session.close();

//----------------------------------------------------------------
