//----------------------------------------------------------------------------------------------------------------------
// SRF/XRF Module Simple Receive Example
// By Nathan Chantrell. http://zorg.org/
//
// Using a Ciseco XRF to receive an SRF based sensor
//
// Licenced under the Creative Commons Attribution-ShareAlike 3.0 Unported (CC BY-SA 3.0) licence:
// http://creativecommons.org/licenses/by-sa/3.0/
//----------------------------------------------------------------------------------------------------------------------

 #include <SoftEasyTransfer.h> // https://github.com/madsci1016/Arduino-EasyTransfer/tree/master/SoftEasyTransfer
 #include <SoftwareSerial.h>

 #define TX_PIN 3          // XRF DIN pin is connected to D3
 #define RX_PIN 4          // XRF DOUT pin is connected to D4

 SoftwareSerial xrf(RX_PIN, TX_PIN); // RX, TX

 SoftEasyTransfer ET;  // Create SoftEasyTransfer object

// Received payload structure for XRF
  typedef struct {
  	  byte nodeID;	// Sensor Node ID
   	  int temp;	// Temperature reading
  	  int supplyV;	// Supply voltage
 } PayloadXrf;

 PayloadXrf xrfrx;
  
void setup() { 
 Serial.begin(9600);
 xrf.begin(9600); // initialize serial communication with the XRF at 9600 bits per second:
 ET.begin(details(xrfrx), &xrf); // initialize SoftEasyTransfer 
}

void loop() {

 if(ET.receiveData()){ 
   
   Serial.println("Received a packet:");
   Serial.print("From Node: ");
   Serial.println(xrfrx.nodeID);
   Serial.print("Value: ");
   Serial.println(xrfrx.temp);
   Serial.print("TX Millivolts: ");
   Serial.println(xrfrx.supplyV);
   Serial.println("--------------------");

  }

}


