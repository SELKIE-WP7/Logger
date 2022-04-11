#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "N2KTypes.h"

/*!
 * Allocates an array of bytes and converts n2k_act_message into a
 * transmittable series of bytes
 *
 * Array will need to be freed by caller
 *
 * @param[in] act n2k_act_message to be packed
 * @param[out] out Pointer to array of bytes
 * @param[out] len Will be set to message length after conversion
 * @returns True on success, false on error
 */
bool n2k_act_to_bytes(const n2k_act_message *act, uint8_t **out, size_t *len) {
	if (act == NULL || out == NULL || len == NULL) { return false; }

	(*out) = calloc(act->length + 2 * act->datalen, sizeof(uint8_t));
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
	(*out) = realloc((*out), ix * sizeof(uint8_t));
	return true;
}

/*!
 *
 * Will allocate an n2k_act_message for output, which must be freed by caller.
 *
 * @param[in] in Array of bytes
 * @param[in] len Number of bytes available in array
 * @param[out] msg Pointer to n2k_act_message pointer for output
 * @param[out] pos Number of bytes consumed
 * @param[in] debug Set true for more verbose output
 * @returns True on success, false on error
 */
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
		if (debug) { fprintf(stderr, "N2K: No start marker found\n"); }
		return false;
	}
	if ((len - start) < in[start + 3]) {
		// Message claims to be larger than available data, so leave
		// (*pos) where it is and return false until we get more data
		if (debug) { fprintf(stderr, "N2K: Insufficient data\n"); }
		return false;
	}

	(*msg) = calloc(1, sizeof(n2k_act_message));
	if ((*msg) == NULL) {
		perror("n2k_act_from_bytes");
		return false;
	}

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
		if (debug) { fprintf(stderr, "N2K: Insufficient data to read in message content\n"); }
		return false;
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
				if (debug) { fprintf(stderr, "N2K: Premature Termination\n"); }
				return false;
			} else if (next == ACT_SOT) {
				// This....probably shouldn't happen.
				// Exit as above, but reposition to before the message start
				(*pos) = (start + off - 2);
				free((*msg)->data);
				free(*msg);
				(*msg) = NULL;
				if (debug) { fprintf(stderr, "N2K: Unexpected start of message marker\n"); }
				return false;
			} else {
				// Any other ESC + character sequence here is invalid
				(*pos) = (start + off);
				free((*msg)->data);
				free(*msg);
				(*msg) = NULL;
				if (debug) {
					fprintf(stderr, "N2K: Bad character escape sequence (ESC + 0x%02x\n", next);
				}
				return false;
			}
		} else {
			(*msg)->data[i] = c;
		}

		if (remaining < ((*msg)->datalen - i + 3)) {
			free((*msg)->data);
			free(*msg);
			(*msg) = NULL;
			if (debug) { fprintf(stderr, "N2K: Out of data while parsing\n"); }
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
	uint8_t cs = n2k_act_checksum(*msg);

	if ((*msg)->csum != cs) {
		if (debug) {
			fprintf(stderr, "Bad checksum (%d => %d\tPGN %d)\n", (*msg)->src, (*msg)->dst, (*msg)->PGN);
		}
		return false; // Signal error, but send the message out
	}
	return true;
}

/*!
 * @param[in] msg ntk_act_message input
 * @returns Unsigned byte containing checksum value
 */
uint8_t n2k_act_checksum(const n2k_act_message *msg) {
	uint8_t csum = ACT_N2K;
	csum += msg->length;
	csum += msg->priority;
	csum += (msg->PGN & 0x0000FF);
	csum += ((msg->PGN & 0x00FF00) >> 8);
	csum += ((msg->PGN & 0xFF0000) >> 16);
	csum += msg->dst;
	csum += msg->src;
	csum += (msg->timestamp & 0x000000FF);
	csum += ((msg->timestamp & 0x0000FF00) >> 8);
	csum += ((msg->timestamp & 0x00FF0000) >> 16);
	csum += ((msg->timestamp & 0xFF000000) >> 24);
	csum += (msg->datalen);

	for (int i = 0; i < msg->datalen; ++i) {
		csum += msg->data[i];
	};

	return 256 - csum;
}

/*!
 * Prints message as a sequence of bytes in hexadecimal form.
 *
 * @param[in] msg n2k_act_message to be printed
 */
void n2k_act_print(const n2k_act_message *msg) {
	uint8_t *tmp = NULL;
	size_t len = 0;
	n2k_act_to_bytes(msg, &tmp, &len);
	fprintf(stdout, "N2k ACT Message: ");
	for (int j = 0; j < len; ++j) {
		fprintf(stdout, "%c%02x", j > 0 ? ':' : ' ', tmp[j]);
	}
	fprintf(stdout, "\n");
}
