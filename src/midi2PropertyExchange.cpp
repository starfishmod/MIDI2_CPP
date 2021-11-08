#include "../include/midi2PropertyExchange.h"
#include "../include/midi2Processor.h"

#ifndef M2_DISABLE_PE

#include <cstring>



void midi2Processor::processPESysex(uint8_t groupOffset, uint8_t s7Byte){
	uint8_t reqPosUsed;
	
	if(ciType[groupOffset] == MIDICI_PE_CAPABILITY && sysexPos[groupOffset] == 13){
		uint8_t sysex[14];
		addCIHeader(MIDICI_PE_CAPABILITYREPLY, sysex, 0x01);
		setBytesFromNumbers(sysex, remoteMUID[groupOffset], 9, 4);
		//Simultaneous Requests Supports
		sysex[13]=numRequests;
		if(sendOutSysex != nullptr) sendOutSysex(groupOffset + groupStart,sysex,14,0);
		
		if(recvPECapabilities != nullptr)recvPECapabilities(groupOffset + groupStart,remoteMUID[groupOffset], s7Byte);
			
	} else
	if(ciType[groupOffset] == MIDICI_PE_CAPABILITYREPLY && sysexPos[groupOffset] == 13 && recvPECapabilities != nullptr){
		recvPECapabilities(groupOffset + groupStart,remoteMUID[groupOffset], s7Byte);
		
	} else {
			if(sysexPos[groupOffset] == 13){
				//debug("new PE req");
			}
		
			if(sysexPos[groupOffset] >= 13){
				reqPosUsed=getPERequestId(groupOffset, s7Byte);	
				if(reqPosUsed == 255){
					//Ignore this Get Message
					return;
				}
			}
			
			if(sysexPos[groupOffset] == 14){ //header Length
				sys7IntBuffer[groupOffset][1] = (int)s7Byte;
			}
			if(sysexPos[groupOffset] == 15){
				sys7IntBuffer[groupOffset][1] += (int)s7Byte << 7;
				
				sys7IntBuffer[groupOffset][2] = PE_HEAD_KEY + PE_HEAD_STATE_IN_OBJECT;
				sys7IntBuffer[groupOffset][3] = 0; //bufferPos

                peRquestDetails[reqPosUsed].partialChunkCount = 1;
			}
			
		uint16_t headerLength = sys7IntBuffer[groupOffset][1];
		
		if(sysexPos[groupOffset] >= 16 && sysexPos[groupOffset] <= 15 + headerLength){
			if(ciType[groupOffset] == MIDICI_PE_GET 
			 || ciType[groupOffset] == MIDICI_PE_SUB 
			 || ciType[groupOffset] == MIDICI_PE_SET){ 
				processPEHeader(groupOffset, reqPosUsed, s7Byte);
			}else{
				// processPEReplytHeader(groupOffset, reqPosUsed, s7Byte);
			}
		}
			
		if(sysexPos[groupOffset] == 15 + headerLength
			&& (
				ciType[groupOffset] == MIDICI_PE_GET 
				|| ciType[groupOffset] == MIDICI_PE_SETREPLY
				|| ciType[groupOffset] == MIDICI_PE_SUBREPLY
				)
		){
			if(ciType[groupOffset] == MIDICI_PE_GET && recvPEGetInquiry != nullptr){
				recvPEGetInquiry(groupOffset + groupStart, remoteMUID[groupOffset], peRquestDetails[reqPosUsed]);
				//debug("Cleanup 1");
			}
            if(ciType[groupOffset] == MIDICI_PE_SETREPLY && recvPESetReply != nullptr){
                recvPESetReply(groupOffset + groupStart, remoteMUID[groupOffset], peRquestDetails[reqPosUsed]);
            }
            if(ciType[groupOffset] == MIDICI_PE_SUBREPLY && recvPESubReply != nullptr){
                recvPESubReply(groupOffset + groupStart, remoteMUID[groupOffset], peRquestDetails[reqPosUsed]);
            }
            cleanupRequestId(peRquestDetails[reqPosUsed].requestId);
            return;
		}
			
		if(sysexPos[groupOffset] == 16 + headerLength){		
			peRquestDetails[reqPosUsed].totalChunks = (int)s7Byte;
		}
		if(sysexPos[groupOffset] == 17 + headerLength){
			peRquestDetails[reqPosUsed].totalChunks += (int)s7Byte << 7;
		}
		
		if(sysexPos[groupOffset] == 18 + headerLength){ 
			peRquestDetails[reqPosUsed].numChunks = (int)s7Byte;
		}
			
		if(sysexPos[groupOffset] == 19 + headerLength){
			peRquestDetails[reqPosUsed].numChunks += (int)s7Byte << 7;
		}
		
		if(sysexPos[groupOffset] == 20 + headerLength){ //Body Length
			sys7IntBuffer[groupOffset][4] = (int)s7Byte;
		}
		if(sysexPos[groupOffset] == 21 + headerLength){
			sys7IntBuffer[groupOffset][4] += (int)s7Byte << 7;
		}
			
		uint16_t bodyLength = sys7IntBuffer[groupOffset][4];
		uint16_t initPos = 22 + headerLength;
		uint8_t charOffset = (sysexPos[groupOffset] - initPos) % PE_HEAD_BUFFERLEN;		
			
		if(
			(sysexPos[groupOffset] >= initPos && sysexPos[groupOffset] <= initPos - 1 + bodyLength)
			|| 	(bodyLength == 0 && sysexPos[groupOffset] == initPos -1)
		){
			
			//Todo - this is broken!
			
			//char messStr[10];
			//sprintf(messStr,"%d - %d - offset %d ",sysexPos[groupOffset], s7Byte, charOffset);
			//debug(messStr);
			
			
			if(bodyLength != 0 )sys7CharBuffer[groupOffset][charOffset] = s7Byte;

            bool lastByteOfSet = (peRquestDetails[reqPosUsed].numChunks == peRquestDetails[reqPosUsed].totalChunks && sysexPos[groupOffset] == initPos - 1 + bodyLength);
			
			if(charOffset == PE_HEAD_BUFFERLEN -1 
				|| sysexPos[groupOffset] == initPos - 1 + bodyLength
				|| bodyLength == 0 
			){
				//debug("OK run");
				
				if(ciType[groupOffset] == MIDICI_PE_SUB && recvPESubInquiry != nullptr){
					recvPESubInquiry(groupOffset + groupStart, remoteMUID[groupOffset], peRquestDetails[reqPosUsed], charOffset+1, sys7CharBuffer[groupOffset], lastByteOfSet);
				}
				
				if(ciType[groupOffset] == MIDICI_PE_SET && recvPESetInquiry != nullptr){
					recvPESetInquiry(groupOffset + groupStart, remoteMUID[groupOffset], peRquestDetails[reqPosUsed], charOffset+1, sys7CharBuffer[groupOffset], lastByteOfSet);
				}

                peRquestDetails[reqPosUsed].partialChunkCount++;
			} 
			
			if(lastByteOfSet){
				cleanupRequestId(peRquestDetails[reqPosUsed].requestId); 
			}
		}
			
	}
	
}


uint8_t midi2Processor::getPERequestId(uint8_t groupOffset, uint8_t s7Byte){
	uint8_t reqPosUsed = 255;
	if(sysexPos[groupOffset] == 13){ //Request Id
		for(uint8_t i =0;i<numRequests;i++){
			if(peRquestDetails[i].requestId == s7Byte){
				reqPosUsed = i;
				break;
			}else if(reqPosUsed == 255 && peRquestDetails[i].requestId == 255){
				peRquestDetails[i].requestId = s7Byte;
				reqPosUsed = i;
				break;
			}
		}
		sys7IntBuffer[groupOffset][0] = (int)reqPosUsed;
		if(reqPosUsed == 255){
			//Serial.println("  - Could not set ReqId");
			//return NAK
		}
	}else {
		reqPosUsed = sys7IntBuffer[groupOffset][0];
	}
	return reqPosUsed;
}

void midi2Processor::processPEHeader(uint8_t groupOffset, uint8_t reqPosUsed, uint8_t s7Byte){
	int clear=0;

	if((sys7IntBuffer[groupOffset][2] & 0xF) == PE_HEAD_STATE_IN_STRING){
		if (s7Byte == '"' && sys7CharBuffer[groupOffset][sys7IntBuffer[groupOffset][3]-1]!='\\') {
			if((sys7IntBuffer[groupOffset][2] & 0xF0) == PE_HEAD_KEY){
				_pvoid = nullptr;
				headerProp = 0;
				if(!strcmp((const char*)sys7CharBuffer[groupOffset],"resource")){
					_pvoid = &peRquestDetails[reqPosUsed].resource;
				}
                if(!strcmp((const char*)sys7CharBuffer[groupOffset],"path")){
                    _pvoid = &peRquestDetails[reqPosUsed].path;
                    headerProp = 1;
                }
                if(!strcmp((const char*)sys7CharBuffer[groupOffset],"mediaType")){
                    _pvoid = &peRquestDetails[reqPosUsed].mediaType;
                    //headerProp = 1;
                }
				if(!strcmp((const char*)sys7CharBuffer[groupOffset],"command")){
					//_pvoid = &peRquestDetails[reqPosUsed].command;
					headerProp = 1;
				}
                if(!strcmp((const char*)sys7CharBuffer[groupOffset],"action")){
					//_pvoid = &peRquestDetails[reqPosUsed].command;
					headerProp = 2;
				}
				if(!strcmp((const char*)sys7CharBuffer[groupOffset],"resId")){
					_pvoid = &peRquestDetails[reqPosUsed].resId;
				}
				if(!strcmp((const char*)sys7CharBuffer[groupOffset],"subscribeId")){
					_pvoid = &peRquestDetails[reqPosUsed].subscribeId;
				}
                if(!strcmp((const char*)sys7CharBuffer[groupOffset],"status")){
                    _pvoid = &peRquestDetails[reqPosUsed].status;
                }
				if(!strcmp((const char*)sys7CharBuffer[groupOffset],"offset")){
					_pvoid = &peRquestDetails[reqPosUsed].offset;
				}
				if(!strcmp((const char*)sys7CharBuffer[groupOffset],"limit")){
					_pvoid = &peRquestDetails[reqPosUsed].limit;
				}
                if(!strcmp((const char*)sys7CharBuffer[groupOffset],"setPartial")){
					_pvoid = &peRquestDetails[reqPosUsed].partial;
				}
				if(!strcmp((const char*)sys7CharBuffer[groupOffset],"mutualEncoding")){
                    headerProp = 3;
				}
			}
            else if(_pvoid != nullptr){
                if(headerProp == 0){
                    memcpy(_pvoid, sys7CharBuffer[groupOffset], PE_HEAD_BUFFERLEN * sizeof(char));
                }

                /*char *t = (char *)_pvoid;
				for (int i = 0; i < PE_HEAD_BUFFERLEN; i++){
					*t++=sys7CharBuffer[groupOffset][i];
				}*/
                headerProp=0;
				_pvoid = nullptr;
			}
            else if(headerProp == 1){
				if(!strcmp((const char*)sys7CharBuffer[groupOffset],"start")){
					peRquestDetails[reqPosUsed].command = MIDICI_PE_COMMAND_START;
				}
				if(!strcmp((const char*)sys7CharBuffer[groupOffset],"end")){
					peRquestDetails[reqPosUsed].command = MIDICI_PE_COMMAND_END;
				}
				if(!strcmp((const char*)sys7CharBuffer[groupOffset],"partial")){
					peRquestDetails[reqPosUsed].command = MIDICI_PE_COMMAND_PARTIAL;
				}
				if(!strcmp((const char*)sys7CharBuffer[groupOffset],"full")){
					peRquestDetails[reqPosUsed].command = MIDICI_PE_COMMAND_FULL;
				}
				if(!strcmp((const char*)sys7CharBuffer[groupOffset],"notify")){
					peRquestDetails[reqPosUsed].command = MIDICI_PE_COMMAND_NOTIFY;
				}
			}
            else if(headerProp == 2){
				if(!strcmp((const char*)sys7CharBuffer[groupOffset],"copy")){
					peRquestDetails[reqPosUsed].action = EXP_MIDICI_PE_ACTION_COPY;
				}
				if(!strcmp((const char*)sys7CharBuffer[groupOffset],"move")){
					peRquestDetails[reqPosUsed].action = EXP_MIDICI_PE_ACTION_MOVE;
				}
				if(!strcmp((const char*)sys7CharBuffer[groupOffset],"delete")){
					peRquestDetails[reqPosUsed].action = EXP_MIDICI_PE_ACTION_DELETE;
				}
				if(!strcmp((const char*)sys7CharBuffer[groupOffset],"createDirectory")){
					peRquestDetails[reqPosUsed].action = EXP_MIDICI_PE_ACTION_CREATE_DIR;
				}
			}
            else if(headerProp == 3){
                if(!strcmp((const char*)sys7CharBuffer[groupOffset],"ASCII")){
                    peRquestDetails[reqPosUsed].mutualEncoding = MIDICI_PE_ASCII;
                }
                if(!strcmp((const char*)sys7CharBuffer[groupOffset],"Mcoded7")){
                    peRquestDetails[reqPosUsed].mutualEncoding = MIDICI_PE_MCODED7;
                }
                if(!strcmp((const char*)sys7CharBuffer[groupOffset],"zlib+Mcoded7")){
                    peRquestDetails[reqPosUsed].mutualEncoding = MIDICI_PE_MCODED7ZLIB;
                }
            }
			clear=1;
        }
        else if(headerProp >= 1 && _pvoid != nullptr){
            char *t = (char *)_pvoid;
            t[headerProp-1] = s7Byte;
            t[headerProp]='\0';
            headerProp++;
        }
        else if(sys7IntBuffer[groupOffset][3] +1 < PE_HEAD_BUFFERLEN){
			sys7CharBuffer[groupOffset][sys7IntBuffer[groupOffset][3]++] = s7Byte;
        }
	} else if((sys7IntBuffer[groupOffset][2] & 0xF) == PE_HEAD_STATE_IN_NUMBER) {;
		if ((s7Byte >= '0' && s7Byte <= '9') ) {
			int *n = (int *)_pvoid;
			*n =  *n * 10 + (s7Byte - '0');
		}else if(_pvoid != nullptr){
			_pvoid = nullptr;
			clear=1;
			sys7IntBuffer[groupOffset][2]=PE_HEAD_KEY + (sys7IntBuffer[groupOffset][2] & 0xF);
		}
	} else if (s7Byte == ':') {
		sys7IntBuffer[groupOffset][2]=PE_HEAD_VALUE + (sys7IntBuffer[groupOffset][2] & 0xF);
	}else if (s7Byte == ',') {
		sys7IntBuffer[groupOffset][2] = PE_HEAD_KEY + (sys7IntBuffer[groupOffset][2] & 0xF);
	}else if ((s7Byte >= '0' && s7Byte <= '9') ) {
		int *n = (int *)_pvoid;
		*n =  s7Byte - '0';
		sys7IntBuffer[groupOffset][2] = (sys7IntBuffer[groupOffset][2] & 0xF0) + PE_HEAD_STATE_IN_NUMBER;
	}else if (s7Byte == 't' || s7Byte == 'f') {
        bool *b = (bool *)_pvoid;
        *b = s7Byte == 't';
        sys7IntBuffer[groupOffset][2] = (sys7IntBuffer[groupOffset][2] & 0xF0) + PE_HEAD_STATE_IN_BOOL;
    }else if (s7Byte == '"') {
		sys7IntBuffer[groupOffset][2] = (sys7IntBuffer[groupOffset][2] & 0xF0) + PE_HEAD_STATE_IN_STRING;
	}
	
	if(clear){
		memset(sys7CharBuffer[groupOffset], 0, PE_HEAD_BUFFERLEN);
		sys7IntBuffer[groupOffset][3]=0;
		sys7IntBuffer[groupOffset][2] = (sys7IntBuffer[groupOffset][2] & 0xF0) + PE_HEAD_STATE_IN_OBJECT;
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
			peRquestDetails[i].requestId = 255;
			memset(peRquestDetails[i].resource,0,PE_HEAD_BUFFERLEN);
			memset(peRquestDetails[i].resId,0,PE_HEAD_BUFFERLEN);
			memset(peRquestDetails[i].subscribeId,0,PE_HEAD_BUFFERLEN);
			peRquestDetails[i].offset=-1;
			peRquestDetails[i].limit=-1;
			peRquestDetails[i].status=-1;
			peRquestDetails[i].command=0;
			return;
		}
	}
}



#endif
