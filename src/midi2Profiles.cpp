#include "../include/midi2Profiles.h"
#include "../include/midi2Processor.h"
#ifndef M2_DISABLE_PROFILE




void midi2Processor::processProfileSysex(uint8_t group, uint8_t s7Byte){
	switch (midici[group].ciType){
		case MIDICI_PROFILE_INQUIRY: //Profile Inquiry	
			if (syExMessInt[group].pos == 12 && recvProfileInquiry != nullptr){
				recvProfileInquiry(group, midici[group]);
			}
			break;
		case MIDICI_PROFILE_INQUIRYREPLY: { //Reply to Profile Inquiry
            //Enabled Profiles Length
            if (syExMessInt[group].pos == 13 || syExMessInt[group].pos == 14) {
                syExMessInt[group].intbuffer1[0] += s7Byte << (7 * (syExMessInt[group].pos - 13));
            }

            //Disabled Profile Length
            int enabledProfileOffset = syExMessInt[group].intbuffer1[0] * 5 + 13;
            if (
                    syExMessInt[group].pos == enabledProfileOffset
                    || syExMessInt[group].pos == 1 + enabledProfileOffset
                    ) {
                syExMessInt[group].intbuffer1[1] += s7Byte << (7 * (syExMessInt[group].pos - enabledProfileOffset));
            }

            if (syExMessInt[group].pos >= 15 && syExMessInt[group].pos < enabledProfileOffset) {
                uint8_t pos = (syExMessInt[group].pos - 13) % 5;
                syExMessInt[group].buffer1[pos] = s7Byte;
                if (pos == 4 && recvSetProfileEnabled != nullptr) {
                    uint8_t profile[5] = {syExMessInt[group].buffer1[0], syExMessInt[group].buffer1[1],
                                          syExMessInt[group].buffer1[2], syExMessInt[group].buffer1[3],
                                          syExMessInt[group].buffer1[4]};
                    recvSetProfileEnabled(group, midici[group], profile);
                }
            }

            if (syExMessInt[group].pos >= 2 + enabledProfileOffset &&
                syExMessInt[group].pos < enabledProfileOffset + syExMessInt[group].intbuffer1[1] * 5) {
                uint8_t pos = (syExMessInt[group].pos - 13) % 5;
                syExMessInt[group].buffer1[pos] = s7Byte;
                if (pos == 4 && recvSetProfileDisabled != nullptr) {
                    uint8_t profile[5] = {syExMessInt[group].buffer1[0], syExMessInt[group].buffer1[1],
                                          syExMessInt[group].buffer1[2], syExMessInt[group].buffer1[3],
                                          syExMessInt[group].buffer1[4]};
                    recvSetProfileDisabled(group, midici[group], profile);
                }
            }
            break;
        }
		case MIDICI_PROFILE_SETON: //Set Profile On Message
			if(syExMessInt[group].pos >= 13 && syExMessInt[group].pos <= 17){
				syExMessInt[group].buffer1[syExMessInt[group].pos-13] = s7Byte;
			}
			if (syExMessInt[group].pos == 17 && recvSetProfileOn != nullptr){
				uint8_t profile[5] = {syExMessInt[group].buffer1[0], syExMessInt[group].buffer1[1], syExMessInt[group].buffer1[2], syExMessInt[group].buffer1[3], syExMessInt[group].buffer1[4]};
				recvSetProfileOn(group,midici[group], profile);
			}
			break;
		case MIDICI_PROFILE_SETOFF: //Set Profile Off Message
			if(syExMessInt[group].pos >= 13 && syExMessInt[group].pos <= 17){
				syExMessInt[group].buffer1[syExMessInt[group].pos-13] = s7Byte;
			}
			if (syExMessInt[group].pos == 17 && recvSetProfileOff != nullptr){
				uint8_t profile[5] = {syExMessInt[group].buffer1[0], syExMessInt[group].buffer1[1], syExMessInt[group].buffer1[2], syExMessInt[group].buffer1[3], syExMessInt[group].buffer1[4]};
				recvSetProfileOff(group,midici[group], profile);
			}
			break;	
		case MIDICI_PROFILE_ENABLED: //Set Profile Enabled Message
			if(syExMessInt[group].pos >= 13 && syExMessInt[group].pos <= 17){
				syExMessInt[group].buffer1[syExMessInt[group].pos-13] = s7Byte;
			}
			if (syExMessInt[group].pos == 17 && recvSetProfileEnabled != nullptr){
				uint8_t profile[5] = {syExMessInt[group].buffer1[0], syExMessInt[group].buffer1[1], syExMessInt[group].buffer1[2], syExMessInt[group].buffer1[3], syExMessInt[group].buffer1[4]};
				recvSetProfileEnabled(group,midici[group], profile);
			}
			break;  
		case MIDICI_PROFILE_DISABLED: //Set Profile Diabled Message
			if(syExMessInt[group].pos >= 13 && syExMessInt[group].pos <= 17){
				syExMessInt[group].buffer1[syExMessInt[group].pos-13] = s7Byte;
			}
			if (syExMessInt[group].pos == 17 && recvSetProfileDisabled != nullptr){
				uint8_t profile[5] = {syExMessInt[group].buffer1[0], syExMessInt[group].buffer1[1], syExMessInt[group].buffer1[2], syExMessInt[group].buffer1[3], syExMessInt[group].buffer1[4]};
				recvSetProfileDisabled(group,midici[group], profile);
			}
			break;
        case MIDICI_PROFILE_SPECIFIC_DATA:
            //Profile
            if(syExMessInt[group].pos >= 13 && syExMessInt[group].pos <= 17){
                syExMessInt[group].buffer1[syExMessInt[group].pos-13] = s7Byte;
                return;
            }
            if(syExMessInt[group].pos >= 18 || syExMessInt[group].pos <= 21){ //Length of Following Profile Specific Data
                syExMessInt[group].intbuffer1[0] += s7Byte << (7 * (syExMessInt[group].pos - 18 ));
                syExMessInt[group].intbuffer1[1] = 1;
                return;
            }


            //******************

            uint16_t charOffset = (syExMessInt[group].pos - 22) % S7_BUFFERLEN;
            uint16_t dataLength = syExMessInt[group].intbuffer1[0];
            if(
                    (syExMessInt[group].pos >= 22 && syExMessInt[group].pos <= 21 + dataLength)
                    || 	(dataLength == 0 && syExMessInt[group].pos == 21)
                    ){
                if(dataLength != 0 )syExMessInt[group].buffer1[charOffset] = s7Byte;

                bool lastByteOfSet = (syExMessInt[group].pos == 21 + dataLength);

                if(charOffset == S7_BUFFERLEN -1
                   || syExMessInt[group].pos == 21 + dataLength
                   || dataLength == 0
                        ){
                    uint8_t profile[5] = {syExMessInt[group].buffer1[0], syExMessInt[group].buffer1[1], syExMessInt[group].buffer1[2], syExMessInt[group].buffer1[3], syExMessInt[group].buffer1[4]};

                    recvProfileDetails(group, midici[group], profile, charOffset+1, syExMessInt[group].buffer1, syExMessInt[group].intbuffer1[1], lastByteOfSet);
                    syExMessInt[group].intbuffer1[1]++;
                }
            }


            //***********

            break;
	}
}

void midi2Processor::sendProfileListRequest(uint8_t group, uint32_t srcMUID, uint32_t destMuid, uint8_t destination){
	if(sendOutSysex == nullptr) return;
	uint8_t sysex[13];
    MIDICI midiCiHeader;
    midiCiHeader.ciType = MIDICI_PROFILE_INQUIRY;
    midiCiHeader.localMUID = srcMUID;
    midiCiHeader.remoteMUID = destMuid;
    midiCiHeader.deviceId = destination;
    createCIHeader(sysex, midiCiHeader);
	sendOutSysex(group,sysex,13,0);
}


void midi2Processor::sendProfileListResponse(uint8_t group, uint32_t srcMUID, uint32_t destMuid,  uint8_t destination, uint8_t profilesEnabledLen, uint8_t* profilesEnabled, uint8_t profilesDisabledLen , uint8_t* profilesDisabled ){
	if(sendOutSysex == nullptr) return;
	uint8_t sysex[13];
    MIDICI midiCiHeader;
    midiCiHeader.ciType = MIDICI_PROFILE_INQUIRYREPLY;
    midiCiHeader.localMUID = srcMUID;
    midiCiHeader.remoteMUID = destMuid;
    midiCiHeader.deviceId = destination;
    createCIHeader(sysex, midiCiHeader);
	sendOutSysex(group,sysex,13,1);
	
	setBytesFromNumbers(sysex, profilesEnabledLen, 0, 2);
	sendOutSysex(group,sysex,2,2);
	sendOutSysex(group,profilesEnabled,profilesEnabledLen*5,2);
	
	setBytesFromNumbers(sysex, profilesDisabledLen, 0, 2);
	sendOutSysex(group,sysex,2,2);
	sendOutSysex(group,profilesDisabled,profilesDisabledLen*5,3);
}

void midi2Processor::sendProfileMessage(uint8_t group, uint32_t srcMUID, uint32_t destMuid,  uint8_t destination,
                                        uint8_t* profile, uint8_t ciType){
    if(sendOutSysex == nullptr) return;
    uint8_t sysex[13];
    MIDICI midiCiHeader;
    midiCiHeader.ciType = ciType;
    midiCiHeader.localMUID = srcMUID;
    midiCiHeader.remoteMUID = destMuid;
    midiCiHeader.deviceId = destination;
    createCIHeader(sysex, midiCiHeader);
    sendOutSysex(group,sysex,13,1);
    sendOutSysex(group,profile,5,3);
}

void midi2Processor::sendProfileOn(uint8_t group, uint32_t srcMUID, uint32_t destMuid, uint8_t destination, uint8_t* profile){
    sendProfileMessage(group, srcMUID, destMuid, destination, profile, (uint8_t) MIDICI_PROFILE_SETON);
}

void midi2Processor::sendProfileOff(uint8_t group, uint32_t srcMUID, uint32_t destMuid, uint8_t destination, uint8_t* profile){
    sendProfileMessage(group, srcMUID, destMuid, destination, profile, (uint8_t) MIDICI_PROFILE_SETOFF);
}

void midi2Processor::sendProfileEnabled(uint8_t group, uint32_t srcMUID, uint32_t destMuid, uint8_t destination, uint8_t* profile){
    sendProfileMessage(group, srcMUID, destMuid, destination, profile, (uint8_t) MIDICI_PROFILE_ENABLED);
}

void midi2Processor::sendProfileDisabled(uint8_t group, uint32_t srcMUID, uint32_t destMuid, uint8_t destination, uint8_t* profile){
    sendProfileMessage(group, srcMUID, destMuid, destination, profile, (uint8_t) MIDICI_PROFILE_DISABLED);
}


void midi2Processor::sendProfileSpecificData(uint8_t group, uint32_t srcMUID, uint32_t destMuid, uint8_t destination,
                             uint8_t* profile, uint16_t datalen, uint8_t*  data){
    if(sendOutSysex == nullptr) return;
    uint8_t sysex[13];
    MIDICI midiCiHeader;
    midiCiHeader.ciType = MIDICI_PROFILE_SPECIFIC_DATA;
    midiCiHeader.localMUID = srcMUID;
    midiCiHeader.remoteMUID = destMuid;
    midiCiHeader.deviceId = destination;
    createCIHeader(sysex, midiCiHeader);
    sendOutSysex(group,sysex,13,1);
    setBytesFromNumbers(sysex, datalen, 0, 4);
    sendOutSysex(group,sysex,4,2);
    sendOutSysex(group,data,datalen,3);
}

#endif
