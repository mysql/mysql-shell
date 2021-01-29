
//@<OUT> describe when unavailable
{
    "clusters": {
        "cluster1": {
            "clusterRole": "PRIMARY", 
            "topology": [
                {
                    "address": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>", 
                    "label": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"
                }, 
                {
                    "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>", 
                    "label": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>"
                }
            ]
        }, 
        "cluster2": {
            "clusterRole": "REPLICA", 
            "topology": [
                {
                    "address": "<<<hostname>>>:<<<__mysql_sandbox_port4>>>", 
                    "label": "<<<hostname>>>:<<<__mysql_sandbox_port4>>>"
                }, 
                {
                    "address": "<<<hostname>>>:<<<__mysql_sandbox_port5>>>", 
                    "label": "<<<hostname>>>:<<<__mysql_sandbox_port5>>>"
                }
            ]
        }, 
        "cluster3": {
            "clusterRole": "REPLICA", 
            "topology": [
                {
                    "address": "<<<hostname>>>:<<<__mysql_sandbox_port6>>>", 
                    "label": "<<<hostname>>>:<<<__mysql_sandbox_port6>>>"
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
                    "label": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"
                }, 
                {
                    "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>", 
                    "label": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>"
                }
            ]
        }, 
        "cluster2": {
            "clusterRole": "REPLICA", 
            "topology": [
                {
                    "address": "<<<hostname>>>:<<<__mysql_sandbox_port4>>>", 
                    "label": "<<<hostname>>>:<<<__mysql_sandbox_port4>>>"
                }, 
                {
                    "address": "<<<hostname>>>:<<<__mysql_sandbox_port5>>>", 
                    "label": "<<<hostname>>>:<<<__mysql_sandbox_port5>>>"
                }
            ]
        }, 
        "cluster3": {
            "clusterRole": "REPLICA", 
            "topology": [
                {
                    "address": "<<<hostname>>>:<<<__mysql_sandbox_port6>>>", 
                    "label": "<<<hostname>>>:<<<__mysql_sandbox_port6>>>"
                }
            ]
        }
    }, 
    "domainName": "cs", 
    "primaryCluster": "cluster1"
}
