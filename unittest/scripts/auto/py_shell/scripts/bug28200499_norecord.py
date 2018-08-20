#@ parse an URI with apostrophe, will create a wrapped native dictionary object
uri = shell.parse_uri('root\'root@localhost')

#@ display the result
print(uri)

import json

#@ convert dictionary to a human-readable string, then to a JSON object
json.loads(str(uri))
