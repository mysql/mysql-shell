//@<OUT> WL#13788 Check the output of rs.options is as expected and that the function gets its information through the primary
{
    "replicaSet": {
        "name": "myrs",
        "tags": {
            "global": [
                {
                    "option": "global_custom",
                    "value": "global_tag"
                }
            ],
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": [
                {
                    "option": "bool_tag",
                    "value": true
                },
                {
                    "option": "int_tag",
                    "value": 111
                },
                {
                    "option": "string_tag",
                    "value": "instance1"
                }
            ],
            "<<<hostname>>>:<<<__mysql_sandbox_port2>>>": [
                {
                    "option": "_disconnect_existing_sessions_when_hidden",
                    "value": true
                },
                {
                    "option": "_hidden",
                    "value": false
                },
                {
                    "option": "bool_tag",
                    "value": false
                },
                {
                    "option": "int_tag",
                    "value": 222
                },
                {
                    "option": "string_tag",
                    "value": "instance2"
                }
            ]
        }
    }
}
