#include "../include/midi2PropertyExchange.h"
#include "../include/midi2Processor.h"

#ifndef M2_DISABLE_PE

#include <cstring>

void midi2Processor::processPESysex(uint8_t group, uint8_t s7Byte){
	uint8_t peRequestIdx;
	
	if(midici[group].ciType == MIDICI_PE_CAPABILITY && syExMessInt[group].pos == 13){
		if(recvPECapabilities != nullptr)recvPECapabilities(group,midici[group], s7Byte);
	} else
	if(midici[group].ciType == MIDICI_PE_CAPABILITYREPLY && syExMessInt[group].pos == 13){
        if(recvPECapabilities != nullptr)recvPECapabilitiesReplies(group,midici[group], s7Byte);
	} else {
        if(syExMessInt[group].pos == 13){
            syExMessInt[group].peRequestIdx = getPERequestId(group, midici[group].remoteMUID, s7Byte);
            return;
        }

        if(syExMessInt[group].pos > 13){
            peRequestIdx=syExMessInt[group].peRequestIdx;
            if(peRequestIdx == 255){
                //Ignore this Get Message
                return;
            }
        }

        if(syExMessInt[group].pos == 14 || syExMessInt[group].pos == 15){ //header Length
            syExMessInt[group].intbuffer1[0] += s7Byte << (7 * (syExMessInt[group].pos - 14 ));
            return;
        }

		uint16_t headerLength = syExMessInt[group].intbuffer1[0];
		
		if(syExMessInt[group].pos >= 16 && syExMessInt[group].pos <= 15 + headerLength){
			processPEHeader(group, peRequestIdx, s7Byte);

            if(syExMessInt[group].pos == 15 + headerLength
                && (
                    midici[group].ciType == MIDICI_PE_GET
                    || midici[group].ciType == MIDICI_PE_SETREPLY
                    || midici[group].ciType == MIDICI_PE_SUBREPLY
                    || midici[group].ciType == MIDICI_PE_NOTIFY
                    )
            ){
                if(midici[group].ciType == MIDICI_PE_GET && recvPEGetInquiry != nullptr){
                    recvPEGetInquiry(group, midici[group], peRquestDetails[peRequestIdx]);
                }
                if(midici[group].ciType == MIDICI_PE_SETREPLY && recvPESetReply != nullptr){
                    recvPESetReply(group, midici[group], peRquestDetails[peRequestIdx]);
                }
                if(midici[group].ciType == MIDICI_PE_SUBREPLY && recvPESubReply != nullptr){
                    recvPESubReply(group, midici[group], peRquestDetails[peRequestIdx]);
                }
                if(midici[group].ciType == MIDICI_PE_NOTIFY && recvPENotify != nullptr){
                    recvPENotify(group, midici[group], peRquestDetails[peRequestIdx]);
                }
                cleanupRequestId(group, midici[group].remoteMUID, peRquestDetails[peRequestIdx].requestId);
            }else if(syExMessInt[group].pos == 15 + headerLength){
                peRquestDetails[peRequestIdx].totalChunks = 0;
                peRquestDetails[peRequestIdx].numChunks = 0;
            }
            return;
        }

		if(syExMessInt[group].pos == 16 + headerLength || syExMessInt[group].pos == 17 + headerLength){
			peRquestDetails[peRequestIdx].totalChunks += s7Byte << (7 * (syExMessInt[group].pos - 16 - headerLength ));
            return;
        }

		if(syExMessInt[group].pos == 18 + headerLength  || syExMessInt[group].pos == 19 + headerLength){
			peRquestDetails[peRequestIdx].numChunks += s7Byte << (7 * (syExMessInt[group].pos - 18 - headerLength ));
            return;
		}

		
		if(syExMessInt[group].pos == 20 + headerLength || syExMessInt[group].pos == 21 + headerLength){ //Body Length
			syExMessInt[group].intbuffer1[1] += s7Byte << (7 * (syExMessInt[group].pos - 20 - headerLength ));
		}

		uint16_t bodyLength = syExMessInt[group].intbuffer1[1];
        uint16_t initPos = 22 + headerLength;
        uint8_t charOffset = (syExMessInt[group].pos - initPos) % PE_HEAD_BUFFERLEN;

		if(
			(syExMessInt[group].pos >= initPos && syExMessInt[group].pos <= initPos - 1 + bodyLength)
			|| 	(bodyLength == 0 && syExMessInt[group].pos == initPos -1)
		){
			if(bodyLength != 0 )syExMessInt[group].buffer1[charOffset] = s7Byte;

            bool lastByteOfSet = (peRquestDetails[peRequestIdx].numChunks == peRquestDetails[peRequestIdx].totalChunks && syExMessInt[group].pos == initPos - 1 + bodyLength);
			
			if(charOffset == PE_HEAD_BUFFERLEN -1 
				|| syExMessInt[group].pos == initPos - 1 + bodyLength
				|| bodyLength == 0 
			){
				if(midici[group].ciType == MIDICI_PE_SUB && recvPESubInquiry != nullptr){
					recvPESubInquiry(group, midici[group], peRquestDetails[peRequestIdx], charOffset+1, syExMessInt[group].buffer1, lastByteOfSet);
				}
				
				if(midici[group].ciType == MIDICI_PE_SET && recvPESetInquiry != nullptr){
					recvPESetInquiry(group, midici[group], peRquestDetails[peRequestIdx], charOffset+1, syExMessInt[group].buffer1, lastByteOfSet);
				}

                peRquestDetails[peRequestIdx].partialChunkCount++;
			} 
			
			if(lastByteOfSet){
				cleanupRequestId(group, midici[group].remoteMUID, peRquestDetails[peRequestIdx].requestId);
			}
		}
			
	}
	
}



void midi2Processor::processPEHeader(uint8_t group, uint8_t peRequestIdx, uint8_t s7Byte){

    /*char messStr[20];
    sprintf(messStr," p: %d %c",peRquestDetails[peRequestIdx]._headerPos, s7Byte);
    debug(messStr);*/

	int clear=0;

	if((peRquestDetails[peRequestIdx]._headerState & 0xF) == PE_HEAD_STATE_IN_STRING){
		if (s7Byte == '"' && syExMessInt[group].buffer1[peRquestDetails[peRequestIdx]._headerPos-1]!='\\') {
			if((peRquestDetails[peRequestIdx]._headerState & 0xF0) == PE_HEAD_KEY){
                peRquestDetails[peRequestIdx]._pvoid = nullptr;
                peRquestDetails[peRequestIdx]._headerProp = 0;
				if(!strcmp((const char*)syExMessInt[group].buffer1,"resource")){
                    peRquestDetails[peRequestIdx]._pvoid = &peRquestDetails[peRequestIdx].resource;
                    peRquestDetails[peRequestIdx]._headerProp = 1;
				}
                if(!strcmp((const char*)syExMessInt[group].buffer1,"path")){
                    peRquestDetails[peRequestIdx]._pvoid = &peRquestDetails[peRequestIdx].path;
                    peRquestDetails[peRequestIdx]._headerProp = 1;
                }
                if(!strcmp((const char*)syExMessInt[group].buffer1,"mediaType")){
                    peRquestDetails[peRequestIdx]._pvoid = &peRquestDetails[peRequestIdx].mediaType;
                    peRquestDetails[peRequestIdx]._headerProp = 1;
                }
				if(!strcmp((const char*)syExMessInt[group].buffer1,"command")){
					//_pvoid = &peRquestDetails[peRequestIdx].command;
                    peRquestDetails[peRequestIdx]._headerProp = 1;
				}
                if(!strcmp((const char*)syExMessInt[group].buffer1,"action")){
					//_pvoid = &peRquestDetails[peRequestIdx].command;
                    peRquestDetails[peRequestIdx]._headerProp = 2;
				}
				if(!strcmp((const char*)syExMessInt[group].buffer1,"resId")){
                   // debug("resId!");
                    peRquestDetails[peRequestIdx]._pvoid = &peRquestDetails[peRequestIdx].resId;
                    peRquestDetails[peRequestIdx]._headerProp = 1;
				}
				if(!strcmp((const char*)syExMessInt[group].buffer1,"subscribeId")){
                    peRquestDetails[peRequestIdx]._pvoid = &peRquestDetails[peRequestIdx].subscribeId;
                    peRquestDetails[peRequestIdx]._headerProp = 1;
				}
                if(!strcmp((const char*)syExMessInt[group].buffer1,"status")){
                    peRquestDetails[peRequestIdx]._pvoid = &peRquestDetails[peRequestIdx].status;
                }
				if(!strcmp((const char*)syExMessInt[group].buffer1,"offset")){
                    peRquestDetails[peRequestIdx]._pvoid = &peRquestDetails[peRequestIdx].offset;
				}
				if(!strcmp((const char*)syExMessInt[group].buffer1,"limit")){
                    peRquestDetails[peRequestIdx]._pvoid = &peRquestDetails[peRequestIdx].limit;
				}
                if(!strcmp((const char*)syExMessInt[group].buffer1,"setPartial")){
                    peRquestDetails[peRequestIdx]._pvoid = &peRquestDetails[peRequestIdx].partial;
				}
				if(!strcmp((const char*)syExMessInt[group].buffer1,"mutualEncoding")){
                    peRquestDetails[peRequestIdx]._headerProp = 3;
				}
			}
            else if(peRquestDetails[peRequestIdx]._pvoid != nullptr){
                if(peRquestDetails[peRequestIdx]._headerProp == 0){
                    memcpy(peRquestDetails[peRequestIdx]._pvoid, syExMessInt[group].buffer1, PE_HEAD_BUFFERLEN * sizeof(char));
                }

                peRquestDetails[peRequestIdx]._headerProp=0;
                peRquestDetails[peRequestIdx]._pvoid = nullptr;
			}
            else if(peRquestDetails[peRequestIdx]._headerProp == 1){
				if(!strcmp((const char*)syExMessInt[group].buffer1,"start")){
					peRquestDetails[peRequestIdx].command = MIDICI_PE_COMMAND_START;
				}
				if(!strcmp((const char*)syExMessInt[group].buffer1,"end")){
					peRquestDetails[peRequestIdx].command = MIDICI_PE_COMMAND_END;
				}
				if(!strcmp((const char*)syExMessInt[group].buffer1,"partial")){
					peRquestDetails[peRequestIdx].command = MIDICI_PE_COMMAND_PARTIAL;
				}
				if(!strcmp((const char*)syExMessInt[group].buffer1,"full")){
					peRquestDetails[peRequestIdx].command = MIDICI_PE_COMMAND_FULL;
				}
				if(!strcmp((const char*)syExMessInt[group].buffer1,"notify")){
					peRquestDetails[peRequestIdx].command = MIDICI_PE_COMMAND_NOTIFY;
				};
                peRquestDetails[peRequestIdx]._headerProp=0;
                peRquestDetails[peRequestIdx]._pvoid = nullptr;
			}
            else if(peRquestDetails[peRequestIdx]._headerProp == 2){
				if(!strcmp((const char*)syExMessInt[group].buffer1,"copy")){
					peRquestDetails[peRequestIdx].action = EXP_MIDICI_PE_ACTION_COPY;
				}
				if(!strcmp((const char*)syExMessInt[group].buffer1,"move")){
					peRquestDetails[peRequestIdx].action = EXP_MIDICI_PE_ACTION_MOVE;
				}
				if(!strcmp((const char*)syExMessInt[group].buffer1,"delete")){
					peRquestDetails[peRequestIdx].action = EXP_MIDICI_PE_ACTION_DELETE;
				}
				if(!strcmp((const char*)syExMessInt[group].buffer1,"createDirectory")){
					peRquestDetails[peRequestIdx].action = EXP_MIDICI_PE_ACTION_CREATE_DIR;
				}
                peRquestDetails[peRequestIdx]._headerProp=0;
                peRquestDetails[peRequestIdx]._pvoid = nullptr;
			}
            else if(peRquestDetails[peRequestIdx]._headerProp == 3){
                if(!strcmp((const char*)syExMessInt[group].buffer1,"ASCII")){
                    peRquestDetails[peRequestIdx].mutualEncoding = MIDICI_PE_ASCII;
                }
                if(!strcmp((const char*)syExMessInt[group].buffer1,"Mcoded7")){
                    peRquestDetails[peRequestIdx].mutualEncoding = MIDICI_PE_MCODED7;
                }
                if(!strcmp((const char*)syExMessInt[group].buffer1,"zlib+Mcoded7")){
                    peRquestDetails[peRequestIdx].mutualEncoding = MIDICI_PE_MCODED7ZLIB;
                }
                peRquestDetails[peRequestIdx]._headerProp=0;
                peRquestDetails[peRequestIdx]._pvoid = nullptr;
            }
			clear=1;
        }
        else if(peRquestDetails[peRequestIdx]._headerProp >= 1 && peRquestDetails[peRequestIdx]._pvoid != nullptr){

            char *t = (char *)peRquestDetails[peRequestIdx]._pvoid;
            t[peRquestDetails[peRequestIdx]._headerProp-1] = s7Byte;
            t[peRquestDetails[peRequestIdx]._headerProp]='\0';
            peRquestDetails[peRequestIdx]._headerProp++;
        }
        else if(peRquestDetails[peRequestIdx]._headerPos +1 < PE_HEAD_BUFFERLEN){
			syExMessInt[group].buffer1[peRquestDetails[peRequestIdx]._headerPos++] = s7Byte;
        }
	} else if((peRquestDetails[peRequestIdx]._headerState & 0xF) == PE_HEAD_STATE_IN_NUMBER) {;
		if ((s7Byte >= '0' && s7Byte <= '9') ) {
			int *n = (int *)peRquestDetails[peRequestIdx]._pvoid;
			*n =  *n * 10 + (s7Byte - '0');
		}else if(peRquestDetails[peRequestIdx]._pvoid != nullptr){
            peRquestDetails[peRequestIdx]._pvoid = nullptr;
			clear=1;
			peRquestDetails[peRequestIdx]._headerState=PE_HEAD_KEY + (peRquestDetails[peRequestIdx]._headerState & 0xF);
		}
	} else if (s7Byte == ':') {
		peRquestDetails[peRequestIdx]._headerState=PE_HEAD_VALUE + (peRquestDetails[peRequestIdx]._headerState & 0xF);
	}else if (s7Byte == ',') {
		peRquestDetails[peRequestIdx]._headerState = PE_HEAD_KEY + (peRquestDetails[peRequestIdx]._headerState & 0xF);
	}else if ((s7Byte >= '0' && s7Byte <= '9') ) {
		int *n = (int *)peRquestDetails[peRequestIdx]._pvoid;
		*n =  s7Byte - '0';
		peRquestDetails[peRequestIdx]._headerState = (peRquestDetails[peRequestIdx]._headerState & 0xF0) + PE_HEAD_STATE_IN_NUMBER;
	}else if (s7Byte == 't' || s7Byte == 'f') {
        bool *b = (bool *)peRquestDetails[peRequestIdx]._pvoid;
        *b = s7Byte == 't';
        peRquestDetails[peRequestIdx]._headerState = (peRquestDetails[peRequestIdx]._headerState & 0xF0) + PE_HEAD_STATE_IN_BOOL;
    }else if (s7Byte == '"') {
		peRquestDetails[peRequestIdx]._headerState = (peRquestDetails[peRequestIdx]._headerState & 0xF0) + PE_HEAD_STATE_IN_STRING;
	}
	
	if(clear){
		memset(syExMessInt[group].buffer1, 0, PE_HEAD_BUFFERLEN);
		peRquestDetails[peRequestIdx]._headerPos=0;
		peRquestDetails[peRequestIdx]._headerState = (peRquestDetails[peRequestIdx]._headerState & 0xF0) + PE_HEAD_STATE_IN_OBJECT;
	}
	
}

void midi2Processor::sendPECapabilityRequest(uint8_t group, uint32_t srcMUID, uint32_t destMuid, uint8_t numSimulRequests){
	if(sendOutSysex == nullptr) return;
	uint8_t sysex[14];
    MIDICI midiCiHeader;
    midiCiHeader.ciType = MIDICI_PE_CAPABILITY;
    midiCiHeader.localMUID = srcMUID;
    midiCiHeader.remoteMUID = destMuid;
    createCIHeader(sysex, midiCiHeader);
	sysex[13] = numSimulRequests;
	sendOutSysex(group,sysex,14,0);
}

void midi2Processor::sendPECapabilityReply(uint8_t group, uint32_t srcMUID, uint32_t destMuid, uint8_t numSimulRequests){
    if(sendOutSysex == nullptr) return;
    uint8_t sysex[14];
    MIDICI midiCiHeader;
    midiCiHeader.ciType = MIDICI_PE_CAPABILITYREPLY;
    midiCiHeader.localMUID = srcMUID;
    midiCiHeader.remoteMUID = destMuid;
    createCIHeader(sysex, midiCiHeader);
    sysex[13] = numSimulRequests;
    sendOutSysex(group,sysex,14,0);
}

void midi2Processor::sendPEGet(uint8_t group, uint32_t srcMUID, uint32_t destMuid, uint8_t requestId, uint16_t headerLen, uint8_t* header){
	if(sendOutSysex == nullptr) return;
	uint8_t sysex[13];
    MIDICI midiCiHeader;
    midiCiHeader.ciType = MIDICI_PE_GET;
    midiCiHeader.localMUID = srcMUID;
    midiCiHeader.remoteMUID = destMuid;
    createCIHeader(sysex, midiCiHeader);
	sendOutSysex(group,sysex,13,1);
	
	sysex[0] = requestId;
	setBytesFromNumbers(sysex, headerLen, 1, 2);
	sendOutSysex(group,sysex,3,2);		
	sendOutSysex(group, header,headerLen,2);

    setBytesFromNumbers(sysex, 1, 0, 2);
    setBytesFromNumbers(sysex, 1, 2, 2);
    setBytesFromNumbers(sysex, 0, 4, 2);
    sendOutSysex(group,sysex,6,3);
}

void midi2Processor::sendPESet(uint8_t group, uint32_t srcMUID, uint32_t destMuid, uint8_t requestId, uint16_t headerLen, uint8_t* header, uint16_t numberOfChunks, uint16_t numberOfThisChunk, uint16_t bodyLength , uint8_t* body ){
    if(sendOutSysex == nullptr) return;
    uint8_t sysex[13];
    MIDICI midiCiHeader;
    midiCiHeader.ciType = MIDICI_PE_SET;
    midiCiHeader.localMUID = srcMUID;
    midiCiHeader.remoteMUID = destMuid;
    createCIHeader(sysex, midiCiHeader);
    sendOutSysex(group,sysex,13,1);

    sysex[0] = requestId;
    setBytesFromNumbers(sysex, headerLen, 1, 2);
    sendOutSysex(group,sysex,3,2);
    sendOutSysex(group, header,headerLen,2);

    setBytesFromNumbers(sysex, numberOfChunks, 0, 2);
    setBytesFromNumbers(sysex, numberOfThisChunk, 2, 2);
    setBytesFromNumbers(sysex, bodyLength, 4, 2);
    sendOutSysex(group,sysex,6,2);

    sendOutSysex(group,body,bodyLength,3);
}

void midi2Processor::sendPEGetReply(uint8_t group, uint32_t srcMUID, uint32_t destMuid, uint8_t requestId, uint16_t headerLen, uint8_t* header, uint16_t numberOfChunks, uint16_t numberOfThisChunk, uint16_t bodyLength , uint8_t* body ){
	if(sendOutSysex == nullptr) return;
	uint8_t sysex[13];
    MIDICI midiCiHeader;
    midiCiHeader.ciType = MIDICI_PE_GETREPLY;
    midiCiHeader.localMUID = srcMUID;
    midiCiHeader.remoteMUID = destMuid;
    createCIHeader(sysex, midiCiHeader);
	sendOutSysex(group,sysex,13,1);
	
	sysex[0] = requestId;
	setBytesFromNumbers(sysex, headerLen, 1, 2);
	sendOutSysex(group,sysex,3,2);		
	sendOutSysex(group, header,headerLen,2);
	
	setBytesFromNumbers(sysex, numberOfChunks, 0, 2);
	setBytesFromNumbers(sysex, numberOfThisChunk, 2, 2);
	setBytesFromNumbers(sysex, bodyLength, 4, 2);
	sendOutSysex(group,sysex,6,2);
	
	sendOutSysex(group,body,bodyLength,3);
}

void midi2Processor::sendPEGetReplyStreamStart(uint8_t group, uint32_t srcMUID, uint32_t destMuid, uint8_t requestId, uint16_t headerLen, uint8_t* header, uint16_t numberOfChunks, uint16_t numberOfThisChunk, uint16_t bodyLength){
	if(sendOutSysex == nullptr) return;
	uint8_t sysex[13];
    MIDICI midiCiHeader;
    midiCiHeader.ciType = MIDICI_PE_GETREPLY;
    midiCiHeader.localMUID = srcMUID;
    midiCiHeader.remoteMUID = destMuid;
	sendOutSysex(group,sysex,13,1);
	
	sysex[0] = requestId;
	setBytesFromNumbers(sysex, headerLen, 1, 2);
	sendOutSysex(group,sysex,3,2);		
	sendOutSysex(group, header,headerLen,2);
	
	setBytesFromNumbers(sysex, numberOfChunks, 0, 2);
	setBytesFromNumbers(sysex, numberOfThisChunk, 2, 2);
	setBytesFromNumbers(sysex, bodyLength, 4, 2);
	sendOutSysex(group,sysex,6,2);
}

void midi2Processor::sendPEGetReplyStreamContinue(uint8_t group, uint16_t partialLength, uint8_t* part, bool last ){
	sendOutSysex(group,part,partialLength, last?3:2);
}

void midi2Processor::sendPESub(uint8_t group, uint32_t srcMUID, uint32_t destMuid, uint8_t requestId, uint16_t headerLen, uint8_t* header, uint16_t numberOfChunks, uint16_t numberOfThisChunk, uint16_t bodyLength , uint8_t* body ){
    if(sendOutSysex == nullptr) return;
    uint8_t sysex[13];
    MIDICI midiCiHeader;
    midiCiHeader.ciType = MIDICI_PE_SUB;
    midiCiHeader.localMUID = srcMUID;
    midiCiHeader.remoteMUID = destMuid;
    createCIHeader(sysex, midiCiHeader);
    sendOutSysex(group,sysex,13,1);

    sysex[0] = requestId;
    setBytesFromNumbers(sysex, headerLen, 1, 2);
    sendOutSysex(group,sysex,3,2);
    sendOutSysex(group, header,headerLen,2);

    setBytesFromNumbers(sysex, numberOfChunks, 0, 2);
    setBytesFromNumbers(sysex, numberOfThisChunk, 2, 2);
    setBytesFromNumbers(sysex, bodyLength, 4, 2);
    sendOutSysex(group,sysex,6,2);

    sendOutSysex(group,body,bodyLength,3);
}

void midi2Processor::sendPESubReply(uint8_t group, uint32_t srcMUID, uint32_t destMuid, uint8_t requestId, uint16_t headerLen, uint8_t* header){
	if(sendOutSysex == nullptr) return;
	uint8_t sysex[13];
    MIDICI midiCiHeader;
    midiCiHeader.ciType = MIDICI_PE_SUBREPLY;
    midiCiHeader.localMUID = srcMUID;
    midiCiHeader.remoteMUID = destMuid;
    createCIHeader(sysex, midiCiHeader);
	sendOutSysex(group,sysex,13,1);
	
	sysex[0] = requestId;
	setBytesFromNumbers(sysex, headerLen, 1, 2);
	sendOutSysex(group,sysex,3,2);
    sendOutSysex(group, header,headerLen,2);

    setBytesFromNumbers(sysex, 1, 0, 2);
    setBytesFromNumbers(sysex, 1, 2, 2);
    setBytesFromNumbers(sysex, 0, 4, 2);
    sendOutSysex(group,sysex,6,3);
}

void midi2Processor::sendPENotify(uint8_t group, uint32_t srcMUID, uint32_t destMuid, uint8_t requestId, uint16_t headerLen, uint8_t* header){
    if(sendOutSysex == nullptr) return;
    uint8_t sysex[13];
    MIDICI midiCiHeader;
    midiCiHeader.ciType = MIDICI_PE_NOTIFY;
    midiCiHeader.localMUID = srcMUID;
    midiCiHeader.remoteMUID = destMuid;
    createCIHeader(sysex, midiCiHeader);
    sendOutSysex(group,sysex,13,1);

    sysex[0] = requestId;
    setBytesFromNumbers(sysex, headerLen, 1, 2);
    sendOutSysex(group,sysex,3,2);
    sendOutSysex(group, header,headerLen,2);

    setBytesFromNumbers(sysex, 1, 0, 2);
    setBytesFromNumbers(sysex, 1, 2, 2);
    setBytesFromNumbers(sysex, 0, 4, 2);
    sendOutSysex(group,sysex,6,3);
}

void midi2Processor::sendPESetReply(uint8_t group, uint32_t srcMUID, uint32_t destMuid, uint8_t requestId, uint16_t headerLen, uint8_t* header){
	if(sendOutSysex == nullptr) return;
	uint8_t sysex[13];
    MIDICI midiCiHeader;
    midiCiHeader.ciType = MIDICI_PE_SETREPLY;
    midiCiHeader.localMUID = srcMUID;
    midiCiHeader.remoteMUID = destMuid;
    createCIHeader(sysex, midiCiHeader);
	sendOutSysex(group,sysex,13,1);
	
	sysex[0] = requestId;
	setBytesFromNumbers(sysex, headerLen, 1, 2);
	sendOutSysex(group,sysex,3,2);
    sendOutSysex(group, header,headerLen,2);

    setBytesFromNumbers(sysex, 1, 0, 2);
    setBytesFromNumbers(sysex, 1, 2, 2);
    setBytesFromNumbers(sysex, 0, 4, 2);
    sendOutSysex(group,sysex,6,3);
}


uint8_t midi2Processor::getPERequestId(uint8_t group, uint32_t muid, uint8_t requestId){
    uint8_t peRequestIdx = 255;

    for(uint8_t i =0;i<numRequests;i++) {
        if (
                peRquestDetails[i].requestId == requestId
                && peRquestDetails[i].muid == muid
                && peRquestDetails[i].group == group
                ) {
            return i;
        }
    }
    for(uint8_t i =0;i<numRequests;i++){
        if(peRquestDetails[i].requestId == 255){
            peRquestDetails[i].requestId = requestId;
            peRquestDetails[i].muid = muid;
            peRquestDetails[i].group = group;
            return i;
        }
    }
    return peRequestIdx;
}

void midi2Processor::cleanupRequestId(uint8_t group, uint32_t muid, uint8_t requestId){
	for(uint8_t i =0;i<numRequests;i++){
		if(
                peRquestDetails[i].requestId == requestId
                && peRquestDetails[i].muid == muid
                && peRquestDetails[i].group == group
                ){
            cleanupRequest(i);
			return;
		}
	}
}

void midi2Processor::cleanupRequest(uint8_t reqpos){
    peRquestDetails[reqpos].requestId = 255;
    peRquestDetails[reqpos].group = 255;
    peRquestDetails[reqpos].muid = M2_CI_BROADCAST;
    memset(peRquestDetails[reqpos].resource,0,PE_HEAD_BUFFERLEN);
    memset(peRquestDetails[reqpos].resId,0,PE_HEAD_BUFFERLEN);
    memset(peRquestDetails[reqpos].subscribeId,0,PE_HEAD_BUFFERLEN);
    memset(peRquestDetails[reqpos].mediaType,0,PE_HEAD_BUFFERLEN);
    memset(peRquestDetails[reqpos].path,0,EXP_MIDICI_PE_EXPERIMENTAL_PATH);
    peRquestDetails[reqpos].offset=-1;
    peRquestDetails[reqpos].limit=-1;
    peRquestDetails[reqpos].status=-1;
    peRquestDetails[reqpos].command=0;
    peRquestDetails[reqpos].action=0;
    peRquestDetails[reqpos].partial=false;
    peRquestDetails[reqpos].totalChunks=-1;
    peRquestDetails[reqpos].numChunks=-1;
    peRquestDetails[reqpos].partialChunkCount=1;
    peRquestDetails[reqpos].mutualEncoding= -1;
    peRquestDetails[reqpos]._pvoid = nullptr;
    peRquestDetails[reqpos]._headerProp = 0;
    peRquestDetails[reqpos]._headerPos = 0;
    peRquestDetails[reqpos]._headerState = PE_HEAD_KEY + PE_HEAD_STATE_IN_OBJECT;
}



#endif
