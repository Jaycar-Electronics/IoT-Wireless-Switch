/************************************************************

  Complete project details at http://randomnerdtutorials.com  

  v1.0 9 April 2018
    
************************************************************** */

// Load Wi-Fi library
#include <ESP8266WiFi.h>

#define OUTPIN D5
#define SHORTPULSE 316
#define LONGPULSE 818
#define sw1_on    0     // This is the referece index in rccmds[]
#define sw1_off   1
#define sw2_on    2
#define sw2_off   3

const byte rfcmds[]={0xF,0xE,0xD,0xC,0xB,0xA,0x7,0x6,0x4,0x8};      //nocmd, 1 on/off,2 on/off,3 on/off,4 on/off,all on/off
const unsigned long address=0x12340;                                //this could be any 20bit value (not all tested)

// Replace with your network credentials
const char* ssid     = " your WiFi Router SSID here";
const char* password = " your WiFi password here ";



// Set web server port number to 80
WiFiServer server(80);

// Variable to store the HTTP request
String header;

// Auxiliar variables to store the current output state
String output_1_State = "off";
String output_2_State = "off";


void setup() {
  pinMode(OUTPIN,OUTPUT);   //pin for 433MHz data
  
  output_1_State = "off";
  output_2_State = "off";
  
  Serial.begin(115200);

  // Connect to Wi-Fi network with SSID and password
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();
}

void loop(){
  WiFiClient client = server.available();   // Listen for incoming clients

  if (client) {                             // If a new client connects,
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
            
            // turns the wireless switches on and off
            if (header.indexOf("GET /s1/on") >= 0) {
              Serial.println("Switch 1 on");
              output_1_State = "on";
              sendrf(packet(address,rfcmds[sw1_on]));  // send command to RF receiver
            } else if (header.indexOf("GET /s1/off") >= 0) {
              Serial.println("Switch 1 off");
              sendrf(packet(address,rfcmds[sw1_off]));  // send command to RF receiver
              output_1_State = "off";
            } else if (header.indexOf("GET /s2/on") >= 0) {
              Serial.println("Switch 2 on");
              sendrf(packet(address,rfcmds[sw2_on]));  // send command to RF receiver
              output_2_State = "on";
            } else if (header.indexOf("GET /s2/off") >= 0) {
              Serial.println("Switch 2 off");
              sendrf(packet(address,rfcmds[sw2_off]));  // send command to RF receiver
              output_2_State = "off";
            }
            
            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS to style the on/off buttons 
            // Feel free to change the background-color and font-size attributes to fit your preferences
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".button { background-color: #195B6A; border: none; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
            client.println(".button2 {background-color: #77878A;}</style></head>");
            
            // Web Page Heading
            client.println("<body><h2>Jaycar</h2>");
            client.println("<h1>IoT Smart Wireless Switch</h1>");
            client.println("<h2> </h2>");
            
            // Display current state, and ON/OFF buttons for Switch 1  
            client.println("<p>Bedroom Lights - " + output_1_State + "</p>");
            // If the output_1_State is off, it displays the ON button       
            if (output_1_State=="off") {
              client.println("<p><a href=\"/s1/on\"><button class=\"button\">ON</button></a></p>");
            } else {
              client.println("<p><a href=\"/s1/off\"><button class=\"button button2\">OFF</button></a></p>");
            } 
               
            // Display current state, and ON/OFF buttons for Switch 2  
            client.println("<p>Kitchen Lights - " + output_2_State + "</p>");
            // If the output_2_State is off, it displays the ON button       
            if (output_2_State=="off") {
              client.println("<p><a href=\"/s2/on\"><button class=\"button\">ON</button></a></p>");
            } else {
              client.println("<p><a href=\"/s2/off\"><button class=\"button button2\">OFF</button></a></p>");
            }
            client.println("</body></html>");
            
            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}

unsigned long packet(unsigned long a,byte c){     //takes address, command, calculates crc and sends it
  unsigned long p;
  byte cx;
  p=((a&0xFFFFF)<<4)|(c&0xF);
  cx=rfcrc(p);
  p=(p<<8)|cx;
  return p;
}

byte rfcrc(unsigned long d){                      //calculate crc
  byte a,b,c;
  a=reverse(d>>16);
  b=reverse(d>>8);
  c=reverse(d);
  return reverse(a+b+c);
}


byte reverse(byte d){                             //reverse bit order in byte
  return ((d&0x80)>>7)|((d&0x40)>>5)|((d&0x20)>>3)|((d&0x10)>>1)|((d&0x08)<<1)|((d&0x04)<<3)|((d&0x02)<<5)|((d&0x01)<<7);
}

void sendrf(unsigned long int k){          //send a raw packet
  unsigned long int i;
  for(int r=0;r<20;r++){                   //do repeats-- do more if success rate is low
    for(i=0x80000000UL;i>0;i=i>>1){        //32 bits of sequence
      if(i&k){                             //hi bit
        digitalWrite(OUTPIN,HIGH);
        delayMicroseconds(LONGPULSE);
        digitalWrite(OUTPIN,LOW);
        delayMicroseconds(SHORTPULSE);
      }else{                               //lo bit
        digitalWrite(OUTPIN,HIGH);
        delayMicroseconds(SHORTPULSE);
        digitalWrite(OUTPIN,LOW);
        delayMicroseconds(LONGPULSE);      
      }
    }
    digitalWrite(OUTPIN,HIGH);        //3 more lo bits
    delayMicroseconds(SHORTPULSE);
    digitalWrite(OUTPIN,LOW);
    delayMicroseconds(LONGPULSE);      
    digitalWrite(OUTPIN,HIGH);
    delayMicroseconds(SHORTPULSE);
    digitalWrite(OUTPIN,LOW);
    delayMicroseconds(LONGPULSE);      
    digitalWrite(OUTPIN,HIGH);
    delayMicroseconds(SHORTPULSE);
    digitalWrite(OUTPIN,LOW);
    delayMicroseconds(LONGPULSE);      
    delayMicroseconds(8000);          //brief delay between repeats
  }
}


