var schema = session.getSchema('world_x')
var coll = schema.createCollection('big_coll_node_js')
var id_m = 1;
while (id_m <= 1000)
{
	var FirstSection = "{\"GNP\": 414972,"
	var extra_id = " \"Extra_ID\" " + ": \"" + id_m.toString() + "\""
	var FinalSection = ", \"Name\": \"Mexico\", \"IndepYear\": 1810, \"geography\": {\"Region\": \"Central America\", \"Continent\": \"North America\", \"SurfaceArea\": 1958201}, \"government\": {\"HeadOfState\": \"Enrique Penia Nieto\", \"GovernmentForm\": \"Federal Republic\"}, \"demographics\": {\"Population\": 98881000, \"LifeExpectancy\": 71.5}}"
	var res = FirstSection.concat(extra_id,FinalSection)
	coll.add(JSON.parse(res)).execute()
	id_m = id_m + 1
}    
