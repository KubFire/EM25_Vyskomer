// Libraries stuff
#include <WiFi.h>
#include <Wire.h>
#include "BMP580.h"  // Vklady naši knihovnu

const char* ssid = " - EM Flight Computer 1 - "; //Jmeno letoveho pocitace ve Wifi - az jich budeme mit nekolik zmenit od kazdeho jmeno
const int BUTTON_PIN = 14; //Pin buttonu, ale momentalne ten button nic nedela
const float BMP_DefaultPressure = 1013; //POTREBA VZDY ZMENIT ten den kdy se startuje!!!!!!!
const int FlightAmount = 7; //max mnozstvi letu co je schopny zaznamenat
const uint8_t MaxAltSensitivity = 10; //kolik readingu za sebou musi klesat aby detekoval klesani - pokud jich je moc malo muze dochazet k falesnym detekovanim - vyladit s irl sensorem.
const float SmoothFactor = 0.4;  //data exponential smoothing - jak moc velkou vahu ma predchozi mereni v smoothovani sumu soucasneho
const char BackgroundColor[8] = "#F7F0DA";
const char TextColor[8] = "#291C24";



//Webserver stuff

WiFiServer server(80);
char currentLine[128] = { 0 };  // Zero-init to start empty
int pos = 0;                    // Position in buffer
bool firstLine = true;          // Process only the first line

BMP580 bmp;  // Vytvorení objektu BMP580


//Simulace stuff
int i = 5;
bool ascent = true;



//Altitude stuff

float BMP_Correction;
float BMP_RawPressure;//pro jistotku, not sure jestli ten tlak taky nepotrebuje merit.
float BMP_RawAltitude;
float BMP_PastAltitude;
float BMP_SmoothAltitude;
uint8_t BMP_Counter;
bool MaxAlt = false;

bool GoTo;
bool PastGoTo;
bool Press;

//Altitude buffer stuff
int flightnum = 0;
char StringMaxAltitude[FlightAmount][10];
float MaxAltitudeBuffer[FlightAmount];
char poradi[2];

void setup() {
  Serial.begin(115200);

  Serial.print("Setting BMP580");

  Wire.begin();  // Inicializace I2C (SDA: GP4, SCL: GP5)
  bmp.begin(0x46);
  Serial.println(bmp.readAltitude(BMP_DefaultPressure));


  Serial.print("Setting AP");
  WiFi.softAP(ssid);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
  server.begin();

  pinMode(BUTTON_PIN, INPUT_PULLUP);

}


void loop() {
  //----------------------------------------------LOOP------------------------------
  Press = digitalRead(BUTTON_PIN);
  if (Press == 0) {
    BMP_sim();
    //BMP(); //na finalni verzi nahradit BMP_sim BMP.

    DataSmooth(BMP_RawAltitude);
    Max_Alt();
  }

  if (GoTo == 1) {
    //Serial.println("Goto 1");
    if (GoTo != PastGoTo) {
      Serial.println("Ready for next Launch");


      if (flightnum < FlightAmount) {
        BMP_SmoothAltitude = 0;
        BMP_PastAltitude = 0;
        BMP_RawAltitude = 0;
        ascent = true;
        MaxAlt = false;
        flightnum++;
      } else {
        flightnum = FlightAmount;
      }
      i = 4;  //simulation, delete later
    }
    Serial.println("Measuring..");
    BMP_sim();
    //BMP(); //na finalni verzi nahradit BMP_sim BMP.
    DataSmooth(BMP_RawAltitude);
    Serial.println(BMP_SmoothAltitude);
    Max_Alt();
  } else {
  }
  PastGoTo = GoTo;


  //-------------------------------------------------WEBSERVER-------------------------------

  WiFiClient client = server.available();

  if (client) {
    currentLine[0] = '\0';
    pos = 0;
    firstLine = true;
    //--------------------------------------WEBSERVER-REQUEST-HANDLING----------------------------
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Serial.write(c);

        if (c == '\n') {            //ta '\n' je vzdy na konci radku, takze hleda konec radku
          currentLine[pos] = '\0';  //prida do stringu ukonceni
          if (firstLine == true) {  //dekodovani prvniho radku requestu, az ho precte uz sem nejde
            Serial.print("Request: ");
            Serial.println(currentLine);
            // === zjistovani stavu buttonu, pokud se jen nacte stranka vyhne se  tomuto uplne
            if (strstr(currentLine, "/on") != NULL) {  //strstr hleda text ve stringu, pta se jestli ho najde
              digitalWrite(26, HIGH);
              GoTo = 1;
            }
            if (strstr(currentLine, "/off") != NULL) {
              digitalWrite(26, LOW);
              GoTo = 0;
            }
            firstLine = false;
          }
          if (currentLine[0] == '\0') {  // Empty line → end of headers
            break;
          }
          pos = 0;
          currentLine[0] = '\0';            //smaze string aby ho mohl zaplnit dalsim radkem
        }                                   //if (c == '\n') {
        else if (c != '\r' && pos < 127) {  //vyhnes se ukladani \r a vyhnes se ukladani c na 128 byte - tam vzdycky patri  ukonceni
          currentLine[pos++] = c;           //tady uklada do currentlinu
        }
      }  // if (client.available())
    }    //  while (client.connected()) {
         //----------------------------------------------------WEBSERVER-BUILDING-----------------------
    client.println("HTTP/1.1 200 OK");
    client.println("Content-type:text/html");
    client.println("Connection: close");
    client.println();

    client.println("<!DOCTYPE html><html>");
    client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
    client.println("<link rel=\"icon\" href=\"data:,\">");
    client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center; background-color:");
    client.println(BackgroundColor);
    client.println("; color:");
    client.println(TextColor);
    client.println(";}");
    client.println("a { text-decoration: none; font-size: 30px; margin: 10px; padding: 10px 20px; cursor: pointer; }");
    client.println(".on { background-color: #f44336; color: white; }");
    client.println(".off { background-color: #4CAF50; color: white; }");


    client.println("</style></head>");
    client.println("<body>");

    client.println("<p> </p>");

    client.println("<h2>EXPEDICE MARS Flight Computer</h2>");

    client.println("<p>Kdyz zmacknes Go For Launch, zacne merit maximalni vysku.</p>");
    client.println("<p> Tlacitko Finish Measuring zmackni az po pristani rakety, ukonci mereni vysky daneho letu a pripravi pocitac na dalsi let!</p>");


    for (int e = 0; e < flightnum; e++) {
      client.println("<h1>");
      client.println("Flight ");
      itoa(e + 1, poradi, 10);
      client.println(poradi);  //placeholder
      client.println(" Max Altitude: ");
      if (MaxAltitudeBuffer[e] == 0.00) {
        client.println("N/A");
      } else {
        dtostrf(MaxAltitudeBuffer[e], 6, 2, StringMaxAltitude[e]);
        client.println(StringMaxAltitude[e]);
      }
      client.println("m");
      client.println("</h1>");
    }
    client.println("<p> </p>");
    //-------------------------------------BUTTON----------------------------
    client.println("</p>");
    client.print("<a href=\"/");
    client.print(GoTo ? "off" : "on");  // toggle link
    client.print("\" class=\"");
    client.print(GoTo ? "on" : "off");  // toggle style
    client.print("\">");
    client.print(GoTo ? "Finish Measuring" : "Go For Launch");  // toggle text
    client.println("</a>");

    client.println("</body></html>");
    client.println();

    client.stop();
    Serial.println("Client disconnected.\n");
  }
}
void DataSmooth(float Raw) {
  BMP_SmoothAltitude = SmoothFactor * Raw + (1 - SmoothFactor) * BMP_SmoothAltitude;
  //Serial.println(i);
  //Serial.println(BMP_SmoothAltitude);
  //Serial.println("___________");
}

void BMP() {
  BMP_RawPressure = bmp.readPressure();
  BMP_RawAltitude =bmp.readAltitude(BMP_DefaultPressure);




}
void BMP_sim() {  //{--------------------------------------------------BMP-----------------------------------
  BMP_PastAltitude = BMP_SmoothAltitude;
  int x = random(100, 200);
  BMP_RawAltitude = 0;
  if ((i < x) && (ascent == true)) {
    i = i + random(1, 5) + random(-5, 5);
    ////Serial.println("Plus");
  }

  else if (i >= x) {
    ascent = false;
    i = i - random(1, 5) + random(-5, 2);

    //Serial.print("Minus: ");
    //Serial.println(i);
  }

  else if (ascent == false && i > 0) {
    i = i - random(1, 5) + random(-5, 2);
    //Serial.print("Descent: ");
    //Serial.println(i);
  }

  BMP_RawAltitude = i;

  delay(50);
}
//--------------------------------------------------------------MAX_ALT-DETECTION----------------------------
void Max_Alt() {
  if (BMP_SmoothAltitude < BMP_PastAltitude) {
    BMP_Counter++;
    delay(5);

  } else {
    BMP_Counter = 0;
  }

  if (BMP_Counter > MaxAltSensitivity && MaxAlt == false) {  //Zvysit na 20!!!!!
    BMP_Counter = 0;
    //Altitude = BMP_SmoothAltitude;
    Serial.println("ALTITUDE MAX REACHED_____________");
    //Serial.println(BMP_SmoothAltitude);
    MaxAlt = true;
    BMP_SmoothAltitude;
    //ULOZENI MAX ALTITUDE, je potreba to lehce postelovat aby bylo ukladani tech jednotlivejch altitud fukcni, ale to musim prvni implementovat buttony
    Serial.print("Flight number: ");
    Serial.println(flightnum - 1);
    MaxAltitudeBuffer[flightnum - 1] = BMP_SmoothAltitude;
    Serial.println(MaxAltitudeBuffer[flightnum - 1]);
  }
}