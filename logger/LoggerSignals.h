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

#ifndef SL_LOGGER_SIGS_H
#define SL_LOGGER_SIGS_H

#include <signal.h>
#include <stdatomic.h>
#include <stdbool.h>
//! @file

/*!
 * @addtogroup loggerTriggers Logger: Global triggers/signal handlers
 * @ingroup logger
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

extern atomic_bool shutdownFlag;
extern atomic_bool rotateNow;
extern atomic_bool pauseLog;

//! Set safe shutdown flag
void signalShutdown(int signnum);

//! Set log rotate flag
void signalRotate(int signnum);

//! Set logger pause flag
void signalPause(int signnum);

//! Clear logger pause flag
void signalUnpause(int signnum);
/*! @} */

//! Install signal handlers
void signalHandlersInstall(void);

//! Return a signal mask with suitable defaults
sigset_t *signalHandlerMask(void);

//! Block signals that we have handlers for
void signalHandlersBlock(void);

//! Unblock signals that we have handlers for
void signalHandlersUnblock(void);
#endif
