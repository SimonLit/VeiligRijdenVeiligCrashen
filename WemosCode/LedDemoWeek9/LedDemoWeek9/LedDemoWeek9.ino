#include <ESP8266WiFi.h>

const char* ssid = "Project";
const char* password = "123456780";

int ledPin = D3;
WiFiServer server(90);

void setup() {
  Serial.begin(115200);
  
   pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);

  // Connect to WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  // If the router isn't available for use comment this while loop.
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    }
  Serial.println("");
  Serial.println("WiFi connected");

  // Start the server
  server.begin();
  Serial.println("Server started");

  // Print the IP address
  Serial.print("Use this URL : ");
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.println("/");

}

void loop() {
 espTestWithLed();
}

void espTestWithLed(void)
{
  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) {
    return;
  }

  // Wait until the client sends some data
  //Serial.println("new client");
  while (!client.available()) {
    delay(1);
  }

  // Read the first line of the request
  String request = client.readStringUntil('\r');
  //Serial.println(request);
  client.flush();

  // Match the request

  int value = LOW;
  if (request.indexOf("/LED=ON") != -1) {
    digitalWrite(ledPin, HIGH);
    value = HIGH;
    client.println("LED is set On\r");
  }
  if (request.indexOf("/LED=OFF") != -1) {
    digitalWrite(ledPin, LOW);
    value = LOW;
    client.println("LED is set Off\r");
  }

  
  
  // Return the response
  /*client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println(""); //  do not forget this one
  client.println("<!DOCTYPE HTML>");
  client.println("<html>");

  client.print("Led pin is now: ");

  if (value == HIGH) {
    client.print("On");
  } else {
    client.print("Off");
  }
  client.println("<br><br>");
  client.println("Click <a href=\"/LED=ON\">here</a> turn the LED on pin 5 ON<br>");
  client.println("Click <a href=\"/LED=OFF\">here</a> turn the LED on pin 5 OFF<br>");
  client.println("<br><br>");
  client.println("Connect to the dash, to prevent a crash.");
  client.println("</html>");

  delay(1);
  Serial.println("Client disconnected");
  Serial.println("");*/                                       
}