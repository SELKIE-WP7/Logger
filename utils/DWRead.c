#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "SELKIELoggerBase.h"
#include "SELKIELoggerDW.h"

#include "version.h"

int main(int argc, char *argv[]) {
	program_state state = {0};
	state.verbose = 1;

	dw_types inputType = DW_TYPE_UNKNOWN;

        char *usage =  "Usage: %1$s [-v] [-q] [-x] datfile\n"
                "\t-v\tIncrease verbosity\n"
		"\t-q\tDecrease verbosity\n"
		"\t-x\tInput file is in HVX format\n"
                "\nVersion: " GIT_VERSION_STRING "\n";

        opterr = 0; // Handle errors ourselves
        int go = 0;
	bool doUsage = false;
        while ((go = getopt(argc, argv, "vqx")) != -1) {
                switch(go) {
                        case 'v':
                                state.verbose++;
                                break;
			case 'q':
				state.verbose--;
				break;
			case 'x':
				inputType = DW_TYPE_HVX;
				break;
			case '?':
				log_error(&state, "Unknown option `-%c'", optopt);
				doUsage = true;
		}
        }

	// Should be 1 spare arguments: The file to convert
        if (argc - optind != 1) {
		log_error(&state, "Invalid arguments");
		doUsage = true;
        }

	if (doUsage) {
		fprintf(stderr, usage, argv[0]);
		return -1;
	}

	if (inputType == DW_TYPE_UNKNOWN) {
		log_error(&state, "File type must be specified");
		return 1;
	}

	char *inFileName = strdup(argv[optind]);
	FILE *inFile = fopen(inFileName, "rb");
	if (inFile == NULL) {
		log_error(&state, "Unable to open input file");
		return -1;
	}

	bool processing = true;
#define BUFSIZE 1024
	char buf[BUFSIZE] = {0};
	size_t hw = fread(buf, sizeof(char), BUFSIZE, inFile);
	uint16_t cycdata[20] = {0};
	uint8_t cycCount = 0;

	uint16_t sysdata[16] = {0};

	while (processing || hw > 25) {
		if (processing && (hw < BUFSIZE)) {
			ssize_t ret = fread(&(buf[hw]), sizeof(char), BUFSIZE - hw, inFile);
			if (ret < 1 || feof(inFile)) {
				processing = false;
			} else {
				hw += ret;
			}
		}

		size_t end = hw;
		dw_hxv tmp = {0};
		switch(inputType) {
			case DW_TYPE_HVX:
				// Read HXV
				if (dw_string_hxv(buf, &end, &tmp)) {
					cycdata[cycCount++] = dw_hxv_cycdat(&tmp);
					fprintf(stdout, "N: %+.2f\tW: %+.2f\tV: %+.2f\n", dw_hxv_north(&tmp)/100.0, dw_hxv_west(&tmp)/100.0, dw_hxv_vertical(&tmp)/100.0);
				}
				if (cycCount > 18) {
					for (int i = 0; i < cycCount; ++i) {
						if (cycdata[i] == 0x7FFF) {
							for (int j = 0; j < cycCount && (j+i) < 20; ++j) {
								cycdata[j] = cycdata[j+i];
							}
							cycCount -= i;
							break;
						}
					}
					if (cycCount < 18) {
						// Might have had a sync byte too close to the end
						break;
					}
					// TODO: print cycdata
					for (int c=0; c < 18; c++) {
						fprintf(stdout, "%04x%c", cycdata[c], c == 17 ? '\n' : ' ');
					}
					fprintf(stdout, "\n");

					uint8_t sysWord = (cycdata[1] & 0xF000) >> 12;
					fprintf(stdout, "Cyclic data collected, containing system data word 0x%02x\n", sysWord);
					sysdata[sysWord] = (cycdata[1] & 0x0FFF);

					uint8_t count = 0;
					for (int i = 0; i < 16; ++i) {
						if (sysdata[i]) {
							++count;
						} else {
							// First gap found, no point continuing to count
							break;
						}
					}
					if (count == 16) {
						fprintf(stdout, "System data found:\n");
						for (int s=0; s < 16; s++) {
							fprintf(stdout, "%04x%c", sysdata[s], s == 15 ? '\n' : ' ');
						}
						fprintf(stdout, "\n");
						memset(&sysdata, 0, 16*sizeof(uint16_t));
					}

					memset(&cycdata, 0, 18*sizeof(uint16_t));
					cycCount -= 18;

				}
				break;
			default:
				// Error!
				return -2;
		}
		memmove(buf, &(buf[end]), hw-end);
		hw -= end;
	}
	fclose(inFile);
	free(inFileName);
	destroy_program_state(&state);
	return 1;
}
