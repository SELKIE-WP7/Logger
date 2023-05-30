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

#include <pthread.h>

#include "Logger.h"

#include "LoggerSignals.h"

/*!
 * @brief Trigger clean software shutdown
 *
 * Will end any running _logging functions and the main loop in Logger.c
 *
 * For obvious reasons, this flag is never reset.
 */
atomic_bool shutdownFlag = false;

/*!
 * @brief Trigger immediate log rotation
 *
 * Will be reset once files have been rotated, which may not happen immediately
 * if the logger is paused.
 */
atomic_bool rotateNow = false;

/*!
 * @brief Pause logging
 *
 * Will not rotate or close log files, but will stop reading from inputs while this
 * variable is set.
 */
atomic_bool pauseLog = false;

/*!
 * Called as a signal handler.
 *
 * By default, this is hooked to SIGINT, SIGQUIT and SIGRTMIN + 1
 *
 * @param[in] signnum Signal number - ignored
 */
void signalShutdown(int signnum __attribute__((unused))) {
	shutdownFlag = true;
}

/*!
 * Called as a signal handler.
 *
 * By default, this is hooked to SIGUSR1, SIGHUP and SIGRTMIN+2
 *
 * This flag will be cleared by the main loop once logs have been successfully
 * rotated. If the logger is paused, logs will be rotated when resumed.
 *
 * @param[in] signnum Signal number - ignored
 */
void signalRotate(int signnum __attribute__((unused))) {
	rotateNow = true;
}

/*!
 * Called as a signal handler.
 *
 * By default, this is hooked to SIGRTMIN + 3
 *
 * The pause flag can only be cleared by the unpause signal handler, or during shutdown.
 * Calling this function while already paused will have no additional effect.
 *
 * @param[in] signnum Signal number - ignored
 */
void signalPause(int signnum __attribute__((unused))) {
	pauseLog = true;
}

/*!
 * Called as a signal handler.
 *
 * By default, this is hooked to SIGRTMIN + 4
 *
 * Calling this while the logger is not paused has no effect.
 *
 * @param[in] signnum Signal number - ignored
 */
void signalUnpause(int signnum __attribute__((unused))) {
	pauseLog = false;
}

/*!
 * Called from main logging thread to establish the signal handlers.
 */
void signalHandlersInstall() {
	sigset_t *hMask = signalHandlerMask();

	/*
	 * While handling pause, unpause and rotate signals, we still want to
	 * pay attention to shutdown requests. As it doesn't matter if multiple
	 * shutdown signals are received, we just clear the mask for SIGINT and
	 * SIGQUIT.
	 */
	sigdelset(hMask, SIGINT);
	sigdelset(hMask, SIGQUIT);

	const struct sigaction saShutdown = {
		.sa_handler = signalShutdown, .sa_mask = *hMask, .sa_flags = SA_RESTART};
	const struct sigaction saRotate = {
		.sa_handler = signalRotate, .sa_mask = *hMask, .sa_flags = SA_RESTART};
	const struct sigaction saPause = {
		.sa_handler = signalPause, .sa_mask = *hMask, .sa_flags = SA_RESTART};
	const struct sigaction saUnpause = {
		.sa_handler = signalUnpause, .sa_mask = *hMask, .sa_flags = SA_RESTART};

	// As well as the "standard" signals, we use SIGRTMIN+n to make each handler
	// available directly

	// Shutdown signalled on SIGINT and SIGQUIT (standard) and SIGRTMIN+1
	sigaction(SIGINT, &saShutdown, NULL);
	sigaction(SIGQUIT, &saShutdown, NULL);
	sigaction(SIGRTMIN + 1, &saShutdown, NULL);

	// Log rotate on common signals (SIGUSR1, SIGHUP) and SIGRTMIN+2
	sigaction(SIGUSR1, &saRotate, NULL);
	sigaction(SIGHUP, &saRotate, NULL);
	sigaction(SIGRTMIN + 2, &saRotate, NULL);

	/*
	 * There isn't really a standard pause/resume signals except SIGSTOP/CONT which
	 * the system handles (and is somewhat more forceful). We use SIGRTMIN+3 and +4
	 * for pause and unpause respectively
	 */
	sigaction(SIGRTMIN + 3, &saPause, NULL);
	sigaction(SIGRTMIN + 4, &saUnpause, NULL);

	free(hMask);
}

/*!
 * Allocates and returns as signal mask that blocks all of the signals that we
 * have explicit handlers for.
 *
 * This can then be used to ensure that source handling threads do not respond
 * to signals, and that the main logging thread doesn't respond to any of these
 * signals until it's ready to do so.
 *
 * The signal set returned by this function must be freed by the caller.
 *
 * @returns Pointer to sigset_t
 */
sigset_t *signalHandlerMask() {
	sigset_t *blocking = calloc(1, sizeof(sigset_t));

	//! If any new signal handlers are added, they also need to be added to this list
	sigemptyset(blocking);
	sigaddset(blocking, SIGINT);
	sigaddset(blocking, SIGQUIT);
	sigaddset(blocking, SIGUSR1);
	sigaddset(blocking, SIGHUP);
	sigaddset(blocking, SIGRTMIN + 1);
	sigaddset(blocking, SIGRTMIN + 2);
	sigaddset(blocking, SIGRTMIN + 3);
	sigaddset(blocking, SIGRTMIN + 4);
	return blocking;
}

void signalHandlersBlock() {
	sigset_t *hMask = signalHandlerMask();
	pthread_sigmask(SIG_BLOCK, hMask, NULL);
	free(hMask);
}

void signalHandlersUnblock() {
	sigset_t *hMask = signalHandlerMask();
	pthread_sigmask(SIG_UNBLOCK, hMask, NULL);
	free(hMask);
}
