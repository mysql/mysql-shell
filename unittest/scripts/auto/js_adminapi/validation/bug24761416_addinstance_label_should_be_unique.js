//@ create cluster on first instance
||

//@ add second instance with label
|ONLINE|

//@ third instance is valid for cluster usage
|"status": "ok"|

//@ add third instance with duplicated label
||An instance with label '1node1' is already part of this InnoDB cluster (ArgumentError)

//@ third instance is still valid for cluster usage
|"status": "ok"|

//@ add third instance with unique label
|ONLINE|
