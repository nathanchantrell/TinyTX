//----------------------------------------------------------------------------------------------------------------------
// OOK RF Module Simple Receive Example
// By Nathan Chantrell. http://zorg.org/
//
// Using a cheap ASK/OOK receiver
//
// Licenced under the Creative Commons Attribution-ShareAlike 3.0 Unported (CC BY-SA 3.0) licence:
// http://creativecommons.org/licenses/by-sa/3.0/
//----------------------------------------------------------------------------------------------------------------------

#include <MANCHESTER.h>   // https://github.com/mchr3k/arduino-libs-manchester

#define RxPin 4   // Pin OOK Receivers data pin is connected to

// Data structure to receive
 typedef struct {
          int nodeID;          // sensor node ID
          int rxD;              // sensor value
          int supplyV;          // tx voltage
 } Payload;
 Payload rx;

unsigned char databuf[6];
unsigned char* data = databuf;
  
void setup() { 
 Serial.begin(9600);
 MANRX_SetRxPin(RxPin);   // Set rx pin
 MANRX_SetupReceive();    // Prepare interrupts
 MANRX_BeginReceive();    // Begin receiving data
}

void loop() {

 if (MANRX_ReceiveComplete()) {      
   
    unsigned char receivedSize;
    unsigned char* msgData;
    MANRX_GetMessageBytes(&receivedSize, &msgData);
    MANRX_BeginReceiveBytes(6, data);
  
   rx = *(Payload*) data;
   
   if (rx.nodeID != 0) {
   Serial.println("Received a packet:");
   Serial.print("From Node: ");
   Serial.println(rx.nodeID);
   Serial.print("Value: ");
   Serial.println(rx.rxD);
   Serial.print("TX Millivolts: ");
   Serial.println(rx.supplyV);
   Serial.println("--------------------");
   }

  }

}


