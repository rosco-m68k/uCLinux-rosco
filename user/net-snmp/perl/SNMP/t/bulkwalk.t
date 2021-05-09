#!/usr/local/bin/perl
#
# $Id: bulkwalk.t,v 5.1.2.1 2003/02/25 18:10:03 rstory Exp $
#
# Test bulkwalk functionality.
use Data::Dumper;

BEGIN {
    unless(grep /blib/, @INC) {
        chdir 't' if -d 't';
        @INC = '../lib' if -d '../lib';
    }
}
use Test;
BEGIN { $num = 62; plan test => $num; }

use SNMP;

require "t/startagent.pl";

use vars qw($agent_port $comm2 $agent_host);

$SNMP::debugging = 0;
$SNMP::verbose = 0;

#print "1..$num\n";

######################################################################
# Fire up a session.
$s1 = new SNMP::Session(
    'DestHost'   => $agent_host,
    'Community'  => $comm2,
    'RemotePort' => $agent_port,
    'Version'    => '2c',
    'UseNumeric' => 1,
    'UseEnum'    => 0,
    'UseLongNames' => 1
);
ok(defined($s1));

######################################################################
# 
# Attempt to use the bulkwalk method to get a few variables from the
# SNMP agent.
# test 1
$vars = new SNMP::VarList ( ['sysUpTime'], ['ifNumber'], # NON-repeaters
			    ['ifSpeed'], ['ifDescr']);	 # Repeated variables.

$expect = scalar @$vars;
@list = $s1->bulkwalk(2, 16, $vars);

ok($s1->{ErrorNum} == 0);

# Did we get back the list of references to returned values?
#
ok(scalar @list == $expect);

# Sanity check the returned values.  list[0] is sysUptime nonrepeater.
ok($list[0][0]->tag eq ".1.3.6.1.2.1.1.3");	# check system.sysUptime OID
ok($list[0][0]->iid eq "0");			# check system.sysUptime.0 IID
ok($list[0][0]->val =~ m/^\d+$/);		# Uptime is numeric 
ok($list[0][0]->type eq "TICKS");		# Uptime should be in ticks.

# Find out how many interfaces to expect.  list[1] is ifNumber nonrepeater.
ok($list[1][0]->tag eq ".1.3.6.1.2.1.2.1");	# Should be system.ifNumber OID.
ok($list[1][0]->iid eq "0");			# system.ifNumber.0 IID.
ok($list[1][0]->val =~ m/^\d+$/);		# Number is all numeric 
#XXX: test fails due SMIv1 codes being returned intstead of SMIv2...
#ok($list[1][0]->type eq "INTEGER32");		# Number should be integer.

$ifaces = $list[1][0]->val;

# Make sure we got an ifSpeed for each interface.  list[2] is ifSpeed repeater.
ok(scalar @{$list[2]} == $ifaces);

# Make sure we got an ifDescr for each interface.  list[3] is ifDescr repeater.
ok(scalar @{$list[3]} == $ifaces);

# Test for reasonable values from the agent.
ok($list[2][0]->tag eq ".1.3.6.1.2.1.2.2.1.5");	# Should be system.ifSpeed OID.
ok($list[2][0]->iid eq "1");			# Instance should be 1.
ok($list[2][0]->val =~ m/^\d+$/);		# Number is all numeric 
ok($list[2][0]->type eq "GAUGE");		# Number should be a gauge.

ok($list[3][0]->tag eq ".1.3.6.1.2.1.2.2.1.2");	# Should be system.ifDescr OID.
ok($list[3][0]->iid eq "1");			# Instance should be 1.

# The first interface is probably loopback.  Check this.
ok($list[3][0]->type eq "OCTETSTR");		# Description is a string.

# This might fail for some weird (Windows?) systems.  Can be safely ignored.
$loopback = $list[3][0]->val;
ok(($loopback =~ /^lo/));

###############################################################################
# Attempt to use the bulkwalk method to get only non-repeaters
# test 2
$vars = new SNMP::VarList ( ['sysUpTime'], ['ifNumber'] ); # NON-repeaters

$expect = scalar @$vars;
@list = $s1->bulkwalk(2, 16, $vars);
ok($s1->{ErrorNum} == 0);

# Did we get back the list of references to returned values?
#
ok(scalar @list == $expect);

# Sanity check the returned values.  list[0] is sysUptime nonrepeater.
ok($list[0][0]->tag eq ".1.3.6.1.2.1.1.3");	# check system.sysUptime OID
ok($list[0][0]->iid eq "0");			# check system.sysUptime.0 IID
ok($list[0][0]->val =~ m/^\d+$/);		# Uptime is numeric 
ok($list[0][0]->type eq "TICKS");		# Uptime should be in ticks.

# Find out how many interfaces to expect.  list[1] is ifNumber nonrepeater.
ok($list[1][0]->tag eq ".1.3.6.1.2.1.2.1");	# Should be system.ifNumber OID.
ok($list[1][0]->iid eq "0");			# system.ifNumber.0 IID.
ok($list[1][0]->val =~ m/^\d+$/);		# Number is all numeric 
#XXX: test fails due SMIv1 codes being returned intstead of SMIv2...
#ok($list[1][0]->type eq "INTEGER32");		# Number should be integer.
$ifaces = $list[1][0]->val;


###############################################################################
# Attempt to use the bulkwalk method to get only repeated variables
# test 3
$vars = new SNMP::VarList ( ['ifIndex'], ['ifSpeed'] ); # repeaters

$expect = scalar @$vars;
@list = $s1->bulkwalk(0, 16, $vars);
ok($s1->{ErrorNum} == 0);

# Did we get back the list of references to returned values?
#
ok(scalar @list == $expect);

# Make sure we got an ifIndex for each interface.  list[0] is ifIndex repeater.
ok(scalar @{$list[0]} == $ifaces);

# Make sure we got an ifSpeed for each interface.  list[0] is ifSpeed repeater.
ok(scalar @{$list[1]} == $ifaces);

# Test for reasonable values from the agent.
ok($list[0][0]->tag eq ".1.3.6.1.2.1.2.2.1.1");	# Should be system.ifIndex OID.
ok($list[0][0]->iid eq "1");			# Instance should be 1.
ok($list[0][0]->val =~ m/^\d+$/);		# Number is all numeric 
#XXX: test fails due SMIv1 codes being returned intstead of SMIv2...
#ok($list[0][0]->type eq "INTEGER32");		# Number should be an integer.

ok($list[1][0]->tag eq ".1.3.6.1.2.1.2.2.1.5");	# Should be system.ifSpeed OID.
ok($list[1][0]->iid eq "1");			# Instance should be 1.
ok($list[1][0]->val =~ m/^\d+$/);		# Number is all numeric 
ok($list[1][0]->type eq "GAUGE");		# Number should be a gauge.

######################################################################
#  Asynchronous Bulkwalk Methods
######################################################################
# 
# Attempt to use the bulkwalk method to get a few variables from the
# SNMP agent.
# test 4
sub async_cb1 {
    my ($vars, $list) = @_;
    ok(defined $list && ref($list) =~ m/ARRAY/);
    ok(defined $vars && ref($vars) =~ m/SNMP::VarList/);

    ok(scalar @$list == scalar @$vars);

    my $vbr;

    # Sanity check the returned values.  First is sysUptime nonrepeater.
    $vbr = $list->[0][0];
    ok($vbr->tag eq ".1.3.6.1.2.1.1.3");	# check system.sysUptime OID
    ok($vbr->iid eq "0");			# check system.sysUptime.0 IID
    ok($vbr->val =~ m/^\d+$/);			# Uptime is numeric 
    ok($vbr->type eq "TICKS");			# Uptime should be in ticks.

    # Find out how many interfaces to expect.  Next is ifNumber nonrepeater.
    $vbr = $list->[1][0];
    ok($vbr->tag eq ".1.3.6.1.2.1.2.1");	# Should be system.ifNumber OID.
    ok($vbr->iid eq "0");			# system.ifNumber.0 IID.
    ok($vbr->val =~ m/^\d+$/);			# Number is all numeric 
#XXX: test fails due SMIv1 codes being returned intstead of SMIv2...
#    ok($vbr->type eq "INTEGER32");		# Number should be integer.
    $ifaces = $vbr->[2];

    # Test for reasonable values from the agent.
    ok(scalar @{$list->[2]} == $ifaces);
    $vbr = $list->[2][0];
    ok($vbr->tag eq ".1.3.6.1.2.1.2.2.1.5");	# Should be ifSpeed OID
    ok($vbr->iid eq "1");			# Instance should be 1.
    ok($vbr->val =~ m/^\d+$/);			# Number is all numeric 
    ok($vbr->type eq "GAUGE");			# Should be a gauge.

    ok(scalar @{$list->[3]} == $ifaces);
    $vbr = $list->[3][0];
    ok($vbr->tag eq ".1.3.6.1.2.1.2.2.1.2");	# Should be ifDescr OID
    ok($vbr->iid eq "1");			# Instance should be 1.

    # The first interface is probably loopback.  Check this.
    ok($vbr->type eq "OCTETSTR");

    # This might fail for some weird (Windows?) systems.  Can be safely ignored.
    ok(($vbr->val =~ /^lo/));

    SNMP::finish();
}

$vars = new SNMP::VarList ( ['sysUpTime'], ['ifNumber'], # NON-repeaters
			    ['ifSpeed'], ['ifDescr']);	 # Repeated variables.

@list = $s1->bulkwalk(2, 16, $vars, [ \&async_cb1, $vars ] );
ok($s1->{ErrorNum} == 0);
SNMP::MainLoop();
ok(1);
