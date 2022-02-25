#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "N2KTypes.h"

bool n2k_act_to_bytes(const n2k_act_message *act, uint8_t **out, size_t *len) {
	if (act == NULL || out == NULL || len == NULL) { return false; }

	(*out) = calloc(act->length + 5, sizeof(uint8_t));
	(*out)[0] = ACT_ESC;
	(*out)[1] = ACT_SOT;
	(*out)[2] = ACT_N2K;
	(*out)[3] = act->length;
	(*out)[4] = act->priority;
	(*out)[5] = (act->PGN & 0x0000FF);
	(*out)[6] = (act->PGN & 0x00FF00) >> 8;
	(*out)[7] = (act->PGN & 0xFF0000) >> 16;
	(*out)[8] = act->dst;
	(*out)[9] = act->src;
	// The byte ordering here is an assumption...
	(*out)[10] = (act->timestamp & 0x000000FF);
	(*out)[11] = (act->timestamp & 0x0000FF00) >> 8;
	(*out)[12] = (act->timestamp & 0x00FF0000) >> 16;
	(*out)[13] = (act->timestamp & 0xFF000000) >> 24;
	(*out)[14] = act->datalen;
	size_t ix = 15;
	for (int i = 0; i < act->datalen; i++) {
		(*out)[ix++] = act->data[i];
		if (act->data[i] == ACT_ESC) {
			// Double up ACT_ESC to represent literal ESC char
			(*out)[ix++] = ACT_ESC;
		}
	}
	(*out)[ix++] = act->csum;
	(*out)[ix++] = ACT_ESC;
	(*out)[ix++] = ACT_EOT;
	(*len) = ix;
	return true;
}

bool n2k_act_from_bytes(const uint8_t *in, const size_t len, n2k_act_message **msg, size_t *pos, bool debug) {
	if (in == NULL || msg == NULL || len < 18 || pos == NULL) { return NULL; }

	ssize_t start = -1;
	while (((*pos) + 18) < len) {
		if ((in[(*pos)] == ACT_ESC) && (in[(*pos) + 1] == ACT_SOT) && (in[(*pos) + 2] == ACT_N2K)) {
			start = (*pos);
			break;
		}
		(*pos)++;
	}

	if (start < 0) {
		/* Nothing found, so bail early.
		 * The variable passed as 'pos' has been incremented as far as
		 * the end of the search (len-18), so the caller knows what can
		 * be discarded.
		 */
		return false;
	}
	if ((len - start) < in[start + 3]) {
		// Message claims to be larger than available data, so leave
		// (*pos) where it is and return false until we get more data
		return false;
	}

	(*msg) = calloc(1, sizeof(n2k_act_message));
	if ((*msg) == NULL) { return false; }

	(*msg)->length = in[start + 3];
	(*msg)->priority = in[start + 4];
	(*msg)->PGN = in[start + 5] + ((uint32_t)in[start + 6] << 8) + ((uint32_t)in[start + 7] << 16);

	// PGN validation?

	(*msg)->dst = in[start + 8];
	(*msg)->src = in[start + 9];

	// Validate src + dst

	(*msg)->timestamp = in[start + 10] + ((uint32_t)in[start + 11] << 8) + ((uint32_t)in[start + 12] << 16) +
	                    ((uint32_t)in[start + 13] << 24);
	(*msg)->datalen = in[start + 14];

	volatile ssize_t remaining = len - start - 15;
	if (remaining <= ((*msg)->datalen + 3)) { // Available data - unusable data before start - message so far
		// Not enough data present to read the rest of the message
		free(*msg);
		(*msg) = NULL;
		// Don't update *pos
		return false;
	}

	uint8_t csum = 0;
	for (int i = 2; i < 15; i++) {
		csum += in[start + i];
	}
	(*msg)->data = calloc((*msg)->datalen, sizeof(uint8_t));
	if ((*msg)->data == NULL) {
		perror("n2k_act_from_bytes:data-calloc");
		free(*msg);
		(*msg) = NULL;
		// Don't update *pos, just in case the memory issue is recoverable
		return false;
	}

	size_t off = 15;
	for (int i = 0; i < (*msg)->datalen; i++) {
		uint8_t c = in[(start + off++)];
		remaining--;
		if (c == ACT_ESC) {
			uint8_t next = in[(start + off++)];
			if (next == ACT_ESC) {
				(*msg)->data[i] = ACT_ESC;
				remaining--;
			} else if (next == ACT_EOT) {
				// Message terminated early
				(*pos) = (start + off);
				free((*msg)->data);
				free(*msg);
				(*msg) = NULL;
				return false;
			} else if (next == ACT_SOT) {
				// This....probably shouldn't happen.
				// Exit as above, but reposition to before the message start
				(*pos) = (start + off - 2);
				free((*msg)->data);
				free(*msg);
				(*msg) = NULL;
				return false;
			} else {
				// Any other ESC + character sequence here is invalid
				free((*msg)->data);
				free(*msg);
				(*msg) = NULL;
				return false;
			}
		} else {
			(*msg)->data[i] = c;
		}
		csum += c;
		if (remaining < ((*msg)->datalen - i + 3)) {
			free((*msg)->data);
			free(*msg);
			(*msg) = NULL;
			return false;
		}
	}

	(*msg)->csum = in[(start + off++)];

	uint8_t ee = in[(start + off++)];
	uint8_t et = in[(start + off++)];
	if (ee != ACT_ESC || et != ACT_EOT) {
		if (et == ACT_ESC && (start + off) < remaining) {
			uint8_t next = in[(start + off)];
			if (next == ACT_EOT) {
				// Ended up with ACT_ESC, ACT_ESC, ACT_EOT - bad escaping?
				off++;
			} else if (debug) {
				fprintf(stderr, "Unexpected sequence at end of message: 0x%02x 0x%02x\n", ee, et);
			}
		}
	}

	(*pos) = start + off;
	if ((*msg)->csum != (uint8_t)(256 - csum)) {
		if (debug) {
			fprintf(stderr, "Bad checksum (%d => %d\tPGN %d)\n", (*msg)->src, (*msg)->dst, (*msg)->PGN);
		}
		return false; // Signal error, but send the message out
	}
	return true;
}
