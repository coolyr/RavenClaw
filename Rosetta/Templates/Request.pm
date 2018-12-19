# These are the REQUEST acts for MyBus

# [10-17-07] (antoine) - started this from the LetsGoPublic Request.pm

$Rosetta::Templates::act{"request"} = {

  ##############################################################################
  # Task-specific requests
  ##############################################################################

  # request nothing (just wait for the user to say something)
  "nothing" => "...",
				   
  # ask if the user wants to make another query
  "next_query" => "You can say, when is the next bus, when is the ".
		  "previous bus, start a new query, or goodbye.",

  # request the departure place
  "origin_place" => "Where are you leaving from?",

  # request the arrival place
  "destination_place" => "Where are you going?",
    
};

