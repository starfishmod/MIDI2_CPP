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

#ifndef MESSAGE_CREATE_H
#define MESSAGE_CREATE_H


#ifdef ARDUINO
   #include <stdint.h>
#else
    #include <cstdint>
#endif

uint32_t mt0NOOP(uint8_t group);
uint32_t mt0JRClock(uint8_t group, uint16_t clockTime);
uint32_t mt0JRTimeStamp(uint8_t group, uint16_t timestamp);

uint32_t mt1MTC(uint8_t group, uint8_t timeCode);
uint32_t mt1SPP(uint8_t group, uint16_t position);
uint32_t mt1SongSelect(uint8_t group, uint8_t song);
uint32_t mt1TuneRequest(uint8_t group);
uint32_t mt1TimingClock(uint8_t group);
uint32_t mt1SeqStart(uint8_t group);
uint32_t mt1SeqCont(uint8_t group);
uint32_t mt1SeqStop(uint8_t group);
uint32_t mt1ActiveSense(uint8_t group);
uint32_t mt1SystemReset(uint8_t group);


uint32_t mt2NoteOn(uint8_t group, uint8_t channel, uint8_t noteNumber, uint16_t velocity);
uint32_t mt2NoteOff(uint8_t group, uint8_t channel, uint8_t noteNumber, uint16_t velocity);
uint32_t mt2PolyPressure(uint8_t group, uint8_t channel, uint8_t noteNumber, uint32_t pressure);
uint32_t mt2CC(uint8_t group, uint8_t channel, uint8_t index, uint32_t value);
uint32_t mt2ProgramChange(uint8_t group, uint8_t channel, uint8_t program);
uint32_t mt2ChannelPressure(uint8_t group, uint8_t channel, uint32_t pressure);
uint32_t mt2PitchBend(uint8_t group, uint8_t channel, uint32_t value);


UMP64 mt4NoteOn(uint8_t group, uint8_t channel, uint8_t noteNumber, uint16_t velocity, 
    uint8_t attributeType, uint16_t attributeData);
UMP64 mt4NoteOff(uint8_t group, uint8_t channel, uint8_t noteNumber, uint16_t velocity,
    uint8_t attributeType, uint16_t attributeData);
UMP64 mt4CPolyPressure(uint8_t group, uint8_t channel, uint8_t noteNumber, uint32_t pressure);
UMP64 mt4PitchBend(uint8_t group, uint8_t channel, uint32_t pitch);
UMP64 mt4CC(uint8_t group, uint8_t channel, uint8_t index, uint32_t value);
UMP64 mt4RPN(uint8_t group, uint8_t channel,uint8_t bank,  uint8_t index, uint32_t value);
UMP64 mt4NRPN(uint8_t group, uint8_t channel,uint8_t bank,  uint8_t index, uint32_t value);
UMP64 mt4RelativeRPN(uint8_t group, uint8_t channel,uint8_t bank,  uint8_t index, int32_t value);
UMP64 mt4RelativeNRPN(uint8_t group, uint8_t channel,uint8_t bank,  uint8_t index, int32_t value);
UMP64 mt4ChannelPressure(uint8_t group, uint8_t channel,uint32_t pressure);
UMP64 mt4ProgramChange(uint8_t group, uint8_t channel, uint8_t program, bool bankValid,
    uint8_t bank, uint8_t index);
				  
				 
#endif

