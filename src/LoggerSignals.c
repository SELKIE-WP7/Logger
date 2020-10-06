#include <pthread.h>

#include "Logger.h"
#include "LoggerSignals.h"

//! Trigger clean software shutdown
volatile bool shutdown = false;

//! Trigger immediate log rotation
volatile bool rotateNow = false;

//! Pause logging
/*!
 * Will not rotate or close log files, but will stop reading from inputs while this variable is set.
 */
volatile bool pauseLog = false;

// Signal handlers documented in header file
void signalShutdown(int signnum __attribute__((unused))) {
	shutdown = true;
}

void signalRotate(int signnum __attribute__((unused))) {
	rotateNow = true;
}

void signalPause(int signnum __attribute__((unused))) {
	pauseLog = true;
}

void signalUnpause(int signnum __attribute__((unused))) {
	pauseLog = false;
}

void signalHandlersInstall() {
	sigset_t *hMask = signalHandlerMask();

	// While handling signals, we still want to pay attention to shutdown requests on more standard signals.
	// As it doesn't matter if multiple shutdown signals are received, we just clear these for all handlers.
	sigdelset(hMask, SIGINT);
	sigdelset(hMask, SIGQUIT);

	const struct sigaction saShutdown = {.sa_handler = signalShutdown, .sa_mask = *hMask, .sa_flags = SA_RESTART};
	const struct sigaction saRotate = {.sa_handler = signalRotate, .sa_mask = *hMask, .sa_flags = SA_RESTART};
	const struct sigaction saPause = {.sa_handler = signalPause, .sa_mask = *hMask, .sa_flags = SA_RESTART};
	const struct sigaction saUnpause = {.sa_handler = signalUnpause, .sa_mask = *hMask, .sa_flags = SA_RESTART};

	// As well as the "standard" signals, we use SIGRTMIN+n to make each handler available directly

	// Shutdown on SIGINT and SIGQUIT (standard) and SIGRTMIN+1
	sigaction(SIGINT, &saShutdown, NULL);
	sigaction(SIGQUIT, &saShutdown, NULL);
	sigaction(SIGRTMIN + 1, &saShutdown, NULL);

	// Log rotate on common signals (SIGUSR1, SIGHUP) and SIGRTMIN+2
	sigaction(SIGUSR1, &saRotate, NULL);
	sigaction(SIGHUP, &saRotate, NULL);
	sigaction(SIGRTMIN + 2, &saRotate, NULL);

	// No real standard pause/resume signals except SIGSTOP/CONT which the system handles
	sigaction(SIGRTMIN + 3, &saPause, NULL);
	sigaction(SIGRTMIN + 4, &saUnpause, NULL);

	free(hMask);
}

sigset_t *signalHandlerMask() {
	sigset_t *blocking = calloc(1, sizeof(sigset_t));

	// If any new signal handlers are added, they also need to be added to this list
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
