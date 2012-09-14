//------------------------------------------------------------------------------------------------------------------------
// NanodeRF Multiple TX to emoncms
// For the NanodeRF http://www.nanode.eu
// Receives data from multiple TinyTX sensors and/or emonTX and uploads to an emoncms server.
// By Nathan Chantrell. http://zorg.org/
// Licenced under GNU GPL V3
// Based on emonbase multiple emontx example for ethershield by Trystan Lea and Glyn Hudson at OpenEnergyMonitor.org
//------------------------------------------------------------------------------------------------------------------------

#define DEBUG                // uncomment for serial output (57600 baud)
//#define OPTIBOOT             // Use watchdog timer (for optiboot bootloader only)

#include <JeeLib.h>          // https://github.com/jcw/jeelib
#include <EtherCard.h>       // https://github.com/jcw/ethercard/tree/development  dev version with DHCP fixes
#include <NanodeMAC.h>       // https://github.com/thiseldo/NanodeMAC

#ifdef OPTIBOOT
#include <avr/wdt.h>
#endif

// Fixed RF12 settings
#define MYNODE 30            // node ID 30 reserved for base station
#define freq RF12_433MHZ     // frequency
#define group 210            // network group 

// emoncms settings, change these settings to match your own setup
#define SERVER  "tardis.chantrell.net"               // emoncms server
#define EMONCMS "emoncms"                            // location of emoncms on server, blank if at root
#define APIKEY  "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"   // API write key

// Time Transmit Stuff
#define TIMETX_PERIOD 60                     // How often to get & transmit the time in minutes
unsigned long timeTX = -TIMETX_PERIOD*60000; // for time transmit function

#define greenLedPin 5           // NanodeRF Green LED on Pin 5
#define redLedPin 6             // NanodeRF Red LED on Pin 6

// The RF12 data payload to be received
typedef struct
{
  int data1;		    // received data 1
  int supplyV;              // voltage
  int data2;		    // received data 2
} Payload;
Payload rx;

// The RF12 data payload to be sent, used to send server time to GLCD display
typedef struct
{
  int hour, mins, sec;  // time
} PayloadOut;
PayloadOut nanode; 

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

  char website[] PROGMEM = SERVER;
  byte Ethernet::buffer[700];
  uint32_t timer;
  Stash stash;
  char line_buf[50];                       // Used to store line of http reply header

// Set mac address using NanodeMAC
  static uint8_t mymac[6] = { 0,0,0,0,0,0 };
  NanodeMAC mac( mymac );

// Flow control varaiables
  int dataReady=0;                         // is set to 1 when there is data ready to be sent
  unsigned long lastRF;                    // used to check for RF recieve failures
  int post_count;                          // used to count number of ethernet posts that dont recieve a reply

//--------------------------------------------------------------------
// Setup
//--------------------------------------------------------------------

void setup () {
  
  pinMode(redLedPin, OUTPUT);               // Set Red LED Pin as output
  pinMode(greenLedPin, OUTPUT);             // Set Green LED Pin as output
  digitalWrite(redLedPin,LOW);              // Turn red LED on to indicate in setup (high=off)
  
  #ifdef DEBUG
    Serial.begin(57600);
    Serial.println("NanodeRF Multiple TX to emoncms");
    Serial.print("Node: "); Serial.print(MYNODE); 
    Serial.print(" Freq: "); 
     if (freq == RF12_433MHZ) Serial.print("433 MHz");
     if (freq == RF12_868MHZ) Serial.print("868 MHz");
     if (freq == RF12_915MHZ) Serial.print("915 MHz"); 
    Serial.print(" Network group: "); Serial.println(group);
  #endif
  
  if (ether.begin(sizeof Ethernet::buffer, mymac) == 0) 
    Serial.println( "Failed to access Ethernet controller");
  if (!ether.dhcpSetup())
    Serial.println("DHCP failed");
    
  ether.printIp("IP:  ", ether.myip);
  ether.printIp("GW:  ", ether.gwip);  
  ether.printIp("DNS: ", ether.dnsip);  

  if (!ether.dnsLookup(website))
    Serial.println("DNS failed");
    
  ether.printIp("SRV: ", ether.hisip);
  
  rf12_initialize(MYNODE, freq,group);      // initialise the RFM12B
  lastRF = millis()-40000;                  // Forces the a send straight away
  
  digitalWrite(redLedPin,HIGH);             // Turn red LED off to indicate setup has finished
  
  #ifdef OPTIBOOT
  wdt_enable(WDTO_8S);   // Enable the watchdog timer with an 8 second timeout
  #endif

}

  void loop () {
    
  #ifdef OPTIBOOT
  wdt_reset(); // Reset the watchdog timer
  #endif
  
//--------------------------------------------------------------------  
// On data receieved from rf12
//--------------------------------------------------------------------

  if (rf12_recvDone() && rf12_crc == 0 && (rf12_hdr & RF12_HDR_CTL) == 0) {

   digitalWrite(greenLedPin,LOW);         // Turn green LED on        
   int nodeID = rf12_hdr & 0x1F;          // extract node ID from received packet
   rx=*(Payload*) rf12_data;              // Get the payload
   
   #ifdef DEBUG
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
  if ((millis()-lastRF)>30000) {
    lastRF = millis();                      // reset lastRF timer
    str.reset();                            // reset json string
    str.print("{rf_fail:1}\0");             // No RF received in 30 seconds so send failure 
    dataReady = 1;                          // Ok, data is ready
  }
  
//--------------------------------------------------------------------
// Send the data
//--------------------------------------------------------------------
 
  ether.packetLoop(ether.packetReceive());
  
  if (dataReady==1) {                      // If data is ready: send data

   #ifdef DEBUG
    Serial.print("Sending to emoncms: ");
    Serial.println(str.buf);
   #endif

   ether.browseUrl(PSTR("http://"SERVER"/"EMONCMS"/api/post?apikey="APIKEY"&json="), str.buf, website, my_callback);

   digitalWrite(greenLedPin,HIGH);        // Turn green LED OFF
     
   dataReady = 0;                         // reset data ready flag
  }
  
}

//--------------------------------------------------------------------
// Ethernet callback recieve reply and decode time
//--------------------------------------------------------------------

static void my_callback (byte status, word off, word len) {
  
  get_header_line(2,off);                // Get the date and time from the header
  
// Decode date time string to get integers for hour, min, sec, day
  char val[1];
  val[0] = line_buf[23]; val[1] = line_buf[24];
  nanode.hour = atoi(val);
  val[0] = line_buf[26]; val[1] = line_buf[27];
  nanode.mins = atoi(val);
  val[0] = line_buf[29]; val[1] = line_buf[30];
  nanode.sec = atoi(val);

// Send time once a minute
  if (((millis()-timeTX)>(TIMETX_PERIOD*60000)) && (nanode.hour>0 || nanode.mins>0 || nanode.sec>0)) {
    timeTX = millis();
    int i = 0; while (!rf12_canSend() && i<10) {rf12_recvDone(); i++;}
    rf12_sendStart(0, &nanode, sizeof nanode);                        
    rf12_sendWait(0);
    #ifdef DEBUG  
    Serial.print("time sent ");
    if ( nanode.hour < 10 ) { Serial.print('0'); }
    Serial.print(nanode.hour);
    Serial.print(":");
    if ( nanode.mins < 10 ) { Serial.print('0'); }
    Serial.print(nanode.mins);
    Serial.print(":");
    if ( nanode.sec < 10 ) { Serial.print('0'); }
    Serial.print(nanode.sec);
    Serial.println();
    #endif
  }
  
}

// decode reply
int get_header_line(int line,word off) {
  memset(line_buf,NULL,sizeof(line_buf));
  if (off != 0) {
    uint16_t pos = off;
    int line_num = 0;
    int line_pos = 0;
   
    while (Ethernet::buffer[pos]) {
      if (Ethernet::buffer[pos]=='\n') {
        line_num++; line_buf[line_pos] = '\0';
        line_pos = 0;
        if (line_num == line) return 1;
      } else {
        if (line_pos<49) {line_buf[line_pos] = Ethernet::buffer[pos]; line_pos++;}
      }  
      pos++; 
    } 
  }
  return 0;
}


