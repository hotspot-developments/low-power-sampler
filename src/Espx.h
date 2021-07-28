// MIT License

// ESPX - Used to abstract differences in ESP8266 and ESP32 system calls.
// Copyright (c) 2021 Simon Chinnick

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef ESPX_H 
#define ESPX_H

#include <stdint.h>
#include <stddef.h>
#if defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESP8266httpUpdate.h>
#else
#include <WiFi.h>
#include <ESP32httpUpdate.h>
#endif


class Espx {

    public:
        static void deepSleep(uint64_t time_us, bool WakeWithWifi);

        static bool rtcUserMemoryRead(uint32_t offset, uint32_t *data, size_t size);
        static bool rtcUserMemoryWrite(uint32_t offset, uint32_t *data, size_t size);

        static t_httpUpdate_return httpUpdate(WiFiClient& client, const String& host, uint16_t port, const String& uri = "/",
                               const String& currentVersion = "");

};

#endif // ESPX_H