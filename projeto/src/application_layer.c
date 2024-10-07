// Application layer protocol implementation

#include "application_layer.h"
#include "link_layer.h"
#include "string.h"

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{
    // Initializing LinkLayer struct
    LinkLayer layerInformation;
    strcpy(layerInformation.serialPort, serialPort);

    if (strcmp(role, "tx") == 0) {
        layerInformation.role = LlTx;
    }
    else {
        layerInformation.role = LlRx;
    }
    layerInformation.baudRate = baudRate;
    layerInformation.nRetransmissions = nTries;
    layerInformation.timeout = timeout;
    
    // Open link layer
    llopen(layerInformation);
}
