#ifndef CLICOMMANDS_H
#define CLICOMMANDS_H

#include <SimpleCLI.h>
void errorCallback(cmd_error* e);
void cmd_send_request_command(CLIArguments& args);
void cmd_enter_testing_mode(CLIArguments& args);
void cmd_exit_testing_mode(CLIArguments& args);
void cmd_send_test_message(CLIArguments& args);
void cmd_test_sd_write(CLIArguments& args);
void cmd_test_sd_read(CLIArguments& args);

void setupCLICommands();

#endif
