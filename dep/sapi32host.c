/* sapi32host.c - out of process SAPI5 host that lets 64 bit NVGT use 32 bit only SAPI voices
 * This program is compiled as a 32 bit executable (nvgt_sapi32host.exe) and is spawned by the sapi5x32 tts engine in NVGT's main process,
 * bridging voices such as Eloquence or older RealSpeak which only ship 32 bit binaries, similar to the bridges used by 64 bit JAWS and NVDA.
 * It speaks a tiny length prefixed binary protocol over its standard input and output pipes:
 * request: uint32 length, uint8 opcode, payload. response: uint32 length, uint8 status (1 = ok), payload.
 * All integers are little endian and native to the machine since both ends always run on the same host.
 *
 * NVGT - NonVisual Gaming Toolkit
 * Copyright (c) 2022-2025 Sam Tupy
 * https://nvgt.dev
 * This software is provided "as-is", without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.
 * Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
 * 1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
*/

/* sapi.h defines several const globals which receive external linkage when compiled as C; sapibridge.c already emits them, so suppress the duplicate definitions in this translation unit. */
#define __SpeechConstants_MODULE_DEFINED__
#define __SpeechStringConstants_MODULE_DEFINED__

#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include "sapibridge.h"
#include "sapi32proto.h"

static HANDLE g_in;
static HANDLE g_out;

static int read_exact(void* buffer, unsigned int size)
{
unsigned char* p=buffer;
while(size)
{
DWORD moved=0;
if(!ReadFile(g_in, p, size, &moved, NULL)) return 0;
if(!moved) return 0;
p+=moved;
size-=moved;
}
return 1;
}
static int write_exact(const void* buffer, unsigned int size)
{
const unsigned char* p=buffer;
while(size)
{
DWORD moved=0;
if(!WriteFile(g_out, p, size, &moved, NULL)) return 0;
if(!moved) return 0;
p+=moved;
size-=moved;
}
return 1;
}
static int send_response(int status, const void* payload, unsigned int size)
{
unsigned int length=size+1;
unsigned char status_byte=status? 1 : 0;
if(!write_exact(&length, 4)) return 0;
if(!write_exact(&status_byte, 1)) return 0;
if(size&&(!write_exact(payload, size))) return 0;
return 1;
}
static int send_int_response(int status, int value)
{
return send_response(status, &value, 4);
}
static int send_string_response(char* text)
{
if(!text) return send_response(0, NULL, 0);
return send_response(1, text, strlen(text));
}
static int payload_int(unsigned char* payload, unsigned int size, int* value)
{
if(size<4) return 0;
memcpy(value, payload, 4);
return 1;
}
static int handle_speak_to_memory(sb_sapi* sapi, unsigned char* payload, unsigned int size)
{
char* text=malloc(size+1);
if(!text) return send_response(0, NULL, 0);
memcpy(text, payload, size);
text[size]=0;
void* pcm=NULL;
int pcm_size=0;
int ok=sb_sapi_speak_to_memory(sapi, text, &pcm, &pcm_size);
free(text);
if((!ok)||(!pcm)||(pcm_size<=0))
{
if(pcm) free(pcm);
return send_response(0, NULL, 0);
}
unsigned int format[3];
format[0]=sb_sapi_get_sample_rate(sapi);
format[1]=sb_sapi_get_channels(sapi);
format[2]=sb_sapi_get_bit_depth(sapi);
unsigned int length=1+sizeof(format)+pcm_size;
unsigned char status_byte=1;
ok=write_exact(&length, 4)&&write_exact(&status_byte, 1)&&write_exact(format, sizeof(format))&&write_exact(pcm, pcm_size);
free(pcm);
return ok;
}
int main(void)
{
g_in=GetStdHandle(STD_INPUT_HANDLE);
g_out=GetStdHandle(STD_OUTPUT_HANDLE);
if((g_in==INVALID_HANDLE_VALUE)||(g_out==INVALID_HANDLE_VALUE)) return 1;
sb_sapi* sapi=calloc(1, sizeof(sb_sapi));
if(!sapi) return 1;
if(!sb_sapi_initialise(sapi))
{
free(sapi);
return 1;
}
int running=1;
while(running)
{
unsigned int length=0;
if(!read_exact(&length, 4)) break;
if((length<1)||(length>SB32_MAX_PACKET)) break;
unsigned char* body=malloc(length);
if(!body) break;
if(!read_exact(body, length))
{
free(body);
break;
}
unsigned char opcode=body[0];
unsigned char* payload=body+1;
unsigned int payload_size=length-1;
int value=0;
int ok=1;
switch(opcode)
{
case SB32_SPEAK_TO_MEMORY:
ok=handle_speak_to_memory(sapi, payload, payload_size);
break;
case SB32_GET_VOICE_COUNT:
ok=send_int_response(1, sb_sapi_count_voices(sapi));
break;
case SB32_GET_VOICE_NAME:
if(!payload_int(payload, payload_size, &value)) ok=send_response(0, NULL, 0);
else ok=send_string_response(sb_sapi_get_voice_name(sapi, value));
break;
case SB32_GET_VOICE_LANGUAGE:
if(!payload_int(payload, payload_size, &value)) ok=send_response(0, NULL, 0);
else ok=send_string_response(sb_sapi_get_voice_language(sapi, value));
break;
case SB32_SET_VOICE:
if(!payload_int(payload, payload_size, &value)) ok=send_response(0, NULL, 0);
else ok=send_response(sb_sapi_set_voice(sapi, value), NULL, 0);
break;
case SB32_GET_VOICE:
ok=send_int_response(1, sb_sapi_get_voice(sapi));
break;
case SB32_SET_RATE:
if(!payload_int(payload, payload_size, &value)) ok=send_response(0, NULL, 0);
else ok=send_response(sb_sapi_set_rate(sapi, value), NULL, 0);
break;
case SB32_GET_RATE:
ok=send_int_response(1, sb_sapi_get_rate(sapi));
break;
case SB32_SET_PITCH:
if(!payload_int(payload, payload_size, &value)) ok=send_response(0, NULL, 0);
else ok=send_response(sb_sapi_set_pitch(sapi, value), NULL, 0);
break;
case SB32_GET_PITCH:
ok=send_int_response(1, sb_sapi_get_pitch(sapi));
break;
case SB32_SET_VOLUME:
if(!payload_int(payload, payload_size, &value)) ok=send_response(0, NULL, 0);
else ok=send_response(sb_sapi_set_volume(sapi, value), NULL, 0);
break;
case SB32_GET_VOLUME:
ok=send_int_response(1, sb_sapi_get_volume(sapi));
break;
case SB32_QUIT:
send_response(1, NULL, 0);
running=0;
break;
default:
ok=send_response(0, NULL, 0);
break;
}
free(body);
if(!ok) break;
}
sb_sapi_cleanup(sapi);
free(sapi);
return 0;
}
