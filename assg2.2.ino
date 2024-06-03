#include <ESP8266WiFi.h>       // Include the WiFi library for the ESP8266
#include <ESP8266WebServer.h>  // Include the WebServer library for the ESP8266
#include <EEPROM.h>            // Include the EEPROM library for storing data

ESP8266WebServer server(80);   // Create a web server object that listens on port 80

const int LED_PIN = 2;         // Define the pin where the LED is connected (D4 on NodeMCU)
const int EEPROM_SIZE = 512;   // Define the size of the EEPROM
// Define the starting addresses for storing WiFi credentials, device ID, and output status in EEPROM
const int WIFI_SSID_ADDR = 0;
const int WIFI_PASS_ADDR = 64;
const int DEVICE_ID_ADDR = 128;
const int OUTPUT_STATUS_ADDR = 192;

String ssid, password, deviceID; // Variables to store WiFi credentials and device ID
bool outputStatus = false;       // Variable to store the LED status

void setup() {
  pinMode(LED_PIN, OUTPUT);      // Set the LED pin as output
  Serial.begin(115200);          // Start serial communication at 115200 baud rate
  EEPROM.begin(EEPROM_SIZE);     // Initialize EEPROM with the specified size

  // Read stored WiFi credentials, device ID, and output status from EEPROM
  ssid = readStringFromEEPROM(WIFI_SSID_ADDR);
  password = readStringFromEEPROM(WIFI_PASS_ADDR);
  deviceID = readStringFromEEPROM(DEVICE_ID_ADDR);
  outputStatus = EEPROM.read(OUTPUT_STATUS_ADDR);

  // If no WiFi credentials are found, start AP mode
  if (ssid.length() == 0 || password.length() == 0) {
    startAPMode();
  } else {
    connectToWiFi(); // Otherwise, try to connect to the stored WiFi
  }

  setupWebServer(); // Setup the web server

  // Set the LED to the last saved status
  if (outputStatus) {
    digitalWrite(LED_PIN, HIGH);
  } else {
    digitalWrite(LED_PIN, LOW);
  }
}

void loop() {
  server.handleClient(); // Handle incoming client requests
}

void startAPMode() {
  Serial.println("Starting AP mode...");
  WiFi.softAP("ESP8266_Config"); // Start the ESP8266 in AP mode with SSID "ESP8266_Config"

  IPAddress IP = WiFi.softAPIP(); // Get the IP address of the ESP8266 in AP mode
  Serial.print("AP address: ");
  Serial.println(IP); // Print the IP address

  Serial.println("AP mode started");
}

void connectToWiFi() {
  Serial.print("Connecting to ");
  Serial.println(ssid); // Print the SSID being connected to

  WiFi.begin(ssid.c_str(), password.c_str()); // Start connecting to the WiFi network

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) { // Try to connect for 20 attempts
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected to WiFi"); // Print success message
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP()); // Print the IP address assigned by the router
  } else {
    Serial.println("\nConnection failed. Starting AP mode."); // Print failure message
    startAPMode(); // Start AP mode if connection fails
  }
}

void setupWebServer() {
  server.on("/", HTTP_GET, handleRoot); // Define the handler for the root URL (GET request)
  server.on("/", HTTP_POST, handlePost); // Define the handler for form submission (POST request)
  server.begin(); // Start the web server
  Serial.println("HTTP server started");
}

void handleRoot() {
  // HTML form for inputting WiFi credentials, device ID, and LED status
  String html = "<html><body><h1>ESP8266 Configuration</h1>"
                "<form action='/' method='POST'>"
                "SSID: <input type='text' name='ssid'><br>"
                "Password: <input type='password' name='password'><br>"
                "Device ID: <input type='text' name='deviceID'><br>"
                "Output Status: <input type='radio' name='outputStatus' value='ON'>ON "
                "<input type='radio' name='outputStatus' value='OFF'>OFF<br>"
                "<input type='submit' value='Save'>"
                "</form></body></html>";
  server.send(200, "text/html", html); // Send the HTML form to the client
}

void handlePost() {
  ssid = server.arg("ssid"); // Get the SSID from the form
  password = server.arg("password"); // Get the password from the form
  deviceID = server.arg("deviceID"); // Get the device ID from the form
  String outputStatusStr = server.arg("outputStatus"); // Get the output status from the form

  // Set the output status based on the form input
  if (outputStatusStr == "ON") {
    outputStatus = true;
  } else {
    outputStatus = false;
  }

  // Save the WiFi credentials, device ID, and LED status to EEPROM
  writeStringToEEPROM(WIFI_SSID_ADDR, ssid);
  writeStringToEEPROM(WIFI_PASS_ADDR, password);
  writeStringToEEPROM(DEVICE_ID_ADDR, deviceID);
  EEPROM.write(OUTPUT_STATUS_ADDR, outputStatus);
  EEPROM.commit(); // Commit the changes to EEPROM

  server.send(200, "text/plain", "Configuration saved. Restarting..."); // Send a response to the client
  delay(2000);
  ESP.restart(); // Restart the ESP8266 to apply the changes
}

String readStringFromEEPROM(int startAddr) {
  String data; // String to store the read data
  char c; // Character to read from EEPROM
  for (int i = startAddr; i < EEPROM_SIZE; i++) {
    c = EEPROM.read(i); // Read a byte from EEPROM
    if (c == 0) {
      break; // Stop reading if a null character is found
    }
    data += c; // Append the read character to the string
  }
  return data; // Return the read string
}

void writeStringToEEPROM(int startAddr, const String &data) {
  for (int i = 0; i < data.length(); i++) {
    EEPROM.write(startAddr + i, data[i]); // Write each character to EEPROM
  }
  EEPROM.write(startAddr + data.length(), 0); // Add a null terminator
}
