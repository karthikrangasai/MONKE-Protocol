#include <iostream>
#include <map>
#include <cassert>
#include <cstring>
#include <vector>
#include <tuple>
#include <ctime>
#include <stdlib.h>
#include <deque>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

#include <time.h>
#include <sys/time.h>
#include <cstring>
#include <algorithm>
#include <sys/select.h>

using namespace std;
/*
	Please move on with life.
	Yours Lovingly,
	Life
*/

enum MESSAGE_TYPES {
    HELLO = (uint8_t)1,   // SYN
    DATA = (uint8_t)2,    //
    ACCEP = (uint8_t)4,   //ACK
    REJECC = (uint8_t)8,  //FIN
    FIN = (uint8_t)16,    //
};

const unsigned int SEQUENCE_NUMBER_BITS = 32;
const unsigned int INDEX_NUMBER_BITS = 32;
const unsigned int RECEIVE_WINDOW_BITS = 16;
const unsigned int COOL_DIRMA = 4;
const unsigned int WINDOW_SIZE_BITS = 3;
const unsigned int window_size = 8;
const unsigned int SOCKET_READ_TIMEOUT_SEC = 10;
const unsigned int MAXIMUM_PAYLOAD = 256;

typedef struct NetworkInformation {
    int sockFD;
    struct sockaddr_in servAddr;
    char* ip;
    int port;

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

class Packet {
   private:
    uint8_t messageType;
    uint8_t applnLayerDataSize;
    uint16_t receiveWindow;
    uint32_t sequenceNum;
    uint32_t acknowledgementNum;
    char* applnLayerData;

   public:
    Packet(uint8_t messageType, uint8_t applnLayerDataSize, uint16_t receiveWindow, uint32_t sequenceNum, uint32_t acknowledgementNum, char* applnLayerData) {
        if (messageType != MESSAGE_TYPES::DATA) {
            this->messageType = messageType;
            this->applnLayerDataSize = applnLayerDataSize;
            this->receiveWindow = receiveWindow;
            this->sequenceNum = sequenceNum;
            this->acknowledgementNum = acknowledgementNum;
            this->applnLayerData = nullptr;
        } else {
            assert(applnLayerDataSize <= MAXIMUM_PAYLOAD);
            this->messageType = messageType;
            this->applnLayerDataSize = applnLayerDataSize;
            this->receiveWindow = receiveWindow;
            this->sequenceNum = sequenceNum;
            this->acknowledgementNum = acknowledgementNum;
            this->applnLayerData = (char*)malloc(applnLayerDataSize);
            memset(this->applnLayerData, 0, applnLayerDataSize);
            if (applnLayerData != nullptr) {
                memcpy(this->applnLayerData, applnLayerData, applnLayerDataSize);
            }
        }
    }

    ~Packet() {}

    uint32_t getSequenceNum() {
        return this->sequenceNum;
    }

    uint32_t getAcknowledgementNum() {
        return this->acknowledgementNum;
    }

    MESSAGE_TYPES getMessageType() {
        return (MESSAGE_TYPES)this->messageType;
    }

    uint8_t getApplnLaterDataSize() {
        return this->applnLayerDataSize;
    }

    uint16_t getReceiveWindow() {
        return this->receiveWindow;
    }

    char* getApplnLaterDataPtr() {
        return this->applnLayerData;
    }

    size_t sizeofPakcet() {
        size_t headerSize = sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint16_t) + sizeof(uint32_t) + sizeof(uint32_t) + (sizeof(char) * this->applnLayerDataSize);
        return headerSize;
    }

    static size_t sizeofPakcetHeaders() {
        size_t headerSize = sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint16_t) + sizeof(uint32_t) + sizeof(uint32_t);
        return headerSize;
    }

    static size_t maxPacketSize() {
        return sizeofPakcetHeaders() + MAXIMUM_PAYLOAD;
    }

    static Packet parsePacket(char* buffer, bool fullPacket) {
        char* bufferPointer = buffer;

        uint8_t messageType = (uint8_t)(*bufferPointer);
        ++bufferPointer;

        uint8_t applnLayerDataSize = (uint8_t)(*bufferPointer);
        ++bufferPointer;

        uint16_t receiveWindow = (uint16_t)(*bufferPointer << 8);
        ++bufferPointer;
        receiveWindow = receiveWindow | *bufferPointer;
        ++bufferPointer;

        uint32_t sequenceNum = (uint32_t)(*bufferPointer << 24);
        ++bufferPointer;
        sequenceNum = sequenceNum | (*bufferPointer << 16);
        ++bufferPointer;
        sequenceNum = sequenceNum | (*bufferPointer << 8);
        ++bufferPointer;
        sequenceNum = sequenceNum | *bufferPointer;
        ++bufferPointer;

        uint32_t acknowledgementNum = (uint32_t)(*bufferPointer << 24);
        ++bufferPointer;
        acknowledgementNum = acknowledgementNum | (*bufferPointer << 16);
        ++bufferPointer;
        acknowledgementNum = acknowledgementNum | (*bufferPointer << 8);
        ++bufferPointer;
        acknowledgementNum = acknowledgementNum | *bufferPointer;
        ++bufferPointer;

        char* applnLayerData = nullptr;
        if (applnLayerDataSize > 0) {
            applnLayerData = (char*)malloc(sizeof(char) * applnLayerDataSize);
            memset(applnLayerData, 0, sizeof(char) * applnLayerDataSize);
            memcpy(applnLayerData, bufferPointer, sizeof(char) * applnLayerDataSize);
            return Packet(messageType, applnLayerDataSize, receiveWindow, sequenceNum, acknowledgementNum, applnLayerData);
        }

        // cout << "messageType: " << messageType << endl;
        // cout << "applnLayerDataSize: " << applnLayerDataSize << endl;
        // cout << "receiveWindow: " << receiveWindow << endl;
        // cout << "sequenceNum: " << sequenceNum << endl;
        // cout << "acknowledgementNum: " << acknowledgementNum << endl;

        return Packet(messageType, applnLayerDataSize, receiveWindow, sequenceNum, acknowledgementNum, applnLayerData);
    }

    char* serializePacket() {
        size_t packetLength = (this->sizeofPakcet() + 1) * sizeof(char);
        char* buffer = (char*)malloc(packetLength);
        memset(buffer, '\0', packetLength);

        char* bufferPtrCopy = buffer;
        *bufferPtrCopy = (uint8_t)this->messageType;
        ++bufferPtrCopy;
        *bufferPtrCopy = (uint8_t)this->applnLayerDataSize;
        ++bufferPtrCopy;

        *bufferPtrCopy = (this->receiveWindow >> 8) & 0xFF;
        ++bufferPtrCopy;
        *bufferPtrCopy = this->receiveWindow & 0xFF;
        ++bufferPtrCopy;

        *bufferPtrCopy = (this->sequenceNum >> 24) & 0xFF;
        ++bufferPtrCopy;
        *bufferPtrCopy = (this->sequenceNum >> 16) & 0xFF;
        ++bufferPtrCopy;
        *bufferPtrCopy = (this->sequenceNum >> 8) & 0xFF;
        ++bufferPtrCopy;
        *bufferPtrCopy = this->sequenceNum & 0xFF;
        ++bufferPtrCopy;

        *bufferPtrCopy = (this->acknowledgementNum >> 24) & 0xFF;
        ++bufferPtrCopy;
        *bufferPtrCopy = (this->acknowledgementNum >> 16) & 0xFF;
        ++bufferPtrCopy;
        *bufferPtrCopy = (this->acknowledgementNum >> 8) & 0xFF;
        ++bufferPtrCopy;
        *bufferPtrCopy = this->acknowledgementNum & 0xFF;
        ++bufferPtrCopy;

        if (packetLength > Packet::sizeofPakcetHeaders()) {
            memcpy(bufferPtrCopy, this->applnLayerData, this->applnLayerDataSize);
        }

        // cout << "serializePacket: \n"
        //      << buffer << endl;

        return buffer;
    }
};

enum MONKE_TYPE {
    SENDER,
    RECEIVER,
};

class MONKE {
    // window, socket, handler everything

    // receiver_details;
    // sender_details;

   private:
    NetworkInformation myNetworkInfo;
    NetworkInformation peerNetworkInfo;
    MONKE_TYPE currentApplicationType;
    time_t timeout;

    // bool senderBusy, receiverBusy;
    // Sender details
    uint32_t senderSequenceNumber;
    uint32_t senderAcknowledgementNumber;
    uint16_t senderReceiveWindow;
    uint16_t window_base;
    uint16_t window_end;
    deque<tuple<time_t, bool, Packet*>> sendWindow;

    // Receiver details
    uint32_t receiverSequenceNumber;
    uint32_t receiverAcknowledgementNumber;
    uint16_t receiverReceiveWindow;
    vector<pair<uint32_t, pair<size_t, char*>>> receiveBuffer;
    char* allData;

   public:
    MONKE() {
        this->timeout = 5;
        // Sender details
        this->senderSequenceNumber = 0;
        this->senderAcknowledgementNumber = 0;
        this->senderReceiveWindow = (1 << WINDOW_SIZE_BITS);  // b'1000'
        this->window_base = 0;
        this->window_end = this->window_base + window_size - 1;
        this->sendWindow = deque<tuple<time_t, bool, Packet*>>();

        // Receiver details
        this->receiverSequenceNumber = 0;
        this->receiverAcknowledgementNumber = 0;
        this->receiverReceiveWindow = (1 << WINDOW_SIZE_BITS);  // b'1000'
        this->receiveBuffer = vector<pair<uint32_t, pair<size_t, char*>>>();
        this->allData = nullptr;
    }

    bool connect(char* destination_ip, size_t n) {
        this->currentApplicationType = SENDER;

        if ((this->myNetworkInfo.sockFD = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
            // deallocate everything ans exit
            cout << "[ERROR] connect() - socket() failed." << endl;
            exit(1);
        }

        // bind to port
        memset(&(this->myNetworkInfo.servAddr), 0, sizeof(this->myNetworkInfo.servAddr));
        this->myNetworkInfo.servAddr.sin_family = AF_INET;
        this->myNetworkInfo.servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        this->myNetworkInfo.servAddr.sin_port = htons(42069);
        int ret = bind(this->myNetworkInfo.sockFD, (struct sockaddr*)&(this->myNetworkInfo.servAddr), sizeof(this->myNetworkInfo.servAddr));
        if (ret < 0) {
            cout << "[ERROR] connect() - bind() failed." << endl;
        }

        cout << "[INFO] Created socket, bound to port number 42069." << endl;

        struct timeval timeout;
        timeout.tv_sec = SOCKET_READ_TIMEOUT_SEC;
        timeout.tv_usec = 0;
        setsockopt(this->myNetworkInfo.sockFD, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

        cout << ">>>> " << sizeof(destination_ip) / sizeof(char) << endl;  // Change this also

        this->peerNetworkInfo.ip = (char*)malloc(n);
        strcpy(this->peerNetworkInfo.ip, destination_ip);
        memset(&(this->peerNetworkInfo.servAddr), 0, sizeof(this->peerNetworkInfo.servAddr));
        this->peerNetworkInfo.servAddr.sin_family = AF_INET;
        this->peerNetworkInfo.servAddr.sin_addr.s_addr = inet_addr(destination_ip);
        this->peerNetworkInfo.servAddr.sin_port = htons(42070);

        // start 3 way handshake
        int numTimeoutsOrAttempts = 0;
        while (numTimeoutsOrAttempts < 5) {
            /* Send SYN Packet */
            // Packet synPacket(MESSAGE_TYPES::HELLO, 0, this->senderReceiveWindow, this->senderSequenceNumber, this->senderAcknowledgementNumber, nullptr);
            // uint32_t seqNum = 0;
            Packet synPacket(MESSAGE_TYPES::HELLO, 0, this->senderReceiveWindow, this->senderSequenceNumber, this->senderAcknowledgementNumber, nullptr);
            socklen_t sockLen = sizeof(this->peerNetworkInfo.servAddr);
            int n = sendto(this->myNetworkInfo.sockFD, synPacket.serializePacket(), synPacket.sizeofPakcet(), 0, (sockaddr*)&(this->peerNetworkInfo.servAddr), sockLen);
            if (n < 0) {
                cout << "[ERROR] Handshake failed" << endl;
                exit(1);
            }
            cout << "[INFO] Sent SYN Packet" << endl;

            /* Receive SYN-ACK Packet */
            char* buffer = (char*)malloc(Packet::maxPacketSize());  //  To check sender with peerNetworkInfo
            struct sockaddr_in servAddr;
            socklen_t servAddrSize = sizeof(servAddr);
            memset(&(servAddr), 0, servAddrSize);
            n = recvfrom(this->myNetworkInfo.sockFD, buffer, Packet::maxPacketSize(), 0, (sockaddr*)&(servAddr), &servAddrSize);
            if (n == -1) {
                if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
                    // Timeout
                    cout << "[WARNING] SYN-ACK receiving timed out." << endl;
                    ++numTimeoutsOrAttempts;
                    continue;
                } else {
                    // Some other error.
                    // deallocate `peerNetworkInfo` and exit
                    cout << "[ERROR] While receiving SYN-ACK Packet, non timeout error." << endl;
                    this->nonTimeoutErrorPrint();
                    exit(1);
                }
            }
            Packet synAckPacket = Packet::parsePacket(buffer, false);
            if (synAckPacket.getMessageType() == (MESSAGE_TYPES::HELLO | MESSAGE_TYPES::ACCEP)) {
                /* Send final ACK */
                cout << "[INFO] Received SYN-ACK Packet" << endl;
                Packet ackPacket(MESSAGE_TYPES::ACCEP, 0, this->senderReceiveWindow, this->senderSequenceNumber, this->senderAcknowledgementNumber, nullptr);
                socklen_t sockLen = sizeof(this->peerNetworkInfo.servAddr);
                int n = sendto(this->myNetworkInfo.sockFD, ackPacket.serializePacket(), synPacket.sizeofPakcet(), 0, (sockaddr*)&(this->peerNetworkInfo.servAddr), sockLen);
                if (n < 0) {
                    // deallocate all class members
                    cout << "[ERROR] Handshake failed" << endl;
                    exit(1);
                }
                cout << "[INFO] Sent ACK Packet" << endl;
                break;
            }
            // else if (synAckPacket.getMessageType() == MESSAGE_TYPES::DATA) {
            //     // Removing unwanted data in from the network buffer
            //     buffer = (char*)realloc(buffer, synAckPacket.getApplnLaterDataSize());
            //     struct sockaddr_in servAddr;
            //     servAddrSize = sizeof(servAddr);
            //     memset(&(servAddr), 0, servAddrSize);
            //     n = recvfrom(this->myNetworkInfo.sockFD, buffer, synAckPacket.getApplnLaterDataSize(), 0, (sockaddr*)&servAddr, &servAddrSize);
            //     memset(buffer, 0, synAckPacket.getApplnLaterDataSize());
            //     buffer = (char*)realloc(buffer, Packet::maxPacketSize());
            //     ++numTimeoutsOrAttempts;
            // }
        }

        if (numTimeoutsOrAttempts >= 5) {
            cout << "[INFO] Too many timeouts, can't handle this BS." << endl;
            return false;
        }

        //     check 5 timeouts and cut
        //     if (success) {
        //     instantiate Sender Window;
        //     instantiate Receiver Window;
        //     instantiate Handlers;
        // }
        cout << "[INFO] Been done that 3 way Handshake." << endl;
        return true;
    }

    void nonTimeoutErrorPrint() {
        switch (errno) {
            case EBADF: {
                cout << "Non time out error : recvfrom : EBADF" << endl;
                break;
            }
            case ECONNREFUSED: {
                cout << "Non time out error : recvfrom : ECONNREFUSED" << endl;
                break;
            }
            case EFAULT: {
                cout << "Non time out error : recvfrom : EFAULT" << endl;
                break;
            }
            case EINTR: {
                cout << "Non time out error : recvfrom : EINTR" << endl;
                break;
            }
            case EINVAL: {
                cout << "Non time out error : recvfrom : EINVAL" << endl;
                break;
            }
            case ENOMEM: {
                cout << "Non time out error : recvfrom : ENOMEM" << endl;
                break;
            }
            case ENOTCONN: {
                cout << "Non time out error : recvfrom : ENOTCONN" << endl;
                break;
            }
            case ENOTSOCK: {
                cout << "Non time out error : recvfrom : ENOTSOCK" << endl;
                break;
            }
            default: {
                cout << "Dead" << endl;
                break;
            }
        }
    }

    void listen() {
        this->currentApplicationType = RECEIVER;

        if ((this->myNetworkInfo.sockFD = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
            // deallocate everything ans exit
            cout << "[ERROR] listen() - socket() failed." << endl;
            exit(1);
        }

        // bind to port
        memset(&(this->myNetworkInfo.servAddr), 0, sizeof(this->myNetworkInfo.servAddr));
        this->myNetworkInfo.servAddr.sin_family = AF_INET;
        this->myNetworkInfo.servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        this->myNetworkInfo.servAddr.sin_port = htons(42070);
        int ret = bind(this->myNetworkInfo.sockFD, (struct sockaddr*)&(this->myNetworkInfo.servAddr), sizeof(this->myNetworkInfo.servAddr));
        if (ret < 0) {
            cout << "[ERROR] listen() - bind() failed." << endl;
        }

        cout << "[INFO] Created socket, bound to port number 42070." << endl;

        struct timeval timeout;
        timeout.tv_sec = SOCKET_READ_TIMEOUT_SEC;
        timeout.tv_usec = 0;
        setsockopt(this->myNetworkInfo.sockFD, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

        bool connected = false;
        while (!connected) {
            /* Wait for SYN Packet */
            int packetHeaderSize = Packet::maxPacketSize();
            char* buffer = (char*)malloc((packetHeaderSize + 1) * sizeof(char));
            memset(buffer, '\0', (packetHeaderSize + 1) * sizeof(char));
            socklen_t peerNetworkInfoSize = sizeof(this->peerNetworkInfo.servAddr);
            memset(&(this->peerNetworkInfo.servAddr), 0, peerNetworkInfoSize);
            int n = recvfrom(this->myNetworkInfo.sockFD, buffer, packetHeaderSize * sizeof(char), MSG_WAITALL, (struct sockaddr*)&(this->peerNetworkInfo.servAddr), &peerNetworkInfoSize);
            if (n == -1) {
                if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
                    // Timeout
                    cout << "[WARNING] SYN receiving timed out." << endl;
                    continue;
                } else {
                    // Some other error.
                    // deallocate `peerNetworkInfo` and exit
                    cout << "[ERROR] While receiving SYN Packet, non timeout error." << endl;
                    this->nonTimeoutErrorPrint();
                    exit(1);
                }
            }
            Packet synPacket = Packet::parsePacket(buffer, false);
            if (synPacket.getMessageType() == MESSAGE_TYPES::HELLO) {
                /* Send SYN-ACK */
                cout << "[INFO] Received SYN Packet." << endl;
                Packet synAckPacket((MESSAGE_TYPES::HELLO | MESSAGE_TYPES::ACCEP), 0, this->receiverReceiveWindow, this->receiverSequenceNumber, synPacket.getSequenceNum(), nullptr);
                socklen_t sockLen = sizeof(this->peerNetworkInfo.servAddr);
                int n = sendto(this->myNetworkInfo.sockFD, synAckPacket.serializePacket(), synAckPacket.sizeofPakcet(), 0, (sockaddr*)&(this->peerNetworkInfo.servAddr), sockLen);
                if (n < 0) {
                    // deallocate all class members
                    cout << "[ERROR] Handshake failed" << endl;
                    exit(1);
                }
                cout << "[INFO] Sent SYN-ACK Packet." << endl;
                /* Wait for ACK Packet */
                memset(buffer, 0, synPacket.getApplnLaterDataSize());
                struct sockaddr_in servAddr;
                socklen_t servAddrSize = sizeof(servAddr);
                memset(&servAddr, 0, servAddrSize);
                n = recvfrom(this->myNetworkInfo.sockFD, buffer, Packet::maxPacketSize(), 0, (sockaddr*)&(servAddr), &servAddrSize);
                if (n == -1) {
                    if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
                        // Timeout
                        cout << "[WARNING] ACK receiving timed out." << endl;
                        continue;
                    } else {
                        // Some other error.
                        // deallocate `peerNetworkInfo` and exit
                        cout << "[ERROR] While receiving ACK Packet, non timeout error." << endl;
                        this->nonTimeoutErrorPrint();
                        exit(1);
                    }
                }
                Packet ackPacket = Packet::parsePacket(buffer, false);
                if (ackPacket.getMessageType() == MESSAGE_TYPES::ACCEP) {
                    // Handshake Done
                    cout << "[INFO] Received ACK Packet." << endl;
                    connected = true;
                }
                // else if (ackPacket.getMessageType() == MESSAGE_TYPES::DATA) {
                //     // Removing unwanted data in from the network buffer
                //     buffer = (char*)realloc(buffer, ackPacket.getApplnLaterDataSize());
                //     servAddrSize = sizeof(servAddr);
                //     memset(&servAddr, 0, servAddrSize);
                //     n = recvfrom(this->myNetworkInfo.sockFD, buffer, ackPacket.getApplnLaterDataSize(), 0, (sockaddr*)&(servAddr), &servAddrSize);
                //     memset(buffer, 0, ackPacket.getApplnLaterDataSize());
                //     buffer = (char*)realloc(buffer, Packet::sizeofPakcetHeaders());
                // }
            }
            // else if (synPacket.getMessageType() == MESSAGE_TYPES::DATA) {
            //     // Removing unwanted data in from the network buffer
            //     buffer = (char*)realloc(buffer, synPacket.getApplnLaterDataSize());
            //     struct sockaddr_in servAddr;
            //     socklen_t servAddrSize = sizeof(servAddr);
            //     memset(&servAddr, 0, servAddrSize);
            //     n = recvfrom(this->myNetworkInfo.sockFD, buffer, synPacket.getApplnLaterDataSize(), 0, (sockaddr*)&(servAddr), &servAddrSize);
            //     memset(buffer, 0, synPacket.getApplnLaterDataSize());
            //     buffer = (char*)realloc(buffer, Packet::sizeofPakcetHeaders());
            // }
            free(buffer);
        }

        // wait for a packet (SYN Packet)
        // start 3 way handshake

        // if(success){
        //     instantiate Sender Window;
        //     instantiate Receiver Window;
        //     instantiate Handlers;
        // }
        cout << "[INFO] Been done that 3 way Handshake." << endl;
    }

    void updateWindowDetails() {
        ++this->window_base;
        this->window_end = (this->window_base + window_size - 1) % ((uint64_t)(1) << 32);  // automatic modulo 2^32
    }

    void send(char* data, size_t n) {
        int dataSize = n;
        int index = 0;
        uint32_t seqNum = this->senderSequenceNumber;
        vector<Packet*> packetList;
        while (index < dataSize) {
            char* dataChunk = nullptr;
            size_t dataChunkSize = 0;
            Packet* packet = nullptr;
            // Chunk Data Accordingly
            if (index + MAXIMUM_PAYLOAD < dataSize) {
                dataChunk = (char*)malloc(sizeof(char) * MAXIMUM_PAYLOAD);
                memcpy(dataChunk, data + index, MAXIMUM_PAYLOAD);
                dataChunkSize = MAXIMUM_PAYLOAD;
                index += MAXIMUM_PAYLOAD;
            } else {
                dataChunk = (char*)malloc(sizeof(char) * (dataSize - index));
                memcpy(dataChunk, data + index, (dataSize - index));
                dataChunkSize = (dataSize - index);
                index += (dataSize - index);
            }
            // Store packet
            if (dataChunk != nullptr) {
                packet = new Packet(MESSAGE_TYPES::DATA, dataChunkSize, this->senderReceiveWindow, seqNum, this->senderAcknowledgementNumber, dataChunk);
                packetList.push_back(packet);
                ++seqNum;
            }
        }
        // maybe FIN packet condition
        Packet* packet = new Packet(MESSAGE_TYPES::FIN, 0, this->senderReceiveWindow, seqNum, this->senderAcknowledgementNumber, nullptr);
        packetList.push_back(packet);
        ++seqNum;

        /* 1. Send as many packets as possible. */
        bool doneSending = false;
        bool finSent = false;
        unsigned int packetListIndex = 0;
        while (!doneSending) {
            if (finSent && this->sendWindow.empty()) {
                // Send exit condition = FIN has been sent and Send Window is empty
                break;
            }
            while ((this->senderSequenceNumber - 1) != this->window_end) {
                if (packetListIndex < packetList.size()) {
                    Packet* packet = packetList[packetListIndex];
                    this->sendWindow.push_back(make_tuple(time(NULL), false, packet));
                    socklen_t peerNetworkInfoSize = sizeof(this->peerNetworkInfo.servAddr);
                    char* packetBuffer = packet->serializePacket();
                    int n = sendto(
                        this->myNetworkInfo.sockFD,
                        packetBuffer,
                        packet->sizeofPakcet(),
                        0,
                        (sockaddr*)&(this->peerNetworkInfo.servAddr),
                        peerNetworkInfoSize);
                    if (packet->getMessageType() == MESSAGE_TYPES::DATA) {
                        cout << "[INFO] Sent a DATA packet with seq num: " << packet->getSequenceNum() << endl;
                    } else if (packet->getMessageType() == MESSAGE_TYPES::FIN) {
                        finSent = true;
                        cout << "[INFO] Sent a FIN packet with seq num: " << packet->getSequenceNum() << endl;
                    }
                    if (n < 0) {
                        cout << "[ERROR] Sending failed." << endl;
                        exit(1);
                    }
                    ++packetListIndex;
                    ++senderSequenceNumber;
                } else {
                    break;
                }
            }

            /*
				1B => 1 DATA + 1 FIN

				senderSequenceNumber = 0
				packetListIndex = 0
					send DATA
					incr seqNUM
				senderSequenceNumber = 1
				packetListIndex = 1
					send FIN
					incr seqNUM
				senderSequenceNumber = 2
				packetListIndex = 2
				

			*/

            // // While there is data available to send
            // while (index < dataSize) {
            //     char* dataChunk = nullptr;
            //     size_t dataChunkSize = 0;
            //     Packet* packet = nullptr;
            //     // While window has space
            //     while ((this->senderSequenceNumber - 1) != this->window_end) {
            //         // Chunk Data Accordingly
            //         if (index + MAXIMUM_PAYLOAD < dataSize) {
            //             dataChunk = (char*)malloc(sizeof(char) * MAXIMUM_PAYLOAD);
            //             memcpy(dataChunk, data + index, MAXIMUM_PAYLOAD);
            //             dataChunkSize = MAXIMUM_PAYLOAD;
            //             index += MAXIMUM_PAYLOAD;
            //         } else {
            //             dataChunk = (char*)malloc(sizeof(char) * (dataSize - index));
            //             memcpy(dataChunk, data + index, (dataSize - index));
            //             dataChunkSize = (dataSize - index);
            //             index += (dataSize - index);
            //         }
            //         // Store packet
            //         if (dataChunk != nullptr) {
            //             packet = new Packet(MESSAGE_TYPES::DATA, dataChunkSize, this->senderReceiveWindow, this->senderSequenceNumber, this->senderAcknowledgementNumber, dataChunk);
            //             this->sendWindow.push_back(make_tuple(time(0), false, packet));
            //         } else {
            //             // maybe FIN packet condition
            //             packet = new Packet(MESSAGE_TYPES::FIN, 0, this->senderReceiveWindow, this->senderSequenceNumber, this->senderAcknowledgementNumber, nullptr);
            //             this->sendWindow.push_back(make_tuple(time(0), false, packet));
            //             finSent = true;
            //         }
            //         /* Send packet */
            //         socklen_t peerNetworkInfoSize = sizeof(this->peerNetworkInfo.servAddr);
            //         int n = sendto(this->myNetworkInfo.sockFD, packet, packet->sizeofPakcet(), 0, (sockaddr*)&(this->peerNetworkInfo.servAddr), peerNetworkInfoSize);
            //         if (n < 0) {
            //             cout << "[ERROR] send() - Sending DATA failed." << endl;
            //             exit(1);
            //         }
            //         ++senderSequenceNumber;
            //     }
            //     if ((this->senderSequenceNumber - 1) == this->window_end) {
            //         break;
            //     }
            // }

            /* 2. Receive as many ACKs as possible. */
            char* buffer = (char*)malloc(Packet::maxPacketSize());
            struct sockaddr_in servAddr;
            socklen_t servAddrSize = sizeof(servAddr);
            memset(&(servAddr), 0, servAddrSize);
            int n = recvfrom(this->myNetworkInfo.sockFD, buffer, Packet::maxPacketSize(), 0, (sockaddr*)&(servAddr), &servAddrSize);
            while (n > 0) {
                Packet received_packet = Packet::parsePacket(buffer, false);
                if (received_packet.getMessageType() == MESSAGE_TYPES::ACCEP) {
                    uint32_t ackPacketACKNum = received_packet.getAcknowledgementNum();  // ackPacketACKNum will be sent packet SEQ_NUM
                    cout << "[INFO] Received ACK packet for seq num: " << ackPacketACKNum << endl;
                    if (this->window_base == ackPacketACKNum) {  //  base ------- end+1seq
                        this->sendWindow.pop_front();
                        this->updateWindowDetails();
                        while (get<1>(this->sendWindow.front())) {
                            this->sendWindow.pop_front();
                            this->updateWindowDetails();
                        }
                    } else {
                        get<1>(this->sendWindow[ackPacketACKNum - this->window_base]) = true;
                    }
                }
                if (this->sendWindow.empty()) {
                    break;
                }
                // else if (received_packet.getMessageType() == MESSAGE_TYPES::DATA) {
                //     buffer = (char*)realloc(buffer, received_packet.getApplnLaterDataSize());
                //     n = recvfrom(this->myNetworkInfo.sockFD, buffer, received_packet.getApplnLaterDataSize(), 0, (sockaddr*)&(this->peerNetworkInfo.servAddr), (socklen_t*)sizeof(this->peerNetworkInfo.servAddr));
                //     memset(buffer, 0, received_packet.getApplnLaterDataSize());
                //     buffer = (char*)realloc(buffer, Packet::sizeofPakcetHeaders());
                // }
                memset(&(servAddr), 0, servAddrSize);
                n = recvfrom(this->myNetworkInfo.sockFD, buffer, Packet::maxPacketSize(), 0, (sockaddr*)&(servAddr), &servAddrSize);
            }
            cout << "Size of sendWindow: " << this->sendWindow.size() << endl;
            cout << "[INFO] Window ends: " << this->window_base << " ----- " << this->window_end << endl;
            if (n < 0) {
                // deallocate everything and then cut
                cout << "[ERROR] send() - Receiving ACKs failed." << endl;
                exit(1);
            }

            //third: receive chance over, resend unack'd
            /* 3. Resend un-ACK'd packets that have timeout. */
            for (auto& packetDetails : this->sendWindow) {
                time_t timeElapsed = time(0) - get<0>(packetDetails);
                if (timeElapsed > this->timeout) {
                    Packet* packet = get<2>(packetDetails);
                    // code for re-sending packet
                    socklen_t peerNetworkInfoSize = (socklen_t)sizeof(this->peerNetworkInfo.servAddr);
                    char* packetBuffer = packet->serializePacket();
                    int n = sendto(
                        this->myNetworkInfo.sockFD,
                        packetBuffer,
                        packet->sizeofPakcet(),
                        0,
                        (sockaddr*)&(this->peerNetworkInfo.servAddr),
                        peerNetworkInfoSize);

                    cout << "[INFO] Resent packet with seq num: " << packet->getSequenceNum() << endl;
                    if (n < 0) {
                        cout << "[ERROR] Sending failed." << endl;
                        exit(1);
                    }
                }
            }
        }
    }

    int recv(char** userBuffer) {
        int dataSizeReceived = -1;
        bool doneReceiving = false;
        bool timeOutCondition = false;
        while (!doneReceiving) {
            char* buffer = (char*)malloc(Packet::maxPacketSize());
            struct sockaddr_in servAddr;
            socklen_t servAddrSize = sizeof(servAddr);
            memset(&(servAddr), 0, servAddrSize);
            int n = recvfrom(this->myNetworkInfo.sockFD, buffer, Packet::maxPacketSize(), 0, (sockaddr*)&(servAddr), &servAddrSize);
            if (n == -1) {
                if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
                    // Timeout
                    cout << "[WARNING] recvfrom() - timed out." << endl;
                    dataSizeReceived = 0;
                    timeOutCondition = true;
                    doneReceiving = true;
                } else {
                    // Some other error.
                    // deallocate `peerNetworkInfo` and exit
                    cout << "[ERROR] While receiving SYN Packet, non timeout error." << endl;
                    this->nonTimeoutErrorPrint();
                    exit(1);
                }
            }
            Packet received_packet = Packet::parsePacket(buffer, false);
            MESSAGE_TYPES messageType = received_packet.getMessageType();
            if (messageType == MESSAGE_TYPES::DATA) {
                cout << "[INFO] Received a DATA packet with seq num: " << received_packet.getSequenceNum() << endl;
                // cout << ">>>>>> Received DATA packet's appln layer data is: " << received_packet.getApplnLaterDataPtr() << endl;
                // size_t applnLayerDataSize = received_packet.getApplnLaterDataSize() * sizeof(char);
                // char* dataBuffer = (char*)malloc(applnLayerDataSize);
                // memset(dataBuffer, 0, applnLayerDataSize);
                // // read applnLayerDataSize amount of data again from buffer
                // memset(&(servAddr), 0, servAddrSize);
                // n = recvfrom(this->myNetworkInfo.sockFD, dataBuffer, applnLayerDataSize, 0, (sockaddr*)&(servAddr), &servAddrSize);
                // if (n == -1) {
                //     if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
                //         // Timeout
                //         cout << "[WARNING] SYN receiving timed out." << endl;
                //         doneReceiving = true;
                //     } else {
                //         // Some other error.
                //         // deallocate `peerNetworkInfo` and exit
                //         cout << "[ERROR] While receiving SYN Packet, non timeout error." << endl;
                //         this->nonTimeoutErrorPrint();
                //         exit(1);
                //     }
                // }
                // cout << "DATA size: " << applnLayerDataSize << endl;
                // char* dataBufferCopy = dataBuffer;
                // for (int i = 0; i < applnLayerDataSize; ++i) {
                //     cout << dataBufferCopy[i] << "-";
                // }
                // cout << dataBuffer << endl;
                // memcpy(received_packet.getApplnLaterDataPtr(), dataBuffer, received_packet.getApplnLaterDataSize());
                // cout << "[INFO] Read data of the DATA packet and memcpy'd into packet." << endl;
                this->receiveBuffer.push_back(make_pair(received_packet.getSequenceNum(), make_pair(received_packet.getApplnLaterDataSize(), received_packet.getApplnLaterDataPtr())));
                --this->receiverReceiveWindow;
                Packet packet = Packet(MESSAGE_TYPES::ACCEP, 0, this->receiverReceiveWindow, this->receiverSequenceNumber, received_packet.getSequenceNum(), nullptr);
                // C++.send(); an a ACCEP msg
                socklen_t peerNetworkInfoSize = (socklen_t)sizeof(this->peerNetworkInfo.servAddr);
                char* packetBuffer = packet.serializePacket();
                n = sendto(
                    this->myNetworkInfo.sockFD,
                    packetBuffer,
                    Packet::sizeofPakcetHeaders(),
                    0,
                    (sockaddr*)&(this->peerNetworkInfo.servAddr),
                    peerNetworkInfoSize);

                if (n < 0) {
                    cout << "[ERROR] Sending failed." << endl;
                    exit(1);
                }
                cout << "[INFO] Sent ACCEP Packet in response to the DATA packet." << endl;
            } else if (messageType == MESSAGE_TYPES::FIN) {
                //a certain amount of data has been successfully received
                Packet packet = Packet(MESSAGE_TYPES::ACCEP, 0, this->receiverReceiveWindow, this->receiverSequenceNumber, received_packet.getSequenceNum(), nullptr);
                socklen_t peerNetworkInfoSize = (socklen_t)sizeof(this->peerNetworkInfo.servAddr);
                char* packetBuffer = packet.serializePacket();
                n = sendto(
                    this->myNetworkInfo.sockFD,
                    packetBuffer,
                    Packet::sizeofPakcetHeaders(),
                    0,
                    (sockaddr*)&(this->peerNetworkInfo.servAddr),
                    peerNetworkInfoSize);

                if (n < 0) {
                    cout << "[ERROR] Sending failed." << endl;
                    exit(1);
                }

                // Packet the data to send to user
                sort(this->receiveBuffer.begin(), this->receiveBuffer.end(), [](const pair<uint32_t, pair<size_t, char*>>& p1, const pair<uint32_t, pair<size_t, char*>>& p2) {
                    return p1.first < p2.first;
                });
                dataSizeReceived = 0;
                for (auto& Buffer : this->receiveBuffer) {
                    dataSizeReceived += Buffer.second.first;
                }
                dataSizeReceived += 1;  // For the most irritating char* shit a.k.a null char at the end of the string
                this->allData = (char*)malloc(dataSizeReceived * sizeof(char));
                memset(this->allData, 0, dataSizeReceived * sizeof(char));
                char* allDataCopy = this->allData;
                for (auto& Buffer : this->receiveBuffer) {
                    memcpy(allDataCopy, Buffer.second.second, Buffer.second.first * sizeof(char));
                    allDataCopy += Buffer.second.first;
                }
                // memcpy(allData, allData, dataSizeReceived * sizeof(char));

                //setup to reinitialize few globals
                this->receiverReceiveWindow = (1 << WINDOW_SIZE_BITS);  // b'1000'
                this->receiveBuffer = vector<pair<uint32_t, pair<size_t, char*>>>();

                // Cut from while loop
                doneReceiving = true;
            } else if (messageType == MESSAGE_TYPES::REJECC) {
                //start a disconnect handshake

                // Cut from while loop
                doneReceiving = true;
            }
        }

        if (!timeOutCondition) {
            if (*userBuffer != nullptr) {
                free(*userBuffer);
                *userBuffer = (char*)malloc(dataSizeReceived * sizeof(char));
            } else {
                *userBuffer = (char*)malloc(dataSizeReceived * sizeof(char));
            }
            // cout << hex << userBuffer << endl;
            memcpy(*userBuffer, this->allData, dataSizeReceived * sizeof(char));
            free(this->allData);
            this->allData = nullptr;
        }
        return dataSizeReceived;
    }
};