//----------------------------------------------------------------------------------------------------------------------
// TinyTX_LDR_Meter - An ATtiny84 and RFM12B Wireless Electricity Consumption Meter Node
// By Nathan Chantrell. For hardware design see http://nathan.chantrell.net/tinytx
//
// Modified for power meter use by Troels. 
// Using an LDR connected between the center and right sensor pads (D10/pin 13 and GND) and a 4K7 resistor fitted
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

#define myNodeID 4      // RF12 node ID in the range 1-30
#define network 210      // RF12 Network group
#define freq RF12_868MHZ // Frequency of RFM12B module

#define powerPin A0       // LDR Vout connected to A0 (ATtiny pin 13)
int powerReading;         // Analogue reading from the sensor

//########################################################################################################################
//Data Structure to be sent
//########################################################################################################################

 typedef struct {
  	  int power;	// Temperature reading
  	  int supplyV;	// Supply voltage
 } Payload;

 Payload tinytx;

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

  powerReading = analogRead(powerPin); // calculate the average
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
    } else if (interval > 1000) { // 1+ sec passed
        // Blinks are 1000 per kWh, or 1 Wh each
        // One hour has 3.6M milliseconds
        watts = nBlinks * 1 * 3.6E6 / interval;

        last = time;
        nBlinks = 0;
    }

  tinytx.power = watts; // Get realtime power
  
  tinytx.supplyV = readVcc(); // Get supply voltage

  rfwrite(); // Send data via RF    
  
    }

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
