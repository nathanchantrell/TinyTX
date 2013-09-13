//----------------------------------------------------------------------------------------------------------------------
// TinyTX_LDR_Meter - An ATtiny84 and RFM12B Wireless Electricity Consumption Meter Node
// By Nathan Chantrell. For hardware design see http://nathan.chantrell.net/tinytx
//
// Modified for power meter use by Troels. 
// - LDR connected between A0/(D10) (ATtiny pin 13) and ground
// - 4K7 resistor between A0/(D10) and D9 (ATtiny pin 12)
//
// Licenced under the Creative Commons Attribution-ShareAlike 3.0 Unported (CC BY-SA 3.0) licence:
// http://creativecommons.org/licenses/by-sa/3.0/
//
// Requires Arduino IDE with arduino-tiny core: http://code.google.com/p/arduino-tiny/
//----------------------------------------------------------------------------------------------------------------------

#include <JeeLib.h> // https://github.com/jcw/jeelib

ISR(WDT_vect) { Sleepy::watchdogEvent(); } // interrupt handler for JeeLabs Sleepy power saving

static unsigned long last;
static unsigned long watts;

#define myNodeID 4       // RF12 node ID in the range 1-30
#define network 210      // RF12 Network group
#define freq RF12_433MHZ // Frequency of RFM12B module

#define USE_ACK           // Enable ACKs, comment out to disable
#define RETRY_PERIOD 5    // How soon to retry (in seconds) if ACK didn't come in
#define RETRY_LIMIT 5     // Maximum number of times to retry
#define ACK_TIME 10       // Number of milliseconds to wait for an ack

#define powerPin A0       // LDR Vout connected to A0 (ATtiny pin 13)
int powerReading;         // Analogue reading from the sensor
int sendInterval = 1;	  // Defines min. interval between sending of data in seconds
			  // (Small number = High accuracy. Larger number = less power hungry)

//########################################################################################################################
//Data Structure to be sent
//########################################################################################################################

 typedef struct {
  	  int power;	// Temperature reading
  	  int supplyV;	// Supply voltage
 } Payload;

 Payload tinytx;

// Wait a few milliseconds for proper ACK
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
     rf12_sendWait(2);           // Wait for RF to finish sending while in standby mode
     rf12_sleep(0);              // Put RF module to sleep
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

  rf12_initialize(myNodeID,freq,network); // Initialize RFM12 with settings defined above 
  rf12_sleep(0);                          // Put the RFM12 to sleep

  analogReference(INTERNAL);  // Set the aref to the internal 1.1V reference
  last = millis();

  pinMode(9, OUTPUT); // set D9/ATtiny pin 12 as output
  digitalWrite(9, HIGH); // and set high
  
}

void loop() {

  bitClear(PRR, PRADC); ADCSRA |= bit(ADEN); // Enable the ADC
  powerReading = analogRead(powerPin); // calculate the average
  ADCSRA &= ~ bit(ADEN); bitSet(PRR, PRADC); // Disable the ADC to save power

  static boolean ledOn = false;  

    if (!ledOn && powerReading < 1010) {
        ledOn = true;
    } else if (ledOn && powerReading > 1020) {
        ledOn = false;

    static int nBlinks = 0;
    unsigned long time = millis();
    unsigned long interval = time - last;

    nBlinks++;
    if (interval < 0) { // millis() overflow
        last = time;
        nBlinks = 0;
    } else if (interval > 1000 * sendInterval) {
        // Blinks are 1000 per kWh, or 1 Wh each
        // One hour has 3.6M milliseconds
        watts = nBlinks * 1 * 3.6E6 / interval;

        last = time;
        nBlinks = 0;

  tinytx.power = watts; // Get realtime power
  
  tinytx.supplyV = readVcc(); // Get supply voltage

  rfwrite(); // Send data via RF    
    }
  
    }

}
