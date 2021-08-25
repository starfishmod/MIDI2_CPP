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
#include "../include/utils.h"
#include "../include/messageCreate.h"


uint32_t mt2Create(uint8_t group,  uint8_t status, uint8_t channel, uint8_t val1, uint8_t val2){
	uint32_t message;
	message = ((UMP_M1CVM << 4) + group + 0L) << 24;
	message +=  (status + channel  + 0L) << 16;
	message +=  (int)val1  << 8;
	message += val2;
	return message;
} 

uint32_t mt2Create_noteOn(uint8_t group, uint8_t channel, uint8_t noteNumber, uint16_t velocity){
	return mt2Create(group,  NOTE_ON, channel, noteNumber, scaleDown(16,7, velocity));
} 
uint32_t mt2Create_noteOff(uint8_t group, uint8_t channel, uint8_t noteNumber, uint16_t velocity){
	return mt2Create(group,  NOTE_OFF, channel, noteNumber, scaleDown(16,7, velocity));
}

uint32_t mt2Create_polyPressure(uint8_t group, uint8_t channel, uint8_t noteNumber, uint32_t pressure){
	return mt2Create(group,  KEY_PRESSURE, channel, noteNumber, scaleDown(32,7, pressure));
} 
uint32_t mt2Create_CC(uint8_t group, uint8_t channel, uint8_t index, uint32_t value){
	return mt2Create(group,  CC, channel, index, scaleDown(32,7, value));
} 
uint32_t mt2Create_programChange(uint8_t group, uint8_t channel, uint8_t program){
	return mt2Create(group,  PROGRAM_CHANGE, channel, program, 0);
} 
uint32_t mt2Create_channelPressure(uint8_t group, uint8_t channel, uint32_t pressure){
	return mt2Create(group,  CHANNEL_PRESSURE, channel, scaleDown(32,7, pressure), 0);
} 
uint32_t mt2Create_pitchBend(uint8_t group, uint8_t channel, uint32_t value){
	int pb = scaleDown(32,14, value);
	return mt2Create(group,  PITCH_BEND, channel, pb & 0x7F, (pb >> 7) & 0x7F);
} 


uint32_t mt4CreateFirstWord(uint8_t group,  uint8_t status, uint8_t channel, uint8_t val1, uint8_t val2){
	uint32_t message;
	message = ((UMP_M2CVM << 4) + group + 0L) << 24;
	message +=  (status + channel  + 0L) << 16;
	message +=  (int)val1  << 8;
	message += val2;
	return message;
} 

UMP64 mt4Create_noteOn(uint8_t group, uint8_t channel, uint8_t noteNumber, uint16_t velocity, uint8_t attributeType, uint16_t attributeData){
	UMP64 umpMess;
	umpMess.UMP[0] = mt4CreateFirstWord(group,  NOTE_ON, channel, noteNumber, attributeType);
	umpMess.UMP[1] = velocity << 16;
	umpMess.UMP[1] += attributeData;
	return umpMess;
}

UMP64 mt4Create_noteOff(uint8_t group, uint8_t channel, uint8_t noteNumber, uint16_t velocity, uint8_t attributeType, uint16_t attributeData){
	UMP64 umpMess;
	umpMess.UMP[0] = mt4CreateFirstWord(group,  NOTE_OFF, channel, noteNumber, attributeType);
	umpMess.UMP[1] = velocity << 16;
	umpMess.UMP[1] += attributeData;
	return umpMess;
}  


UMP64 mt4Create_polyPressure(uint8_t group, uint8_t channel, uint8_t noteNumber, uint32_t pressure){
	UMP64 umpMess;
	umpMess.UMP[0] = mt4CreateFirstWord(group,  KEY_PRESSURE, channel, noteNumber, 0);
	umpMess.UMP[1] = pressure;
	return umpMess;
}

UMP64 mt4Create_pitchBend(uint8_t group, uint8_t channel, uint32_t pitch){
	UMP64 umpMess;
	umpMess.UMP[0] = mt4CreateFirstWord(group,  PITCH_BEND, channel, 0, 0);
	umpMess.UMP[1] = pitch;
	return umpMess;
} 

UMP64 mt4Create_cc(uint8_t group, uint8_t channel, uint8_t index, uint32_t value){
	UMP64 umpMess;
	umpMess.UMP[0] = mt4CreateFirstWord(group,  CC , channel, index, 0);
	umpMess.UMP[1] = value;
	return umpMess;
} 

UMP64 mt4Create_rpn(uint8_t group, uint8_t channel,uint8_t bank,  uint8_t index, uint32_t value){
	UMP64 umpMess;
	umpMess.UMP[0] = mt4CreateFirstWord(group,  RPN , channel, bank, index);
	umpMess.UMP[1] = value;
	return umpMess;
} 


UMP64 mt4Create_channelPressure(uint8_t group, uint8_t channel,uint32_t pressure){
	UMP64 umpMess;
	umpMess.UMP[0] = mt4CreateFirstWord(group,  CHANNEL_PRESSURE, channel, 0, 0);
	umpMess.UMP[1] = pressure;
	return umpMess;
}

UMP64 mt4Create_programChange(uint8_t group, uint8_t channel, uint8_t program, bool bankValid, uint8_t bank, uint8_t index){
	UMP64 umpMess;
	umpMess.UMP[0] = mt4CreateFirstWord(group,  PROGRAM_CHANGE, channel, program, bankValid?1:0);
	umpMess.UMP[1] = bankValid? ((uint32_t)bank << 8)+ index :0;
	return umpMess;
}
				  
				 
				  
				




