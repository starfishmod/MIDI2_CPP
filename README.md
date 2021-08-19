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
* Process and send UMP Streams
* Process and Send MIDI-CI Messages
* Build UMP 32 bit Words to send

This library is designed to use a small footprint. This means it is upto the appliction to:
 * Store Remote MIDI-CI Device details
 * Upon recieving MIDI-CI Message to interpret the Messages data structure (e.g. Profile Id bytes, Note On Articulation etc.)
 * Handle logic and NAK sending and recieving.
 * Large data is handled in small chunks to keep memory small.

This means the overheads for a simple MIDI 2.0 device is down to compile size around 10k with a memory footprint of around 1k.

### TODO
* Protocol Negotiation
* PE Get Reply handling
* PE Set Methods
* PE Handling of Mcoded7
* Profile Specific Data Message
* Send Invalid MUID
* Sending JR Timestamp/Clock
* Handling of Per Note Controllers
* Universal SysEx handling (Other than Device ID and MIDI-CI)

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
midi2Processor MIDI2 (0,2); 

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

### Example Process MIDI-CI Messages
UMP Streams accepts a series of 32 bit values. UMP messages that have 64bit will provide 2 UMP words.

```C++
#define M2_ENABLE_PROFILE

#include "midi2.h"
midi2Processor MIDI2 (0,2); 

void rawSysex(uint8_t group, uint8_t *sysex ,int length, uint8_t state){
  //Generated SysEX Message from MIDI-CI or other Messages are sent to this function.
  //Write SysEx Message to Serial output
  
  //Start SysEx out 
  if (state < 2)Serial.write((char)SYSEX_START);
 
  for(int i=0; i < length; i++){
    Serial.write((char)(sysex[i] & 0x7F));
  }

  //End SysEx
  if (state == 0 || state==3) Serial.write((char)SYSEX_STOP);
}

void discovery(uint8_t group, uint32_t remoteMuid, uint8_t ciVer, uint8_t* manuId, uint8_t* famId, uint8_t* modelId, uint8_t *verId, uint8_t ciSupport, int maxSysex){
}

void nak(uint8_t group, uint32_t remoteMuid){
}

void invalidMUID(uint8_t group, uint32_t remoteMuid, uint32_t terminateMuid){
  MIDI2.groupBlockMUID = random(0xFFFFEFF);
  MIDI2.sendDiscoveryRequest(0,1);
}

void profileInquiry(uint8_t group, uint32_t remoteMUID, uint8_t destination){  
  uint8_t profileNone[0] = {};
  
  //return a Profile on Channel 1
  if(destination == 0 || destination == 0x7F){
    uint8_t profileDrawBar[5] = {0x7E, 0x40, 0x01, 0x01};
    MIDI2.sendProfileListResponse(group, remoteMUID, 1, 0, 1, profileDrawBar, 0, profileNone);
  }

  if(destination == 0x7F){
    MIDI2.sendProfileListResponse(group, remoteMUID, 1, 0x7F, 0, profileNone, 0, profileNone);
  }
}

void setup()
{
  randomSeed(analogRead(8));
  Serial.begin(31250);
  MIDI2.groupBlockMUID = random(0xFFFFEFF);
  MIDI2.devId[0]=0x7E; // Research SysEx ID
  
  MIDI2.modelId[0]=0x7F;
  MIDI2.modelId[1]=0x20;
  
  MIDI2.setRecvDiscovery(discovery);
  MIDI2.setRecvNAK(nak);
  MIDI2.setRecvInvalidateMUID(invalidMUID);

  MIDI2.setRecvProfileInquiry(profileInquiry);
  
  MIDI2.sendDiscoveryRequest(0,1);
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
## midiBsToUMP
Class used for transalting between a raw MIDI 1.0 Byte stream and UMP

### Common methods
#### void midi1BytestreamParse(uint8_t midi1Byte)
Process incoming MIDI 1.0 Byte Stream

#### bool availableUMP()
Check if there are available UMP packets after processing the Byte Stream

#### uint32_t readUMP()
Return a 32Bit word for a UMP Packet. A Bystream conversion may create several UMP packets.

---
## midi2Processor
Processing of MIDI 1.0 and 2.0 Channel voice messages use the same processing function.
#### midi2Processor(uint8_t groupStart, uint8_t totalGroups)
#### midi2Processor(uint8_t groupStart, uint8_t totalGroups, uint8_t numPERequests) - When M2_ENABLE_PROFILE is defined
Initialize the with the Group number to start with and the amount of UMP groups to work with. numPERequests represents the amount simultaneous PE streams that can be handled

### public variables
#### uint32_t groupBlockMUID = 0; 
#### uint8_t devId[3];
#### uint8_t famId[2];
#### uint8_t modelId[2];
#### uint8_t ver[4]; 
#### uint16_t sysExMax = 512;

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
Any SysEx generated from either MIDI-CI resposes or from direct Method calls are sent here
Message are often chunked. as such the ```state``` argument contains information if this is a whole SysEx message or partial.
Leading 0xF0 and 0xF7 are never sent.

 * **0** - Complete SysEx message
 * **1** - Start of a SysEx message
 * **2** - Continue a SysEx message
 * **3** - Last part of a SysEx message

#### inline void setRecvDiscoveryRequest(void (\*fptr)(uint8_t group, uint32_t remoteMuid, uint8_t ciVer, uint8_t\* manuId, uint8_t\* famId, uint8_t\* modelId, uint8_t\* verId, uint8_t ciSupport, uint16_t maxSysex))
Process Incoming Discovery Request Device details or Reply to Discovery Device details. When the class recieves a Discovery Message it will automatically reply with a Reply to Discovery Message. This is sent to the function set by ```setRawSysEx```.

After triggering off a ```sendDiscoveryRequest``` replies will be sent here.

#### inline void setRecvNAK(void (\*fptr)(uint8_t group, uint32_t remoteMuid))
#### inline void setRecvInvalidateMUID(void (\*fptr)(uint8_t group, uint32_t remoteMuid, uint32_t terminateMuid))

## Common SysEx Methods
### void sendDiscoveryRequest(uint8_t group, uint8_t ciVer)
### void sendNAK(uint8_t group, uint32_t remoteMuid, uint8_t ciVer)

---
## midi2Processor M2_ENABLE_IDREQ Methods
These are available if ```#define M2_ENABLE_IDREQ``` is set. 
If a Device ID Request is recieved the class will automatically send a reply based on the DeviceId, Model, Famil and Version information provided.

### void sendIdentityRequest (uint8_t group)
Send out a Universal SysEx Device ID Request

### inline void setHandleIdResponse(void (\*fptr)(uint8_t\* devId, uint8_t\* famId, uint8_t\* modelId, uint8_t\* ver))
Set handler for recieving Universal SysEx Device ID Reply.

---
## midi2Processor M2_ENABLE_PROFILE Methods
This enable processing of MIDI-CI Profiles. These methods are available if ```#define M2_ENABLE_PROFILE``` is set.

### void sendProfileListRequest(uint8_t group, uint32_t remoteMuid, uint8_t ciVer, uint8_t destination)
Send a Request to get list of Profiles. use a destination of 0x7F to get all Profiles across all channels.
Enabled and Disabled Profiles are sent to the ```setRecvProfileEnabled``` and ```setRecvProfileDisabled``` respectively.

### void sendProfileListResponse(uint8_t group, uint32_t remoteMuid, uint8_t ciVer, uint8_t destination, uint8_t profilesEnabledLen, uint8_t\* profilesEnabled, uint8_t profilesDisabledLen , uint8_t\* profilesDisabled)
Send a Profile List Response Message.

```profilesEnabledLen``` and ```profilesDisabledLen``` represent how many Profiles. ```profilesEnabled``` and ```profilesDisabled``` arguments should be 5 times the length of ```profilesEnabledLen``` and ```profilesDisabledLen``` respectively.

### void sendProfileOn(uint8_t group, uint32_t remoteMuid, uint8_t ciVer, uint8_t destination, uint8_t\* profile)
### void sendProfileOff(uint8_t group, uint32_t remoteMuid, uint8_t ciVer, uint8_t destination, uint8_t\* profile)
### void sendProfileEanbled(uint8_t group, uint32_t remoteMuid, uint8_t ciVer, uint8_t destination, uint8_t\* profile)
### void sendProfileDisabled(uint8_t group, uint32_t remoteMuid, uint8_t ciVer, uint8_t destination, uint8_t\* profile)

### inline void setRecvProfileInquiry(void (\*fptr)(uint8_t group, uint32_t remoteMuid, uint8_t destination))
```sendProfileListResponse``` should be used in Response to this Message.

### inline void setRecvProfileEnabled(void (\*fptr)(uint8_t group, uint32_t remoteMuid, uint8_t destination, uint8_t\* profile))
### inline void setRecvProfileDisabled(void (\*fptr)(uint8_t group, uint32_t remoteMuid, uint8_t destination, uint8_t\* profile))
### inline void setRecvProfileOn(void (\*fptr)(uint8_t group, uint32_t remoteMuid, uint8_t destination, uint8_t\* profile))
### inline void setRecvProfileOff(void (\*fptr)(uint8_t group, uint32_t remoteMuid, uint8_t destination, uint8_t\* profile))


---
## midi2Processor M2_ENABLE_PE Methods
This enable processing of MIDI-CI Property Exchange. These methods are available if ```#define M2_ENABLE_PE``` is set.

Property Exchange requires a bit more memory than other parts of MIDI-CI. Enabling this will increase memory requirements.
### void sendPECapabilityRequest(uint8_t group, uint32_t remoteMuid, uint8_t ciVer, uint8_t numRequests)
### void sendPEGet(uint8_t group, uint32_t remoteMuid, uint8_t ciVer, uint8_t requestId, uint16_t headerLen, uint8_t* header)
### void sendPEGetReply(uint8_t group, uint32_t remoteMuid, uint8_t ciVer, uint8_t requestId, uint16_t headerLen, uint8_t\* header, int numberOfChunks, int numberOfThisChunk, uint16_t bodyLength , uint8_t\* body)

### inline void setPECapabilities(void (\*fptr)(uint8_t group, uint32_t remoteMuid, uint8_t numRequests))
### inline void setRecvPEGetInquiry(void (\*fptr)(uint8_t group, uint32_t remoteMuid,  peHeader requestDetails))
### inline void setRecvPESetInquiry(void (\*fptr)(uint8_t group, uint32_t remoteMuid,  peHeader requestDetails, uint8_t bodyLen, uint8_t\*  body))

---
## midi2Processor M2_ENABLE_JR
Enable Processing of JR Timestamps. These methods are available if ```#M2_ENABLE_JR M2_ENABLE_PE``` is set.

### inline void setJrClock(void (\*fptr)(uint8_t group,uint16_t timing))
