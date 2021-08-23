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

#ifndef UTILS_H
#define UTILS_H

#ifdef ARDUINO
    #define SERIAL_PRINT  Serial.print
#else
    #define SERIAL_PRINT  printf
#endif

#define NOTE_OFF 0x80
#define NOTE_ON 0x90
#define KEY_PRESSURE 0xA0
#define CC 0xB0
#define RPN 0x20
#define NRPN 0x30
#define RPN_RELATIVE 0x40
#define NRPN_RELATIVE 0x50
#define PROGRAM_CHANGE 0xC0
#define CHANNEL_PRESSURE 0xD0
#define PITCH_BEND 0xE0
#define PITCH_BEND_PERNOTE 0x60
#define NRPN_PERNOTE 0x10
#define RPN_PERNOTE 0x00
#define PERNOTE_MANAGE 0xF0

#define SYSEX_START 0xF0
#define TIMING_CODE 0xF1
#define SPP 0xF2
#define SONG_SELECT 0xF3
#define TUNEREQUEST 0xF6
#define SYSEX_STOP 0xF7
#define TIMINGCLOCK 0xF8
#define SEQSTART 0xFA
#define SEQCONT 0xFB
#define SEQSTOP 0xFC
#define ACTIVESENSE 0xFE
#define SYSTEMRESET 0xFF


#define S7UNIVERSAL_NRT 0x7E
#define S7UNIVERSAL_RT 0x7F

#define M2_CI_BROADCAST 0xFFFFFFF


#define PE_HEAD_STATE_IN_OBJECT          2
#define PE_HEAD_STATE_IN_STRING          4
#define PE_HEAD_STATE_IN_NUMBER          8

#define PE_HEAD_KEY		16
#define PE_HEAD_VALUE	32
#define PE_HEAD_BUFFERLEN	36

#include <math.h>


struct UMP64{
	uint32_t UMP[2];
};

struct UMP128{
	uint32_t UMP[4];
};


#ifdef M2_ENABLE_PE
struct peHeader {
    char resource[PE_HEAD_BUFFERLEN]="";
    char resId[PE_HEAD_BUFFERLEN]="";
    int  offset = -1;
    int  limit = -1;
    int  status = -1;
    int  totalChunks = -1;
    int  numChunks = -1;
    uint8_t requestId = 255;
    //int patch = -1;
    //char mediaType[BUFFERLEN];
    //char encoding[BUFFERLEN];
};
#endif



uint32_t scaleUp(uint32_t srcVal, uint8_t srcBits, uint8_t dstBits);

uint32_t scaleDown(uint32_t srcVal, uint8_t srcBits, uint8_t dstBits);

long getNumberFromBytes(uint8_t* message, uint8_t offset, uint8_t amount);

void setBytesFromNumbers(uint8_t* message, long number, uint8_t start, uint8_t amount);

void printUMP(uint32_t UMP);

#endif
