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
session.sql('select * from resultset_dumper.bindata');

//@ X Raw Json Format
shell.options.resultFormat = 'json/raw';
session.sql('select * from resultset_dumper.bindata');

//@ X Json Wrapping
testutil.callMysqlsh([__uripwd, "--js", "--quiet-start=2", "-i", "--json", "-e", "session.sql('select * from resultset_dumper.bindata');"]);
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
session.runSql('select * from resultset_dumper.bindata');

//@ Classic Raw Json Format
shell.options.resultFormat = 'json/raw';
session.runSql('select * from resultset_dumper.bindata');

//@ Classic Json Wrapping
testutil.callMysqlsh([__mysqluripwd, "--js", "--quiet-start=2", "-i", "--json", "-e", "session.runSql('select * from resultset_dumper.bindata');"]);
shell.options.resultFormat = 'table';
session.runSql('drop schema resultset_dumper');
session.close();
//------------------------------------------


//@<>---------- TEST FOR MULTIBYTE CHARACTER FORMATTING ----------
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
      values('®7⃣⏰☕♒♣\0⛽🌄🌠🎨🐍🐾').
      values('ascii text').
      values('látin1 text');

//@ table in table format
shell.options.resultFormat = 'table';
table.select();

//@ table in tabbed format
shell.options.resultFormat="tabbed";
table.select();

//@ table in vertical format
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
collection.add({_id:'11', name:'pure ascii text', year: 2014});
collection.add({_id:'12', name:'látiñ text row', year: 2016});

//@ Pulling as collection in JSON format
collection.find();

//@ pulling as table in table format
shell.options.resultFormat="table";
var table = schema.getCollectionAsTable('mbcollection');
table.select();

//@ pulling as table in tabbed format
shell.options.resultFormat="tabbed";
table.select();

//@ pulling as table in vertical format
shell.options.resultFormat="vertical";
table.select();

session.dropSchema('resultset_dumper');
session.close();

//@<>---------- MISC TESTS FOR TABLE FORMATTING ----------

shell.connect(__mysqluripwd);

shell.options.resultFormat="table";

session.runSql("create schema resultset_dumper");

session.runSql("use resultset_dumper");
session.runSql("create table table1 (col1 varchar(100), col2 double, col3 bit(64), col4 text)");
session.runSql("create table table2 (col1 bit(1), col2 bit(2), col3 bit(3), col4 bit(4), col5 bit(5), col56 bit(56), col64 bit(64))");

var col1 = ["hello", "world", null]
var col2 = [0.809643, null, 1]
var col3 = ['0', '1', '01', null]
var col4 = ['bla bla', 'bla blabla blaaaaa', null]

for (i = 0; i < 10; i++) {
  if (col3[i % col3.length] == null)
    session.runSql("insert into table1 values (?, ?, ?, ?)", [col1[i % col1.length], col2[i % col2.length], col3[i % col3.length], col4[i % col4.length]]);
  else
    session.runSql("insert into table1 values (?, ?, b?, ?)", [col1[i % col1.length], col2[i % col2.length], col3[i % col3.length], col4[i % col4.length]]);
}

var col1 = ["hello", "world", "foo bar", "fóo", "foo–bar", "foo-bar", "many values", "Park_Güell", "Ashmore_and_Cartier_Islands"]
var col2 = [1, 0.70964040738497, 0.39888085877797, 0.972853873]
var col3 = ['0', '1', '01010101']
var col4 = ['blablablablablab lablablablablabla blablablabl ablablablablabla', 'bla bla', 'bla blabla blaaaaa']

for (i = 10; i < 1000; i++)
  session.runSql("insert into table1 values (?, ?, b?, ?)", [col1[i % col1.length], col2[i % col2.length], col3[i % col3.length], col4[i % col4.length]]);

// currently, 1000 rows are considered to calculate column widths
// so after 1000, we add some columns that are bigger than before

var col1 = ["hello world", "Alfonso_Aráu", "André-Marie_Ampère", "Very long text but not that long really, but at least longer than before"]
var col2 = [0.1180964040738497123, 0.398880858, 0.9733873]
var col3 = ['0', '1', '01010101111110000001100']
var col4 = ['bla bla', 'blablablabla blablablabla blablablabla blablablabla blablablabla\nblablablabla blablablabla blablablabla\nblablablabla bla!']

for (i = 0; i < 20; i++)
  session.runSql("insert into table1 values (?, ?, b?, ?)", [col1[i % col1.length], col2[i % col2.length], col3[i % col3.length], col4[i % col4.length]]);

session.runSql("insert into table2 values (b'1', b'10', b'101', b'1001', b'10001', b'10101011010010101', b'01010101111110000001100')");

//@ dump a few rows to get a table with narrow values only
session.runSql("select * from table1 limit 10");

//@ dump a few rows to get a table with slightly wider values
session.runSql("select * from table1 limit 20");

//@# dump everything
// validation only checks for a few rows at the top and a few at the end
session.runSql("select * from table1");

//@<> check binary columns output
testutil.wipeAllOutput();

session.runSql("select * from table2");
EXPECT_STDOUT_CONTAINS_MULTILINE(`+------+------+------+------+------+------------------+--------------------+
| col1 | col2 | col3 | col4 | col5 | col56            | col64              |
+------+------+------+------+------+------------------+--------------------+
| 0x01 | 0x02 | 0x05 | 0x09 | 0x11 | 0x00000000015695 | 0x00000000002AFC0C |
+------+------+------+------+------+------------------+--------------------+`);

//@<> cleanup
//session.dropSchema('resultset_dumper');
session.close();

//----------------------------------------------------------------



