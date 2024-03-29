# Coop-Door
3.01 02/04/22 testing
- removes webserver info
- adds Apple Home Kit connection with homeSpan
- moves causeCode definition to integer definition instead of in setup


2.09 12/24/20 deployed
- adds motor timer to prevent from running over 30 seconds (average up/down time = 23 seconds)
- adds cause code for motor overdrive. Reference 2.05 - 11/22/2020
- moves secret information over to secureSettings.h file - located in libraries folder
- changed wifi up button html code from button press #4 to button press #9, in an attempt to see if this is the issue with the door opening on it's own...
	- learned that the code somewhat stops processing when there is a wifi connection established, thus added the next comment
- adds client.stop() for each of the 3 pages that load based on button pressed (open/stop/close)
- adds debugging info for the motor monitor
- adds setup cause code. Reference 2.05 - 11/22/2020 for number
- reordered if statements for web page reactions - put the up at the bottom

2.08 11/28/20
- changed to seperate ThingSpeak channel to help seperate the amount of data being sent
- version 2.07 had thingspeak commented out when loaded - had that commented out becuase of testing, just forgot to uncomment...
- attempting to send cause code text instead of a code to help with tweeting to understand without having to open readme.md file "causeCodeText"
- adds thingSpeakOff to allow for easier turning tweeting off/on


2.07 11/27/20
- changed web page to not have bi-directional buttons for open/stop/close. still use the feedback to the text above the buttons
- made autoOpenOn a volatile variable
- removes refresh on web page
	- this was to help prevent the random raising/closing doors that was happening

2.06 11/23/20
-  ambientLightSensorLevel from master to control WiFi connected LED
- sets format of timeStamp by adding a dot between minutes and seconds
- standarized serial monitor header with coop control
- changed version from signed long to a float - this allows the decimal point in the variable for serial monitor header

2.05 - 11/22/2020
- LED for wireless active 
- time stamp for door open/close
- thingspeak communication
- cause codes implementation
		25   = Setup cause code
		100 = master says raise door 
		125 = button says up 
		150 = wifi says raise coop door 
		175 = raise section, none of the above 
		200 = master says lower door 
		225 = button says down 
		250 = wifi says lower coop door 
		275 = lower section, none of the above 
		300 = motor running too long
- removed comments for setting static IP:
		// Set IP address
		IPAddress local_IP(10, 4, 20, 240);                   // production IP address
		//IPAddress local_IP(192, 168, 151, 247);                   // development IP address
		IPAddress gateway(10, 4, 20, 1);
		IPAddress subnet(255, 255, 255, 0);
		IPAddress primaryDNS(8, 8, 8, 8);
		IPAddress secondaryDNS(8, 8, 4, 4);
- information for time formatting:
		 https://en.cppreference.com/w/c/chrono/strftime
		 %A = Full weekday name
		 %a = Abbreviated weekday name
		 %B = Full month name
		 %m = Month as decimal number
		 %b = Abbreviated month name
		 %d = Day of the month
		 %0d = zero based Day of the month
		 %j = Day of the year
		 %U = Week of the year (sunday first day of week)
		 %Y = 4 digit year
		 %y = 2 digit year
		 %H = Hour in 24h format
		 %I = Hour in 12h format
		 %M = Minute
		 %S = Second

2.04 = 11/30/19 - adds override buttons inside box to move motor in case the rope gets caught
2.03 = 10/9/19 adds automated button to turn off reception of strings from master (for the really cold days we want the chickens to stay inside)
2.02 = 10/6/19 implemented. Still need to complete master code, but all other is working.
2.01 = completly cleared out all coop control stuff except wifi and communication with Master start again!
1.00 = implemented still need to test communication from master (haven't implemented Master code yet)
0.03 = adds communication with Mega Master
0.02 = adds web control, lost control from master - need to go 232... - not fully implemented yet
0.01 = not implemented, but the control from master is working along with the sensors
