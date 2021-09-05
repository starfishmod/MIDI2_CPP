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
    #include <stdint.h>
#else
    #define SERIAL_PRINT  printf
    #include <cstdint>
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
#define S7IDREQUEST 0x06
#define S7MIDICI 0x0D

#define MIDICI_DISCOVERY 0x70
#define MIDICI_DISCOVERYREPLY 0x71
#define MIDICI_INVALIDATEMUID 0x7E
#define MIDICI_NAK 0x7F

#define MIDICI_PROFILE_INQUIRY 0x20
#define MIDICI_PROFILE_INQUIRYREPLY 0x21
#define MIDICI_PROFILE_SETON 0x22
#define MIDICI_PROFILE_SETOFF 0x23
#define MIDICI_PROFILE_ENABLED 0x24
#define MIDICI_PROFILE_DISABLED 0x25

#define MIDICI_PE_CAPABILITY 0x30
#define MIDICI_PE_CAPABILITYREPLY 0x31
#define MIDICI_PE_GET 0x34
#define MIDICI_PE_GETREPLY 0x35
#define MIDICI_PE_SET 0x36
#define MIDICI_PE_SETREPLY 0x37

#define MIDI_PORT 0x7F

#define M2_CI_BROADCAST 0xFFFFFFF


#define PE_HEAD_STATE_IN_OBJECT          2
#define PE_HEAD_STATE_IN_STRING          4
#define PE_HEAD_STATE_IN_NUMBER          8

#define PE_HEAD_KEY		16
#define PE_HEAD_VALUE	32
#define PE_HEAD_BUFFERLEN	36

#define UMP_UTILITY 0x00
#define UMP_SYSTEM 0x01
#define UMP_M1CVM 0x02
#define UMP_SYSEX7 0x03
#define UMP_M2CVM 0x04

#include <math.h>



struct UMP64{
	uint32_t UMP[2];
};

struct UMP128{
	uint32_t UMP[4];
};


#ifndef M2_DISABLE_PE
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
