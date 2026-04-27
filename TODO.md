# TODO 

## Hardware

[✓] Test Solar Panel to see if working now that its wires have been soldered back onto it

[✓] Confirm Solar Panel is working

[✓] Test Microcontroller code

[] Wait for the arrival of the DS18B20 waterproof temperature sensor insha'Allah

[] Test sensor using Arduino UNO making sure it works

[] Integrate to device changing some logic to fit ESP32 needs (replaces LM35)

[] Enclosure work

## Software (website + micrcontroller code)

[✓] Make sure everything's responsive

[✓] Add devices table

[✓] Add /devices POST route

[✓] Link device to user and allow user to give nicknames to their devices

[✓] Make a Multidevice dashboard

[✓] Move health calculation to backend as maximum hardware power is sent by user (input box)

[✓] Add /api/data route for ESP32 to send to

[✓] Add /api/commands/(device_id) route for ESP32 to periodically check (whether to renew baseline or not)

[✓] Make ESP32 periodically check /api/commands/(device_id) route and send data to /api/data route correctly

[✓] Make error handling responsive (no reloads + AJAX display)

[✓] Add forgot password routing

[] Add Push notifications

[] Add Google indexing

[] Add /graphs POST route

[] Make graphs (power over time, light intensity over power, etc)

[] Split routes

[] Make everything responsive

[] Move to PostgreSQL or MySQL database
