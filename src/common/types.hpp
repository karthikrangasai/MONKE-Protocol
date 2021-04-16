#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

#define usl uint32_t
// Note: The above was originally meant to define unsigned long, however
//       we want strictly 32 bits to be allocated.
/* 
	"Then shalt thou count to 32, no more, no less. 32 shall be the
	number thou shalt count, and the number of the counting shall be 32.
	33 shalt thou not count, neither count thou 31, excepting that thou 
	then proceed to 32. 34 is right out. Once the number 32, being the 
	32nd number, be reached, then loadest thou thy values, which being 
	32 bits, shall serve you well "
*/

enum MESSAGE_TYPES {
    HELLO = 1,   // SYN
    DATA = 2,    //
    ACCEP = 4,   //ACK
    REJECC = 8,  //FIN
    PING = 16,   //
};

const unsigned int SEQUENCE_NUMBER_BITS = 32;

const unsigned int INDEX_NUMBER_BITS = 32;

const unsigned int RECEIVE_WINDOW_BITS = 27;

const unsigned int COOL_DIRMA = 4;

const unsigned int MAXIMUM_PAYLOAD = 512;

typedef struct NetworkInformation {
    int sockFD;
    struct sockaddr_in servAddr;
    char ip[16];
    int port;

    // NetworkInformation(char receiverIP[], int receiverPort) {
    //     strcpy(this->receiverIP, receiverIP);
    //     this->receiverPort = receiverPort;
    // }

    bool createSocket() {
        this->sockFD = socket(AF_INET, SOCK_DGRAM, 0);
        if (this->sockFD < 0) {
            return false;
        }

        memset(&servAddr, 0, sizeof(servAddr));
        // Filling server information
        servAddr.sin_family = AF_INET;
        servAddr.sin_port = htons(this->port);
        inet_aton(this->ip, &(this->servAddr.sin_addr));
    }

} NetworkInformation;