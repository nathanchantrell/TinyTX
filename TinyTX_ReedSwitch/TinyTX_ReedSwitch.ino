//----------------------------------------------------------------------------------------------------------------------
// TinyTX - An ATtiny84 and RFM12B Wireless Sensor Node
// By Nathan Chantrell. For hardware design see http://nathan.chantrell.net/tinytx
//
// Using a reed switch to trigger a pin change interrupt to wake the sensor
//
// Licenced under the Creative Commons Attribution-ShareAlike 3.0 Unported (CC BY-SA 3.0) licence:
// http://creativecommons.org/licenses/by-sa/3.0/
//
// Requires Arduino IDE with arduino-tiny core: http://code.google.com/p/arduino-tiny/
// and small change to OneWire library, see: http://arduino.cc/forum/index.php/topic,91491.msg687523.html#msg687523
//----------------------------------------------------------------------------------------------------------------------

#include <JeeLib.h> // https://github.com/jcw/jeelib
#include <PinChangeInterrupt.h> // http://code.google.com/p/arduino-tiny/downloads/list
#include <avr/sleep.h>

ISR(WDT_vect) { Sleepy::watchdogEvent(); } // interrupt handler for JeeLabs Sleepy power saving

#define myNodeID 1        // RF12 node ID in the range 1-30
#define network 210       // RF12 Network group
#define freq RF12_433MHZ  // Frequency of RFM12B module
#define RETRY_PERIOD 5    // How soon to retry (in seconds) if ACK didn't come in
#define RETRY_LIMIT 5     // Maximum number of times to retry
#define ACK_TIME 10       // Number of milliseconds to wait for an ack

#define SW_PIN 10         // Pin the switch is on D10/ATtiny pin 13

//########################################################################################################################
//Data Structure to be sent
//########################################################################################################################

 typedef struct {
  	  int switchT;	// Switch Trigger
  	  int supplyV;	// Supply voltage
 } Payload;

 Payload tinytx;

//########################################################################################################################

void setup() {

  rf12_initialize(myNodeID,freq,network); // Initialize RFM12 with settings defined above 
  rf12_sleep(0);                          // Put the RFM12 to sleep
  
  pinMode(SW_PIN, INPUT);                   //set the pin to input
  digitalWrite(SW_PIN, HIGH);               //use the internal pullup resistor
  attachPcInterrupt(SW_PIN,wakeUp,FALLING); // attach a PinChange Interrupt on the falling edge
  
  PRR = bit(PRTIM1); // only keep timer 0 going
  
  ADCSRA &= ~ bit(ADEN); bitSet(PRR, PRADC);   // Disable the ADC to save power

  set_sleep_mode(SLEEP_MODE_PWR_DOWN);         // Set sleep mode
  sleep_mode();                                // Sleep now

}

void wakeUp(){}

void loop() {
  
  tinytx.switchT = millis(); // testing
   
  tinytx.supplyV = readVcc(); // Get supply voltage

  rfwrite(); // Send data via RF 

  set_sleep_mode(SLEEP_MODE_PWR_DOWN); // Set sleep mode
  sleep_mode(); // Sleep now

}

//--------------------------------------------------------------------------------------------------
// Send payload data via RF
//-------------------------------------------------------------------------------------------------
 static void rfwrite(){
   for (byte i = 0; i <= RETRY_LIMIT; ++i) {  // tx and wait for ack up to RETRY_LIMIT times
     rf12_sleep(-1);              // Wake up RF module
      while (!rf12_canSend())
      rf12_recvDone();
      rf12_sendStart(RF12_HDR_ACK, &tinytx, sizeof tinytx); 
      rf12_sendWait(2);           // Wait for RF to finish sending while in standby mode
      byte acked = waitForAck();
      rf12_sleep(0);              // Put RF module to sleep
      if (acked) { return; }
  
   Sleepy::loseSomeTime(RETRY_PERIOD * 1000);     // If no ack received wait and try again
   }
 }

// Wait a few milliseconds for proper ACK
 static byte waitForAck() {
   MilliTimer ackTimer;
   while (!ackTimer.poll(ACK_TIME)) {
     if (rf12_recvDone() && rf12_crc == 0 &&
        rf12_hdr == (RF12_HDR_DST | RF12_HDR_CTL | myNodeID))
        return 1;
     }
   return 0;
 }

//--------------------------------------------------------------------------------------------------
// Read current supply voltage
//--------------------------------------------------------------------------------------------------
 long readVcc() {
   bitClear(PRR, PRADC); ADCSRA |= bit(ADEN); // Enable the ADC
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
   ADCSRA &= ~ bit(ADEN); bitSet(PRR, PRADC); // Disable the ADC to save power
} 
