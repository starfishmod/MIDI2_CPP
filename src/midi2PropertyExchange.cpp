#include "../include/midi2PropertyExchange.h"
#include "../include/midi2Processor.h"
#include "../include/utils.h"




#ifndef M2_DISABLE_PE



#include <string.h>

#ifdef ARDUINO
    #define SERIAL_PRINT  Serial.print
    #include <stdint.h>
#else
    #define SERIAL_PRINT  printf
    #include <stdio.h>
    #include <cstdint>
#endif


void midi2Processor::processPESysex(uint8_t groupOffset, uint8_t s7Byte){
	switch (ciType[groupOffset]){
		case 0x30: //Inquiry: Property Exchange Capabilities
			if(sysexPos[groupOffset] == 13){
				uint8_t sysex[14];
				addCIHeader(0x31,sysex,0x01);
				setBytesFromNumbers(sysex, remoteMUID[groupOffset], 9, 4);
				//Simultaneous Requests Supports
				sysex[13]=numRequests;
				if(sendOutSysex !=0) sendOutSysex(groupOffset + groupStart,sysex,14,0);
				
				if(recvPECapabilities != 0)recvPECapabilities(groupOffset + groupStart,remoteMUID[groupOffset], s7Byte);
			}
			
			break;
		case 0x31: //Reply to Property Exchange Capabilities
			if(sysexPos[groupOffset] == 13 && recvPECapabilities != 0){
				recvPECapabilities(groupOffset + groupStart,remoteMUID[groupOffset], s7Byte);
			}
			break;
		
		case 0x34: { // Inquiry: Get Property Data
			uint8_t reqPosUsed;
			if(sysexPos[groupOffset] >= 13){
					reqPosUsed=getPERequestId(groupOffset, s7Byte);
					
					//Serial.print(" - reqPosUsed");Serial.println(reqPosUsed);
			
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
			}
			
			
			if(sysexPos[groupOffset] >= 16 && sysexPos[groupOffset] <= 15 + sys7IntBuffer[groupOffset][1]){
				processPERequestHeader(groupOffset, reqPosUsed, s7Byte);
			}
			
			if(sysexPos[groupOffset] == 15 + sys7IntBuffer[groupOffset][1]){
				if(recvPEGetInquiry != 0) recvPEGetInquiry(groupOffset + groupStart, remoteMUID[groupOffset], peRquestDetails[reqPosUsed]);	
				cleanupRequestId(peRquestDetails[reqPosUsed].requestId); 
			}
			
			
			
			
		
			break;
			
		}
		case 0x35: // Reply To Get Property Data - Needs Work!
			/*uint8_t reqPosUsed;
			if(sysexPos[groupOffset] >= 13){
					reqPosUsed=getPERequestId(groupOffset, s7Byte); //Should this use the same pe Header structs?? / reqId's may mismatch here
			
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
			}
			
			
			if(sysexPos[groupOffset] >= 16 && sysexPos[groupOffset] <= 15 + sys7IntBuffer[groupOffset][1]){

				
				bool clear=false;

				if((sys7IntBuffer[groupOffset][2] & 0xF) == PE_HEAD_STATE_IN_STRING){
					//Serial.println(" - in PE_HEAD_STATE_IN_STRING 1");
					if (s7Byte == '"' && sys7CharBuffer[groupOffset][sys7IntBuffer[groupOffset][3]-1]!='\\') {
						//Serial.println("  - found end of string");
						if((sys7IntBuffer[groupOffset][2] & 0xF0) == PE_HEAD_KEY){
							if(!strcmp(sys7CharBuffer[groupOffset],"status")){
								//Serial.println("   - Set Resource");
								_pvoid = &peRquestDetails[reqPosUsed].status;
							}
							if(!strcmp(sys7CharBuffer[groupOffset],"totalCount")){
								//Serial.println("   - Set resId");
								_pvoid = &peRquestDetails[reqPosUsed].totalCount;
							}
							if(!strcmp(sys7CharBuffer[groupOffset],"cacheTime")){
								//Serial.println("   - Set resId");
								_pvoid = &peRquestDetails[reqPosUsed].cacheTime;
							}
							if(!strcmp(sys7CharBuffer[groupOffset],"mediaType")){
								//Serial.println("   - Set resId");
								_pvoid = &peRquestDetails[reqPosUsed].mediaType;
							}
							if(!strcmp(sys7CharBuffer[groupOffset],"mutualEncoding")){
								//Serial.println("   - Set resId");
								_pvoid = &peRquestDetails[reqPosUsed].mutualEncoding;
							}
						}else if(_pvoid!=0){
							char *t = (char *)_pvoid;
							for (int i = 0; i < PE_HEAD_BUFFERLEN; i++){
								*t++=sys7CharBuffer[groupOffset][i];
							}
							_pvoid = 0;
						}
						clear=true;
						}else if(sys7IntBuffer[groupOffset][3] + 1 < PE_HEAD_BUFFERLEN){
						sys7CharBuffer[groupOffset][sys7IntBuffer[groupOffset][3]++] = s7Byte;
						}
				} else if((sys7IntBuffer[groupOffset][2] & 0xF) == PE_HEAD_STATE_IN_NUMBER){
					//Serial.println(" - in PE_HEAD_STATE_IN_NUMBER");
					if ((s7Byte >= '0' && s7Byte <= '9') ) {
						int *n = (int *)_pvoid;
						*n =  *n * 10 + (s7Byte - '0');
					}else if(_pvoid!=0){
						_pvoid = 0;
						clear=true;
						sys7IntBuffer[groupOffset][2]=PE_HEAD_KEY + sys7IntBuffer[groupOffset][2] & 0xF;
					}
				} else if (s7Byte == ':') {
					//Serial.println(" - in PE_HEAD_VALUE");
					sys7IntBuffer[groupOffset][2]=PE_HEAD_VALUE + sys7IntBuffer[groupOffset][2] & 0xF;
				}else if (s7Byte == ',') {
					//Serial.println(" - in PE_HEAD_KEY");
					sys7IntBuffer[groupOffset][2] = PE_HEAD_KEY + sys7IntBuffer[groupOffset][2] & 0xF;
				}else if ((s7Byte >= '0' && s7Byte <= '9') ) {
					int *n = (int *)_pvoid;
					*n =  s7Byte - '0';
					sys7IntBuffer[groupOffset][2] = (sys7IntBuffer[groupOffset][2] & 0xF0) + PE_HEAD_STATE_IN_NUMBER;
					//Serial.println(" - set PE_HEAD_STATE_IN_NUMBER");
				} else if (s7Byte == '"') {
					//Serial.println(" - set PE_HEAD_STATE_IN_STRING 2");
					sys7IntBuffer[groupOffset][2] = (sys7IntBuffer[groupOffset][2] & 0xF0) + PE_HEAD_STATE_IN_STRING;
				}
				
				if(clear){
					//Serial.println(" - CLEAR");
					memset(sys7CharBuffer[groupOffset], 0, PE_HEAD_BUFFERLEN);
					
					sys7IntBuffer[groupOffset][3]=0;
					sys7IntBuffer[groupOffset][2] = (sys7IntBuffer[groupOffset][2] & 0xF0) + PE_HEAD_STATE_IN_OBJECT;
				}
				
			}
			
			
			if(sysexPos[groupOffset] == 16 + sys7IntBuffer[groupOffset][1]){ 
				requestDetails.totalChunks = (int)s7Byte;
			}
			if(sysexPos[groupOffset] == 17 + sys7IntBuffer[groupOffset][1]){
				requestDetails.totalChunks = += (int)s7Byte << 7;
			}
			
			if(sysexPos[groupOffset] == 18 + sys7IntBuffer[groupOffset][1]){ 
				requestDetails.numChunks = (int)s7Byte;
			}
			if(sysexPos[groupOffset] == 19 + sys7IntBuffer[groupOffset][1]){
				requestDetails.numChunks = += (int)s7Byte << 7;
			}
			
			if(sysexPos[groupOffset] == 20 + sys7IntBuffer[groupOffset][1]){ //Body Length
				sys7IntBuffer[groupOffset][4] = (int)s7Byte;
			}
			if(sysexPos[groupOffset] == 21 + sys7IntBuffer[groupOffset][1]){
				sys7IntBuffer[groupOffset][4] = += (int)s7Byte << 7;
			}
			
			
			
			if(sysexPos[groupOffset] == 15 + sys7IntBuffer[groupOffset][1]){
				//if(recvPEGetReply != 0) recvPEGetReply(groupOffset + groupStart, remoteMUID[groupOffset], peRquestDetails[reqPosUsed]);	
				//cleanupRequestId(requestDetails.requestId); 
			}*/
			
			
			
		
			break;
			
			
		case 0x36: {// Inquiry: Set Property Data
			uint8_t reqPosUsed;
			if(sysexPos[groupOffset] >= 13){
					reqPosUsed=getPERequestId(groupOffset, s7Byte);
					
					//Serial.print(" - reqPosUsed");Serial.println(reqPosUsed);
			
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
			}
			
			
			if(sysexPos[groupOffset] >= 16 && sysexPos[groupOffset] <= 15 + sys7IntBuffer[groupOffset][1]){
				processPERequestHeader(groupOffset, reqPosUsed, s7Byte);
			}
			
			
			if(sysexPos[groupOffset] == 16 + sys7IntBuffer[groupOffset][1]){ 
				peRquestDetails[reqPosUsed].totalChunks = (int)s7Byte;
			}
			if(sysexPos[groupOffset] == 17 + sys7IntBuffer[groupOffset][1]){
				peRquestDetails[reqPosUsed].totalChunks += (int)s7Byte << 7;
			}
			
			if(sysexPos[groupOffset] == 18 + sys7IntBuffer[groupOffset][1]){ 
				peRquestDetails[reqPosUsed].numChunks = (int)s7Byte;
			}
			if(sysexPos[groupOffset] == 19 + sys7IntBuffer[groupOffset][1]){
				peRquestDetails[reqPosUsed].numChunks += (int)s7Byte << 7;
			}
			
			if(sysexPos[groupOffset] == 20 + sys7IntBuffer[groupOffset][1]){ //Body Length
				sys7IntBuffer[groupOffset][4] = (int)s7Byte;
			}
			if(sysexPos[groupOffset] == 21 + sys7IntBuffer[groupOffset][1]){
				sys7IntBuffer[groupOffset][4] += (int)s7Byte << 7;
			}
			
			
			int initPos = 22 + sys7IntBuffer[groupOffset][1];
			if(sysexPos[groupOffset] >= initPos && sysexPos[groupOffset] <= initPos - 1 + sys7IntBuffer[groupOffset][4]){
				uint8_t charOffset = (initPos -  sysexPos[groupOffset]) % PE_HEAD_BUFFERLEN;
				sys7CharBuffer[groupOffset][charOffset] = s7Byte;
				
				if(charOffset == PE_HEAD_BUFFERLEN -1 
					|| sysexPos[groupOffset] == initPos - 1 + sys7IntBuffer[groupOffset][4]
				){
					if(recvPESetInquiry != 0) recvPESetInquiry(groupOffset + groupStart, remoteMUID[groupOffset], peRquestDetails[reqPosUsed], charOffset+1, sys7CharBuffer[groupOffset]);	
				} 
			}
			
			
			if(sysexPos[groupOffset] == initPos - 1 + sys7IntBuffer[groupOffset][4] && peRquestDetails[reqPosUsed].numChunks == peRquestDetails[reqPosUsed].totalChunks){	
				cleanupRequestId(peRquestDetails[reqPosUsed].requestId); 
			}
		
			break;
		}
	}
	
}


uint8_t midi2Processor::getPERequestId(uint8_t groupOffset, uint8_t s7Byte){
	uint8_t reqPosUsed = 255;
	if(sysexPos[groupOffset] == 13){ //Request Id
		//Serial.println("  Set ReqId ");
		for(uint8_t i =0;i<numRequests;i++){
			//Serial.print("  - i ");Serial.println( i);
			if(peRquestDetails[i].requestId == s7Byte){
				//Serial.print("  - Found old pos ");Serial.println( i);
				reqPosUsed = i;
				break;
			}else if(reqPosUsed==255 && peRquestDetails[i].requestId == 255){
				//Serial.print("  - Found unsed Pos ");Serial.println( i);
				peRquestDetails[i].requestId = s7Byte;
				reqPosUsed = i;
				break;
			}else {
				//Serial.print("  - exisiting ReqId ");Serial.println(peRquestDetails[i].requestId);
			}
		}
		sys7IntBuffer[groupOffset][0] = (int)reqPosUsed;
		if(reqPosUsed == 255){
			//Serial.println("  - Could not set ReqId");
			//return NAK
		}
	}else {
		reqPosUsed = sys7IntBuffer[groupOffset][0];
		//Serial.print("  - preset requid ");Serial.println( reqPosUsed);
	}
	return reqPosUsed;
}

void midi2Processor::processPERequestHeader(uint8_t groupOffset, uint8_t reqPosUsed, uint8_t s7Byte){
	int clear=0;

	if((sys7IntBuffer[groupOffset][2] & 0xF) == PE_HEAD_STATE_IN_STRING){
		if (s7Byte == '"' && sys7CharBuffer[groupOffset][sys7IntBuffer[groupOffset][3]-1]!='\\') {
			if((sys7IntBuffer[groupOffset][2] & 0xF0) == PE_HEAD_KEY){
				if(!strcmp((const char*)sys7CharBuffer[groupOffset],"resource")){
					_pvoid = &peRquestDetails[reqPosUsed].resource;
				}
				if(!strcmp((const char*)sys7CharBuffer[groupOffset],"resId")){
					_pvoid = &peRquestDetails[reqPosUsed].resId;
				}
				if(!strcmp((const char*)sys7CharBuffer[groupOffset],"offset")){
					_pvoid = &peRquestDetails[reqPosUsed].offset;
				}
				if(!strcmp((const char*)sys7CharBuffer[groupOffset],"limit")){
					_pvoid = &peRquestDetails[reqPosUsed].limit;
				}
				/*if(!strcmp(sys7CharBuffer[groupOffset],"mutualEncoding")){
					_pvoid = &peRquestDetails[reqPosUsed].mutualEncoding;
				}*/
			}else if(_pvoid!=0){
				char *t = (char *)_pvoid;
				for (int i = 0; i < PE_HEAD_BUFFERLEN; i++){
					*t++=sys7CharBuffer[groupOffset][i];
				}
				_pvoid = 0;

			}
			clear=1;
			}else if(sys7IntBuffer[groupOffset][3] +1 < PE_HEAD_BUFFERLEN){
			sys7CharBuffer[groupOffset][sys7IntBuffer[groupOffset][3]++] = s7Byte;
			}
	} else if((sys7IntBuffer[groupOffset][2] & 0xF) == PE_HEAD_STATE_IN_NUMBER){;
		if ((s7Byte >= '0' && s7Byte <= '9') ) {
			int *n = (int *)_pvoid;
			*n =  *n * 10 + (s7Byte - '0');
		}else if(_pvoid!=0){
			_pvoid = 0;
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
	} else if (s7Byte == '"') {
		sys7IntBuffer[groupOffset][2] = (sys7IntBuffer[groupOffset][2] & 0xF0) + PE_HEAD_STATE_IN_STRING;
	}
	
	if(clear){
		memset(sys7CharBuffer[groupOffset], 0, PE_HEAD_BUFFERLEN);
		sys7IntBuffer[groupOffset][3]=0;
		sys7IntBuffer[groupOffset][2] = (sys7IntBuffer[groupOffset][2] & 0xF0) + PE_HEAD_STATE_IN_OBJECT;
	}
	
}

void midi2Processor::sendPECapabilityRequest(uint8_t group, uint32_t remoteMuid, uint8_t ciVersion, uint8_t numSimulRequests){
	if(sendOutSysex ==0) return;
	uint8_t sysex[14];
	addCIHeader(0x30,sysex,ciVersion);
	setBytesFromNumbers(sysex, remoteMuid, 9, 4);
	sysex[13] = numRequests;
	sendOutSysex(group,sysex,14,0);
}

void midi2Processor::sendPEGet(uint8_t group, uint32_t remoteMuid, uint8_t ciVersion, uint8_t requestId, uint16_t headerLen, uint8_t* header){
	if(sendOutSysex ==0) return;
	uint8_t sysex[13];
	addCIHeader(0x35,sysex,ciVersion);
	setBytesFromNumbers(sysex, remoteMuid, 9, 4);
	sendOutSysex(group,sysex,13,1);
	
	sysex[0] = requestId;
	setBytesFromNumbers(sysex, headerLen, 1, 2);
	sendOutSysex(group,sysex,3,2);		
	sendOutSysex(group, header,headerLen,2);
	
	setBytesFromNumbers(sysex, 0, 0, 6);
	sendOutSysex(group,sysex,6,3);
}


void midi2Processor::sendPEGetReply(uint8_t group, uint32_t remoteMuid, uint8_t ciVersion, uint8_t requestId, uint16_t headerLen, uint8_t* header, int numberOfChunks, int numberOfThisChunk, uint16_t bodyLength , uint8_t* body ){
	if(sendOutSysex ==0) return;
	uint8_t sysex[13];
	addCIHeader(0x35,sysex,ciVersion);
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

void midi2Processor::cleanupRequestId(uint8_t requestId){
	for(uint8_t i =0;i<numRequests;i++){
		if(peRquestDetails[i].requestId == requestId){
			peRquestDetails[i].requestId = 255;
			memset(peRquestDetails[i].resource,0,PE_HEAD_BUFFERLEN);
			memset(peRquestDetails[i].resId,0,PE_HEAD_BUFFERLEN);
			peRquestDetails[i].offset=-1;
			peRquestDetails[i].limit=-1;
			peRquestDetails[i].status=-1;
			return;
		}
	}
}



#endif
