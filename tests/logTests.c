
#include "SELKIELoggerBase.h"

int main(int argc, char *argv[]) {
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
