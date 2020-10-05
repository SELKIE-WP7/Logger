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
