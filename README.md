
# UCONN-eCTF
Code and Tools used for the 2017 MITRE eCTF Competition

### ATMega1284_Boot
Contains MITRE's example bootloader code with minor edits to function in Atmel Studio. Will eventually contain full encrypted bootloader.

### ATMega1284_Enc_noSCP
Performs encryption and decryption in CFB mode. Also computes hashes using CBC_MAC. Does not have any side channel resistance.

### ATMega1284_Firm_Ex
Contains the simplest possible test code I could think of. It's just a 1Hz blink sketch. Used to test bootloader functionality.

### ATMega1284_TRNG
Contains test code for a True Random Number Generator (TRNG) based on watchdog timer jitter. The code has been tested. The randomness has not.

### ATMega1284_VarClk
Contains test code for a randomly varying clock for the ATMega. Switches back and forth between 1MHz and 8MHz successfully. Has functions for reading and writing from/to UART0 & UART1. Only the writing functions (and clock switching) have been tested.
NOTE: UART will not work in anything but 8MHz mode. Prepare accordingly.

### host_tools
Contains host tools being worked on by Luke, James, and Cameron.


### Other

##### Blink_FW0.json
Example blink firmware for use by fw_update

