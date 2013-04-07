//----------------------------------------------------------------------------------------------------------------------
// TinyTX - An ATtiny84 and BMP085 Wireless Air Pressure/Temperature Sensor Node
// By Nathan Chantrell. For hardware design see http://nathan.chantrell.net/tinytx
//
// Using the BMP085 sensor connected via I2C
// I2C can be connected withf SDA to D8 and SCL to D7 or SDA to D10 and SCL to D9
//
// Licenced under the Creative Commons Attribution-ShareAlike 3.0 Unported (CC BY-SA 3.0) licence:
// http://creativecommons.org/licenses/by-sa/3.0/
//
// Requires Arduino IDE with arduino-tiny core: http://code.google.com/p/arduino-tiny/
//----------------------------------------------------------------------------------------------------------------------

#include <JeeLib.h> // https://github.com/jcw/jeelib
#include <PortsBMP085.h> // Part of JeeLib

ISR(WDT_vect) { Sleepy::watchdogEvent(); } // interrupt handler for JeeLabs Sleepy power saving

#define myNodeID 1        // RF12 node ID in the range 1-30
#define network 210       // RF12 Network group
#define freq RF12_433MHZ  // Frequency of RFM12B module

//#define USE_ACK           // Enable ACKs, comment out to disable
#define RETRY_PERIOD 5    // How soon to retry (in seconds) if ACK didn't come in
#define RETRY_LIMIT 5     // Maximum number of times to retry
#define ACK_TIME 10       // Number of milliseconds to wait for an ack

PortI2C i2c (2);         // BMP085 SDA to D8 and SCL to D7
// PortI2C i2c (1);      // BMP085 SDA to D10 and SCL to D9
BMP085 psensor (i2c, 3); // ultra high resolution
#define BMP085_POWER 9   // BMP085 Power pin is connected on D9

//########################################################################################################################
//Data Structure to be sent
//########################################################################################################################

 typedef struct {
  	  int16_t temp;	// Temperature reading
  	  int supplyV;	// Supply voltage
    	  int32_t pres;	// Pressure reading
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

  pinMode(BMP085_POWER, OUTPUT); // set power pin for BMP085 to output
  digitalWrite(BMP085_POWER, HIGH); // turn BMP085 sensor on
  Sleepy::loseSomeTime(50);
  psensor.getCalibData();
  
  rf12_initialize(myNodeID,freq,network); // Initialize RFM12 with settings defined above 
  rf12_sleep(0);                          // Put the RFM12 to sleep
   
  PRR = bit(PRTIM1); // only keep timer 0 going
  
  ADCSRA &= ~ bit(ADEN); bitSet(PRR, PRADC); // Disable the ADC to save power
  
}

void loop() {
   
  // Get raw temperature reading
  psensor.startMeas(BMP085::TEMP);
  Sleepy::loseSomeTime(16);
  int32_t traw = psensor.getResult(BMP085::TEMP);

  // Get raw pressure reading
  psensor.startMeas(BMP085::PRES);
  Sleepy::loseSomeTime(32);
  int32_t praw = psensor.getResult(BMP085::PRES);
 
  // Calculate actual temperature and pressure
  int32_t press;
  psensor.calculate(tinytx.temp, press);
  tinytx.pres = (press * 0.01);

  tinytx.supplyV = readVcc(); // Get supply voltage

  rfwrite(); // Send data via RF 

  Sleepy::loseSomeTime(60000); //JeeLabs power save function: enter low power mode for 60 seconds (valid range 16-65000 ms)

}
