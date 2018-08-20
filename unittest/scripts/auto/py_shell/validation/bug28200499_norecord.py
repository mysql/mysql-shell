#@ parse an URI with apostrophe, will create a wrapped native dictionary object
||

#@<OUT> display the result
{"host": "localhost", "user": "root'root"}

#@<OUT> convert dictionary to a human-readable string, then to a JSON object
{
    "host": "localhost",
    "user": "root'root"
}
