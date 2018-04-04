#include "stdafx.h"
/*Copyright 2018 Jesus D. Romero

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files(the "Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to
the following conditions :

The above copyright notice and this permission notice shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
*/
#include "Timer.h"
#include <iostream>
namespace DR {
	namespace SoporteBase {
		namespace Imagenes {
		void Timer::Start(){
			::QueryPerformanceCounter(&start);
		}
		void Timer::Stop() {
			::QueryPerformanceCounter(&stop);
		}
		double Timer::GetTime() {
			return (stop.QuadPart-start.QuadPart)/(double)(frequency.QuadPart);
		}
		LARGE_INTEGER Timer::Init() {
			LARGE_INTEGER test;
			::QueryPerformanceFrequency(&test);
			return test;
		}
		LARGE_INTEGER Timer::frequency=Init();
		void Timer::Print(const char * message) {
			std::cout<<message<<GetTime();
		}
		void Timer::PrintLine(const char * message) {
			std::cout<<message<<GetTime()<<std::endl;
		}
		void Timer::Print() {
			std::cout<<GetTime();
		}
		void Timer::PrintLine() {
			std::cout<<GetTime()<<std::endl;
		}

		}
	}
}