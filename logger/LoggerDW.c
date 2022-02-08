#include "Logger.h"

#include "LoggerDW.h"
#include "LoggerNet.h"
#include "LoggerSignals.h"

#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>

#include "SELKIELoggerDW.h"

/*!
 * Opens a network connection
 *
 * It is assumed that no other setup is required for these devices.
 *
 * @param ptargs Pointer to log_thread_args_t
 */
void *dw_setup(void *ptargs) {
	log_thread_args_t *args = (log_thread_args_t *)ptargs;

	// Delegate connection logic to net_ functions
	if (!dw_net_connect(ptargs)) {
		log_error(args->pstate, "[DW:%s] Unable to open a connection", args->tag);
		args->returnCode = -1;
		return NULL;
	}

	log_info(args->pstate, 2, "[DW:%s] Connected", args->tag);
	args->returnCode = 0;
	return NULL;
}

/*!
 * Reads messages from the connection established by dw_setup(), and pushes them to the
 * queue. As messages are already in the right format, no further processing is done here.
 *
 * @param ptargs Pointer to log_thread_args_t
 */
void *dw_logging(void *ptargs) {
	signalHandlersBlock();
	log_thread_args_t *args = (log_thread_args_t *)ptargs;
	dw_params *dwInfo = (dw_params *)args->dParams;

	log_info(args->pstate, 1, "[DW:%s] Logging thread started", args->tag);

	const size_t bufSize = 1024;
	uint8_t *buf = calloc(bufSize, sizeof(uint8_t));
	int dw_hw = 0;
	time_t lastRead = time(NULL);
	time_t lastGoodSignal = time(NULL);

	uint16_t cycdata[20] = {0};
	uint8_t cCount = 0;

	bool sdset[16] = {0};
	uint16_t sysdata[16] = {0};
	while (!shutdownFlag) {
		time_t now = time(NULL);
		if ((lastRead + dwInfo->timeout) < now) {
			log_warning(args->pstate, "[DW:%s] Network timeout, reconnecting",
			            args->tag);
			close(dwInfo->handle);
			dwInfo->handle = -1;
			errno = 0;
			if (net_connect(args)) {
				log_info(args->pstate, 1, "[DW:%s] Reconnected", args->tag);
			} else {
				log_error(args->pstate, "[DW:%s] Unable to reconnect: %s",
				          args->tag, strerror(errno));
				args->returnCode = -2;
				pthread_exit(&(args->returnCode));
				return NULL;
			}
		}

		int ti = 0;
		if (dw_hw < bufSize - 1) {
			errno = 0;
			ti = read(dwInfo->handle, &(buf[dw_hw]), bufSize - dw_hw);
			if (ti >= 0) {
				dw_hw += ti;
				if (ti > 0) {
					// 0 may not be an error, but could be a dropped
					// connection if it persists
					lastRead = now;
				}
			} else {
				if (errno != EAGAIN) {
					// clang-format off
					log_error(args->pstate, "[DW:%s] Unexpected error while reading from network (%s)",
					          args->tag, strerror(errno));
					// clang-format on
					args->returnCode = -1;
					pthread_exit(&(args->returnCode));
				}
			}
		}

		if (dw_hw < 25) {
			// Sleep briefly, then loop until we have more than the minimum
			// number of bytes available
			usleep(5E4);
			continue;
		}
		/////////// Message parsing
		size_t end = dw_hw;
		dw_hxv tmp = {0};
		if (dw_string_hxv((char *)buf, &end, &tmp)) {
			uint16_t cd = dw_hxv_cycdat(&tmp);
			// Parse and queue
			dw_push_message(args, dwInfo->sourceNum, DWCHAN_SIG, tmp.status);
			if (tmp.status < 2) {
				lastGoodSignal = now;
				dw_push_message(args, dwInfo->sourceNum, DWCHAN_DN,
				                dw_hxv_north(&tmp));
				dw_push_message(args, dwInfo->sourceNum, DWCHAN_DW,
				                dw_hxv_west(&tmp));
				dw_push_message(args, dwInfo->sourceNum, DWCHAN_DV,
				                dw_hxv_vertical(&tmp));
				cycdata[cCount++] = cd;
			}
		}

		if ((now - lastGoodSignal) > 300) {
			log_warning(args->pstate, "[DW:%s] No valid data received from buoy",
			            args->tag);
			// Reset timer for another 5 minutes
			lastGoodSignal = now;
		}

		if (cCount > 18) {
			bool syncFound = false;
			for (int i = 0; i < cCount; ++i) {
				if (cycdata[i] == 0x7FFF) {
					syncFound = true;
					if (i > 0) {
						for (int j = 0; j < cCount && (j + i) < 20; ++j) {
							cycdata[j] = cycdata[j + i];
						}
						cCount = cCount - i;
					}
					break; // End search
				}
			}
			if (!syncFound) {
				cCount = 0;
				memset(cycdata, 0, 20 * sizeof(cycdata[0]));
				continue; // Continue main processing loop
			}

			dw_spectrum ds = {0};
			if (!dw_spectrum_from_array(cycdata, &ds)) {
				log_info(args->pstate, 2, "[DW:%s] Invalid spectrum data\n",
				         args->tag);
			} else {
				sysdata[ds.sysseq] = ds.sysword;
				sdset[ds.sysseq] = true;
				// Parse and queue frequency data?
				if (dwInfo->parseSpectrum) {
					for (int n = 0; n < 4; ++n) {
						// clang-format off
						dw_push_message(args, dwInfo->sourceNum, DWCHAN_SPF, ds.frequencyBin[n]);
						dw_push_message(args, dwInfo->sourceNum, DWCHAN_SPD, ds.direction[n]);
						dw_push_message(args, dwInfo->sourceNum, DWCHAN_SPS, ds.spread[n]);
						dw_push_message(args, dwInfo->sourceNum, DWCHAN_SPM, ds.m2[n]);
						dw_push_message(args, dwInfo->sourceNum, DWCHAN_SPN, ds.n2[n]);
						dw_push_message(args, dwInfo->sourceNum, DWCHAN_SPR, ds.rpsd[n]);
						dw_push_message(args, dwInfo->sourceNum, DWCHAN_SPK, ds.K[n]);
						// clang-format on
					}
				}
			}

			uint8_t count = 0;
			for (int i = 0; i < 16; ++i) {
				if (sdset[i]) {
					++count;
				} else {
					// First gap found, no point continuing to count
					break;
				}
			}

			if (count == 16) {
				dw_system dsys = {0};
				if (!dw_system_from_array(sysdata, &dsys)) {
					log_info(args->pstate, 2, "[DW:%s] Invalid system data\n",
					         args->tag);
				} else {
					// Parse system data and queue
					// clang-format off
					dw_push_message(args, dwInfo->sourceNum, DWCHAN_LAT, dsys.lat);
					dw_push_message(args, dwInfo->sourceNum, DWCHAN_LON, dsys.lon);
					dw_push_message(args, dwInfo->sourceNum, DWCHAN_ORIENT, dsys.orient);
					dw_push_message(args, dwInfo->sourceNum, DWCHAN_INCLIN, dsys.incl);
					dw_push_message(args, dwInfo->sourceNum, DWCHAN_GPSFIX, dsys.GPSfix);
					dw_push_message(args, dwInfo->sourceNum, DWCHAN_HRMS, dsys.Hrms);
					dw_push_message(args, dwInfo->sourceNum, DWCHAN_TREF, dsys.refTemp);
					dw_push_message(args, dwInfo->sourceNum, DWCHAN_TWTR, dsys.waterTemp);
					dw_push_message(args, dwInfo->sourceNum, DWCHAN_WEEKS, dsys.opTime);
					// clang-format on
				}
				memset(&sysdata, 0, 16 * sizeof(uint16_t));
				memset(&sdset, 0, 16 * sizeof(bool));
			}

			cycdata[0] = cycdata[18];
			cycdata[1] = cycdata[19];
			memset(&(cycdata[2]), 0, 18 * sizeof(uint16_t));
			cCount = 2;
		}

		msg_t *sm = msg_new_bytes(dwInfo->sourceNum, 3, dw_hw, buf);
		if (!queue_push(args->logQ, sm)) {
			log_error(args->pstate, "[DW:%s] Error pushing message to queue",
			          args->tag);
			msg_destroy(sm);
			args->returnCode = -1;
			pthread_exit(&(args->returnCode));
		}
		dw_hw = 0;
		memset(buf, 0, bufSize);
	}
	free(buf);
	pthread_exit(NULL);
	return NULL; // Superfluous, as returning zero via pthread_exit above
}

/*!
 * Terminates thread in the event of an error
 */
void dw_push_message(log_thread_args_t *args, uint8_t sNum, uint8_t cNum, float data) {
	msg_t *mm = msg_new_float(sNum, cNum, data);
	if (mm == NULL) {
		log_error(args->pstate, "[DW:%s] Unable to allocate message", args->tag);
		args->returnCode = -1;
		pthread_exit(&(args->returnCode));
	}
	if (!queue_push(args->logQ, mm)) {
		log_error(args->pstate, "[DW:%s] Error pushing data to queue", args->tag);
		msg_destroy(mm);
		args->returnCode = -1;
		pthread_exit(&(args->returnCode));
	}
}

/*!
 * Cleanly shutdown network connection
 *
 * @param ptargs Pointer to log_thread_args_t
 */
void *dw_shutdown(void *ptargs) {
	log_thread_args_t *args = (log_thread_args_t *)ptargs;
	dw_params *dwInfo = (dw_params *)args->dParams;
	if (dwInfo->handle >= 0) { // Admittedly 0 is unlikely
		shutdown(dwInfo->handle, SHUT_RDWR);
		close(dwInfo->handle);
	}
	dwInfo->handle = -1;
	if (dwInfo->addr) {
		free(dwInfo->addr);
		dwInfo->addr = NULL;
	}
	if (dwInfo->sourceName) {
		free(dwInfo->sourceName);
		dwInfo->sourceName = NULL;
	}
	return NULL;
}

bool dw_net_connect(void *ptargs) {
	log_thread_args_t *args = (log_thread_args_t *)ptargs;
	dw_params *dwInfo = (dw_params *)args->dParams;

	if (dwInfo->addr == NULL) {
		log_error(args->pstate, "[DW:%s] Bad connection details provided", args->tag);
		return false;
	}

	// If we already have an open handle, try and close it
	if (dwInfo->handle >= 0) { // Admittedly 0 is unlikely
		shutdown(dwInfo->handle, SHUT_RDWR);
		close(dwInfo->handle);
	}

	dwInfo->handle = -1;

	struct sockaddr_in targetSA;

	/* Create the socket. */
	errno = 0;
	dwInfo->handle = socket(PF_INET, SOCK_STREAM, 0);

	struct hostent *hostinfo;
	targetSA.sin_family = AF_INET;
	targetSA.sin_port = htons(1180);
	hostinfo = gethostbyname(dwInfo->addr);

	if (hostinfo == NULL) {
		perror("dw_net_connect(hostinfo)");
		return false;
	}
	targetSA.sin_addr = *(struct in_addr *)hostinfo->h_addr;

	errno = 0;
	int rs = connect(dwInfo->handle, (struct sockaddr *)&targetSA, sizeof(targetSA));
	if ((rs < 0) && (errno != EINPROGRESS)) {
		perror("dw_net_connect(connect)");
		return false;
	}

	// clang-format off
	if (fcntl(dwInfo->handle, F_SETFL, fcntl(dwInfo->handle, F_GETFL, NULL) | O_NONBLOCK) < 0) {
		perror("dw_net_connect(fcntl)");
		return false;
	}
	int enable = 1;
	if (setsockopt(dwInfo->handle, IPPROTO_TCP, TCP_NODELAY, (void *)&enable, sizeof(enable))) {
		perror("dw_net_connect(NODELAY)");
		return false;
	}
	// clang-format on
	return true;
}

device_callbacks dw_getCallbacks() {
	device_callbacks cb = {.startup = &dw_setup,
	                       .logging = &dw_logging,
	                       .shutdown = &dw_shutdown,
	                       .channels = &dw_channels};
	return cb;
}

dw_params dw_getParams() {
	dw_params dw = {.addr = NULL,
	                // .port = 1180,
	                .handle = -1,
	                .timeout = 60,
	                .recordRaw = true,
	                .parseSpectrum = false};
	return dw;
}

/*!
 * Populate list of channels and push to queue as a map message
 */
void *dw_channels(void *ptargs) {
	log_thread_args_t *args = (log_thread_args_t *)ptargs;
	dw_params *dwInfo = (dw_params *)args->dParams;

	msg_t *m_sn = msg_new_string(dwInfo->sourceNum, SLCHAN_NAME, strlen(dwInfo->sourceName),
	                             dwInfo->sourceName);

	if (!queue_push(args->logQ, m_sn)) {
		log_error(args->pstate, "[DW:%s] Error pushing channel name to queue", args->tag);
		msg_destroy(m_sn);
		args->returnCode = -1;
		pthread_exit(&(args->returnCode));
	}

	int nChans = 16;
	if (dwInfo->parseSpectrum) { nChans = 24; }

	strarray *channels = sa_new(nChans);
	sa_create_entry(channels, SLCHAN_NAME, 4, "Name");
	sa_create_entry(channels, SLCHAN_MAP, 8, "Channels");
	sa_create_entry(channels, SLCHAN_TSTAMP, 9, "Timestamp");
	sa_create_entry(channels, SLCHAN_RAW, 8, "Raw Data");
	/*
	 * Cyclic data:
	 */
	sa_create_entry(channels, DWCHAN_SIG, 6, "Signal");
	sa_create_entry(channels, DWCHAN_DN, 14, "Displacement N");
	sa_create_entry(channels, DWCHAN_DW, 14, "Displacement W");
	sa_create_entry(channels, DWCHAN_DV, 14, "Displacement V");
	/*
	 * System data:
	 */
	sa_create_entry(channels, DWCHAN_LAT, 8, "Latitude");
	sa_create_entry(channels, DWCHAN_LON, 9, "Longitude");
	sa_create_entry(channels, DWCHAN_ORIENT, 11, "Orientation");
	sa_create_entry(channels, DWCHAN_INCLIN, 11, "Inclination");
	sa_create_entry(channels, DWCHAN_GPSFIX, 10, "GPS Status");
	sa_create_entry(channels, DWCHAN_HRMS, 10, "RMS Height");
	sa_create_entry(channels, DWCHAN_TREF, 16, "Ref. Temperature");
	sa_create_entry(channels, DWCHAN_TWTR, 17, "Water Temperature");
	sa_create_entry(channels, DWCHAN_WEEKS, 15, "Weeks Remaining");
	/*
	 * Spectral data:
	 */
	if (dwInfo->parseSpectrum) {
		sa_create_entry(channels, DWCHAN_SPF, 15, "Sp-FrequencyBin");
		sa_create_entry(channels, DWCHAN_SPD, 16, "Sp-Direction");
		sa_create_entry(channels, DWCHAN_SPS, 9, "Sp-Spread");
		sa_create_entry(channels, DWCHAN_SPM, 5, "Sp-m2");
		sa_create_entry(channels, DWCHAN_SPN, 5, "Sp-n2");
		sa_create_entry(channels, DWCHAN_SPR, 7, "Sp-RPSD");
		sa_create_entry(channels, DWCHAN_SPK, 4, "Sp-K");
	}
	msg_t *m_cmap = msg_new_string_array(dwInfo->sourceNum, SLCHAN_MAP, channels);

	if (!queue_push(args->logQ, m_cmap)) {
		log_error(args->pstate, "[DW:%s] Error pushing channel map to queue", args->tag);
		msg_destroy(m_cmap);
		sa_destroy(channels);
		free(channels);
		args->returnCode = -1;
		pthread_exit(&(args->returnCode));
	}

	sa_destroy(channels);
	free(channels);
	return NULL;
}

bool dw_parseConfig(log_thread_args_t *lta, config_section *s) {
	if (lta->dParams) {
		log_error(lta->pstate, "[DW:%s] Refusing to reconfigure", lta->tag);
		return false;
	}

	dw_params *dw = calloc(1, sizeof(dw_params));
	if (!dw) {
		log_error(lta->pstate, "[DW:%s] Unable to allocate memory for device parameters",
		          lta->tag);
		return false;
	}
	(*dw) = dw_getParams();

	config_kv *t = NULL;
	if ((t = config_get_key(s, "host"))) { dw->addr = config_qstrdup(t->value); }
	t = NULL;

	if ((t = config_get_key(s, "name"))) {
		dw->sourceName = config_qstrdup(t->value);
	} else {
		// Must set a name, so nick the tag value
		dw->sourceName = strdup(lta->tag);
	}
	t = NULL;

	if ((t = config_get_key(s, "sourcenum"))) {
		errno = 0;
		int sn = strtol(t->value, NULL, 0);
		if (errno) {
			log_error(lta->pstate, "[DW:%s] Error parsing source number: %s", lta->tag,
			          strerror(errno));
			return false;
		}
		if (sn < 0) {
			log_error(lta->pstate, "[DW:%s] Invalid source number (%s)", lta->tag,
			          t->value);
			return false;
		}
		if (sn < 10) {
			dw->sourceNum += sn;
		} else {
			dw->sourceNum = sn;
			if (sn < SLSOURCE_EXT || sn > (SLSOURCE_EXT + 0x0F)) {
				log_warning(
					lta->pstate,
					"[DW:%s] Unexpected Source ID number (0x%02x)- this may cause analysis problems",
					lta->tag, sn);
			}
		}
	}

	if ((t = config_get_key(s, "timeout"))) {
		errno = 0;
		dw->timeout = strtol(t->value, NULL, 0);
		if (errno) {
			log_error(lta->pstate, "[DW:%s] Error parsing timeout: %s", lta->tag,
			          strerror(errno));
			return false;
		}

		if (dw->timeout <= 0) {
			log_error(lta->pstate,
			          "[DW:%s] Invalid timeout value (%d is not greater than zero)",
			          lta->tag, dw->timeout);
			return false;
		}
	}
	t = NULL;

	if ((t = config_get_key(s, "raw"))) {
		errno = 0;
		int tmp = config_parse_bool(t->value);
		if (tmp < 0) {
			log_error(lta->pstate, "[DW:%s] Invalid value provided for 'raw': %s",
			          lta->tag, t->value);
			return false;
		}
		dw->recordRaw = (tmp == 1);
	}
	t = NULL;

	if ((t = config_get_key(s, "spectrum"))) {
		errno = 0;
		int tmp = config_parse_bool(t->value);
		if (tmp < 0) {
			log_error(lta->pstate, "[DW:%s] Invalid value provided for 'spectrum': %s",
			          lta->tag, t->value);
			return false;
		}
		dw->parseSpectrum = (tmp == 1);
	}
	t = NULL;
	lta->dParams = dw;
	return true;
}
