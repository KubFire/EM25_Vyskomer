// Load Wi-Fi library
#include <WiFi.h>

// Replace with your network credentials
const char* ssid     = "ESP32-Access-Point";
const char* password = "123456789";

// Set web server port number to 80
WiFiServer server(80);

// Variable to store the HTTP request
String header;
char currentLine[128] = {0};  // Zero-init to start empty
int pos = 0;                  // Position in buffer
bool firstLine = true;  // Process only the first line
long BMP_AvgTemperature;
long BMP_AvgPressure;
long BMP_AvgAltitude;

long BMP_Temperature;
long BMP_Pressure;
long BMP_Altitude;
long BMP_Correction;
long BMP_DefaultPressure;
long BMP_PastAltitude;
int BMP_Counter;

int FlightCount = 1;
int AltitudeCurrent;
int Altitude1;
int Altitude2;
int Altitude3;

// Assign output variables to GPIO pins

long Test;
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
}


void loop() {
  WiFiClient client = server.available();
  BMP();
  String TESTING = String(Test);  // Keep your existing logic

  if (client) {
    Serial.println("New Client.");

    // Reset state
    currentLine[0] = '\0';
    pos = 0;
    firstLine = true;

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
                Test = 1;
              }
              if (strstr(currentLine, "/off") != NULL) {
                digitalWrite(26, LOW);
                Test = 0;
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

  // === SEND RESPONSE ===
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

    client.println("<body>");
    client.println("<h1>Current Altitude:");
    client.println(AltitudeCurrent);
    client.println("</h1>");
    switch(FlightCount){

    case 1:
    client.print("<p>Max Altitude Flight 1");
    client.print(Altitude1);
    client.println("meters");
    break;

    case 2:
    client.print("<p>Max Altitude Flight 1");
    client.print(Altitude1);
    client.println("meters");

    client.print("<p>Max Altitude Flight 2");
    client.print(Altitude2 );
    client.println("meters");
    break;

    case 3:
    client.print("<p>Max Altitude Flight 1");
    client.print(Altitude1);
    client.println("meters");

    client.print("<p>Max Altitude Flight 2");
    client.print(Altitude2);
    client.println("meters");

    client.print("<p>Max Altitude Flight 3");
    client.print(Altitude3);
    client.println("meters");
    break;



    }


    client.println("</p>");
    client.print("<a href=\"/on\" class=\"");
    client.print(Test ? "on" : "off"); //takova ta super kompaktni if formula
    client.println("\">ON</a>");

    client.println("</body></html>");
    client.println();

    client.stop();
    Serial.println("Client disconnected.\n");
  }
}

void BMP1(){

  Test = random(900, 1000);

}
void BMP(){   //{-------------------------------------------BMP---------------------
  BMP_PastAltitude = BMP_AvgAltitude;
    //Serial.println("Hello, its yo fav Pressure Sensor sensing!");
  int z = 0;
  //BMP_AvgTemperature = 0;
  //BMP_AvgPressure = 0;
  BMP_AvgAltitude = 0;

  for(z; z<5;z++){                    //--------DATA SMOOTHING-----------
    //Serial.println("Doing a pass");
    //BMP_Temperature = (random(20,40)*100); //temperature = 32.17
    //BMP_AvgTemperature = BMP_AvgTemperature+BMP_Temperature;

    //BMP_Pressure = ((random(950,1050)+BMP_Correction));//pressure = 1017.12
    //BMP_AvgPressure = BMP_AvgPressure+BMP_Pressure;

    BMP_Altitude = (random(200,400)); //KALIBROVAT CORRECTION S POMOCI PREDPOVEDI //height 271.72
    BMP_AvgAltitude = BMP_AvgAltitude+BMP_Altitude;

    //BMP_Altitude = (bmp.readAltitude(BMP_DefaultPressure)*100); //KALIBROVAT CORRECTION S POMOCI PREDPOVEDI //height 271.72
    //BMP_AvgAltitude = BMP_AvgAltitude+BMP_Altitude;
  }
  //BMP_AvgTemperature = BMP_AvgTemperature/5;
  //BMP_AvgPressure = BMP_AvgPressure/5;
  BMP_AvgAltitude = BMP_AvgAltitude/5;
  //Serial.println(BMP_AvgTemperature);
  //Serial.println(BMP_AvgPressure);
  Serial.println(BMP_AvgAltitude);
  
}
void Max_Alt(){
                                          //--------MAX_ALT-DETECTION-----------
  if(BMP_AvgAltitude<BMP_PastAltitude){
    BMP_Counter++;
    delay(5);

  }
  else{
    BMP_Counter = 0;
  }

  if(BMP_Counter >3){ //Zvysit na 20!!!!!
    BMP_Counter = 0;
    Altitude1 = BMP_AvgAltitude;
    //ULOZENI MAX ALTITUDE, je potreba to lehce postelovat aby bylo ukladani tech jednotlivejch altitud fukcni, ale to musim prvni implementovat buttony
  }

  
}