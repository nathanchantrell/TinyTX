//----------------------------------------------------------------------------------------------------------------------
// Geiger Counter Node using Tiny328
// By Nathan Chantrell http://nathan.chantrell.net/
//
// Using the RH Electronics Geiger Counter Kit V3.0
// http://www.rhelectronics.net/store/radiation-detector-geiger-counter-diy-kit-second-edition.html
//
// Licenced under the Creative Commons Attribution-ShareAlike 3.0 Unported (CC BY-SA 3.0) licence:
// http://creativecommons.org/licenses/by-sa/3.0/
//
// Requires Arduino IDE with arduino-tiny core: http://code.google.com/p/arduino-tiny/
//----------------------------------------------------------------------------------------------------------------------

#include <SPI.h>
#include <JeeLib.h> // https://github.com/jcw/jeelib

ISR(WDT_vect) { Sleepy::watchdogEvent(); } // interrupt handler for JeeLabs Sleepy power saving

#define myNodeID 23        // RF12 node ID in the range 1-30
#define network 210       // RF12 Network group
#define freq RF12_433MHZ  // Frequency of RFM12B module

#define USE_ACK           // Enable ACKs, comment out to disable
#define RETRY_PERIOD 5    // How soon to retry (in seconds) if ACK didn't come in
#define RETRY_LIMIT 5     // Maximum number of times to retry
#define ACK_TIME 10       // Number of milliseconds to wait for an ack

#define LOG_PERIOD 60000  //Logging period in milliseconds, recommended value 15000-60000.

#define MAX_PERIOD 60000  //Maximum logging period

unsigned long counts;     //variable for GM Tube events

unsigned long cpm;        //variable for CPM

unsigned int multiplier;  //variable for calculation CPM in this sketch

unsigned long previousMillis; //variable for time measurement

//########################################################################################################################
//Data Structure to be sent
//########################################################################################################################

 typedef struct {
      int cpm; // Temperature reading
      int supplyV;  // Supply voltage
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

void tube_impulse(){               //procedure for capturing events from Geiger Kit

  counts++;

}

void setup() {

  counts = 0;
  cpm = 0;
  multiplier = MAX_PERIOD / LOG_PERIOD;   //calculating multiplier, depend on your log period
  pinMode(3, INPUT);                      // set pin INT1 input for capturing GM Tube events
  digitalWrite(3, HIGH);                  // turn on internal pullup resistors, solder C-INT on the PCB
  attachInterrupt(1, tube_impulse, FALLING); //define external interrupts
 
  rf12_initialize(myNodeID,freq,network); // Initialize RFM12 with settings defined above 
  rf12_sleep(0);                          // Put the RFM12 to sleep
  
  PRR = bit(PRTIM1); // only keep timer 0 going
  
  ADCSRA &= ~ bit(ADEN); bitSet(PRR, PRADC); // Disable the ADC to save power

}

void loop() {
   
  unsigned long currentMillis = millis();

  if(currentMillis - previousMillis > LOG_PERIOD){

    previousMillis = currentMillis;

    cpm = counts * multiplier;

    counts = 0;

    tinytx.cpm = cpm;
  
    tinytx.supplyV = readVcc(); // Get supply voltage

    rfwrite(); // Send data via RF 

  }

}

