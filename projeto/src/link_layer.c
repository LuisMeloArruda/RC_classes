// Link layer protocol implementation

#include "link_layer.h"
#include "serial_port.h"

// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source

// Connection establishment macros
#define FLAG 0x7E
#define SND_SNT 0x03 // Frames sent by sender
#define RCV_ANS 0x03 // Answers from the receiver 
#define SET 0x03
#define UA 0x07

////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////

static void send_set_cmd(int fd)
{
    // Create buffer
    unsigned char buf[15] = {0};
    
    // Send SET command
    buf[0] = FLAG;
    buf[1] = SND_SNT;
    buf[2] = SET;
    buf[3] = SND_SNT ^ SET;
    buf[4] = FLAG;

    int bytes = write(fd, buf, 15);

    // Wait until all bytes have been written to the serial port
    sleep(1);

    printf("Sent SET command\n");
} 

int llopen(LinkLayer connectionParameters)
{
    // Open Serial Port
    int fd = openSerialPort(connectionParameters.serialPort, connectionParameters.baudRate);

    if (connectionParameters.role == LlTx) {
        printf("ola");
        send_set_cmd(fd);
    }
    else {
        printf("adeus");
    }

    return fd;
}

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(const unsigned char *buf, int bufSize)
{
    // TODO

    return 0;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet)
{
    // TODO

    return 0;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(int showStatistics)
{
    // TODO

    int clstat = closeSerialPort();
    return clstat;
}