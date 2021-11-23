#include "../include/midi2Protocol.h"
#include "../include/midi2Processor.h"
#include <cstdint>
#ifndef M2_DISABLE_PROTOCOL

void midi2Processor::processProtocolSysex(uint8_t group, uint8_t s7Byte){
	switch (midici[group].ciType){

		case MIDICI_PROTOCOL_NEGOTIATION:
		case MIDICI_PROTOCOL_NEGOTIATION_REPLY: {
            //Authority Level
            if (syExMessInt[group].pos == 13 ) {
                syExMessInt[group].intbuffer1[0] = s7Byte;
            }
            //Number of Supported Protocols (np)
            if (syExMessInt[group].pos == 14 ) {
                syExMessInt[group].intbuffer1[1] = s7Byte;
            }

            int protocolOffset = syExMessInt[group].intbuffer1[1] * 5 + 14;

            if (syExMessInt[group].pos >= 15 && syExMessInt[group].pos < protocolOffset) {
                uint8_t pos = (syExMessInt[group].pos - 14) % 5;
                syExMessInt[group].buffer1[pos] = s7Byte;
                if (pos == 4 && recvProtocolAvailable != nullptr) {
                    uint8_t protocol[5] = {syExMessInt[group].buffer1[0], syExMessInt[group].buffer1[1],
                                          syExMessInt[group].buffer1[2], syExMessInt[group].buffer1[3],
                                          syExMessInt[group].buffer1[4]};
                    recvProtocolAvailable(group, midici[group], syExMessInt[group].intbuffer1[0], protocol);
                }
            }
            break;
        }

        case MIDICI_PROTOCOL_SET: //Set Profile On Message
            //Authority Level
            if (syExMessInt[group].pos == 13 ) {
                syExMessInt[group].intbuffer1[0] = s7Byte;
            }
			if(syExMessInt[group].pos >= 14 && syExMessInt[group].pos <= 18){
				syExMessInt[group].buffer1[syExMessInt[group].pos-13] = s7Byte;
			}
			if (syExMessInt[group].pos == 18 && recvSetProtocol != nullptr){
				uint8_t protocol[5] = {syExMessInt[group].buffer1[0], syExMessInt[group].buffer1[1], syExMessInt[group].buffer1[2], syExMessInt[group].buffer1[3], syExMessInt[group].buffer1[4]};
                recvSetProtocol(group,midici[group], syExMessInt[group].intbuffer1[0], protocol);
			}
			break;

        case MIDICI_PROTOCOL_TEST_RESPONDER:
        case MIDICI_PROTOCOL_TEST:
            //Authority Level
            if (syExMessInt[group].pos == 13 ) {
                syExMessInt[group].intbuffer1[0] = s7Byte;
                syExMessInt[group].intbuffer1[1] = 1;
            }
            if(syExMessInt[group].pos >= 14 && syExMessInt[group].pos <= 61){
                if(s7Byte != syExMessInt[group].pos - 14){
                    syExMessInt[group].intbuffer1[1] = 0;
                }
            }
            if (syExMessInt[group].pos == 61 && recvProtocolTest != nullptr){
                recvProtocolTest(group,midici[group], syExMessInt[group].intbuffer1[0], !!(syExMessInt[group].intbuffer1[1]));
            }


            break;

		case MIDICI_PROTOCOL_CONFIRM: //Set Profile Off Message
            //Authority Level
            if (syExMessInt[group].pos == 13 ) {
                syExMessInt[group].intbuffer1[0] = s7Byte;
                if (recvSetProtocolConfirm != nullptr){
                    recvSetProtocolConfirm(group,midici[group], syExMessInt[group].intbuffer1[0]);
                }
            }
			break;
	}
}




void midi2Processor::sendProtocolNegotiation(uint8_t group, uint32_t srcMUID, uint32_t destMuid,
                                             uint8_t authorityLevel, uint8_t numProtocols, uint8_t* protocols ){
	if(sendOutSysex == nullptr) return;
	uint8_t sysex[14];
    MIDICI midiCiHeader;
    midiCiHeader.ciType = MIDICI_PROTOCOL_NEGOTIATION;
    midiCiHeader.localMUID = srcMUID;
    midiCiHeader.remoteMUID = destMuid;
    createCIHeader(sysex, midiCiHeader);
    sysex[13] = authorityLevel;
    sendOutSysex(group,sysex,14,1);
	sendOutSysex(group,protocols,numProtocols*5,3);
}

void midi2Processor::sendProtocolNegotiationReply(uint8_t group, uint32_t srcMUID, uint32_t destMuid,
                                             uint8_t authorityLevel, uint8_t numProtocols, uint8_t* protocols ){
    if(sendOutSysex == nullptr) return;
    uint8_t sysex[14];
    MIDICI midiCiHeader;
    midiCiHeader.ciType = MIDICI_PROTOCOL_NEGOTIATION_REPLY;
    midiCiHeader.localMUID = srcMUID;
    midiCiHeader.remoteMUID = destMuid;
    createCIHeader(sysex, midiCiHeader);
    sysex[13] = authorityLevel;
    sendOutSysex(group,sysex,14,1);
    sendOutSysex(group,protocols,numProtocols*5,3);
}


void midi2Processor::sendSetProtocol(uint8_t group, uint32_t srcMUID, uint32_t destMuid,
                                 uint8_t authorityLevel, uint8_t* protocol){
	if(sendOutSysex == nullptr) return;
    uint8_t sysex[14];
    MIDICI midiCiHeader;
    midiCiHeader.ciType = MIDICI_PROTOCOL_SET;
    midiCiHeader.localMUID = srcMUID;
    midiCiHeader.remoteMUID = destMuid;
    createCIHeader(sysex, midiCiHeader);
    sysex[13] = authorityLevel;
    sendOutSysex(group,sysex,14,1);
	sendOutSysex(group,protocol,5,3);
}
void midi2Processor::sendProtocolTest(uint8_t group, uint32_t srcMUID, uint32_t destMuid,
                                 uint8_t authorityLevel){
	if(sendOutSysex == nullptr) return;
    uint8_t sysex[14];
    MIDICI midiCiHeader;
    midiCiHeader.ciType = MIDICI_PROTOCOL_TEST;
    midiCiHeader.localMUID = srcMUID;
    midiCiHeader.remoteMUID = destMuid;
    createCIHeader(sysex, midiCiHeader);
    sysex[13] = authorityLevel;
    sendOutSysex(group,sysex,14,1);
    uint8_t testData[48]={0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47};
	sendOutSysex(group,testData,48,3);
}

void midi2Processor::sendProtocolTestResponder(uint8_t group, uint32_t srcMUID, uint32_t destMuid,
                                      uint8_t authorityLevel){
    if(sendOutSysex == nullptr) return;
    uint8_t sysex[14];
    MIDICI midiCiHeader;
    midiCiHeader.ciType = MIDICI_PROTOCOL_TEST_RESPONDER;
    midiCiHeader.localMUID = srcMUID;
    midiCiHeader.remoteMUID = destMuid;
    createCIHeader(sysex, midiCiHeader);
    sysex[13] = authorityLevel;
    sendOutSysex(group,sysex,14,1);
    uint8_t testData[48]={0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47};
    sendOutSysex(group,testData,48,3);
}

#endif
