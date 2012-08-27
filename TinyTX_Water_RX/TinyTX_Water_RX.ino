//----------------------------------------------------------------------------------------------------------------------
// TinyTX Water Sensor Receiver Example
// By Nathan Chantrell. 
//
// Using the TinyTX as a receiver for the TinyTX_Water Sensor
// Could also run on a Nanode/WiNode etc
// Brings D9 high for 5 sec on receipt of water alarm and D10 high on low battery
//
// Licenced under the Creative Commons Attribution-ShareAlike 3.0 Unported (CC BY-SA 3.0) licence:
// http://creativecommons.org/licenses/by-sa/3.0/
//----------------------------------------------------------------------------------------------------------------------

//#define DEBUG     //uncomment enable serial printing

#include <JeeLib.h> // https://github.com/jcw/jeelib

// Fixed RF12 settings
#define MYNODE 30            //node ID of the receiever
#define freq RF12_433MHZ     //frequency
#define group 210            //network group
#define tinyTXNode 1         // node ID of the tinyTX to watch

 typedef struct {
          int rxD;              // sensor value
          int supplyV;          // tx voltage
 } Payload;
 Payload rx;

 int nodeID;    //node ID of tx, extracted from RF datapacket. Not transmitted as part of structure

void setup () {

 #ifdef DEBUG
 Serial.begin(9600);
 #endif
 
 rf12_initialize(MYNODE, freq,group); // Initialise the RFM12B

 pinMode(9, OUTPUT); // set D9 to output
 pinMode(10, OUTPUT); // set D10 to output

 #ifdef DEBUG 
 Serial.println("TinyTX Water Sensor Receiver");
 Serial.println("----------------------------");
 Serial.println("Waiting for an alarm");
 Serial.println(" ");
 #endif 
 
}

void loop() {

 if (rf12_recvDone() && rf12_crc == 0 && (rf12_hdr & RF12_HDR_CTL) == 0) {
  nodeID = rf12_hdr & 0x1F;  // get node ID

  if (nodeID == tinyTXNode) {
   rx = *(Payload*) rf12_data;
   
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
   
   int alarm = rx.rxD;
   int millivolts = rx.supplyV;

   #ifdef DEBUG
   Serial.print("Received an alarm from Node: ");
   Serial.println(nodeID);
   #endif
   if (alarm == 999) {
   digitalWrite(9, HIGH); // set D9 high
   #ifdef DEBUG
   Serial.println("Water detected!");
   #endif
   } else if (alarm == 100) {
   digitalWrite(10, HIGH); // set D10 high
   #ifdef DEBUG
   Serial.println("Battery Low!");  
   #endif
   }   
   #ifdef DEBUG
   Serial.print("TX Millivolts: ");
   Serial.println(millivolts);
   Serial.println("--------------------");
   #endif
   delay(5000); // reset outputs after 5000ms
   digitalWrite(9, LOW); // set D9 low
   digitalWrite(10, LOW); // set D10 low
  }

 }

}
