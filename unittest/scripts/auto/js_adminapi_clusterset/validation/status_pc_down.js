
//@<OUT> describe when unavailable
{
    "clusters": {
        "cluster1": {
            "clusterRole": "PRIMARY",
            "topology": [
                {
                    "address": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                    "label": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                    "role": "HA"
                },
                {
                    "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
                    "label": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
                    "role": "HA"
                }
            ]
        },
        "cluster2": {
            "clusterRole": "REPLICA",
            "topology": [
                {
                    "address": "<<<hostname>>>:<<<__mysql_sandbox_port4>>>",
                    "label": "<<<hostname>>>:<<<__mysql_sandbox_port4>>>",
                    "role": "HA"
                },
                {
                    "address": "<<<hostname>>>:<<<__mysql_sandbox_port5>>>",
                    "label": "<<<hostname>>>:<<<__mysql_sandbox_port5>>>",
                    "role": "HA"
                }
            ]
        },
        "cluster3": {
            "clusterRole": "REPLICA",
            "topology": [
                {
                    "address": "<<<hostname>>>:<<<__mysql_sandbox_port6>>>",
                    "label": "<<<hostname>>>:<<<__mysql_sandbox_port6>>>",
                    "role": "HA"
                }
            ]
        }
    },
    "domainName": "cs",
    "primaryCluster": "cluster1"
}

//@<OUT> describe when unreachable
{
    "clusters": {
        "cluster1": {
            "clusterRole": "PRIMARY",
            "topology": [
                {
                    "address": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                    "label": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                    "role": "HA"
                },
                {
                    "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
                    "label": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
                    "role": "HA"
                }
            ]
        },
        "cluster2": {
            "clusterRole": "REPLICA",
            "topology": [
                {
                    "address": "<<<hostname>>>:<<<__mysql_sandbox_port4>>>",
                    "label": "<<<hostname>>>:<<<__mysql_sandbox_port4>>>",
                    "role": "HA"
                },
                {
                    "address": "<<<hostname>>>:<<<__mysql_sandbox_port5>>>",
                    "label": "<<<hostname>>>:<<<__mysql_sandbox_port5>>>",
                    "role": "HA"
                }
            ]
        },
        "cluster3": {
            "clusterRole": "REPLICA",
            "topology": [
                {
                    "address": "<<<hostname>>>:<<<__mysql_sandbox_port6>>>",
                    "label": "<<<hostname>>>:<<<__mysql_sandbox_port6>>>",
                    "role": "HA"
                }
            ]
        }
    },
    "domainName": "cs",
    "primaryCluster": "cluster1"
}
