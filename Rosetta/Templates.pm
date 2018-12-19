#!/usr/bin/perl

# MeetingLine NLG
# [2003-04-18]  (dbohus): moved the getBestReply into Rosetta, as
#                         it does not really belong here
# [2002-11-05]	(ridy):   based on the BusLine file

package Rosetta::Templates;

@Rosetta::Templates::ISA = qw(Rosetta);

$VERSION = 0.06;

use strict;

use Rosetta;
use Rosetta::Templates::Inform;
use Rosetta::Templates::Request;
#use Rosetta::Templates::EstablishContext;
#use Rosetta::Templates::ExplicitConfirm;
#use Rosetta::Templates::ImplicitConfirm;

sub Rosetta::Templates::new {
    my $self = {};
    my $class = shift;
    my %args = @_;

    for (keys %args) {
	$self->{$_} = $args{$_};
	print "$_ = $args{$_}\n" if $self->{DEBUG};
    }
    bless $self, $class;
}

# [2003-11-07] (dbohus): changed this to do regexp matching on the object
# [2003-04-18] (dbohus): changed name to getActObject
# [2002-10-09] (dbohus): changed act/content to act/object to reflect
#                        RavenClaw
sub Rosetta::Templates::getActObject {
  my $self    = shift;
  my $act     = shift;
  my $object  = shift;
  my $key;
  if(defined($Rosetta::Templates::act{$act}->{$object})) {
    return $Rosetta::Templates::act{$act}->{$object}
  } else {
    # do regexp matching on the object
    foreach $key (keys(%{$Rosetta::Templates::act{$act}})) {
      if($object =~ /$key/) {
	print "Matched on key: $key\n";
	return $Rosetta::Templates::act{$act}->{$key};
      }
    }
  }
}

# [2003-10-07] (dbohus): changed it to do matching on regular expressions
# [2003-04-18] (dbohus): introduced getActObjectVersion to identify
#                        prompts based on act, object and version
sub Rosetta::Templates::getActObjectVersion {
    my $self    = shift;
    my $act     = shift;
    my $object  = shift;
    my $version = shift;
    my $key;
    if(!defined($Rosetta::Templates::act{$act}->{$object})) {
      # check regular expression match on the object
      foreach $key (keys(%{$Rosetta::Templates::act{$act}})) {
	if($object =~ /$key/) {
	  print "Matched on key: $key\n";
	  $object = $key;
	}
      }
    }
    if(exists($Rosetta::Templates::act{$act}->{$object}->{$version})) {
      return $Rosetta::Templates::act{$act}->{$object}->{$version};
    } else {
      return $Rosetta::Templates::act{$act}->{$object}->{"default"};
    }
}

# [2003-10-09] (dbohus): added this so that it works again given the
#                        sable changes made by antoine
sub Rosetta::Templates::getHeader {
  return "";
#  return "{?xml version=\"1.0\" encoding=\"ISO-8859-1\"?}{speak version=\"1.0\" xmlns=\"http://www.w3.org/2001/10/synthesis\"xml:lang=\"en-US\"}";


  #return q{{?xml version="1.0"?} {!DOCTYPE SABLE PUBLIC "-//SABLE//DTD SABLE speech mark up//EN" "Sable.v0_2.dtd" []} {SABLE}};
}

# [2003-10-09] (dbohus): added this so that it works again given the
#                        sable changes made by antoine
sub Rosetta::Templates::getFooter {
  return "";
  #return "{/speak}";
  #  return q{{/SABLE}};
}

# [2004-04-21] (dbohus): This is the final filter applied on the utterances in Rosetta
sub Rosetta::Templates::finalFilter {
  # this filter is applied at the end on the utterance. It contains
  # various regexp replaces which do abbreviations mostly

  my $self = shift;
  my $arg  = shift;

  my $prolong_break = " {break time=\"800ms\"/} ";
  $arg =~ s/ \.\.\.\. /$prolong_break/g;

  my $long_break = " {break time=\"600ms\"/} ";
  $arg =~ s/ \.\.\. /$long_break/g;

  my $short_break = " {break time=\"400ms\"/} ";
  $arg =~ s/ \.\. /$short_break/g;

  return $arg;
}

1;
