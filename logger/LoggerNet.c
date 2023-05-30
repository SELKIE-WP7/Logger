/*
 *  Copyright (C) 2023 Swansea University
 *
 *  This file is part of the SELKIELogger suite of tools.
 *
 *  SELKIELogger is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the Free
 *  Software Foundation, either version 3 of the License, or (at your option)
 *  any later version.
 *
 *  SELKIELogger is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 *  more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this SELKIELogger product.
 *  If not, see <http://www.gnu.org/licenses/>.
*/

#include "Logger.h"

#include "LoggerNet.h"
#include "LoggerSignals.h"

#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>

/*!
 * The actual work is delegated to net_connect() so we can reuse the logic
 * elsewhere
 *
 * It is assumed that no other setup is required for these devices.
 *
 * @param[in] ptargs Pointer to log_thread_args_t
 * @returns NULL - Exit code in ptargs->returnCode if required
 */
void *net_setup(void *ptargs) {
	log_thread_args_t *args = (log_thread_args_t *)ptargs;
	// net_params *netInfo = (net_params *) args->dParams;

	if (!net_connect(ptargs)) {
		log_error(args->pstate, "[Network:%s] Unable to open a connection", args->tag);
		args->returnCode = -1;
		return NULL;
	}

	log_info(args->pstate, 2, "[Network:%s] Connected", args->tag);
	args->returnCode = 0;
	return NULL;
}

/*!
 * Reads messages from the connection established by net_setup(), and pushes them to the
 * queue. Data is not interpreted, just pushed into the queue with suitable headers.
 *
 * Message size is variable, based on min/max limits and the amount of data
 * available to read from the source.
 *
 * @param[in] ptargs Pointer to log_thread_args_t
 * @returns NULL - Exit code in ptargs->returnCode if required
 */
void *net_logging(void *ptargs) {
	signalHandlersBlock();
	log_thread_args_t *args = (log_thread_args_t *)ptargs;
	net_params *netInfo = (net_params *)args->dParams;

	log_info(args->pstate, 1, "[Network:%s] Logging thread started", args->tag);

	uint8_t *buf = calloc(netInfo->maxBytes, sizeof(uint8_t));
	int net_hw = 0;
	time_t lastRead = time(NULL);
	while (!shutdownFlag) {
		time_t now = time(NULL);
		if ((lastRead + netInfo->timeout) < now) {
			log_warning(args->pstate, "[Network:%s] Network timeout, reconnecting",
			            args->tag);
			close(netInfo->handle);
			netInfo->handle = -1;
			errno = 0;
			if (net_connect(args)) {
				log_info(args->pstate, 1, "[Network:%s] Reconnected", args->tag);
			} else {
				log_error(args->pstate, "[Network:%s] Unable to reconnect: %s",
				          args->tag, strerror(errno));
				args->returnCode = -2;
				pthread_exit(&(args->returnCode));
				return NULL;
			}
		}

		int ti = 0;
		if (net_hw < netInfo->maxBytes - 1) {
			errno = 0;
			ti = read(netInfo->handle, &(buf[net_hw]), netInfo->maxBytes - net_hw);
			if (ti >= 0) {
				net_hw += ti;
				if (ti > 0) {
					// 0 may not be an error, but could be a dropped
					// connection if it persists
					lastRead = now;
				}
			} else {
				if (errno != EAGAIN) {
					log_error(
						args->pstate,
						"[Network:%s] Unexpected error while reading from network (%s)",
						args->tag, strerror(errno));
					args->returnCode = -1;
					pthread_exit(&(args->returnCode));
				}
			}
		}

		if (net_hw < netInfo->minBytes) {
			// Sleep briefly, then loop until we have more than the minimum
			// number of bytes available
			usleep(5E4);
			continue;
		}

		msg_t *sm = msg_new_bytes(netInfo->sourceNum, 3, net_hw, buf);
		if (!queue_push(args->logQ, sm)) {
			log_error(args->pstate, "[Network:%s] Error pushing message to queue",
			          args->tag);
			msg_destroy(sm);
			args->returnCode = -1;
			pthread_exit(&(args->returnCode));
		}
		net_hw = 0;
		memset(buf, 0, netInfo->maxBytes);
	}
	free(buf);
	pthread_exit(NULL);
	return NULL; // Superfluous, as returning zero via pthread_exit above
}

/*!
 * Simple wrapper around net_closeConnection(), which will do any cleanup required.
 *
 * @param[in] ptargs Pointer to log_thread_args_t
 * @returns NULL
 */
void *net_shutdown(void *ptargs) {
	log_thread_args_t *args = (log_thread_args_t *)ptargs;
	net_params *netInfo = (net_params *)args->dParams;

	if (netInfo->handle >= 0) { // Admittedly 0 is unlikely
		shutdown(netInfo->handle, SHUT_RDWR);
		close(netInfo->handle);
	}
	netInfo->handle = -1;
	if (netInfo->addr) {
		free(netInfo->addr);
		netInfo->addr = NULL;
	}
	if (netInfo->sourceName) {
		free(netInfo->sourceName);
		netInfo->sourceName = NULL;
	}
	return NULL;
}

/*!
 * Closes any existing connection, then attempts to open a new connection using
 * the details in net_params.
 *
 * Socket is set as nonblocking, so zero-length reads are to be expected.
 *
 * @param[in] ptargs Pointer to log_thread_args_t
 * @returns True on success, false on error
 */
bool net_connect(void *ptargs) {
	log_thread_args_t *args = (log_thread_args_t *)ptargs;
	net_params *netInfo = (net_params *)args->dParams;

	if (netInfo->addr == NULL || netInfo->port <= 0) {
		log_error(args->pstate, "[Network:%s] Bad connection details provided", args->tag);
		return false;
	}

	// If we already have an open handle, try and close it
	if (netInfo->handle >= 0) { // Admittedly 0 is unlikely
		shutdown(netInfo->handle, SHUT_RDWR);
		close(netInfo->handle);
	}

	netInfo->handle = -1;

	struct sockaddr_in targetSA;

	/* Create the socket. */
	errno = 0;
	netInfo->handle = socket(PF_INET, SOCK_STREAM, 0);

	struct hostent *hostinfo;
	targetSA.sin_family = AF_INET;
	targetSA.sin_port = htons(netInfo->port);
	hostinfo = gethostbyname(netInfo->addr);

	if (hostinfo == NULL) {
		perror("net_connect(hostinfo)");
		return false;
	}
	targetSA.sin_addr = *(struct in_addr *)hostinfo->h_addr;

	errno = 0;
	int rs = connect(netInfo->handle, (struct sockaddr *)&targetSA, sizeof(targetSA));
	if ((rs < 0) && (errno != EINPROGRESS)) {
		perror("net_connect(connect)");
		return false;
	}

	// clang-format off
	if (fcntl(netInfo->handle, F_SETFL, fcntl(netInfo->handle, F_GETFL, NULL) | O_NONBLOCK) < 0) {
		perror("net_connect(fcntl)");
		return false;
	}
	// clang-format on
	int enable = 1;
	if (setsockopt(netInfo->handle, IPPROTO_TCP, TCP_NODELAY, (void *)&enable,
	               sizeof(enable))) {
		perror("net_connect(NODELAY)");
		return false;
	}

	return true;
}

/*!
 * @returns device_callbacks for generic network sources
 */
device_callbacks net_getCallbacks() {
	device_callbacks cb = {.startup = &net_setup,
	                       .logging = &net_logging,
	                       .shutdown = &net_shutdown,
	                       .channels = &net_channels};
	return cb;
}

/*!
 * @returns Default parameters for generic network sources
 */
net_params net_getParams() {
	net_params mp = {.addr = NULL,
	                 .port = -1,
	                 .handle = -1,
	                 .minBytes = 10,
	                 .maxBytes = 1024,
	                 .timeout = 60};
	return mp;
}

/*!
 * Populate list of channels and push to queue as a map message
 *
 * @param[in] ptargs Pointer to log_thread_args_t
 * @returns NULL - Exit code in ptargs->returnCode if required
 */
void *net_channels(void *ptargs) {
	log_thread_args_t *args = (log_thread_args_t *)ptargs;
	net_params *netInfo = (net_params *)args->dParams;

	msg_t *m_sn = msg_new_string(netInfo->sourceNum, SLCHAN_NAME, strlen(netInfo->sourceName),
	                             netInfo->sourceName);

	if (!queue_push(args->logQ, m_sn)) {
		log_error(args->pstate, "[Network:%s] Error pushing channel name to queue",
		          args->tag);
		msg_destroy(m_sn);
		args->returnCode = -1;
		pthread_exit(&(args->returnCode));
	}

	strarray *channels = sa_new(4);
	sa_create_entry(channels, SLCHAN_NAME, 4, "Name");
	sa_create_entry(channels, SLCHAN_MAP, 8, "Channels");
	sa_create_entry(channels, SLCHAN_TSTAMP, 9, "Timestamp");
	sa_create_entry(channels, SLCHAN_RAW, 8, "Raw Data");

	msg_t *m_cmap = msg_new_string_array(netInfo->sourceNum, SLCHAN_MAP, channels);

	if (!queue_push(args->logQ, m_cmap)) {
		log_error(args->pstate, "[Network:%s] Error pushing channel map to queue",
		          args->tag);
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

/*!
 * @param[in] lta Pointer to log_thread_args_t
 * @param[in] s Pointer to config_section to be parsed
 * @returns True on success, false on error
 */
bool net_parseConfig(log_thread_args_t *lta, config_section *s) {
	if (lta->dParams) {
		log_error(lta->pstate, "[Network:%s] Refusing to reconfigure", lta->tag);
		return false;
	}

	net_params *net = calloc(1, sizeof(net_params));
	if (!net) {
		log_error(lta->pstate,
		          "[Network:%s] Unable to allocate memory for device parameters",
		          lta->tag);
		return false;
	}
	(*net) = net_getParams();

	config_kv *t = NULL;
	if ((t = config_get_key(s, "host"))) { net->addr = config_qstrdup(t->value); }
	t = NULL;

	if ((t = config_get_key(s, "port"))) {
		errno = 0;
		net->port = strtol(t->value, NULL, 0);
		if (errno) {
			log_error(lta->pstate, "[Network:%s] Error parsing port number: %s",
			          lta->tag, strerror(errno));
			free(net);
			return false;
		}
	}
	t = NULL;

	if ((t = config_get_key(s, "name"))) {
		net->sourceName = config_qstrdup(t->value);
	} else {
		// Must set a name, so nick the tag value
		net->sourceName = strdup(lta->tag);
	}
	t = NULL;

	if ((t = config_get_key(s, "sourcenum"))) {
		errno = 0;
		int sn = strtol(t->value, NULL, 0);
		if (errno) {
			log_error(lta->pstate, "[Network:%s] Error parsing source number: %s",
			          lta->tag, strerror(errno));
			free(net);
			return false;
		}
		if (sn < 0) {
			log_error(lta->pstate, "[Network:%s] Invalid source number (%s)", lta->tag,
			          t->value);
			free(net);
			return false;
		}
		if (sn < 10) {
			net->sourceNum += sn;
		} else {
			net->sourceNum = sn;
			if (sn < SLSOURCE_EXT || sn > (SLSOURCE_EXT + 0x0F)) {
				log_warning(
					lta->pstate,
					"[Network:%s] Unexpected Source ID number (0x%02x)- this may cause analysis problems",
					lta->tag, sn);
			}
		}
	}

	if ((t = config_get_key(s, "minbytes"))) {
		errno = 0;
		net->minBytes = strtol(t->value, NULL, 0);
		if (errno) {
			log_error(lta->pstate,
			          "[Network:%s] Error parsing minimum message size: %s", lta->tag,
			          strerror(errno));
			free(net);
			return false;
		}
		if (net->minBytes <= 0) {
			log_error(
				lta->pstate,
				"[Network:%s] Invalid minimum packet size specified (%d is not greater than zero)",
				lta->tag, net->minBytes);
			free(net);
			return false;
		}
	}
	t = NULL;

	if ((t = config_get_key(s, "maxbytes"))) {
		errno = 0;
		net->maxBytes = strtol(t->value, NULL, 0);
		if (errno) {
			log_error(lta->pstate,
			          "[Network:%s] Error parsing maximum message size: %s", lta->tag,
			          strerror(errno));
			free(net);
			return false;
		}

		if (net->maxBytes <= 0) {
			log_error(
				lta->pstate,
				"[Network:%s] Invalid maximum packet size specified (%d is not greater than zero)",
				lta->tag, net->maxBytes);
			free(net);
			return false;
		}

		if (net->maxBytes < net->minBytes) {
			log_error(
				lta->pstate,
				"[Network:%s] Invalid maximum packet size specified (%d is smaller than specified minimum packet size)",
				lta->tag, net->maxBytes);
			free(net);
			return false;
		}
	}
	t = NULL;

	if ((t = config_get_key(s, "timeout"))) {
		errno = 0;
		net->timeout = strtol(t->value, NULL, 0);
		if (errno) {
			log_error(lta->pstate, "[Network:%s] Error parsing timeout: %s", lta->tag,
			          strerror(errno));
			free(net);
			return false;
		}

		if (net->timeout <= 0) {
			log_error(
				lta->pstate,
				"[Network:%s] Invalid timeout value (%d is not greater than zero)",
				lta->tag, net->timeout);
			free(net);
			return false;
		}
	}
	t = NULL;
	lta->dParams = net;
	return true;
}
