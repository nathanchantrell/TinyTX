//--------------------------------------------------------------------------------------
// TinyTX NanodeRF Cosm Relay Example
// Receives data from one TinyTX node and uploads to http://cosm.com/
// Based on NanodeRF_singleCT_Pachube.ino by Trystan Lea, Glyn Hudson and Igor Dutra
// Licenced under GNU GPL V3
//--------------------------------------------------------------------------------------

#define DEBUG     //comment out to disable serial printing to increase long term stability 

#include <JeeLib.h>	     // https://github.com/jcw/jeelib
#include <EtherCard.h>	     // https://github.com/jcw/ethercard 

// Fixed RF12 settings
#define MYNODE 30            // node ID 30 reserved for base station
#define freq RF12_433MHZ     // frequency
#define group 210            // network group 
#define tinyTXNode 27        // node ID of the tinyTX to upload

// Cosm settings, change these settings to match your own setup
#define FEED    "xxxxx"
#define APIKEY  "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx" 

// The RF12 data payload
typedef struct {
  	  int rxD;		// sensor value
	  int supplyV;		// tx voltage
} Payload;
Payload rx;      

//---------------------------------------------------------------------
// The PacketBuffer class is used to generate the json string that is send via ethernet - JeeLabs
//---------------------------------------------------------------------
class PacketBuffer : public Print {
public:
    PacketBuffer () : fill (0) {}
    const char* buffer() { return buf; }
    byte length() { return fill; }
    void reset()
    { 
      memset(buf,NULL,sizeof(buf));
      fill = 0; 
    }
    virtual size_t write (uint8_t ch)
        { if (fill < sizeof buf) buf[fill++] = ch; }
    byte fill;
    char buf[150];
    private:
};
PacketBuffer str;

//--------------------------------------------------------------------------
// Ethernet
//--------------------------------------------------------------------------

// ethernet interface mac address, must be unique on the LAN
static byte mymac[] = { 0x44,0x29,0x49,0x21,0x30,0x31 };

byte Ethernet::buffer[700];
static uint32_t timer;
Stash stash;

//Domain name of remote webserver 
char website[] PROGMEM = "api.cosm.com";

//--------------------------------------------------------------------------
// Flow control varaiables
int dataReady=0;                         // is set to 1 when there is data ready to be sent
unsigned long lastRF;                    // used to check for RF recieve failures
int post_count;                          // used to count number of ethernet posts that dont recieve a reply
int dhcp_count =0;

//NanodeRF error indication LED variables 
const int redLED=6;                      //NanodeRF RED indicator LED
const int greenLED=5;                    //NanodeRF GREEN indicator LED
int error=0;                             //Etherent (controller/DHCP) error flag
int RFerror=0;                           //RF error flag - high when no data received 

int dhcp_status = 0;
int dns_status = 0;
int request_attempt = 0;

char line_buf[50];

//-----------------------------------------------------------------------------------
// Ethernet callback
// recieve reply and decode
//-----------------------------------------------------------------------------------
static void my_callback (byte status, word off, word len) {
  
  get_header_line(1,off);      // Get the date and time from the header
  Serial.println(line_buf);    // Print out the date and time

      if (strcmp(line_buf,"HTTP/1.1 200 OK")) {Serial.println("ok recieved"); request_attempt = 0;}
   
}

//**********************************************************************************************************************
// SETUP
//**********************************************************************************************************************
void setup () {
  
  //Nanode RF LED indictor  setup - green flashing means good - red on for a long time means bad! 
  //High means off since NanodeRF tri-state buffer inverts signal 
  pinMode(redLED, OUTPUT); digitalWrite(redLED,LOW);            
  pinMode(greenLED, OUTPUT); digitalWrite(greenLED,LOW);       
  delay(100); digitalWrite(redLED,HIGH);                        //turn off redLED
  
  Serial.begin(9600);
  Serial.println("\n[Cosm Relay]");

  if (ether.begin(sizeof Ethernet::buffer, mymac) == 0) {
    Serial.println( "Failed to access Ethernet controller");
    error=1;  
  }

  dhcp_status = 0;
  dns_status = 0;
  request_attempt = 0;
  error=0;
 
  rf12_initialize(MYNODE, freq,group);
  lastRF = millis()-40000;                                        // setting lastRF back 40s is useful as it forces the ethernet code to run straight away
   
  digitalWrite(greenLED,HIGH);                                    //Green LED off - indicate that setup has finished
 
  #ifdef UNO
  wdt_enable(WDTO_8S); 
  #endif;
  
}
//**********************************************************************************************************************


//**********************************************************************************************************************
// LOOP
//**********************************************************************************************************************
void loop () {
  
  #ifdef UNO
  wdt_reset();
  #endif
  
  //-----------------------------------------------------------------------------------
  // Get DHCP address
  // Putting DHCP setup and DNS lookup in the main loop allows for: 
  // powering nanode before ethernet is connected
  //-----------------------------------------------------------------------------------
  // OLD Lib: if (ether.dhcpExpired()) dhcp_status = 0;    // if dhcp expired start request for new lease by changing status
  if (!ether.dhcpValid()) dhcp_status = 0;    // if dhcp expired start request for new lease by changing status
  
  if (!dhcp_status){
    
    #ifdef UNO
    wdt_disable();
    #endif 
    
    dhcp_status = ether.dhcpSetup();           // DHCP setup
    
    #ifdef UNO
    wdt_enable(WDTO_8S);
    #endif
    
    Serial.print("DHCP status: ");             // print
    Serial.println(dhcp_status);               // dhcp status
    
    if (dhcp_status){                          // on success print out ip's
      ether.printIp("IP:  ", ether.myip);
      ether.printIp("GW:  ", ether.gwip);  
      
      static byte dnsip[] = {8,8,8,8};  
      ether.copyIp(ether.dnsip, dnsip);
      ether.printIp("DNS: ", ether.dnsip);                            //comment out this line if posting to a static IP server this includes local host           
    } else { error=1; }  
  }
  
  //-----------------------------------------------------------------------------------
  // Get server address via DNS
  //-----------------------------------------------------------------------------------
  if (dhcp_status && !dns_status){
    
    #ifdef UNO
    wdt_disable();
    #endif 
    
    dns_status = ether.dnsLookup(website);    // Attempt DNS lookup
    
    #ifdef UNO
    wdt_enable(WDTO_8S);
    #endif;
    
    Serial.print("DNS status: ");             // print
    Serial.println(dns_status);               // dns status
    if (dns_status){
      ether.printIp("SRV: ", ether.hisip);      // server ip
    } else { error=1; }  
  }
  
  if (error==1 || RFerror==1) digitalWrite(redLED,LOW);      //turn on red LED if RF / DHCP or Etherent controllor error. Need way to notify of server error
    else digitalWrite(redLED,HIGH);

  //---------------------------------------------------------------------
  // On data receieved from rf12
  //---------------------------------------------------------------------
  if (rf12_recvDone() && rf12_crc == 0 && (rf12_hdr & RF12_HDR_CTL) == 0) 
  {
    digitalWrite(greenLED,LOW);                                   // turn green LED on to indicate RF recieve 
    rx=*(Payload*) rf12_data;                                 // Get the payload
    int nodeID = rf12_hdr & 0x1F;                             //extract node ID from received packet
    
    RFerror=0;                                                    //reset RF error flag

    #ifdef DEBUG 
      Serial.print("RF recieved from node: ");
      Serial.print(nodeID);
      Serial.print(", Value: ");
      Serial.print(rx.rxD);
      Serial.print(", Millivolts: ");
      Serial.println(rx.supplyV);
    #endif
    
    if (nodeID == tinyTXNode) {dataReady = 1;}   // OK, send data          
    lastRF = millis();                                            // reset lastRF timer
    digitalWrite(greenLED,HIGH);                                  // Turn green LED on OFF
    
  }
    
   
  ether.packetLoop(ether.packetReceive());
  
  if (dataReady){

    #ifdef DEBUG 
      Serial.println("Sending to Cosm");
    #endif
    
     // Cosm, generate two fake values as payload - by using a separate stash,
    // we can determine the size of the generated message ahead of time
    byte sd = stash.create();
    stash.print("0,");              //datasteam name
    stash.println(rx.rxD); 
    stash.print("1,");              //datasteam name
    stash.println(rx.supplyV);
    stash.save();
    
    // generate the header with payload - note that the stash size is used,
    // and that a "stash descriptor" is passed in as argument using "$H"
    request_attempt ++;
    Stash::prepare(PSTR("PUT http://$F/v2/feeds/$F.csv HTTP/1.0" "\r\n"
                        "Host: $F" "\r\n"
                        "X-PachubeApiKey: $F" "\r\n"
                        "Content-Length: $D" "\r\n"
                        "\r\n"
                        "$H"),
            website, PSTR(FEED), website, PSTR(APIKEY), stash.size(), sd);

    // send the packet - this also releases all stash buffers once done
    ether.tcpSend();
    Serial.println((char*) ether.buffer);

    // END Cosm  
    
    dataReady =0;
  }
   
}

//--------------------------------------------------------------------------
// Decode reply stuff
//--------------------------------------------------------------------------

int get_header_line(int line,word off)
{
  memset(line_buf,NULL,sizeof(line_buf));
  if (off != 0)
  {
    uint16_t pos = off;
    int line_num = 0;
    int line_pos = 0;
    
    while (Ethernet::buffer[pos])
    {
      if (Ethernet::buffer[pos]=='\n')
      {
        line_num++; line_buf[line_pos] = '\0';
        line_pos = 0;
        if (line_num == line) return 1;
      }
      else
      {
        if (line_pos<49) {line_buf[line_pos] = Ethernet::buffer[pos]; line_pos++;}
      }  
      pos++; 
    } 
  }
  return 0;
}

int get_reply_data(word off)
{
  memset(line_buf,NULL,sizeof(line_buf));
  if (off != 0)
  {
    uint16_t pos = off;
    int line_num = 0;
    int line_pos = 0;
    
    // Skip over header until data part is found
    while (Ethernet::buffer[pos]) {
      if (Ethernet::buffer[pos-1]=='\n' && Ethernet::buffer[pos]=='\r') break;
      pos++; 
    }
    pos+=4;
    while (Ethernet::buffer[pos])
    {
      if (line_pos<49) {line_buf[line_pos] = Ethernet::buffer[pos]; line_pos++;} else break;
      pos++; 
    }
    line_buf[line_pos] = '\0';
  }
  return 0;
}

