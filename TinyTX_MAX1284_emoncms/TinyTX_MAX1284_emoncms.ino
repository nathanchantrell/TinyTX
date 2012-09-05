//------------------------------------------------------------------------------------------------------------------
// MAX1284 Multiple TX to emoncms
// For the MAX1284 Internet Gateway http://max1284.homelabs.org.uk/
// Receives data from multiple TinyTX sensors and/or emonTX and uploads to an emoncms server.
// Gets NTP time once an hour and transmits for GLCD displays
// See README for required modifications to Jeelib library.
// By Nathan Chantrell. http://zorg.org/
// Licenced under GNU GPL V3
// Based on emonbase multiple emontx example for ethershield by Trystan Lea and Glyn Hudson at OpenEnergyMonitor.org
//------------------------------------------------------------------------------------------------------------------

#define DEBUG                // uncomment for serial output (57600 baud)

#include <SPI.h>
#include <JeeLib.h>          // https://github.com/jcw/jeelib
#include <Ethernet52.h>      // https://bitbucket.org/homehack/ethernet52/src
#include <EthernetUdp.h>
#include <avr/wdt.h>
 
// Fixed RF12 settings
#define MYNODE 30            // node ID 30 reserved for base station
#define freq RF12_433MHZ     // frequency
#define group 210            // network group 

// emoncms settings, change these settings to match your own setup
IPAddress server(192,168,0,21);                      // IP address of emoncms server
#define HOSTNAME "www.chantrell.net"                 // Hostname of emoncms server
#define EMONCMS "emoncms"                            // location of emoncms on server, blank if at root
#define APIKEY  "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"   // API write key 

// NTP Stuff
#define USE_NTP                           // Comment out to disable NTP transmit function
#define NTP_PERIOD 60                     // How often to get & transmit the time in minutes
#define UDP_PORT 8888                     // Local port to listen for UDP packets
#define NTP_PACKET_SIZE 48                // NTP time stamp is in the first 48 bytes of the message
IPAddress timeServer(192, 43, 244, 18);   // time.nist.gov NTP server
byte packetBuffer[ NTP_PACKET_SIZE];      // Buffer to hold incoming and outgoing packets 
EthernetUDP Udp;                          // A UDP instance to let us send and receive packets over UDP
unsigned long timeTX = -NTP_PERIOD*60000; // for time transmit function

#define ledPin 15            // MAX1284 LED on Pin 15 / PD7

// The RF12 data payload received
typedef struct
{
  int data1;		 // received data 1
  int supplyV;           // voltage
  int data2;		 // received data 2
} Payload;
Payload rx; 

// The RF12 data payload to be sent, used to send server time to GLCD display
typedef struct
{
  int hour, mins, sec;  // time
} PayloadOut;
PayloadOut ntp; 

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
    Serial.println("-------------------------------------------------");
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
  Serial.println("-------------------------------------------------");
  #endif

  rf12_initialize(MYNODE,freq,group);     // initialise the RFM12B
  
  #ifdef USE_NTP
  Udp.begin(UDP_PORT);                    // Start UDP for NTP client
  #endif
  
  digitalWrite(ledPin,LOW);               // Turn LED off to indicate setup has finished
  
  delay(1000);
  
  wdt_enable(WDTO_8S);                    // Enable the watchdog timer with an 8 second timeout
    
}

//--------------------------------------------------------------------
// Loop
//--------------------------------------------------------------------

void loop () {
  
  wdt_reset(); // Reset the watchdog timer
  
  #ifdef USE_NTP
  if ((millis()-timeTX)>(NTP_PERIOD*60000)){    // Send NTP time
    getTime();
  } 
  #endif

//--------------------------------------------------------------------  
// On data receieved from rf12
//--------------------------------------------------------------------

  if (dataReady==0 && rf12_recvDone() && rf12_crc == 0 && (rf12_hdr & RF12_HDR_CTL) == 0) 
  {

   int nodeID = rf12_hdr & 0x1F;          // extract node ID from received packet
   rx=*(Payload*) rf12_data;              // Get the payload
   digitalWrite(ledPin,HIGH);             // Turn LED on    
   
   #ifdef DEBUG
    Serial.println();
    Serial.print("Data received from Node ");
    Serial.println(nodeID);
   #endif
   
   if (RF12_WANTS_ACK) {                  // Send ACK if requested
     #ifdef DEBUG
      Serial.println("-> ack sent");
     #endif
     rf12_sendStart(RF12_ACK_REPLY, 0, 0);
   }
   
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
 
  if (!client.connected() && dataReady==1) {     // If not connected and data is ready: send data



   #ifdef DEBUG
    Serial.println("Connecting to server");
   #endif
   
   wdt_reset(); // Reset the watchdog timer
    
   // Send the data  
   if (client.connect(server, 80)) {     // Connect to server

    if (client.connected()) {            // If connected send data
    
     #ifdef DEBUG
      Serial.print("Sending to emoncms: ");
      Serial.println(str.buf);
     #endif
   
     // The HTTP GET request   
     client.print("GET "); client.print("/"EMONCMS"/api/post.json?apikey="APIKEY"&json="); client.print(str.buf);       
     client.println(" HTTP/1.1");
     client.print("Host: ");
     client.println(HOSTNAME);
     client.print("User-Agent: MAX1284");
     client.println("\r\n");
 
     #ifdef DEBUG
      Serial.println("Sent OK");
     #endif

     digitalWrite(ledPin,LOW);           // Turn LED OFF
     dataReady = 0;                      // reset data ready flag

    } 
    else { // If not connected retry on next loop
     #ifdef DEBUG
     Serial.println("Retrying... ");
     #endif
    }


   } 
   else {   // We didn't get a connection to the server, will retry on next loop
     #ifdef DEBUG
     Serial.print("ERROR: Connection failed to ");
     Serial.print(server);
     Serial.println(", will retry");
     #endif
   }

   client.stop();  // Disconnect
   #ifdef DEBUG
    Serial.println("Disconnected");
   #endif
  }

}

//--------------------------------------------------------------------
// Get NTP time, convert from unix time and send via RF
//--------------------------------------------------------------------

  void getTime() {
    #ifdef DEBUG  
    Serial.println();
    Serial.println("Getting NTP Time");
    #endif

    sendNTPpacket(timeServer);                                           // Send NTP packet to time server
    delay(1000);                                                         // Wait for reply

    if ( Udp.parsePacket() ) {                                           // Packet received
      timeTX = millis();                                                 // Reset timer
      Udp.read(packetBuffer,NTP_PACKET_SIZE);                            // Read packet into the buffer
      unsigned long highWord = word(packetBuffer[40], packetBuffer[41]); // Timestamp starts at byte 40 & is 4 bytes
      unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);  // or 2 words. Extract the two words.
      unsigned long secsSince1900 = highWord << 16 | lowWord;            // Combine into a long integer

      // Now convert NTP time into everyday time:
      const unsigned long seventyYears = 2208988800UL;                   // Unix time starts on 1/1/1970
      unsigned long epoch = secsSince1900 - seventyYears;                // Subtract seventy years:
      ntp.hour = ((epoch  % 86400L) / 3600);                             // 86400 equals secs per day
      ntp.hour += 1;                                                     // TO DO: Implement something for BST
      ntp.mins = ((epoch  % 3600) / 60);                                 // 3600 equals secs per minute
      ntp.sec = (epoch %60);                                             // seconds

      // Send the time via RF
      int i = 0; while (!rf12_canSend() && i<10) {rf12_recvDone(); i++;}
      rf12_sendStart(0, &ntp, sizeof ntp);                        
      rf12_sendWait(0);
      #ifdef DEBUG  
      Serial.print("Time sent: ");
      Serial.print(ntp.hour);
      Serial.print(":");
      if ( ntp.mins < 10 ) { Serial.print('0'); }
      Serial.print(ntp.mins);
      Serial.print(":");
      if ( ntp.sec < 10 ) { Serial.print('0'); }
      Serial.println(ntp.sec);
      Serial.println();
      #endif
    } else
    {
      #ifdef DEBUG  
      Serial.println("Didn't get reply from NTP server");
      Serial.println();
      #endif
      timeTX = (millis()-30000); // try again in 30 seconds
    }
  }

//--------------------------------------------------------------------
// send an NTP request to the time server at the given address 
//--------------------------------------------------------------------

  unsigned long sendNTPpacket(IPAddress& address) {
    memset(packetBuffer, 0, NTP_PACKET_SIZE); // set all bytes in the buffer to 0

    // Initialize values needed to form NTP request
    packetBuffer[0] = 0b11100011;   // LI, Version, Mode
    packetBuffer[1] = 0;            // Stratum, or type of clock
    packetBuffer[2] = 6;            // Polling Interval
    packetBuffer[3] = 0xEC;         // Peer Clock Precision

    // 8 bytes of zero for Root Delay & Root Dispersion
    packetBuffer[12]  = 49; 
    packetBuffer[13]  = 0x4E;
    packetBuffer[14]  = 49;
    packetBuffer[15]  = 52;

    // Send a packet requesting a timestamp: 		   
    Udp.beginPacket(address, 123); //NTP requests are to port 123
    Udp.write(packetBuffer,NTP_PACKET_SIZE);
    Udp.endPacket(); 
  }

