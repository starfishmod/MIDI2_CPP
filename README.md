# MIDI2 C++
A MIDI 2.0 Library for Embedded/Arduino Devices

## IMPORTANT! Please read first
THIS LIBRARY IS UNDER DEVELOPMENT - Code here today, is likely gone tomorrow!

I AM NOT A PROFESIONAL C++ DEVLOPER. If you see code here that could be:
* styled/structured better
* written better
* use less resources
* has memory leaks, bugs, 
* fundamentally flawed

Then please submit PR's and/or issues - but PR's preferred. 

Given this - please use this library at you own risk! Long term I hope that this library will be improved so it can be useful to others.

## What does this do?
Please read the MIDI 2.0 specification on https://midi.org/specifications to understand the following.

This library can:
* Convert MIDI 1.0 Bystream to UMP
* Process UMP Streams
* Process MIDI-CI Messages
* Build UMP 32 bit Words

## Example Convert MIDI 1.0 Bystream to UMP

Here is a quick Arduino example

```C++
#include "midi2.h"

midiBsToUMP BS2UMP;

void setup()
{
  Serial.begin(31250);
  
  //Produce MIDI 2.0 Channel Voice Message (Message Type 0x4)
  //If not set it will return MIDI 1.0 Channel Voice Messages (Message Type 0x2)
  BS2UMP.outputMIDI2 = true; 
  
  //Set the UMP group of the output UMP message. By default this is set to Group 1
  //defaultGroup value is 0 based
  BS2UMP.defaultGroup = 0; //Group 1
}

void loop()
{
  uint8_t inByte = 0;
  // if there is a serial MIDI byte:
  if (Serial.available() > 0) {
    // get incoming byte:
    inByte = Serial.read();
    if(inByte == 0xFE) return; //Skip ActiveSense 
    
    BS2UMP.midi1BytesteamParse(inByte);
    while(BS2UMP.availableUMP()){
      uint32_t ump = BS2UMP.readUMP();
      //ump contains a ump 32 bit value. UMP messages that have 64bit will produce 2 UMP words
    }
  }
}
```

## Example Process UMP Streams
UMP Streams accepts a series of 32 bit values. UMP messages that have 64bit will provide 2 UMP words.

```C++

#include "midi2.h"
midi2GroupBlock MIDI2 (0,2); 

void noteOff(uint8_t group, uint8_t channel, uint8_t noteNumber, unsigned int velocity, int attributeType, unsigned int attributeData){
//Process incoming MIDI note Off event.
}

void noteOn(uint8_t group, uint8_t channel, uint8_t noteNumber, unsigned int velocity, int attributeType, unsigned int attributeData){ 
}


void cc(uint8_t group, uint8_t channel, uint8_t index, uint32_t value){  
}

void rpn(uint8_t group, uint8_t channel, uint8_t bank,  uint8_t index, uint32_t value){  
}

void setup()
{
  MIDI2.setMidiNoteOff(noteOff);
  MIDI2.setMidiNoteOn(noteOn);

  MIDI2.setControlChange(cc);
  MIDI2.setRPN(rpn);

}

void loop()
{
...
  while(BS2UMP.availableUMP()){
      uint32_t ump = BS2UMP.readUMP();
      MIDI2.processUMP(ump);
  }
...  
}

```    
