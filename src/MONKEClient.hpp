/**
 * @file Window.hpp
 * @brief 
 */
#include <iostream>
#include <vector>
#include <map>
#include <string.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

#include "common/types.hpp"
#include "client/Window.hpp"
#include "client/PacketHandler.hpp"

using namespace std;

#define usl uint32_t

class MONKEClient {
   private:
    NetworkInformation senderSock;
    NetworkInformation receiverSock;

    // int senderSockFD;
    // sockaddr_in senderServAddr;
    // char senderIP[16];
    // uint16_t senderPort;

    uint32_t indexNumber;
    NewSendWindow window;

    unsigned int MSS;
    unsigned int timeout;
    unsigned int bufferSize;

    bool connected;

   public:
    MONKEClient(char senderIP[], int senderPort = 42069) {
        // this->sock = NetworkInformation(senderIP, senderPort);
        strcpy(this->senderSock.ip, senderIP);
        this->senderSock.port = senderPort;
        senderSock.createSocket();

        this->indexNumber = 0;
        this->window = NewSendWindow();
        this->connected = false;

        this->timeout = 3;
        this->bufferSize = sizeof(Packet);
    }

    ~MONKEClient();

    bool connect(char receiverIP[], int receiverPort = 42069) {
        strcpy(this->receiverSock.ip, receiverIP);
        this->receiverSock.port = receiverPort;
        receiverSock.createSocket();

        // Perform Handshake
        // sendTo calls etc etc
        // Rewrite RAW RDT Stuff here
        Packet synPacket(MESSAGE_TYPES::HELLO, (1 << RECEIVE_WINDOW_BITS) - 1, 0, (1 << INDEX_NUMBER_BITS) - 1, (1 << SEQUENCE_NUMBER_BITS) - 1, NULL);
        socklen_t sockLen = sizeof(this->receiverSock.servAddr);
        int n = sendto(this->senderSock.sockFD, &synPacket, this->bufferSize, 0, (sockaddr *)&(this->receiverSock.servAddr), sockLen);

        if (n < 0) {
            cout << "[ERROR] Handshake failed" << endl;
            exit(1);
        }

        char received_packet[this->bufferSize];
        n = recvfrom(this->senderSock.sockFD, received_packet, this->bufferSize, 0, (sockaddr *)&(this->receiverSock.servAddr), &sockLen);

        Packet synAckPacket = Packet::parsePacket(received_packet);

        if (synAckPacket.getMessageType() == (MESSAGE_TYPES::HELLO | MESSAGE_TYPES::ACCEP)) {
            // proceed with
        }

        // IF eveything is fine:
        this->connected = true;
    }

    size_t send(char data[]) {
        if (this->connected) {
            // Create the Packet Handler
            SendPacketHandler packetHandler(
                this->senderSock,
                this->receiverSock,
                this->window,
                this->MSS,
                this->timeout,
                this->bufferSize);
            // Create the ACK Handler

            SendACKHandler ackHandler(
                this->senderSock,
                this->receiverSock,
                this->window,
                this->timeout,
                this->bufferSize);

            packetHandler.start(data);
            ackHandler.start();

            packetHandler.join();
            ackHandler.join();
        } else {
            // Raise error
        }
    }
};