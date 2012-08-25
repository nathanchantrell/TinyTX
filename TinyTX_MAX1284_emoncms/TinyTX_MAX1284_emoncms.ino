//------------------------------------------------------------------------------------------------------------------
// MAX1284 Multiple TX to emoncms
// For the MAX1284 Internet Gateway http://max1284.homelabs.org.uk/
// Receives data from multiple TinyTX sensors and/or emonTX and uploads to an emoncms server.
// See README for required modifications to Jeelib library.
// By Nathan Chantrell. http://zorg.org/
// Licenced under GNU GPL V3
// Based on emonbase multiple emontx example for ethershield by Trystan Lea and Glyn Hudson at OpenEnergyMonitor.org
//------------------------------------------------------------------------------------------------------------------

//#define DEBUG                // uncomment for serial output (57600 baud)

#include <SPI.h>
#include <JeeLib.h>          // https://github.com/jcw/jeelib
#include <Ethernet52.h>      // https://bitbucket.org/homehack/ethernet52/src
 
// Fixed RF12 settings
#define MYNODE 30            // node ID 30 reserved for base station
#define freq RF12_433MHZ     // frequency
#define group 210            // network group 

// emoncms settings, change these settings to match your own setup
#define SERVER  "tardis.chantrell.net";              // emoncms server
#define EMONCMS "emoncms"                            // location of emoncms on server, blank if at root
#define APIKEY  "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"   // API write key 

#define ledPin 15            // MAX1284 LED on Pin 15 / PD7

// The RF12 data payload
typedef struct
{
  int data1;		    // received data 1
  int supplyV;              // voltage
  int data2;		    // received data 2
} Payload;
Payload rx; 

// PacketBuffer class used to generate the json string that is send via ethernet - JeeLabs
class PacketBuffer : public Print {
public:
    PacketBuffer () : fill (0) {}
    const char* buffer() { return buf; }
    byte length() { return fill; }
    void reset() { memset(buf,NULL,sizeof(buf)); fill = 0; }
    virtual size_t write(uint8_t ch)
        { if (fill < sizeof buf) buf[fill++] = ch; }  
    byte fill;
    char buf[150];
    private:
};
PacketBuffer str;

// Ethernet
  byte mac[] = {  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };  // ethernet mac address, must be unique on the LAN
  byte ip[] = {192, 168, 0, 178 };                       // Backup static IP to be used if DHCP fails
  EthernetClient client;

// Flow control varaiables
  int dataReady=0;                         // is set to 1 when there is data ready to be sent
  unsigned long lastRF;                    // used to check for RF recieve failures

//--------------------------------------------------------------------
// Setup
//--------------------------------------------------------------------

void setup () {
  
  pinMode(ledPin, OUTPUT);                 // Set LED Pin as output
  digitalWrite(ledPin,HIGH);               // Turn LED on
  
  #ifdef DEBUG
    Serial.begin(57600);
    Serial.println("MAX1284 Multiple TX to emoncms");
    Serial.print("Node: "); Serial.print(MYNODE); 
    Serial.print(" Freq: "); 
     if (freq == RF12_433MHZ) Serial.print("433 MHz");
     if (freq == RF12_868MHZ) Serial.print("868 MHz");
     if (freq == RF12_915MHZ) Serial.print("915 MHz");  
    Serial.print(" Network group: "); Serial.println(group);
  #endif
 
// Manually configure ip address if DHCP fails
  if (Ethernet.begin(mac) == 0) {
  #ifdef DEBUG
    Serial.println("DHCP failed");
  #endif
    Ethernet.begin(mac,ip);  
  }

// print the IP address:
  #ifdef DEBUG
  Serial.print("IP:  ");
  for (byte thisByte = 0; thisByte < 4; thisByte++) {
   Serial.print(Ethernet.localIP()[thisByte], DEC); 
   Serial.print(".");
  }
  Serial.println();
  #endif

  rf12_initialize(MYNODE,freq,group);     // initialise the RFM12B
  lastRF = millis()-40000;                // Forces the a send straight away

  digitalWrite(ledPin,LOW);               // Turn LED off to indicate setup has finished
}

void loop () {

//--------------------------------------------------------------------  
// On data receieved from rf12
//--------------------------------------------------------------------

  if (rf12_recvDone() && rf12_crc == 0 && (rf12_hdr & RF12_HDR_CTL) == 0) 
  {

   digitalWrite(ledPin,HIGH);             // Turn LED on    
   int nodeID = rf12_hdr & 0x1F;          // extract node ID from received packet
   rx=*(Payload*) rf12_data;              // Get the payload
   
// JSON creation: format: {key1:value1,key2:value2} and so on
    
   str.reset();                           // Reset json string     
   str.print("{rf_fail:0,");              // RF recieved so no failure
   
   str.print("node");  
   str.print(nodeID);                     // Add node ID
   str.print("_data1:");
   str.print(rx.data1);                   // Add reading 1
   
   str.print(",node");  
   str.print(nodeID);                     // Add node ID
   str.print("_data2:"); 
   str.print(rx.data2);                   // Add reading 2
   
   str.print(",node");   
   str.print(nodeID);                     // Add node ID
   str.print("_v:");
   str.print(rx.supplyV);                 // Add tx battery voltage reading

   str.print("}\0");

   dataReady = 1;                         // Ok, data is ready
   lastRF = millis();                     // reset lastRF timer

   #ifdef DEBUG
    Serial.println("Data received");
   #endif
  }

// If no data is recieved from rf12 module the server is updated every 30s with RFfail = 1 indicator for debugging
  if ((millis()-lastRF)>30000)
  {
    lastRF = millis();                    // reset lastRF timer
    str.reset();                          // reset json string
    str.print("{rf_fail:1}\0");           // No RF received in 30 seconds so send failure 
    dataReady = 1;                        // Ok, data is ready
  }
  
//--------------------------------------------------------------------
// Send the data
//--------------------------------------------------------------------
 
  if (dataReady==1) {                     // If data is ready: send data

   #ifdef DEBUG
    Serial.print("Sending to emoncms: ");
    Serial.println(str.buf);
   #endif

   char server[] = SERVER;    
   String url = "/"EMONCMS"/api/post?apikey="APIKEY"&json=";

   // Send the data  
   if (client.connect(server, 80)) {
    client.print("GET "); client.print(url); client.print(str.buf); client.println();
    delay(200);
    client.stop();
     
    digitalWrite(ledPin,LOW);           // Turn LED OFF
     
    dataReady = 0;                      // reset data ready flag
    } 
    else { 
     #ifdef DEBUG
     Serial.println("connection failed"); 
     #endif
     delay(1000); // wait 1s before trying again 
    }

  }

}

