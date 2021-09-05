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

#ifndef MC7_H
#define MC7_H

#include <string.h>

class mcoded7Decode{

	private:
		uint8_t dumpPos=0;
		uint8_t dump[7];
		
		uint8_t fBit=0;
		uint8_t cnt=0;
		uint8_t bits=0;

	public:
		
		
		mcoded7Decode();
		
		uint8_t availableBytes(){ return (dumpPos > 0);}
		
		uint8_t* getBytes();
		
		void reset(){
			fBit=0; cnt=0; bits=0;dumpPos=0;
		}
		
		void parseS7Byte(uint8_t s7Byte){		
			if ((cnt % 8) == 0) {
				reset();
				bits = s7Byte;
			} else {
				fBit = ((bits >> (7 - (cnt % 8))) & 1) << 7;
				dump[dumpPos++] = s7Byte | fBit;
			}
			cnt++;		
		}
	
};

#endif

