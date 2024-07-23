//Core - canbus manipulation
#include <mcp_can.h>
#include <SPI.h>

//Interface
#if !( defined(ESP8266))
  #error This code is intended to run on the ESP8266 only! Please check your Tools->Board setting.
#endif

#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <ArduinoOTA.h>

//For update Page
#include <ESP8266mDNS.h>

//For Working with Hex String
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h> // for sscanf


extern const char   html_template[];
extern const char   html_update[];
extern const char   js_main[];
extern const char   css_main[];

const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 4, 1);
DNSServer dnsServer;
ESP8266WebServer    server(80);
WiFiClient          wifiClient;
WebSocketsServer    webSocket = WebSocketsServer(8000);

//Global Variables not saved to EEPROM
int MessageRecording = 0; //When this is 1, all new messages are put into the hidden array.
int OutputDatatoUI = 1;
int OutputSerial = 0;
String Hostname = "iPhone";
String APPassword = "dudewhereismycar";
byte numhideMessages = 0; // Number of messages currently in the array
byte numblockedMessages = 0; // Number of messages currently in the array
byte numMessageReplacements = 0; // Number of message replacements currently in the array
const byte MAX_NUM_MESSAGES = 500; // Maximum number of messages in the array
const byte MESSAGE_LENGTH = 8; // Length of each message
const byte MAX_NUM_MESSAGE_REPLACEMENTS = 20; // Maximum number of message replacements in the array
const byte REPLACEMENT_MESSAGE_LENGTH = 16; // Length of each replacement message

//Arrays and variables Stored in EEPROM
int StartUIServices = 1;
byte blockedmessages[MAX_NUM_MESSAGES][MESSAGE_LENGTH]; // Array to store blocked messages
byte hidemessages[MAX_NUM_MESSAGES][MESSAGE_LENGTH]; // Array to store hidden messages
byte messageReplacements[MAX_NUM_MESSAGE_REPLACEMENTS][REPLACEMENT_MESSAGE_LENGTH] = {}; // Array to store message replacements


// Define the EEPROM start address
const int EEPROM_START_ADDR = 0;
const int EEPROM_ADDR_OUTPUT_DATATOUI = EEPROM_START_ADDR;
const int EEPROM_ADDR_OUTPUT_SERIAL = EEPROM_ADDR_OUTPUT_DATATOUI + sizeof(OutputDatatoUI);
const int EEPROM_ADDR_OUTPUT_NUM_HIDE_MESSAGES = EEPROM_ADDR_OUTPUT_SERIAL + sizeof(OutputSerial);
const int EEPROM_ADDR_OUTPUT_NUM_BLOCKED_MESSAGES = EEPROM_ADDR_OUTPUT_NUM_HIDE_MESSAGES + sizeof(int); // Assuming numhideMessages is an integer
const int EEPROM_ADDR_OUTPUT_NUM_MESSAGEREPLACMENTS = EEPROM_ADDR_OUTPUT_NUM_BLOCKED_MESSAGES + sizeof(int); // Assuming numblockedMessages is an integer
const int EEPROM_ADDR_StartUIServices = EEPROM_ADDR_OUTPUT_NUM_MESSAGEREPLACMENTS + sizeof(int); // Assuming StartUIServices is an integer
const int EEPROM_ADDR_BLOCKED_MESSAGES = EEPROM_ADDR_StartUIServices + sizeof(int); // Assuming it's an integer
const int EEPROM_ADDR_HIDDEN_MESSAGES = EEPROM_ADDR_BLOCKED_MESSAGES + MAX_NUM_MESSAGES * MESSAGE_LENGTH;
const int EEPROM_ADDR_MESSAGE_REPLACEMENTS = EEPROM_ADDR_HIDDEN_MESSAGES + MAX_NUM_MESSAGES * MESSAGE_LENGTH;

void saveglobalvariables() {
    EEPROM.put(EEPROM_ADDR_OUTPUT_DATATOUI, OutputDatatoUI);
    EEPROM.put(EEPROM_ADDR_OUTPUT_SERIAL, OutputSerial);
    EEPROM.put(EEPROM_ADDR_OUTPUT_NUM_HIDE_MESSAGES, numhideMessages);
    EEPROM.put(EEPROM_ADDR_OUTPUT_NUM_MESSAGEREPLACMENTS, numMessageReplacements);
    EEPROM.put(EEPROM_ADDR_OUTPUT_NUM_BLOCKED_MESSAGES, numblockedMessages);
    EEPROM.put(EEPROM_ADDR_StartUIServices, StartUIServices);
    EEPROM.commit();
}

void loadglobalvariables() {
    EEPROM.get(EEPROM_ADDR_OUTPUT_DATATOUI, OutputDatatoUI);
    EEPROM.get(EEPROM_ADDR_OUTPUT_SERIAL, OutputSerial);
    EEPROM.get(EEPROM_ADDR_OUTPUT_NUM_HIDE_MESSAGES, numhideMessages);
    EEPROM.get(EEPROM_ADDR_OUTPUT_NUM_MESSAGEREPLACMENTS, numMessageReplacements);
    EEPROM.get(EEPROM_ADDR_OUTPUT_NUM_BLOCKED_MESSAGES, numblockedMessages);
    EEPROM.get(EEPROM_ADDR_StartUIServices, StartUIServices);
}

void showconfiguration(){
  
  processoutputtoui("Config: "); 
  processoutputtoui("----------------------------------------------");
  String str = "Hostname: " + String(Hostname);
  processoutputtoui(str); 
  str = "APPassword: " + String(APPassword);
  processoutputtoui(str); 
  str = "Show Data UI Output: " + String(OutputDatatoUI);
  processoutputtoui(str); 
  str = "Show Data over Serial Port: " + String(OutputSerial);
  processoutputtoui(str); 
  str = "numhideMessages: " + String(numhideMessages);
  processoutputtoui(str); 
  str = "numblockedMessages: " + String(numblockedMessages);
  processoutputtoui(str); 
  str = "numMessageReplacements: " + String(numMessageReplacements);
  processoutputtoui(str); 
  str = "Start UI Services (webinterface): " + String(StartUIServices);
  processoutputtoui(str); 
  
  str = "Hiden These Messages: "; 
  
  processoutputtoui("----------------------------------------------");
}

void loadHiddenMessagesFromEEPROM() {
    for (int i = 0; i < numhideMessages; i++) {
        EEPROM.get(EEPROM_ADDR_HIDDEN_MESSAGES + i * MESSAGE_LENGTH, hidemessages[i]);
    }
}

void saveHiddenMessagesToEEPROM() {
    for (int i = 0; i < numhideMessages; i++) {
        EEPROM.put(EEPROM_ADDR_HIDDEN_MESSAGES + i * MESSAGE_LENGTH, hidemessages[i]);
        EEPROM.commit(); // Committing changes to EEPROM
    }
}

void showHiddenMessages() {
    // Read hidden messages from EEPROM
    byte hiddenMessages[MAX_NUM_MESSAGES][MESSAGE_LENGTH];
    bool isMessageHidden[MAX_NUM_MESSAGES];
    for (int i = 0; i < MAX_NUM_MESSAGES; ++i) {
        EEPROM.get(EEPROM_ADDR_HIDDEN_MESSAGES + i * MESSAGE_LENGTH, hiddenMessages[i]);
    }

    // Display hidden messages
    for (int i = 0; i < MAX_NUM_MESSAGES; ++i) {
        // Check if the message is hidden
        if (isMessageHidden[i]) {
            // Process or display the hidden message as needed
            // For example:
            processoutputtoui(reinterpret_cast<char*>(hiddenMessages[i]));
        }
    }
}

void saveBlockedMessagesToEEPROM() {
    for (int i = 0; i < numblockedMessages; i++) {
        EEPROM.put(EEPROM_ADDR_BLOCKED_MESSAGES + i * MESSAGE_LENGTH, blockedmessages[i]);
        EEPROM.commit(); // Committing changes to EEPROM
        processoutputtoui("Saved Blocked Messages to EEPROM");
    }
}

void loadBlockedMessagesFromEEPROM() {
    for (int i = 0; i < numblockedMessages; i++) {
        EEPROM.get(EEPROM_ADDR_BLOCKED_MESSAGES + i * MESSAGE_LENGTH, blockedmessages[i]);
    }
}

void saveMessageReplacementsToEEPROM() {
    for (int i = 0; i < numMessageReplacements; i++) {
        EEPROM.put(EEPROM_ADDR_MESSAGE_REPLACEMENTS + i * REPLACEMENT_MESSAGE_LENGTH, messageReplacements[i]);
        EEPROM.commit(); // Committing changes to EEPROM
    }
    // Output message
    processoutputtoui("Saved Message Replacements to EEPROM");
}

void loadMessageReplacementsFromEEPROM() {
    for (int i = 0; i < numMessageReplacements; i++) {
        EEPROM.get(EEPROM_ADDR_MESSAGE_REPLACEMENTS + i * REPLACEMENT_MESSAGE_LENGTH, messageReplacements[i]);
    }
}

void startAP(bool new_device) {

  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  //WiFi.softAP("iPhone", "dudewhereismycar");
  WiFi.softAP(Hostname.c_str(), APPassword.c_str());
  dnsServer.start(DNS_PORT, "*", apIP);
}

void sendDeviceMessage(String msg) {
  //Prints Commands from web interface to serial port
  Serial.print(msg);
  Serial.print("\r\n");
}

void startServer() {

  server.on("/",            handleMain);
  server.on("/update.html",      handleUpdate);
  server.on("/main.js",     handleMainJS);
  server.on("/main.css",    handleMainCSS);
  server.onNotFound(handleNotFound);
  server.on("/favicon.ico", handleNotFound);
  server.begin();
}

void handleMain() {

  server.sendHeader("Cache-Control", "public, max-age=120, immutable");
  server.send_P(200, "text/html", html_template );
}

void handleUpdate() {

  server.sendHeader("Cache-Control", "public, max-age=120, immutable");
  server.send_P(200, "text/html", html_update );
}

void handleMainJS() {
  server.sendHeader("Cache-Control", "public, max-age=120, immutable");
  server.send_P(200, "text/javascript", js_main);
}

void handleMainCSS() {

  server.sendHeader("Cache-Control", "public, max-age=120, immutable");
  server.send_P(200, "text/css", css_main);
}

void handleNotFound() {

  server.send(404,   "text/html", "<html><body><p>404 Error</p></body></html>" );
}

/* ############################ tools ############################################# */


void sendRawDataOverSocket(const char * msgc, int len) {

  if ( strlen(msgc) == 0 or len == 0  ) return;
  StaticJsonDocument<400> console_data;
  console_data["type"] = "console";
  console_data["content"] = msgc;
  char   b[400];
  size_t lenn = serializeJson(console_data, b);  // serialize to buffer
  webSocket.broadcastTXT(b, lenn);
}

void processoutputtoui(String command) {
  String str = command;
  int str_len = str.length() + 1;
  char char_array[str_len];
  str.toCharArray(char_array, str_len);
  sendRawDataOverSocket(char_array, sizeof(char_array));
}


void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {

  switch (type) {
    case WStype_TEXT: {
        StaticJsonDocument<300> socket_data;
        DeserializationError error = deserializeJson(socket_data, payload);
        if (error) {
          StaticJsonDocument<70> error_data;
          error_data["type"] = "error";
          error_data["content"] = "error parsing the last message";
          char   b[70]; // create temp buffer
          size_t len = serializeJson(error_data, b);  // serialize to buffer
          webSocket.sendTXT(num, b);
          break;
        }
        if (socket_data["action"] == "console") {
          sendDeviceMessage(socket_data["content"]);
          String str = socket_data["content"];
          processconsolecommand(str);
        }
      }
      break;
    default:
      break;
  }
}
void sendErrorOverSocket(uint8_t num, const char * msg) {

  StaticJsonDocument<100> error_message;
  error_message["type"] = "error";
  error_message["msg"] = msg;
  char   b[100];
  size_t len = serializeJson(error_message, b);  // serialize to buffer
  if (num != 255) webSocket.sendTXT(num, b);
  else webSocket.broadcastTXT(b, len);
}

unsigned long rxId;
byte len;
byte rxBuf[8];

byte txBuf0[] = {0xAA,0x55,0xAA,0x55,0xAA,0x55,0xAA,0x55};
byte txBuf1[] = {0x55,0xAA,0x55,0xAA,0x55,0xAA,0x55,0xAA};
char msgString[128];                        // Array to store serial string

MCP_CAN CAN0(D1);                              // CAN0 interface usins CS on digital pin 2
MCP_CAN CAN1(D3);                               // CAN1 interface using CS on digital pin 3

#define CAN0_INT D4    //define interrupt pin for CAN0 recieve buffer
#define CAN1_INT D2    //define interrupt pin for CAN1 recieve buffer




void setup() {
  EEPROM.begin(44716);
  Serial.begin(115200);
  
  //Check if flash is new, new flashes have -1 for thiw value, if so don't load from EEPROM and put OutputDatatoUI = 0
  EEPROM.get(EEPROM_ADDR_OUTPUT_DATATOUI, OutputDatatoUI);
  if (OutputDatatoUI == -1) {
    OutputDatatoUI = 0;
    OutputSerial = 0;
    OutputDatatoUI = 0;
    Hostname = "iPhone";
    
    APPassword = "dudewhereismycar";
    StartUIServices = 1;    
  } else {
    loadglobalvariables();
    loadHiddenMessagesFromEEPROM();
    loadBlockedMessagesFromEEPROM();
    loadMessageReplacementsFromEEPROM();
    processoutputtoui("Config Loaded from EEPROM!");
  }
  

  startAP(true);

  // For update Web Page
  //---------------------------------------
  MDNS.begin(Hostname);
    
    server.on("/update", HTTP_POST, []() {
      server.sendHeader("Connection", "close");
      server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
      ESP.restart();
    }, []() {
      HTTPUpload& upload = server.upload();
      if (upload.status == UPLOAD_FILE_START) {
        Serial.setDebugOutput(true);
        WiFiUDP::stopAll();
        Serial.printf("Update: %s\n", upload.filename.c_str());
        uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
        if (!Update.begin(maxSketchSpace)) { //start with max available size
          Update.printError(Serial);
        }
      } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
          Update.printError(Serial);
        }
      } else if (upload.status == UPLOAD_FILE_END) {
        if (Update.end(true)) { //true to set the size to the current progress
          Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
        } else {
          Update.printError(Serial);
        }
        Serial.setDebugOutput(false);
      }
      yield();
    });
    MDNS.addService("http", "tcp", 80);
    //--------------------------------------------------------------------------

  startServer();
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  pinMode(CAN0_INT, INPUT_PULLUP);
  pinMode(CAN1_INT, INPUT_PULLUP);
  
  // init CAN0 bus, baudrate: 250k@16MHz
  if(CAN0.begin(MCP_ANY, CAN_500KBPS, MCP_16MHZ) == CAN_OK) {
    Serial.print("CAN0: Init OK!\r\n");
    CAN0.setMode(MCP_NORMAL);
  } else {
    Serial.print("CAN0: Init Fail!!!\r\n");
  }
  
  // init CAN1 bus, baudrate: 250k@16MHz
  if(CAN1.begin(MCP_ANY, CAN_500KBPS, MCP_16MHZ) == CAN_OK) {
    Serial.print("CAN1: Init OK!\r\n");
    CAN1.setMode(MCP_NORMAL);
  } else {
    Serial.print("CAN1: Init Fail!!!\r\n");
  }
  
  SPI.setClockDivider(SPI_CLOCK_DIV2);         // Set SPI to run at 8MHz (16MHz / 2 = 8 MHz)
  
  //timer.every(60000, print_message);
}

bool checkAndRecordMessage(byte rxBuf[]) {
  // Check if the message already exists in the hidemessages array
  for (int i = 0; i < numhideMessages; i++) {
    bool match = true;
    for (int j = 0; j < MESSAGE_LENGTH; j++) {
      // Print the byte count and the byte being compared
      Serial.print("Byte ");
      Serial.print(j + 1);
      Serial.print(": ");
      Serial.print(rxBuf[j], HEX);
      Serial.print(" vs ");
      Serial.println(hidemessages[i][j], HEX);

      // Compare the byte of the received message with the byte of the stored message
      if (rxBuf[j] != hidemessages[i][j]) {
        match = false;
        break;
      }
    }
    // Print whether the match is true or false
    if (match) {
      processoutputtoui("Message Already There");
      return true;
    }
  }

  // If the message does not exist in the array, add it
  if (numhideMessages < MAX_NUM_MESSAGES) {
    for (int i = 0; i < MESSAGE_LENGTH; i++) {
      hidemessages[numhideMessages][i] = rxBuf[i];
    }
    numhideMessages++;
    processoutputtoui("Added Message");
    return false;
  } else {
    // hidemessages array is full, unable to add message
    Serial.println("hidemessages array is full");
    return false;
  }
}

void loop() {  
  server.handleClient();
  MDNS.update();
  webSocket.loop();

  if(!digitalRead(CAN0_INT)) { // If interrupt pin is low, read CAN1 receive buffer
    CAN0.readMsgBuf(&rxId, &len, rxBuf);       // Read data: len = data length, buf = data byte(s)
    
    if (!shouldBlockTransmission(rxBuf)) {
      CAN1.sendMsgBuf(rxId, 1, len, rxBuf);
      
      if (!shouldHideTransmission(rxBuf)) {
        if(OutputDatatoUI == 1) {
          printWebMessage(rxId, len, rxBuf, "A");
        }
        
        if(OutputSerial == 1) {
          printMessage(rxId, len, rxBuf);
        }
      }
      
    }
    else {

      if(OutputDatatoUI == 1) {
        printblockedWebMessage(rxId, len, rxBuf, "A");
      }
      
      if(OutputSerial == 1) {
        printblockedMessage(rxId, len, rxBuf);
      }

    }
  }
  
  if(!digitalRead(CAN1_INT)) { // If interrupt pin is low, read CAN1 receive buffer
    CAN1.readMsgBuf(&rxId, &len, rxBuf);       // Read data: len = data length, buf = data byte(s)
        
    //Lookups and transforms here:
    replaceMessage(rxBuf);
    
    if (!shouldBlockTransmission(rxBuf)) {
      CAN0.sendMsgBuf(rxId, 1, len, rxBuf);
      
      if(MessageRecording == 1) {
        checkAndRecordMessage(rxBuf);
      }

      if (!shouldHideTransmission(rxBuf)) {
        if(OutputDatatoUI == 1) {
          printWebMessage(rxId, len, rxBuf, "B");
        }
        
        if(OutputSerial == 1) {
          printMessage(rxId, len, rxBuf);
        }
      }
    }
    else {
    
      if(OutputDatatoUI == 1) {
        printblockedWebMessage(rxId, len, rxBuf, "B");
      }
      
      if(OutputSerial == 1) {
        printblockedMessage(rxId, len, rxBuf);
      }

    }
  }
}

void replaceMessage(byte* receivedMessage) {
    // Iterate through the replacement array
    for (int i = 0; i < MAX_NUM_MESSAGE_REPLACEMENTS; i++) {
        byte* originalMessage = messageReplacements[i];
        byte* replacementMessage = messageReplacements[i] + MESSAGE_LENGTH;

        // Compare the first MESSAGE_LENGTH bytes of the received message with the original message
        if (memcmp(receivedMessage, originalMessage, MESSAGE_LENGTH) == 0) {
            // If the received message matches the original message, replace it with the replacement message
            memcpy(receivedMessage, replacementMessage, MESSAGE_LENGTH);
            return; // Exit the function once replacement is done
        }
    }
    // If no replacement is found, do nothing
}

byte* parseHexString(const char* hexString) {
    int length = strlen(hexString);
    int byteCount = (length + 1) / 3; // Each byte is represented as "XX " (3 characters)
    byte* byteArray = (byte*)malloc(byteCount);
    if (byteArray == NULL) {
        // Memory allocation failed
        return NULL;
    }

    const char* ptr = hexString;
    for (int i = 0; i < byteCount; i++) {
        // Convert each byte from hexadecimal string to byte
        char byteStr[3] = {ptr[0], ptr[1], '\0'};
        sscanf(byteStr, "%hhx", &byteArray[i]);
        ptr += 3; // Move pointer to next byte
        if (ptr[0] == '\0') // Check if end of string is reached
            break;
    }

    return byteArray;
}

void showMessageReplacements() {
    for (int i = 0; i < numMessageReplacements; i++) {
        String originalMessage = byteArrayToString(messageReplacements[i], REPLACEMENT_MESSAGE_LENGTH / 2);
        String replacementMessage = byteArrayToString(messageReplacements[i] + REPLACEMENT_MESSAGE_LENGTH / 2, REPLACEMENT_MESSAGE_LENGTH / 2);
        
        String message = "Original: " + originalMessage + "  ->  Replacement: " + replacementMessage;
        
        // Output to UI
        processoutputtoui(message.c_str());

        // Output to Serial
        Serial.println(message);
    }
}

void clearMessageReplacements() {
    memset(messageReplacements, 0, sizeof(messageReplacements)); // Reset all elements of the array to zero
    numMessageReplacements = 0; // Reset the count of replacements
}

void printMessage(unsigned long id, byte length, byte* data) {
  char idString[20];
  sprintf(idString, "ID: %lx", id);
  Serial.print(idString);
  Serial.print(" DLC: ");
  Serial.print(length);
  Serial.print("Data (Hex):");
  for (byte i = 0; i < length; i++) {
    if (data[i] < 0x10) {
      Serial.print("0"); // Ensure leading zero for single-digit hex values
    }
    Serial.print(data[i], HEX);
    Serial.print(" ");
  }
  
  Serial.print("Data (ASCII):");
  for (byte i = 0; i < length; i++) {
    if (isPrintable(data[i])) {
      Serial.write(data[i]); // Print ASCII character if printable
    } else {
      Serial.print("."); // Print dot for non-printable characters
    }
  }
  Serial.println();
}

void printWebMessage(unsigned long id, byte length, byte* data, String CAN) {
  char idString[50];
  String str = "CAN" + CAN + " -> ";
  String temp;
  
  sprintf(idString, " ID: %lx", id);
  str = str + idString;
  sprintf(idString, " DLC: %lx", length);
  str = str + idString;
  str = str + " Data (Hex):";
  for (byte i = 0; i < length; i++) {
    if (data[i] < 0x10) {
      str = str + "0"; // Ensure leading zero for single-digit hex values
    }
    str = str + String(data[i], HEX);
    str = str + " ";
  }
  str = str + "Data (ASCII):";
  for (byte i = 0; i < length; i++) {
    if (isPrintable(data[i])) {
      char asciiValue = (char)data[i];
      str += asciiValue;
    } else {
      str = str + "."; // Print dot for non-printable characters
    }
  }
  processoutputtoui(str);
}

void printblockedMessage(unsigned long id, byte length, byte* data) {
  char idString[20];
  sprintf(idString, "BLOCKED! - ID: %lx", id);
  Serial.print(idString);
  Serial.print(" DLC: ");
  Serial.print(length);
  Serial.print("Data (Hex):");
  for (byte i = 0; i < length; i++) {
    if (data[i] < 0x10) {
      Serial.print("0"); // Ensure leading zero for single-digit hex values
    }
    Serial.print(data[i], HEX);
    Serial.print(" ");
  }
  
  Serial.print("Data (ASCII):");
  for (byte i = 0; i < length; i++) {
    if (isPrintable(data[i])) {
      Serial.write(data[i]); // Print ASCII character if printable
    } else {
      Serial.print("."); // Print dot for non-printable characters
    }
  }
  Serial.println();
}

void printblockedWebMessage(unsigned long id, byte length, byte* data, String CAN) {
  char idString[50];
  String str = "BLOCKED! - CAN" + CAN + " -> ";
  String temp;
  
  sprintf(idString, " ID: %lx", id);
  str = str + idString;
  sprintf(idString, " DLC: %lx", length);
  str = str + idString;
  str = str + " Data (Hex):";
  for (byte i = 0; i < length; i++) {
    if (data[i] < 0x10) {
      str = str + "0"; // Ensure leading zero for single-digit hex values
    }
    str = str + String(data[i], HEX);
    str = str + " ";
  }
  str = str + "Data (ASCII):";
  for (byte i = 0; i < length; i++) {
    if (isPrintable(data[i])) {
      char asciiValue = (char)data[i];
      str += asciiValue;
    } else {
      str = str + "."; // Print dot for non-printable characters
    }
  }
  processoutputtoui(str);
}


// Function to add a message to the block array using string
// e.g const char* inputString = "01 00 00 00 00 00 00 00";
//     addMessage(inputString);
void addblockedMessage(const char* hexString) {
    if (numblockedMessages < MAX_NUM_MESSAGES) {
        byte* message = parseHexString(hexString);
        if (message != NULL) {
            // Check if the message is already in the array
            bool messageExists = false;
            for (int i = 0; i < numblockedMessages; i++) {
                if (memcmp(blockedmessages[i], message, MESSAGE_LENGTH) == 0) {
                    messageExists = true;
                    break;
                }
            }
            if (!messageExists) {
                // Message doesn't exist in the array, so add it
                memcpy(blockedmessages[numblockedMessages], message, MESSAGE_LENGTH);
                numblockedMessages++;
                printf("Message added to the blocked list.\n");
                processoutputtoui("Message added to the blocked list.\n");
            } else {
                printf("Message already exists in the blocked list.\n");
                processoutputtoui("Message already exists in the blocked list.\n");
            }
            free(message); // Free allocated memory
        } else {
            printf("Failed to add message to blocked list. Memory allocation error.\n");
            processoutputtoui("Failed to add message to blocked list. Memory allocation error.\n");
        }
    } else {
        printf("Cannot add message. Array is full.\n");
        processoutputtoui("Cannot add message. Array is full.\n");
    }
}

//     addMessage(inputString);
void addhideMessage(const char* hexString) {
    if (numhideMessages < MAX_NUM_MESSAGES) {
        byte* message = parseHexString(hexString);
        if (message != NULL) {
            // Check if the message already exists in the hidemessages array
            bool messageExists = false;
            for (int i = 0; i < numhideMessages; i++) {
                if (memcmp(hidemessages[i], message, MESSAGE_LENGTH) == 0) {
                    messageExists = true;
                    break;
                }
            }

            // If the message doesn't already exist, add it to the array
            if (!messageExists) {
                memcpy(hidemessages[numhideMessages], message, MESSAGE_LENGTH);
                numhideMessages++;
                printf("Message added to the hide list.\n");
                processoutputtoui("Message added to the hide list.\n");
            } else {
                printf("Message already exists in the hide list.\n");
                processoutputtoui("Message already exists in the hide list.\n");
            }
            free(message); // Free allocated memory
        } else {
            printf("Failed to add message to hide list. Memory allocation error.\n");
            processoutputtoui("Failed to add message to hide list. Memory allocation error.\n");
        }
    } else {
        printf("Cannot add message. Hide list is full.\n");
        processoutputtoui("Cannot add message. Hide list is full.\n");
    }
}

// Function to clear all messages from the array
void clearblockedMessages() {
  memset(blockedmessages, 0, sizeof(blockedmessages)); // Reset all elements of the array to zero
  numblockedMessages = 0;
  Serial.println("All blocked messages cleared from the array.");
  processoutputtoui("All blocked messages cleared from the array.");
}

// Function to clear all messages from the array
void clearhideMessages() {
  memset(hidemessages, 0, sizeof(hidemessages)); // Reset all elements of the array to zero
  numhideMessages = 0;
  Serial.println("All Hidden messages cleared from the array.");
  processoutputtoui("All Hidden messages cleared from the array.");
}

void addMessageReplacement(byte* replacementMessage) {
    if (numMessageReplacements < MAX_NUM_MESSAGE_REPLACEMENTS) {
        memcpy(messageReplacements[numMessageReplacements], replacementMessage, REPLACEMENT_MESSAGE_LENGTH);
        numMessageReplacements++;
        Serial.println("Replacement message added to the array.");
    } else {
        Serial.println("Cannot add replacement message. Array is full.");
    }
}

bool shouldBlockTransmission(byte* message) {
  for (int i = 0; i < numblockedMessages; i++) {
    bool shouldBlock = true;
    for (int j = 0; j < MESSAGE_LENGTH; j++) {
      if (message[j] != blockedmessages[i][j]) {
        shouldBlock = false;
        break;
      }
    }
    if (shouldBlock) {
      return true; // Block transmission if message matches
      processoutputtoui("Message Blocked");
    }
  }
  return false; // Allow transmission if message doesn't match
}

bool shouldHideTransmission(byte* message) {
  for (int i = 0; i < numhideMessages; i++) {
    bool shouldHide = true;
    for (int j = 0; j < MESSAGE_LENGTH; j++) {
      if (message[j] != hidemessages[i][j]) {
        shouldHide = false;
        break;
      }
    }
    if (shouldHide) {
      return true; // Hide transmission if message matches
      //processoutputtoui("Message Hidden");
    }
  }
  return false; // Allow Record of Transmission in UI
}

void displayBlockList() {
  for (int i = 0; i < numblockedMessages; i++) {
    String blockedmessageStr = "Blocked Message " + String(i + 1) + ": ";
    for (int j = 0; j < MESSAGE_LENGTH; j++) {
      char hex[3];
      sprintf(hex, "%02X ", blockedmessages[i][j]);
      blockedmessageStr += hex;
    }
    processoutputtoui(blockedmessageStr);
  }
}

void displayHideList() {
  for (int i = 0; i < numhideMessages; i++) {
    String hidemessageStr = "Hide Message from UI: " + String(i + 1) + ": ";
    for (int j = 0; j < MESSAGE_LENGTH; j++) {
      char hex[3];
      sprintf(hex, "%02X ", hidemessages[i][j]);
      hidemessageStr += hex;
    }
    processoutputtoui(hidemessageStr);
  }

}

String byteArrayToString(byte* byteArray, int length) {
    String result = "";
    for (int i = 0; i < length; i++) {
        if (byteArray[i] < 0x10) {
            result += "0"; // Ensure leading zero for single-digit hex values
        }
        result += String(byteArray[i], HEX);
        if (i < length - 1) {
            result += " ";
        }
    }
    return result;
}

/*
int OutputDatatoUI = 1; //0 = None, 1 = All, 2 = CAN-A/0, 3 = CAN-B/1
int OutputSerial = 0;// 0 = No ouput, 1 = output to serial port as well.
*/
void processconsolecommand(String command) {
    
  String str = "Console Input: " + command;
  int str_len = str.length() + 1;
  char char_array[str_len];
  str.toCharArray(char_array, str_len);

  if (command.startsWith("Block ")) {
    // Extract the message part after "Block "
    String messageString = command.substring(6);
    const char* message = messageString.c_str();
    byte* byteArray = parseHexString(message);
    if (byteArray != NULL) {
      addblockedMessage(message); // Add the message to the block list
      String finaloutputMessage = messageString + ": Added To Block List";
      processoutputtoui(finaloutputMessage);
            
    } else {
      String finaloutputMessage = messageString + ": Could not be added to the block list";
      processoutputtoui(finaloutputMessage);
    }
  }

   
  else if (command.startsWith("Hide ")) {
    // Extract the message part after "Block "
    String messageString = command.substring(5);
    const char* message = messageString.c_str();
    byte* byteArray = parseHexString(message);
    if (byteArray != NULL) {
      addhideMessage(message); // Add the message to the block list
      String finaloutputMessage = messageString + ": Added To Hide List";
      processoutputtoui(finaloutputMessage);
    } else {
      String finaloutputMessage = messageString + ": Could not be added to the hide list";
      processoutputtoui(finaloutputMessage);
    }
  }
  else if (command == "Show Block List") {
    // Display the current block list
    displayBlockList();
  }
  else if(command == "Clear Block Message List") {
    //Empties Block List
    clearblockedMessages();
  }
  else if (command == "Show Replace List") {
    // Display the current block list
    showMessageReplacements();
  }
  else if(command == "Clear Replace Message List") {
    //Empties Block List
    clearMessageReplacements();
  }
    else if (command == "Show Hide List") {
    // Display the current block list
    displayHideList();
  }
  else if(command == "Clear Hide Message List") {
    //Empties Block List
    clearhideMessages();
  }
  else if(command == "Disable Serial Output") {
    OutputSerial = 0;
  }
  else if(command == "Enable Serial Output") {
    OutputSerial = 1;
  }

  else if(command == "Start Recording") {
    MessageRecording = 1;
  }
  else if(command == "Stop Recording") {
    MessageRecording = 0;
  }
  else if(command == "AutoUI: On") {
    StartUIServices = 1;
    EEPROM.put(EEPROM_ADDR_StartUIServices, StartUIServices);
    EEPROM.get(EEPROM_ADDR_StartUIServices, StartUIServices);
  }
  else if(command == "AutoUI: Off") {
    StartUIServices = 0;
    EEPROM.put(EEPROM_ADDR_StartUIServices, StartUIServices);
    EEPROM.get(EEPROM_ADDR_StartUIServices, StartUIServices);
  }


  

  else if(command == "Disable UI Output") {
    OutputDatatoUI = 0;
  }
  else if(command == "Enable UI Output") {
    OutputDatatoUI = 1;
  }
  else if(command == "Restart") {
    saveBlockedMessagesToEEPROM();
    saveMessageReplacementsToEEPROM();
    saveHiddenMessagesToEEPROM();
    saveglobalvariables();
    processoutputtoui("Rebooting Now!!");
    ESP.restart();
  }
  else if(command == "Show Configuration") {
    showconfiguration();
  }
  
  

  if (command.startsWith("REPLACE ")) {
    // Extract the original and replacement messages from the command
    String messageString = command.substring(8);
    int separatorIndex = messageString.indexOf(" to ");
    if (separatorIndex != -1) {
      String originalMessage = messageString.substring(0, separatorIndex);
      String replacementMessage = messageString.substring(separatorIndex + 4);

      // Parse original and replacement messages into byte arrays
      byte* originalByteArray = parseHexString(originalMessage.c_str());
      byte* replacementByteArray = parseHexString(replacementMessage.c_str());

      if (originalByteArray != NULL && replacementByteArray != NULL) {
        // Combine original and replacement byte arrays into one array
        byte combinedArray[REPLACEMENT_MESSAGE_LENGTH];
        memcpy(combinedArray, originalByteArray, REPLACEMENT_MESSAGE_LENGTH / 2);
        memcpy(combinedArray + REPLACEMENT_MESSAGE_LENGTH / 2, replacementByteArray, REPLACEMENT_MESSAGE_LENGTH / 2);

        // Add combined array as a replacement
        addMessageReplacement(combinedArray);

        processoutputtoui("Replacement message added: " + byteArrayToString(combinedArray, REPLACEMENT_MESSAGE_LENGTH));
        
        free(originalByteArray);
        free(replacementByteArray);
      } else {
        processoutputtoui("Failed to add replacement message. Invalid hexadecimal format.");
      }
    } 
  } else if (command == "Clear Replacement Message List") {
    clearMessageReplacements();
  }
  sendRawDataOverSocket(char_array, sizeof(char_array));
}

bool validateString(String str, int len) {
  // Check if the string contains any spaces
  if (str.indexOf(' ') != -1) {
    Serial.println("String cannot contain spaces.");
    return false;
  }
  
  // Check if the string length is at least 6 characters
  if (str.length() < len) {
    Serial.println("String must be at least " + String(6) + " characters long.");
    return false;
  }

  // String meets all criteria
  return true;
}
