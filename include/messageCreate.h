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
    
#endif

uint32_t mt2Create(uint8_t group,  uint8_t status, uint8_t channel, uint8_t val1, uint8_t val2);

uint32_t mt2Create_noteOn(uint8_t group, uint8_t channel, uint8_t noteNumber, uint16_t velocity);
uint32_t mt2Create_noteOff(uint8_t group, uint8_t channel, uint8_t noteNumber, uint16_t velocity);

uint32_t mt2Create_polyPressure(uint8_t group, uint8_t channel, uint8_t noteNumber, uint32_t pressure);
uint32_t mt2Create_CC(uint8_t group, uint8_t channel, uint8_t index, uint32_t value);
uint32_t mt2Create_programChange(uint8_t group, uint8_t channel, uint8_t program);
uint32_t mt2Create_channelPressure(uint8_t group, uint8_t channel, uint32_t pressure);
uint32_t mt2Create_pitchBend(uint8_t group, uint8_t channel, uint32_t value);

uint32_t mt4CreateFirstWord(uint8_t group,  uint8_t status, uint8_t channel, uint8_t val1, uint8_t val2);

UMP64 mt4Create_noteOn(uint8_t group, uint8_t channel, uint8_t noteNumber, uint16_t velocity, 
    uint8_t attributeType, uint16_t attributeData);

UMP64 mt4Create_noteOff(uint8_t group, uint8_t channel, uint8_t noteNumber, uint16_t velocity,
    uint8_t attributeType, uint16_t attributeData);

UMP64 mt4Create_polyPressure(uint8_t group, uint8_t channel, uint8_t noteNumber, uint32_t pressure);

UMP64 mt4Create_pitchBend(uint8_t group, uint8_t channel, uint32_t pitch);

UMP64 mt4Create_cc(uint8_t group, uint8_t channel, uint8_t index, uint32_t value);

UMP64 mt4Create_rpn(uint8_t group, uint8_t channel,uint8_t bank,  uint8_t index, uint32_t value);
UMP64 mt4Create_nrpn(uint8_t group, uint8_t channel,uint8_t bank,  uint8_t index, uint32_t value);

UMP64 mt4Create_channelPressure(uint8_t group, uint8_t channel,uint32_t pressure);

UMP64 mt4Create_programChange(uint8_t group, uint8_t channel, uint8_t program, bool bankValid,
    uint8_t bank, uint8_t index);
				  
				 
#endif

