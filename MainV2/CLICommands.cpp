#include "CLICommands.h"
#include "CANCommands.h"
#include "SDHelper.h" // Include the SDHelper header

extern SimpleCLI cli;
extern Command sendRequestCommand;

void errorCallback(cmd_error* e) {
    CommandError cmdError(e);

    Serial.print("ERROR: ");
    Serial.println(cmdError.toString());

    if (cmdError.hasCommand()) {
        Serial.print("Did you mean \"");
        Serial.print(cmdError.getCommand().toString());
        Serial.println("\"?");
    }
}
void cmd_send_request_command(CLIArguments& args) {
    if (args.count() > 0) {
        int wheel = args.getInt(0); // The first argument specifies the wheel (0 to 3)
        // Convert the wheel number to the appropriate Request Command ID
        int requestCommandID = 0x08F89540 | (wheel << 4); // Assuming the IDs follow a pattern
        
        // Create and send the Request Command message
        uint8_t requestData[] = { 0x05 }; // Data to send
        CAN_SendMessage(requestCommandID, sizeof(requestData), requestData);
    } else {
        Serial.println("Missing argument. Usage: send-request <wheel_number>");
    }
}

void cmd_enter_testing_mode(CLIArguments& args) {
    // Implement logic to enter testing mode
    Serial.println("Entered testing mode. Use 'exit-testing' to exit.");
    // You can add additional logic here to signal that the system is in testing mode
}

void cmd_exit_testing_mode(CLIArguments& args) {
    // Implement logic to exit testing mode
    Serial.println("Exited testing mode.");
    // You can add additional logic here to signal that the system has exited testing mode
}

void cmd_send_test_message(CLIArguments& args) {
    if (args.count() > 0) {
        // Parse and send test message using CAN_SendMessage function
        // Example: cmd_send_test_message 0x123 1 2 3
        uint32_t id = args.getHex(0);
        uint8_t length = args.getInt(1);
        uint8_t data[8];
        for (int i = 0; i < length; i++) {
            data[i] = args.getInt(i + 2);
        }
        CAN_SendMessage(id, length, data);
    } else {
        Serial.println("Usage: send-test-message <CAN_ID> <DLC> <data...>");
    }
}

void cmd_test_sd_write(CLIArguments& args) {
    writeFile(SD, "/test.txt", "Testing SD write functionality.\n");
    Serial.println("Test message written to SD card.");
}

void cmd_test_sd_read(CLIArguments& args) {
    Serial.println("Reading content from test.txt on SD card:");
    readFile(SD, "/test.txt");
}

void setupCLICommands() {
    sendRequestCommand = cli.addCmd("send-request", cmd_send_request_command);
    cli.addCmd("enter-testing", cmd_enter_testing_mode);
    cli.addCmd("exit-testing", cmd_exit_testing_mode);
    cli.addCmd("send-test-message", cmd_send_test_message);
    cli.addCmd("test-sd-write", cmd_test_sd_write); // Add the new SD test commands
    cli.addCmd("test-sd-read", cmd_test_sd_read);
}
