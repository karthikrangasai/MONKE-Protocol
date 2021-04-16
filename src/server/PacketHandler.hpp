#include <iostream>
#include <vector>
#include <map>
#include <thread>
#include <mutex>
#include <time.h>
#include <cstring>
#include <pthread.h>
#include <algorithm>
#include <sys/select.h>

#include "common/Packet.hpp"
#include "Window.hpp"
using namespace std;

#define usl uint32_t

mutex writeBufferLock;

class ReceivePacketHandler {
   private:
    thread t;
    NetworkInformation senderSock;
    NetworkInformation receiverSock;
    int fileHandle;  // Basically the socket to read from // file to write the buffer to
    ReceiveWindow window;
    int timeout;
    int bufferSize;

    vector<Packet*> recvBuffer;

   public:
    ReceivePacketHandler(
        NetworkInformation senderSock,
        NetworkInformation receiverSock,
        ReceiveWindow window,
        int MSS,
        int timeout,
        int bufferSize,
        vector<Packet*>& recvBuffer)
        : window{window} {
        this->senderSock = senderSock;
        this->receiverSock = receiverSock;
        this->timeout = timeout;
        this->bufferSize = bufferSize;
        this->recvBuffer = recvBuffer;
    }

    vector<Packet*> getWriteBuffer() {}

    void run() {
        cout << "[INFO] Packet receipt monitoring has begun" << endl;
        int chance = 0;
        int ready;
        struct timeval tv;
        fd_set rfds;

        while (true) {
            tv.tv_sec = this->timeout;
            tv.tv_usec = 0;
            FD_SET(this->senderSock.sockFD, &rfds);
            ready = select(this->senderSock.sockFD + 1, &rfds, NULL, NULL, &tv);
            if (ready == -1) {
                cout << "[ERROR] There was an error" << endl;
                exit(0);
            }
            if (ready == 0) {
                if (!this->window.receipt()) {
                    continue;
                } else {
                    if (chance == 5) {
                        cout << "[WARNING] There have been 5 consecutive timeouts! Exiting...." << endl;
                        break;
                    } else {
                        chance += 1;
                        continue;
                    }
                }
                // Notify Sender overlord that you cut.
            } else {
                chance = 0;
                if (!this->window.receipt()) {
                    this->window.start_receipt();
                }
            }

            char received_packet[this->bufferSize];
            int ret = recv(this->senderSock.sockFD, received_packet, this->bufferSize, 0);
            if (ret < 0) {
                cout << "[ERROR] Could not receive any data!" << endl;
                exit(1);
                // Cut the sender and say OS is dead.
            }
            Packet packet = Packet::parsePacket(received_packet);

            // Send ACK
            Packet ackPacket = Packet(MESSAGE_TYPES::ACCEP, this->window.currentFreeWindowSize(), 0, packet.getIndexNum(), packet.getSequenceNum(), nullptr);
            socklen_t sockLen = sizeof(this->senderSock.servAddr);
            int ret = sendto(this->receiverSock.sockFD, &ackPacket, sizeof(Packet), 0, (sockaddr*)&(this->senderSock.servAddr), sockLen);
            if (ret < 0) {
                cout << "[ERROR] Could not send a data packet." << endl;
                exit(1);
            }

            pair<uint32_t, uint32_t> key(packet.getIndexNum(), packet.getSequenceNum());
            if (!(this->window.exists(key))) {
                this->window.store(packet);
            }

            Packet* nextPacket = this->window.next();
            if (nextPacket != nullptr) {
                // write to buffer
                // this->writeBuffer.
            }
        }
    }
};