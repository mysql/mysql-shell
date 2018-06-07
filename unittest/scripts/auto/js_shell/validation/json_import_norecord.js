//@ Only X Protocol is supported
||

//@ Setup test
||

//@ FR1-01
||

//@ FR1-02 function call
||

//@ FR1-02 cli call
||

//@ FR1-03
||

//@ FR1-04
||

//@ FR1-05
||

//@ FR1-06  Validate that import works with multiline documents.
||

//@ FR2-01a
||

//@ FR2-01b
||

//@ FR2-02a
||

//@ FR2-02b
||

//@ FR2-03 Using shell function call
||

//@ FR2-04
||

//@ FR5-01
||

//@<OUT> FR5-01 output
[
    {
        "_id": "57506d62f57802807471dd22", 
        "categories": [
            "Bakery",
            "Cafe",
            "Coffee",
            "Dessert"
        ], 
        "contact": {
            "email": "HotBakeryCafe@example.net", 
            "location": [
                -73.96485799999999,
                40.761899
            ], 
            "phone": "264-555-0171"
        }, 
        "name": "Hot Bakery Cafe", 
        "stars": 4
    },
    {
        "_id": "57506d62f57802807471dd28", 
        "categories": [
            "Bagels",
            "Sandwiches",
            "Coffee"
        ], 
        "contact": {
            "email": "XYZBagelsRestaurant@example.net", 
            "location": [
                -74.0707363,
                40.59321569999999
            ], 
            "phone": "435-555-0190"
        }, 
        "name": "XYZ Bagels Restaurant", 
        "stars": 4
    },
    {
        "_id": "57506d62f57802807471dd35", 
        "categories": [
            "Coffee",
            "Cafe",
            "Bakery",
            "Chocolates"
        ], 
        "contact": {
            "email": "XYZCoffeeBar@example.net", 
            "location": [
                -74.0166091,
                40.6284767
            ], 
            "phone": "644-555-0193"
        }, 
        "name": "XYZ Coffee Bar", 
        "stars": 5
    },
    {
        "_id": "57506d62f57802807471dd39", 
        "categories": [
            "Pizza",
            "Italian"
        ], 
        "contact": {
            "email": "GreenFeastPizzeria@example.com", 
            "location": [
                -74.1220973,
                40.6129407
            ], 
            "phone": "840-555-0102"
        }, 
        "name": "Green Feast Pizzeria", 
        "stars": 2
    },
    {
        "_id": "57506d62f57802807471dd3a", 
        "categories": [
            "Pasta",
            "Italian",
            "Buffet",
            "Cafeteria"
        ], 
        "contact": {
            "email": "ZZZPastaBuffet@example.com", 
            "location": [
                -73.9446421,
                40.7253944
            ], 
            "phone": "769-555-0152"
        }, 
        "name": "ZZZ Pasta Buffet", 
        "stars": 0
    },
    {
        "_id": "57506d62f57802807471dd3b", 
        "categories": [
            "Bagels",
            "Cookies",
            "Sandwiches"
        ], 
        "contact": {
            "email": "BlueBagelsGrill@example.com", 
            "location": [
                -73.92506,
                40.8275556
            ], 
            "phone": "786-555-0102"
        }, 
        "name": "Blue Bagels Grill", 
        "stars": 3
    },
    {
        "_id": "57506d62f57802807471dd3d", 
        "categories": [
            "Pizza",
            "Pasta",
            "Italian",
            "Coffee",
            "Sandwiches"
        ], 
        "contact": {
            "email": "SunBakeryTrattoria@example.org", 
            "location": [
                -74.0056649,
                40.7452371
            ], 
            "phone": "386-555-0189"
        }, 
        "name": "Sun Bakery Trattoria", 
        "stars": 4
    },
    {
        "_id": "57506d62f57802807471dd41", 
        "categories": [
            "Steak",
            "Seafood"
        ], 
        "contact": {
            "email": "456SteakRestaurant@example.com", 
            "location": [
                -73.9365108,
                40.8497077
            ], 
            "phone": "990-555-0165"
        }, 
        "name": "456 Steak Restaurant", 
        "stars": 0
    },
    {
        "_id": "57506d62f57802807471dd42", 
        "categories": [
            "Bakery",
            "Cookies",
            "Cake",
            "Coffee"
        ], 
        "contact": {
            "email": "456CookiesShop@example.org", 
            "location": [
                -73.8850023,
                40.7494272
            ], 
            "phone": "604-555-0149"
        }, 
        "name": "456 Cookies Shop", 
        "stars": 4
    },
    {
        "_id": "57506d62f57802807471dd44", 
        "categories": [
            "Steak",
            "Salad",
            "Chinese"
        ], 
        "contact": {
            "email": "XYZSteakBuffet@example.org", 
            "location": [
                -73.9799932,
                40.7660886
            ], 
            "phone": "229-555-0197"
        }, 
        "name": "XYZ Steak Buffet", 
        "stars": 3
    }
]

//@ FR5-02
||

//@<OUT> FR5-02 output


//@ FRA1-02A
||

//@ FRA1-02B
||

//@ FRA1-03
||

//@ FRB1-01
||

//@<OUT> FRB1-01 target collection correctness
[
    {
        "_id": "55cb9c666c522cafdb053a1a", 
        "geometry": {
            "coordinates": [
                [
                    [
                        -73.94400504048726,
                        40.70196179219718
                    ],
                    [
                        -73.94220058705264,
                        40.700890667467746
                    ],
                    [
                        -73.94193078816193,
                        40.70072523469547
                    ]
                ]
            ], 
            "type": "Polygon"
        }, 
        "name": "Bedford"
    }
]


//@<OUT> FRB1-01 target table correctness
+----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+----+
| user_column_name                                                                                                                                                                                                                       | id |
+----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+----+
| {"_id": "55cb9c666c522cafdb053a1a", "name": "Bedford", "geometry": {"type": "Polygon", "coordinates": [[[-73.94400504048726, 40.70196179219718], [-73.94220058705264, 40.700890667467746], [-73.94193078816193, 40.70072523469547]]]}} |  1 |
+----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+----+

//@<OUT> FRB1-03
+----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| doc                                                                                                                                                                                                                                    |
+----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| {"_id": "55cb9c666c522cafdb053a1a", "name": "Bedford", "geometry": {"type": "Polygon", "coordinates": [[[-73.94400504048726, 40.70196179219718], [-73.94220058705264, 40.700890667467746], [-73.94193078816193, 40.70072523469547]]]}} |
+----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+

//@ FRB1-04 function call
||

//@ FRB1-04 cli
||

//@ FRB1-05
||

//@<OUT> FRB2-02
+-------+---------+------+-----+---------+----------------+
| Field | Type    | Null | Key | Default | Extra          |
+-------+---------+------+-----+---------+----------------+
| doc   | json    | YES  |     | NULL    |                |
| id    | int(11) | NO   | PRI | NULL    | auto_increment |
+-------+---------+------+-----+---------+----------------+

//@<OUT> FRB3-01a
+---------------+---------+------+-----+---------+----------------+
| Field         | Type    | Null | Key | Default | Extra          |
+---------------+---------+------+-----+---------+----------------+
| ident         | int(11) | NO   | PRI | NULL    | auto_increment |
| json_data     | json    | YES  |     | NULL    |                |
| default_value | int(11) | NO   |     | 42      |                |
+---------------+---------+------+-----+---------+----------------+

//@<OUT> FRB3-01b 8.0 {VER(>=8.0)}
+-------+------------------------------------------------------------------------------------------------------------------------------------------------------------------------+---------------+
| ident | json_data                                                                                                                                                              | default_value |
+-------+------------------------------------------------------------------------------------------------------------------------------------------------------------------------+---------------+
|     1 | {"_id": "0000000000001", "City": "Kraków", "area": 556.76, "District": "Dzielnica I Stare Miasto", "residents": 35573, "population density": 6382.82}                 |            42 |
|     2 | {"_id": "0000000000002", "City": "Kraków", "area": 584.52, "District": "Dzielnica II Grzegórzki", "residents": 29230, "population density": 5000.68}                 |            42 |
|     3 | {"_id": "0000000000003", "City": "Kraków", "area": 643.79, "District": "Dzielnica III Prądnik Czerwony", "residents": 47775, "population density": 7420.9}           |            42 |
|     4 | {"_id": "0000000000004", "City": "Kraków", "area": 2341.87, "District": "Dzielnica IV Prądnik Biały", "residents": 69135, "population density": 2952.13}            |            42 |
|     5 | {"_id": "0000000000005", "City": "Kraków", "area": 561.9, "District": "Dzielnica V Krowodrza", "residents": 31870, "population density": 5671.83}                     |            42 |
|     6 | {"_id": "0000000000006", "City": "Kraków", "area": 955.96, "District": "Dzielnica VI Bronowice", "residents": 23465, "population density": 2454.6}                    |            42 |
|     7 | {"_id": "0000000000007", "City": "Kraków", "area": 2873.1, "District": "Dzielnica VII Zwierzyniec", "residents": 20454, "population density": 711.91}                 |            42 |
|     8 | {"_id": "0000000000008", "City": "Kraków", "area": 4618.87, "District": "Dzielnica VIII Dębniki", "residents": 59395, "population density": 1295.92}                 |            42 |
|     9 | {"_id": "0000000000009", "City": "Kraków", "area": 541.51, "District": "Dzielnica IX Łagiewniki-Borek Fałęcki", "residents": 14859, "population density": 2743.99} |            42 |
|    10 | {"_id": "000000000000a", "City": "Kraków", "area": 2560.4, "District": "Dzielnica X Swoszowice", "residents": 25608, "population density": 1000.16}                   |            42 |
|    11 | {"_id": "000000000000b", "City": "Kraków", "area": 954.0, "District": "Dzielnica XI Podgórze Duchackie", "residents": 52859, "population density": 5540.78}          |            42 |
|    12 | {"_id": "000000000000c", "City": "Kraków", "area": 1847.39, "District": "Dzielnica XII Bieżanów-Prokocim", "residents": 63026, "population density": 3411.62}       |            42 |
|    13 | {"_id": "000000000000d", "City": "Kraków", "area": 2566.71, "District": "Dzielnica XIII Podgórze", "residents": 34045, "population density": 1326.41}                |            42 |
|    14 | {"_id": "000000000000e", "City": "Kraków", "area": 1225.68, "District": "Dzielnica XIV Czyżyny", "residents": 26699, "population density": 2178.3}                   |            42 |
|    15 | {"_id": "000000000000f", "City": "Kraków", "area": 559.0, "District": "Dzielnica XV Mistrzejowice", "residents": 53015, "population density": 9483.9}                 |            42 |
|    16 | {"_id": "0000000000010", "City": "Kraków", "area": 369.9, "District": "Dzielnica XVI Bieńczyce", "residents": 42633, "population density": 11525.55}                 |            42 |
|    17 | {"_id": "0000000000011", "City": "Kraków", "area": 2381.55, "District": "Dzielnica XVII Wzgórza Krzesławickie", "residents": 20303, "population density": 852.51}   |            42 |
|    18 | {"_id": "0000000000012", "City": "Kraków", "area": 6540.99, "District": "Dzielnica XVIII Nowa Huta", "residents": 54588, "population density": 834.55}                |            42 |
+-------+------------------------------------------------------------------------------------------------------------------------------------------------------------------------+---------------+

//@<OUT> FRB3-01b 5.7 {VER(<8.0)}
+-------+------------------------------------------------------------------------------------------------------------------------------------------------------------------------+---------------+
| ident | json_data                                                                                                                                                              | default_value |
+-------+------------------------------------------------------------------------------------------------------------------------------------------------------------------------+---------------+
|     1 | {"_id": "0000000000001", "City": "Kraków", "area": 556.76, "District": "Dzielnica I Stare Miasto", "residents": 35573, "population density": 6382.82}                 |            42 |
|     2 | {"_id": "0000000000002", "City": "Kraków", "area": 584.52, "District": "Dzielnica II Grzegórzki", "residents": 29230, "population density": 5000.68}                 |            42 |
|     3 | {"_id": "0000000000003", "City": "Kraków", "area": 643.79, "District": "Dzielnica III Prądnik Czerwony", "residents": 47775, "population density": 7420.9}           |            42 |
|     4 | {"_id": "0000000000004", "City": "Kraków", "area": 2341.87, "District": "Dzielnica IV Prądnik Biały", "residents": 69135, "population density": 2952.13}            |            42 |
|     5 | {"_id": "0000000000005", "City": "Kraków", "area": 561.9, "District": "Dzielnica V Krowodrza", "residents": 31870, "population density": 5671.83}                     |            42 |
|     6 | {"_id": "0000000000006", "City": "Kraków", "area": 955.96, "District": "Dzielnica VI Bronowice", "residents": 23465, "population density": 2454.6}                    |            42 |
|     7 | {"_id": "0000000000007", "City": "Kraków", "area": 2873.1, "District": "Dzielnica VII Zwierzyniec", "residents": 20454, "population density": 711.91}                 |            42 |
|     8 | {"_id": "0000000000008", "City": "Kraków", "area": 4618.87, "District": "Dzielnica VIII Dębniki", "residents": 59395, "population density": 1295.92}                 |            42 |
|     9 | {"_id": "0000000000009", "City": "Kraków", "area": 541.51, "District": "Dzielnica IX Łagiewniki-Borek Fałęcki", "residents": 14859, "population density": 2743.99} |            42 |
|    10 | {"_id": "000000000000a", "City": "Kraków", "area": 2560.4, "District": "Dzielnica X Swoszowice", "residents": 25608, "population density": 1000.16}                   |            42 |
|    11 | {"_id": "000000000000b", "City": "Kraków", "area": 954, "District": "Dzielnica XI Podgórze Duchackie", "residents": 52859, "population density": 5540.78}            |            42 |
|    12 | {"_id": "000000000000c", "City": "Kraków", "area": 1847.39, "District": "Dzielnica XII Bieżanów-Prokocim", "residents": 63026, "population density": 3411.62}       |            42 |
|    13 | {"_id": "000000000000d", "City": "Kraków", "area": 2566.71, "District": "Dzielnica XIII Podgórze", "residents": 34045, "population density": 1326.41}                |            42 |
|    14 | {"_id": "000000000000e", "City": "Kraków", "area": 1225.68, "District": "Dzielnica XIV Czyżyny", "residents": 26699, "population density": 2178.3}                   |            42 |
|    15 | {"_id": "000000000000f", "City": "Kraków", "area": 559, "District": "Dzielnica XV Mistrzejowice", "residents": 53015, "population density": 9483.9}                   |            42 |
|    16 | {"_id": "0000000000010", "City": "Kraków", "area": 369.9, "District": "Dzielnica XVI Bieńczyce", "residents": 42633, "population density": 11525.55}                 |            42 |
|    17 | {"_id": "0000000000011", "City": "Kraków", "area": 2381.55, "District": "Dzielnica XVII Wzgórza Krzesławickie", "residents": 20303, "population density": 852.51}   |            42 |
|    18 | {"_id": "0000000000012", "City": "Kraków", "area": 6540.99, "District": "Dzielnica XVIII Nowa Huta", "residents": 54588, "population density": 834.55}                |            42 |
+-------+------------------------------------------------------------------------------------------------------------------------------------------------------------------------+---------------+

//@ E2
||

//@<OUT> E2 count
+----------+
| count(1) |
+----------+
|    25359 |
+----------+

//@ E3
|<Collection:blubb>|

//@ E4
|<Table:blubb_table>|

//@<OUT> E4 describe
+-------+---------+------+-----+---------+----------------+
| Field | Type    | Null | Key | Default | Extra          |
+-------+---------+------+-----+---------+----------------+
| doc   | json    | YES  |     | NULL    |                |
| id    | int(11) | NO   | PRI | NULL    | auto_increment |
+-------+---------+------+-----+---------+----------------+

//@ Import document with size greater than mysqlx_max_allowed_packet
||

//@ Import document with size equal to mysqlx_max_allowed_packet
||

//@ Import 2 megabyte document with recommended mysqlx_max_allowed_packet value
||

//@ Import document with size less than mysqlx_max_allowed_packet
||

//@ Teardown
||
