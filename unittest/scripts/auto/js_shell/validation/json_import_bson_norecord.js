//@<OUT> Enabling ObjectID conversion
doc
{"_id": "5bfe3d5ff6d2f36d067c6fea", "name": "Sample String", "nestedDoc": {"_id": "5bfe3d5ff6d2f36d067c6fe9", "date": {"$date": "2018-11-28T07:01:51.691Z"}}, "timestamp": {"$timestamp": {"i": 1, "t": 1543388511}}}
{"_id": "5bfe3d64f6d2f36d067c6feb", "decimals": [85.23, {"$numberDecimal": "1385.30000000000"}, {"$numberDecimal": "8645.87"}], "integers": [8535, {"$numberLong": "4522"}, {"$numberInt": "850"}]}
{"_id": "5bfe3d68f6d2f36d067c6fec", "binaryData": {"$type": "00", "$binary": "SGVsbG8gV29ybGQh"}, "regExpression": {"$regex": "^some*", "$options": "i"}}

//@<OUT> Enabling extractOidTime
doc
{"_id": "5bfe3d5ff6d2f36d067c6fea", "name": "Sample String", "idTime": "2018-11-28 07:01:51", "nestedDoc": {"_id": "5bfe3d5ff6d2f36d067c6fe9", "date": {"$date": "2018-11-28T07:01:51.691Z"}}, "timestamp": {"$timestamp": {"i": 1, "t": 1543388511}}}
{"_id": "5bfe3d64f6d2f36d067c6feb", "idTime": "2018-11-28 07:01:56", "decimals": [85.23, {"$numberDecimal": "1385.30000000000"}, {"$numberDecimal": "8645.87"}], "integers": [8535, {"$numberLong": "4522"}, {"$numberInt": "850"}]}
{"_id": "5bfe3d68f6d2f36d067c6fec", "idTime": "2018-11-28 07:02:00", "binaryData": {"$type": "00", "$binary": "SGVsbG8gV29ybGQh"}, "regExpression": {"$regex": "^some*", "$options": "i"}}

//@<OUT> Enabling full BSON type conversion
doc
{"_id": "5bfe3d5ff6d2f36d067c6fea", "name": "Sample String", "nestedDoc": {"_id": "5bfe3d5ff6d2f36d067c6fe9", "date": "2018-11-28T07:01:51.691Z"}, "timestamp": "2018-11-28 07:01:51"}
{"_id": "5bfe3d64f6d2f36d067c6feb", "decimals": [85.23, "1385.30000000000", "8645.87"], "integers": [8535, 4522, 850]}
{"_id": "5bfe3d68f6d2f36d067c6fec", "binaryData": "SGVsbG8gV29ybGQh", "regExpression": "^some*"}

//@<OUT> Enabling full BSON type conversion except for Date
doc
{"_id": "5bfe3d5ff6d2f36d067c6fea", "name": "Sample String", "nestedDoc": {"_id": "5bfe3d5ff6d2f36d067c6fe9", "date": {"$date": "2018-11-28T07:01:51.691Z"}}, "timestamp": "2018-11-28 07:01:51"}
{"_id": "5bfe3d64f6d2f36d067c6feb", "decimals": [85.23, "1385.30000000000", "8645.87"], "integers": [8535, 4522, 850]}
{"_id": "5bfe3d68f6d2f36d067c6fec", "binaryData": "SGVsbG8gV29ybGQh", "regExpression": "^some*"}

//@<OUT> Enabling full BSON type conversion except for timestamp
doc
{"_id": "5bfe3d5ff6d2f36d067c6fea", "name": "Sample String", "nestedDoc": {"_id": "5bfe3d5ff6d2f36d067c6fe9", "date": "2018-11-28T07:01:51.691Z"}, "timestamp": {"$timestamp": {"i": 1, "t": 1543388511}}}
{"_id": "5bfe3d64f6d2f36d067c6feb", "decimals": [85.23, "1385.30000000000", "8645.87"], "integers": [8535, 4522, 850]}
{"_id": "5bfe3d68f6d2f36d067c6fec", "binaryData": "SGVsbG8gV29ybGQh", "regExpression": "^some*"}

//@<OUT> Enabling full BSON type conversion except for binary
doc
{"_id": "5bfe3d5ff6d2f36d067c6fea", "name": "Sample String", "nestedDoc": {"_id": "5bfe3d5ff6d2f36d067c6fe9", "date": "2018-11-28T07:01:51.691Z"}, "timestamp": "2018-11-28 07:01:51"}
{"_id": "5bfe3d64f6d2f36d067c6feb", "decimals": [85.23, "1385.30000000000", "8645.87"], "integers": [8535, 4522, 850]}
{"_id": "5bfe3d68f6d2f36d067c6fec", "binaryData": {"$type": "00", "$binary": "SGVsbG8gV29ybGQh"}, "regExpression": "^some*"}

//@<OUT> Enabling full BSON type conversion except for regex
doc
{"_id": "5bfe3d5ff6d2f36d067c6fea", "name": "Sample String", "nestedDoc": {"_id": "5bfe3d5ff6d2f36d067c6fe9", "date": "2018-11-28T07:01:51.691Z"}, "timestamp": "2018-11-28 07:01:51"}
{"_id": "5bfe3d64f6d2f36d067c6feb", "decimals": [85.23, "1385.30000000000", "8645.87"], "integers": [8535, 4522, 850]}
{"_id": "5bfe3d68f6d2f36d067c6fec", "binaryData": "SGVsbG8gV29ybGQh", "regExpression": {"$regex": "^some*", "$options": "i"}}

//@<OUT> Enabling decimalAsDouble
doc
{"_id": "5bfe3d5ff6d2f36d067c6fea", "name": "Sample String", "nestedDoc": {"_id": "5bfe3d5ff6d2f36d067c6fe9", "date": "2018-11-28T07:01:51.691Z"}, "timestamp": "2018-11-28 07:01:51"}
{"_id": "5bfe3d64f6d2f36d067c6feb", "decimals": [85.23, 1385.3, 8645.87], "integers": [8535, 4522, 850]}
{"_id": "5bfe3d68f6d2f36d067c6fec", "binaryData": "SGVsbG8gV29ybGQh", "regExpression": "^some*"}

//@<OUT> Disabling ignoreRegexOptions
doc
{"_id": "5bfe3d5ff6d2f36d067c6fea", "name": "Sample String", "nestedDoc": {"_id": "5bfe3d5ff6d2f36d067c6fe9", "date": "2018-11-28T07:01:51.691Z"}, "timestamp": "2018-11-28 07:01:51"}
{"_id": "5bfe3d64f6d2f36d067c6feb", "decimals": [85.23, "1385.30000000000", "8645.87"], "integers": [8535, 4522, 850]}
{"_id": "5bfe3d68f6d2f36d067c6fec", "binaryData": "SGVsbG8gV29ybGQh", "regExpression": "/^some*/i"}

