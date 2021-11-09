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
 
#include <cstdlib>


midi2Processor::midi2Processor(uint8_t numRequestsTotal){

	
	#ifndef M2_DISABLE_PE
	numRequests = numRequestsTotal;
	peRquestDetails  = ( struct peHeader * )malloc(sizeof(peHeader) *  numRequestsTotal);
	
	for(uint8_t i =0;i<numRequests;i++){
        cleanupRequest(i);
	}
	#endif
	
	
	/*sys7CharBuffer = (uint8_t**)malloc(sizeof(uint8_t*) * groups);
	for(uint8_t i=0; i< groups; i++){
		*(sys7CharBuffer + i) = (uint8_t*)malloc(sizeof(uint8_t*) * 
		#ifndef M2_DISABLE_PE
		PE_HEAD_BUFFERLEN
		#else
		20
		#endif
		);
	}
	
	sys7IntBuffer = (uint16_t**)malloc(sizeof(uint16_t*) * groups);
	for(uint8_t i=0; i< groups; i++){
		*(sys7IntBuffer + i) = (uint16_t*)malloc(sizeof(uint16_t) * 
		#ifndef M2_DISABLE_PE
		5
		#else
		2
		#endif
		);
	}*/
	
	//debug("MIDICI Inst loaded");
}

midi2Processor::~midi2Processor() {
	#ifndef M2_DISABLE_PE
	free(peRquestDetails); peRquestDetails = nullptr;
	#endif
	
}

void midi2Processor::addCIHeader(uint8_t _ciType, uint8_t* sysexHeader, uint8_t _ciVer){

	sysexHeader[0]=S7UNIVERSAL_NRT;
	sysexHeader[1]=MIDI_PORT;
	sysexHeader[2]=S7MIDICI;
	sysexHeader[3]=_ciType;
	sysexHeader[4]=_ciVer;
	setBytesFromNumbers(sysexHeader, m2procMUID, 5, 4);
}

void midi2Processor::endSysex7(uint8_t group){
    //syExMess[group]._state = false;
    //debug("endSysex7");
    syExMess[group]._pos = 0;
    syExMess[group].realtime = 0;
    syExMess[group].universalId = 0;
    syExMess[group].deviceId = 255;
    syExMess[group].ciType = 255;
    syExMess[group].ciVer = 255;
    syExMess[group].remoteMUID = 0;
    syExMess[group].destMuid = 0;
    syExMess[group].reqPosUsed = 255;
    memset(syExMess[group].buffer1, 0, sizeof(syExMess[group].buffer1));
    memset(syExMess[group].intbuffer1, 0, sizeof(syExMess[group].intbuffer1));
}

void midi2Processor::startSysex7(uint8_t group){
	//Reset ALL SYSEX etc
    //syExMess[group]._state = false;
    syExMess[group]._pos = 0;
}

void midi2Processor::processSysEx(uint8_t group, uint8_t s7Byte){
	if(syExMess[group]._pos == 0){
		if(s7Byte == S7UNIVERSAL_NRT || s7Byte == S7UNIVERSAL_RT){
            syExMess[group].realtime =  s7Byte;
			syExMess[group]._pos++;
			return;
		}
	}
	
	if(syExMess[group].realtime == S7UNIVERSAL_NRT || syExMess[group].realtime == S7UNIVERSAL_RT){
		if(syExMess[group]._pos == 1){
			syExMess[group].deviceId =  s7Byte;
			syExMess[group]._pos++;
			return;
		}
		
		if(syExMess[group]._pos == 2){
			syExMess[group].universalId =  s7Byte;
			syExMess[group]._pos++;
			return;
		}
	}
	
	if(syExMess[group].realtime == S7UNIVERSAL_NRT){
		switch(syExMess[group].universalId){
			/*
			case 0x00: // Sample DUMP
			case 0x01:
			case 0x02:
			case 0x03:
			case 0x05: // Sample Dump Extensions
				break;
			case 0x04: // MIDI Time Code
				break;
			*/
			#ifndef M2_DISABLE_IDREQ  
			case S7IDREQUEST:
				processDeviceID(group, s7Byte);
				break;
			#endif
			/*
			case 0x07 : // File Dump
				break;
			case 0x08 : // MIDI Tuning Standard (Non-Real Time)
				break;	
			case 0x09 : // General MIDI
				break;
			case 0x0A : // Downloadable Sounds
				break;
			case 0x0B : // File Reference Message
				break;
			case 0x0C : // MIDI Visual Control
				break;	
			case 0x7B : // End of File
				break;	
			case 0x7C : // Wait
				break;			
			case 0x7D : // Cancel
				break;	
			case 0x7E : // NAK
				break;
			case 0x7F : // ACK
				break;
			*/
			case S7MIDICI: // MIDI-CI
				processMIDICI(group, s7Byte);
				break;
			
		}
	}else if(syExMess[group].realtime == S7UNIVERSAL_RT) {
		//This block of code represents potential future Universal SysEx Work
		/*switch(syExMess[group].universalId){
			
			case 0x01: // MIDI Time Code
				break
			case 0x02: // MIDI Show Control
				break;
			case 0x03: // Notation Information
				break;
			case 0x04: // Device Control
				break;	
			case 0x05: // Real Time MTC Cueing
				break;
			case 0x06: // MIDI Machine Control Commands
				break;
			case 0x07 : // MIDI Machine Control Responses
				break;
			case 0x08 : // MIDI Tuning Standard (Real Time)
				break;	
			case 0x09 : // Controller Destination Setting (See GM2 Documentation)
				break;
			case 0x0A : // Key-based Instrument Control
				break;	
			case 0x0B : // Scalable Polyphony MIDI MIP Message
				break;
			case 0x0C : // Mobile Phone Control Message
				break;	
					
		}*/
	}else {
		//TODO send out custom SysEx 7 byte
	}
	syExMess[group]._pos++;
}

void midi2Processor::processMIDICI(uint8_t group, uint8_t s7Byte){
	if(syExMess[group]._pos == 3){
		syExMess[group].ciType =  s7Byte;
	}
	
	if(syExMess[group]._pos == 4){
	    syExMess[group].ciVer =  s7Byte;
	} 
	if(syExMess[group]._pos >= 5 && syExMess[group]._pos <= 8){
        syExMess[group].remoteMUID += s7Byte << (7 * (syExMess[group]._pos - 5));
	}
	
	if(syExMess[group]._pos >= 9 && syExMess[group]._pos <= 12){
        syExMess[group].destMuid += s7Byte << (7 * (syExMess[group]._pos - 9));
	}
	
	if(syExMess[group]._pos >= 12 && syExMess[group].destMuid != m2procMUID && syExMess[group].destMuid != M2_CI_BROADCAST){
		//Serial.println("  -> Not for this device ");
		return; //Not for this device
	}
	
	//break up each Process based on ciType
	switch (syExMess[group].ciType){
		case MIDICI_DISCOVERY: //Discovery Request'
			//debug("  - Discovery Request ");//debug(syExMess[group]._pos);debug(s7Byte);
			if(syExMess[group]._pos >= 13 && syExMess[group]._pos <= 23){
				syExMess[group].buffer1[syExMess[group]._pos-13] = s7Byte;
			}
            if(syExMess[group]._pos == 24){
                syExMess[group].intbuffer1[0] = s7Byte; // ciSupport
            }
            if(syExMess[group]._pos >= 25 && syExMess[group]._pos <= 28){
                syExMess[group].intbuffer1[1] += s7Byte << (7 * (syExMess[group]._pos - 25)); //maxSysEx
            }
			if(syExMess[group]._pos==28){
				//debug("  - Discovery Request 28 ");
				if (recvDiscoveryRequest != nullptr){
					uint8_t manuIdR[3] = {syExMess[group].buffer1[0], syExMess[group].buffer1[1], syExMess[group].buffer1[2]};
					uint8_t famIdR[2] = {syExMess[group].buffer1[3], syExMess[group].buffer1[4]};
					uint8_t modelIdR[2] = {syExMess[group].buffer1[5], syExMess[group].buffer1[6]};
					uint8_t verR[4] = {syExMess[group].buffer1[7], syExMess[group].buffer1[8], syExMess[group].buffer1[9], syExMess[group].buffer1[10]};
					recvDiscoveryRequest(
						group,
						syExMess[group].remoteMUID,
						syExMess[group].ciVer,
						manuIdR,
						famIdR,
						modelIdR,
						verR,
                        syExMess[group].intbuffer1[0],
                        syExMess[group].intbuffer1[1]
					);
				}
				
				//Send Discovery Reply
				//debug("  -> Discovery Reply ");
				if(sendOutSysex == nullptr) return;
				//debug("  -> Discovery Reply 2");
				
				uint8_t sysex[13];
				addCIHeader(MIDICI_DISCOVERYREPLY,sysex,0x01);
				setBytesFromNumbers(sysex, syExMess[group].remoteMUID, 9, 4);
				
				sendOutSysex(group,sysex,13,1);
				sendOutSysex(group,sysexId,3,2);
				sendOutSysex(group,famId,2,2);
				sendOutSysex(group,modelId,2,2);
				sendOutSysex(group,ver,4,2);
				
				//Capabilities
				sysex[0]=ciSupport; 					
				setBytesFromNumbers(sysex, sysExMax, 1, 4);
				sendOutSysex(group,sysex,5,3);
				
			}

			break;
		case MIDICI_DISCOVERYREPLY: //Discovery Reply'
            if(syExMess[group]._pos >= 13 && syExMess[group]._pos <= 23){
                syExMess[group].buffer1[syExMess[group]._pos-13] = s7Byte;
            }
            if(syExMess[group]._pos == 24){
                syExMess[group].intbuffer1[0] = s7Byte; // ciSupport
            }
            if(syExMess[group]._pos >= 25 && syExMess[group]._pos <= 28){
                syExMess[group].intbuffer1[1] += s7Byte << (7 * (syExMess[group]._pos - 25)); //maxSysEx
            }
			if(syExMess[group]._pos==28){
				if (recvDiscoveryReply != nullptr){
					uint8_t manuIdR[3] = {syExMess[group].buffer1[0], syExMess[group].buffer1[1], syExMess[group].buffer1[2]};
					uint8_t famIdR[2] = {syExMess[group].buffer1[3], syExMess[group].buffer1[4]};
					uint8_t modelIdR[2] = {syExMess[group].buffer1[5], syExMess[group].buffer1[6]};
					uint8_t verR[4] = {syExMess[group].buffer1[7], syExMess[group].buffer1[8], syExMess[group].buffer1[9], syExMess[group].buffer1[10]};
					recvDiscoveryReply(
						group,
						syExMess[group].remoteMUID,
						syExMess[group].ciVer,
						manuIdR,
						famIdR,
						modelIdR,
						verR,
                        syExMess[group].intbuffer1[0],
                        syExMess[group].intbuffer1[1]
                        );
				}
			}
			break;
			
		case MIDICI_INVALIDATEMUID: //MIDI-CI Invalidate MUID Message
			if(syExMess[group]._pos == 13){
				syExMess[group].destMuid = 0;
			}
		
			if(syExMess[group]._pos >= 13 && syExMess[group]._pos <= 16){
				syExMess[group].destMuid += s7Byte << (7 * (syExMess[group]._pos - 13));
			}
		
			if (syExMess[group]._pos == 16 && recvInvalidateMUID != nullptr){
				recvInvalidateMUID(group,syExMess[group].remoteMUID, syExMess[group].destMuid);
			}
			break;	
		case MIDICI_NAK: //MIDI-CI NAK
			if (recvNAK != nullptr){
				recvNAK(group,syExMess[group].remoteMUID);
			}
			break;
			
		#ifndef M2_DISABLE_PROTOCOL
		#endif     
		
		#ifndef M2_DISABLE_PROFILE  
		case 0x20: //Profile Inquiry
		case 0x21: //Reply to Profile Inquiry
		case 0x22: //Set Profile On Message
		case 0x23: //Set Profile Off Message	
		case 0x24: //Set Profile Enabled Message  
		case 0x25: //Set Profile Disabled Message
		case 0x2F: //ProfileSpecific Data
			processProfileSysex(group, s7Byte);
			break;   
		#endif   
		
		
		#ifndef M2_DISABLE_PE 
		case 0x30: //Inquiry: Property Exchange Capabilities
		case 0x31: //Reply to Property Exchange Capabilities			
		case 0x34:  // Inquiry: Get Property Data
		case 0x35: // Reply To Get Property Data - Needs Work!
		case 0x36: // Inquiry: Set Property Data
		case 0x37: // Reply To Inquiry: Set Property Data
		case 0x38: // Inquiry: Subscribe Property Data
		case 0x39: // Reply To Subscribe Property Data
		case 0x3F: // Notify
			processPESysex(group, s7Byte);
			break;
		#endif   
	
	}
}

void midi2Processor::processUMP(uint32_t UMP){
	umpMess[messPos] = UMP;
		
	uint8_t mt = (umpMess[0] >> 28)  & 0xF;
	uint8_t group = (umpMess[0] >> 24) & 0xF;

	
	if(messPos == 0 && mt <= UMP_M1CVM){ //32bit Messages

		if(mt == UMP_UTILITY){ //32 bits Utility Messages
			// TODO Break up into JR TimeStamp Messages offsets
			uint8_t status = (umpMess[0] >> 20) & 0xF;
			
			#ifndef M2_DISABLE_JR
			uint16_t timing = (umpMess[0] >> 16) & 0xFFFF;
			#endif
			
			switch(status){
				case 0: // NOOP 
				//if(group== 0 && noop != 0) noop();
				break;
			#ifndef M2_DISABLE_JR	
				case 0b1: // JR Clock Message 
					if(recvJRClock != nullptr) recvJRClock(group, timing);
					break;
				case 0b10: //JR Timestamp Message
					//??? Message out or attach to next message?
					if(recvJRTimeStamp != nullptr) recvJRTimeStamp(group, timing);
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
					if(timingCode != nullptr) timingCode(group, timing);
				}
				break;
				case SPP:
				{
					uint16_t position = ((umpMess[0] >> 8) & 0x7F)  + ((umpMess[0] & 0x7F) << 7);
					if(songPositionPointer != nullptr) songPositionPointer(group, position);
				}
				break;
				case SONG_SELECT:
				{
					uint8_t song = (umpMess[0] >> 8) & 0x7F;
					if(songSelect != nullptr) songSelect(group, song);
				}
				break;
				case TUNEREQUEST:
				if(tuneRequest != nullptr) tuneRequest(group);
				break;
				case TIMINGCLOCK:
				if(timingClock != nullptr) timingClock(group);
				break;
				case SEQSTART:
				if(seqStart != nullptr) seqStart(group);
				break;
				case SEQCONT:
				if(seqCont != nullptr) seqCont(group);
				break;
				case SEQSTOP:
				if(seqStop != nullptr) seqStop(group);
				break;
				case ACTIVESENSE:
				if(activeSense != nullptr) activeSense(group);
				break;
				case SYSTEMRESET:
				if(systemReset != nullptr) systemReset(group);
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
					if(midiNoteOff != nullptr) midiNoteOff(group, channel, val1, scaleUp(val2,7,16), 0, 0);
					break;
				case NOTE_ON: //Note On
					if(midiNoteOn != nullptr) midiNoteOn(group, channel, val1, scaleUp(val2,7,16), 0, 0);
					break;
				case KEY_PRESSURE: //Poly Pressure
					if(polyPressure != nullptr) polyPressure(group, channel, val1, scaleUp(val2,7,32));
					break;	
				case CC: //CC
					if(controlChange != nullptr) controlChange(group, channel, val1, scaleUp(val2,7,32));
					break;
				case PROGRAM_CHANGE: //Program Change Message
					if(programChange != nullptr) programChange(group, channel, val1, false, 0, 0);
					break;
				case CHANNEL_PRESSURE: //Channel Pressure
					if(channelPressure != nullptr) channelPressure(group, channel, scaleUp(val1,7,32));
					break;
				case PITCH_BEND: //PitchBend
					if(pitchBend != nullptr) pitchBend(group, channel, scaleUp((val2 << 7) + val1,14,32));
					break;		
			}				
		}
        return;
		
	}else		
	if(messPos == 1 && mt <= UMP_M2CVM){ //64bit Messages
		if(mt == UMP_SYSEX7){ //64 bits Data Messages (including System Exclusive)
			
			uint8_t numbytes  = (umpMess[0] >> 16) & 0xF;
			uint8_t status = (umpMess[0] >> 20) & 0xF;
			if(status == 0 || status == 1){
				startSysex7(group);
			}
			
			if(numbytes > 0)processSysEx(group, (umpMess[0] >> 8) & 0x7F);
			if(numbytes > 1)processSysEx(group, umpMess[0] & 0x7F);
			if(numbytes > 2)processSysEx(group, (umpMess[1] >> 24) & 0x7F);
			if(numbytes > 3)processSysEx(group, (umpMess[1] >> 16) & 0x7F);
			if(numbytes > 4)processSysEx(group, (umpMess[1] >> 8) & 0x7F);
			if(numbytes > 5)processSysEx(group, umpMess[1] & 0x7F);
			
			if(status == 0 || status == 3){
				endSysex7(group);
			}
		} else 
		if(mt == UMP_M2CVM){//64 bits MIDI 2.0 Channel Voice Messages
		
			uint8_t status = (umpMess[0] >> 16) & 0xF0;
			uint8_t channel = (umpMess[0] >> 16) & 0xF;
			uint8_t val1 = (umpMess[0] >> 8) & 0xFF;
			uint8_t val2 = umpMess[0] & 0xFF;
			
			switch(status){
				case NOTE_OFF: //Note Off
					if(midiNoteOff != nullptr) midiNoteOff(group, channel, val1, umpMess[1] >> 16, val2, umpMess[1] & 65535);
					break;
				
				case NOTE_ON: //Note On
					if(midiNoteOn != nullptr) midiNoteOn(group, channel, val1, umpMess[1] >> 16, val2, umpMess[1] & 65535);
					break;
					
				case KEY_PRESSURE: //Poly Pressure
					if(polyPressure != nullptr) polyPressure(group, channel, val1, umpMess[1]);
					break;	
				
				case CC: //CC
					if(controlChange != nullptr) controlChange(group, channel, val1, umpMess[1]);
					break;	
				
				case RPN: //RPN
					if(rpn != nullptr) rpn(group, channel, val1, val2, umpMess[1]);
					break;	
				
				case NRPN: //NRPN
					if(nrpn != nullptr) nrpn(group, channel, val1, val2, umpMess[1]);
					break;	
				
				case RPN_RELATIVE: //Relative RPN
					if(rrpn != nullptr) rrpn(group, channel, val1, val2, (int32_t)umpMess[1]/*twoscomplement*/);
					break;	
				case NRPN_RELATIVE: //Relative NRPN
					if(rnrpn != nullptr) rnrpn(group, channel, val1, val2, (int32_t)umpMess[1]/*twoscomplement*/);
					break;
				
				case PROGRAM_CHANGE: //Program Change Message
					if(programChange != nullptr) programChange(group, channel, umpMess[1] >> 24, umpMess[0] & 1 , (umpMess[1] >> 8) & 0x7f , umpMess[1] & 0x7f);
					break;
				case CHANNEL_PRESSURE: //Channel Pressure
					if(channelPressure != nullptr) channelPressure(group, channel, umpMess[1]);
					break;
				case PITCH_BEND: //PitchBend
					if(pitchBend != nullptr) pitchBend(group, channel, umpMess[1]);
					break;	
					
				case PITCH_BEND_PERNOTE: //Per Note PitchBend 6
					//if(midiNoteOn != 0) channelPressure(group, channel, val1, umpMess[1]); 
					break;		
				case NRPN_PERNOTE: //Assignable Per-Note Controller 1
					//if(midiNoteOn != 0) channelPressure(group, channel, val1, umpMess[1]); 
					break;	
					
				case RPN_PERNOTE: //Registered Per-Note Controller 0 
					//if(midiNoteOn != 0) channelPressure(group, channel, val1, umpMess[1]); 
					break;	
					
				case PERNOTE_MANAGE: //Per-Note Management Message
					//if(midiNoteOn != 0) channelPressure(group, channel, val1, umpMess[1]); 
					break;	
					
			}
		}
        messPos =0;
	}else		
	if(messPos == 3 && mt <= 0x05){ //128bit Messages
		messPos =0;
	} else {
		messPos++;
	}
	
}

void midi2Processor::sendDiscoveryRequest(uint8_t group, uint8_t ciVersion){
	if(sendOutSysex == nullptr) return;
	
	uint8_t sysex[13];
	addCIHeader(MIDICI_DISCOVERY,sysex,ciVersion);
	setBytesFromNumbers(sysex, M2_CI_BROADCAST, 9, 4);
	sendOutSysex(group,sysex,13,1);
	sendOutSysex(group,sysexId,3,2);
	sendOutSysex(group,famId,2,2);
	sendOutSysex(group,modelId,2,2);
	sendOutSysex(group,ver,4,2);
	
	//Capabilities
	sysex[0]=ciSupport;
	setBytesFromNumbers(sysex, sysExMax, 1, 4);
	sendOutSysex(group,sysex,5,3);
}

void midi2Processor::sendNAK(uint8_t group, uint32_t _remoteMuid, uint8_t ciVersion){
	if(sendOutSysex == nullptr) return;
	uint8_t sysex[13];
	addCIHeader(MIDICI_NAK,sysex,ciVersion);
	setBytesFromNumbers(sysex, _remoteMuid, 9, 4);
	sendOutSysex(group,sysex,13,0);
}

void midi2Processor::sendInvalidateMUID(uint8_t group, uint32_t terminateMuid, uint8_t ciVersion){
	if(sendOutSysex == nullptr) return;
	uint8_t sysex[13];
	addCIHeader(MIDICI_INVALIDATEMUID,sysex,ciVersion);
	setBytesFromNumbers(sysex, M2_CI_BROADCAST, 9, 4);
	sendOutSysex(group,sysex,13,1);
	setBytesFromNumbers(sysex, terminateMuid, 0, 4);
	sendOutSysex(group,sysex,4,3);
}


#ifndef M2_DISABLE_IDREQ
void midi2Processor::sendIdentityRequest (uint8_t group){
	if(sendOutSysex == nullptr) return;
	uint8_t sysex[]={S7UNIVERSAL_NRT,MIDI_PORT,S7IDREQUEST,0x01};
	sendOutSysex(group,sysex,4,0);
}
void midi2Processor::processDeviceID(uint8_t group, uint8_t s7Byte){
	if(syExMess[group]._pos == 3 && s7Byte == 0x01){
		//Identity Request - send a reply?
		//Serial.println("  -Identity Request - send a reply");
		uint8_t sysex[]={
			S7UNIVERSAL_NRT,MIDI_PORT,S7IDREQUEST,0x02
			,sysexId[0],sysexId[1],sysexId[2] //SyexId
			,famId[0],famId[1] //family id
			,modelId[0],modelId[1] //model id
			,ver[0],ver[1],ver[2],ver[3] //version id
		};
		
		if(sendOutSysex != nullptr) sendOutSysex(group, sysex,15,0);
		
	}
	if(syExMess[group]._pos == 3 && s7Byte == 0x02){
		//Identity Reply
		syExMess[group].buffer1[0] = s7Byte;
	}
	if(syExMess[group].buffer1[0] == 0x02){
		if(syExMess[group]._pos >= 4 && syExMess[group]._pos <= 14){
			syExMess[group].buffer1[syExMess[group]._pos-3] = s7Byte;
		}

		if (syExMess[group]._pos == 14 && sendOutIdResponse != nullptr){
			uint8_t manuIdR[3] = {syExMess[group].buffer1[1], syExMess[group].buffer1[2], syExMess[group].buffer1[3]};
			uint8_t famIdR[2] = {syExMess[group].buffer1[4], syExMess[group].buffer1[5]};
			uint8_t modelIdR[2] = {syExMess[group].buffer1[6], syExMess[group].buffer1[7]};
			uint8_t verR[4] = {syExMess[group].buffer1[8], syExMess[group].buffer1[9], syExMess[group].buffer1[10], syExMess[group].buffer1[11]};
			sendOutIdResponse(manuIdR, famIdR, modelIdR, verR);
		}
	}
}
#endif





