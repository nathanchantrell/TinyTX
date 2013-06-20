//----------------------------------------------------------------------------------------------------------------------
// TinyTX - An ATtiny84 and OOK RF Module Wireless Temperature Sensor Node
// By Nathan Chantrell. http://nathan.chantrell.net/tinytx
//
// Using the Dallas DS18B20 temperature sensor and a cheap ASK/OOK transmitter
//
// Licenced under the Creative Commons Attribution-ShareAlike 3.0 Unported (CC BY-SA 3.0) licence:
// http://creativecommons.org/licenses/by-sa/3.0/
//
// Requires Arduino IDE with arduino-tiny core: http://code.google.com/p/arduino-tiny/
// and small change to OneWire library, see: http://arduino.cc/forum/index.php/topic,91491.msg687523.html#msg687523
//----------------------------------------------------------------------------------------------------------------------

#include <MANCHESTER.h> // https://github.com/mchr3k/arduino-libs-manchester
#include <OneWire.h> // http://www.pjrc.com/teensy/arduino_libraries/OneWire.zip
#include <DallasTemperature.h> // http://download.milesburton.com/Arduino/MaximTemperature/DallasTemperature_LATEST.zip
#include <Narcoleptic.h> // https://code.google.com/p/narcoleptic/

#define myNodeID 90  // node ID
#define TX_PIN 5     // Transmitter data is connected to D5
#define TX_POWER 6   // Transmitter power is connected to D6
#define TX_GND 1     // Transmitter power is connected to D1

#define ONE_WIRE_BUS 10   // DS18B20 Temperature sensor is connected on D10
#define ONE_WIRE_POWER 9  // DS18B20 Power pin is connected on D9

OneWire oneWire(ONE_WIRE_BUS); // Setup a oneWire instance

DallasTemperature sensors(&oneWire); // Pass our oneWire reference to Dallas Temperature

//########################################################################################################################
//Data Structure to be sent
//########################################################################################################################

 typedef struct {
  	  int nodeID;	// Sensor Node ID
   	  int temp;	// Temperature reading
  	  int supplyV;	// Supply voltage
 } Payload;

 Payload tinytx;
 
//--------------------------------------------------------------------------------------------------
// Send payload data via RF
//-------------------------------------------------------------------------------------------------

 static void rfwrite(){
  digitalWrite(TX_POWER, HIGH); // turn transmitter on
  delay(10); // Allow 10ms for tx to be ready
  tinytx.nodeID=myNodeID;
  MANCHESTER.TransmitBytes(sizeof tinytx, (uint8_t*)&tinytx);
  digitalWrite(TX_POWER, LOW); // turn transmitter off
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
 
void setup()
{
  pinMode(ONE_WIRE_POWER, OUTPUT);   // set power pin for DS18B20 to output
  pinMode(TX_POWER, OUTPUT);         // set power pin for transmitter to output
  pinMode(TX_GND, OUTPUT);           // set power pin for transmitter to output
  digitalWrite(TX_GND, LOW);         // set transmitter GND low

  MANCHESTER.SetTxPin(TX_PIN);       // sets the pin for the transmitter
  
  PRR = bit(PRTIM1); // only keep timer 0 going
  
  ADCSRA &= ~ bit(ADEN); bitSet(PRR, PRADC); // Disable the ADC to save power
}

void loop()
{  
  
  digitalWrite(ONE_WIRE_POWER, HIGH); // turn DS18B20 sensor on

  delay(5); // Allow 5ms for the sensor to be ready
 
  sensors.begin(); //start up temp sensor
  sensors.requestTemperatures(); // Get the temperature
  tinytx.temp=(sensors.getTempCByIndex(0)*100); // Read first sensor and convert to integer, reversed at receiving end
  
  digitalWrite(ONE_WIRE_POWER, LOW); // turn DS18B20 off
  
  tinytx.supplyV = readVcc(); // Get supply voltage
    
  rfwrite(); // Send data via RF 
  
  Narcoleptic.delay(10000); // enter low power mode for 60 seconds, valid range 16-32767 ms with standard Narcoleptic lib
                            // change variable for delay from int to unsigned int in Narcoleptic.cpp and Narcoleptic.h
                            // to extend range up to 65535 ms

}


