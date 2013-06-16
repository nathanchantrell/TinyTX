//----------------------------------------------------------------------------------------------------------------------
// TinyTX - An ATtiny84 and Ciseco SRF Wireless Temperature Sensor Node
// By Nathan Chantrell. For hardware design see http://nathan.chantrell.net/tinytx
//
// Using the Dallas DS18B20 temperature sensor
// and Ciseco SRF Radio: http://shop.ciseco.co.uk/srf-wireless-rf-radio-surface-mount/
// SRF is set to sleep mode 2, see http://zorg.org/cuy
//
// Licenced under the Creative Commons Attribution-ShareAlike 3.0 Unported (CC BY-SA 3.0) licence:
// http://creativecommons.org/licenses/by-sa/3.0/
//
// Requires Arduino IDE with arduino-tiny core: http://code.google.com/p/arduino-tiny/
// and small change to OneWire library, see: http://arduino.cc/forum/index.php/topic,91491.msg687523.html#msg687523
//----------------------------------------------------------------------------------------------------------------------

#include <OneWire.h> // http://www.pjrc.com/teensy/arduino_libraries/OneWire.zip
#include <DallasTemperature.h> // http://download.milesburton.com/Arduino/MaximTemperature/DallasTemperature_LATEST.zip
#include <Narcoleptic.h> // https://code.google.com/p/narcoleptic/
#include <SoftEasyTransfer.h> // https://github.com/madsci1016/Arduino-EasyTransfer/tree/master/SoftEasyTransfer
#include <SoftwareSerial.h>

#define myNodeID 99       // node ID in the range 0-255
#define TX_PIN 2          // SRF DIN pin is connected to D2
#define RX_PIN 0          // SRF DOUT pin is connected to D0 (currently not used)
#define SLEEP_PIN 1       // SRF SLEEP pin is connected to D1

#define ONE_WIRE_BUS 10   // DS18B20 Temperature sensor is connected on D10/ATtiny pin 13
#define ONE_WIRE_POWER 9  // DS18B20 Power pin is connected on D9/ATtiny pin 12

SoftwareSerial srf(RX_PIN, TX_PIN); // RX, TX

OneWire oneWire(ONE_WIRE_BUS); // Setup a oneWire instance

DallasTemperature sensors(&oneWire); // Pass our oneWire reference to Dallas Temperature

SoftEasyTransfer ET;  // Create SoftEasyTransfer object

//########################################################################################################################
//Data Structure to be sent
//########################################################################################################################

 typedef struct {
  	  byte nodeID;	// Sensor Node ID in the range 0 to 255
   	  int temp;	// Temperature reading
  	  int supplyV;	// Supply voltage
 } Payload;

 Payload tinytx;

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

  srf.begin(9600); // initialize serial communication with the SRF at 9600 bits per second:
  
  ET.begin(details(tinytx), &srf); // initialize SoftEasyTransfer
  
  tinytx.nodeID = myNodeID; // set nodeID
  
  pinMode(ONE_WIRE_POWER, OUTPUT); // set power pin for DS18B20 to output
  pinMode(SLEEP_PIN, OUTPUT);      // set sleep pin for SRF to output
  
  PRR = bit(PRTIM1); // only keep timer 0 going
  
  ADCSRA &= ~ bit(ADEN); bitSet(PRR, PRADC); // Disable the ADC to save power

}

void loop() {
  
  digitalWrite(ONE_WIRE_POWER, HIGH); // turn DS18B20 sensor on

  delay(5); // Allow 5ms for the sensor to be ready
 
  sensors.begin(); //start up temp sensor
  sensors.requestTemperatures(); // Get the temperature
  tinytx.temp=(sensors.getTempCByIndex(0)*100); // Read first sensor and convert to integer, reversed at receiving end
  
  digitalWrite(ONE_WIRE_POWER, LOW); // turn DS18B20 off
  
  tinytx.supplyV = readVcc(); // Get supply voltage

  digitalWrite(SLEEP_PIN, LOW);      // turn SRF on
  
  delay(10); // Allow 10ms for the srf to be ready
  
  ET.sendData(); // Send data via RF 

  digitalWrite(SLEEP_PIN, HIGH);     // turn SRF off

  Narcoleptic.delay(60000); // enter low power mode for 60 seconds, valid range 16-32000 ms with standard Narcoleptic lib
                            // change variable for delay from int to unsigned int in Narcoleptic.cpp and Narcoleptic.h
                            // to extend range up to 65000 ms
}

