#include "sha2.hpp"
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <string.h>
#include <string>

#define ch(x, y, z) ((x & y) ^ (~x & z))
#define maj(x, y, z) ((x & y) ^ (x & z) ^ (y & z))
#define RotR(x, n) ((x >> n) | (x << ((sizeof(x) << 3) - n)))
#define Sig0(x) ((RotR(x, 28)) ^ (RotR(x, 34)) ^ (RotR(x, 39)))
#define Sig1(x) ((RotR(x, 14)) ^ (RotR(x, 18)) ^ (RotR(x, 41)))
#define sig0(x) (RotR(x, 1) ^ RotR(x, 8) ^ (x >> 7))
#define sig1(x) (RotR(x, 19) ^ RotR(x, 61) ^ (x >> 6))

std::string sha512::get(const std::string& input) {
	size_t n_buffer;					  // amount of message blocks
	uint64_t** buffer;					  // message block buffers (each 1024-bit = 16 64-bit words)
	uint64_t* h = new uint64_t[HASH_LEN]; // buffer holding the message digest (512-bit = 8 64-bit words)

	buffer = pre_process(input.c_str(), input.length(), n_buffer);
	process(buffer, n_buffer, h);

	free_buffer(buffer, n_buffer);
	return digest(h);
}

std::string sha512::get(const std::vector<uint8_t>& input) {
	size_t n_buffer;					  // amount of message blocks
	uint64_t** buffer;					  // message block buffers (each 1024-bit = 16 64-bit words)
	uint64_t* h = new uint64_t[HASH_LEN]; // buffer holding the message digest (512-bit = 8 64-bit words)

	buffer = pre_process((const char*)input.data(), input.size(), n_buffer);
	process(buffer, n_buffer, h);

	free_buffer(buffer, n_buffer);
	return digest(h);
}

uint64_t** sha512::pre_process(const char* input, size_t input_size, size_t& n_buffer) {
	// Padding: input || 1 || 0*k || l (in 128-bit representation)
	size_t l = input_size * CHAR_LEN_BITS;		   // length of input in bits
	size_t k = (896 - 1 - l) % MESSAGE_BLOCK_SIZE; // length of zero bit padding (l + 1 + k = 896 mod 1024)
	n_buffer = (l + 1 + k + 128) / MESSAGE_BLOCK_SIZE;

	uint64_t** buffer = new uint64_t*[n_buffer];

	for (size_t i = 0; i < n_buffer; i++) {
		buffer[i] = new uint64_t[SEQUENCE_LEN];
	}

	uint64_t in;
	size_t index;

	// Either copy existing message, add 1 bit or add 0 bit
	for (size_t i = 0; i < n_buffer; i++) {
		for (size_t j = 0; j < SEQUENCE_LEN; j++) {
			in = 0x0ULL;
			for (size_t k = 0; k < WORD_LEN; k++) {
				index = i * 128 + j * 8 + k;
				if (index < input_size) {
					in = in << 8 | (uint64_t)input[index];
				} else if (index == input_size) {
					in = in << 8 | 0x80ULL;
				} else {
					in = in << 8 | 0x0ULL;
				}
			}
			buffer[i][j] = in;
		}
	}

	// Append the length to the last two 64-bit blocks
	append_len(l, buffer[n_buffer - 1][SEQUENCE_LEN - 1], buffer[n_buffer - 1][SEQUENCE_LEN - 2]);
	return buffer;
}

void sha512::process(uint64_t** buffer, size_t n_buffer, uint64_t* h) {
	uint64_t s[WORKING_VAR_LEN];
	uint64_t w[MESSAGE_SCHEDULE_LEN];

	memcpy(h, prime, WORKING_VAR_LEN * sizeof(uint64_t));

	for (size_t i = 0; i < n_buffer; i++) {
		// copy over to message schedule
		memcpy(w, buffer[i], SEQUENCE_LEN * sizeof(uint64_t));

		// Prepare the message schedule
		for (size_t j = 16; j < MESSAGE_SCHEDULE_LEN; j++) {
			w[j] = w[j - 16] + sig0(w[j - 15]) + w[j - 7] + sig1(w[j - 2]);
		}
		// Initialize the working variables
		memcpy(s, h, WORKING_VAR_LEN * sizeof(uint64_t));

		// Compression
		for (size_t j = 0; j < MESSAGE_SCHEDULE_LEN; j++) {
			uint64_t temp1 = s[7] + Sig1(s[4]) + ch(s[4], s[5], s[6]) + k[j] + w[j];
			uint64_t temp2 = Sig0(s[0]) + maj(s[0], s[1], s[2]);

			s[7] = s[6];
			s[6] = s[5];
			s[5] = s[4];
			s[4] = s[3] + temp1;
			s[3] = s[2];
			s[2] = s[1];
			s[1] = s[0];
			s[0] = temp1 + temp2;
		}

		// Compute the intermediate hash values
		for (size_t j = 0; j < WORKING_VAR_LEN; j++) {
			h[j] += s[j];
		}
	}
}

void sha512::append_len(size_t l, uint64_t& lo, uint64_t& hi) {
	lo = l;
	hi = 0x00ULL;
}

std::string sha512::digest(uint64_t* h) {
	std::stringstream ss;
	for (size_t i = 0; i < OUTPUT_LEN; i++) {
		ss << std::hex << std::setw(16) << std::setfill('0') << h[i];
	}
	delete[] h;
	return ss.str();
}

void sha512::free_buffer(uint64_t** buffer, size_t n_buffer) {
	for (size_t i = 0; i < n_buffer; i++)
		delete[] buffer[i];

	delete[] buffer;
}