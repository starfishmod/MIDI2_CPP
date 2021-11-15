/**********************************************************
 * MIDI 2.0 Library 
 * Author: Andrew Mee
 * 
 * MIT License
 * Copyright 2021 Andrew Mee
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * 
 * ********************************************************/
#include <cstdio>
#include <cstdint>
#include <cstdlib>

#ifndef M2_DISABLE_PE
#include <cstring>
#endif

#include "utils.h"

class midi2Processor{
    
  private:
  
	uint32_t umpMess[4];
	uint8_t messPos=0;

    MIDICI midici[UMPGROUPS];
    umpSysex7Internal syExMessInt[UMPGROUPS];

    void (*midiNoteOff)(uint8_t group, uint8_t channel, uint8_t noteNumber, uint16_t velocity, uint8_t attributeType, uint16_t attributeData) = nullptr;
    void (*midiNoteOn)(uint8_t group, uint8_t channel, uint8_t noteNumber, uint16_t velocity, uint8_t attributeType, uint16_t attributeData) = nullptr;
    void (*controlChange)(uint8_t group, uint8_t channel, uint8_t index, uint32_t value) = nullptr;
    void (*rpn)(uint8_t group, uint8_t channel, uint8_t bank, uint8_t index, uint32_t value) = nullptr;
    void (*nrpn)(uint8_t group, uint8_t channel, uint8_t bank, uint8_t index, uint32_t value) = nullptr;
    void (*rnrpn)(uint8_t group, uint8_t channel, uint8_t bank, uint8_t index, int32_t value) = nullptr;
    void (*rrpn)(uint8_t group, uint8_t channel, uint8_t bank, uint8_t index, int32_t value) = nullptr;
    void (*polyPressure)(uint8_t group, uint8_t channel, uint8_t noteNumber, uint32_t pressure) = nullptr;
    void (*channelPressure)(uint8_t group, uint8_t channel, uint32_t pressure) = nullptr;
    void (*pitchBend)(uint8_t group, uint8_t channel, uint32_t value) = nullptr;
    void (*programChange)(uint8_t group, uint8_t channel, uint8_t program, bool bankValid, uint8_t bank, uint8_t index) = nullptr;
    void (*sendOutSysex)(uint8_t group, uint8_t *sysex ,uint16_t length, uint8_t state) = nullptr;
    void (*timingCode)(uint8_t group, uint8_t timeCode) = nullptr;
    void (*songSelect)(uint8_t group, uint8_t song) = nullptr;
    void (*songPositionPointer)(uint8_t group, uint16_t position) = nullptr;
    void (*tuneRequest)(uint8_t group) = nullptr;
    void (*timingClock)(uint8_t group) = nullptr;
    void (*seqStart)(uint8_t group) = nullptr;
    void (*seqCont)(uint8_t group) = nullptr;
    void (*seqStop)(uint8_t group) = nullptr;
    void (*activeSense)(uint8_t group) = nullptr;
    void (*systemReset)(uint8_t group) = nullptr;

    bool (*checkMUID)(uint8_t group, uint32_t muid) = nullptr;
    void (*recvDiscoveryRequest)(uint8_t group, MIDICI ciDetails,
            uint8_t* manuId, uint8_t* famId, uint8_t* modelId, uint8_t *verId, uint8_t ciSupport,
            uint16_t maxSysex) = nullptr;
    void (*recvDiscoveryReply)(uint8_t group, MIDICI ciDetails, uint8_t* manuId, uint8_t* famId, uint8_t* modelId, uint8_t *verId, uint8_t ciSupport, uint16_t maxSysex) = nullptr;
    void (*recvNAK)(uint8_t group, MIDICI ciDetails) = nullptr;
    void (*recvInvalidateMUID)(uint8_t group, MIDICI ciDetails, uint32_t terminateMuid) = nullptr;
    void (*recvUnknownMIDICI)(uint8_t group, umpSysex7Internal * syExMess, MIDICI ciDetails, uint8_t s7Byte) = nullptr;


    void endSysex7(uint8_t group);
    void startSysex7(uint8_t group);
    void processSysEx(uint8_t group, uint8_t s7Byte);
    void processMIDICI(uint8_t group, uint8_t s7Byte);
    
    void (*sendOutDebug)(char *message) = nullptr;
    void debug(char *message){
		if(sendOutDebug == nullptr) return;
		sendOutDebug(message);
	}
	void debug(uint8_t b){
		char messStr[2];
		sprintf(messStr,"%x",b);
		debug(messStr);
	}
   
    
  public:
	//This Device's Data
	//uint32_t m2procMUID = 0;

    midi2Processor(uint8_t numRequestsTotal);
	~midi2Processor();

    void processUMP(uint32_t UMP);
    void sendDiscoveryRequest(uint8_t group, uint32_t srcMUID, uint8_t* sysexId, uint8_t* famId,
                              uint8_t* modelId, uint8_t* ver,
                              uint8_t ciSupport, uint16_t sysExMax);
    void sendDiscoveryReply(uint8_t group, uint32_t srcMUID, uint32_t destMuid,  uint8_t* sysexId, uint8_t* famId,
                              uint8_t* modelId, uint8_t* ver,
                              uint8_t ciSupport, uint16_t sysExMax);
	void sendNAK(uint8_t group, uint32_t srcMUID, uint32_t destMuid);
	void sendInvalidateMUID(uint8_t group, uint32_t srcMUID, uint32_t terminateMuid);
    void createCIHeader(uint8_t* sysexHeader, MIDICI midiCiHeader);

	inline void setDebug(void (*fptr)(char *message)){ sendOutDebug = fptr; }
	

	//-----------------------Handlers ---------------------------
	inline void setNoteOff(void (*fptr)(uint8_t group, uint8_t channel, uint8_t noteNumber, uint16_t velocity, uint8_t attributeType, uint16_t attributeData)){ midiNoteOff = fptr; }
	inline void setNoteOn(void (*fptr)(uint8_t group, uint8_t channel, uint8_t noteNumber, uint16_t velocity, uint8_t attributeType, uint16_t attributeData)){ midiNoteOn = fptr; }
	inline void setControlChange(void (*fptr)(uint8_t group, uint8_t channel, uint8_t index, uint32_t value)){ controlChange = fptr; }
	inline void setRPN(void (*fptr)(uint8_t group, uint8_t channel,uint8_t bank,  uint8_t index, uint32_t value)){ rpn = fptr; }
	inline void setNRPN(void (*fptr)(uint8_t group, uint8_t channel,uint8_t bank,  uint8_t index, uint32_t value)){ nrpn = fptr; }
	inline void setRelativeNRPN(void (*fptr)(uint8_t group, uint8_t channel,uint8_t bank,  uint8_t index, int32_t value/*twoscomplement*/)){ rnrpn = fptr; }
	inline void setRelativeRPN(void (*fptr)(uint8_t group, uint8_t channel,uint8_t bank,  uint8_t index, int32_t value/*twoscomplement*/)){ rrpn = fptr; }
	inline void setPolyPressure(void (*fptr)(uint8_t group, uint8_t channel, uint8_t noteNumber, uint32_t pressure)){ polyPressure = fptr; }
	inline void setChannelPressure(void (*fptr)(uint8_t group, uint8_t channel, uint32_t pressure)){ channelPressure = fptr; }
	inline void setPitchBend(void (*fptr)(uint8_t group, uint8_t channel, uint32_t value)){ pitchBend = fptr; }
	inline void setProgramChange(void (*fptr)(uint8_t group, uint8_t channel, uint8_t program, bool bankValid, uint8_t bank, uint8_t index)){ programChange = fptr; }
	//TODO per note etc

	inline void setTimingCode(void (*fptr)(uint8_t group, uint8_t timeCode)){ timingCode = fptr; }
	inline void setSongSelect(void (*fptr)(uint8_t group,uint8_t song)){ songSelect = fptr; }
	inline void setSongPositionPointer(void (*fptr)(uint8_t group,uint16_t position)){ songPositionPointer = fptr; }
	inline void setTuneRequest(void (*fptr)(uint8_t group)){ tuneRequest = fptr; }
	inline void setTimingClock(void (*fptr)(uint8_t group)){ timingClock = fptr; }
	inline void setSeqStart(void (*fptr)(uint8_t group)){ seqStart = fptr; }
	inline void setSeqCont(void (*fptr)(uint8_t group)){ seqCont = fptr; }
	inline void setSeqStop(void (*fptr)(uint8_t group)){ seqStop = fptr; }
	inline void setActiveSense(void (*fptr)(uint8_t group)){ activeSense = fptr; }
	inline void setSystemReset(void (*fptr)(uint8_t group)){ systemReset = fptr; }
	
	inline void setCheckMUID(bool (*fptr)(uint8_t group, uint32_t muid)){ checkMUID = fptr; }
	inline void setRawSysEx(void (*fptr)(uint8_t group, uint8_t *sysex ,uint16_t length, uint8_t state)){ sendOutSysex = fptr; }
	inline void setRecvDiscovery(void (*fptr)(uint8_t group, MIDICI ciDetails, uint8_t* manuId, uint8_t* famId, uint8_t* modelId, uint8_t *verId, uint8_t ciSupport, uint16_t maxSysex)){ recvDiscoveryRequest = fptr;}
	inline void setRecvDiscoveryReply(void (*fptr)(uint8_t group, MIDICI ciDetails, uint8_t* manuId, uint8_t* famId, uint8_t* modelId, uint8_t *verId, uint8_t ciSupport, uint16_t maxSysex)){ recvDiscoveryReply = fptr;}
	inline void setRecvNAK(void (*fptr)(uint8_t group, MIDICI ciDetails)){ recvNAK = fptr;}
	inline void setRecvInvalidateMUID(void (*fptr)(uint8_t group, MIDICI ciDetails, uint32_t terminateMuid)){ recvInvalidateMUID = fptr;}
	inline void setRecvUnknownMIDICI(void (*fptr)(uint8_t group, umpSysex7Internal * syExMess, MIDICI ciDetails, uint8_t s7Byte)){ recvUnknownMIDICI = fptr;}

	
#ifndef M2_DISABLE_IDREQ
  private:
	void (*recvIdRequest)(uint8_t group) = nullptr;
	void (*recvIdResponse)(uint8_t group, uint8_t* sysexId, uint8_t* famId, uint8_t* modelId, uint8_t* ver) = nullptr;
  public:
	void sendIdentityRequest (uint8_t group);
    inline void setHandleId(void (*fptr)(uint8_t group)){ recvIdRequest = fptr;}
    inline void setHandleIdResponse(void (*fptr)(uint8_t group, uint8_t* sysexId, uint8_t* famId, uint8_t* modelId, uint8_t* ver)){ recvIdResponse = fptr;}
#endif



#ifndef M2_DISABLE_JR
  private:
	void (*recvJRClock)(uint8_t group, uint16_t timing) = nullptr;
	void (*recvJRTimeStamp)(uint8_t group, uint16_t timestamp) = nullptr;
	void processDeviceID(uint8_t group, uint8_t s7Byte);
  public:
	inline void setJRClock(void (*fptr)(uint8_t group,uint16_t timing)){ recvJRClock = fptr;}
	inline void setJRTimeStamp(void (*fptr)(uint8_t group,uint16_t timestamp)){ recvJRTimeStamp = fptr;}
#endif



#ifndef M2_DISABLE_PROFILE
  private:
    void (*recvProfileInquiry)(uint8_t group, MIDICI ciDetails) = nullptr;
    void (*recvSetProfileEnabled)(uint8_t group, MIDICI ciDetails, uint8_t* profile) = nullptr;
    void (*recvSetProfileDisabled)(uint8_t group, MIDICI ciDetails, uint8_t* profile) = nullptr;
    void (*recvSetProfileOn)(uint8_t group, MIDICI ciDetails, uint8_t* profile) = nullptr;
    void (*recvSetProfileOff)(uint8_t group, MIDICI ciDetails, uint8_t* profile) = nullptr;
    void processProfileSysex(uint8_t group, uint8_t s7Byte);
  public:
	inline void setRecvProfileInquiry(void (*fptr)(uint8_t group, MIDICI ciDetails)){ recvProfileInquiry = fptr;}
	inline void setRecvProfileEnabled(void (*fptr)(uint8_t group, MIDICI ciDetails, uint8_t* profile)){ recvSetProfileEnabled = fptr;}
	inline void setRecvProfileDisabled(void (*fptr)(uint8_t group, MIDICI ciDetails, uint8_t* profile)){ recvSetProfileDisabled = fptr;}
	inline void setRecvProfileOn(void (*fptr)(uint8_t group, MIDICI ciDetails, uint8_t* profile)){ recvSetProfileOn = fptr;}
	inline void setRecvProfileOff(void (*fptr)(uint8_t group, MIDICI ciDetails, uint8_t* profile)){ recvSetProfileOff = fptr;}
	//TODO Profile Specific Data Message
	
	void sendProfileListRequest(uint8_t group, uint32_t srcMUID, uint32_t destMuid,  uint8_t destination);
	void sendProfileListResponse(uint8_t group, uint32_t srcMUID, uint32_t destMuid, uint8_t destination,
	        uint8_t profilesEnabledLen, uint8_t* profilesEnabled, uint8_t profilesDisabledLen , 
			uint8_t* profilesDisabled );
	void sendProfileOn(uint8_t group, uint32_t srcMUID, uint32_t destMuid, uint8_t destination,
	        uint8_t* profile);
	void sendProfileOff(uint8_t group, uint32_t srcMUID, uint32_t destMuid, uint8_t destination,
	        uint8_t* profile);
	void sendProfileEnabled(uint8_t group, uint32_t srcMUID, uint32_t destMuid, uint8_t destination,
	        uint8_t* profile);
	void sendProfileDisabled(uint8_t group, uint32_t srcMUID, uint32_t destMuid, uint8_t destination,
	        uint8_t* profile);
#endif



#ifndef M2_DISABLE_PE
  private:
	peHeader *peRquestDetails;
    uint8_t numRequests;
    void (*recvPECapabilities)(uint8_t group, MIDICI ciDetails, uint8_t numSimulRequests) = nullptr;
    void (*recvPECapabilitiesReplies)(uint8_t group, MIDICI ciDetails, uint8_t numSimulRequests) = nullptr;
    void (*recvPEGetInquiry)(uint8_t group, MIDICI ciDetails, peHeader requestDetails) = nullptr;
    void (*recvPESetReply)(uint8_t group, MIDICI ciDetails, peHeader requestDetails) = nullptr;
    void (*recvPESubReply)(uint8_t group, MIDICI ciDetails, peHeader requestDetails) = nullptr;
    void (*recvPESetInquiry)(uint8_t group, MIDICI ciDetails, peHeader requestDetails, uint16_t bodyLen, uint8_t*  body, bool lastByteOfSet) = nullptr;
    void (*recvPESubInquiry)(uint8_t group, MIDICI ciDetails, peHeader requestDetails, uint16_t bodyLen, uint8_t*  body, bool lastByteOfSet) = nullptr;
    uint8_t getPERequestId(uint8_t group, uint32_t muid ,uint8_t s7Byte);
    void cleanupRequestId(uint8_t group, uint32_t muid, uint8_t requestId);
    void cleanupRequest(uint8_t reqpos);
    void processPEHeader(uint8_t group, uint8_t peRequestIdx, uint8_t s7Byte);
    void processPESysex(uint8_t group, uint8_t s7Byte);
  public:
	void sendPECapabilityRequest(uint8_t group,uint32_t srcMUID, uint32_t destMuid,  uint8_t numSimulRequests);
	void sendPECapabilityReply(uint8_t group, uint32_t srcMUID, uint32_t destMuid,  uint8_t numSimulRequests);

	void sendPEGet(uint8_t group, uint32_t srcMUID, uint32_t destMuid, uint8_t requestId,
	        uint16_t headerLen, uint8_t* header);

    void sendPESet(uint8_t group, uint32_t srcMUID, uint32_t destMuid, uint8_t requestId,
                   uint16_t headerLen, uint8_t* header, uint16_t numberOfChunks, uint16_t numberOfThisChunk,
                   uint16_t bodyLength , uint8_t* body);

    void sendPESub(uint8_t group, uint32_t srcMUID, uint32_t destMuid, uint8_t requestId,
                   uint16_t headerLen, uint8_t* header, uint16_t numberOfChunks, uint16_t numberOfThisChunk,
                   uint16_t bodyLength , uint8_t* body);
	
	void sendPEGetReply(uint8_t group, uint32_t srcMUID, uint32_t destMuid, uint8_t requestId,
	        uint16_t headerLen, uint8_t* header, uint16_t numberOfChunks, uint16_t numberOfThisChunk,
			uint16_t bodyLength , uint8_t* body );

    void sendPEGetReplyStreamStart(uint8_t group, uint32_t srcMUID, uint32_t destMuid,  uint8_t requestId, uint16_t headerLen, uint8_t* header, uint16_t numberOfChunks, uint16_t numberOfThisChunk, uint16_t bodyLength);
    void sendPEGetReplyStreamContinue(uint8_t group, uint16_t partialLength, uint8_t* part, bool last );
			
	void sendPESubReply(uint8_t group, uint32_t srcMUID, uint32_t destMuid, uint8_t requestId,
	        uint16_t headerLen, uint8_t* header);		
	void sendPESetReply(uint8_t group, uint32_t srcMUID, uint32_t destMuid, uint8_t requestId,
	        uint16_t headerLen, uint8_t* header);		
			
    inline void setPECapabilities(void (*fptr)(uint8_t group, MIDICI ciDetails, uint8_t numSimulRequests)){ recvPECapabilities = fptr;}
    inline void setPECapabilitiesReply(void (*fptr)(uint8_t group, MIDICI ciDetails, uint8_t numSimulRequests)){ recvPECapabilitiesReplies = fptr;}
    inline void setRecvPEGetInquiry(void (*fptr)(uint8_t group, MIDICI ciDetails,  peHeader requestDetails)){ recvPEGetInquiry = fptr;}
    inline void setRecvPESetReply(void (*fptr)(uint8_t group, MIDICI ciDetails,  peHeader requestDetails)){ recvPESetReply = fptr;}
    inline void setRecvPESubReply(void (*fptr)(uint8_t group, MIDICI ciDetails,  peHeader requestDetails)){ recvPESubReply = fptr;}
    inline void setRecvPESetInquiry(void (*fptr)(uint8_t group, MIDICI ciDetails,  peHeader requestDetails, uint16_t bodyLen, uint8_t*  body, bool lastByteOfSet)){ recvPESetInquiry = fptr;}
    inline void setRecvPESubInquiry(void (*fptr)(uint8_t group, MIDICI ciDetails,  peHeader requestDetails, uint16_t bodyLen, uint8_t*  body, bool lastByteOfSet)){ recvPESubInquiry = fptr;}
    //TODO PE Notify
#endif
	
};
