#!/usr/bin/perl

use Rosetta::Templates;

use strict "vars", "subs";

my $debug_flag = shift;

my $NLG  = new Rosetta::Templates(port => 22345);
$NLG->tcp_server($debug_flag);