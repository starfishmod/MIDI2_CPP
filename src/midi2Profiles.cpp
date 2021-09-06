#include "../include/midi2Profiles.h"
#include "../include/midi2Processor.h"
#include "../include/utils.h"


#ifndef M2_DISABLE_PROFILE


#ifdef ARDUINO
    #include <stdint.h>
#else
    #include <cstdint>
#endif


void midi2Processor::processProfileSysex(uint8_t groupOffset, uint8_t s7Byte){
	switch (ciType[groupOffset]){
		case MIDICI_PROFILE_INQUIRY: //Profile Inquiry	
			if (sysexPos[groupOffset] == 12 && recvProfileInquiry != 0){
				recvProfileInquiry(groupOffset + groupStart, remoteMUID[groupOffset], sysUniPort[groupOffset]);
			}
			break;
		case MIDICI_PROFILE_INQUIRYREPLY: //Reply to Profile Inquiry
			
			if(sysexPos[groupOffset] == 13){
				sys7IntBuffer[groupOffset][0] = (int)s7Byte;
			}
			if(sysexPos[groupOffset] == 14){
				sys7IntBuffer[groupOffset][0] += (int)s7Byte << 7;
			}
			
			if(sysexPos[groupOffset] == 13 + sys7CharBuffer[groupOffset][0]*5){
				sys7IntBuffer[groupOffset][1] = (int)s7Byte;
			}
			if(sysexPos[groupOffset] == 14 + sys7CharBuffer[groupOffset][0]*5){
				sys7IntBuffer[groupOffset][1] += (int)s7Byte << 7;
			}
			if(sysexPos[groupOffset] >= 15 && sysexPos[groupOffset] <= 12 + sys7IntBuffer[groupOffset][0]*5){
				uint8_t pos = (sysexPos[groupOffset] - 13) % 5;
				sys7CharBuffer[groupOffset][pos] = s7Byte;
				if(pos==4 && recvSetProfileEnabled!=0){
					uint8_t profile[5] = {sys7CharBuffer[groupOffset][0], sys7CharBuffer[groupOffset][1], sys7CharBuffer[groupOffset][2], sys7CharBuffer[groupOffset][3], sys7CharBuffer[groupOffset][4]};
					recvSetProfileEnabled(groupOffset + groupStart,remoteMUID[groupOffset], sysUniPort[groupOffset], profile); 
				}
			}
			
			if(sysexPos[groupOffset] >= 15 + sys7CharBuffer[groupOffset][0]*5  && sysexPos[groupOffset] <= 12 + sys7IntBuffer[groupOffset][0]*5 + sys7IntBuffer[groupOffset][1]*5){
				uint8_t pos = (sysexPos[groupOffset] - 13) % 5;
				sys7CharBuffer[groupOffset][pos] = s7Byte;
				if(pos==4 && recvSetProfileDisabled!=0){
					uint8_t profile[5] = {sys7CharBuffer[groupOffset][0], sys7CharBuffer[groupOffset][1], sys7CharBuffer[groupOffset][2], sys7CharBuffer[groupOffset][3], sys7CharBuffer[groupOffset][4]};
					recvSetProfileDisabled(groupOffset + groupStart,remoteMUID[groupOffset], sysUniPort[groupOffset], profile); 
				}
			}
			break;
		case MIDICI_PROFILE_SETON: //Set Profile On Message
			if(sysexPos[groupOffset] >= 13 && sysexPos[groupOffset] <= 17){
				sys7CharBuffer[groupOffset][sysexPos[groupOffset]-13] = s7Byte; 
			}
			if (sysexPos[groupOffset] == 16 && recvInvalidateMUID != 0){
				uint8_t profile[5] = {sys7CharBuffer[groupOffset][0], sys7CharBuffer[groupOffset][1], sys7CharBuffer[groupOffset][2], sys7CharBuffer[groupOffset][3], sys7CharBuffer[groupOffset][4]};
				recvSetProfileOn(groupOffset + groupStart,remoteMUID[groupOffset], sysUniPort[groupOffset], profile); 
			}
			break;
		case MIDICI_PROFILE_SETOFF: //Set Profile Off Message
			if(sysexPos[groupOffset] >= 13 && sysexPos[groupOffset] <= 17){
				sys7CharBuffer[groupOffset][sysexPos[groupOffset]-13] = s7Byte; 
			}
			if (sysexPos[groupOffset] == 16 && recvInvalidateMUID != 0){
				uint8_t profile[5] = {sys7CharBuffer[groupOffset][0], sys7CharBuffer[groupOffset][1], sys7CharBuffer[groupOffset][2], sys7CharBuffer[groupOffset][3], sys7CharBuffer[groupOffset][4]};
				recvSetProfileOff(groupOffset + groupStart,remoteMUID[groupOffset], sysUniPort[groupOffset], profile); 
			}
			break;	
		case MIDICI_PROFILE_ENABLED: //Set Profile Enabled Message
			if(sysexPos[groupOffset] >= 13 && sysexPos[groupOffset] <= 17){
				sys7CharBuffer[groupOffset][sysexPos[groupOffset]-13] = s7Byte; 
			}
			if (sysexPos[groupOffset] == 16 && recvInvalidateMUID != 0){
				uint8_t profile[5] = {sys7CharBuffer[groupOffset][0], sys7CharBuffer[groupOffset][1], sys7CharBuffer[groupOffset][2], sys7CharBuffer[groupOffset][3], sys7CharBuffer[groupOffset][4]};
				recvSetProfileEnabled(groupOffset + groupStart,remoteMUID[groupOffset], sysUniPort[groupOffset], profile); 
			}
			break;  
		case MIDICI_PROFILE_DISABLED: //Set Profile Diabled Message
			if(sysexPos[groupOffset] >= 13 && sysexPos[groupOffset] <= 17){
				sys7CharBuffer[groupOffset][sysexPos[groupOffset]-13] = s7Byte; 
			}
			if (sysexPos[groupOffset] == 16 && recvInvalidateMUID != 0){
				uint8_t profile[5] = {sys7CharBuffer[groupOffset][0], sys7CharBuffer[groupOffset][1], sys7CharBuffer[groupOffset][2], sys7CharBuffer[groupOffset][3], sys7CharBuffer[groupOffset][4]};
				recvSetProfileDisabled(groupOffset + groupStart,remoteMUID[groupOffset], sysUniPort[groupOffset], profile); 
			}
			break;  
	}
}

void midi2Processor::sendProfileListRequest(uint8_t group, uint32_t remoteMuid, uint8_t ciVersion, uint8_t destination){
	if(sendOutSysex ==0) return;
	uint8_t sysex[13];
	addCIHeader(MIDICI_PROFILE_INQUIRY,sysex,ciVersion);
	sysex[1] = destination;
	setBytesFromNumbers(sysex, remoteMuid, 9, 4);
	sendOutSysex(group,sysex,13,0);
}


void midi2Processor::sendProfileListResponse(uint8_t group, uint32_t remoteMuid, uint8_t ciVersion, uint8_t destination, uint8_t profilesEnabledLen, uint8_t* profilesEnabled, uint8_t profilesDisabledLen , uint8_t* profilesDisabled ){
	if(sendOutSysex ==0) return;
	uint8_t sysex[13];
	addCIHeader(MIDICI_PROFILE_INQUIRYREPLY, sysex, ciVersion);
	sysex[1] = destination;
	setBytesFromNumbers(sysex, remoteMuid, 9, 4);
	sendOutSysex(group,sysex,13,1);
	
	setBytesFromNumbers(sysex, profilesEnabledLen, 0, 2);
	sendOutSysex(group,sysex,2,2);
	sendOutSysex(group,profilesEnabled,profilesEnabledLen*5,2);
	
	setBytesFromNumbers(sysex, profilesDisabledLen, 0, 2);
	sendOutSysex(group,sysex,2,2);
	sendOutSysex(group,profilesDisabled,profilesDisabledLen*5,3);
}

void midi2Processor::sendProfileOn(uint8_t group, uint32_t remoteMuid, uint8_t ciVersion, uint8_t destination, uint8_t* profile){
	if(sendOutSysex ==0) return;
	uint8_t sysex[13];
	addCIHeader(MIDICI_PROFILE_SETON, sysex, ciVersion);
	sysex[1] = destination;
	setBytesFromNumbers(sysex, remoteMuid, 9, 4);
	sendOutSysex(group,sysex,13,1);
	sendOutSysex(group,profile,5,3);
}

void midi2Processor::sendProfileOff(uint8_t group, uint32_t remoteMuid, uint8_t ciVersion, uint8_t destination, uint8_t* profile){
	if(sendOutSysex ==0) return;
	uint8_t sysex[13];
	addCIHeader(MIDICI_PROFILE_SETOFF, sysex, ciVersion);
	sysex[1] = destination;
	setBytesFromNumbers(sysex, remoteMuid, 9, 4);
	sendOutSysex(group,sysex,13,1);
	sendOutSysex(group,profile,5,3);
}

void midi2Processor::sendProfileEnabled(uint8_t group, uint32_t remoteMuid, uint8_t ciVersion, uint8_t destination, uint8_t* profile){
	if(sendOutSysex ==0) return;
	uint8_t sysex[13];
	addCIHeader(MIDICI_PROFILE_ENABLED, sysex, ciVersion);
	sysex[1] = destination;
	setBytesFromNumbers(sysex, remoteMuid, 9, 4);
	sendOutSysex(group,sysex,13,1);
	sendOutSysex(group,profile,5,3);
}

void midi2Processor::sendProfileDisabled(uint8_t group, uint32_t remoteMuid, uint8_t ciVersion, uint8_t destination, uint8_t* profile){
	if(sendOutSysex ==0) return;
	uint8_t sysex[13];
	addCIHeader(MIDICI_PROFILE_DISABLED, sysex, ciVersion);
	sysex[1] = destination;
	setBytesFromNumbers(sysex, remoteMuid, 9, 4);
	sendOutSysex(group,sysex,13,1);
	sendOutSysex(group,profile,5,3);
}




#endif
