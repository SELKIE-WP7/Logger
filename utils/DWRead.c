#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "SELKIELoggerBase.h"
#include "SELKIELoggerDW.h"

#include "version.h"

/*!
 * @file
 * @brief Read messages from Datawell HVX file
 * @ingroup Executables
 */

//! Allocated buffer size
#define BUFSIZE 1024

/*!
 * Reads messages from a HVX file (or equivalent data extracted from a
 * SELKIELogger data file) and prints out a simple representation.
 *
 * @param[in] argc Argument count
 * @param[in] argv Arguments
 * @returns -1 or 1 on error, and 0 on success
 */
int main(int argc, char *argv[]) {
	program_state state = {0};
	state.verbose = 1;

	dw_types inputType = DW_TYPE_UNKNOWN;

	char *usage = "Usage: %1$s [-v] [-q] [-x] datfile\n"
		      "\t-v\tIncrease verbosity\n"
		      "\t-q\tDecrease verbosity\n"
		      "\t-x\tInput file is in HVX format\n"
		      "\nVersion: " GIT_VERSION_STRING "\n";

	opterr = 0; // Handle errors ourselves
	int go = 0;
	bool doUsage = false;
	while ((go = getopt(argc, argv, "vqx")) != -1) {
		switch (go) {
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

	char buf[BUFSIZE] = {0};
	size_t hw = fread(buf, sizeof(char), BUFSIZE, inFile);
	uint16_t cycdata[20] = {0};
	uint8_t cycCount = 0;

	bool sdset[16] = {0};
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
		switch (inputType) {
			case DW_TYPE_HVX:
				// Read HXV
				if (dw_string_hxv(buf, &end, &tmp)) {
					cycdata[cycCount++] = dw_hxv_cycdat(&tmp);
					fprintf(stdout,
					        "[cyc] Signal: %d, Displacements - N: %+.2f\tW: %+.2f\tV: %+.2f\n",
					        tmp.status, dw_hxv_north(&tmp) / 100.0,
					        dw_hxv_west(&tmp) / 100.0,
					        dw_hxv_vertical(&tmp) / 100.0);
				}
				if (cycCount > 18) {
					bool syncFound = false;
					for (int i = 0; i < cycCount; ++i) {
						if (cycdata[i] == 0x7FFF) {
							syncFound = true;
							if (i > 0) {
								// clang-format off
								for (int j = 0; j < cycCount && (j + i) < 20; ++j) {
									cycdata[j] = cycdata[j + i];
								}
								cycCount = cycCount - i;
								// clang-format on
							}
							break;
						}
					}
					if (!syncFound) {
						cycCount = 0;
						memset(cycdata, 0, 20 * sizeof(cycdata[0]));
						break;
					}

					dw_spectrum ds = {0};
					if (!dw_spectrum_from_array(cycdata, &ds)) {
						fprintf(stderr,
						        "Failed to extract spectrum data\n");
					} else {
						sysdata[ds.sysseq] = ds.sysword;
						sdset[ds.sysseq] = true;
						fprintf(stdout, "[spe] System data word %d\n",
						        ds.sysseq);
						fprintf(stdout,
						        "[spe] #\tBin\tFreq\tDir.\tSpread\tM2\tN2\tRPSD\tK\n");
						for (int n = 0; n < 4; ++n) {
							fprintf(stdout,
							        "[spe] %1d\t%02d\t%.3f\t%.2f\t%.2f\t%.3f\t%.3f\t%.3f\t%.3f\n",
							        n, ds.frequencyBin[n],
							        ds.frequency[n], ds.direction[n],
							        ds.spread[n], ds.m2[n], ds.n2[n],
							        ds.rpsd[n], ds.K[n]);
						}
						fprintf(stdout, "\n");
					}

					uint8_t count = 0;
					for (int i = 0; i < 16; ++i) {
						if (sdset[i]) {
							++count;
						} else {
							// First gap found
							break;
						}
					}

					if (count == 16) {
						dw_system dsys = {0};
						// clang-format off
						if (!dw_system_from_array(sysdata, &dsys)) {
							fprintf(stderr, "Failed to extract system data\n");
						} else {
							fprintf(stdout, "[sys] Transmission number %d\n", dsys.number);
							fprintf(stdout, "[sys] GPS position %s\n", dsys.GPSfix ? "OK" : "old or invalid");
							fprintf(stdout, "[sys] RMS Height: %.3f, Fz: %.3f\n", dsys.Hrms, dsys.fzero);
							fprintf(stdout, "[sys] Ref. Temp: %.1f'C, Water Temp: %.1f'C\n", dsys.refTemp, dsys.waterTemp);
							fprintf(stdout, "[sys] Expected operational time remaining: %d weeks\n", dsys.opTime);
							fprintf(stdout, "[sys] Accelerometer offsets: X: %.3f, Y: %.3f, Z: %.3f\n", dsys.a_x_off, dsys.a_y_off, dsys.a_z_off);
							{
								int latm = 1;
								char latc = 'N';
								int lonm = 1;
								char lonc = 'E';
								if (dsys.lat < 0) {
									latm = -1;
									latc = 'S';
								}
								if (dsys.lon < 0) {
									lonm = -1;
									lonc = 'W';
								}
								fprintf(stdout,
								        "[sys] Position: %.6f%c %.6f%c, Orientation: %.3f, Inclination: %.3f\n",
								        latm * dsys.lat, latc,
								        lonm * dsys.lon, lonc,
								        dsys.orient, dsys.incl);
								fprintf(stdout, "\n");
							}
							// clang-format on
						}
						memset(&sysdata, 0, 16 * sizeof(uint16_t));
						memset(&sdset, 0, 16 * sizeof(bool));
					}

					cycdata[0] = cycdata[18];
					cycdata[1] = cycdata[19];
					memset(&(cycdata[2]), 0, 18 * sizeof(uint16_t));
					cycCount = 2;
				}
				break;
			default:
				// Error!
				return -2;
		}
		memmove(buf, &(buf[end]), hw - end);
		hw -= end;
	}
	fclose(inFile);
	free(inFileName);
	destroy_program_state(&state);
	return 1;
}
