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

#ifndef MIDI2_H
#define MIDI2_H

#ifdef M2_ENABLE_PE
#include <string.h>
#endif

#include "utils.h"
#include "messageCreate.h"
#include "bytestreamUMP.h"



class midi2Processor{
    
  private:
  
	uint32_t umpMess[4];
	uint8_t messPos=0;
	
	
	uint8_t groupStart;
	uint8_t groups;
	
	uint32_t syExMess[2]={0,0};
    uint16_t syExMessPos=0;
    
    
    //SysEx7Based Data
    bool *sysex7State;   
    uint16_t *sysexPos;
    uint8_t *sysexMode; 
    uint8_t *sysUniNRTMode;
    uint8_t *sysUniPort;
    uint8_t **sys7CharBuffer;
    uint16_t **sys7IntBuffer;
    
    uint8_t *ciType;
    uint8_t *ciVer;
    uint32_t *remoteMuid;
    uint32_t *destMuid;
    
    #ifdef M2_ENABLE_PE
	peHeader *peRquestDetails;
    uint8_t numRequests;
    void * _pvoid;
	#endif
    
  public:
  
	//This Device's Data
	uint32_t groupBlockMUID = 0; 
	uint8_t devId[3];
    uint8_t famId[2];
    uint8_t modelId[2];
    uint8_t ver[4]; 
    
    uint16_t sysExMax = 512;
    
        void (*midiNoteOff)(uint8_t group, uint8_t channel, uint8_t noteNumber, uint16_t velocity, uint8_t attributeType, uint16_t attributeData) = 0;
    void (*midiNoteOn)(uint8_t group, uint8_t channel, uint8_t noteNumber, uint16_t velocity, uint8_t attributeType, uint16_t attributeData) = 0;
    void (*controlChange)(uint8_t group, uint8_t channel, uint8_t index, uint32_t value) = 0;
    void (*rpn)(uint8_t group, uint8_t channel, uint8_t bank, uint8_t index, uint32_t value) = 0;
    void (*polyPressure)(uint8_t group, uint8_t channel, uint8_t noteNumber, uint32_t pressure) = 0;
    void (*channelPressure)(uint8_t group, uint8_t channel, uint32_t pressure) = 0;
    void (*pitchBend)(uint8_t group, uint8_t channel, uint32_t value) = 0;
    void (*programChange)(uint8_t group, uint8_t channel, uint8_t program, bool bankValid, uint8_t bank, uint8_t index) = 0;
    void (*sendOutSysex)(uint8_t group, uint8_t *sysex ,uint16_t length, uint8_t state) = 0;
    void (*timingCode)(uint8_t group, uint8_t timeCode) = 0;
    void (*songSelect)(uint8_t group, uint8_t song) = 0;
    void (*songPositionPointer)(uint8_t group, uint16_t position) = 0;
    void (*tuneRequest)(uint8_t group) = 0;
    void (*timingClock)(uint8_t group) = 0;
    void (*seqStart)(uint8_t group) = 0;
    void (*seqCont)(uint8_t group) = 0;
    void (*seqStop)(uint8_t group) = 0;
    void (*activeSense)(uint8_t group) = 0;
    void (*systemReset)(uint8_t group) = 0;
    
    
    void (*recvDiscoveryRequest)(uint8_t group, uint32_t remoteMuid, uint8_t ciVer, uint8_t* manuId, uint8_t* famId, uint8_t* modelId, uint8_t *verId, uint8_t ciSupport, int maxSysex) = 0;
    void (*recvDiscoveryReply)(uint8_t group, uint32_t remoteMuid, uint8_t ciVer, uint8_t* manuId, uint8_t* famId, uint8_t* modelId, uint8_t *verId, uint8_t ciSupport, int maxSysex) = 0;
    void (*recvNAK)(uint8_t group, uint32_t remoteMuid) = 0;
    void (*recvInvalidateMUID)(uint8_t group, uint32_t remoteMuid, uint32_t terminateMuid) = 0;
    
    #ifdef M2_ENABLE_JR
    void (*jrClock)(uint8_t group, uint16_t timing) = 0;
    #endif
    
    #ifdef M2_ENABLE_PROFILE
    void (*recvProfileInquiry)(uint8_t group, uint32_t remoteMuid, uint8_t destination) = 0;
    void (*recvSetProfileEnabled)(uint8_t group, uint32_t remoteMuid, uint8_t destination, uint8_t* profile) = 0;
    void (*recvSetProfileDisabled)(uint8_t group, uint32_t remoteMuid, uint8_t destination, uint8_t* profile) = 0;
    void (*recvSetProfileOn)(uint8_t group, uint32_t remoteMuid, uint8_t destination, uint8_t* profile) = 0;
    void (*recvSetProfileOff)(uint8_t group, uint32_t remoteMuid, uint8_t destination, uint8_t* profile) = 0;
    #endif
    
    #ifdef M2_ENABLE_PE
    void (*recvPECapabilities)(uint8_t group, uint32_t remoteMuid, uint8_t numRequests) = 0;
    void (*recvPEGetInquiry)(uint8_t group, uint32_t remoteMuid, peHeader requestDetails) = 0;
    void (*recvPESetInquiry)(uint8_t group, uint32_t remoteMuid, peHeader requestDetails, uint8_t bodyLen, uint8_t*  body) = 0;
    #endif
    
    void addCIHeader(uint8_t ciType, uint8_t* sysexHeader);
    void endSysex7(uint8_t groupOffset);
    void startSysex7(uint8_t groupOffset);
    void processSysEx(uint8_t groupOffset, uint8_t newByte);
    void processUniS7NRT(uint8_t groupOffset, uint8_t newByte);

    midi2Processor(uint8_t grStart, uint8_t totalGroups
		#ifdef M2_ENABLE_PE
		, uint8_t numRequestsTotal
		#endif
	);
	
	~midi2Processor();
	
    void processUMP(uint32_t UMP);
    
    
    #ifdef M2_ENABLE_IDREQ
    void sendIdentityRequest (uint8_t group);
    #endif
    
    void sendDiscoveryRequest(uint8_t group, uint8_t ciVer);
	
	void sendNAK(uint8_t group, uint32_t remoteMuid, uint8_t ciVer);
	
	#ifdef M2_ENABLE_PROFILE
	void sendProfileListRequest(uint8_t group, uint32_t remoteMuid, uint8_t ciVer, uint8_t destination);
	
	void sendProfileListResponse(uint8_t group, uint32_t remoteMuid, uint8_t ciVer, uint8_t destination,
	        uint8_t profilesEnabledLen, uint8_t* profilesEnabled, uint8_t profilesDisabledLen , 
			uint8_t* profilesDisabled );
	
	void sendProfileOn(uint8_t group, uint32_t remoteMuid, uint8_t ciVer, uint8_t destination, 
	        uint8_t* profile);
	
	void sendProfileOff(uint8_t group, uint32_t remoteMuid, uint8_t ciVer, uint8_t destination,
	        uint8_t* profile);
	
	void sendProfileEnabled(uint8_t group, uint32_t remoteMuid, uint8_t ciVer, uint8_t destination,
	        uint8_t* profile);
	
	void sendProfileDisabled(uint8_t group, uint32_t remoteMuid, uint8_t ciVer, uint8_t destination, 
	        uint8_t* profile);
    #endif 
    
    
    #ifdef M2_ENABLE_PE
    void sendPECapabilityRequest(uint8_t group, uint32_t remoteMuid, uint8_t ciVer, uint8_t numRequests);
	
	void sendPEGet(uint8_t group, uint32_t remoteMuid, uint8_t ciVer, uint8_t requestId, 
	        uint16_t headerLen, uint8_t* header);
	
	void sendPEGetReply(uint8_t group, uint32_t remoteMuid, uint8_t ciVer, uint8_t requestId, 
	        uint16_t headerLen, uint8_t* header, int numberOfChunks, int numberOfThisChunk, 
			uint16_t bodyLength , uint8_t* body );
	
	//TODO Move to Private if not needed??
	void cleanupRequestId(uint8_t requestId);
	
    #endif   
    
	//-----------------------Handlers ---------------------------
	inline void setNoteOff(void (*fptr)(uint8_t group, uint8_t channel, uint8_t noteNumber, uint16_t velocity, uint8_t attributeType, uint16_t attributeData));
	inline void setNoteOn(void (*fptr)(uint8_t group, uint8_t channel, uint8_t noteNumber, uint16_t velocity, uint8_t attributeType, uint16_t attributeData));
	inline void setControlChange(void (*fptr)(uint8_t group, uint8_t channel, uint8_t index, uint32_t value));
	inline void setRPN(void (*fptr)(uint8_t group, uint8_t channel,uint8_t bank,  uint8_t index, uint32_t value));
	inline void setPolyPressure(void (*fptr)(uint8_t group, uint8_t channel, uint8_t noteNumber, uint32_t pressure));
	inline void setChannelPressure(void (*fptr)(uint8_t group, uint8_t channel, uint32_t pressure));
	inline void setPitchBend(void (*fptr)(uint8_t group, uint8_t channel, uint32_t value));
	inline void setProgramChange(void (*fptr)(uint8_t group, uint8_t channel, uint8_t program, bool bankValid, uint8_t bank, uint8_t index));
	//TODO NRPNs, relative, per note etc
	
	inline void setTimingCode(void (*fptr)(uint8_t group,uint8_t timeCode));
	inline void setSongSelect(void (*fptr)(uint8_t group,uint8_t song));
	inline void setSongPositionPointer(void (*fptr)(uint8_t group,uint16_t positio));
	inline void setTuneRequest(void (*fptr)(uint8_t group));
	inline void setTimingClock(void (*fptr)(uint8_t group));
	inline void setSeqStart(void (*fptr)(uint8_t group));
	inline void setSeqCont(void (*fptr)(uint8_t group));
	inline void setSeqStop(void (*fptr)(uint8_t group));
	inline void setActiveSense(void (*fptr)(uint8_t group));
	inline void setSystemReset(void (*fptr)(uint8_t group));
	
	
	inline void setRawSysEx(void (*fptr)(uint8_t group, uint8_t *sysex ,uint16_t length, uint8_t state));
	inline void setRecvDiscovery(void (*fptr)(uint8_t group, uint32_t remoteMuid, uint8_t ciVer, uint8_t* manuId, uint8_t* famId, uint8_t* modelId, uint8_t *verId, uint8_t ciSupport, uint16_t maxSysex));
	inline void setRecvNAK(void (*fptr)(uint8_t group, uint32_t remoteMuid));
	inline void setRecvInvalidateMUID(void (*fptr)(uint8_t group, uint32_t remoteMuid, uint32_t terminateMuid));
	
	#ifdef M2_ENABLE_JR
	inline void setJrClock(void (*fptr)(uint8_t group,uint16_t timing));
	#endif
	
	#ifdef M2_ENABLE_PROFILE
	inline void setRecvProfileInquiry(void (*fptr)(uint8_t group, uint32_t remoteMuid, uint8_t destination)){ recvProfileInquiry = fptr;}
	inline void setRecvProfileEnabled(void (*fptr)(uint8_t group, uint32_t remoteMuid, uint8_t destination, uint8_t* profile)){ recvSetProfileEnabled = fptr;}
	inline void setRecvProfileDisabled(void (*fptr)(uint8_t group, uint32_t remoteMuid, uint8_t destination, uint8_t* profile)){ recvSetProfileDisabled = fptr;}
	inline void setRecvProfileOn(void (*fptr)(uint8_t group, uint32_t remoteMuid, uint8_t destination, uint8_t* profile)){ recvSetProfileOn = fptr;}
	inline void setRecvProfileOff(void (*fptr)(uint8_t group, uint32_t remoteMuid, uint8_t destination, uint8_t* profile)){ recvSetProfileOff = fptr;}
	//TODO Profile Specific Data Message
    #endif
    
    
    #ifdef M2_ENABLE_PE
    inline void setPECapabilities(void (*fptr)(uint8_t group, uint32_t remoteMuid, uint8_t numRequests)){ recvPECapabilities = fptr;}
    inline void setRecvPEGetInquiry(void (*fptr)(uint8_t group, uint32_t remoteMuid,  peHeader requestDetails)){ recvPEGetInquiry = fptr;}
    inline void setRecvPESetInquiry(void (*fptr)(uint8_t group, uint32_t remoteMuid,  peHeader requestDetails, uint8_t bodyLen, uint8_t*  body)){ recvPESetInquiry = fptr;}
    //TODO PE Notify
    #endif
	
	
	
	
	#ifdef M2_ENABLE_IDREQ
    inline void setHandleIdResponse(void (*fptr)(uint8_t* devId, uint8_t* famId, uint8_t* modelId, uint8_t* ver)){ sendOutIdResponse = fptr;}
    #endif
	
};

#endif

