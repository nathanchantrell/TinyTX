//----------------------------------------------------------------------------------------------------------------------
// TinyTX - An ATtiny84 and RFM12B Wireless Temperature & Humidity Sensor Node
// By Nathan Chantrell. For hardware design see http://nathan.chantrell.net/tinytx
//
// Using the DHT22 temperature and humidity sensor
//
// Licenced under the Creative Commons Attribution-ShareAlike 3.0 Unported (CC BY-SA 3.0) licence:
// http://creativecommons.org/licenses/by-sa/3.0/
//
// Requires Arduino IDE with arduino-tiny core: http://code.google.com/p/arduino-tiny/
//----------------------------------------------------------------------------------------------------------------------

#include <DHT22.h> // https://github.com/nathanchantrell/Arduino-DHT22
#include <JeeLib.h> // https://github.com/jcw/jeelib

ISR(WDT_vect) { Sleepy::watchdogEvent(); } // interrupt handler for JeeLabs Sleepy power saving

#define myNodeID 16      // RF12 node ID in the range 1-30
#define network 210      // RF12 Network group
#define freq RF12_433MHZ // Frequency of RFM12B module

#define DHT22_PIN 10     // DHT sensor is connected on D10/ATtiny pin 13
#define DHT22_POWER 9 // DHT Power pin is connected on pin D9/ATtiny pin 12

DHT22 myDHT22(DHT22_PIN); // Setup the DHT

//########################################################################################################################
//Data Structure to be sent
//########################################################################################################################

 typedef struct {
   	  int temp;	// Temperature reading
  	  int supplyV;	// Supply voltage
     	  int humidity;	// Humidity reading
 } Payload;

 Payload tinytx;

//########################################################################################################################

void setup() {

  rf12_initialize(myNodeID,freq,network); // Initialize RFM12 with settings defined above 
  rf12_sleep(0);                          // Put the RFM12 to sleep

  pinMode(DHT22_POWER, OUTPUT); // set power pin for DHT to output
  
}

void loop() {
  
  digitalWrite(DHT22_POWER, HIGH); // turn DHT sensor on

  DHT22_ERROR_t errorCode;
  
  delay(2000); // Sensor requires minimum 2s warm-up after power-on.
  
  errorCode = myDHT22.readData(); // read data from sensor

  if (errorCode == DHT_ERROR_NONE) { // data is good

    tinytx.temp = (myDHT22.getTemperatureC()*100); // Get temperature reading and convert to integer, reversed at receiving end
    
    tinytx.humidity = (myDHT22.getHumidity()*100); // Get humidity reading and convert to integer, reversed at receiving end

    tinytx.supplyV = readVcc(); // Get supply voltage

    rfwrite(); // Send data via RF 

  }

  digitalWrite(DHT22_POWER, LOW); // turn DS18B20 off
  
  Sleepy::loseSomeTime(60000); //JeeLabs power save function: enter low power mode for 60 seconds (valid range 16-65000 ms)
    
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
