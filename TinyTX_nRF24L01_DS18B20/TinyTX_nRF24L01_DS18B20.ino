//----------------------------------------------------------------------------------------------------------------------
// An ATtiny84 and nRF24L01 Wireless Temperature Sensor Node
// By Nathan Chantrell. http://nathan.chantrell.net/
//
// Using the Dallas DS18B20 temperature sensor
//
// Licenced under the Creative Commons Attribution-ShareAlike 3.0 Unported (CC BY-SA 3.0) licence:
// http://creativecommons.org/licenses/by-sa/3.0/
//
// Requires Arduino IDE with arduino-tiny core: http://code.google.com/p/arduino-tiny/
// and small change to OneWire library, see: http://arduino.cc/forum/index.php/topic,91491.msg687523.html#msg687523
// Uses forked version of Mirf library at https://github.com/stanleyseow/attiny-nRF24L01
//----------------------------------------------------------------------------------------------------------------------

// nRF24L01 Library from https://github.com/stanleyseow/attiny-nRF24L01
#include <SPI85.h>
#include <Mirf85.h>
#include <MirfHardwareSpiDriver85.h>

#include <OneWire.h> // http://www.pjrc.com/teensy/arduino_libraries/OneWire.zip
#include <DallasTemperature.h> // http://download.milesburton.com/Arduino/MaximTemperature/DallasTemperature_LATEST.zip
#include <Narcoleptic.h> // https://code.google.com/p/narcoleptic/

#define myNodeID 1        // node ID 
#define ONE_WIRE_BUS 10   // DS18B20 Temperature sensor is connected on D10
#define ONE_WIRE_POWER 9  // DS18B20 Power pin is connected on D9

OneWire oneWire(ONE_WIRE_BUS); // Setup a oneWire instance

DallasTemperature sensors(&oneWire); // Pass our oneWire reference to Dallas Temperature

//########################################################################################################################
//Data Structure to be sent
//########################################################################################################################
 
 typedef struct {
          byte nodeID;  // nodeID
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
 
  Mirf.cePin = 8;  // CE on D8
  Mirf.csnPin = 7; // CSN on D7
  Mirf.spi = &MirfHardwareSpi;
  Mirf.init();    
  Mirf.channel = 0x5a; // Channel number = 90. Range 0 - 127 or 0 - 84 in the US.
  Mirf.rfsetup = 0x27; // 250kpbs full power
  byte RADDR[] = {0xe2, 0xf0, 0xf0, 0xf0, 0xf0};
  byte TADDR[] = {0xe3, 0xf0, 0xf0, 0xf0, 0xf0};
  Mirf.setRADDR(RADDR);  
  Mirf.setTADDR(TADDR);
  Mirf.config();
  
  // Enable dynamic payload 
 Mirf.configRegister( FEATURE, 1<<EN_DPL ); 
 Mirf.configRegister( DYNPD, 1<<DPL_P0 | 1<<DPL_P1 | 1<<DPL_P2 | 1<<DPL_P3 | 1<<DPL_P4 | 1<<DPL_P5 ); 
  
  
  pinMode(ONE_WIRE_POWER, OUTPUT); // set power pin for DS18B20 to output
  
  PRR = bit(PRTIM1); // only keep timer 0 going 
  
  ADCSRA &= ~ bit(ADEN); bitSet(PRR, PRADC); // Disable the ADC to save power

}

void loop() {
  
  digitalWrite(ONE_WIRE_POWER, HIGH); // turn DS18B20 sensor on
  digitalWrite(ONE_WIRE_GND, LOW); // set GND pin low
  
  delay(5); // Allow 5ms for the sensor to be ready

  tinytx.nodeID = myNodeID;

  sensors.begin(); //start up temp sensor
  sensors.requestTemperatures(); // Get the temperature
  tinytx.temp=(sensors.getTempCByIndex(0)*100); // Read first sensor and convert to integer, reversed at receiving end
  
  digitalWrite(ONE_WIRE_POWER, LOW); // turn DS18B20 off
  
  tinytx.supplyV = readVcc(); // Get supply voltage

  Mirf.payload = sizeof (tinytx);
  Mirf.send((byte *) &tinytx); 

  while( Mirf.isSending() ) {
    delay(1);
  }

 Narcoleptic.delay(60000); // enter low power mode for 60 seconds, valid range 16-32000 ms with standard Narcoleptic lib
                            // change variable for delay from int to unsigned int in Narcoleptic.cpp and Narcoleptic.h
                            // to extend range up to 65000 ms
    
}

