#include <iostream>
#include <vector>
#include <map>
#include <thread>
#include <mutex>
#include <time.h>
#include <cstring>
#include <pthread>
#include <algorithm>

#include "common/Packet.hpp"
#include "client/Window.hpp"
using namespace std;

#define usl uint32_t


class SendPacketHandler {
    
    char senderIP[16];
    int senderPort;
    int senderSocket;
    char receiverIP[16];
    int receiverPort;
    SendWindow window;
    int MSS;
    int maxPayload;
    int timeout;
    int bufferSize;

    SendPacketHandler(char sIP[], int sp, int ss, char rIP[], int rp, SendWindow w, int MSS, int t, int bs)
    {
        strcpy(this->senderIP, sIP);
        strcpy(this->receiverIP, rIP);
        this->senderPort = sp;
        this->receiverPort = rp;
        this->senderSocket = ss;
        this->window = w;
        this->MSS = MSS;
        this->maxPayload = 520;
        this->timeout = t;
        this->bufferSize = bs;
    }

    void run(char data[]) {
        vector<Packet> packets = this->generate_packets(data);
        cout<<"[INFO] Beginning packet transmission..."<<endl;
        while(!this->window.empty())
        {
            if(this->window.full())
                continue;
            else if(!this->window.full() && this->window.next() >= packets.size)
                continue;
            else {
                packet = packets[this->window.next()];
                this->window.consume(packet.getSequenceNum());
                single_packet = SendSinglePacketHandler(this->senderSocket,
                                                        this->receiverIP,
                                                        this->receiverPort,
                                                        this->window,
                                                        packet,
                                                        this->timeout)
                pthread_t p;
                int rc = pthread_create(&p, NULL, single_packet.run, NULL);
                if(rc)
                {
                    cout<<"[ERROR] Could not create thread"<<endl;
                    exit(1);
                }
            }
        }
        pthread_exit(NULL);
        cout<<"[INFO] Packet transmission will now be stopped"<<endl;
        this->window.stop_transmission();
    }

    vector<Packet> generate_packets(char data[]) {
        int index = 0;
        int count = 0;
        vector<Packet> packet_list;
        while(index < sizeof(data)) {
            char data_piece[this->maxPayload];
            if(index + this->maxPayload < sizeof(data)) {
                memcpy(data_piece, &data[index], this->maxPayload);
                index += this->maxPayload;
            }
            else {
                memcpy(data_piece, &data[index], sizeof(data) - index);
                index += this->maxPayload;
            }
            usl seqNum = count % this->window.maxSequenceNumber();
            // TODO: URGENT: Add message types support!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            Packet p = Packet(types.DATA,); //....add args here)
            packet_list.push_back(p);
            count += 1;
        }
        return packet_list;
    }
}


class SendSinglePacketHandler {

    int senderSocket;
    char receiverIP[16];
    int receiverPort;
    SendWindow window;
    Packet packet;
    int timeout;

    SendSinglePacketHandler(int ss, char rIP[], int rp, SendWindow w, Packet p, int t)
    {
        this->senderSocket = ss;
        strcpy(this->receiverIP, rIP);
        this->receiverPort = rp;
        this->window = w;
        this->packet = packet;
        this->timeout = t;
    }

    void *run()
    {
        cout<<"[DEBUG] Transmitting packet :: Sequence number : "<<this->packet.getSequenceNum();
        this->udp_send(this->packet);
        this->window.start(this->packet.getSequenceNum());

        time_t t;
        while(this->window.unacked(this->packet.getSequenceNum())) {
            t = time(NULL) - this->window.start_time(this->packet.getSequenceNum());
            if(t > this->timeout) {
                // Retransmit packet
                this->udp_send(this->packet);
                this->window.restart(this->packet.getSequenceNum());
            }
        }
        unique_lock<mutex> lock(transmissionLock);
        this->window.stop(this->packet.getSequenceNum());
        transmissionLock.unlock();
    }

    void udp_send(Packet p) {
        int x;
        unique_lock<mutex> lock(transmissionLock);
        x = this->senderSocket.send(p);
        if(x < 0)
        {
            cout<<"[ERROR] Could not send data"<<endl;
        }
        transmissionLock.unlock();
    }
}


class SendACKHandler {

}