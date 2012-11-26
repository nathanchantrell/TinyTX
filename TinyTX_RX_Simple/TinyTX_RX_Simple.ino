//----------------------------------------------------------------------------------------------------------------------
// TinyTX Simple Receive Example
// By Nathan Chantrell. 
//
// Licenced under the Creative Commons Attribution-ShareAlike 3.0 Unported (CC BY-SA 3.0) licence:
// http://creativecommons.org/licenses/by-sa/3.0/
//----------------------------------------------------------------------------------------------------------------------

#include <JeeLib.h> // https://github.com/jcw/jeelib

// Fixed RF12 settings
#define MYNODE 30            //node ID of the receiever
#define freq RF12_433MHZ     //frequency
#define group 210            //network group

 typedef struct {
          int rxD;              // sensor value
          int supplyV;          // tx voltage
 } Payload;
 Payload rx;

 int nodeID;    //node ID of tx, extracted from RF datapacket. Not transmitted as part of structure

void setup () {

 Serial.begin(9600);
 rf12_initialize(MYNODE, freq,group); // Initialise the RFM12B
 
 Serial.println("TinyTX Simple Receive Example");
 Serial.println("-----------------------------");
 Serial.println("Waiting for data");
 Serial.println(" ");
}

void loop() {

 if (rf12_recvDone() && rf12_crc == 0 && (rf12_hdr & RF12_HDR_CTL) == 0) {
  nodeID = rf12_hdr & 0x1F;  // get node ID
  rx = *(Payload*) rf12_data;
  int value = rx.rxD;
  int millivolts = rx.supplyV;
  
 if (RF12_WANTS_ACK) {                  // Send ACK if requested
   rf12_sendStart(RF12_ACK_REPLY, 0, 0);
   Serial.println("-> ack sent");
 }

  Serial.println("Received a packet:");
  Serial.print("From Node: ");
  Serial.println(nodeID);
  Serial.print("Value: ");
  Serial.println(value);
  Serial.print("TX Millivolts: ");
  Serial.println(millivolts);
  Serial.println("--------------------");
 }

}
