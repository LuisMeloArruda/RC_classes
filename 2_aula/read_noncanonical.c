// Read from serial port in non-canonical mode
//
// Modified by: Eduardo Nuno Almeida [enalmeida@fe.up.pt]

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>

// Baudrate settings are defined in <asm/termbits.h>, which is
// included by <termios.h>
#define BAUDRATE B38400
#define _POSIX_SOURCE 1 // POSIX compliant source

// Auxiliary macros
#define FALSE 0
#define TRUE 1
#define BUF_SIZE 256

// Connection establishment macros
#define FLAG 0x7E
#define SND_SNT 0x03 // Frames sent by sender
#define RCV_ANS 0x03 // Answers from the receiver 
#define SET 0x03
#define UA 0x07

// Serial port variables
int sp_denominator;

// State machine variables
enum state_machine{
    START,
    FLAG_RCV,
    A_RCV,
    C_RCV,
    BCC_OK,
    STOP
};
enum state_machine state = START;


void send_ua_cmd()
{   
    // Create buffer
    unsigned char buf[5] = {0};

    // Send UA command
    buf[0] = FLAG;
    buf[1] = RCV_ANS;
    buf[2] = UA;
    buf[3] = RCV_ANS ^ UA;
    buf[4] = FLAG;
    
    int bytes = write(sp_denominator, buf, 5);

    // Wait until all bytes have been written to the serial port
    sleep(1);

    printf("Sent UA command\n");
}


int main(int argc, char *argv[])
{
    // Program usage: Uses either COM1 or COM2
    const char *serialPortName = argv[1];

    if (argc < 2)
    {
        printf("Incorrect program usage\n"
               "Usage: %s <SerialPort>\n"
               "Example: %s /dev/ttyS1\n",
               argv[0],
               argv[0]);
        exit(1);
    }

    // Open serial port device for reading and writing and not as controlling tty
    // because we don't want to get killed if linenoise sends CTRL-C.
    sp_denominator = open(serialPortName, O_RDWR | O_NOCTTY);
    if (sp_denominator < 0)
    {
        perror(serialPortName);
        exit(-1);
    }

    struct termios oldtio;
    struct termios newtio;

    // Save current port settings
    if (tcgetattr(sp_denominator, &oldtio) == -1)
    {
        perror("tcgetattr");
        exit(-1);
    }

    // Clear struct for new port settings
    memset(&newtio, 0, sizeof(newtio));

    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    // Set input mode (non-canonical, no echo,...)
    newtio.c_lflag = 0;
    newtio.c_cc[VTIME] = 0; // Inter-character timer unused
    newtio.c_cc[VMIN] = 0;  // Blocking read until 5 chars received

    // VTIME e VMIN should be changed in order to protect with a
    // timeout the reception of the following character(s)

    // Now clean the line and activate the settings for the port
    // tcflush() discards data written to the object referred to
    // by sp_denominator but not transmitted, or data received but not read,
    // depending on the value of queue_selector:
    //   TCIFLUSH - flushes data received but not read.
    tcflush(sp_denominator, TCIOFLUSH);

    // Set new port settings
    if (tcsetattr(sp_denominator, TCSANOW, &newtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }

    printf("New termios structure set\n");

    while (state != STOP) {
        // Create buffer
        unsigned char buf[1] = {0};

        // Read byte
        int bytes = read(sp_denominator, buf, 1);
        if (bytes == 0) continue;
        unsigned char read_byte = buf[0];

        printf("Read byte: 0x%02X \n", read_byte);

        // State machine logic
        switch (state)
        {
            case START:
                if (read_byte == FLAG) state = FLAG_RCV;
                break;
            case FLAG_RCV:
                if (read_byte == SND_SNT) state = A_RCV;
                else if (read_byte != FLAG) state = START;
                break;
            case A_RCV:
                if (read_byte == SET) state = C_RCV;
                else if (read_byte == FLAG) state = FLAG_RCV;
                else state = START;
                break;
            case C_RCV:
                if (read_byte == (SND_SNT ^ SET)) state = BCC_OK;
                else if (read_byte == FLAG) state = FLAG_RCV;
                else state = START;
                break;
            case BCC_OK:
                if (read_byte == FLAG) state = STOP;
                else state = START;
        }
    }

    printf("Message received and verified\n");

    // Send UA command
    send_ua_cmd();

    // Restore the old port settings
    if (tcsetattr(sp_denominator, TCSANOW, &oldtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }

    close(sp_denominator);

    return 0;
}
