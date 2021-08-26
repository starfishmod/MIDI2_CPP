#include "../include/midi2Profiles.h"
#include "../include/midi2Processor.h"
#include "../include/utils.h"




#ifdef M2_ENABLE_PROFILE



#ifdef ARDUINO
    #define SERIAL_PRINT  Serial.print
    #include <stdint.h>
#else
    #define SERIAL_PRINT  printf
    #include <stdio.h>
    #include <cstdint>
#endif


void midi2Processor::processProfileSysex(uint8_t groupOffset, uint8_t s7Byte){
	switch (ciType[groupOffset]){
		case 0x20: //Profile Inquiry
			
			if (sysexPos[groupOffset] == 12 && recvProfileInquiry != 0){
				//Serial.print("<-Profile Inquiry: remoteMuid ");Serial.print(remoteMUID[groupOffset]);
				//Serial.print(" dest ");Serial.println(sysUniPort[groupOffset]);
				recvProfileInquiry(groupOffset + groupStart, remoteMUID[groupOffset], sysUniPort[groupOffset]);
			}
			break;
		case 0x21: //Reply to Profile Inquiry
			//Serial.print("<-Reply to Profile Inquiry: remoteMuid ");Serial.print(remoteMUID[groupOffset]);
			//Serial.print(" dest ");Serial.println(sysUniPort[groupOffset]);
			
			//Serial.print(sysexPos[groupOffset]);Serial.print(" - ");Serial.println(s7Byte);
			
			if(sysexPos[groupOffset] == 13){
				sys7IntBuffer[groupOffset][0] = (int)s7Byte;
			}
			if(sysexPos[groupOffset] == 14){
				sys7IntBuffer[groupOffset][0] += (int)s7Byte << 7;
			}
			
			if(sysexPos[groupOffset] == 13 + sys7CharBuffer[groupOffset][0]*5){
				sys7IntBuffer[groupOffset][1] = (int)s7Byte;
			}
			if(sysexPos[groupOffset] == 14 + sys7CharBuffer[groupOffset][0]*5){
				sys7IntBuffer[groupOffset][1] += (int)s7Byte << 7;
			}
			if(sysexPos[groupOffset] >= 15 && sysexPos[groupOffset] <= 12 + sys7IntBuffer[groupOffset][0]*5){
				uint8_t pos = (sysexPos[groupOffset] - 13) % 5;
				sys7CharBuffer[groupOffset][pos] = s7Byte;
				if(pos==4 && recvSetProfileEnabled!=0){
					uint8_t profile[5] = {sys7CharBuffer[groupOffset][0], sys7CharBuffer[groupOffset][1], sys7CharBuffer[groupOffset][2], sys7CharBuffer[groupOffset][3], sys7CharBuffer[groupOffset][4]};
					recvSetProfileEnabled(groupOffset + groupStart,remoteMUID[groupOffset], sysUniPort[groupOffset], profile); 
				}
			}
			
			if(sysexPos[groupOffset] >= 15 + sys7CharBuffer[groupOffset][0]*5  && sysexPos[groupOffset] <= 12 + sys7IntBuffer[groupOffset][0]*5 + sys7IntBuffer[groupOffset][1]*5){
				uint8_t pos = (sysexPos[groupOffset] - 13) % 5;
				sys7CharBuffer[groupOffset][pos] = s7Byte;
				if(pos==4 && recvSetProfileDisabled!=0){
					uint8_t profile[5] = {sys7CharBuffer[groupOffset][0], sys7CharBuffer[groupOffset][1], sys7CharBuffer[groupOffset][2], sys7CharBuffer[groupOffset][3], sys7CharBuffer[groupOffset][4]};
					recvSetProfileDisabled(groupOffset + groupStart,remoteMUID[groupOffset], sysUniPort[groupOffset], profile); 
				}
			}
			//processProfileInquiryReply(s7Byte);
			break;
		case 0x22: //Set Profile On Message
			if(sysexPos[groupOffset] >= 13 && sysexPos[groupOffset] <= 17){
				sys7CharBuffer[groupOffset][sysexPos[groupOffset]-13] = s7Byte; 
			}
			if (sysexPos[groupOffset] == 16 && recvInvalidateMUID != 0){
				uint8_t profile[5] = {sys7CharBuffer[groupOffset][0], sys7CharBuffer[groupOffset][1], sys7CharBuffer[groupOffset][2], sys7CharBuffer[groupOffset][3], sys7CharBuffer[groupOffset][4]};
				recvSetProfileOn(groupOffset + groupStart,remoteMUID[groupOffset], sysUniPort[groupOffset], profile); 
			}
			break;
		case 0x23: //Set Profile Off Message
			if(sysexPos[groupOffset] >= 13 && sysexPos[groupOffset] <= 17){
				sys7CharBuffer[groupOffset][sysexPos[groupOffset]-13] = s7Byte; 
			}
			if (sysexPos[groupOffset] == 16 && recvInvalidateMUID != 0){
				uint8_t profile[5] = {sys7CharBuffer[groupOffset][0], sys7CharBuffer[groupOffset][1], sys7CharBuffer[groupOffset][2], sys7CharBuffer[groupOffset][3], sys7CharBuffer[groupOffset][4]};
				recvSetProfileOff(groupOffset + groupStart,remoteMUID[groupOffset], sysUniPort[groupOffset], profile); 
			}
			break;	
		case 0x24: //Set Profile Enabled Message
			if(sysexPos[groupOffset] >= 13 && sysexPos[groupOffset] <= 17){
				sys7CharBuffer[groupOffset][sysexPos[groupOffset]-13] = s7Byte; 
			}
			if (sysexPos[groupOffset] == 16 && recvInvalidateMUID != 0){
				uint8_t profile[5] = {sys7CharBuffer[groupOffset][0], sys7CharBuffer[groupOffset][1], sys7CharBuffer[groupOffset][2], sys7CharBuffer[groupOffset][3], sys7CharBuffer[groupOffset][4]};
				recvSetProfileEnabled(groupOffset + groupStart,remoteMUID[groupOffset], sysUniPort[groupOffset], profile); 
			}
			break;  
		case 0x25: //Set Profile Diabled Message
			if(sysexPos[groupOffset] >= 13 && sysexPos[groupOffset] <= 17){
				sys7CharBuffer[groupOffset][sysexPos[groupOffset]-13] = s7Byte; 
			}
			if (sysexPos[groupOffset] == 16 && recvInvalidateMUID != 0){
				uint8_t profile[5] = {sys7CharBuffer[groupOffset][0], sys7CharBuffer[groupOffset][1], sys7CharBuffer[groupOffset][2], sys7CharBuffer[groupOffset][3], sys7CharBuffer[groupOffset][4]};
				recvSetProfileDisabled(groupOffset + groupStart,remoteMUID[groupOffset], sysUniPort[groupOffset], profile); 
			}
			break;  
	}
}
	





void midi2Processor::sendProfileListRequest(uint8_t group, uint32_t remoteMuid, uint8_t ciVersion, uint8_t destination){
	if(sendOutSysex ==0) return;
	uint8_t sysex[13];
	addCIHeader(0x20,sysex,ciVersion);
	sysex[1] = destination;
	setBytesFromNumbers(sysex, remoteMuid, 9, 4);
	sendOutSysex(group,sysex,13,0);
}


void midi2Processor::sendProfileListResponse(uint8_t group, uint32_t remoteMuid, uint8_t ciVersion, uint8_t destination, uint8_t profilesEnabledLen, uint8_t* profilesEnabled, uint8_t profilesDisabledLen , uint8_t* profilesDisabled ){
	if(sendOutSysex ==0) return;
	uint8_t sysex[13];
	addCIHeader(0x21,sysex,ciVersion);
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

void midi2Processor::sendProfileOn(uint8_t group, uint32_t remoteMuid, uint8_t ciVersion, uint8_t destination, uint8_t* profile){
	if(sendOutSysex ==0) return;
	uint8_t sysex[13];
	addCIHeader(0x22,sysex,ciVersion);
	sysex[1] = destination;
	setBytesFromNumbers(sysex, remoteMuid, 9, 4);
	sendOutSysex(group,sysex,13,1);
	sendOutSysex(group,profile,5,3);
}

void midi2Processor::sendProfileOff(uint8_t group, uint32_t remoteMuid, uint8_t ciVersion, uint8_t destination, uint8_t* profile){
	if(sendOutSysex ==0) return;
	uint8_t sysex[13];
	addCIHeader(0x23,sysex,ciVersion);
	sysex[1] = destination;
	setBytesFromNumbers(sysex, remoteMuid, 9, 4);
	sendOutSysex(group,sysex,13,1);
	sendOutSysex(group,profile,5,3);
}

void midi2Processor::sendProfileEnabled(uint8_t group, uint32_t remoteMuid, uint8_t ciVersion, uint8_t destination, uint8_t* profile){
	if(sendOutSysex ==0) return;
	uint8_t sysex[13];
	addCIHeader(0x24,sysex,ciVersion);
	sysex[1] = destination;
	setBytesFromNumbers(sysex, remoteMuid, 9, 4);
	sendOutSysex(group,sysex,13,1);
	sendOutSysex(group,profile,5,3);
}

void midi2Processor::sendProfileDisabled(uint8_t group, uint32_t remoteMuid, uint8_t ciVersion, uint8_t destination, uint8_t* profile){
	if(sendOutSysex ==0) return;
	uint8_t sysex[13];
	addCIHeader(0x25,sysex,ciVersion);
	sysex[1] = destination;
	setBytesFromNumbers(sysex, remoteMuid, 9, 4);
	sendOutSysex(group,sysex,13,1);
	sendOutSysex(group,profile,5,3);
}




#endif
