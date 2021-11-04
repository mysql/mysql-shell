//@<OUT> CLI -- --help
The following objects provide command line operations:

   cluster
      Represents an InnoDB Cluster.

   clusterset
      Represents an InnoDB ClusterSet.

   customObj
      Dummy global object

   dba
      InnoDB Cluster, ReplicaSet, and ClusterSet management functions.

   rs
      Represents an InnoDB ReplicaSet.

   shell
      Gives access to general purpose functions and properties.

   util
      Global object that groups miscellaneous tools like upgrade checker and
      JSON import.

//@<OUT> CLI customObj showInfo --help
NAME
      show-info - Retrieves personal information

SYNTAX
      customObj show-info <name> <age> <adult> <hobbies> --address=<str>
      <family>

WHERE
      name: String - Name of person.
      age: Integer - Age of person.
      adult: Bool - If person is an adult.
      hobbies: Array - List of hobbies.

OPTIONS
--address=<str>
            Adress location.

--wifeName=<str>
            Wife name.

--sonName=<str>
            Son name.

--dogName[:<type>]=<value>
            Dog name.

//@<OUT> WL14297 - TSFR_2_6_1
My name is John
Next year I will be 31
I am an adult
My hobbies are:
run
tennis
My address is Str.123
My family is:
Tess
Jack
Nick

//@<OUT> WL14297 - TSFR_2_6_2
ERROR: The following required option is missing: --address

//@<OUT> WL14297 - TSFR_5_1_3_3 - Testing list params can be passed as anonymous args, comma separated list of values or json array
a
b
c
a
b
c
a
b
c
a
b
c
a
b
c

//@<OUT> WL14297 - TSFR_6_1_2 - Testing list option can be passed as comma separated list of values, json array or repeated list definition
test
{
    "testOption": [
        "a", 
        "b", 
        "c"
    ]
}
test
{
    "testOption": [
        "a", 
        "b", 
        "c"
    ]
}
test
{
    "testOption": [
        "a", 
        "b", 
        "c"
    ]
}
test
{
    "testOption": [
        "a", 
        "b", 
        "c"
    ]
}
test
{
    "testOption": [
        "a", 
        "b", 
        "c"
    ]
}

//@<OUT> WL14297 - TSFR_8_2_1 - Testing dictionary with no options takes options that don't belong to other dictionary
{
    "testOther": 23, 
    "yetAnother": "sample"
}
{
    "testOption": 45
}

//@<OUT> WL14297 - TSFR_4_1_1 - Testing JSON Escaping is honored
"test"
{
    "optionA": "\"testOption\""
}

