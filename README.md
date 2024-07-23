Car21 Auto Canbus sniffer (or MITM)

Used to sniff your canbus and inject can messages. The module can act as MITM (Man-in-the-Middle). This can block messages, transform messages as well as a nice UI to manage messages. 

WHY build this:
I built this to give my car features I can't get otherwise. E.g When I go over 25km/h, I want the doors to automatically lock. The other things is my car is a PHEV, plug in hybrid. 
When my battery is above 50%, I want the car to automatically switch to EV move when I start it. Lastly the dash doesn't display directions or what song is playing like my sons Nissan Leaf, it
would be cool to add that in as well. Using my ODB adapter and diagnostic equipment, I can capture the necessary messages that make text scroll on the dashboard and mimic that.

Features:
 *Has CAN1 (A) and CAN2 (B). Allows to be setup as MITM. Tested and Working on ODB Port only so far.
 *UI show all CAN Messages from both interfaces, and can be paused and cleared at the push of a button.
 *Can disable Serial and UI CAN output.
 *Has ability to transform, or block messages between CAN Busses.
 *All config stored in EEPROM, in case of power failure.
 *Feature Rich WebUI.
 *Because its built for automotive, Sets up AP called "iPhone", to remain hidden :).
 *Firmware can be updated, by uploading a newly compiled binary in web interface.
 *Recording mode to hide known messages to assist in finding specific message.
 *Commands to hide, transform or block messages all done by entering commands in the UI.


UI features:
 *Web UI with buttons to access featured quickly. However you need to type in commands, manual to come!!
 *Recording feature. This will record all messages to be hidden from UI. E.g. lets say you want to trace a message about a button on your dash. 
  1) Start recording to the module can add all current messages to the Ignore List. 
  2) After a minute or so, turn recording off. Then enable output to UI, all the messages in the hidden list should be hidden away and you won't see them.
  3) Push the button in your dash or access what ever you want in the car, and disable the UI. You should see a much smaller amount of messages in the UI. 


Issues to be resolved / Features to add:
  1) Wifi, need to add STA as well. So STA (Wifi Client) then AP fallback (Setup local Access Point)
  2) Improve UI for Mobile
  3) Need to fix MITM for my car, Mitsubishi Eclipse Cross PHEV 2022. MITM beteen ECU and Combination Cluster
  4) Manual injection of message/s to Specific CAN interface
  5) Others to be named Later


Arduino Library Requirements:
 *WebSerial (Search in Library Manager)
 *MCP2515 Library: https://github.com/coryjfowler/MCP_CAN_lib

 Hardware:
  *ESP8266 Board (I used NodeMCU), but you can use any generic ESP8266 with SPI interface and 4 available IO's
  *2x MCP2515, cheep CANBUS Transceivers (Currently testing with 8hz transceivers, suspect I need 16Hz ones to fix MITM)
  *Depending on your car, it would be an advantage to use an extension cable for where you want to fit the device. These can be found on Aliexpress or Temu.
