/* sapi32proto.h - opcodes shared between NVGT's sapi5x32 tts engine and the 32 bit SAPI host process (sapi32host.c)
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

#ifndef sapi32proto_h
#define sapi32proto_h

/* Largest request packet the host will accept, sanity limit to catch stream desync. */
#define SB32_MAX_PACKET 0x400000

enum sb32_opcode
{
SB32_SPEAK_TO_MEMORY=1,
SB32_GET_VOICE_COUNT,
SB32_GET_VOICE_NAME,
SB32_GET_VOICE_LANGUAGE,
SB32_SET_VOICE,
SB32_GET_VOICE,
SB32_SET_RATE,
SB32_GET_RATE,
SB32_SET_PITCH,
SB32_GET_PITCH,
SB32_SET_VOLUME,
SB32_GET_VOLUME,
SB32_QUIT
};

#endif
