/* sapi32.cpp - SAPI engine that reaches 32 bit only voices through the nvgt_sapi32host.exe helper process
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

#ifdef _WIN32 // This file can be lazy wildcarded into builds.
#include <cstdint>
#include <cstring>
#include <set>
#include <stdexcept>
#include <sapi32proto.h>
#include "sapi32.h"

using namespace std;

static bool pipe_write_exact(HANDLE h, const void *data, size_t size) {
	const unsigned char *p = (const unsigned char *)data;
	while (size) {
		DWORD moved = 0;
		if (!WriteFile(h, p, (DWORD)size, &moved, nullptr) || !moved) return false;
		p += moved;
		size -= moved;
	}
	return true;
}
static bool pipe_read_exact(HANDLE h, void *data, size_t size) {
	unsigned char *p = (unsigned char *)data;
	while (size) {
		DWORD moved = 0;
		if (!ReadFile(h, p, (DWORD)size, &moved, nullptr) || !moved) return false;
		p += moved;
		size -= moved;
	}
	return true;
}
sapi5_32_engine::sapi5_32_engine() : tts_engine_impl("SAPI5x32"), process(nullptr), stdin_write(nullptr), stdout_read(nullptr) {
	// Locate the helper next to the running executable, preferring the lib directory which is where it ships both in the NVGT distribution and in bundled games.
	wchar_t module_path[MAX_PATH];
	DWORD len = GetModuleFileNameW(nullptr, module_path, MAX_PATH);
	if (!len || len >= MAX_PATH) throw runtime_error("Failed to determine executable path");
	wstring dir(module_path);
	size_t slash = dir.find_last_of(L'\\');
	if (slash == wstring::npos) throw runtime_error("Failed to determine executable directory");
	dir.resize(slash + 1);
	wstring helper = dir + L"lib\\nvgt_sapi32host.exe";
	if (GetFileAttributesW(helper.c_str()) == INVALID_FILE_ATTRIBUTES) helper = dir + L"nvgt_sapi32host.exe";
	if (GetFileAttributesW(helper.c_str()) == INVALID_FILE_ATTRIBUTES) throw runtime_error("nvgt_sapi32host.exe not found");
	SECURITY_ATTRIBUTES sa = {sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE};
	HANDLE child_stdin_read = nullptr, child_stdout_write = nullptr;
	if (!CreatePipe(&child_stdin_read, &stdin_write, &sa, 0)) throw runtime_error("Failed to create stdin pipe for SAPI host");
	if (!CreatePipe(&stdout_read, &child_stdout_write, &sa, 0)) {
		CloseHandle(child_stdin_read);
		CloseHandle(stdin_write);
		throw runtime_error("Failed to create stdout pipe for SAPI host");
	}
	SetHandleInformation(stdin_write, HANDLE_FLAG_INHERIT, 0);
	SetHandleInformation(stdout_read, HANDLE_FLAG_INHERIT, 0);
	STARTUPINFOW si = {};
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESTDHANDLES;
	si.hStdInput = child_stdin_read;
	si.hStdOutput = child_stdout_write;
	si.hStdError = nullptr;
	PROCESS_INFORMATION pi = {};
	BOOL created = CreateProcessW(helper.c_str(), nullptr, nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
	CloseHandle(child_stdin_read);
	CloseHandle(child_stdout_write);
	if (!created) {
		CloseHandle(stdin_write);
		CloseHandle(stdout_read);
		throw runtime_error("Failed to launch nvgt_sapi32host.exe");
	}
	CloseHandle(pi.hThread);
	process = pi.hProcess;
	int count = transact_get_int(SB32_GET_VOICE_COUNT, -1);
	if (count <= 0) {
		shutdown();
		throw runtime_error("32 bit SAPI host reported no voices");
	}
	// Most voices install both a 32 and a 64 bit token, and the 64 bit ones are already served by the sapi5 engine. Only expose voices exclusive to the 32 bit registry view so users don't see duplicates.
	set<string> native_names;
	sb_sapi native;
	memset(&native, 0, sizeof(native));
	if (sb_sapi_initialise(&native)) {
		int native_count = sb_sapi_count_voices(&native);
		for (int i = 0; i < native_count; i++) {
			char *name = sb_sapi_get_voice_name(&native, i);
			if (name) native_names.insert(name);
		}
		sb_sapi_cleanup(&native);
	}
	for (int i = 0; i < count; i++) {
		string index_payload((const char *)&i, 4), name, language;
		if (!transact(SB32_GET_VOICE_NAME, index_payload, name) || name.empty()) continue;
		if (native_names.find(name) != native_names.end()) continue;
		transact(SB32_GET_VOICE_LANGUAGE, index_payload, language);
		voices.push_back({i, name, language});
	}
	if (voices.empty()) {
		shutdown();
		throw runtime_error("No 32 bit exclusive SAPI voices found");
	}
}
sapi5_32_engine::~sapi5_32_engine() { shutdown(); }
void sapi5_32_engine::shutdown() {
	if (stdin_write) {
		string response;
		transact(SB32_QUIT, "", response); // may tear the connection down itself on failure, so re-check the handle
	}
	if (stdin_write) {
		CloseHandle(stdin_write);
		stdin_write = nullptr;
	}
	if (stdout_read) {
		CloseHandle(stdout_read);
		stdout_read = nullptr;
	}
	if (process) {
		if (WaitForSingleObject(process, 2000) != WAIT_OBJECT_0) TerminateProcess(process, 1);
		CloseHandle(process);
		process = nullptr;
	}
}
// Closes the pipes and terminates the host without the usual quit handshake. Used when a transaction fails partway, since the stream position is then unknown and every later exchange would read garbage. io_mtx must be held.
bool sapi5_32_engine::kill_connection() {
	if (stdin_write) {
		CloseHandle(stdin_write);
		stdin_write = nullptr;
	}
	if (stdout_read) {
		CloseHandle(stdout_read);
		stdout_read = nullptr;
	}
	if (process) {
		TerminateProcess(process, 1);
		CloseHandle(process);
		process = nullptr;
	}
	return false;
}
bool sapi5_32_engine::transact(unsigned char opcode, const string &payload, string &response) {
	lock_guard<mutex> lock(io_mtx);
	if (!stdin_write || !stdout_read) return false;
	if (payload.size() >= SB32_MAX_PACKET) return false; // nothing was sent, the stream is still in sync
	uint32_t length = (uint32_t)payload.size() + 1;
	string packet((const char *)&length, 4);
	packet.push_back((char)opcode);
	packet += payload;
	if (!pipe_write_exact(stdin_write, packet.data(), packet.size())) return kill_connection();
	uint32_t response_length = 0;
	if (!pipe_read_exact(stdout_read, &response_length, 4)) return kill_connection();
	if (response_length < 1 || response_length > 0x8000000) return kill_connection();
	string body(response_length, 0);
	if (!pipe_read_exact(stdout_read, &body[0], response_length)) return kill_connection();
	if (body[0] != 1) return false; // failure response, but fully consumed; the stream stays usable
	response.assign(body, 1, response_length - 1);
	return true;
}
bool sapi5_32_engine::transact_set_int(unsigned char opcode, int value) {
	string response;
	return transact(opcode, string((const char *)&value, 4), response);
}
int sapi5_32_engine::transact_get_int(unsigned char opcode, int fallback) {
	string response;
	if (!transact(opcode, "", response) || response.size() < 4) return fallback;
	int value = 0;
	memcpy(&value, response.data(), 4);
	return value;
}
tts_pcm_generation_state sapi5_32_engine::get_pcm_generation_state() { return PCM_PREFERRED; }
tts_audio_data* sapi5_32_engine::speak_to_pcm(const string &text) {
	if (text.empty()) return nullptr;
	string response;
	if (!transact(SB32_SPEAK_TO_MEMORY, text, response)) return nullptr;
	if (response.size() <= 12) return nullptr;
	uint32_t sample_rate = 0, channels = 0, bits = 0;
	memcpy(&sample_rate, response.data(), 4);
	memcpy(&channels, response.data() + 4, 4);
	memcpy(&bits, response.data() + 8, 4);
	size_t pcm_size = response.size() - 12;
	void *pcm = malloc(pcm_size);
	if (!pcm) return nullptr;
	memcpy(pcm, response.data() + 12, pcm_size);
	return new tts_audio_data(this, pcm, (unsigned int)pcm_size, sample_rate, channels, bits);
}
float sapi5_32_engine::get_rate() { return transact_get_int(SB32_GET_RATE, 0); }
float sapi5_32_engine::get_pitch() { return transact_get_int(SB32_GET_PITCH, 0); }
float sapi5_32_engine::get_volume() { return transact_get_int(SB32_GET_VOLUME, 0); }
void sapi5_32_engine::set_rate(float rate) { transact_set_int(SB32_SET_RATE, rate); }
void sapi5_32_engine::set_pitch(float pitch) { transact_set_int(SB32_SET_PITCH, pitch); }
void sapi5_32_engine::set_volume(float volume) { transact_set_int(SB32_SET_VOLUME, volume); }
bool sapi5_32_engine::get_rate_range(float& minimum, float& midpoint, float& maximum) { minimum = -10; midpoint = 0; maximum = 10; return true; }
bool sapi5_32_engine::get_pitch_range(float& minimum, float& midpoint, float& maximum) { minimum = -10; midpoint = 0; maximum = 10; return true; }
bool sapi5_32_engine::get_volume_range(float& minimum, float& midpoint, float& maximum) { minimum = 0; midpoint = 50; maximum = 100; return true; }
int sapi5_32_engine::get_voice_count() { return (int)voices.size(); }
string sapi5_32_engine::get_voice_name(int index) {
	if (index < 0 || index >= (int)voices.size()) return "";
	return voices[index].name;
}
string sapi5_32_engine::get_voice_language(int index) {
	if (index < 0 || index >= (int)voices.size()) return "";
	return voices[index].language;
}
bool sapi5_32_engine::set_voice(int voice) {
	if (voice < 0 || voice >= (int)voices.size()) return false;
	return transact_set_int(SB32_SET_VOICE, voices[voice].remote_index);
}
int sapi5_32_engine::get_current_voice() {
	int remote = transact_get_int(SB32_GET_VOICE, -1);
	if (remote < 0) return -1;
	for (size_t i = 0; i < voices.size(); i++) {
		if (voices[i].remote_index == remote) return (int)i;
	}
	return -1;
}

#endif // _WIN32
