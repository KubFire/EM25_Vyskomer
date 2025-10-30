// Load Wi-Fi library
#include <WiFi.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Wire.h>

// Replace with your network credentials
const char* ssid     = "ESP32-Access-Point";
const char* password = "123456789";
#define SEALEVELPRESSURE_HPA (1013.25)

// Set web server port number to 80
WiFiServer server(80);

// Variable to store the HTTP request
String header;
char currentLine[128] = {0};  // Zero-init to start empty
int pos = 0;                  // Position in buffer
bool firstLine = true;  // Process only the first line
long BMP_AvgTemperature;
long BMP_AvgPressure;
long BMP_RawAltitude;

const uint8_t LOOKBACK   = 5;   // how many points must be lower than the peak
float altBuffer[LOOKBACK + 1];
uint8_t bufIdx = 0;          // points to the *oldest* entry
bool bufferFull = false;

long BMP_Temperature;
long BMP_Pressure;
long BMP_Altitude;
long BMP_Correction;
long BMP_DefaultPressure;
long BMP_PastAltitude;
int BMP_Counter;
long BMP_SmoothAltitude;
int FlightCount = 1;
int AltitudeCurrent;
int Altitude;
bool GoTo;
bool PastGoTo;
bool Press;
int i= 5;
bool ascent = true;
bool PastPress = false;
long Presstime;
bool MaxAlt = false;
Adafruit_BME280 bme; // I2C
float SmoothFactor = 0.4;
float apogeeAltitude = 0.0;
float BMP_MaxAltitude = 0;
int presscount = 0;
int flightnum = 0;
char StringMaxAltitude[5][10];
float MaxAltitudeBuffer[5];
char poradi[3];

void setup() {
  Serial.begin(115200);
  // Initialize the output variables as outputs

  // Connect to Wi-Fi network with SSID and password
  Serial.print("Setting AP (Access Point)…");
  // Remove the password parameter, if you want the AP (Access Point) to be open
  WiFi.softAP(ssid, password);

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
  server.begin();
  bme.begin();
  pinMode(14, INPUT_PULLUP);


}


void loop() {
  //----------------------------------------------LOOP------------------------------
 Press = digitalRead(14);
  if(Press == 0){
    BMP_sim();
    DataSmooth(BMP_RawAltitude);
    Max_Alt();
    Presstime = millis();
  }

  if(GoTo == 1){
    //Serial.println("Goto 1");
    if(GoTo != PastGoTo){
      Serial.println("Ready for next Launch");

      BMP_MaxAltitude = 0;
      BMP_SmoothAltitude = 0;
      BMP_PastAltitude = 0;
      BMP_RawAltitude = 0;
      ascent = true;
      MaxAlt = false;
      flightnum++;
      if(flightnum>5){
        flightnum = 5;
        
      }
      i=4;

    }
    Serial.println("Measuring..");
    BMP_sim();
    DataSmooth(BMP_RawAltitude);
    Serial.println(BMP_SmoothAltitude);
    Max_Alt();
  }
  else{

  }
  PastGoTo = GoTo;


//-------------------------------------------------WEBSERVER-------------------------------

  WiFiClient client = server.available();

  if (client) {
    Serial.println("New Client.");

    // Reset state
    currentLine[0] = '\0';
    pos = 0;
    firstLine = true;
    //--------------------------------------WEBSERVER-REQUEST-HANDLING----------------------------
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Serial.write(c);

        if (c == '\n') { //ta '\n' je vzdy na konci radku, takze hleda konec radku
          currentLine[pos] = '\0'; //prida do stringu ukonceni 
          if(firstLine == true) { //dekodovani prvniho radku requestu, az ho precte uz sem nejde
              Serial.print("Request: ");
              Serial.println(currentLine);
              // === zjistovani stavu buttonu, pokud se jen nacte stranka vyhne se  tomuto uplne
              if (strstr(currentLine, "/on") != NULL) { //strstr hleda text ve stringu, pta se jestli ho najde
                digitalWrite(26, HIGH);
                GoTo = 1;
              }
              if (strstr(currentLine, "/off") != NULL) {
                digitalWrite(26, LOW);
                GoTo = 0;
              }
              firstLine = false;
          }
          if(currentLine[0] == '\0') {// Empty line → end of headers
            break;
          }
          pos = 0;
          currentLine[0] = '\0'; //smaze string aby ho mohl zaplnit dalsim radkem
        } //if (c == '\n') {
        else if (c != '\r' && pos < 127) {//vyhnes se ukladani \r a vyhnes se ukladani c na 128 byte - tam vzdycky patri  ukonceni
          currentLine[pos++] = c; //tady uklada do currentlinu
        }  
      }  // if (client.available())  
    }  //  while (client.connected()) {
//----------------------------------------------------WEBSERVER-BUILDING-----------------------
    client.println("HTTP/1.1 200 OK");
    client.println("Content-type:text/html");
    client.println("Connection: close");
    client.println();

    client.println("<!DOCTYPE html><html>");
    client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
    client.println("<link rel=\"icon\" href=\"data:,\">");
    client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
      client.println("a { text-decoration: none; font-size: 30px; margin: 10px; padding: 10px 20px; cursor: pointer; }");
      client.println(".on { background-color: #4CAF50; color: white; }");
      client.println(".off { background-color: #f44336; color: white; }");
    client.println("</style></head>");


    for(int e = 0; e < flightnum; e++){
      client.println("<h1>");
      client.println("Flight ");
      itoa(flightnum, poradi, 10);
      client.println(poradi); //placeholder
      client.println(" Max Altitude: ");
      if(MaxAltitudeBuffer[e] == 0.00){
        client.println("N/A");
      }

      else{
        dtostrf(MaxAltitudeBuffer[e], 6, 2, StringMaxAltitude[e]);        
        client.println(StringMaxAltitude[e]);

      }
      client.println("m");
      client.println("</h1>");
    }
   

//-------------------------------------BUTTON----------------------------
    client.println("</p>");
    client.print("<a href=\"/");
    client.print(GoTo ? "off" : "on");     // toggle link
    client.print("\" class=\"");
    client.print(GoTo ? "on" : "off");     // toggle style
    client.print("\">");
    client.print(GoTo ? "ON" : "OFF");     // toggle text
    client.println("</a>");

    client.println("</body></html>");
    client.println();

    client.stop();
    Serial.println("Client disconnected.\n");
  }





}
void DataSmooth(float Raw){
  BMP_SmoothAltitude = SmoothFactor * Raw +(1-SmoothFactor)*BMP_SmoothAltitude;
  //Serial.println(i);
  //Serial.println(BMP_SmoothAltitude);
  //Serial.println("___________");

}
void BMP(){

}
void BMP_sim(){   //{--------------------------------------------------BMP-----------------------------------
  BMP_PastAltitude = BMP_SmoothAltitude;
  int x = random(100,200);
  BMP_RawAltitude = 0;
  if((i<x) && (ascent == true)){
    i = i+random(1,5) +random(-5,5);
    ////Serial.println("Plus");
  }

  else if(i>=x){
    ascent = false;
    i = i-random(1,5) + random(-5,2);

        //Serial.print("Minus: ");
        //Serial.println(i);
  }

  else if(ascent == false&&i>0){
    i = i-random(1,5) + random(-5,2);
            //Serial.print("Descent: ");
        //Serial.println(i);
  }

  BMP_RawAltitude = i;

  delay(50);

}
//--------------------------------------------------------------MAX_ALT-DETECTION----------------------------
void Max_Alt(){
  if(BMP_SmoothAltitude<BMP_PastAltitude){
    BMP_Counter++;
    delay(5);

  }
  else{
    BMP_Counter = 0;
  }

  if(BMP_Counter >5&&MaxAlt == false){ //Zvysit na 20!!!!!
    BMP_Counter = 0;
    //Altitude = BMP_SmoothAltitude;
    Serial.println("ALTITUDE MAX REACHED_____________");
    //Serial.println(BMP_SmoothAltitude);
    MaxAlt = true;
    BMP_MaxAltitude = BMP_SmoothAltitude;
    //ULOZENI MAX ALTITUDE, je potreba to lehce postelovat aby bylo ukladani tech jednotlivejch altitud fukcni, ale to musim prvni implementovat buttony
    Serial.print("Flight number: ");
    Serial.println(flightnum-1);
    MaxAltitudeBuffer[flightnum-1] = BMP_MaxAltitude;
    Serial.println(MaxAltitudeBuffer[flightnum-1]);
  }

  
}