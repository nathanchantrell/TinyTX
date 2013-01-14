//----------------------------------------------------------------------------------------------------------------------
// TinyTX_Hall_Effect_Gas - An ATtiny84 Gas Usage Monitoring Node
// By Nathan Chantrell. For hardware design see http://nathan.chantrell.net/tinytx
//
// Simplified version, sends whenever there is a new pulse.
//
// Using an A3214EUA-T or A3213EUA-T Hall Effect Sensor
// Power (pin 1) of sensor to D9, output (pin 3) to D10, 0.1uF capactior across power & ground
//
// Licenced under the Creative Commons Attribution-ShareAlike 3.0 Unported (CC BY-SA 3.0) licence:
// http://creativecommons.org/licenses/by-sa/3.0/
//
// Requires Arduino IDE with arduino-tiny core: http://code.google.com/p/arduino-tiny/
//----------------------------------------------------------------------------------------------------------------------

#include <JeeLib.h> // https://github.com/jcw/jeelib

#define myNodeID 9        // RF12 node ID in the range 1-30
#define network 210       // RF12 Network group
#define freq RF12_433MHZ  // Frequency of RFM12B module

#define USE_ACK           // Enable ACKs, comment out to disable
#define RETRY_PERIOD 5    // How soon to retry (in seconds) if ACK didn't come in
#define RETRY_LIMIT 5     // Maximum number of times to retry
#define ACK_TIME 10       // Number of milliseconds to wait for an ack

#define powerPin 9        // Sensors power pin is on D9 (ATtiny pin 12)
#define sensorPin 10      // Sensors data pin is on D10 (ATtiny pin 13)

byte lastPulse = 0;       // Last reading from sensor
 
//--------------------------------------------------------------------------------------------------
//Data Structure to be sent
//--------------------------------------------------------------------------------------------------

 typedef struct {
  	  int gas;	// Pulses since last TX
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
  
  ADCSRA &= ~ bit(ADEN); bitSet(PRR, PRADC); // Disable the ADC to save power

  rf12_initialize(myNodeID,freq,network); // Initialize RFM12 with settings defined above 
  rf12_sleep(0);                          // Put the RFM12 to sleep

  pinMode(powerPin, OUTPUT);     // set power pin as output
  digitalWrite(powerPin, HIGH);  // and set high

  pinMode(sensorPin, INPUT);     // set sensor pin as input
  digitalWrite(sensorPin, HIGH); // and turn on pullup
  
}

void loop() {
  
  byte newPulse = digitalRead(sensorPin);  // Read sensor pin

  if (newPulse == 0 && lastPulse == 1) { // New pulse detected

    tinytx.gas = 1;  // Each pulse = 0.01 m3/pulse, multiply by 0.01 at receiving end.

    tinytx.supplyV = readVcc(); // Get supply voltage

    rfwrite(); // Send data via RF    
    
  } 
  
  lastPulse = newPulse;

}

