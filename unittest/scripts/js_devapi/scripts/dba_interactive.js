// Assumptions: ensure_schema_does_not_exist is available
// Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
// validateMemer and validateNotMember are defined on the setup script
//@ Initialization
dba.dropMetadataSchema();

//@ Session: validating members
var members = dir(dba);

print("Session Members:", members.length);
validateMember(members, 'createCluster');
validateMember(members, 'deleteLocalInstance');
validateMember(members, 'deployLocalInstance');
validateMember(members, 'dropCluster');
validateMember(members, 'dropMetadataSchema');
validateMember(members, 'getCluster');
validateMember(members, 'help');
validateMember(members, 'killLocalInstance');
validateMember(members, 'resetSession');
validateMember(members, 'startLocalInstance');
validateMember(members, 'validateInstance');
validateMember(members, 'stopLocalInstance');

//@# Dba: createCluster errors
var Cluster = dba.createCluster();
var Cluster = dba.createCluster(5);
var Cluster = dba.createCluster('');

//@# Dba: createCluster with interaction
var Cluster = dba.createCluster('devCluster');
print(Cluster)

//@# Dba: getCluster errors
var Cluster = dba.getCluster();
var Cluster = dba.getCluster(5);
var Cluster = dba.getCluster('', 5);
var Cluster = dba.getCluster('');
var Cluster = dba.getCluster('devCluster');

//@ Dba: getCluster
print(Cluster);

//@ Dba: addInstance
Cluster.addInstance({dbUser: "root", host: "127.0.0.1", port:__mysql_port_adminapi}, "root");

//@ Dba: removeInstance
Cluster.removeInstance({host: "127.0.0.1", port:__mysql_port_adminapi});

//@# Dba: dropCluster errors
var Cluster = dba.dropCluster();
var Cluster = dba.dropCluster(5);
var Cluster = dba.dropCluster('');
var Cluster = dba.dropCluster('sample', 5);
var Cluster = dba.dropCluster('sample', {}, 5);

//@ Dba: dropCluster interaction no options, cancel
var Cluster = dba.dropCluster('sample');

//@ Dba: dropCluster interaction missing option, ok error
var Cluster = dba.dropCluster('sample', {});

//@ Dba: dropCluster interaction no options, ok success
var Cluster = dba.dropCluster('devCluster');