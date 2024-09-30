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
    int sp_denominator = open(serialPortName, O_RDWR | O_NOCTTY);
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
    newtio.c_cc[VMIN] = 5;  // Blocking read until 5 chars received

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

    // Create buffer
    unsigned char buf[BUF_SIZE] = {0};

    // Read establishment message
    int bytes = read(sp_denominator, buf, 5);
    
    // Print out received message in hexadecimal
    printf(":");
    for (int i = 0; i<5; i++) {
        printf("0x%02X ", buf[i]);                               
    }
    printf(":\n");
    
    // Verify message
    if (buf[0] != FLAG) {
        printf("First byte is not flag 0x7E\n");
        return 1;
    }        
    if (buf[1] != SND_SNT) {
        printf("Frame was not sent by Sender\n");
        return 1;
    }
    if (buf[2] != SET) {
        printf("Control wasn't SET\n");
        return 1;
    }
    if (buf[3] != (buf[1] ^ buf[2])) {
        printf("Message was corrupted\n");
        return 1;
    }
    if (buf[4] != FLAG) {
        printf("Last byte is not flag 0x7E\n");
        return 1;
    }
    
    printf("Message was verified and is correct\n");
    
    // Send UA command
    buf[0] = FLAG;
    buf[1] = RCV_ANS;
    buf[2] = UA;
    buf[3] = buf[1] ^ buf[2];
    buf[4] = FLAG;
    
    bytes = write(sp_denominator, buf, 5);
    
    // Wait until all bytes have been written to the serial port
    sleep(1);

    // Restore the old port settings
    if (tcsetattr(sp_denominator, TCSANOW, &oldtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }

    close(sp_denominator);

    return 0;
}
