//----------------------------------------------------------------------------------------------------------------------
// TinyTX_Rain_Gauge - An ATtiny84 Rainfall Monitor
// By Nathan Chantrell. For hardware design see http://nathan.chantrell.net/tinytx
//
// Using a tipping bucket rain gauge with a reed switch across D10 and GND
//
// Licenced under the Creative Commons Attribution-ShareAlike 3.0 Unported (CC BY-SA 3.0) licence:
// http://creativecommons.org/licenses/by-sa/3.0/
//
// Requires Arduino IDE with arduino-tiny core: http://code.google.com/p/arduino-tiny/
//----------------------------------------------------------------------------------------------------------------------

#include <JeeLib.h> // https://github.com/jcw/jeelib
#include <PinChangeInterrupt.h> // http://code.google.com/p/arduino-tiny/downloads/list
#include <avr/sleep.h>

#define myNodeID 3        // RF12 node ID in the range 1-30
#define network 210       // RF12 Network group
#define freq RF12_433MHZ  // Frequency of RFM12B module

#define USE_ACK           // Enable ACKs, comment out to disable
#define RETRY_PERIOD 5    // How soon to retry (in seconds) if ACK didn't come in
#define RETRY_LIMIT 5     // Maximum number of times to retry
#define ACK_TIME 10       // Number of milliseconds to wait for an ack

#define BUCKETMM 0.3      // How many mm of rain for each bucket tip
#define reedPin 10        // Reed switch across D10 (ATtiny pin 13) and GND

//--------------------------------------------------------------------------------------------------
//Data Structure to be sent
//--------------------------------------------------------------------------------------------------

 typedef struct {
  	  int rain;	// Rainfall
  	  int supplyV;	// Supply voltage
 } Payload;

 Payload tinytx;
 
//--------------------------------------------------------------------------------------------------
// Wait for an ACK
//--------------------------------------------------------------------------------------------------
 #ifdef USE_ACK
  static byte waitForAck() {
   MilliTimer ackTimer;
   while (!ackTimer.poll(ACK_TIME)) {
     if (rf12_recvDone() && rf12_crc == 0 &&
        rf12_hdr == (RF12_HDR_DST | RF12_HDR_CTL | myNodeID))
        return 1;
     }
   return 0;
  }
 #endif

//--------------------------------------------------------------------------------------------------
// Send payload data via RF
//-------------------------------------------------------------------------------------------------
 static void rfwrite(){
  #ifdef USE_ACK
   for (byte i = 0; i <= RETRY_LIMIT; ++i) {  // tx and wait for ack up to RETRY_LIMIT times
     rf12_sleep(-1);              // Wake up RF module
      while (!rf12_canSend())
      rf12_recvDone();
      rf12_sendStart(RF12_HDR_ACK, &tinytx, sizeof tinytx); 
      rf12_sendWait(2);           // Wait for RF to finish sending while in standby mode
      byte acked = waitForAck();  // Wait for ACK
      rf12_sleep(0);              // Put RF module to sleep
      if (acked) { return; }      // Return if ACK received
      
   Sleepy::loseSomeTime(RETRY_PERIOD * 1000);     // If no ack received wait and try again
   }
  #else
     rf12_sleep(-1);              // Wake up RF module
     while (!rf12_canSend())
     rf12_recvDone();
     rf12_sendStart(0, &tinytx, sizeof tinytx); 
     rf12_sendWait(2);            // Wait for RF to finish sending while in standby mode
     rf12_sleep(0);               // Put RF module to sleep
     return;
  #endif
 }

//--------------------------------------------------------------------------------------------------
// Read current supply voltage
//--------------------------------------------------------------------------------------------------
 long readVcc() {
   bitClear(PRR, PRADC); ADCSRA |= bit(ADEN); // Enable the ADC
   long result;
   // Read 1.1V reference against Vcc
   #if defined(__AVR_ATtiny84__) 
    ADMUX = _BV(MUX5) | _BV(MUX0); // For ATtiny84
   #else
    ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);  // For ATmega328
   #endif 
   delay(2); // Wait for Vref to settle
   ADCSRA |= _BV(ADSC); // Convert
   while (bit_is_set(ADCSRA,ADSC));
   result = ADCL;
   result |= ADCH<<8;
   result = 1126400L / result; // Back-calculate Vcc in mV
   ADCSRA &= ~ bit(ADEN); bitSet(PRR, PRADC); // Disable the ADC to save power
   return result;
} 
    
//########################################################################################################################

void setup() {
  
  PRR = bit(PRTIM1); // only keep timer 0 going
  
  ADCSRA &= ~ bit(ADEN); bitSet(PRR, PRADC); // Disable the ADC to save power

  rf12_initialize(myNodeID,freq,network); // Initialize RFM12 with settings defined above 
  rf12_sleep(0);                          // Put the RFM12 to sleep

  pinMode(reedPin, INPUT);     // set reed pin as input
  digitalWrite(reedPin, HIGH); // and turn on pullup
  attachPcInterrupt(reedPin,wakeUp,FALLING); // attach a PinChange Interrupt on the falling edge
  
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);         // Set sleep mode
  sleep_mode();                                // Sleep now
  
}

void wakeUp(){}

void loop() {
  
  tinytx.rain = (BUCKETMM * 100); // Send mm per tip as an integer (multiply by 0.01 at receiving end)

  tinytx.supplyV = readVcc(); // Get supply voltage

  rfwrite(); // Send data via RF   
 
  Sleepy::loseSomeTime(1000); 
  
  set_sleep_mode(SLEEP_MODE_PWR_DOWN); // Set sleep mode
  sleep_mode(); // Sleep now
  
}



