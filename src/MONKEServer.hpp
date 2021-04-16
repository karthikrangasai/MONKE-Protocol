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
#include "server/Window.hpp"
#include "server/PacketHandler.hpp"

using namespace std;

#define usl uint32_t

class MONKEServer {
   private:
    NetworkInformation senderSock;
    NetworkInformation receiverSock;

    ReceiveWindow window;

    unsigned int MSS;
    unsigned int timeout;
    unsigned int bufferSize;

    bool connected;

    thread receiverPacketHandlerThread;
    vector<Packet *> writeBuffer;

   public:
    MONKEServer(char receiverIP[], int receiverPort = 42069) {
        // this->sock = NetworkInformation(senderIP, senderPort);
        strcpy(this->receiverSock.ip, receiverIP);
        this->receiverSock.port = receiverPort;
        receiverSock.createSocket();

        this->window = ReceiveWindow();
        this->connected = false;

        this->timeout = 3;
        this->bufferSize = sizeof(Packet);
    }

    ~MONKEServer();

    bool bindsocket() {
        // strcpy(this->receiverSock.ip, receiverIP);
        // this->receiverSock.port = receiverPort;
        // receiverSock.createSocket();
        socklen_t sockLen = sizeof(this->receiverSock.servAddr);
        int ret = bind(this->receiverSock.sockFD, (sockaddr *)&(this->receiverSock.servAddr), sockLen);

        return (ret == 0);
        // Perform Handshake
        // sendTo calls etc etc
        // Rewrite RAW RDT Stuff here

        // IF eveything is fine:
        // this->connected = true;
    }

    void listen() {
        while (true) {
            char received_packet[this->bufferSize];
            socklen_t sockLen = sizeof(this->senderSock.servAddr);
            int n = recvfrom(this->receiverSock.sockFD, received_packet, this->bufferSize, 0, (sockaddr *)&(this->senderSock.servAddr), &sockLen);

            if (n < 0) {
                cout << "[ERROR] Handshake failed" << endl;
                exit(1);
            }

            Packet synPacket = Packet::parsePacket(received_packet);

            if (synPacket.getMessageType() == MESSAGE_TYPES::HELLO) {
                // proceed with HandShake
                Packet synAckPacket(MESSAGE_TYPES::HELLO | MESSAGE_TYPES::ACCEP, (1 << RECEIVE_WINDOW_BITS) - 1, 0, (1 << INDEX_NUMBER_BITS) - 1, (1 << SEQUENCE_NUMBER_BITS) - 1, NULL);
                n = sendto(this->receiverSock.sockFD, &synAckPacket, this->bufferSize, 0, (sockaddr *)&(this->senderSock.servAddr), sockLen);
            } else {
                continue;
            }
        }

        // Upon on succesful connection, create a packetHandler and leave it to it's thread:this->receiverPacketHandlerThread
    }

    size_t recv(char *buffer) {
        if (this->connected) {
            ///////////////////////////////
            // make this a blocking call //
            ///////////////////////////////

            // Create the Packet Handler
            ReceivePacketHandler packetHandler(
                this->senderSock,
                this->receiverSock,
                this->window,
                this->MSS,
                this->timeout,
                this->bufferSize,
                this->writeBuffer);

            // packetHandler.start(data);

            // packetHandler.join();
        } else {
            // Raise error
        }
    }
};