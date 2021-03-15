exports.end_point = function(port) {
    switch(port){
        case __mysql_sandbox_port1:
            return __endpoint1;
        case __mysql_sandbox_port2:
            return __endpoint2;
        case __mysql_sandbox_port3:
            return __endpoint3;
        case __mysql_sandbox_port4:
            return __endpoint4;
        case __mysql_sandbox_port5:
            return __endpoint5;
        case __mysql_sandbox_port6:
            return __endpoint6;
    }
};


exports.sandbox_uri = function(port) {
    switch(port){
        case __mysql_sandbox_port1:
            return __sandbox_uri1;
        case __mysql_sandbox_port2:
            return __sandbox_uri2;
        case __mysql_sandbox_port3:
            return __sandbox_uri3;
        case __mysql_sandbox_port4:
            return __sandbox_uri4;
        case __mysql_sandbox_port5:
            return __sandbox_uri5;
        case __mysql_sandbox_port6:
            return __sandbox_uri6;
    }
};

exports.sandbox = function(port) {
    switch(port){
        case __mysql_sandbox_port1:
            return __sandbox1;
        case __mysql_sandbox_port2:
            return __sandbox2;
        case __mysql_sandbox_port3:
            return __sandbox3;
        case __mysql_sandbox_port4:
            return __sandbox4;
        case __mysql_sandbox_port5:
            return __sandbox5;
        case __mysql_sandbox_port6:
            return __sandbox6;
    }
};
