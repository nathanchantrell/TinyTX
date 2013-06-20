#TinyTX - An ATtiny84 and RFM12B wireless sensor node

By Nathan Chantrell. For hardware design see http://nathan.chantrell.net/tinytx

Licenced under the Creative Commons Attribution-ShareAlike 3.0 Unported (CC BY-SA 3.0) licence: http://creativecommons.org/licenses/by-sa/3.0/

# Code for TinyTX nodes

##TinyTX_DS18B20
With DS18B20 temperature sensor. 


##TinyTX_DHT22
With DHT22 temperature & humidity sensor.


##TinyTX_TMP36
With TMP36 Analogue Temperature Sensor.


##TinyTX_LDR_Meter  
With LDR to read LED pulses on electricity meter.


##TinyTX_LDR 
With LDR to read light levels.


##TinyTX_ReedSwitch
Detect a normally closed reed switch opening and closing with pin change interrupt to wake from sleep.


##TinyTX_Hall_Effect_Gas
Using an A3214EUA-T or A3213EUA-T Hall Effect Sensor to read the pulse magnet on a gas meter.


##TinyTX_Rain_Gauge
Using a tipping bucket rain gauge with a reed switch to indicate each tip. A pin change interrupt is used to wake the TinyTX


##TinyTX_Water
A leak/flood sensor using the TinyTX, a 100K resistor and two wire probes. See below for a receiver.


##TinyTX_SRF_DS18B20
Version using the Ciseco SRF radio (http://shop.ciseco.co.uk/srf-wireless-rf-radio-surface-mount/) instead of the RFM12B
Uses the SoftEasyTransfer library to send checksummed typedef structs (https://github.com/madsci1016/Arduino-EasyTransfer/)
This is transmit only for now as receive requires a hardware modification to the TinyTX.

##TinyTX_OOK_DS18B20
Version using a cheap OOK/ASK radio and Manchester encoding.
This is transmit only as these cheap radios are only transmitters.


# Code for receiving end

##OSWIN ATMega1284P Emoncms gateway code
For the code I am currently using on my gateway node see the separate repository here:
https://github.com/nathanchantrell/oswin
This support the RFM12B, SRF and OOK nodes.
More info on OSWIN here: http://nathan.chantrell.net/oswin/

##TinyTX_RX_Simple
A simple receive example for RFM12B nodes, outputing received data on the serial monitor. 

##TinyTX_NanodeRF_emoncms
For the Nanode RF (http://www.nanode.eu/) to upload multiple RFM12B nodes to emoncms

##TinyTX_NanodeRF_Cosm
Nanode RF example to upload data from a single RFM12B TinyTX to http://cosm.com

##TinyTX_MAX1284_emoncms
For the MAX1284 internet gateway (http://max1284.homelabs.org.uk/) to upload multiple RFM12B nodes to emoncms

##TinyTX_Water_RX
Receiver example for the leak/flood sensor above, can be run on a Tiny TX and brings D9 high on a water alarm and D10 high on a low battery alarm. Could be used to trigger an external device, sound a buzzer etc. Same code could also be used on a Nanode/WiNode etc.

##TinyTX_SRF_RX_Simple
Simple receive example for the SRF version.

##TinyTX_OOK_RX_Simple
Simple receive example for the OOK version.
