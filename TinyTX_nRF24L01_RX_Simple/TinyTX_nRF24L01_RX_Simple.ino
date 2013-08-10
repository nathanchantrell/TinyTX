//----------------------------------------------------------------------------------------------------------------------
// Simple receive example for nRF24L01 Wireless Node
// By Nathan Chantrell. http://nathan.chantrell.net/
//
//
// Licenced under the Creative Commons Attribution-ShareAlike 3.0 Unported (CC BY-SA 3.0) licence:
// http://creativecommons.org/licenses/by-sa/3.0/
//----------------------------------------------------------------------------------------------------------------------


#include <SPI.h>
#include "nRF24L01.h" // https://github.com/stanleyseow/RF24
#include "RF24.h"

RF24 radio(9,10);

typedef struct {
         byte nodeID;   // nodeID
         int rxD;	// Sensor value
 	 int supplyV;	// Supply voltage
} Payload;
 
Payload rx;
  
void setup () {
   
  Serial.begin(57600);

  radio.begin();
  radio.setDataRate(RF24_250KBPS); // Data rate = 250kbps
  radio.setPALevel(RF24_PA_MAX);   // Output power = Max
  radio.setChannel(90);            // Channel number = 90. Range 0 - 127 or 0 - 84 in the US.
  radio.enableDynamicPayloads();
  radio.setCRCLength(RF24_CRC_16);
  radio.startListening();
   
 Serial.println("nRF24L01 Simple Receive Example");
 Serial.println("-----------------------------");
 Serial.println("Waiting for data");
 Serial.println(" ");   

}

void loop () { 

 if ( radio.available() ) {
   bool done = false;
   while (!done) {

    done = radio.read( &rx, sizeof(rx) );

    rx = *(Payload*) &rx;
    int value = rx.rxD;
    int millivolts = rx.supplyV;
    int nodeID = rx.nodeID;
  
    Serial.println("Received a packet:");
    Serial.print("From Node: ");
    Serial.println(nodeID);
    Serial.print("Value: ");
    Serial.println(value);
    Serial.print("TX Millivolts: ");
    Serial.println(millivolts);
    Serial.println("--------------------");
  
    done = true;
 
   }

 }

}
