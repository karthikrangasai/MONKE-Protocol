#include <iostream>
#include <vector>
#include <map>
#include <thread>
#include <mutex>
#include <time.h>
#include <cstring>
#include <pthread>
#include <algorithm>
#include <sys/select.h>

#include "common/Packet.hpp"
#include "client/Window.hpp"
using namespace std;

#define usl uint32_t


class ReceivePacketHandler {

    int fileHandle; // Basically the socket to read from
    int receiverSocket;
    char senderIP[16];
    int senderPort;
    char receiverIP[16];
    int receiverPort;
    ReceiveWindow window;
    int timeout;
    int bufferSize;

    ReceivePacketHandler(int ss, int rsock, char sIP[], int sp, char rIP[], int rp, SendWindow w, int MSS, int t, int bs)
    {
        strcpy(this->senderIP, sIP);
        strcpy(this->receiverIP, rIP);
        this->senderPort = sp;
        this->receiverPort = rp;
        this->fileHandle = ss;
        this->receiverSocket = rsock;
        this->window = w;
        this->timeout = t;
        this->bufferSize = bs;
    }

    void run() {
        cout<<"[INFO] Packet receipt monitoring has begun"<<endl;
        int chance = 0;
        int ready;
        struct timeval tv;
        fd_set rfds;

        while(1) {
            tv.tv_sec = this->timeout;
            tv.tv_usec = 0;
            FD_SET(this->receiverSocket, &rfds);
            ready = select(this->receiverSocket+1, &rfds, NULL, NULL, &tv);
            if(ready == -1) {
                cout<<"[ERROR] There was an error"<<endl;
                exit(0);
            }
            if(!ready) {
                if(!this->window.receipt())
                    continue;
                else {
                    if(chance == 5) {
                        cout<<"[WARNING] There have been 5 consecutive timeouts! Exiting...."<<endl;
                        break;
                    }
                    else {
                        chance += 1;
                        continue;
                    }
                }
            }
            else {
                chance = 0
                if(!this->window.receipt())
                    this->window.start_receipt();
            }
            char received_packet[this->bufferSize];
            int ret = recv(this->receiverSocket, received_packet, this->bufferSize, 0);
            if(ret < 0) {
                cout<<"[ERROR] Could not receive any data!"<<endl;
                exit(1);
            }
            if(this->window.out_of_order)
        }
    }
}