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


#include "SELKIELoggerBase.h"

/*! @file logTests.c
 *
 * @brief Test logging functions
 *
 * @test Check that various combinations of log states and parameters are
 * correctly handled for code coverage and memory checking purposes. Checks are
 * carried out external to the program itself.
 *
 * @ingroup testing
 */

/*!
 * Check log functions
 *
 * @returns 0 (Pass) unconditionally
 * */
int main(void) {
	program_state state = {0};
	state.verbose = 0;
	state.log = fopen("/dev/null", "w");
	log_info(&state, 1, "Hidden message 1");
	log_info(&state, 0, "Displayed message 1");
	log_warning(&state, "Warning message during startup");
	log_error(&state, "Error message during startup");
	state.started = true;
	state.verbose++;
	log_info(&state, 1, "Running state");
	log_info(&state, 2, "Currently running with verbosity %d", state.verbose);
	log_warning(&state, "Warning message while running");
	log_error(&state, "Error message while running");
	state.shutdown = true;
	log_info(&state, 1, "Shutting down state");
	log_warning(&state, "Warning message during shutdown");
	log_error(&state, "Error message during shutdown");
	return 0;
}
