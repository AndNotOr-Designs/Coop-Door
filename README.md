# Coop-Door

2.07 11/24/20
- changed web page to not have bi-directional buttons for open/stop/close. still use the feedback to the text above the buttons

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
		100 = master says raise door 
		125 = button says up 
		150 = wifi says raise coop door 
		175 = raise section, none of the above 
		200 = master says lower door 
		225 = button says down 
		250 = wifi says lower coop door 
		275 = lower section, none of the above 
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
