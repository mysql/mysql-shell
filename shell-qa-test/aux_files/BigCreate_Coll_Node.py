schema = session.get_schema('world_x')
coll = schema.create_collection('big_coll_node_py')
id_m = 1
while id_m <= 1000:
	res = {
		"GNP": 414972,
		"Extra_ID": str(id_m),
		"Name": "Mexico", 
		"IndepYear": 1810,
		"geography": {
			"Region": "Central America",
			"Continent": "North America", 
			"SurfaceArea": 1958201
			}, 
		"government": {
			"HeadOfState": "Enrique Penia Nieto", 
			"GovernmentForm": "Federal Republic"
			}, 
		"demographics": {
			"Population": 98881000,
			"LifeExpectancy": 71.5
			}
		}
	coll.add(res).execute()
	id_m = id_m + 1
