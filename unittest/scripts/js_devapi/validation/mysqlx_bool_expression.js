//@ Expression evaluation (true)
|1 in (1,2,3)  =>  true|
|1 in [1,2,3]  =>  true|
|[1] in ([1], [2])  =>  true|
|2 in ((1+1))  =>  true|
|[1] in [[1], [2], [3]]  =>  true|
|[] in [[], [2], [3]]  =>  true|
|{'a':5} in [{'a':5}]  =>  true|
|{'a':5} in {'a':5, 'b':6}  =>  true|

//@ Expression evaluation (false)
|4 in (1,2,3)  =>  false|
|4 in [1,2,3]  =>  false|
|[4] in [[1], [2], [3]]  =>  false|
|{'a':5} in [{'a':6}]  =>  false|
|{'a':5} in {'b':6}  =>  false|

//@<OUT> Expression evaluation (filter)
6 in array  =>  [
    {
        "_id": "id2",
        "array": [
            5,
            6,
            7
        ]
    }
]
null in array  =>  []
null in $.array  =>  []
null not in array  =>  [
    {
        "_id": "id2",
        "array": [
            5,
            6,
            7
        ]
    }
]
null not in $.array  =>  [
    {
        "_id": "id2",
        "array": [
            5,
            6,
            7
        ]
    }
]

//@<OUT> IN basic - collection find
(1>5) in (true, false)
(1+5) in (1, 2, 3, 4, 5)
('a'>'b') in (true, false)
1 in (1,2,3)
true IN [(1>5), not (false), (true or false), (false and true)]
true IN ((1>5), not (false), (true or false), (false and true))
actors in actors
{ "name" : "MILLA PECK" } IN actors
[1,2,3] in actors
actor.name IN ['a name', null, (1<5-4), myvar.jsonobj.name]
true IN [1-5/2*2 > 3-2/1*2]
[
    {
        "_id": "a6f4b93e1a264a108393524f29546a8c",
        "actors": [
            {
                "birthdate": "12 Jan 1984",
                "country": "Mexico",
                "name": "MILLA PECK"
            },
            {
                "birthdate": "26 Jul 1975",
                "country": "Botswana",
                "name": "VAL BOLGER"
            },
            {
                "birthdate": "16 Mar 1978",
                "country": "Syria",
                "name": "SCARLETT BENING"
            }
        ],
        "additionalinfo": {
            "director": "Sharice Legaspi",
            "productioncompanies": [
                "Qvodrill",
                "Indigoholdings"
            ],
            "writers": [
                "Rusty Couturier",
                "Angelic Orduno",
                "Carin Postell"
            ]
        },
        "description": "A Fast-Paced Documentary of a Pastry Chef And a Dentist who must Pursue a Forensic Psychologist in The Gulf of Mexico",
        "duration": 130,
        "genre": "Science fiction",
        "language": "English",
        "rating": "G",
        "releaseyear": 2006,
        "title": "AFRICAN EGG"
    }
]

//@# IN syntactically valid but unsupported
||CONT_IN expression requires operator that produce a JSON value.
||CONT_IN expression requires operator that produce a JSON value.
||CONT_IN expression requires operator that produce a JSON value.
||CONT_IN expression requires operator that produce a JSON value.
||CONT_IN expression requires operator that produce a JSON value.

//@<OUT> IN basic - collection modify
(1>5) in (true, false)
(1+5) in (1, 2, 3, 4, 5)
('a'>'b') in (true, false)
1 in (1,2,3)
true IN [(1>5), not (false), (true or false), (false and true)]
true IN ((1>5), not (false), (true or false), (false and true))
actors in actors
{ "name" : "MILLA PECK" } IN actors
[1,2,3] in actors
actor.name IN ['a name', null, (1<5-4), myvar.jsonobj.name]
true IN [1-5/2*2 > 3-2/1*2]
Query OK, 0 rows affected (0.00 sec)

//@<OUT> IN basic - collection remove
(1>5) in (true, false)
(1+5) in (1, 2, 3, 4, 5)
('a'>'b') in (true, false)
1 in (1,2,3)
true IN [(1>5), not (false), (true or false), (false and true)]
true IN ((1>5), not (false), (true or false), (false and true))
actors in actors
{ "name" : "MILLA PECK" } IN actors
[1,2,3] in actors
actor.name IN ['a name', null, (1<5-4), myvar.jsonobj.name]
true IN [1-5/2*2 > 3-2/1*2]
Query OK, 0 rows affected (0.00 sec)

//@<OUT> IN basic - table select
(1>5) in (true, false)
(1+5) in (1, 2, 3, 4, 5)
('a'>'b') in (true, false)
1 in (1,2,3)
true IN [(1>5), not (false), (true or false), (false and true)]
true IN ((1>5), not (false), (true or false), (false and true))
doc->'$.actors' in doc->'$.actors'

//@<OUT> IN basic - table update
(1>5) in (true, false)
(1+5) in (1, 2, 3, 4, 5)
('a'>'b') in (true, false)
1 in (1,2,3)
true IN [(1>5), not (false), (true or false), (false and true)]
true IN ((1>5), not (false), (true or false), (false and true))
doc->'$.actors' in doc->'$.actors'

//@<OUT> IN basic - table delete
(1>5) in (true, false)
(1+5) in (1, 2, 3, 4, 5)
('a'>'b') in (true, false)
1 in (1,2,3)
true IN [(1>5), not (false), (true or false), (false and true)]
true IN ((1>5), not (false), (true or false), (false and true))
doc->'$.actors' in doc->'$.actors'

//@<OUT> WL10848 F2 - The evaluation of the IN operation between 2 operands is equivalent to a call to the JSON_CONTAINS() function with said operands Rules defined for JSON_CONTAINS():
'African Egg' IN ('African Egg', 1, true, NULL, [0,1,2], { 'title' : 'Atomic Firefighter' })
1 IN ('African Egg', 1, true, NULL, [0,1,2], { 'title' : 'Atomic Firefighter' })
false IN ('African Egg', 1, true, NULL, [0,1,2], { 'title' : 'Atomic Firefighter' })
[0,1,2] IN ('African Egg', 1, true, NULL, [0,1,2], { 'title' : 'Atomic Firefighter' })
{ 'title' : 'Atomic Firefighter' } IN ('African Egg', 1, true, NULL, [0,1,2], { 'title' : 'Atomic Firefighter' })
[
    {
        "{ 'title' : 'Atomic Firefighter' } IN ('African Egg', 1, true, NULL, [0,1,2], { 'title' : 'Atomic Firefighter' }) ": true
    }
]

//@<OUT> WL10848 F3 - If the right side operand of the IN operator is a comma separated list of expressions enclosed in parenthesis -- like ('foo', 'bar', 'baz', current_date()) -- the expression must translate to the existing SQL IN operator
title IN ('African Egg', 'The Witcher', 'Jurassic Perk')
releaseyear IN (2006, 2010, 2017)
[
    {
        "releaseyear IN (2006, 2010, 2017)": true
    }
]

//@<OUT> WL10848 F4 - If any of the operands is the SQL NULL value (like when a document field that does not exist), the operation evaluates to NULL
'African Egg' in movietitle
NULL in title
[
    {
        "NULL in title": false
    }
]

//@<OUT> WL10848 F5 - The result of the evaluation of the IN operator is a boolean value. The operation evaluates to TRUE if the left side operand is contained in the right side and FALSE otherwise
1 IN [1,2,3]
0 IN [1,2,3]
[
    {
        "0 IN [1,2,3]": false
    }
]

//@<OUT> WL10848 F6 - The result of the evaluation of the NOT IN operator is a boolean value. The operation evaluates to TRUE if the left side operand is NOT contained in the right side and FALSE otherwise
1 NOT IN [1,2,3]
0 NOT IN [1,2,3]
[
    {
        "0 NOT IN [1,2,3]": true
    }
]

//@<OUT> Search for empty strings in a field
[]

//@<OUT> Search for a field in an empty string
[]
[]

//@<OUT> Search for an array in a field
[
    {
        "_id": "a6f4b93e1a264a108393524f29546a8c",
        "actors": [
            {
                "birthdate": "12 Jan 1984",
                "country": "Mexico",
                "name": "MILLA PECK"
            },
            {
                "birthdate": "26 Jul 1975",
                "country": "Botswana",
                "name": "VAL BOLGER"
            },
            {
                "birthdate": "16 Mar 1978",
                "country": "Syria",
                "name": "SCARLETT BENING"
            }
        ],
        "additionalinfo": {
            "director": "Sharice Legaspi",
            "productioncompanies": [
                "Qvodrill",
                "Indigoholdings"
            ],
            "writers": [
                "Rusty Couturier",
                "Angelic Orduno",
                "Carin Postell"
            ]
        },
        "description": "A Fast-Paced Documentary of a Pastry Chef And a Dentist who must Pursue a Forensic Psychologist in The Gulf of Mexico",
        "duration": 130,
        "genre": "Science fiction",
        "language": "English",
        "rating": "G",
        "releaseyear": 2006,
        "title": "AFRICAN EGG"
    }
]

//@<OUT> Search for a document in a field
[
    {
        "_id": "a6f4b93e1a264a108393524f29546a8c",
        "actors": [
            {
                "birthdate": "12 Jan 1984",
                "country": "Mexico",
                "name": "MILLA PECK"
            },
            {
                "birthdate": "26 Jul 1975",
                "country": "Botswana",
                "name": "VAL BOLGER"
            },
            {
                "birthdate": "16 Mar 1978",
                "country": "Syria",
                "name": "SCARLETT BENING"
            }
        ],
        "additionalinfo": {
            "director": "Sharice Legaspi",
            "productioncompanies": [
                "Qvodrill",
                "Indigoholdings"
            ],
            "writers": [
                "Rusty Couturier",
                "Angelic Orduno",
                "Carin Postell"
            ]
        },
        "description": "A Fast-Paced Documentary of a Pastry Chef And a Dentist who must Pursue a Forensic Psychologist in The Gulf of Mexico",
        "duration": 130,
        "genre": "Science fiction",
        "language": "English",
        "rating": "G",
        "releaseyear": 2006,
        "title": "AFRICAN EGG"
    }
]

//@<OUT> Search for a field in a custom array
[
    {
        "_id": "a6f4b93e1a264a108393524f29546a8c",
        "actors": [
            {
                "birthdate": "12 Jan 1984",
                "country": "Mexico",
                "name": "MILLA PECK"
            },
            {
                "birthdate": "26 Jul 1975",
                "country": "Botswana",
                "name": "VAL BOLGER"
            },
            {
                "birthdate": "16 Mar 1978",
                "country": "Syria",
                "name": "SCARLETT BENING"
            }
        ],
        "additionalinfo": {
            "director": "Sharice Legaspi",
            "productioncompanies": [
                "Qvodrill",
                "Indigoholdings"
            ],
            "writers": [
                "Rusty Couturier",
                "Angelic Orduno",
                "Carin Postell"
            ]
        },
        "description": "A Fast-Paced Documentary of a Pastry Chef And a Dentist who must Pursue a Forensic Psychologist in The Gulf of Mexico",
        "duration": 130,
        "genre": "Science fiction",
        "language": "English",
        "rating": "G",
        "releaseyear": 2006,
        "title": "AFRICAN EGG"
    }
]

//@<OUT> Search for a boolean in a field
[]
[]

//@<OUT> Search for nested values in a document
[
    {
        "_id": "a6f4b93e1a264a108393524f29546a8c",
        "actors": [
            {
                "birthdate": "12 Jan 1984",
                "country": "Mexico",
                "name": "MILLA PECK"
            },
            {
                "birthdate": "26 Jul 1975",
                "country": "Botswana",
                "name": "VAL BOLGER"
            },
            {
                "birthdate": "16 Mar 1978",
                "country": "Syria",
                "name": "SCARLETT BENING"
            }
        ],
        "additionalinfo": {
            "director": "Sharice Legaspi",
            "productioncompanies": [
                "Qvodrill",
                "Indigoholdings"
            ],
            "writers": [
                "Rusty Couturier",
                "Angelic Orduno",
                "Carin Postell"
            ]
        },
        "description": "A Fast-Paced Documentary of a Pastry Chef And a Dentist who must Pursue a Forensic Psychologist in The Gulf of Mexico",
        "duration": 130,
        "genre": "Science fiction",
        "language": "English",
        "rating": "G",
        "releaseyear": 2006,
        "title": "AFRICAN EGG"
    }
]

//@<OUT> Search for field in an array of documents

//@<OUT> Search for a value in a nested array
[
    {
        "_id": "a6f4b93e1a264a108393524f29546a8c",
        "actors": [
            {
                "birthdate": "12 Jan 1984",
                "country": "Mexico",
                "name": "MILLA PECK"
            },
            {
                "birthdate": "26 Jul 1975",
                "country": "Botswana",
                "name": "VAL BOLGER"
            },
            {
                "birthdate": "16 Mar 1978",
                "country": "Syria",
                "name": "SCARLETT BENING"
            }
        ],
        "additionalinfo": {
            "director": "Sharice Legaspi",
            "productioncompanies": [
                "Qvodrill",
                "Indigoholdings"
            ],
            "writers": [
                "Rusty Couturier",
                "Angelic Orduno",
                "Carin Postell"
            ]
        },
        "description": "A Fast-Paced Documentary of a Pastry Chef And a Dentist who must Pursue a Forensic Psychologist in The Gulf of Mexico",
        "duration": 130,
        "genre": "Science fiction",
        "language": "English",
        "rating": "G",
        "releaseyear": 2006,
        "title": "AFRICAN EGG"
    }
]
