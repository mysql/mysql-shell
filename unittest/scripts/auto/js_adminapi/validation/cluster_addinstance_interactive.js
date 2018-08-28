//@ Initialization
||

//@ Create cluster 1 {VER(>=5.7.24)}
||

//@ addInstance() errors using exitStateAction option {VER(>=5.7.24)}
||Invalid value for exitStateAction, string value cannot be empty.
||Invalid value for exitStateAction, string value cannot be empty.
||Error joining instance to cluster: Invalid value for exitStateAction, can't be set to the value of ':'
||Error joining instance to cluster: Invalid value for exitStateAction, can't be set to the value of 'AB'
||Error joining instance to cluster: Invalid value for exitStateAction, can't be set to the value of '10'

//@ Add instance using a valid exitStateAction 1 {VER(>=5.7.24)}
||

//@ Finalization
||
