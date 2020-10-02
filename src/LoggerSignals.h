#ifndef SL_LOGGER_SIGS_H
#define SL_LOGGER_SIGS_H

#include <signal.h>
#include <stdbool.h>

/*!
 * \addtogroup loggerTriggers Logger: Global triggers/signal handlers
 *
 * In order to allow the behaviour of the main logging thread to be controlled,
 * it will check the state of defined trigger variables and react accordingly.
 * These variables can be set by signal handlers, input threads or by
 * conditions detected in the main logging loop.
 *
 * The signal handlers are hooked to multiple signals. The SIGRTMIN + n flags
 * are used to be independent of system defined meanings, while other signals
 * are hooked to match traditional service behaviours.
 *
 * @{
 */
//! Trigger clean software shutdown
static volatile bool shutdown = false;

//! Trigger immediate log rotation
static volatile bool rotateNow = false;

//! Pause logging
/*!
 * Will not rotate or close log files, but will stop reading from inputs while this variable is set.
 */
static volatile bool pauseLog = false;


/*! @brief Set safe shutdown flag
 *
 * By default, this is hooked to SIGINT, SIGQUIT and SIGRTMIN + 1
 */
void signalShutdown(int signnum);

/*! @brief Set log rotate flag
 *
 * By default, this is hooked to SIGUSR1, SIGHUP and SIGRTMIN + 2
 *
 * This flag will be cleared by the main loop once logs have been successfully
 * rotated. If the logger is paused, logs will be rotated when resumed.
 */
void signalRotate(int signnum);

/*! @brief Set logger pause flag
 *
 * By default, this is hooked to SIGRTMIN + 3
 * 
 * The pause flag can only be cleared by the unpause signal handler, or during shutdown.
 * Calling this function while already paused will have no additional effect.
 */
void signalPause(int signnum);

/*! @brief Clear logger pause flag
 *
 * By default, this is hooked to SIGRTMIN + 4
 *
 * Calling this while the logger is not paused has no effect.
 */
void signalUnpause(int signnum);
/*! @} */

#endif
