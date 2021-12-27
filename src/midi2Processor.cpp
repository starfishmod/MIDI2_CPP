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
}

midi2Processor::~midi2Processor() {
	#ifndef M2_DISABLE_PE
	free(peRquestDetails); peRquestDetails = nullptr;
	#endif
}

void midi2Processor::createCIHeader(uint8_t* sysexHeader, MIDICI midiCiHeader){
	sysexHeader[0]=S7UNIVERSAL_NRT;
	sysexHeader[1]=midiCiHeader.deviceId;//MIDI_PORT;
	sysexHeader[2]=S7MIDICI;
	sysexHeader[3]=midiCiHeader.ciType;
	sysexHeader[4]=midiCiHeader.ciVer;
	setBytesFromNumbers(sysexHeader, midiCiHeader.localMUID, 5, 4);
	setBytesFromNumbers(sysexHeader, midiCiHeader.remoteMUID, 9, 4);
}

void midi2Processor::endSysex7(uint8_t group){
    syExMessInt[group].pos = 0;
    syExMessInt[group].realtime = 0;
    syExMessInt[group].universalId = 0;
    midici[group].deviceId = 255;
    midici[group].ciType = 255;
    midici[group].ciVer = 255;
    midici[group].remoteMUID = 0;
    midici[group].localMUID = 0;
    syExMessInt[group].peRequestIdx = 255;
    memset(syExMessInt[group].buffer1, 0, sizeof(syExMessInt[group].buffer1));
    memset(syExMessInt[group].intbuffer1, 0, sizeof(syExMessInt[group].intbuffer1));
}

void midi2Processor::startSysex7(uint8_t group){
    syExMessInt[group].pos = 0;
}

void midi2Processor::processSysEx(uint8_t group, uint8_t s7Byte){
	if(syExMessInt[group].pos == 0){
		if(s7Byte == S7UNIVERSAL_NRT || s7Byte == S7UNIVERSAL_RT){
            syExMessInt[group].realtime =  s7Byte;
			syExMessInt[group].pos++;
			return;
		}
	}
	
	if(syExMessInt[group].realtime == S7UNIVERSAL_NRT || syExMessInt[group].realtime == S7UNIVERSAL_RT){
		if(syExMessInt[group].pos == 1){
			midici[group].deviceId =  s7Byte;
			syExMessInt[group].pos++;
			return;
		}
		
		if(syExMessInt[group].pos == 2){
            syExMessInt[group].universalId =  s7Byte;
			syExMessInt[group].pos++;
			return;
		}
	}
	
	if(syExMessInt[group].realtime == S7UNIVERSAL_NRT){
		switch(syExMessInt[group].universalId){
			/*
			case 0x00: // Sample DUMP
			case 0x01:
			case 0x02:
			case 0x03:
			case 0x05: // Sample Dump Extensions
				break;
			 */

            #ifdef M2_ENABLE_MTC
            case 0x04: // MIDI Time Code
			    //MIDI Cueing Set-Up Messages - F0 7E <device ID> 04 <sub-ID 2> hr mn sc fr ff sl sm <add. info.> F7
				break;
            #endif

			#ifndef M2_DISABLE_IDREQ  
			case S7IDREQUEST:
				processDeviceID(group, s7Byte);
				break;
            #endif

			/*
			case 0x07 : // File Dump
				break;
			 */

            #if defined(M2_ENABLE_MIDI_TUNING_STANDARD) || defined(M2_ENABLE_GM2)
			case 0x08 : // MIDI Tuning Standard (Non-Real Time)
			    //BULK TUNING DUMP REQUEST - F0 7E <device ID> 08 00 <tt- tuning program number (0 – 127)> F7
			    //BULK TUNING DUMP - F0 7E <device ID> 08 01 tt <tuning name(16)> [xx yy zz](128) chksum F7
			    //BULK TUNING DUMP REQUEST (BANK) - F0 7E 08 03 <bb - bank: 0-127> tt F7
			    //KEY-BASED TUNING DUMP - F0 7E <device ID> 08 04 bb tt <tuning name>(16) [xx yy zz](128) chksum F7
			    //SINGLE NOTE TUNING CHANGE (BANK) F0 7E <device ID> 08 07 bb tt ll
                    //[<MIDI key number> <frequency data for that key>(3)](ll) F7
                //SCALE/OCTAVE TUNING DUMP, 1 byte format - F0 7E <device ID> 08 05 bb tt <tuning name>(16) [xx](12) chksum F7
                //SCALE/OCTAVE TUNING DUMP, 2 byte format - F0 7E <device ID> 08 06 bb tt <tuning name>(16) [xx yy](12) chksum F7
                //SCALE/OCTAVE TUNING 1-BYTE FORM - F0 7E <device ID> 08 08 <ff - channel/options>
                    //<gg -channel byte 2> <hh -channel byte 3> [ss](12)  F7
                ///SCALE/OCTAVE TUNING 2-BYTE FORM - F0 7E <device ID> 08 09 ff gg hh [ss tt](12) F7
				break;
            #endif

            #if defined(M2_ENABLE_GM1) || defined(M2_ENABLE_GM2)
			case 0x09 : // General MIDI
                // GM2 System On - F0 7E <device ID> 09 03 F7
                // GM1 System On - F0 7E <device ID> 09 01 F7
                // GM System Off - F0 7E <device ID> 09 02 F7
				break;
            #endif

			/*case 0x0A : // Downloadable Sounds
				break;
			case 0x0B : // File Reference Message
				break;
			case 0x0C : // MIDI Visual Control
			    //FOH 7EH Dev OCH 01H {. . .} F7H
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
	}else if(syExMessInt[group].realtime == S7UNIVERSAL_RT) {
		//This block of code represents potential future Universal SysEx Work
		switch(syExMessInt[group].universalId){
            #ifdef M2_ENABLE_MTC
			case 0x01: // MIDI Time Code
		        //Full Message - F0 7F <device ID> 01 <sub-ID 2> hr mn sc fr F7
		        //User Bits Message - F0 7F <device ID> 01 <sub-ID 2> u1 u2 u3 u4 u5 u6 u7 u8 u9 F7
				break
            #endif

			/*case 0x02: // MIDI Show Control
				break;
			case 0x03: // Notation Information
				break;*/

            #ifdef M2_ENABLE_GM2
			case 0x04: // Device Control
		        //GLOBAL PARAMETER CONTROL - F0 7F <device ID> 04 05 sw pw vw [[sh sl] ... ] [pp vv] ... F7
                //MASTER FINE TUNING - F0 7F <device ID> 04 03 lsb msb F7
                //MASTER COARSE TUNING - F0 7F <device ID> 04 04 00 msb F7
				break;
            #endif

            #ifdef M2_ENABLE_MTC
			case 0x05: // Real Time MTC Cueing
		        //Real Time MIDI Cueing Set-Up Message - F0 7F <device ID> 05 <sub-id #2> sl sm <additional info.> F7
				break;
            #endif

            /*case 0x06: // MIDI Machine Control Commands
				break;
			case 0x07 : // MIDI Machine Control Responses
				break;*/

            #if defined(M2_ENABLE_MIDI_TUNING_STANDARD) || defined(M2_ENABLE_GM2)
			case 0x08 : // MIDI Tuning Standard (Real Time)
		        //SINGLE NOTE TUNING CHANGE F0 7F <device ID> 08 02 <tt tuning program number (0 – 127)>
		            //<ll - number of changes> [<MIDI key number> <frequency data for that key>(3)](ll) F7
                //SINGLE NOTE TUNING CHANGE (BANK) F0 7F <device ID> 08 07 bb tt ll
                    //[<MIDI key number> <frequency data for that key>(3)](ll) F7
                //SCALE/OCTAVE TUNING 1-BYTE FORM - F0 7F <device ID> 08 08 <ff - channel/options>
                    //<gg -channel byte 2> <hh - channel byte 3> [ss](12)  F7
                //SCALE/OCTAVE TUNING 2-BYTE FORM - F0 7F <device ID> 08 09 ff gg hh [ss tt](12) F7
				break;
            #endif

            #ifdef M2_ENABLE_GM2
			case 0x09 : // Controller Destination Setting (See GM2 Documentation)
                //Channel Pressure/Polyphonic Key Pressure - F0 7F <device ID> 09 01/02 0n [pp rr] ... F7
                //Control Change - F0 7F <device ID> 09 03 0n cc [pp rr] ... F7
				break;
			case 0x0A : // Key-based Instrument Control
                //KEY-BASED INSTRUMENT CONTROL - F0 7F <device ID> 0A 01 0n kk [nn vv] .. F7
				break;
            #endif

            /*case 0x0B : // Scalable Polyphony MIDI MIP Message
				break;
			case 0x0C : // Mobile Phone Control Message
				break;	
		    */
		}
	}else if (recvUnknownSysEx != nullptr){
        recvUnknownSysEx(group, &syExMessInt[group], s7Byte);
	}
	syExMessInt[group].pos++;
}

void midi2Processor::processMIDICI(uint8_t group, uint8_t s7Byte){
	if(syExMessInt[group].pos == 3){
		midici[group].ciType =  s7Byte;
	}
	
	if(syExMessInt[group].pos == 4){
	    midici[group].ciVer =  s7Byte;
	} 
	if(syExMessInt[group].pos >= 5 && syExMessInt[group].pos <= 8){
        midici[group].remoteMUID += s7Byte << (7 * (syExMessInt[group].pos - 5));
	}
	
	if(syExMessInt[group].pos >= 9 && syExMessInt[group].pos <= 12){
        midici[group].localMUID += s7Byte << (7 * (syExMessInt[group].pos - 9));
	}
	
	if(syExMessInt[group].pos >= 12
       && midici[group].localMUID != M2_CI_BROADCAST
       && checkMUID && !checkMUID(group, midici[group].localMUID)
        ){
		return; //Not for this device
	}
	
	//break up each Process based on ciType
    if(syExMessInt[group].pos >= 12) {
        switch (midici[group].ciType) {
            case MIDICI_DISCOVERY: //Discovery Request
                if (syExMessInt[group].pos >= 13 && syExMessInt[group].pos <= 23) {
                    syExMessInt[group].buffer1[syExMessInt[group].pos - 13] = s7Byte;
                }
                if (syExMessInt[group].pos == 24) {
                    syExMessInt[group].intbuffer1[0] = s7Byte; // ciSupport
                }
                if (syExMessInt[group].pos >= 25 && syExMessInt[group].pos <= 28) {
                    syExMessInt[group].intbuffer1[1] += s7Byte << (7 * (syExMessInt[group].pos - 25)); //maxSysEx
                }
                if (syExMessInt[group].pos == 28) {
                    //debug("  - Discovery Request 28 ");
                    if (recvDiscoveryRequest != nullptr) {
                        uint8_t manuIdR[3] = {syExMessInt[group].buffer1[0], syExMessInt[group].buffer1[1],
                                              syExMessInt[group].buffer1[2]};
                        uint8_t famIdR[2] = {syExMessInt[group].buffer1[3], syExMessInt[group].buffer1[4]};
                        uint8_t modelIdR[2] = {syExMessInt[group].buffer1[5], syExMessInt[group].buffer1[6]};
                        uint8_t verR[4] = {syExMessInt[group].buffer1[7], syExMessInt[group].buffer1[8],
                                           syExMessInt[group].buffer1[9], syExMessInt[group].buffer1[10]};
                        recvDiscoveryRequest(
                                group,
                                midici[group],
                                manuIdR,
                                famIdR,
                                modelIdR,
                                verR,
                                syExMessInt[group].intbuffer1[0],
                                syExMessInt[group].intbuffer1[1]
                        );
                    }
                }

                break;
            case MIDICI_DISCOVERYREPLY: //Discovery Reply'
                if (syExMessInt[group].pos >= 13 && syExMessInt[group].pos <= 23) {
                    syExMessInt[group].buffer1[syExMessInt[group].pos - 13] = s7Byte;
                }
                if (syExMessInt[group].pos == 24) {
                    syExMessInt[group].intbuffer1[0] = s7Byte; // ciSupport
                }
                if (syExMessInt[group].pos >= 25 && syExMessInt[group].pos <= 28) {
                    syExMessInt[group].intbuffer1[1] += s7Byte << (7 * (syExMessInt[group].pos - 25)); //maxSysEx
                }
                if (syExMessInt[group].pos == 28) {
                    if (recvDiscoveryReply != nullptr) {
                        uint8_t manuIdR[3] = {syExMessInt[group].buffer1[0], syExMessInt[group].buffer1[1],
                                              syExMessInt[group].buffer1[2]};
                        uint8_t famIdR[2] = {syExMessInt[group].buffer1[3], syExMessInt[group].buffer1[4]};
                        uint8_t modelIdR[2] = {syExMessInt[group].buffer1[5], syExMessInt[group].buffer1[6]};
                        uint8_t verR[4] = {syExMessInt[group].buffer1[7], syExMessInt[group].buffer1[8],
                                           syExMessInt[group].buffer1[9], syExMessInt[group].buffer1[10]};
                        recvDiscoveryReply(
                                group,
                                midici[group],
                                manuIdR,
                                famIdR,
                                modelIdR,
                                verR,
                                syExMessInt[group].intbuffer1[0],
                                syExMessInt[group].intbuffer1[1]
                        );
                    }
                }
                break;

            case MIDICI_INVALIDATEMUID: //MIDI-CI Invalidate MUID Message

                if (syExMessInt[group].pos >= 13 && syExMessInt[group].pos <= 16) {
                    syExMessInt[group].buffer1[syExMessInt[group].pos - 13] = s7Byte;
                }

                //terminate MUID
                if (syExMessInt[group].pos == 16 && recvInvalidateMUID != nullptr) {
                    uint32_t terminateMUID = syExMessInt[group].buffer1[0]
                            + ((uint32_t)syExMessInt[group].buffer1[1] << 7)
                            + ((uint32_t)syExMessInt[group].buffer1[2] << 14)
                            + ((uint32_t)syExMessInt[group].buffer1[3] << 21);
                    recvInvalidateMUID(group, midici[group], terminateMUID);
                }
                break;
            case MIDICI_NAK: //MIDI-CI NAK
                if (recvNAK != nullptr) {
                    recvNAK(group, midici[group]);
                }
                break;

#ifndef M2_DISABLE_PROTOCOL
            case MIDICI_PROTOCOL_NEGOTIATION:
            case MIDICI_PROTOCOL_NEGOTIATION_REPLY:
            case MIDICI_PROTOCOL_SET:
            case MIDICI_PROTOCOL_TEST:
            case MIDICI_PROTOCOL_TEST_RESPONDER:
            case MIDICI_PROTOCOL_CONFIRM:
                processProtocolSysex(group, s7Byte);
                break;
#endif

#ifndef M2_DISABLE_PROFILE
            case MIDICI_PROFILE_INQUIRY: //Profile Inquiry
            case MIDICI_PROFILE_INQUIRYREPLY: //Reply to Profile Inquiry
            case MIDICI_PROFILE_SETON: //Set Profile On Message
            case MIDICI_PROFILE_SETOFF: //Set Profile Off Message
            case MIDICI_PROFILE_ENABLED: //Set Profile Enabled Message
            case MIDICI_PROFILE_DISABLED: //Set Profile Disabled Message
            case MIDICI_PROFILE_SPECIFIC_DATA: //ProfileSpecific Data
                processProfileSysex(group, s7Byte);
                break;
#endif


#ifndef M2_DISABLE_PE
            case MIDICI_PE_CAPABILITY: //Inquiry: Property Exchange Capabilities
            case MIDICI_PE_CAPABILITYREPLY: //Reply to Property Exchange Capabilities
            case MIDICI_PE_GET:  // Inquiry: Get Property Data
            case MIDICI_PE_GETREPLY: // Reply To Get Property Data - Needs Work!
            case MIDICI_PE_SET: // Inquiry: Set Property Data
            case MIDICI_PE_SETREPLY: // Reply To Inquiry: Set Property Data
            case MIDICI_PE_SUB: // Inquiry: Subscribe Property Data
            case MIDICI_PE_SUBREPLY: // Reply To Subscribe Property Data
            case MIDICI_PE_NOTIFY: // Notify
                processPESysex(group, s7Byte);
                break;
#endif

            default:
                if (recvUnknownMIDICI) {
                    recvUnknownMIDICI(group, &syExMessInt[group], midici[group], s7Byte);
                }
                break;

        }
    }
}

void midi2Processor::processUMP(uint32_t UMP){
	umpMess[messPos] = UMP;
		
	uint8_t mt = (umpMess[0] >> 28)  & 0xF;
	uint8_t group = (umpMess[0] >> 24) & 0xF;

	
	if(messPos == 0
        && (mt <= UMP_M1CVM || mt==0x6 || mt==0x7)
            ){ //32bit Messages

		if(mt == UMP_UTILITY){ //32 bits Utility Messages
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
	if(messPos == 1
       && (mt == UMP_SYSEX7 || mt == UMP_M2CVM || mt==0x8 || mt==0x9  || mt==0xA)
        ){ //64bit Messages
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
					if(perNotePB != 0) perNotePB(group, channel, val1, umpMess[1]);
					break;

				case NRPN_PERNOTE: //Assignable Per-Note Controller 1
                    if(nrpnPerNote != nullptr) nrpnPerNote(group, channel, val1, val2, umpMess[1]);
					break;	
					
				case RPN_PERNOTE: //Registered Per-Note Controller 0
                    if(rpnPerNote != nullptr) rpnPerNote(group, channel, val1, val2, umpMess[1]);
					break;	
					
				case PERNOTE_MANAGE: //Per-Note Management Message
                    if(perNoteManage != nullptr) perNoteManage(group, channel, val1, (bool)(val2 & 2), (bool)(val2 & 1));
					break;	
					
			}
		}
        messPos =0;
	}else
    if(messPos == 2
       && (mt == 0xB || mt == 0xC)
            ){ //96bit Messages
        messPos =0;
    }else
    if(messPos == 3
             && (mt == UMP_DATA || mt >= 0xD)
    ){ //128bit Messages
        if(mt == UMP_DATA){ //128 bits Data Messages (including System Exclusive 8)
            uint8_t status = (umpMess[0] >> 20) & 0xF;
            //SysEx 8
            if(status <= 3){
                //SysEx 8
                /*uint8_t numbytes  = (umpMess[0] >> 16) & 0xF;
                uint8_t streamId  = (umpMess[0] >> 8) & 0xFF;
                if(status == 0 || status == 1){
                    startSysex7(group); //streamId
                }

                if(numbytes > 0)processSysEx(group, umpMess[0] & 0xFF);
                if(numbytes > 1)processSysEx(group, (umpMess[1] >> 24) & 0xFF);
                if(numbytes > 2)processSysEx(group, (umpMess[1] >> 16) & 0xFF);
                if(numbytes > 3)processSysEx(group, (umpMess[1] >> 8) & 0xFF);
                if(numbytes > 4)processSysEx(group, umpMess[1] & 0xFF);

                if(numbytes > 5)processSysEx(group, (umpMess[2] >> 24) & 0xFF);
                if(numbytes > 6)processSysEx(group, (umpMess[2] >> 16) & 0xFF);
                if(numbytes > 7)processSysEx(group, (umpMess[2] >> 8) & 0xFF);
                if(numbytes > 8)processSysEx(group, umpMess[2] & 0xFF);

                if(numbytes > 9)processSysEx(group, (umpMess[3] >> 24) & 0xFF);
                if(numbytes > 10)processSysEx(group, (umpMess[3] >> 16) & 0xFF);
                if(numbytes > 11)processSysEx(group, (umpMess[3] >> 8) & 0xFF);
                if(numbytes > 12)processSysEx(group, umpMess[3] & 0xFF);

                if(status == 0 || status == 3){
                    endSysex7(group);
                }*/

            }else if(status == 8 || status ==9){
                //Beginning of Mixed Data Set
                //uint8_t mdsId  = (umpMess[0] >> 16) & 0xF;

                if(status == 8){
                    /*uint16_t numValidBytes  = umpMess[0] & 0xFFFF;
                    uint16_t numChunk  = (umpMess[1] >> 16) & 0xFFFF;
                    uint16_t numOfChunk  = umpMess[1] & 0xFFFF;
                    uint16_t manuId  = (umpMess[2] >> 16) & 0xFFFF;
                    uint16_t deviceId  = umpMess[2] & 0xFFFF;
                    uint16_t subId1  = (umpMess[3] >> 16) & 0xFFFF;
                    uint16_t subId2  = umpMess[3] & 0xFFFF;*/
                }else{
                    // MDS bytes?
                }

            }

        }
        messPos =0;
    } else {
		messPos++;
	}
	
}

void midi2Processor::sendDiscoveryRequest(uint8_t group, uint32_t srcMUID,
                                          uint8_t* sysexId, uint8_t* famId,
                                          uint8_t* modelId, uint8_t* ver,
                                          uint8_t ciSupport, uint16_t sysExMax
                   ){
	if(sendOutSysex == nullptr) return;
	
	uint8_t sysex[13];
    MIDICI midiCiHeader;
    midiCiHeader.ciType = MIDICI_DISCOVERY;
    midiCiHeader.localMUID = srcMUID;
    createCIHeader(sysex, midiCiHeader);
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

void midi2Processor::sendDiscoveryReply(uint8_t group,  uint32_t srcMUID, uint32_t destMuid,
                                          uint8_t* sysexId, uint8_t* famId,
                                          uint8_t* modelId, uint8_t* ver,
                                          uint8_t ciSupport, uint16_t sysExMax
){
    if(sendOutSysex == nullptr) return;

    uint8_t sysex[13];
    MIDICI midiCiHeader;
    midiCiHeader.ciType = MIDICI_DISCOVERYREPLY;
    midiCiHeader.localMUID = srcMUID;
    midiCiHeader.remoteMUID = destMuid;
    createCIHeader(sysex, midiCiHeader);

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

void midi2Processor::sendNAK(uint8_t group, uint32_t srcMUID, uint32_t destMuid){
	if(sendOutSysex == nullptr) return;
	uint8_t sysex[13];
    MIDICI midiCiHeader;
    midiCiHeader.ciType = MIDICI_NAK;
    midiCiHeader.localMUID = srcMUID;
    midiCiHeader.remoteMUID = destMuid;
    createCIHeader(sysex, midiCiHeader);
	sendOutSysex(group,sysex,13,0);
}

void midi2Processor::sendInvalidateMUID(uint8_t group, uint32_t srcMUID, uint32_t terminateMuid){
	if(sendOutSysex == nullptr) return;
	uint8_t sysex[13];
    MIDICI midiCiHeader;
    midiCiHeader.ciType = MIDICI_INVALIDATEMUID;
    midiCiHeader.localMUID = srcMUID;
    createCIHeader(sysex, midiCiHeader);
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
	if(syExMessInt[group].pos == 3 && s7Byte == 0x01){
        if(recvIdRequest != nullptr) recvIdRequest(group);
        return;
	}
	if(syExMessInt[group].pos == 3 && s7Byte == 0x02){
		//Identity Reply
		syExMessInt[group].buffer1[0] = s7Byte;
	}
	if(syExMessInt[group].buffer1[0] == 0x02){
		if(syExMessInt[group].pos >= 4 && syExMessInt[group].pos <= 14){
			syExMessInt[group].buffer1[syExMessInt[group].pos-3] = s7Byte;
		}

		if (syExMessInt[group].pos == 14 && recvIdResponse != nullptr){
			uint8_t manuIdR[3] = {syExMessInt[group].buffer1[1], syExMessInt[group].buffer1[2], syExMessInt[group].buffer1[3]};
			uint8_t famIdR[2] = {syExMessInt[group].buffer1[4], syExMessInt[group].buffer1[5]};
			uint8_t modelIdR[2] = {syExMessInt[group].buffer1[6], syExMessInt[group].buffer1[7]};
			uint8_t verR[4] = {syExMessInt[group].buffer1[8], syExMessInt[group].buffer1[9], syExMessInt[group].buffer1[10], syExMessInt[group].buffer1[11]};
            recvIdResponse(group, manuIdR, famIdR, modelIdR, verR);
		}
	}
}
#endif





