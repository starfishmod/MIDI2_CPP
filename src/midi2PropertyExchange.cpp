#include "../include/midi2PropertyExchange.h"
#include "../include/midi2Processor.h"

#ifndef M2_DISABLE_PE

#include <cstring>

void midi2Processor::processPESysex(uint8_t group, uint8_t s7Byte){
	uint8_t reqPosUsed;
	
	if(syExMess[group].ciType == MIDICI_PE_CAPABILITY && syExMess[group]._pos == 13){
		uint8_t sysex[14];
		addCIHeader(MIDICI_PE_CAPABILITYREPLY, sysex, 0x01);
		setBytesFromNumbers(sysex, syExMess[group].remoteMUID, 9, 4);
		//Simultaneous Requests Supports
		sysex[13]=numRequests;
		if(sendOutSysex != nullptr) sendOutSysex(group,sysex,14,0);
		
		if(recvPECapabilities != nullptr)recvPECapabilities(group,syExMess[group].remoteMUID, s7Byte);
			
	} else
	if(syExMess[group].ciType == MIDICI_PE_CAPABILITYREPLY && syExMess[group]._pos == 13 && recvPECapabilities != nullptr){
		recvPECapabilities(group,syExMess[group].remoteMUID, s7Byte);
		
	} else {
        if(syExMess[group]._pos == 13){
            syExMess[group].reqPosUsed = getPERequestId(group, s7Byte);
            return;
        }

        if(syExMess[group]._pos > 13){
            reqPosUsed=syExMess[group].reqPosUsed;
            if(reqPosUsed == 255){
                //Ignore this Get Message
                return;
            }
        }

        if(syExMess[group]._pos == 14 || syExMess[group]._pos == 15){ //header Length
            syExMess[group].intbuffer1[0] += s7Byte << (7 * (syExMess[group]._pos - 14 ));
            return;
        }

		uint16_t headerLength = syExMess[group].intbuffer1[0];
		
		if(syExMess[group]._pos >= 16 && syExMess[group]._pos <= 15 + headerLength){
			processPEHeader(group, reqPosUsed, s7Byte);

            if(syExMess[group]._pos == 15 + headerLength
                && (
                    syExMess[group].ciType == MIDICI_PE_GET
                    || syExMess[group].ciType == MIDICI_PE_SETREPLY
                    || syExMess[group].ciType == MIDICI_PE_SUBREPLY
                    )
            ){
                if(syExMess[group].ciType == MIDICI_PE_GET && recvPEGetInquiry != nullptr){
                    recvPEGetInquiry(group, syExMess[group].remoteMUID, peRquestDetails[reqPosUsed]);
                }
                if(syExMess[group].ciType == MIDICI_PE_SETREPLY && recvPESetReply != nullptr){
                    recvPESetReply(group, syExMess[group].remoteMUID, peRquestDetails[reqPosUsed]);
                }
                if(syExMess[group].ciType == MIDICI_PE_SUBREPLY && recvPESubReply != nullptr){
                    recvPESubReply(group, syExMess[group].remoteMUID, peRquestDetails[reqPosUsed]);
                }
                cleanupRequestId(peRquestDetails[reqPosUsed].requestId);
            }else if(syExMess[group]._pos == 15 + headerLength){
                peRquestDetails[reqPosUsed].totalChunks = 0;
                peRquestDetails[reqPosUsed].numChunks = 0;
            }
            return;
        }

		if(syExMess[group]._pos == 16 + headerLength || syExMess[group]._pos == 17 + headerLength){
			peRquestDetails[reqPosUsed].totalChunks += s7Byte << (7 * (syExMess[group]._pos - 16 - headerLength ));
            return;
        }

		if(syExMess[group]._pos == 18 + headerLength  || syExMess[group]._pos == 19 + headerLength){
			peRquestDetails[reqPosUsed].numChunks += s7Byte << (7 * (syExMess[group]._pos - 18 - headerLength ));
            return;
		}

		
		if(syExMess[group]._pos == 20 + headerLength || syExMess[group]._pos == 21 + headerLength){ //Body Length
			syExMess[group].intbuffer1[1] += s7Byte << (7 * (syExMess[group]._pos - 20 - headerLength ));
		}

		uint16_t bodyLength = syExMess[group].intbuffer1[1];
        uint16_t initPos = 22 + headerLength;
        uint8_t charOffset = (syExMess[group]._pos - initPos) % PE_HEAD_BUFFERLEN;

		if(
			(syExMess[group]._pos >= initPos && syExMess[group]._pos <= initPos - 1 + bodyLength)
			|| 	(bodyLength == 0 && syExMess[group]._pos == initPos -1)
		){
			if(bodyLength != 0 )syExMess[group].buffer1[charOffset] = s7Byte;

            bool lastByteOfSet = (peRquestDetails[reqPosUsed].numChunks == peRquestDetails[reqPosUsed].totalChunks && syExMess[group]._pos == initPos - 1 + bodyLength);
			
			if(charOffset == PE_HEAD_BUFFERLEN -1 
				|| syExMess[group]._pos == initPos - 1 + bodyLength
				|| bodyLength == 0 
			){
				if(syExMess[group].ciType == MIDICI_PE_SUB && recvPESubInquiry != nullptr){
					recvPESubInquiry(group, syExMess[group].remoteMUID, peRquestDetails[reqPosUsed], charOffset+1, syExMess[group].buffer1, lastByteOfSet);
				}
				
				if(syExMess[group].ciType == MIDICI_PE_SET && recvPESetInquiry != nullptr){
					recvPESetInquiry(group, syExMess[group].remoteMUID, peRquestDetails[reqPosUsed], charOffset+1, syExMess[group].buffer1, lastByteOfSet);
				}

                peRquestDetails[reqPosUsed].partialChunkCount++;
			} 
			
			if(lastByteOfSet){
				cleanupRequestId(peRquestDetails[reqPosUsed].requestId); 
			}
		}
			
	}
	
}

uint8_t midi2Processor::getPERequestId(uint8_t group, uint8_t s7Byte){
    uint8_t reqPosUsed = 255;

    for(uint8_t i =0;i<numRequests;i++){
        if(peRquestDetails[i].requestId == s7Byte){
            return i;

        }else if(peRquestDetails[i].requestId == 255){
            peRquestDetails[i].requestId = s7Byte;
            return i;
        }
    }
    return reqPosUsed;
}

void midi2Processor::processPEHeader(uint8_t group, uint8_t reqPosUsed, uint8_t s7Byte){

    /*char messStr[20];
    sprintf(messStr," p: %d %c",peRquestDetails[reqPosUsed]._headerPos, s7Byte);
    debug(messStr);*/

	int clear=0;

	if((peRquestDetails[reqPosUsed]._headerState & 0xF) == PE_HEAD_STATE_IN_STRING){
		if (s7Byte == '"' && syExMess[group].buffer1[peRquestDetails[reqPosUsed]._headerPos-1]!='\\') {
			if((peRquestDetails[reqPosUsed]._headerState & 0xF0) == PE_HEAD_KEY){
                peRquestDetails[reqPosUsed]._pvoid = nullptr;
                peRquestDetails[reqPosUsed]._headerProp = 0;
				if(!strcmp((const char*)syExMess[group].buffer1,"resource")){
                    peRquestDetails[reqPosUsed]._pvoid = &peRquestDetails[reqPosUsed].resource;
                    peRquestDetails[reqPosUsed]._headerProp = 1;
				}
                if(!strcmp((const char*)syExMess[group].buffer1,"path")){
                    peRquestDetails[reqPosUsed]._pvoid = &peRquestDetails[reqPosUsed].path;
                    peRquestDetails[reqPosUsed]._headerProp = 1;
                }
                if(!strcmp((const char*)syExMess[group].buffer1,"mediaType")){
                    peRquestDetails[reqPosUsed]._pvoid = &peRquestDetails[reqPosUsed].mediaType;
                    peRquestDetails[reqPosUsed]._headerProp = 1;
                }
				if(!strcmp((const char*)syExMess[group].buffer1,"command")){
					//_pvoid = &peRquestDetails[reqPosUsed].command;
                    peRquestDetails[reqPosUsed]._headerProp = 1;
				}
                if(!strcmp((const char*)syExMess[group].buffer1,"action")){
					//_pvoid = &peRquestDetails[reqPosUsed].command;
                    peRquestDetails[reqPosUsed]._headerProp = 2;
				}
				if(!strcmp((const char*)syExMess[group].buffer1,"resId")){
                   // debug("resId!");
                    peRquestDetails[reqPosUsed]._pvoid = &peRquestDetails[reqPosUsed].resId;
                    peRquestDetails[reqPosUsed]._headerProp = 1;
				}
				if(!strcmp((const char*)syExMess[group].buffer1,"subscribeId")){
                    peRquestDetails[reqPosUsed]._pvoid = &peRquestDetails[reqPosUsed].subscribeId;
                    peRquestDetails[reqPosUsed]._headerProp = 1;
				}
                if(!strcmp((const char*)syExMess[group].buffer1,"status")){
                    peRquestDetails[reqPosUsed]._pvoid = &peRquestDetails[reqPosUsed].status;
                }
				if(!strcmp((const char*)syExMess[group].buffer1,"offset")){
                    peRquestDetails[reqPosUsed]._pvoid = &peRquestDetails[reqPosUsed].offset;
				}
				if(!strcmp((const char*)syExMess[group].buffer1,"limit")){
                    peRquestDetails[reqPosUsed]._pvoid = &peRquestDetails[reqPosUsed].limit;
				}
                if(!strcmp((const char*)syExMess[group].buffer1,"setPartial")){
                    peRquestDetails[reqPosUsed]._pvoid = &peRquestDetails[reqPosUsed].partial;
				}
				if(!strcmp((const char*)syExMess[group].buffer1,"mutualEncoding")){
                    peRquestDetails[reqPosUsed]._headerProp = 3;
				}
			}
            else if(peRquestDetails[reqPosUsed]._pvoid != nullptr){
                if(peRquestDetails[reqPosUsed]._headerProp == 0){
                    memcpy(peRquestDetails[reqPosUsed]._pvoid, syExMess[group].buffer1, PE_HEAD_BUFFERLEN * sizeof(char));
                }

                peRquestDetails[reqPosUsed]._headerProp=0;
                peRquestDetails[reqPosUsed]._pvoid = nullptr;
			}
            else if(peRquestDetails[reqPosUsed]._headerProp == 1){
				if(!strcmp((const char*)syExMess[group].buffer1,"start")){
					peRquestDetails[reqPosUsed].command = MIDICI_PE_COMMAND_START;
				}
				if(!strcmp((const char*)syExMess[group].buffer1,"end")){
					peRquestDetails[reqPosUsed].command = MIDICI_PE_COMMAND_END;
				}
				if(!strcmp((const char*)syExMess[group].buffer1,"partial")){
					peRquestDetails[reqPosUsed].command = MIDICI_PE_COMMAND_PARTIAL;
				}
				if(!strcmp((const char*)syExMess[group].buffer1,"full")){
					peRquestDetails[reqPosUsed].command = MIDICI_PE_COMMAND_FULL;
				}
				if(!strcmp((const char*)syExMess[group].buffer1,"notify")){
					peRquestDetails[reqPosUsed].command = MIDICI_PE_COMMAND_NOTIFY;
				};
                peRquestDetails[reqPosUsed]._headerProp=0;
                peRquestDetails[reqPosUsed]._pvoid = nullptr;
			}
            else if(peRquestDetails[reqPosUsed]._headerProp == 2){
				if(!strcmp((const char*)syExMess[group].buffer1,"copy")){
					peRquestDetails[reqPosUsed].action = EXP_MIDICI_PE_ACTION_COPY;
				}
				if(!strcmp((const char*)syExMess[group].buffer1,"move")){
					peRquestDetails[reqPosUsed].action = EXP_MIDICI_PE_ACTION_MOVE;
				}
				if(!strcmp((const char*)syExMess[group].buffer1,"delete")){
					peRquestDetails[reqPosUsed].action = EXP_MIDICI_PE_ACTION_DELETE;
				}
				if(!strcmp((const char*)syExMess[group].buffer1,"createDirectory")){
					peRquestDetails[reqPosUsed].action = EXP_MIDICI_PE_ACTION_CREATE_DIR;
				}
                peRquestDetails[reqPosUsed]._headerProp=0;
                peRquestDetails[reqPosUsed]._pvoid = nullptr;
			}
            else if(peRquestDetails[reqPosUsed]._headerProp == 3){
                if(!strcmp((const char*)syExMess[group].buffer1,"ASCII")){
                    peRquestDetails[reqPosUsed].mutualEncoding = MIDICI_PE_ASCII;
                }
                if(!strcmp((const char*)syExMess[group].buffer1,"Mcoded7")){
                    peRquestDetails[reqPosUsed].mutualEncoding = MIDICI_PE_MCODED7;
                }
                if(!strcmp((const char*)syExMess[group].buffer1,"zlib+Mcoded7")){
                    peRquestDetails[reqPosUsed].mutualEncoding = MIDICI_PE_MCODED7ZLIB;
                }
                peRquestDetails[reqPosUsed]._headerProp=0;
                peRquestDetails[reqPosUsed]._pvoid = nullptr;
            }
			clear=1;
        }
        else if(peRquestDetails[reqPosUsed]._headerProp >= 1 && peRquestDetails[reqPosUsed]._pvoid != nullptr){

            char *t = (char *)peRquestDetails[reqPosUsed]._pvoid;
            t[peRquestDetails[reqPosUsed]._headerProp-1] = s7Byte;
            t[peRquestDetails[reqPosUsed]._headerProp]='\0';
            peRquestDetails[reqPosUsed]._headerProp++;
        }
        else if(peRquestDetails[reqPosUsed]._headerPos +1 < PE_HEAD_BUFFERLEN){
			syExMess[group].buffer1[peRquestDetails[reqPosUsed]._headerPos++] = s7Byte;
        }
	} else if((peRquestDetails[reqPosUsed]._headerState & 0xF) == PE_HEAD_STATE_IN_NUMBER) {;
		if ((s7Byte >= '0' && s7Byte <= '9') ) {
			int *n = (int *)peRquestDetails[reqPosUsed]._pvoid;
			*n =  *n * 10 + (s7Byte - '0');
		}else if(peRquestDetails[reqPosUsed]._pvoid != nullptr){
            peRquestDetails[reqPosUsed]._pvoid = nullptr;
			clear=1;
			peRquestDetails[reqPosUsed]._headerState=PE_HEAD_KEY + (peRquestDetails[reqPosUsed]._headerState & 0xF);
		}
	} else if (s7Byte == ':') {
		peRquestDetails[reqPosUsed]._headerState=PE_HEAD_VALUE + (peRquestDetails[reqPosUsed]._headerState & 0xF);
	}else if (s7Byte == ',') {
		peRquestDetails[reqPosUsed]._headerState = PE_HEAD_KEY + (peRquestDetails[reqPosUsed]._headerState & 0xF);
	}else if ((s7Byte >= '0' && s7Byte <= '9') ) {
		int *n = (int *)peRquestDetails[reqPosUsed]._pvoid;
		*n =  s7Byte - '0';
		peRquestDetails[reqPosUsed]._headerState = (peRquestDetails[reqPosUsed]._headerState & 0xF0) + PE_HEAD_STATE_IN_NUMBER;
	}else if (s7Byte == 't' || s7Byte == 'f') {
        bool *b = (bool *)peRquestDetails[reqPosUsed]._pvoid;
        *b = s7Byte == 't';
        peRquestDetails[reqPosUsed]._headerState = (peRquestDetails[reqPosUsed]._headerState & 0xF0) + PE_HEAD_STATE_IN_BOOL;
    }else if (s7Byte == '"') {
		peRquestDetails[reqPosUsed]._headerState = (peRquestDetails[reqPosUsed]._headerState & 0xF0) + PE_HEAD_STATE_IN_STRING;
	}
	
	if(clear){
		memset(syExMess[group].buffer1, 0, PE_HEAD_BUFFERLEN);
		peRquestDetails[reqPosUsed]._headerPos=0;
		peRquestDetails[reqPosUsed]._headerState = (peRquestDetails[reqPosUsed]._headerState & 0xF0) + PE_HEAD_STATE_IN_OBJECT;
	}
	
}

void midi2Processor::sendPECapabilityRequest(uint8_t group, uint32_t remoteMuid, uint8_t ciVersion){
	if(sendOutSysex == nullptr) return;
	uint8_t sysex[14];
	addCIHeader(MIDICI_PE_CAPABILITY,sysex,ciVersion);
	setBytesFromNumbers(sysex, remoteMuid, 9, 4);
	sysex[13] = numRequests;
	sendOutSysex(group,sysex,14,0);
}

void midi2Processor::sendPEGet(uint8_t group, uint32_t remoteMuid, uint8_t ciVersion, uint8_t requestId, uint16_t headerLen, uint8_t* header){
	if(sendOutSysex == nullptr) return;
	uint8_t sysex[13];
	addCIHeader(MIDICI_PE_GET,sysex,ciVersion);
	setBytesFromNumbers(sysex, remoteMuid, 9, 4);
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

void midi2Processor::sendPESet(uint8_t group, uint32_t remoteMuid, uint8_t ciVersion, uint8_t requestId, uint16_t headerLen, uint8_t* header, uint16_t numberOfChunks, uint16_t numberOfThisChunk, uint16_t bodyLength , uint8_t* body ){
    if(sendOutSysex == nullptr) return;
    uint8_t sysex[13];
    addCIHeader(MIDICI_PE_SET,sysex,ciVersion);
    setBytesFromNumbers(sysex, remoteMuid, 9, 4);
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

void midi2Processor::sendPEGetReply(uint8_t group, uint32_t remoteMuid, uint8_t ciVersion, uint8_t requestId, uint16_t headerLen, uint8_t* header, uint16_t numberOfChunks, uint16_t numberOfThisChunk, uint16_t bodyLength , uint8_t* body ){
	if(sendOutSysex == nullptr) return;
	uint8_t sysex[13];
	addCIHeader(MIDICI_PE_GETREPLY,sysex,ciVersion);
	setBytesFromNumbers(sysex, remoteMuid, 9, 4);
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

void midi2Processor::sendPEGetReplyStreamStart(uint8_t group, uint32_t remoteMuid, uint8_t ciVersion, uint8_t requestId, uint16_t headerLen, uint8_t* header, uint16_t numberOfChunks, uint16_t numberOfThisChunk, uint16_t bodyLength){
	if(sendOutSysex == nullptr) return;
	uint8_t sysex[13];
	addCIHeader(MIDICI_PE_GETREPLY,sysex,ciVersion);
	setBytesFromNumbers(sysex, remoteMuid, 9, 4);
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

void midi2Processor::sendPESub(uint8_t group, uint32_t remoteMuid, uint8_t ciVersion, uint8_t requestId, uint16_t headerLen, uint8_t* header, uint16_t numberOfChunks, uint16_t numberOfThisChunk, uint16_t bodyLength , uint8_t* body ){
    if(sendOutSysex == nullptr) return;
    uint8_t sysex[13];
    addCIHeader(MIDICI_PE_SUB,sysex,ciVersion);
    setBytesFromNumbers(sysex, remoteMuid, 9, 4);
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

void midi2Processor::sendPESubReply(uint8_t group, uint32_t remoteMuid, uint8_t ciVersion, uint8_t requestId, uint16_t headerLen, uint8_t* header){
	if(sendOutSysex == nullptr) return;
	uint8_t sysex[13];
	addCIHeader(MIDICI_PE_SUBREPLY,sysex,ciVersion);
	setBytesFromNumbers(sysex, remoteMuid, 9, 4);
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

void midi2Processor::sendPESetReply(uint8_t group, uint32_t remoteMuid, uint8_t ciVersion, uint8_t requestId, uint16_t headerLen, uint8_t* header){
	if(sendOutSysex == nullptr) return;
	uint8_t sysex[13];
	addCIHeader(MIDICI_PE_SETREPLY,sysex,ciVersion);
	setBytesFromNumbers(sysex, remoteMuid, 9, 4);
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


void midi2Processor::cleanupRequestId(uint8_t requestId){
	for(uint8_t i =0;i<numRequests;i++){
		if(peRquestDetails[i].requestId == requestId){
            cleanupRequest(i);
			return;
		}
	}
}

void midi2Processor::cleanupRequest(uint8_t reqpos){
    peRquestDetails[reqpos].requestId = 255;
    peRquestDetails[reqpos].group = 255;
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
