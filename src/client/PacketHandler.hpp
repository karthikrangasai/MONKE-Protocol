#include <iostream>
#include <vector>
#include <map>
#include <thread>
#include <mutex>
#include <time.h>
#include <sys/time.h>
#include <cstring>
#include <algorithm>
#include <sys/select.h>

#include "common/types.hpp"
#include "common/Packet.hpp"
#include "Window.hpp"
using namespace std;

#define usl uint32_t

mutex socketWrite;

class SendPacketHandler {
   private:
    thread t;

   public:
    NetworkInformation senderSock;
    NetworkInformation receiverSock;
    NewSendWindow window;
    unsigned int MSS;
    unsigned int maxPayload;
    unsigned int timeout;
    unsigned int bufferSize;

    SendPacketHandler(
        NetworkInformation senderSock,
        NetworkInformation receiverSock,
        NewSendWindow window,
        unsigned int MSS,
        unsigned int timeout,
        unsigned int bufferSize)
        : window{window} {
        this->senderSock = senderSock;
        this->receiverSock = receiverSock;
        this->MSS = MSS;
        this->maxPayload = MAXIMUM_PAYLOAD;
        this->timeout = timeout;
        this->bufferSize = bufferSize;
    }

    // void run(char data[]) {
    //     // We know that we are sending data and not doing hand-shake
    //     // So this defaults to Message Type = DATA.
    //     vector<Packet> packets = this->generate_packets(data);
    //     cout << "[INFO] Beginning packet transmission..." << endl;
    //     while (!this->window.empty()) {
    //         if (this->window.full()) {
    //             continue;
    //         } else if (!this->window.full() && (this->window.currentFreeWindowSize() >= packets.size())) {
    //             continue;
    //         } else {
    //             Packet packet = packets[this->window.next()];
    //             this->window.consume(make_pair(packet.getIndexNum(), packet.getSequenceNum()));
    //             SendSinglePacketHandler single_packet = SendSinglePacketHandler(this->senderSock, this->receiverSock, this->window, packet, this->timeout);

    //             thread p(&SendSinglePacketHandler::run, single_packet);
    //         }
    //     }
    //     cout << "[INFO] Packet transmission will now be stopped" << endl;
    //     this->window.stop_transmission();
    // }

    vector<Packet> generate_packets(char data[]) {
        int index = 0;
        // int count = 0;
        vector<Packet> packet_list;
        while (index < sizeof(data)) {
            // Chunk Data Accordingly
            char dataChunk[this->maxPayload];
            if (index + this->maxPayload < sizeof(data)) {
                memcpy(dataChunk, &data[index], this->maxPayload);
                index += this->maxPayload;
            } else {
                memcpy(dataChunk, &data[index], sizeof(data) - index);
                index += this->maxPayload;
            }
            // Get IndexNum and SeqNum pair
            pair<uint32_t, uint32_t> packetID = this->window.getCurrentIndexAndSeqNum();
            // Create Packet
            Packet packet = Packet(MESSAGE_TYPES::DATA, this->window.currentFreeWindowSize(), 0, packetID.first, packetID.second, dataChunk);

            // usl sequenceNum = count % this->window.maxSequenceNumber();
            // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            // URGENT: How to decide Acknowledgement Numbers.
            // TODO: Get the Receive Window for the sender.
            // int receiveWindow = 0;
            // int acknowledgementNum = 0;  // ACK = latest_SEQ + 1
            // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            // Packet p = Packet(MESSAGE_TYPES::DATA, receiveWindow, acknowledgementNum, this->indexNumber, sequenceNum, dataChunk);
            packet_list.push_back(packet);
            // count += 1;
        }
        return packet_list;
    }

    void run(char data[]) {
        // We know that we are sending data and not doing hand-shake
        // So this defaults to Message Type = DATA.
        // vector<Packet> packets = this->generate_packets(data);
        vector<thread*> packetHandlerThreads;
        int index = 0;
        while (index < sizeof(data)) {
            // Chunk Data Accordingly
            char dataChunk[this->maxPayload];
            if (index + this->maxPayload < sizeof(data)) {
                memcpy(dataChunk, &data[index], this->maxPayload);
                index += this->maxPayload;
            } else {
                memcpy(dataChunk, &data[index], sizeof(data) - index);
                index += this->maxPayload;
            }

            // Get IndexNum and SeqNum pair
            pair<uint32_t, uint32_t> packetID = this->window.getCurrentIndexAndSeqNum();
            // Create Packet
            Packet packet = Packet(MESSAGE_TYPES::DATA, this->window.currentFreeWindowSize(), 0, packetID.first, packetID.second, dataChunk);
            // while(this->window.empty());

            // Empty -> Add packet
            // Not empty
            // 		if Full -> Block
            // 		else -> Add packet
            while (this->window.full())  // Blocking call - Send Window (Client)
                ;
            this->window.addTo();
            SendSinglePacketHandler single_packet = SendSinglePacketHandler(this->senderSock, this->receiverSock, this->window, packet, this->timeout);
            thread p(&SendSinglePacketHandler::run, single_packet);
            packetHandlerThreads.push_back(&p);
            // p.join();  // Sequentially send chunks
        }

        // Can call join here.
        // for each singleHandler join()
        // sender -> datahandler -> 16 * packethandlers
        // (n-16) done and 16 remaining. after spawning nth thread, can `run` die
        for (thread* t : packetHandlerThreads) {
            if (t->joinable()) {
                t->join();
            }
        }
    }

    void start(char data[]) {
        this->t = thread(&SendPacketHandler::run, this, data);
    }

    void join() {
        t.join();
    }
};

class SendSinglePacketHandler {
   public:
    NetworkInformation senderSock;
    NetworkInformation receiverSock;
    NewSendWindow window;
    Packet packet;
    int timeout;

    SendSinglePacketHandler(
        NetworkInformation senderSock,
        NetworkInformation receiverSock,
        NewSendWindow window,
        Packet packet,
        int timeout)
        : window{window}, packet{packet} {
        this->senderSock = senderSock;
        this->receiverSock = receiverSock;
        this->timeout = timeout;
    }

    void run() {
        cout << "[DEBUG] Transmitting packet :: Sequence number : " << this->packet.getSequenceNum();
        this->udp_send(this->packet);
        this->window.start(make_pair(this->packet.getIndexNum(), this->packet.getSequenceNum()));

        time_t t;
        while (this->window.unacked(make_pair(this->packet.getIndexNum(), this->packet.getSequenceNum()))) {
            t = time(NULL) - this->window.start_time(make_pair(this->packet.getIndexNum(), this->packet.getSequenceNum()));
            if (t > this->timeout) {
                // Retransmit packet
                this->udp_send(this->packet);
                this->window.restart(make_pair(this->packet.getIndexNum(), this->packet.getSequenceNum()));
            }
        }
        this->window.stop(make_pair(this->packet.getIndexNum(), this->packet.getSequenceNum()));
    }

    void udp_send(Packet packet) {
        int x;

        //  Why are we using mutex here ?????????????
        //  That too transmission Lock mutex
        //  If we want one, we have to create a new mutex
        unique_lock<mutex> lock(socketWrite);
        socklen_t sockLen = sizeof(this->receiverSock.servAddr);
        int ret = sendto(this->senderSock.sockFD, &packet, sizeof(Packet), 0, (sockaddr*)&(this->receiverSock.servAddr), sockLen);
        if (ret < 0) {
            cout << "[ERROR] Could not send a data packet." << endl;
        }
        socketWrite.unlock();
    }
};

class SendACKHandler {
   private:
    thread t;

   public:
    NetworkInformation senderSock;
    NetworkInformation receiverSock;

    NewSendWindow window;

    unsigned int timeout;
    unsigned int bufferSize;
    // uint32_t indexNumber;  // For Packet Management

    SendACKHandler(
        NetworkInformation senderSock,
        NetworkInformation receiverSock,
        NewSendWindow window,
        unsigned int timeout,
        unsigned int bufferSize)
        : window{window} {
        this->senderSock = senderSock;
        this->receiverSock = receiverSock;
        this->timeout = timeout;
        this->bufferSize = bufferSize;
    }

    void run() {
        while (this->window.transmitting()) {
            if (this->window.empty()) {
                continue;
            }

            // select
            fd_set readFileSet;
            FD_ZERO(&readFileSet);
            FD_SET(this->senderSock.sockFD, &readFileSet);

            timeval timeout;
            timeout.tv_sec = this->timeout;
            timeout.tv_usec = 0;

            int ready = select(this->senderSock.sockFD + 1, &readFileSet, NULL, NULL, &timeout);

            if (ready <= 0) {
                // Error
                continue;
            }

            // Receive Data
            char received_packet[this->bufferSize];
            socklen_t sockLen = sizeof(this->receiverSock.servAddr);
            if (recvfrom(this->senderSock.sockFD, received_packet, this->bufferSize, 0, (sockaddr*)&(this->receiverSock.servAddr), (socklen_t*)&sockLen) < 0) {
                //Failed to Recieve Data
                break;
            } else {
                //Recieved Data!!
                Packet packet = Packet::parsePacket(received_packet);
                pair<uint32_t, uint32_t> key = make_pair(packet.getIndexNum(), packet.getSequenceNum());
                if (!(this->window.exists(key))) {
                    // Drop packet or throw error
                }
                this->window.mark_acked(key);
            }
        }
    }

    void start() {
        this->t = thread(&SendACKHandler::run, this);
    }

    void join() {
        t.join();
    }
};