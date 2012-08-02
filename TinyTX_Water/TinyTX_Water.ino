//----------------------------------------------------------------------------------------------------------------------
// TinyTX_Water - An ATtiny84 and RFM12B Leak/Flood sensor
// By Nathan Chantrell. For hardware design see http://nathan.chantrell.net/tinytx
//
// Wire probes on A0 (ATtiny pin 13) and ground and 100K between A0 and D9 (ATtiny pin 12)
// See TinyTX_Water_RX for a receiver example
//
// Licenced under the Creative Commons Attribution-ShareAlike 3.0 Unported (CC BY-SA 3.0) licence:
// http://creativecommons.org/licenses/by-sa/3.0/
//
// Requires Arduino IDE with arduino-tiny core: http://code.google.com/p/arduino-tiny/
//----------------------------------------------------------------------------------------------------------------------

#include <JeeLib.h> // https://github.com/jcw/jeelib

ISR(WDT_vect) { Sleepy::watchdogEvent(); } // interrupt handler for JeeLabs Sleepy power saving

#define myNodeID 1      // RF12 node ID in the range 1-30
#define network 210      // RF12 Network group
#define freq RF12_433MHZ // Frequency of RFM12B module

int CAL = 400;  // Analogue reading below which will trigger a tx

//########################################################################################################################
//Data Structure to be sent
//########################################################################################################################

 typedef struct {
  	  int alarm;	// alarm status
  	  int supplyV;	// Supply voltage
 } Payload;

 Payload tinytx;

//########################################################################################################################

void setup() {

  rf12_initialize(myNodeID,freq,network); // Initialize RFM12 with settings defined above 
  rf12_sleep(0);                          // Put the RFM12 to sleep

  pinMode(9, OUTPUT); // set D9 to output
  
}

void loop() {

  digitalWrite(9, HIGH); // set D9 high
  delay(10);
  int reading = analogRead(0);
  digitalWrite(9, LOW); // set D9 low

  tinytx.supplyV = readVcc(); // Get supply voltage
  
  if (reading < CAL){  // water detected
  tinytx.alarm = 999;   // set the alarm message
  rfwrite(); // Send data via RF 
  } 
  
  if (tinytx.supplyV < 2500){  // low battery
  tinytx.alarm = 100;   // set the alarm message
  rfwrite(); // Send data via RF 
  } 
  
  Sleepy::loseSomeTime(30000); //JeeLabs power save function: enter low power mode for 30 seconds (valid range 16-65000 ms)
  
}

//--------------------------------------------------------------------------------------------------
// Send payload data via RF
//--------------------------------------------------------------------------------------------------
 static void rfwrite(){
   rf12_sleep(-1);     //wake up RF module
   while (!rf12_canSend())
   rf12_recvDone();
   rf12_sendStart(0, &tinytx, sizeof tinytx); 
   rf12_sendWait(2);    //wait for RF to finish sending while in standby mode
   rf12_sleep(0);    //put RF module to sleep
}

//--------------------------------------------------------------------------------------------------
// Read current supply voltage
//--------------------------------------------------------------------------------------------------
 long readVcc() {
   long result;
   // Read 1.1V reference against Vcc
   ADMUX = _BV(MUX5) | _BV(MUX0);
   delay(2); // Wait for Vref to settle
   ADCSRA |= _BV(ADSC); // Convert
   while (bit_is_set(ADCSRA,ADSC));
   result = ADCL;
   result |= ADCH<<8;
   result = 1126400L / result; // Back-calculate Vcc in mV
   return result;
}

