#include <ESP8266WiFi.h>

WiFiServer server(80);
IPAddress IP(192, 168, 9, 9);
IPAddress mask = (255, 255, 255, 0);

int count = 0;
String msg = "0";
String command;  // command send to clinet

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_AP);
  WiFi.softAP("wtfff", "11223344");
  WiFi.softAPConfig(IP, IP, mask);
  server.begin();
  WiFi.softAPIP();
}

void loop() {
	
  // read command from python
  if (Serial.available()) {
    command = Serial.readString();
  }
  
  // wait for a client to connect (Gets a client that is connected to the server and has data available for reading)
  WiFiClient client = server.available();
	
// if command is valid
  if (command.length() > 0){
      Serial.print("");
	  
      // send command to client 
	  // client side has delay while listening
      client.println(command + '\r');
      command = "";
      count++;
      if (count > 10){ 
        command = "";
        count = 0;
      }
  }
 
   // if a client
  if (client)
  {
    // the client connected to the server
    while (client.connected())
    {  
      // client send some message
      if (client.available())
      {
        // read the message from client until the end character
        String line = client.readStringUntil('\r');

        // if a valid command
        if (line.length() > 0) {
          
		  Serial.print("");
          // print the message to python
          Serial.println(line); // + ';' at end
          client.flush();
		  
          // ack the client
          client.println(msg + "\r");
		  Serial.print("");
          break;
        }
      }
    }
    delay(1); 
  }
}


