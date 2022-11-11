#include <SoftwareSerial.h>
#include <DHT.h>
#include <SPI.h>
#include <Ethernet.h>
#define DHTPIN 5
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);
bool AlarmSent=false;
float AlarmTemp = 24;
float calib=254;
int relayPin = 8;
int motor =9;
int led = 4;
const unsigned long event1= 0; // intervalul de timp la care se dorește repetarea evenimentului 1
const unsigned long event2= 0; // intervalul de timp la care se dorește repetarea evenimentului 2
unsigned long previous1=0;
unsigned long previous2=0;
//Crearea unui obiect software serial care să comunice cu A6
SoftwareSerial mySerial(2, 3); //A6 Tx & Rx sunt conectați la Arduino la pinii #3 & #2
// Aici se introduce adresa MAC și IP
// Adresa Ip depinde de rețeaua de internet locală
byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};
IPAddress ip(192, 168, 0, 177);
// Inițializăm serverul Ethernet cu adressa IP și portul pe care dorim să le folosim
// Portul 80 este setat în mod implicit pentru HTTP
EthernetServer server(80);

void setup() {
  // Deschidem comunicația serială și așteptăm să portul să se deschidă
  Serial.begin(115200);
  mySerial.begin(115200);
  //Începem să citim date de la senzorul DHT11
  dht.begin(); 
  // Începem comunicația serială cu Arduino și cu A6
  mySerial.begin(115200);
  while (!Serial) {
    ; // Așteptăm până se conectează portul serial
  }
   pinMode(relayPin, OUTPUT);
   pinMode(led,OUTPUT);
   pinMode(motor,OUTPUT);
   Serial.println("Ethernet WebServer Example");

   //pornim conexiunea Ethernet și serverul
   Ethernet.begin(mac, ip);

   // Verificăm dacă modulul Ethernet este prezent  
   if (Ethernet.hardwareStatus() == EthernetNoHardware) {
     Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
     while (true) {
      delay(1); // do nothing, no point running without Ethernet hardware
     }
   }
  if (Ethernet.linkStatus() == LinkOFF) {
    Serial.println("Ethernet cable is not connected.");
  }

  // pornim serverul
  server.begin();
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());

  Serial.println("Initializing..."); 
  delay(1000);

  mySerial.println("AT"); // Returnează OK dacă conexiunea s-a efectuat cu succes
  updateSerial();
  mySerial.println("AT+CMGF=1"); // Configurăm modul de transfer
  updateSerial();
  mySerial.println("AT+CMGS=\"+40756987041\"");
  updateSerial();
  mySerial.print("Link confirmed, sending sensor data"); //conținutul mesajului
  updateSerial();
  mySerial.write(26);  
}
void loop() {
  unsigned long currentTime=millis();
  if(currentTime-previous1>=event1)
  {
   previous1=currentTime;
   digitalWrite(motor,HIGH);
   delay(1000);
   digitalWrite(motor,LOW);
   delay(20000);
  }
  if(currentTime-previous2>=event2)
  {
   digitalWrite(relayPin,HIGH);
   int value=analogRead(1);
   int lumina=analogRead(0);
   float ph=analogRead(2);
  // calculăm valoarea temperaturii, a umidițății și indicele de căldură
   float h = dht.readHumidity();
   float t = dht.readTemperature();
   float hic = dht.computeHeatIndex(t, h, false);

   // verificăm dacă senzorul este detectat, dacă nu transmitem un mesaj de eroare pe serial
  if (isnan(h) ||isnan(t))
  {
    Serial.println(F("Failed to read from DHT sensor!"));
    delay(5000);
    return;
  }
    if(lumina<540)
  digitalWrite(led,HIGH);
  else
   digitalWrite(led,LOW);
  if(t >= AlarmTemp && AlarmSent == false)
  {
    mySerial.println("AT+CMGF=1"); // Configurăm modul de transfer
    updateSerial();
    mySerial.println("AT+CMGS=\"+40756987041\"");
    updateSerial();
    mySerial.println("ATENTIE TEMPERATURA PESTE 30 DE GRADE!!! "); //conținutul mesajului
    updateSerial();
    mySerial.write(26);
    AlarmSent = true; 
  }
  // ascultă clienții care vin
  EthernetClient client = server.available();
  if (client) {
    Serial.println("new client");
    // o solicitare http se termină cu o linie goală
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        if (c == '\n'&& currentLineIsBlank) {
          // trimite un antet standard de răspuns http
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println();
          client.println("<html>");
          client.println("<HEAD>");
          client.println("<meta name='apple-mobile-web-app-capable' content='yes' />");
           client.println("<meta name='apple-mobile-web-app-status-bar-style' content='black-translucent' />");
           client.println("<link rel='stylesheet' type='text/css' href='https://randomnerdtutorials.com/ethernetcss.css' />");
           client.println("<TITLE> Proiect de diplomă </TITLE>");
           client.println("</HEAD>");
           client.println("<body bgcolor=#219db0>");
           client.println("<H1>Proiectarea unui acvariu inteligent</H1>");
           client.println("<br />");  
           client.println("<H2>Arduino with Ethernet Shield</H2>"); 
           client.println("<br />"); 
           client.println("<hr color=black />");
           client.print("<H3>Temperature: </H3>");
           client.print(t);
           client.println("<br />");
           client.print("<H3>Umiditate: </H3>");
           client.print(h);
           client.println("<br />");
           client.print("<H3>Heat index: </H3>");
           client.print(hic);
           client.println("<br />");
           client.println("<hr color=black />");
           client.print("<H3>Nivel apa: </H3> ");
           client.print(value);
           client.println("<br />");
           client.println("<hr color=black />");
           client.print("<H3>Nivel iluminare: </H3>");
           client.print(lumina);
           client.println("<br />");
           client.println("<hr color=black />");
           client.print("<H3>Ph apa: </H3> ");
           client.print(ph-calib);
           client.println("<br />");
           client.println("<br />");
           client.println("</BODY>");
           client.println("</html>");
           break;
        }
        if (c == '\n') {
          // începe o nouă linie
          currentLineIsBlank = true;
        } else if (c != '\r') {
          // s-a primit un caracter pe linia curentă
          currentLineIsBlank = false;
        }
      }
    }
    // alocăm timp browserului web să primească datele
    delay(1);
    // închidem conexiunea
    client.stop();   
  }
  previous1=currentTime;
}
}
// actualizează funcția serială pentru operații mySerial
void updateSerial()
{
  delay(500);
  while (Serial.available()) 
  {
    // redirecționează ce primește Serial către portul Software Serial 
    mySerial.write(Serial.read());
  }
  while(mySerial.available()) 
  {
    // redirecționează ce primește Software Serial către portul Serial 
    Serial.write(mySerial.read());
  }
}
