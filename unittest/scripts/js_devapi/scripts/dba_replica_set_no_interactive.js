// Assumptions: ensure_schema_does_not_exist is available
// Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
// validateMemer and validateNotMember are defined on the setup script
dba.dropMetadataSchema({enforce:true});

var ClusterPassword = 'testing';
//@ ReplicaSet: validating members
var Cluster = dba.createCluster('devCluster', ClusterPassword);
var rset = Cluster.getReplicaSet();

var members = dir(rset);

print("Replica Set Members:", members.length);
validateMember(members, 'name');
validateMember(members, 'getName');
validateMember(members, 'addInstance');
validateMember(members, 'removeInstance');
validateMember(members, 'help');
validateMember(members, 'rejoinInstance');

//@# ReplicaSet: addInstance errors
rset.addInstance()
rset.addInstance(5,6,7,1)
rset.addInstance(5, 5)
rset.addInstance('', 5)
rset.addInstance( 5)
rset.addInstance({host: "localhost", schema: 'abs', user:"sample", authMethod:56});
rset.addInstance({port: __mysql_port_adminapi});
rset.addInstance({host: "localhost", port:__mysql_port_adminapi}, "root");

// Cleanup
dba.dropCluster('devCluster', {dropDefaultReplicaSet: true});
