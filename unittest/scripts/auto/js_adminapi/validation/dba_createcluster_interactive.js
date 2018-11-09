//@ WL#12049: Initialization
||

//@ WL#12049: Unsupported server version {VER(<5.7.24)}
||Option 'exitStateAction' not supported on target server version: '<<<__version>>>'

//@ WL#12049: Create cluster errors using exitStateAction option {VER(>=5.7.24)}
||Invalid value for exitStateAction, string value cannot be empty.
||Invalid value for exitStateAction, string value cannot be empty.
||Error starting cluster: Invalid value for exitStateAction, can't be set to the value of ':'
||Error starting cluster: Invalid value for exitStateAction, can't be set to the value of 'AB'
||Error starting cluster: Invalid value for exitStateAction, can't be set to the value of '10'

//@ WL#12049: Create cluster specifying a valid value for exitStateAction (ABORT_SERVER) {VER(>=5.7.24)}
||

//@ WL#12049: Finalization
||

//@ WL#11032: Initialization
||

//@ WL#11032: Unsupported server version {VER(<5.7.20)}
||Option 'memberWeight' not available for target server version.

//@ WL#11032: Create cluster errors using memberWeight option {VER(>=5.7.20)}
||Option 'memberWeight' Integer expected, but value is String (TypeError)
||Option 'memberWeight' is expected to be of type Integer, but is Bool (TypeError)
||Option 'memberWeight' Integer expected, but value is String (TypeError)
||Option 'memberWeight' Integer expected, but Float value is out of range (TypeError)


//@ WL#11032: Create cluster specifying a valid value for memberWeight (integer) {VER(>=5.7.20)}
||

//@ WL#11032: Finalization
||

//@ WL#12067: Initialization
||

//@ WL#12067: TSF1_6 Unsupported server version {VER(<8.0.14)}
||Option 'failoverConsistency' not supported on target server version: '<<<__version>>>'

//@ WL#12067: Create cluster errors using failoverConsistency option {VER(>=8.0.14)}
||Invalid value for failoverConsistency, string value cannot be empty.
||Invalid value for failoverConsistency, string value cannot be empty.
||Error starting cluster: Invalid value for failoverConsistency, can't be set to the value of ':'
||Error starting cluster: Invalid value for failoverConsistency, can't be set to the value of 'AB'
||Error starting cluster: Invalid value for failoverConsistency, can't be set to the value of '10'
||Option 'failoverConsistency' is expected to be of type String, but is Integer (TypeError)

//@ WL#12067: TSF1_1 Create cluster using a valid as value for failoverConsistency {VER(>=8.0.14)}
||

//@ WL#12067: Finalization
||
