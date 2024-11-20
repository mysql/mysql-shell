const default_cluster_guideline =
{
    "destinations": [
        {
            "match": "$.server.memberRole = PRIMARY",
            "name": "Primary"
        },
        {
            "match": "$.server.memberRole = SECONDARY",
            "name": "Secondary"
        },
        {
            "match": "$.server.memberRole = READ_REPLICA",
            "name": "ReadReplica"
        }
    ],
    "name": "default_cluster_rg",
    "routes": [
        {
            "connectionSharingAllowed": true,
            "destinations": [
                {
                    "classes": [
                        "Primary"
                    ],
                    "priority": 0,
                    "strategy": "round-robin"
                }
            ],
            "enabled": true,
            "match": "$.session.targetPort = $.router.port.rw",
            "name": "rw"
        },
        {
            "connectionSharingAllowed": true,
            "destinations": [
                {
                    "classes": [
                        "Secondary"
                    ],
                    "priority": 0,
                    "strategy": "round-robin"
                },
                {
                    "classes": [
                        "Primary"
                    ],
                    "priority": 1,
                    "strategy": "round-robin"
                }
            ],
            "enabled": true,
            "match": "$.session.targetPort = $.router.port.ro",
            "name": "ro"
        }
    ],
    "version": "1.0"
}

const expected_default_cluster_guideline_destinations = ["Primary", "Secondary", "ReadReplica"];
const expected_default_cluster_guideline_destinations_match = ["$.server.memberRole = PRIMARY", "$.server.memberRole = SECONDARY", "$.server.memberRole = READ_REPLICA"];

const expected_default_cluster_guideline_routes = ["rw", "ro"];
const expected_default_cluster_guideline_routes_match = ["$.session.targetPort = $.router.port.rw", "$.session.targetPort = $.router.port.ro"
];
const expected_default_cluster_guideline_routes_destinations = ["round-robin(Primary)", "round-robin(Secondary), round-robin(Primary)"];

const cluster_guideline_ro_targets_read_replicas =
{
    "destinations": [
        {
            "match": "$.server.memberRole = PRIMARY",
            "name": "Primary"
        },
        {
            "match": "$.server.memberRole = SECONDARY",
            "name": "Secondary"
        },
        {
            "match": "$.server.memberRole = READ_REPLICA",
            "name": "ReadReplica"
        }
    ],
    "name": "default_cluster_read_replicas_rg",
    "routes": [
        {
            "connectionSharingAllowed": true,
            "destinations": [
                {
                    "classes": [
                        "Primary"
                    ],
                    "priority": 0,
                    "strategy": "round-robin"
                }
            ],
            "enabled": true,
            "match": "$.session.targetPort = $.router.port.rw",
            "name": "rw"
        },
        {
            "connectionSharingAllowed": true,
            "destinations": [
                {
                    "classes": [
                        "ReadReplica"
                    ],
                    "priority": 0,
                    "strategy": "round-robin"
                },
                {
                    "classes": [
                        "Primary"
                    ],
                    "priority": 1,
                    "strategy": "round-robin"
                }
            ],
            "enabled": true,
            "match": "$.session.targetPort = $.router.port.ro",
            "name": "ro"
        }
    ],
    "version": "1.0"
}

const expected_cluster_guideline_ro_targets_read_replicas_routes_destinations = ["round-robin(Primary)", "round-robin(ReadReplica), round-robin(Primary)"];

const cluster_guideline_ro_targets_all =
{
    "destinations": [
        {
            "match": "$.server.memberRole = PRIMARY",
            "name": "Primary"
        },
        {
            "match": "$.server.memberRole = SECONDARY",
            "name": "Secondary"
        },
        {
            "match": "$.server.memberRole = READ_REPLICA",
            "name": "ReadReplica"
        }
    ],
    "name": "default_cluster_all_rg",
    "routes": [
        {
            "connectionSharingAllowed": true,
            "destinations": [
                {
                    "classes": [
                        "Primary"
                    ],
                    "priority": 0,
                    "strategy": "round-robin"
                }
            ],
            "enabled": true,
            "match": "$.session.targetPort = $.router.port.rw",
            "name": "rw"
        },
        {
            "connectionSharingAllowed": true,
            "destinations": [
                {
                    "classes": [
                        "Secondary",
                        "ReadReplica"
                    ],
                    "priority": 0,
                    "strategy": "round-robin"
                },
                {
                    "classes": [
                        "Primary"
                    ],
                    "priority": 1,
                    "strategy": "round-robin"
                }
            ],
            "enabled": true,
            "match": "$.session.targetPort = $.router.port.ro",
            "name": "ro"
        }
    ],
    "version": "1.0"
}

const expected_cluster_guideline_ro_targets_all_routes_destinations = ["round-robin(Primary)", "round-robin(Secondary, ReadReplica), round-robin(Primary)"];

const default_clusterset_guideline =
{
    "destinations": [
        {
            "match": "$.server.memberRole = PRIMARY AND $.server.clusterRole = PRIMARY",
            "name": "Primary"
        },
        {
            "match": "$.server.memberRole = SECONDARY AND $.server.clusterRole = PRIMARY",
            "name": "PrimaryClusterSecondary"
        },
        {
            "match": "$.server.memberRole = READ_REPLICA AND $.server.clusterRole = PRIMARY",
            "name": "PrimaryClusterReadReplica"
        }
    ],
    "name": "default_cluster_rg",
    "routes": [
        {
            "connectionSharingAllowed": true,
            "destinations": [
                {
                    "classes": [
                        "Primary"
                    ],
                    "priority": 0,
                    "strategy": "round-robin"
                }
            ],
            "enabled": true,
            "match": "$.session.targetPort = $.router.port.rw",
            "name": "rw"
        },
        {
            "connectionSharingAllowed": true,
            "destinations": [
                {
                    "classes": [
                        "PrimaryClusterSecondary"
                    ],
                    "priority": 0,
                    "strategy": "round-robin"
                },
                {
                    "classes": [
                        "Primary"
                    ],
                    "priority": 1,
                    "strategy": "round-robin"
                }
            ],
            "enabled": true,
            "match": "$.session.targetPort = $.router.port.ro",
            "name": "ro"
        }
    ],
    "version": "1.0"
}

const expected_default_clusterset_guideline_destinations = ["Primary", "PrimaryClusterSecondary", "PrimaryClusterReadReplica"];
const expected_default_clusterset_guideline_destinations_match = ["$.server.memberRole = PRIMARY AND $.server.clusterRole = PRIMARY","$.server.memberRole = SECONDARY AND $.server.clusterRole = PRIMARY", "$.server.memberRole = READ_REPLICA AND $.server.clusterRole = PRIMARY"];

const expected_default_clusterset_guideline_routes = ["rw", "ro"];
const expected_default_clusterset_guideline_routes_match = ["$.session.targetPort = $.router.port.rw", "$.session.targetPort = $.router.port.ro"];
const expected_default_clusterset_guideline_routes_destinations = ["round-robin(Primary)", "round-robin(PrimaryClusterSecondary), round-robin(Primary)"];

function group_fields(arr, field) {
    l = [];
    for (r in arr) {
        l.push(arr[r][field]);
    }
    return l;
}

function COMMON_RG_TESTS(rg, rg_name, expected_destinations, expected_destinations_match, expected_routes, expected_routes_match, expected_routes_destinations, expected_json, expected_check_output) {
    // rg.name()
    var name;
    EXPECT_NO_THROWS(function() { name = rg.getName(); });

    EXPECT_EQ(rg_name, name, ".name");
    EXPECT_EQ(rg_name, rg.name, ".rg.name");

    // rg.destinations()
    var default_destinations;
    EXPECT_NO_THROWS(function() { default_destinations = rg.destinations().fetchAll(); });

    EXPECT_JSON_EQ(expected_destinations, group_fields(default_destinations, 0), ".expected_destinations");
    EXPECT_JSON_EQ(expected_destinations_match, group_fields(   default_destinations, 1), ".expected_destinations_match");

    // rg.routes()
    var default_routes;
    EXPECT_NO_THROWS(function() { default_routes = rg.routes().fetchAll() });

    EXPECT_JSON_EQ(expected_routes, group_fields(default_routes, 0), ".expected_routes");
    EXPECT_JSON_EQ(expected_routes_match, group_fields(default_routes, 3), ".expected_routes_match");
    EXPECT_JSON_EQ(expected_routes_destinations, group_fields(default_routes, 4, ".expected_routes_destinations"));

    // rg.asJson()
    var json_defaults;
    EXPECT_NO_THROWS(function() { json_defaults = rg.asJson(); });

    EXPECT_JSON_EQ(expected_json, json_defaults, ".expected_json");

    // rg.show()
    EXPECT_NO_THROWS(function() { rg.show(); });

    EXPECT_OUTPUT_CONTAINS_MULTILINE(expected_check_output);
}
