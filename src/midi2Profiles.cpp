#include "../include/midi2Profiles.h"
#include "../include/midi2Processor.h"
#include <cstdint>
#ifndef M2_DISABLE_PROFILE




void midi2Processor::processProfileSysex(uint8_t group, uint8_t s7Byte){
	switch (syExMess[group].ciType){
		case MIDICI_PROFILE_INQUIRY: //Profile Inquiry	
			if (syExMess[group]._pos == 12 && recvProfileInquiry != nullptr){
				recvProfileInquiry(group, syExMess[group].remoteMUID, syExMess[group].deviceId);
			}
			break;
		case MIDICI_PROFILE_INQUIRYREPLY: { //Reply to Profile Inquiry
            //Enabled Profiles Length
            if (syExMess[group]._pos == 13 || syExMess[group]._pos == 14) {
                syExMess[group].intbuffer1[0] += s7Byte << (7 * (syExMess[group]._pos - 13));
            }

            //Disabled Profile Length
            int enabledProfileOffset = syExMess[group].intbuffer1[0] * 5 + 13;
            if (
                    syExMess[group]._pos == enabledProfileOffset
                    || syExMess[group]._pos == 1 + enabledProfileOffset
                    ) {
                syExMess[group].intbuffer1[1] += s7Byte << (7 * (syExMess[group]._pos - enabledProfileOffset));
            }

            if (syExMess[group]._pos >= 15 && syExMess[group]._pos < enabledProfileOffset) {
                uint8_t pos = (syExMess[group]._pos - 13) % 5;
                syExMess[group].buffer1[pos] = s7Byte;
                if (pos == 4 && recvSetProfileEnabled != nullptr) {
                    uint8_t profile[5] = {syExMess[group].buffer1[0], syExMess[group].buffer1[1],
                                          syExMess[group].buffer1[2], syExMess[group].buffer1[3],
                                          syExMess[group].buffer1[4]};
                    recvSetProfileEnabled(group, syExMess[group].remoteMUID, syExMess[group].deviceId, profile);
                }
            }

            if (syExMess[group]._pos >= 2 + enabledProfileOffset &&
                syExMess[group]._pos < enabledProfileOffset + syExMess[group].intbuffer1[1] * 5) {
                uint8_t pos = (syExMess[group]._pos - 13) % 5;
                syExMess[group].buffer1[pos] = s7Byte;
                if (pos == 4 && recvSetProfileDisabled != nullptr) {
                    uint8_t profile[5] = {syExMess[group].buffer1[0], syExMess[group].buffer1[1],
                                          syExMess[group].buffer1[2], syExMess[group].buffer1[3],
                                          syExMess[group].buffer1[4]};
                    recvSetProfileDisabled(group, syExMess[group].remoteMUID, syExMess[group].deviceId, profile);
                }
            }
            break;
        }
		case MIDICI_PROFILE_SETON: //Set Profile On Message
			if(syExMess[group]._pos >= 13 && syExMess[group]._pos <= 17){
				syExMess[group].buffer1[syExMess[group]._pos-13] = s7Byte;
			}
			if (syExMess[group]._pos == 16 && recvInvalidateMUID != nullptr){
				uint8_t profile[5] = {syExMess[group].buffer1[0], syExMess[group].buffer1[1], syExMess[group].buffer1[2], syExMess[group].buffer1[3], syExMess[group].buffer1[4]};
				recvSetProfileOn(group,syExMess[group].remoteMUID, syExMess[group].deviceId, profile);
			}
			break;
		case MIDICI_PROFILE_SETOFF: //Set Profile Off Message
			if(syExMess[group]._pos >= 13 && syExMess[group]._pos <= 17){
				syExMess[group].buffer1[syExMess[group]._pos-13] = s7Byte;
			}
			if (syExMess[group]._pos == 16 && recvInvalidateMUID != nullptr){
				uint8_t profile[5] = {syExMess[group].buffer1[0], syExMess[group].buffer1[1], syExMess[group].buffer1[2], syExMess[group].buffer1[3], syExMess[group].buffer1[4]};
				recvSetProfileOff(group,syExMess[group].remoteMUID, syExMess[group].deviceId, profile);
			}
			break;	
		case MIDICI_PROFILE_ENABLED: //Set Profile Enabled Message
			if(syExMess[group]._pos >= 13 && syExMess[group]._pos <= 17){
				syExMess[group].buffer1[syExMess[group]._pos-13] = s7Byte;
			}
			if (syExMess[group]._pos == 16 && recvInvalidateMUID != nullptr){
				uint8_t profile[5] = {syExMess[group].buffer1[0], syExMess[group].buffer1[1], syExMess[group].buffer1[2], syExMess[group].buffer1[3], syExMess[group].buffer1[4]};
				recvSetProfileEnabled(group,syExMess[group].remoteMUID, syExMess[group].deviceId, profile);
			}
			break;  
		case MIDICI_PROFILE_DISABLED: //Set Profile Diabled Message
			if(syExMess[group]._pos >= 13 && syExMess[group]._pos <= 17){
				syExMess[group].buffer1[syExMess[group]._pos-13] = s7Byte;
			}
			if (syExMess[group]._pos == 16 && recvInvalidateMUID != nullptr){
				uint8_t profile[5] = {syExMess[group].buffer1[0], syExMess[group].buffer1[1], syExMess[group].buffer1[2], syExMess[group].buffer1[3], syExMess[group].buffer1[4]};
				recvSetProfileDisabled(group,syExMess[group].remoteMUID, syExMess[group].deviceId, profile);
			}
			break;  
	}
}

void midi2Processor::sendProfileListRequest(uint8_t group, uint32_t remoteMuid, uint8_t ciVersion, uint8_t destination){
	if(sendOutSysex == nullptr) return;
	uint8_t sysex[13];
	addCIHeader(MIDICI_PROFILE_INQUIRY,sysex,ciVersion);
	sysex[1] = destination;
	setBytesFromNumbers(sysex, remoteMuid, 9, 4);
	sendOutSysex(group,sysex,13,0);
}


void midi2Processor::sendProfileListResponse(uint8_t group, uint32_t remoteMuid, uint8_t ciVersion, uint8_t destination, uint8_t profilesEnabledLen, uint8_t* profilesEnabled, uint8_t profilesDisabledLen , uint8_t* profilesDisabled ){
	if(sendOutSysex == nullptr) return;
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
	if(sendOutSysex == nullptr) return;
	uint8_t sysex[13];
	addCIHeader(MIDICI_PROFILE_SETON, sysex, ciVersion);
	sysex[1] = destination;
	setBytesFromNumbers(sysex, remoteMuid, 9, 4);
	sendOutSysex(group,sysex,13,1);
	sendOutSysex(group,profile,5,3);
}

void midi2Processor::sendProfileOff(uint8_t group, uint32_t remoteMuid, uint8_t ciVersion, uint8_t destination, uint8_t* profile){
	if(sendOutSysex == nullptr) return;
	uint8_t sysex[13];
	addCIHeader(MIDICI_PROFILE_SETOFF, sysex, ciVersion);
	sysex[1] = destination;
	setBytesFromNumbers(sysex, remoteMuid, 9, 4);
	sendOutSysex(group,sysex,13,1);
	sendOutSysex(group,profile,5,3);
}

void midi2Processor::sendProfileEnabled(uint8_t group, uint32_t remoteMuid, uint8_t ciVersion, uint8_t destination, uint8_t* profile){
	if(sendOutSysex == nullptr) return;
	uint8_t sysex[13];
	addCIHeader(MIDICI_PROFILE_ENABLED, sysex, ciVersion);
	sysex[1] = destination;
	setBytesFromNumbers(sysex, remoteMuid, 9, 4);
	sendOutSysex(group,sysex,13,1);
	sendOutSysex(group,profile,5,3);
}

void midi2Processor::sendProfileDisabled(uint8_t group, uint32_t remoteMuid, uint8_t ciVersion, uint8_t destination, uint8_t* profile){
	if(sendOutSysex == nullptr) return;
	uint8_t sysex[13];
	addCIHeader(MIDICI_PROFILE_DISABLED, sysex, ciVersion);
	sysex[1] = destination;
	setBytesFromNumbers(sysex, remoteMuid, 9, 4);
	sendOutSysex(group,sysex,13,1);
	sendOutSysex(group,profile,5,3);
}




#endif
