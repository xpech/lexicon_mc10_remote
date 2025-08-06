#include "lexicon.h"
#include "LittleFS.h" 
#include <SoftwareSerial.h>

EspSoftwareSerial::UART lexiconSerial;

#define LEXICON_RX IO_18 // RX pin for Lexicon
#define LEXICON_TX IO_19 // TX pin for Lexicon

// last command status
int lexiconError = 0;
extern ESP8266WebServer server; // Pointer to the server instance

/*
	Each transmission by the RC is the following format:
<St> <Zn> <Cc> <Dl> <Data> <Et>
	St (Start transmission): 0x21 ‘!’
	Zn (Zone number): see below.
	Cc (Command code): the code for the command
	Dl (Data length): the number of data items following this item,excluding the ETR
	Data: the parameters for the command
	Et (End transmission): 0x0D


	Each response by the AVR is the following format::
	<St> <Zn> <Cc> <Ac> <Dl> <Data> <Et>
	St (Start transmission): 0x21 ‘!’
	Zn (Zone number): see below.
	Cc (Command code): the code for the command
	Ac (Answer code): see below.
	Dl (Data Length): the number of data items following this item, excluding the ETR
	Data: the parameters for the response of length n. n is limited to 255.
	Et (End transmission): 0x0D

*/
char *sendCommand(int zone, int command, char *data)
{
	lexiconError = 0; // Reset error code

	lexiconSerial.write(0x21);			// Start transmission
	lexiconSerial.write(zone);			// Zone number
	lexiconSerial.write(command);		// Command code
	lexiconSerial.write(strlen(data)); // Data length
	lexiconSerial.print(data);			// Data
	lexiconSerial.write(0x0D);			// End transmission
	lexiconSerial.flush();				// Ensure all data is sent

	// Wait for response
	while (lexiconSerial.available() == 0)
	{
		delay(10); // Wait for data to be available
	}
	int responseCode = lexiconSerial.read(); // Read the first byte of the response
	if (responseCode != 0x21)
	{
		// Invalid response, return NULL
		lexiconError = 1; // Set error code for bad response
		return "Bad response code";
	}
	int responseZone = lexiconSerial.read();	 // Read the zone number from the response
	int responseCommand = lexiconSerial.read(); // Read the command code from the response
	int responseLength = lexiconSerial.read();	 // Read the data length from the response
	if (responseLength <= 0 || responseLength > 255)
	{
		// Invalid data length, return NULL
		lexiconError = 1; // Set error code for bad response
		return "Invalid data length";
	}
	char responseData[256]; // Buffer for response data
	int bytesRead = 0;
	while (bytesRead < responseLength && lexiconSerial.available() > 0)
	{
		responseData[bytesRead++] = lexiconSerial.read(); // Read each byte of data
	}
	responseData[bytesRead] = '\0'; // Null-terminate the string
	if (bytesRead != responseLength)
	{
		// Not enough data read, return NULL
		lexiconError = 1; // Set error code for bad response
		return "Not enough data read";
	}
	// Check if the response is valid
	if (responseZone != zone || responseCommand != command)
	{
		// Zone or command mismatch, return NULL
		lexiconError = 1; // Set error code for bad response
		return "Zone or command mismatch";
	}
	return responseData;
}

const char *sendCommandRC5(int zone, int command1, int command2)
{
	lexiconSerial.write(0x21);		// Start transmission
	lexiconSerial.write(zone);		// Zone number
	lexiconSerial.write(0x08);		// Command code for RC5 (not used)
	lexiconSerial.write(0x02);		// Data length (2 bytes for RC5 command)
	lexiconSerial.write(command1); // Command code
	lexiconSerial.write(command2); // Data length
	lexiconSerial.write(0x0D);		// End transmission
	lexiconSerial.flush();			// Ensure all data is sent
	// Wait for response
	while (lexiconSerial.available() == 0)
	{
		delay(10); // Wait for data to be available
	}
	/*
	RESPONSE:
	Byte: Description:
	St 0x21
	Zn Zone number
	Cc 0x08
	Ac Answer code
	Dl 0x02
	Data1 RC5 System code
	Data2 RC5 Command code
	Et 0x0D*/

	int responseCode = lexiconSerial.read(); // Read the first byte of the response
	if (responseCode != 0x21)
	{
		// Invalid response, return NULL
		lexiconError = 1; // Set error code for bad response
		return "RC5 Bad response code";
	}
	int responseZone = Serial.read();	 // Read the zone number from the response
	int responseCommand = Serial.read(); // Read the command code from the response
	int responseLength = Serial.read();	 // Read the data length from the response
	if (responseLength != 2)
	{
		// Invalid data length, return NULL
		lexiconError = 1; // Set error code for bad response
		return "Invalid data length";
	}
	int rc5SystemCode = Serial.read();	// Read the first byte of RC5 command
	int rc5CommandCode = Serial.read(); // Read the second byte of RC5 command
	if (responseZone != zone || responseCommand != 0x08)
	{
		// Zone or command mismatch, return NULL
		lexiconError = 1; // Set error code for bad response
		return "Zone or command mismatch";
	}
	char *result = new char[256];
	// sprintf(result, "%02X%02X", rc5SystemCode, rc5CommandCode);
	return "OK";
}

int lexiconSetup()
{
	// Initialize the server
	server.on("/lexicon", HTTP_GET, handleLixiconIndex);
	server.on("/lexicon_cmd", HTTP_GET, handleLixiconCommand);
	server.on("/lexicon_rc5", HTTP_GET, handleLixiconRC5Command);
	server.begin(); // Start the server

	// Initialize the SoftwareSerial for Lexicon communication
	lexiconSerial.begin(38400,SWSERIAL_8N1, LEXICON_RX, LEXICON_TX, false); // Set baud rate to 9600
	lexiconSerial.setTimeout(1000); // Set timeout for reading data
	lexiconError = 0; // Reset error code
	return 0; // Return success
}

void handleLixiconIndex()
{
	SPIFFS.begin(); // Ensure SPIFFS is mounted
	// Serve the lexicon HTML file

  	server.setContentLength(CONTENT_LENGTH_UNKNOWN);

  	server.send(200, F("text/html"), "");
	File f = SPIFFS.open("/lexicon.hml", "r"); // Open the HTML file from SPIFFS
	if (f.isFile())
	{
		String content = f.readString(); // Read the file content
		server.sendContent(content); // Send the content to the client
		f.close(); // Close the file
	}
	else
	{
		server.send(404, F("text/plain"), F("File not found")); // Send 404 if file not found
	}
	/*
<html>
<head>
<title>Lexicon Index</title>
</head>
<body>
<h1>Lexicon Index</h1>
<div id="Output"></div>
<button date-type="RC5" data-command1="0x10" data-command2="0x0C">On/Off</button>
<button date-type="RC5" data-command1="0x10" data-command2="0x10">Volume +</button>
<button date-type="RC5" data-command1="0x10" data-command2="0x11">Volume -</button>
<script>
<!--

</p>
</body>

	*/
}

int getIntFromHexArg(const String &argName)
{
	if (server.hasArg(argName))
	{
		String argValue = server.arg(argName);
		const char *arg = argValue.c_str(); // Get the command string as a C-style string
		return strtol(arg, 0, 16);
	}
	return 0; // Default value if argument is not present
}

void handleLixiconCommand()
{
	int zone = getIntFromHexArg("zone"); // Get the zone number from the request
	int command = getIntFromHexArg("command"); // Get the zone number from the request
	String data = server.arg("command"); // Get the zone number from the request
	const char* res = sendCommand( zone,  command, (char*)(data.c_str())); // Send the command to the Lexicon device
	server.send(200, F("text/plain"), res); // Send success response
}

void handleLixiconRC5Command()
{
	int zone = getIntFromHexArg("zone"); // Get the zone number from the request
	int command1 = getIntFromHexArg("command1"); // Get the zone number from the request
	int command2 = getIntFromHexArg("command2"); // Get the zone number from the request
	String data = server.arg("command"); // Get the zone number from the request
	const char* res = sendCommandRC5( zone,  command1, command2); // Send the command to the Lexicon device
	server.send(200, F("text/plain"), res); // Send success response
}