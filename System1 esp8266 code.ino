#include <ESP8266WiFi.h>

String messageSendToServer;
int messageID = 0;
boolean tag = false;
char ssid[] = "wtfff";
char pass[] = "11223344";
String clientRec = "Client receive the message!!!";

IPAddress server(192, 168, 9, 9);
WiFiClient client;

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
  }
  // "9" indicates connected to server in WIFI package
  Serial.println("9");
}

void loop() {
  client.connect(server, 80);

  // send data to server
  if (Serial.available()) {

    // recFromMega means RX from mega sensors 
    String recFromMega = Serial.readString();

    // send the message to the server
    client.println(recFromMega + '\r');

    // wait for server send back ack knowledge
    String answer = client.readStringUntil('\r');

    // for successful get ack back from server side (answer should be equal to 0)
    Serial.println(answer);

    // clean buffer prevent bouncing
    client.flush();

  }
  
  delay(100);
  
  // listen command send from server side 
  if (client.available()) {
    delay(50);
	
    // read server command
    String fromServer = client.readStringUntil('\r');

    // if command is valid 
    if (fromServer.length() > 0){

      // tell mega thought TX what command is
       Serial.println(fromServer);

        // ack the server client receive message
        // client.println(clientRec + '\r');   
        
        client.flush();
      }
  }
}
