# These are the INFORM acts for MyBus

# [10-17-07] (antoine) started working on this, based on the
#                        LetsGoPublic NLG
#

$Rosetta::Templates::act{"inform"} = {

    ###############################################################
    # Greetings
    ###############################################################

    # welcome to the system
    "welcome" => "Welcome to MyBus.",
 
    # quitting
    "goodbye" => 
      "Thank you for using MyBus. Goodbye.",

    # looking up database (to announce a delay)
    "looking_up_database_first" => "Just a minute. Let me check that for you.", 
    "looking_up_database_subsequent" => "Just a second.", 

    # inform the user we're starting a new query
    "starting_new_query" => "Okay let's start from the beginning.",

    ###############################################################
    # Query results
    ###############################################################

    "result" => sub {

                my %args = @_;

		my $dep_time = &convertTime($args{"result.departure_time"});
		my $arr_time = &convertTime($args{"result.arrival_time"});
		return "There is a <result.route> leaving <origin_place> at <result.departure_time $dep_time>. ".
		  "It will arrive at <destination_place> at <result.arrival_time $arr_time>.";
	      },

    "error" => sub {

                my %args = @_;
		
		my $dep_time = &convertTime($args{"result.departure_time"});
		my $arr_time = &convertTime($args{"result.arrival_time"});

		if ($args{"result.failed"} == 1) {
		  # Generic error message
		  return "I'm sorry, I could not complete your request due to an internal error.";
		} elsif ($args{"result.failed"} == 2) {
		  # No buses after given time
		  if (defined $args{"result.departure_time"}) {
		    return "There is no bus running after <result.departure_time $dep_time>.";		
		  } else {
		    if (defined $args{"result.route"}) {
		      return "Sorry, there are no more <result.route> running today.";
		    } else {
		      return "Sorry, there are no more buses running today.";
		    }
		  }
		} elsif ($args{"result.failed"} == 3) {
		  # No buses before given time (for a "PREVIOUS BUS" kind of request)
		  return "There is no bus running before <result.departure_time $dep_time>.";		
		} elsif ($args{"result.failed"} == 4) {
		  # Origin = Destination
		  return "The departure and arrival places you gave me correspond to the same stop. Please provide two different stops.";		
		} elsif ($args{"result.failed"} == 5) {
		  # No known route between the two stops
		  return "I'm sorry, I don't know any route that goes between <origin_place> and <destination_place>.";		
		}

	      },

	"vad_error" => "I'm sorry.  I didn't understand you.  Please repeat yourself."
};

sub convertTime
{
    my $hr, my $min;
    if ($_[0] =~ /(\d+)(\d\d)/)
    {
	$hr = $1;
	$min = $2;
    }
    else
    {
	$hr = 0;
	$min = $_[0];
    }

    my $suf = "a.m.";
    if ($hr > 12)
    {
        $suf = "p.m.";
	$hr -= 12;
    }
    elsif ($hr == 12)
    {
	$suf = "p.m.";
    }
    elsif ($hr == 0)
    {
	$hr = 12;
    }

    my $str;
    if ($min == 0)
    {
	$str = "$hr $suf";
    }
    else
    {
	$str = "$hr:$min $suf";
    }
    
    return $str;
}

1;