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
    
    
    uint8_t getPERequestId(uint8_t groupOffset, uint8_t newByte){
		uint8_t reqPosUsed = 255;
		if(sysexPos[groupOffset] == 13){ //Request Id
			Serial.println("  Set ReqId ");
			for(uint8_t i =0;i<numRequests;i++){
				Serial.print("  - i ");Serial.println( i);
				if(peRquestDetails[i].requestId == newByte){
					Serial.print("  - Found old pos ");Serial.println( i);
					reqPosUsed = i;
					break;
				}else if(reqPosUsed==255 && peRquestDetails[i].requestId == 255){
					Serial.print("  - Found unsed Pos ");Serial.println( i);
					peRquestDetails[i].requestId = newByte;
					reqPosUsed = i;
					break;
				}else {
					Serial.print("  - exisiting ReqId ");Serial.println(peRquestDetails[i].requestId);
				}
			}
			sys7IntBuffer[groupOffset][0] = (int)reqPosUsed;
			if(reqPosUsed == 255){
				Serial.println("  - Could not set ReqId");
				//return NAK
			}
		}else {
			reqPosUsed = sys7IntBuffer[groupOffset][0];
			//Serial.print("  - preset requid ");Serial.println( reqPosUsed);
		}
		return reqPosUsed;
	}
	
	void processPERequestHeader(uint8_t groupOffset, uint8_t reqPosUsed, uint8_t newByte){
		int clear=0;

		if((sys7IntBuffer[groupOffset][2] & 0xF) == PE_HEAD_STATE_IN_STRING){
			if (newByte == '"' && sys7CharBuffer[groupOffset][sys7IntBuffer[groupOffset][3]-1]!='\\') {
				if((sys7IntBuffer[groupOffset][2] & 0xF0) == PE_HEAD_KEY){
					if(!strcmp(sys7CharBuffer[groupOffset],"resource")){
						_pvoid = &peRquestDetails[reqPosUsed].resource;
					}
					if(!strcmp(sys7CharBuffer[groupOffset],"resId")){
						_pvoid = &peRquestDetails[reqPosUsed].resId;
					}
					if(!strcmp(sys7CharBuffer[groupOffset],"offset")){
						_pvoid = &peRquestDetails[reqPosUsed].offset;
					}
					if(!strcmp(sys7CharBuffer[groupOffset],"limit")){
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
				sys7CharBuffer[groupOffset][sys7IntBuffer[groupOffset][3]++] = newByte;
			  }
		} else if((sys7IntBuffer[groupOffset][2] & 0xF) == PE_HEAD_STATE_IN_NUMBER){;
			if ((newByte >= '0' && newByte <= '9') ) {
				int *n = (int *)_pvoid;
				*n =  *n * 10 + (newByte - '0');
			}else if(_pvoid!=0){
				_pvoid = 0;
				clear=1;
				sys7IntBuffer[groupOffset][2]=PE_HEAD_KEY + sys7IntBuffer[groupOffset][2] & 0xF;
			}
		} else if (newByte == ':') {
			sys7IntBuffer[groupOffset][2]=PE_HEAD_VALUE + sys7IntBuffer[groupOffset][2] & 0xF;
		}else if (newByte == ',') {
			sys7IntBuffer[groupOffset][2] = PE_HEAD_KEY + sys7IntBuffer[groupOffset][2] & 0xF;
		}else if ((newByte >= '0' && newByte <= '9') ) {
			int *n = (int *)_pvoid;
			*n =  newByte - '0';
			sys7IntBuffer[groupOffset][2] = (sys7IntBuffer[groupOffset][2] & 0xF0) + PE_HEAD_STATE_IN_NUMBER;
		} else if (newByte == '"') {
			sys7IntBuffer[groupOffset][2] = (sys7IntBuffer[groupOffset][2] & 0xF0) + PE_HEAD_STATE_IN_STRING;
		}
		
		if(clear){
			memset(sys7CharBuffer[groupOffset], 0, PE_HEAD_BUFFERLEN);
			sys7IntBuffer[groupOffset][3]=0;
			sys7IntBuffer[groupOffset][2] = (sys7IntBuffer[groupOffset][2] & 0xF0) + PE_HEAD_STATE_IN_OBJECT;
		}
		
	}
    #endif
    
    
    #ifdef M2_ENABLE_IDREQ
    void (*sendOutIdResponse)(uint8_t* devId, uint8_t* famId, uint8_t* modelId, uint8_t* ver) = 0;
    #endif
    
    
    
    void addCIHeader(uint8_t ciType, uint8_t* sysexHeader){

		sysexHeader[0]=0x7E;
		sysexHeader[1]=0x7F;
		sysexHeader[2]=0x0D;
		sysexHeader[3]=ciType;
		sysexHeader[4]=0x01; //TODO add version to arguments
		setBytesFromNumbers(sysexHeader, groupBlockMUID, 5, 4);
	}

    
    void endSysex7(uint8_t groupOffset){
      sysex7State[groupOffset] = false;
      sysexPos[groupOffset] = 0;
    };
    
    void startSysex7(uint8_t groupOffset){
      //Reset ALL SYSEX etc
      sysex7State[groupOffset] = true;
      sysexPos[groupOffset] = 0;
      sysexMode[groupOffset] = 0;
      sysUniNRTMode[groupOffset] = 0;
      sysUniPort[groupOffset] = -1;
      ciType[groupOffset] = 0;
      remoteMuid[groupOffset] = 0;
      destMuid[groupOffset] = 0;
      ciVer[groupOffset] = 0;
    };
    
    void processSysEx(uint8_t groupOffset, uint8_t newByte){
		//Serial.print(sysexPos[groupOffset]);Serial.print(" - ");Serial.println(newByte);
		
      if(sysexPos[groupOffset] == 0){
        if(newByte == S7UNIVERSAL_NRT || newByte == S7UNIVERSAL_RT){
          sysexMode[groupOffset] =  newByte;
          sysexPos[groupOffset]++;
          return;
        }
      }
      
      if(sysexMode[groupOffset] == S7UNIVERSAL_NRT){
        processUniS7NRT(groupOffset, newByte);
      }else{
        //TODO  
      }
      
      sysexPos[groupOffset]++;
    };
    
    
    //Proces all Non-Realtime Universal SysEx incl. MIDI-CI
    void processUniS7NRT(uint8_t groupOffset, uint8_t newByte){
		//Serial.print("  - processUniS7NRT:");Serial.println(sysUniNRTMode[groupOffset],HEX);
      if(sysexPos[groupOffset] == 1){
         sysUniPort[groupOffset] =  newByte;
          return;
      }
      
      if(sysexPos[groupOffset] == 2){
         sysUniNRTMode[groupOffset] =  newByte;
          return;
      }
      
      switch(sysUniNRTMode[groupOffset]){
		  
		#ifdef M2_ENABLE_IDREQ  
		case 0x06:
          if(sysexPos[groupOffset] == 3 && newByte == 0x01){
            //Identity Request - send a reply?
            //Serial.println("  -Identity Request - send a reply");
            uint8_t sysex[]={
			0x7E,0x7F,0x06,0x02
			,devId[0],devId[1],devId[2] //SyexId
			,famId[0],famId[1] //family id
			,modelId[0],modelId[1] //model id
			,ver[0],ver[1],ver[2],ver[3] //version id
			};
			
			if(sendOutSysex !=0) sendOutSysex(groupOffset + groupStart, sysex,15,0);
			
          }
          if(sysexPos[groupOffset] == 3 && newByte == 0x02){
            //Identity Reply
            sys7CharBuffer[groupOffset][0] = newByte;
            
          }
          if(sys7CharBuffer[groupOffset][0] == 0x02){
			if(sysexPos[groupOffset] >= 4 && sysexPos[groupOffset] <= 14){
				sys7CharBuffer[groupOffset][sysexPos[groupOffset]-3] = newByte; 
			}

			if (sysexPos[groupOffset] == 14 && sendOutIdResponse != 0){
				uint8_t manuIdR[3] = {sys7CharBuffer[groupOffset][1], sys7CharBuffer[groupOffset][2], sys7CharBuffer[groupOffset][3]};
				uint8_t famIdR[2] = {sys7CharBuffer[groupOffset][4], sys7CharBuffer[groupOffset][5]};
				uint8_t modelIdR[2] = {sys7CharBuffer[groupOffset][6], sys7CharBuffer[groupOffset][7]};
				uint8_t verR[4] = {sys7CharBuffer[groupOffset][8], sys7CharBuffer[groupOffset][9], sys7CharBuffer[groupOffset][10], sys7CharBuffer[groupOffset][11]};
				sendOutIdResponse(manuIdR, famIdR, modelIdR, verR);
			}
		  }
          break;
        #endif
        
		case 0x0D: // MIDI-CI
			
          if(sysexPos[groupOffset] == 3){ 
            ciType[groupOffset] =  newByte;
            //Serial.print("  - MIDI-CI ");Serial.println( ciType[groupOffset], HEX);
          }
          
          //Serial.print("  - MIDI-CI ");Serial.println( ciType[groupOffset], HEX);
          
          if(sysexPos[groupOffset] == 4){
            ciVer[groupOffset] =  newByte;
          } 
          if(sysexPos[groupOffset] >= 5 && sysexPos[groupOffset] <= 8){
			  sys7CharBuffer[groupOffset][sysexPos[groupOffset]-5] = newByte;  
          }
          if(sysexPos[groupOffset] == 8){
			  remoteMuid[groupOffset] =  sys7CharBuffer[groupOffset][0] + ((int)sys7CharBuffer[groupOffset][1] << 7) + ((sys7CharBuffer[groupOffset][2] + 0L) << 14) + ((sys7CharBuffer[groupOffset][3] + 0L) << 21);
			  //Serial.print("  remoteMuid:");
			 // Serial.println(remoteMuid[groupOffset]);
			 
		  }
          
          if(sysexPos[groupOffset] >= 9 && sysexPos[groupOffset] <= 12){
			  sys7CharBuffer[groupOffset][sysexPos[groupOffset]-9] = newByte;
          }
          if(sysexPos[groupOffset] == 12){
			  destMuid[groupOffset] =  sys7CharBuffer[groupOffset][0] + ((int)sys7CharBuffer[groupOffset][1] << 7) + ((sys7CharBuffer[groupOffset][2] + 0L) << 14) + ((sys7CharBuffer[groupOffset][3] + 0L) << 21);
			  //Serial.print("  destMuid:");
			 // Serial.println(destMuid[groupOffset]);
		  }
          
          
          
          
          if(sysexPos[groupOffset] >= 12 && destMuid[groupOffset] != groupBlockMUID && destMuid[groupOffset] != M2_CI_BROADCAST){
			  
			 // Serial.println("  Not for this device");
            return; //Not for this device
          }
          
          //TODO break up each Process based on ciType
          switch (ciType[groupOffset]){
            case 0x70: //Discovery Request'
			   //Serial.print("  - Discovery Request ");Serial.print(sysexPos[groupOffset]);Serial.print(" ");Serial.println(newByte,HEX);
			   if(sysexPos[groupOffset] >= 13 && sysexPos[groupOffset] <= 28){
				  sys7CharBuffer[groupOffset][sysexPos[groupOffset]-13] = newByte; 
			   }
			   if(sysexPos[groupOffset]==28){
				   //Serial.println("  - Discovery Request ");
				   if (recvDiscoveryRequest != 0){
						uint8_t manuIdR[3] = {sys7CharBuffer[groupOffset][0], sys7CharBuffer[groupOffset][1], sys7CharBuffer[groupOffset][2]};
						uint8_t famIdR[2] = {sys7CharBuffer[groupOffset][3], sys7CharBuffer[groupOffset][4]};
						uint8_t modelIdR[2] = {sys7CharBuffer[groupOffset][5], sys7CharBuffer[groupOffset][6]};
						uint8_t verR[4] = {sys7CharBuffer[groupOffset][7], sys7CharBuffer[groupOffset][8], sys7CharBuffer[groupOffset][9], sys7CharBuffer[groupOffset][10]};
						recvDiscoveryRequest(
							groupOffset + groupStart,
							remoteMuid[groupOffset],
							ciVer[groupOffset],
							manuIdR,
							famIdR,
							modelIdR,
							verR,
							sys7CharBuffer[groupOffset][11],
							sys7CharBuffer[groupOffset][12] + (sys7CharBuffer[groupOffset][13] << 7) + ((sys7CharBuffer[groupOffset][14] + 0L) << 14) + ((sys7CharBuffer[groupOffset][15] + 0L) << 21)
						);
					}
					
					//Send Discovery Reply
					if(sendOutSysex ==0) return;
					
					uint8_t sysex[13];
					addCIHeader(0x71,sysex);
					setBytesFromNumbers(sysex, remoteMuid[groupOffset], 9, 4);
					
					sendOutSysex(groupOffset + groupStart,sysex,13,1);
					sendOutSysex(groupOffset + groupStart,devId,3,2);
					sendOutSysex(groupOffset + groupStart,famId,2,2);
					sendOutSysex(groupOffset + groupStart,modelId,2,2);
					sendOutSysex(groupOffset + groupStart,ver,4,2);
					
					//Capabilities
					sysex[0]=0; 
					#ifdef M2_ENABLE_PROTOCOL
					sysex[0] += 0b10;
					#endif
					#ifdef M2_ENABLE_PROFILE
					sysex[0] += 0b100;
					#endif
					#ifdef M2_ENABLE_PE
					sysex[0] += 0b1000;
					#endif 
					sendOutSysex(groupOffset + groupStart,sysex,1,2);
					
					setBytesFromNumbers(sysex, sysExMax, 0, 4);
					sendOutSysex(groupOffset + groupStart,sysex,4,3);
					
			   }

               break;
            case 0x71: //Discovery Reply'
               if(sysexPos[groupOffset] >= 13 && sysexPos[groupOffset] <= 28){
				  sys7CharBuffer[groupOffset][sysexPos[groupOffset]-13] = newByte; 
			   }
			   if(sysexPos[groupOffset]==28){
				   if (recvDiscoveryReply != 0){
						uint8_t manuIdR[3] = {sys7CharBuffer[groupOffset][0], sys7CharBuffer[groupOffset][1], sys7CharBuffer[groupOffset][2]};
						uint8_t famIdR[2] = {sys7CharBuffer[groupOffset][3], sys7CharBuffer[groupOffset][4]};
						uint8_t modelIdR[2] = {sys7CharBuffer[groupOffset][5], sys7CharBuffer[groupOffset][6]};
						uint8_t verR[4] = {sys7CharBuffer[groupOffset][7], sys7CharBuffer[groupOffset][8], sys7CharBuffer[groupOffset][9], sys7CharBuffer[groupOffset][10]};
						recvDiscoveryReply(
							groupOffset + groupStart,
							remoteMuid[groupOffset],
							ciVer[groupOffset],
							manuIdR,
							famIdR,
							modelIdR,
							verR,
							sys7CharBuffer[groupOffset][11],
							sys7CharBuffer[groupOffset][12] + (sys7CharBuffer[groupOffset][13] << 7) + (sys7CharBuffer[groupOffset][14] << 14) + (sys7CharBuffer[groupOffset][15] << 21)
							);
					}
				}
				break;
				
			case 0x7E: //MIDI-CI Invalidate MUID Message
				if(sysexPos[groupOffset] == 13){
					destMuid[groupOffset] = 0;
				}
			
				if(sysexPos[groupOffset] >= 13 && sysexPos[groupOffset] <= 16){
					destMuid[groupOffset] =  destMuid[groupOffset] + (newByte << (7 * (sysexPos[groupOffset] - 13)));
			    }
			
                if (sysexPos[groupOffset] == 16 && recvInvalidateMUID != 0){
					recvInvalidateMUID(groupOffset + groupStart,remoteMuid[groupOffset], destMuid[groupOffset]);
				}
				break;	
			case 0x7F: //MIDI-CI NAK
                if (recvNAK != 0){
					recvNAK(groupOffset + groupStart,remoteMuid[groupOffset]);
				}
				break;
               
            #ifdef M2_ENABLE_PROTOCOL
            #endif     
            
            #ifdef M2_ENABLE_PROFILE  
            case 0x20: //Profile Inquiry
				
				if (sysexPos[groupOffset] == 12 && recvProfileInquiry != 0){
					//Serial.print("<-Profile Inquiry: remoteMuid ");Serial.print(remoteMuid[groupOffset]);
					//Serial.print(" dest ");Serial.println(sysUniPort[groupOffset]);
					recvProfileInquiry(groupOffset + groupStart, remoteMuid[groupOffset], sysUniPort[groupOffset]);
				}
               break;
            case 0x21: //Reply to Profile Inquiry
				//Serial.print("<-Reply to Profile Inquiry: remoteMuid ");Serial.print(remoteMuid[groupOffset]);
				//Serial.print(" dest ");Serial.println(sysUniPort[groupOffset]);
				
				//Serial.print(sysexPos[groupOffset]);Serial.print(" - ");Serial.println(newByte);
				
				if(sysexPos[groupOffset] == 13){
					sys7IntBuffer[groupOffset][0] = (int)newByte;
				}
				if(sysexPos[groupOffset] == 14){
					sys7IntBuffer[groupOffset][0] += (int)newByte << 7;
				}
				
				if(sysexPos[groupOffset] == 13 + sys7CharBuffer[groupOffset][0]*5){
					sys7IntBuffer[groupOffset][1] = (int)newByte;
				}
				if(sysexPos[groupOffset] == 14 + sys7CharBuffer[groupOffset][0]*5){
					sys7IntBuffer[groupOffset][1] += (int)newByte << 7;
				}
				if(sysexPos[groupOffset] >= 15 && sysexPos[groupOffset] <= 12 + sys7IntBuffer[groupOffset][0]*5){
					uint8_t pos = (sysexPos[groupOffset] - 13) % 5;
					sys7CharBuffer[groupOffset][pos] = newByte;
					if(pos==4 && recvSetProfileEnabled!=0){
						uint8_t profile[5] = {sys7CharBuffer[groupOffset][0], sys7CharBuffer[groupOffset][1], sys7CharBuffer[groupOffset][2], sys7CharBuffer[groupOffset][3], sys7CharBuffer[groupOffset][4]};
						recvSetProfileEnabled(groupOffset + groupStart,remoteMuid[groupOffset], sysUniPort[groupOffset], profile); 
					}
				}
				
				if(sysexPos[groupOffset] >= 15 + sys7CharBuffer[groupOffset][0]*5  && sysexPos[groupOffset] <= 12 + sys7IntBuffer[groupOffset][0]*5 + sys7IntBuffer[groupOffset][1]*5){
					uint8_t pos = (sysexPos[groupOffset] - 13) % 5;
					sys7CharBuffer[groupOffset][pos] = newByte;
					if(pos==4 && recvSetProfileDisabled!=0){
						uint8_t profile[5] = {sys7CharBuffer[groupOffset][0], sys7CharBuffer[groupOffset][1], sys7CharBuffer[groupOffset][2], sys7CharBuffer[groupOffset][3], sys7CharBuffer[groupOffset][4]};
						recvSetProfileDisabled(groupOffset + groupStart,remoteMuid[groupOffset], sysUniPort[groupOffset], profile); 
					}
				}
               //processProfileInquiryReply(newByte);
               break;
            case 0x22: //Set Profile On Message
				if(sysexPos[groupOffset] >= 13 && sysexPos[groupOffset] <= 17){
				  sys7CharBuffer[groupOffset][sysexPos[groupOffset]-13] = newByte; 
				}
				if (sysexPos[groupOffset] == 16 && recvInvalidateMUID != 0){
					uint8_t profile[5] = {sys7CharBuffer[groupOffset][0], sys7CharBuffer[groupOffset][1], sys7CharBuffer[groupOffset][2], sys7CharBuffer[groupOffset][3], sys7CharBuffer[groupOffset][4]};
					recvSetProfileOn(groupOffset + groupStart,remoteMuid[groupOffset], sysUniPort[groupOffset], profile); 
				}
				break;
			case 0x23: //Set Profile Off Message
				if(sysexPos[groupOffset] >= 13 && sysexPos[groupOffset] <= 17){
				  sys7CharBuffer[groupOffset][sysexPos[groupOffset]-13] = newByte; 
				}
				if (sysexPos[groupOffset] == 16 && recvInvalidateMUID != 0){
					uint8_t profile[5] = {sys7CharBuffer[groupOffset][0], sys7CharBuffer[groupOffset][1], sys7CharBuffer[groupOffset][2], sys7CharBuffer[groupOffset][3], sys7CharBuffer[groupOffset][4]};
					recvSetProfileOff(groupOffset + groupStart,remoteMuid[groupOffset], sysUniPort[groupOffset], profile); 
				}
				break;	
            case 0x24: //Set Profile Enabled Message
				if(sysexPos[groupOffset] >= 13 && sysexPos[groupOffset] <= 17){
				  sys7CharBuffer[groupOffset][sysexPos[groupOffset]-13] = newByte; 
				}
				if (sysexPos[groupOffset] == 16 && recvInvalidateMUID != 0){
					uint8_t profile[5] = {sys7CharBuffer[groupOffset][0], sys7CharBuffer[groupOffset][1], sys7CharBuffer[groupOffset][2], sys7CharBuffer[groupOffset][3], sys7CharBuffer[groupOffset][4]};
					recvSetProfileEnabled(groupOffset + groupStart,remoteMuid[groupOffset], sysUniPort[groupOffset], profile); 
				}
				break;  
			case 0x25: //Set Profile Diabled Message
				if(sysexPos[groupOffset] >= 13 && sysexPos[groupOffset] <= 17){
				  sys7CharBuffer[groupOffset][sysexPos[groupOffset]-13] = newByte; 
				}
				if (sysexPos[groupOffset] == 16 && recvInvalidateMUID != 0){
					uint8_t profile[5] = {sys7CharBuffer[groupOffset][0], sys7CharBuffer[groupOffset][1], sys7CharBuffer[groupOffset][2], sys7CharBuffer[groupOffset][3], sys7CharBuffer[groupOffset][4]};
					recvSetProfileDisabled(groupOffset + groupStart,remoteMuid[groupOffset], sysUniPort[groupOffset], profile); 
				}
				break;   
            #endif   
            
            
            #ifdef M2_ENABLE_PE 
            case 0x30: //Inquiry: Property Exchange Capabilities
				if(sysexPos[groupOffset] == 13){
					uint8_t sysex[14];
					addCIHeader(0x31,sysex);
					setBytesFromNumbers(sysex, remoteMuid[groupOffset], 9, 4);
					//Simultaneous Requests Supports
					sysex[13]=numRequests;
					if(sendOutSysex !=0) sendOutSysex(groupOffset + groupStart,sysex,14,0);
					
					if(recvPECapabilities != 0)recvPECapabilities(groupOffset + groupStart,remoteMuid[groupOffset], newByte);
				}
				
               break;
            case 0x31: //Reply to Property Exchange Capabilities
               if(sysexPos[groupOffset] == 13 && recvPECapabilities != 0){
					recvPECapabilities(groupOffset + groupStart,remoteMuid[groupOffset], newByte);
			   }
               break;
            
            case 0x34: { // Inquiry: Get Property Data
				uint8_t reqPosUsed;
				if(sysexPos[groupOffset] >= 13){
					 reqPosUsed=getPERequestId(groupOffset, newByte);
					 
					 //Serial.print(" - reqPosUsed");Serial.println(reqPosUsed);
				
					if(reqPosUsed == 255){
						//Ignore this Get Message
						return;
					}
				 }
				
				if(sysexPos[groupOffset] == 14){ //header Length
					sys7IntBuffer[groupOffset][1] = (int)newByte;
				}
				if(sysexPos[groupOffset] == 15){
					sys7IntBuffer[groupOffset][1] += (int)newByte << 7;
					
					sys7IntBuffer[groupOffset][2] = PE_HEAD_KEY + PE_HEAD_STATE_IN_OBJECT;
					sys7IntBuffer[groupOffset][3] = 0; //bufferPos
				}
				
				
				if(sysexPos[groupOffset] >= 16 && sysexPos[groupOffset] <= 15 + sys7IntBuffer[groupOffset][1]){
					processPERequestHeader(groupOffset, reqPosUsed, newByte);
				}
				
				if(sysexPos[groupOffset] == 15 + sys7IntBuffer[groupOffset][1]){
					if(recvPEGetInquiry != 0) recvPEGetInquiry(groupOffset + groupStart, remoteMuid[groupOffset], peRquestDetails[reqPosUsed]);	
					cleanupRequestId(peRquestDetails[reqPosUsed].requestId); 
				}
				
				
				
				
            
				break;
				
			}
			case 0x35: // Reply To Get Property Data - Needs Work!
				/*uint8_t reqPosUsed;
				if(sysexPos[groupOffset] >= 13){
					 reqPosUsed=getPERequestId(groupOffset, newByte); //Should this use the same pe Header structs?? / reqId's may mismatch here
				
					if(reqPosUsed == 255){
						//Ignore this Get Message
						return;
					}
				 }
				
				if(sysexPos[groupOffset] == 14){ //header Length
					sys7IntBuffer[groupOffset][1] = (int)newByte;
				}
				if(sysexPos[groupOffset] == 15){
					sys7IntBuffer[groupOffset][1] += (int)newByte << 7;
					
					sys7IntBuffer[groupOffset][2] = PE_HEAD_KEY + PE_HEAD_STATE_IN_OBJECT;
					sys7IntBuffer[groupOffset][3] = 0; //bufferPos
				}
				
				
				if(sysexPos[groupOffset] >= 16 && sysexPos[groupOffset] <= 15 + sys7IntBuffer[groupOffset][1]){

					
					bool clear=false;

					if((sys7IntBuffer[groupOffset][2] & 0xF) == PE_HEAD_STATE_IN_STRING){
						//Serial.println(" - in PE_HEAD_STATE_IN_STRING 1");
						if (newByte == '"' && sys7CharBuffer[groupOffset][sys7IntBuffer[groupOffset][3]-1]!='\\') {
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
							sys7CharBuffer[groupOffset][sys7IntBuffer[groupOffset][3]++] = newByte;
						  }
					} else if((sys7IntBuffer[groupOffset][2] & 0xF) == PE_HEAD_STATE_IN_NUMBER){
						//Serial.println(" - in PE_HEAD_STATE_IN_NUMBER");
						if ((newByte >= '0' && newByte <= '9') ) {
							int *n = (int *)_pvoid;
							*n =  *n * 10 + (newByte - '0');
						}else if(_pvoid!=0){
							_pvoid = 0;
							clear=true;
							sys7IntBuffer[groupOffset][2]=PE_HEAD_KEY + sys7IntBuffer[groupOffset][2] & 0xF;
						}
					} else if (newByte == ':') {
						//Serial.println(" - in PE_HEAD_VALUE");
						sys7IntBuffer[groupOffset][2]=PE_HEAD_VALUE + sys7IntBuffer[groupOffset][2] & 0xF;
					}else if (newByte == ',') {
						//Serial.println(" - in PE_HEAD_KEY");
						sys7IntBuffer[groupOffset][2] = PE_HEAD_KEY + sys7IntBuffer[groupOffset][2] & 0xF;
					}else if ((newByte >= '0' && newByte <= '9') ) {
						int *n = (int *)_pvoid;
						*n =  newByte - '0';
						sys7IntBuffer[groupOffset][2] = (sys7IntBuffer[groupOffset][2] & 0xF0) + PE_HEAD_STATE_IN_NUMBER;
						//Serial.println(" - set PE_HEAD_STATE_IN_NUMBER");
					} else if (newByte == '"') {
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
					requestDetails.totalChunks = (int)newByte;
				}
				if(sysexPos[groupOffset] == 17 + sys7IntBuffer[groupOffset][1]){
					requestDetails.totalChunks = += (int)newByte << 7;
				}
				
				if(sysexPos[groupOffset] == 18 + sys7IntBuffer[groupOffset][1]){ 
					requestDetails.numChunks = (int)newByte;
				}
				if(sysexPos[groupOffset] == 19 + sys7IntBuffer[groupOffset][1]){
					requestDetails.numChunks = += (int)newByte << 7;
				}
				
				if(sysexPos[groupOffset] == 20 + sys7IntBuffer[groupOffset][1]){ //Body Length
					sys7IntBuffer[groupOffset][4] = (int)newByte;
				}
				if(sysexPos[groupOffset] == 21 + sys7IntBuffer[groupOffset][1]){
					sys7IntBuffer[groupOffset][4] = += (int)newByte << 7;
				}
				
				
				
				if(sysexPos[groupOffset] == 15 + sys7IntBuffer[groupOffset][1]){
					//if(recvPEGetReply != 0) recvPEGetReply(groupOffset + groupStart, remoteMuid[groupOffset], peRquestDetails[reqPosUsed]);	
					//cleanupRequestId(requestDetails.requestId); 
				}*/
				
				
				
            
				break;
				
				
			case 0x36: {// Inquiry: Set Property Data
				uint8_t reqPosUsed;
				if(sysexPos[groupOffset] >= 13){
					 reqPosUsed=getPERequestId(groupOffset, newByte);
					 
					 //Serial.print(" - reqPosUsed");Serial.println(reqPosUsed);
				
					if(reqPosUsed == 255){
						//Ignore this Get Message
						return;
					}
				 }
				
				if(sysexPos[groupOffset] == 14){ //header Length
					sys7IntBuffer[groupOffset][1] = (int)newByte;
				}
				if(sysexPos[groupOffset] == 15){
					sys7IntBuffer[groupOffset][1] += (int)newByte << 7;
					
					sys7IntBuffer[groupOffset][2] = PE_HEAD_KEY + PE_HEAD_STATE_IN_OBJECT;
					sys7IntBuffer[groupOffset][3] = 0; //bufferPos
				}
				
				
				if(sysexPos[groupOffset] >= 16 && sysexPos[groupOffset] <= 15 + sys7IntBuffer[groupOffset][1]){
					processPERequestHeader(groupOffset, reqPosUsed, newByte);
				}
				
				
				if(sysexPos[groupOffset] == 16 + sys7IntBuffer[groupOffset][1]){ 
					peRquestDetails[reqPosUsed].totalChunks = (int)newByte;
				}
				if(sysexPos[groupOffset] == 17 + sys7IntBuffer[groupOffset][1]){
					peRquestDetails[reqPosUsed].totalChunks += (int)newByte << 7;
				}
				
				if(sysexPos[groupOffset] == 18 + sys7IntBuffer[groupOffset][1]){ 
					peRquestDetails[reqPosUsed].numChunks = (int)newByte;
				}
				if(sysexPos[groupOffset] == 19 + sys7IntBuffer[groupOffset][1]){
					peRquestDetails[reqPosUsed].numChunks += (int)newByte << 7;
				}
				
				if(sysexPos[groupOffset] == 20 + sys7IntBuffer[groupOffset][1]){ //Body Length
					sys7IntBuffer[groupOffset][4] = (int)newByte;
				}
				if(sysexPos[groupOffset] == 21 + sys7IntBuffer[groupOffset][1]){
					sys7IntBuffer[groupOffset][4] += (int)newByte << 7;
				}
				
				
				int initPos = 22 + sys7IntBuffer[groupOffset][1];
				if(sysexPos[groupOffset] >= initPos && sysexPos[groupOffset] <= initPos - 1 + sys7IntBuffer[groupOffset][4]){
					uint8_t charOffset = (initPos -  sysexPos[groupOffset]) % PE_HEAD_BUFFERLEN;
					sys7CharBuffer[groupOffset][charOffset] = newByte;
					
					if(charOffset == PE_HEAD_BUFFERLEN -1 
						|| sysexPos[groupOffset] == initPos - 1 + sys7IntBuffer[groupOffset][4]
					){
						if(recvPESetInquiry != 0) recvPESetInquiry(groupOffset + groupStart, remoteMuid[groupOffset], peRquestDetails[reqPosUsed], charOffset+1, sys7CharBuffer[groupOffset]);	
					} 
				}
				
				
				if(sysexPos[groupOffset] == initPos - 1 + sys7IntBuffer[groupOffset][4] && peRquestDetails[reqPosUsed].numChunks == peRquestDetails[reqPosUsed].totalChunks){	
					cleanupRequestId(peRquestDetails[reqPosUsed].requestId); 
				}
				
				break;
               }
            #endif   
            
          }
          
          break;
        
        
      }
    }
    
  
  
    
  public:
  
	//This Device's Data
	uint32_t groupBlockMUID = 0; 
	uint8_t devId[3];
    uint8_t famId[2];
    uint8_t modelId[2];
    uint8_t ver[4]; 
    
    uint16_t sysExMax = 512;
    
    
  
    midi2Processor(uint8_t grStart, uint8_t totalGroups
		#ifdef M2_ENABLE_PE
		, uint8_t numRequestsTotal
		#endif
	){
		
		groupStart = grStart;
		groups = totalGroups;
		
		sysexPos = malloc(sizeof(int) * groups); 
		sysexMode = malloc(sizeof(uint8_t) * groups); 
		sysUniNRTMode = malloc(sizeof(uint8_t) * groups); 
		sysUniPort = malloc(sizeof(uint8_t) * groups); 
		ciType = malloc(sizeof(uint8_t) * groups); 
		ciVer = malloc(sizeof(uint8_t) * groups); 
		remoteMuid = malloc(sizeof(uint32_t) *  groups); 
		destMuid = malloc(sizeof(uint32_t) *  groups); 
		
		#ifdef M2_ENABLE_PE
		numRequests = numRequestsTotal;
		peRquestDetails  = malloc(sizeof(peHeader) *  numRequestsTotal);
		
		for(uint8_t i =0;i<numRequests;i++){
			peRquestDetails[i].requestId = 255;
			memset(peRquestDetails[i].resource,0,PE_HEAD_BUFFERLEN);
			memset(peRquestDetails[i].resId,0,PE_HEAD_BUFFERLEN);
			peRquestDetails[i].offset=-1;
			peRquestDetails[i].limit=-1;
			peRquestDetails[i].status=-1;
   
		}
		#endif
		
		
		sys7CharBuffer = (uint8_t**)malloc(sizeof(uint8_t*) * groups);
		for(uint8_t i=0; i< groups; i++){
			*(sys7CharBuffer + i) = (uint8_t*)malloc(sizeof(uint8_t*) * 
			#ifdef M2_ENABLE_PE
			PE_HEAD_BUFFERLEN
			#else
			20
			#endif
			);
		}
		
		sys7IntBuffer = (uint16_t**)malloc(sizeof(uint16_t*) * groups);
		for(uint8_t i=0; i< groups; i++){
			*(sys7IntBuffer + i) = (uint16_t*)malloc(sizeof(uint16_t) * 
			#ifdef M2_ENABLE_PE
			5
			#else
			2
			#endif
			);
		}
  
	};
	
	~midi2Processor() { 
		free(sysexPos); sysexPos = NULL; 
		free(sysexMode); sysexMode = NULL; 
		free(sysUniNRTMode); sysUniNRTMode = NULL; 
		free(sysUniNRTMode); sysUniNRTMode = NULL; 
		free(ciVer); ciVer = NULL; 
		free(remoteMuid); remoteMuid = NULL; 
		free(destMuid); destMuid = NULL; 
		free(sys7CharBuffer); sys7CharBuffer = NULL; 
		free(sys7IntBuffer); sys7IntBuffer = NULL; 
		
		#ifdef M2_ENABLE_PE
		free(peRquestDetails); peRquestDetails = NULL;
		#endif
		
	} 
	
	
    void processUMP(uint32_t UMP){
		umpMess[messPos] = UMP;
		//Serial.print(" UMP Proc: ");Serial.print(messPos);Serial.print("  ");Serial.println(umpMess[messPos]);

		
		uint8_t mt = umpMess[0] >> 28  & 0xF;
		uint8_t group = umpMess[0] >> 24 & 0xF;
		uint8_t groupOffset = group - groupStart;  
	
		
		if(messPos == 0 && mt <= 0x02){ //32bit Messages
			
			if(group < groupStart || group > groupStart + groups -1){
				//Not for this Group Block
				//Serial.println("  Not for this Group Block");
						
			}else			
			if(mt == 0){ //32 bits Utility Messages
			  // TODO Break up into JR TimeStamp Messages offsets
			  uint8_t group = (umpMess[0] >> 24) & 0xF;
			  uint8_t status = (umpMess[0] >> 20) & 0xF;
			  
			  #ifdef M2_ENABLE_JR
			  uint16_t timing = (umpMess[0] >> 16) & 0xFFFF;
			  #endif
			  
			  switch(status){
				  case 0: // NOOP 
					//if(group== 0 && noop != 0) noop();
					break;
				#ifdef M2_ENABLE_JR	
				  case 0b1: // JR Clock Message 
					if(jrClock != 0) jrClock(group, timing);
					break;
				  case 0b10: //JR Timestamp Message
					//??? Message out or attach to next message?
				  
					break;
				#endif
			  }
			  
			} else 
			if(mt == 1){ //32 bits System Real Time and System Common Messages (except System Exclusive)
			  //Send notice
			  uint8_t group = (umpMess[0] >> 24) & 0xF;
			  uint8_t status = umpMess[0] >> 16 & 0xFF;
			  switch(status){
				  case TIMING_CODE:
					uint8_t timing = (umpMess[0] >> 8) & 0x7F;
					if(timingCode != 0) timingCode(group, timing); 
					break;
				  case SPP:
					uint16_t position = (umpMess[0] >> 8) & 0x7F  + ((umpMess[0] & 0x7F) << 7);
					if(songPositionPointer != 0) songPositionPointer(group, position);
					break;
				  case SONG_SELECT:
					uint8_t song = (umpMess[0] >> 8) & 0x7F;
					if(songSelect != 0) songSelect(group, song); 
					break;
				  case TUNEREQUEST:
					if(tuneRequest != 0) tuneRequest(group); 
					break;
				  case TIMINGCLOCK:
					if(timingClock != 0) timingClock(group); 
					break;
				  case SEQSTART:
					if(seqStart != 0) seqStart(group);
					break;
				  case SEQCONT:
					if(seqCont != 0) seqCont(group);
					break;
				  case SEQSTOP:
					if(seqStop != 0) seqStop(group);
					break;
				  case ACTIVESENSE:
					if(activeSense != 0) activeSense(group);
					break;
				  case SYSTEMRESET:
					if(systemReset != 0) systemReset(group);
					break;
			  }
			
			} else 
			if(mt == 2){ //32 Bits MIDI 1.0 Channel Voice Messages
				//Serial.println("  UMP 0x02 (M1) Message");
				uint8_t group = (umpMess[0] >> 24) & 0xF;
				uint8_t status = umpMess[0] >> 16 & 0xF0;
				uint8_t channel = (umpMess[0] >> 16) & 0xF;
				uint8_t val1 = (umpMess[0] >> 8) & 0x7F;
				uint8_t val2 = umpMess[0] & 0x7F;
				
				
				switch(status){
				   case NOTE_OFF: //Note Off
						if(midiNoteOff != 0) midiNoteOff(group, channel, val1, scaleUp(val2,7,16), 0, 0); 
						break;
				   case NOTE_ON: //Note On
						if(midiNoteOn != 0) midiNoteOn(group, channel, val1, scaleUp(val2,7,16), 0, 0); 
						break;
				   case CHANNEL_PRESSURE: //Poly Pressure
						if(polyPressure != 0) polyPressure(group, channel, val1, scaleUp(val2,7,32)); 
						break;	
				   case CC: //CC
						if(controlChange != 0) controlChange(group, channel, val1, scaleUp(val2,7,32)); 
						break;
				   case PROGRAM_CHANGE: //Program Change Message
						if(programChange != 0) programChange(group, channel, val1, false, 0, 0);  
						break;
				   case CHANNEL_PRESSURE: //Channel Pressure
						if(channelPressure != 0) channelPressure(group, channel, scaleUp(val1,7,32)); 
						break;
				   case PITCH_BEND: //PitchBend
						if(pitchBend != 0) pitchBend(group, channel, scaleUp((val2 << 7) + val1,14,32)); 
						break;		
				}				
			}
			return;
			
		}else		
		if(messPos == 1 && mt <= 0x04){ //64bit Messages
			if(group < groupStart || group > groupStart + groups -1){
				//Not for this Group Block
				//Serial.println("  Not for this Group Block");
						
			}else			
			if(mt == 3){ //64 bits Data Messages (including System Exclusive)
				
				uint8_t numbytes  = (umpMess[0] >> 16) & 0xF;
				uint8_t status = (umpMess[0] >> 20) & 0xF;
				if(status == 0 || status == 1){
				  startSysex7(groupOffset);
				}
				
				if(numbytes > 0)processSysEx(groupOffset, (umpMess[0] >> 8) & 0x7F);
				if(numbytes > 1)processSysEx(groupOffset, umpMess[0] & 0x7F);
				if(numbytes > 2)processSysEx(groupOffset, (umpMess[1] >> 24) & 0x7F);
				if(numbytes > 3)processSysEx(groupOffset, (umpMess[1] >> 16) & 0x7F);
				if(numbytes > 4)processSysEx(groupOffset, (umpMess[1] >> 8) & 0x7F);
				if(numbytes > 5)processSysEx(groupOffset, umpMess[1] & 0x7F);
				
				if(status == 0 || status == 3){
				  endSysex7(groupOffset);
				}
			} else 
			if(mt == 4){//64 bits MIDI 2.0 Channel Voice Messages
			
			   uint8_t status = umpMess[0] >> 16 & 0xF0;
			   uint8_t channel = umpMess[0] >> 16 & 0xF;
			   uint8_t val1 = umpMess[0] >> 8 & 0xFF;
			   uint8_t val2 = umpMess[0] & 0xFF;
			   
			   switch(status){
				   case NOTE_OFF: //Note Off
						if(midiNoteOff != 0) midiNoteOff(group, channel, val1, umpMess[1] >> 16, val2, umpMess[1] & 65535); 
						break;
				   
				   case NOTE_ON: //Note On
						if(midiNoteOn != 0) midiNoteOn(group, channel, val1, umpMess[1] >> 16, val2, umpMess[1] & 65535); 
						break;
						
				   case KEY_PRESSURE: //Poly Pressure
						if(polyPressure != 0) polyPressure(group, channel, val1, umpMess[1]); 
						break;	
				   
				   case CC: //CC
						if(controlChange != 0) controlChange(group, channel, val1, umpMess[1]);
						break;	
				   
				   case RPN: //RPN
						if(rpn != 0) rpn(group, channel, val1, val2, umpMess[1]); 
						break;	
				   
				   case NRPN: //NRPN
						//if(RPN != 0) RPN(group, channel, val1, val2, umpMess[1]); 
						break;	
				   
				   case RPN_RELATIVE: //Relative RPN
						//if(RRPN != 0) RRPN(group, channel, val1, val2, umpMess[1]/*twoscomplement*/); 
						break;	
				   case NRPN_RELATIVE: //Relative NRPN
						//if(RNPN != 0) RNPN(group, channel, val1, val2, umpMess[1]/*twoscomplement*/); 
						break;
				   
				   case PROGRAM_CHANGE: //Program Change Message
						if(programChange != 0) programChange(group, channel, umpMess[1] >> 24, umpMess[0] & 1 , (umpMess[1] >> 8) & 0x7f , umpMess[1] & 0x7f);
						break;
				   case CHANNEL_PRESSURE: //Channel Pressure
						if(channelPressure != 0) channelPressure(group, channel, umpMess[1]); 
						break;
				   case PITCH_BEND: //PitchBend
						if(pitchBend != 0) pitchBend(group, channel, umpMess[1]);
						break;	
						
				   case PITCH_BEND_PERNOTE: //Per Note PitchBend 6
						//if(midiNoteOn != 0) channelPressure(group, channel, umpMess[1]); 
						break;		
				   case NRPN_PERNOTE: //Assignable Per-Note Controller 1
						//if(midiNoteOn != 0) channelPressure(group, channel, umpMess[1]); 
						break;	
						
				   case RPN_PERNOTE: //Registered Per-Note Controller 0 
						//if(midiNoteOn != 0) channelPressure(group, channel, umpMess[1]); 
						break;	
						
				   case PERNOTE_MANAGE: //Per-Note Management Message
						//if(midiNoteOn != 0) channelPressure(group, channel, umpMess[1]); 
						break;	
						
			   }
			}
			messPos =0;
		}else		
		if(messPos == 3 && mt <= 0x05){ //128bit Messages
			if(group < groupStart || group > groupStart + groups -1){
				//Not for this Group Block
						
			}
			messPos =0;
		} else {
			messPos++;
		}
		
    }
    
    
    #ifdef M2_ENABLE_IDREQ
    void sendIdentityRequest (uint8_t group){
		if(sendOutSysex ==0) return;
		uint8_t sysex[]={0x7E,0x7F,0x06,0x01};
		sendOutSysex(group,sysex,4,0);
	}
    #endif
    
    void sendDiscoveryRequest(uint8_t group, uint8_t ciVer){
		if(sendOutSysex ==0) return;
		
		uint8_t sysex[13];
		addCIHeader(0x70,sysex);
		setBytesFromNumbers(sysex, M2_CI_BROADCAST, 9, 4);
		sendOutSysex(group,sysex,13,1);
		sendOutSysex(group,devId,3,2);
		sendOutSysex(group,famId,2,2);
		sendOutSysex(group,modelId,2,2);
		sendOutSysex(group,ver,4,2);
		
		//Capabilities
		sysex[0]=0; 
		#ifdef M2_ENABLE_PROTOCOL
		sysex[0] += 0b10;
		#endif
		#ifdef M2_ENABLE_PROFILE
		sysex[0] += 0b100;
		#endif
		#ifdef M2_ENABLE_PE
		sysex[0] += 0b1000;
		#endif 
		sendOutSysex(group,sysex,1,2);
		
		setBytesFromNumbers(sysex, sysExMax, 0, 4);
		sendOutSysex(group,sysex,4,3);
	}
	
	void sendNAK(uint8_t group, uint32_t remoteMuid, uint8_t ciVer){
		if(sendOutSysex ==0) return;
		uint8_t sysex[13];
		addCIHeader(0x7F,sysex);
		setBytesFromNumbers(sysex, remoteMuid, 9, 4);
		sendOutSysex(group,sysex,13,0);
	}
	
	#ifdef M2_ENABLE_PROFILE
	void sendProfileListRequest(uint8_t group, uint32_t remoteMuid, uint8_t ciVer, uint8_t destination){
		if(sendOutSysex ==0) return;
		uint8_t sysex[13];
		addCIHeader(0x20,sysex);
		sysex[1] = destination;
		setBytesFromNumbers(sysex, remoteMuid, 9, 4);
		sendOutSysex(group,sysex,13,0);
	}
	
	
	void sendProfileListResponse(uint8_t group, uint32_t remoteMuid, uint8_t ciVer, uint8_t destination, uint8_t profilesEnabledLen, uint8_t* profilesEnabled, uint8_t profilesDisabledLen , uint8_t* profilesDisabled ){
		if(sendOutSysex ==0) return;
		uint8_t sysex[13];
		addCIHeader(0x21,sysex);
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
	
	void sendProfileOn(uint8_t group, uint32_t remoteMuid, uint8_t ciVer, uint8_t destination, uint8_t* profile){
		if(sendOutSysex ==0) return;
		uint8_t sysex[13];
		addCIHeader(0x22,sysex);
		sysex[1] = destination;
		setBytesFromNumbers(sysex, remoteMuid, 9, 4);
		sendOutSysex(group,sysex,13,1);
		sendOutSysex(group,profile,5,3);
	}
	
	void sendProfileOff(uint8_t group, uint32_t remoteMuid, uint8_t ciVer, uint8_t destination, uint8_t* profile){
		if(sendOutSysex ==0) return;
		uint8_t sysex[13];
		addCIHeader(0x23,sysex);
		sysex[1] = destination;
		setBytesFromNumbers(sysex, remoteMuid, 9, 4);
		sendOutSysex(group,sysex,13,1);
		sendOutSysex(group,profile,5,3);
	}
	
	void sendProfileEanbled(uint8_t group, uint32_t remoteMuid, uint8_t ciVer, uint8_t destination, uint8_t* profile){
		if(sendOutSysex ==0) return;
		uint8_t sysex[13];
		addCIHeader(0x24,sysex);
		sysex[1] = destination;
		setBytesFromNumbers(sysex, remoteMuid, 9, 4);
		sendOutSysex(group,sysex,13,1);
		sendOutSysex(group,profile,5,3);
	}
	
	void sendProfileDisabled(uint8_t group, uint32_t remoteMuid, uint8_t ciVer, uint8_t destination, uint8_t* profile){
		if(sendOutSysex ==0) return;
		uint8_t sysex[13];
		addCIHeader(0x25,sysex);
		sysex[1] = destination;
		setBytesFromNumbers(sysex, remoteMuid, 9, 4);
		sendOutSysex(group,sysex,13,1);
		sendOutSysex(group,profile,5,3);
	}
    #endif 
    
    
    #ifdef M2_ENABLE_PE
    void sendPECapabilityRequest(uint8_t group, uint32_t remoteMuid, uint8_t ciVer, uint8_t numRequests){
		if(sendOutSysex ==0) return;
		uint8_t sysex[14];
		addCIHeader(0x30,sysex);
		setBytesFromNumbers(sysex, remoteMuid, 9, 4);
		sysex[13] = numRequests;
		sendOutSysex(group,sysex,14,0);
	}
	
	void sendPEGet(uint8_t group, uint32_t remoteMuid, uint8_t ciVer, uint8_t requestId, uint16_t headerLen, uint8_t* header){
		if(sendOutSysex ==0) return;
		uint8_t sysex[13];
		addCIHeader(0x35,sysex);
		setBytesFromNumbers(sysex, remoteMuid, 9, 4);
		sendOutSysex(group,sysex,13,1);
		
		sysex[0] = requestId;
		setBytesFromNumbers(sysex, headerLen, 1, 2);
		sendOutSysex(group,sysex,3,2);		
		sendOutSysex(group, header,headerLen,2);
		
		setBytesFromNumbers(sysex, 0, 0, 6);
		sendOutSysex(group,sysex,6,3);
	}
	
	
	void sendPEGetReply(uint8_t group, uint32_t remoteMuid, uint8_t ciVer, uint8_t requestId, uint16_t headerLen, uint8_t* header, int numberOfChunks, int numberOfThisChunk, uint16_t bodyLength , uint8_t* body ){
		if(sendOutSysex ==0) return;
		uint8_t sysex[13];
		addCIHeader(0x35,sysex);
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
	
	//TODO Move to Private if not needed??
	void cleanupRequestId(uint8_t requestId){
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
    
	//-----------------------Handlers ---------------------------
	inline void setNoteOff(void (*fptr)(uint8_t group, uint8_t channel, uint8_t noteNumber, uint16_t velocity, uint8_t attributeType, uint16_t attributeData)){ midiNoteOff = fptr; }
	inline void setNoteOn(void (*fptr)(uint8_t group, uint8_t channel, uint8_t noteNumber, uint16_t velocity, uint8_t attributeType, uint16_t attributeData)){ midiNoteOn = fptr; }
	inline void setControlChange(void (*fptr)(uint8_t group, uint8_t channel, uint8_t index, uint32_t value)){ controlChange = fptr; }
	inline void setRPN(void (*fptr)(uint8_t group, uint8_t channel,uint8_t bank,  uint8_t index, uint32_t value)){ rpn = fptr; }
	inline void setPolyPressure(void (*fptr)(uint8_t group, uint8_t channel, uint8_t noteNumber, uint32_t pressure)){ polyPressure = fptr; }
	inline void setChannelPressure(void (*fptr)(uint8_t group, uint8_t channel, uint32_t pressure)){ channelPressure = fptr; }
	inline void setPitchBend(void (*fptr)(uint8_t group, uint8_t channel, uint32_t value)){ pitchBend = fptr; }
	inline void setProgramChange(void (*fptr)(uint8_t group, uint8_t channel, uint8_t program, bool bankValid, uint8_t bank, uint8_t index)){ programChange = fptr; }
	//TODO NRPNs, relative, per note etc
	
	inline void setTimingCode(void (*fptr)(uint8_t group,uint8_t timeCode)){ timingCode = fptr; }
	inline void setSongSelect(void (*fptr)(uint8_t group,uint8_t song)){ songSelect = fptr; }
	inline void setSongPositionPointer(void (*fptr)(uint8_t group,uint16_t positio)){ songPositionPointer = fptr; }
	inline void setTuneRequest(void (*fptr)(uint8_t group)){ tuneRequest = fptr; }
	inline void setTimingClock(void (*fptr)(uint8_t group)){ timingClock = fptr; }
	inline void setSeqStart(void (*fptr)(uint8_t group)){ seqStart = fptr; }
	inline void setSeqCont(void (*fptr)(uint8_t group)){ seqCont = fptr; }
	inline void setSeqStop(void (*fptr)(uint8_t group)){ seqStop = fptr; }
	inline void setActiveSense(void (*fptr)(uint8_t group)){ activeSense = fptr; }
	inline void setSystemReset(void (*fptr)(uint8_t group)){ systemReset = fptr; }
	
	
	inline void setRawSysEx(void (*fptr)(uint8_t group, uint8_t *sysex ,uint16_t length, uint8_t state)){ sendOutSysex = fptr; }
	inline void setRecvDiscovery(void (*fptr)(uint8_t group, uint32_t remoteMuid, uint8_t ciVer, uint8_t* manuId, uint8_t* famId, uint8_t* modelId, uint8_t *verId, uint8_t ciSupport, uint16_t maxSysex)){ recvDiscoveryRequest = fptr;}
	inline void setRecvNAK(void (*fptr)(uint8_t group, uint32_t remoteMuid)){ recvNAK = fptr;}
	inline void setRecvInvalidateMUID(void (*fptr)(uint8_t group, uint32_t remoteMuid, uint32_t terminateMuid)){ recvInvalidateMUID = fptr;}
	
	#ifdef M2_ENABLE_JR
	inline void setJrClock(void (*fptr)(uint8_t group,uint16_t timing)){ jrClock = fptr; }
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

