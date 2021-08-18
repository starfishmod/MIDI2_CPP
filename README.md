# MIDI2.0 C++
A MIDI 2.0 Library for Embedded/Arduino Devices

## IMPORTANT! Please read first
THIS LIBRARY IS UNDER DEVELOPMENT - Code here today, is likely gone tomorrow!

I AM NOT A PROFESIONAL C++ DEVELOPER. If you see code here that :
* could be styled/structured better
* could be written better
* could use less resources
* has memory leaks, bugs, 
* is fundamentally flawed

Then please submit PR's and/or issues - but PR's preferred. 

Given this - please use this library at your own risk! Long term I hope that this library will be improved enough so it can be useful to others.

## What does this do?
Please read the MIDI 2.0 specification on https://midi.org/specifications to understand the following.

This library can:
* Convert MIDI 1.0 Bystream to UMP
* Process UMP Streams
* Process MIDI-CI Messages
* Build UMP 32 bit Words

### Example Convert MIDI 1.0 Bystream to UMP

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
    
    BS2UMP.midi1BytestreamParse(inByte);
    while(BS2UMP.availableUMP()){
      uint32_t ump = BS2UMP.readUMP();
      //ump contains a ump 32 bit value. UMP messages that have 64bit will produce 2 UMP words
    }
  }
}
```

### Example Process UMP Streams
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
  MIDI2.setNoteOff(noteOff);
  MIDI2.setNoteOn(noteOn);
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

---
## Defines
### #define M2_ENABLE_PROFILE
This enable processing of MIDI-CI Profiles

### #define M2_ENABLE_PROFILE
This enable processing of MIDI-CI Property Exchange

### #define M2_ENABLE_JR
Enable Processing of JR Timestamps

### #define M2_ENABLE_IDREQ
This enable processing of System Exclusive handling Device ID Request and Reply

---
## midiBsToUMP
### Common methods
#### void midi1BytestreamParse(uint8_t midi1Byte)
Process incoming MIDI 1.0 Byte Stream

#### bool availableUMP()
Check if there are available UMP packets after processing the Byte Stream

#### uint32_t readUMP()
Return a 32Bit word for a UMP Packet. A Bystream conversion may create several UMP packets.

---
## midi2GroupBlock
Processing of MIDI 1.0 and 2.0 Channel voice messages use the same processing function.
#### midi2GroupBlock(uint8_t groupStart, uint8_t totalGroups)
#### midi2GroupBlock(uint8_t groupStart, uint8_t totalGroups, uint8_t numPERequests) - When M2_ENABLE_PROFILE is defined
Initialize the with the Group number to start with and the amount of UMP groups to work with. numPERequests represents the amount simultaneous PE streams that can be handled

### Common Methods
#### void processUMP(uint32_t UMP)
Process incoming UMP messages broken up into 32bit words

### Common Handlers
#### inline void setNoteOff(void (\*fptr)(uint8_t group, uint8_t channel, uint8_t noteNumber, uint16_t velocity, uint8_t attributeType, uint16_t attributeData))
Set the callable funtion when a Note Off is processed by processUMP

#### inline void setMidiNoteOn(void (\*fptr)(uint8_t group, uint8_t channel, uint8_t noteNumber, uint16_t velocity, uint8_t attributeType, uint16_t attributeData))
#### inline void setControlChange(void (\*fptr)(uint8_t group, uint8_t channel, uint8_t index, uint32_t value))
#### inline void setRPN(void (\*fptr)(uint8_t group, uint8_t channel,uint8_t bank,  uint8_t index, uint32_t value))
#### inline void setPolyPressure(void (\*fptr)(uint8_t group, uint8_t channel, uint8_t noteNumber, uint32_t pressure))
#### inline void setChannelPressure(void (\*fptr)(uint8_t group, uint8_t channel, uint32_t pressure))
#### inline void setPitchBend(void (\*fptr)(uint8_t group, uint8_t channel, uint32_t value))
#### inline void setProgramChange(void (\*fptr)(uint8_t group, uint8_t channel, uint8_t program, bool bankValid, uint8_t bank, uint8_t index))
#### inline void setTimingCode(void (\*fptr)(uint8_t group,uint8_t timeCode)){
#### inline void setSongSelect(void (\*fptr)(uint8_t group,uint8_t song))
#### inline void setSongPositionPointer(void (*\fptr)(uint8_t group,uint16_t positio))
#### inline void setTuneRequest(void (\*fptr)(uint8_t group))
#### inline void setTimingClock(void (\*fptr)(uint8_t group))
#### inline void setSeqStart(void (\*fptr)(uint8_t group))
#### inline void setSeqCont(void (\*fptr)(uint8_t group))
#### inline void setSeqStop(void (\*fptr)(uint8_t group))
#### inline void setActiveSense(void (\*fptr)(uint8_t group))
#### inline void setSystemReset(void (\*fptr)(uint8_t group))

### Common SysEx Handlers
#### inline void setRawSysEx(void (\*fptr)(uint8_t group, uint8_t\* sysex ,uint16_t length, uint8_t state))
#### inline void setRecvDiscoveryRequest(void (\*fptr)(uint8_t group, uint32_t remoteMuid, uint8_t ciVer, uint8_t\* manuId, uint8_t\* famId, uint8_t\* modelId, uint8_t\* verId, uint8_t ciSupport, uint16_t maxSysex))
#### inline void setRecvDiscoveryReply(void (\*fptr)(uint8_t group, uint32_t remoteMuid,uint8_t ciVer, uint8_t\* manuId, uint8_t\* famId, uint8_t\* modelId, uint8_t\* verId, uint8_t ciSupport, uint16_t maxSysex))
#### inline void setRecvNAK(void (\*fptr)(uint8_t group, uint32_t remoteMuid))
#### inline void setRecvInvalidateMUID(void (\*fptr)(uint8_t group, uint32_t remoteMuid, uint32_t terminateMuid))

