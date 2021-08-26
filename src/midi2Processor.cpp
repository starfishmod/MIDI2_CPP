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

#include "../include/midi2Processor.h"
#include "../include/utils.h"
 


#ifdef ARDUINO
    #include <stdlib.h>
#endif

midi2Processor::midi2Processor(uint8_t grStart, uint8_t totalGroups, uint8_t numRequestsTotal){
	groupStart = grStart;
	groups = totalGroups;
	
	sysexPos = (uint16_t*)malloc(sizeof(int) * groups); 
	sysexMode = (uint8_t*)malloc(sizeof(uint8_t) * groups); 
	sysUniNRTMode = (uint8_t*)malloc(sizeof(uint8_t) * groups); 
	sysUniPort = (uint8_t*)malloc(sizeof(uint8_t) * groups); 
	ciType = (uint8_t*)malloc(sizeof(uint8_t) * groups); 
	ciVer = (uint8_t*)malloc(sizeof(uint8_t) * groups); 
	remoteMUID = (uint32_t*)malloc(sizeof(uint32_t) *  groups); 
	destMuid = (uint32_t*)malloc(sizeof(uint32_t) *  groups); 
	
	#ifdef M2_ENABLE_PE
	numRequests = numRequestsTotal;
	peRquestDetails  = ( struct peHeader * )malloc(sizeof(peHeader) *  numRequestsTotal);
	
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
}

midi2Processor::~midi2Processor() { 
	free(sysexPos); sysexPos = NULL; 
	free(sysexMode); sysexMode = NULL; 
	free(sysUniNRTMode); sysUniNRTMode = NULL; 
	free(sysUniNRTMode); sysUniNRTMode = NULL; 
	free(ciVer); ciVer = NULL; 
	free(remoteMUID); remoteMUID = NULL; 
	free(destMuid); destMuid = NULL; 
	free(sys7CharBuffer); sys7CharBuffer = NULL; 
	free(sys7IntBuffer); sys7IntBuffer = NULL; 
	
	#ifdef M2_ENABLE_PE
	free(peRquestDetails); peRquestDetails = NULL;
	#endif
	
}


void midi2Processor::addCIHeader(uint8_t _ciType, uint8_t* sysexHeader, uint8_t _ciVer){

	sysexHeader[0]=S7UNIVERSAL_NRT;
	sysexHeader[1]=MIDI_PORT;
	sysexHeader[2]=S7MIDICI;
	sysexHeader[3]=_ciType;
	sysexHeader[4]=_ciVer;
	setBytesFromNumbers(sysexHeader, groupBlockMUID, 5, 4);
}


void midi2Processor::endSysex7(uint8_t groupOffset){
	sysex7State[groupOffset] = false;
	sysexPos[groupOffset] = 0;
}

void midi2Processor::startSysex7(uint8_t groupOffset){
	//Reset ALL SYSEX etc
	sysex7State[groupOffset] = true;
	sysexPos[groupOffset] = 0;
	sysexMode[groupOffset] = 0;
	sysUniNRTMode[groupOffset] = 0;
	sysUniPort[groupOffset] = -1;
	ciType[groupOffset] = 0;
	remoteMUID[groupOffset] = 0;
	destMuid[groupOffset] = 0;
	ciVer[groupOffset] = 0;
}

void midi2Processor::processSysEx(uint8_t groupOffset, uint8_t s7Byte){	
	if(sysexPos[groupOffset] == 0){
		if(s7Byte == S7UNIVERSAL_NRT || s7Byte == S7UNIVERSAL_RT){
			sysexMode[groupOffset] =  s7Byte;
			sysexPos[groupOffset]++;
			return;
		}
	}
	
	if(sysexMode[groupOffset] == S7UNIVERSAL_NRT){
		processUniS7NRT(groupOffset, s7Byte);
	}else{
	//TODO  
	}
	
	sysexPos[groupOffset]++;
}


//Proces all Non-Realtime Universal SysEx incl. MIDI-CI
void midi2Processor::processUniS7NRT(uint8_t groupOffset, uint8_t s7Byte){
	if(sysexPos[groupOffset] == 1){
		sysUniPort[groupOffset] =  s7Byte;
		return;
	}
	
	if(sysexPos[groupOffset] == 2){
		sysUniNRTMode[groupOffset] =  s7Byte;
		return;
	}
	
	switch(sysUniNRTMode[groupOffset]){
		
	#ifdef M2_ENABLE_IDREQ  
	case S7IDREQUEST:
		if(sysexPos[groupOffset] == 3 && s7Byte == 0x01){
		//Identity Request - send a reply?
		//Serial.println("  -Identity Request - send a reply");
		uint8_t sysex[]={
			S7UNIVERSAL_NRT,MIDI_PORT,S7IDREQUEST,0x02
			,devId[0],devId[1],devId[2] //SyexId
			,famId[0],famId[1] //family id
			,modelId[0],modelId[1] //model id
			,ver[0],ver[1],ver[2],ver[3] //version id
		};
		
		if(sendOutSysex !=0) sendOutSysex(groupOffset + groupStart, sysex,15,0);
		
		}
		if(sysexPos[groupOffset] == 3 && s7Byte == 0x02){
			//Identity Reply
			sys7CharBuffer[groupOffset][0] = s7Byte;
		
		}
		if(sys7CharBuffer[groupOffset][0] == 0x02){
			if(sysexPos[groupOffset] >= 4 && sysexPos[groupOffset] <= 14){
				sys7CharBuffer[groupOffset][sysexPos[groupOffset]-3] = s7Byte; 
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
	
	case S7MIDICI: // MIDI-CI
		
		if(sysexPos[groupOffset] == 3){ 
			ciType[groupOffset] =  s7Byte;
		}
		
		if(sysexPos[groupOffset] == 4){
		ciVer[groupOffset] =  s7Byte;
		} 
		if(sysexPos[groupOffset] >= 5 && sysexPos[groupOffset] <= 8){
			sys7CharBuffer[groupOffset][sysexPos[groupOffset]-5] = s7Byte;  
		}
		if(sysexPos[groupOffset] == 8){
			remoteMUID[groupOffset] =  sys7CharBuffer[groupOffset][0] + ((int)sys7CharBuffer[groupOffset][1] << 7) + ((sys7CharBuffer[groupOffset][2] + 0L) << 14) + ((sys7CharBuffer[groupOffset][3] + 0L) << 21);
		}
		
		if(sysexPos[groupOffset] >= 9 && sysexPos[groupOffset] <= 12){
			sys7CharBuffer[groupOffset][sysexPos[groupOffset]-9] = s7Byte;
		}
		if(sysexPos[groupOffset] == 12){
			destMuid[groupOffset] =  sys7CharBuffer[groupOffset][0] + ((int)sys7CharBuffer[groupOffset][1] << 7) + ((sys7CharBuffer[groupOffset][2] + 0L) << 14) + ((sys7CharBuffer[groupOffset][3] + 0L) << 21);
		}
		
		if(sysexPos[groupOffset] >= 12 && destMuid[groupOffset] != groupBlockMUID && destMuid[groupOffset] != M2_CI_BROADCAST){
			return; //Not for this device
		}
		
		//break up each Process based on ciType
		switch (ciType[groupOffset]){
			case MIDICI_DISCOVERY: //Discovery Request'
				//Serial.print("  - Discovery Request ");Serial.print(sysexPos[groupOffset]);Serial.print(" ");Serial.println(s7Byte,HEX);
				if(sysexPos[groupOffset] >= 13 && sysexPos[groupOffset] <= 28){
					sys7CharBuffer[groupOffset][sysexPos[groupOffset]-13] = s7Byte; 
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
							remoteMUID[groupOffset],
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
					addCIHeader(MIDICI_DISCOVERYREPLY,sysex,0x01);
					setBytesFromNumbers(sysex, remoteMUID[groupOffset], 9, 4);
					
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
			case MIDICI_DISCOVERYREPLY: //Discovery Reply'
				if(sysexPos[groupOffset] >= 13 && sysexPos[groupOffset] <= 28){
					sys7CharBuffer[groupOffset][sysexPos[groupOffset]-13] = s7Byte; 
				}
				if(sysexPos[groupOffset]==28){
					if (recvDiscoveryReply != 0){
						uint8_t manuIdR[3] = {sys7CharBuffer[groupOffset][0], sys7CharBuffer[groupOffset][1], sys7CharBuffer[groupOffset][2]};
						uint8_t famIdR[2] = {sys7CharBuffer[groupOffset][3], sys7CharBuffer[groupOffset][4]};
						uint8_t modelIdR[2] = {sys7CharBuffer[groupOffset][5], sys7CharBuffer[groupOffset][6]};
						uint8_t verR[4] = {sys7CharBuffer[groupOffset][7], sys7CharBuffer[groupOffset][8], sys7CharBuffer[groupOffset][9], sys7CharBuffer[groupOffset][10]};
						recvDiscoveryReply(
							groupOffset + groupStart,
							remoteMUID[groupOffset],
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
				
			case MIDICI_INVALIDATEMUID: //MIDI-CI Invalidate MUID Message
				if(sysexPos[groupOffset] == 13){
					destMuid[groupOffset] = 0;
				}
			
				if(sysexPos[groupOffset] >= 13 && sysexPos[groupOffset] <= 16){
					destMuid[groupOffset] =  destMuid[groupOffset] + (s7Byte << (7 * (sysexPos[groupOffset] - 13)));
				}
			
				if (sysexPos[groupOffset] == 16 && recvInvalidateMUID != 0){
					recvInvalidateMUID(groupOffset + groupStart,remoteMUID[groupOffset], destMuid[groupOffset]);
				}
				break;	
			case MIDICI_NAK: //MIDI-CI NAK
				if (recvNAK != 0){
					recvNAK(groupOffset + groupStart,remoteMUID[groupOffset]);
				}
				break;
				
			#ifdef M2_ENABLE_PROTOCOL
			#endif     
			
			#ifdef M2_ENABLE_PROFILE  
			case 0x20: //Profile Inquiry
			case 0x21: //Reply to Profile Inquiry
			case 0x22: //Set Profile On Message
			case 0x23: //Set Profile Off Message	
			case 0x24: //Set Profile Enabled Message  
			case 0x25: //Set Profile Disabled Message
				processProfileSysex(groupOffset, s7Byte);
				break;   
			#endif   
			
			
			#ifdef M2_ENABLE_PE 
			case 0x30: //Inquiry: Property Exchange Capabilities
			case 0x31: //Reply to Property Exchange Capabilities			
			case 0x34:  // Inquiry: Get Property Data
			case 0x35: // Reply To Get Property Data - Needs Work!
			case 0x36: // Inquiry: Set Property Data
				processPESysex(groupOffset, s7Byte);
				break;
			#endif   
		
		}
		
		break;
	
	
	}
}




void midi2Processor::processUMP(uint32_t UMP){
	umpMess[messPos] = UMP;
	//Serial.print(" UMP Proc: ");Serial.print(messPos);Serial.print("  ");Serial.println(umpMess[messPos]);

	
	uint8_t mt = umpMess[0] >> 28  & 0xF;
	uint8_t group = umpMess[0] >> 24 & 0xF;
	uint8_t groupOffset = group - groupStart;  

	
	if(messPos == 0 && mt <= UMP_M1CVM){ //32bit Messages
		
		if(group < groupStart || group > groupStart + groups -1){
			//Not for this Group Block
			//Serial.println("  Not for this Group Block");
					
		}else			
		if(mt == UMP_UTILITY){ //32 bits Utility Messages
			// TODO Break up into JR TimeStamp Messages offsets
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
		if(mt == UMP_SYSTEM){ //32 bits System Real Time and System Common Messages (except System Exclusive)
			//Send notice
			uint8_t status = umpMess[0] >> 16 & 0xFF;
			switch(status){
				case TIMING_CODE:
				{
					uint8_t timing = (umpMess[0] >> 8) & 0x7F;
					if(timingCode != 0) timingCode(group, timing); 
				}
				break;
				case SPP:
				{
					uint16_t position = ((umpMess[0] >> 8) & 0x7F)  + ((umpMess[0] & 0x7F) << 7);
					if(songPositionPointer != 0) songPositionPointer(group, position);
				}
				break;
				case SONG_SELECT:
				{
					uint8_t song = (umpMess[0] >> 8) & 0x7F;
					if(songSelect != 0) songSelect(group, song); 
				}
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
		if(mt == UMP_M1CVM){ //32 Bits MIDI 1.0 Channel Voice Messages
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
				case KEY_PRESSURE: //Poly Pressure
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
	if(messPos == 1 && mt <= UMP_M2CVM){ //64bit Messages
		if(group < groupStart || group > groupStart + groups -1){
			//Not for this Group Block
			//Serial.println("  Not for this Group Block");
					
		}else			
		if(mt == UMP_SYSEX7){ //64 bits Data Messages (including System Exclusive)
			
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
		if(mt == UMP_M2CVM){//64 bits MIDI 2.0 Channel Voice Messages
		
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
					//if(rpn != 0) rpn(group, channel, val1, val2, umpMess[1]); 
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

void midi2Processor::sendDiscoveryRequest(uint8_t group, uint8_t ciVersion){
	if(sendOutSysex ==0) return;
	
	uint8_t sysex[13];
	addCIHeader(MIDICI_DISCOVERY,sysex,ciVersion);
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

void midi2Processor::sendNAK(uint8_t group, uint32_t _remoteMuid, uint8_t ciVersion){
	if(sendOutSysex ==0) return;
	uint8_t sysex[13];
	addCIHeader(MIDICI_NAK,sysex,ciVersion);
	setBytesFromNumbers(sysex, _remoteMuid, 9, 4);
	sendOutSysex(group,sysex,13,0);
}

void midi2Processor::setInvalidateMUID(uint8_t group, uint32_t _Muid, uint8_t ciVersion){
	if(sendOutSysex ==0) return;
	uint8_t sysex[13];
	addCIHeader(MIDICI_INVALIDATEMUID,sysex,ciVersion);
	setBytesFromNumbers(sysex, M2_CI_BROADCAST, 9, 4);
	sendOutSysex(group,sysex,13,1);
	setBytesFromNumbers(sysex, _Muid, 0, 4);
	sendOutSysex(group,sysex,4,3);
}



#ifdef M2_ENABLE_IDREQ
void midi2Processor::sendIdentityRequest (uint8_t group){
	if(sendOutSysex ==0) return;
	uint8_t sysex[]={S7UNIVERSAL_NRT,MIDI_PORT,S7IDREQUEST,0x01};
	sendOutSysex(group,sysex,4,0);
}

#endif





