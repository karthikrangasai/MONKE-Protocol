#include <iostream>
#include <map>
#include <cassert>
#include <cstring>
#include <vector>
#include <tuple>
#include <ctime>
#include <stdlib.h>
#include <deque>
#include <fstream>

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
const unsigned int SOCKET_READ_TIMEOUT_SEC = 2;
const unsigned int MAXIMUM_PAYLOAD = 255;

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
            assert((int)applnLayerDataSize <= MAXIMUM_PAYLOAD);
            this->messageType = messageType;
            this->applnLayerDataSize = applnLayerDataSize;
            this->receiveWindow = receiveWindow;
            this->sequenceNum = sequenceNum;
            this->acknowledgementNum = acknowledgementNum;
            this->applnLayerData = (char*)malloc((int)applnLayerDataSize);
            memset(this->applnLayerData, 0, (int)applnLayerDataSize);
            if (applnLayerData != nullptr) {
                memcpy(this->applnLayerData, applnLayerData, (int)applnLayerDataSize);
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
        size_t headerSize = sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint16_t) + sizeof(uint32_t) + sizeof(uint32_t) + (sizeof(char) * (int)this->applnLayerDataSize);
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
        // this->logMessage(">>>>> Parse Packet:\n";
        char* bufferPointer = buffer;

        uint8_t messageType = (uint8_t)(*bufferPointer);
        ++bufferPointer;

        uint8_t applnLayerDataSize = (uint8_t)(*bufferPointer);
        ++bufferPointer;

        uint16_t receiveWindow = (uint16_t)(*bufferPointer << 8);
        ++bufferPointer;
        receiveWindow = receiveWindow | *bufferPointer;
        ++bufferPointer;

        uint32_t sequenceNum = 0;
        sequenceNum = sequenceNum | ((uint32_t)((uint8_t)(*bufferPointer) << 24) & 0xFF000000);
        ++bufferPointer;
        sequenceNum = sequenceNum | ((uint32_t)((uint8_t)(*bufferPointer) << 16) & 0x00FF0000);
        ++bufferPointer;
        sequenceNum = sequenceNum | ((uint16_t)((uint8_t)(*bufferPointer) << 8) & 0xFF00);
        ++bufferPointer;
        sequenceNum = sequenceNum | (((uint8_t)(*bufferPointer)) & 0xFF);
        ++bufferPointer;

        uint32_t acknowledgementNum = 0;
        acknowledgementNum = acknowledgementNum | ((uint32_t)((uint8_t)(*bufferPointer) << 24) & 0xFF000000);
        ++bufferPointer;
        acknowledgementNum = acknowledgementNum | ((uint32_t)((uint8_t)(*bufferPointer) << 16) & 0x00FF0000);
        ++bufferPointer;
        acknowledgementNum = acknowledgementNum | ((uint16_t)((uint8_t)(*bufferPointer) << 8) & 0xFF00);
        ++bufferPointer;
        acknowledgementNum = acknowledgementNum | (((uint8_t)(*bufferPointer)) & 0xFF);
        ++bufferPointer;

        char* applnLayerData = nullptr;
        if ((int)applnLayerDataSize > 0) {
            applnLayerData = (char*)malloc(sizeof(char) * (int)applnLayerDataSize);
            memset(applnLayerData, 0, sizeof(char) * (int)applnLayerDataSize);
            memcpy(applnLayerData, bufferPointer, sizeof(char) * (int)applnLayerDataSize);
            return Packet(messageType, applnLayerDataSize, receiveWindow, sequenceNum, acknowledgementNum, applnLayerData);
        }

        // this->logMessage("      messageType: " << (int)messageType << "\n"
        //      << "      applnLayerDataSize: " << (int)applnLayerDataSize << "\n"
        //      << "      receiveWindow: " << receiveWindow << "\n"
        //      << "      sequenceNum: " << sequenceNum << "\n"
        //      << "      acknowledgementNum: " << acknowledgementNum);

        return Packet(messageType, applnLayerDataSize, receiveWindow, sequenceNum, acknowledgementNum, applnLayerData);
    }

    char* serializePacket() {
        size_t packetLength = (this->sizeofPakcet()) * sizeof(char);
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
            memcpy(bufferPtrCopy, this->applnLayerData, (int)(this->applnLayerDataSize));
        }

        // this->logMessage("serializePacket: \n"
        //      << buffer);

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

    fstream logFile;

   public:
    MONKE() {
        this->timeout = 1;
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

        this->logFile = fstream("output.log", fstream::out);
    }

    void logMessage(string s) {
        this->logFile << s << endl;
    }

    bool connect(char* destination_ip, size_t n) {
        this->currentApplicationType = SENDER;

        if ((this->myNetworkInfo.sockFD = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
            // deallocate everything ans exit
            this->logMessage("[ERROR] connect() - socket() failed.");
            exit(1);
        }

        // bind to port
        memset(&(this->myNetworkInfo.servAddr), 0, sizeof(this->myNetworkInfo.servAddr));
        this->myNetworkInfo.servAddr.sin_family = AF_INET;
        this->myNetworkInfo.servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        this->myNetworkInfo.servAddr.sin_port = htons(42069);
        int ret = bind(this->myNetworkInfo.sockFD, (struct sockaddr*)&(this->myNetworkInfo.servAddr), sizeof(this->myNetworkInfo.servAddr));
        if (ret < 0) {
            this->logMessage("[ERROR] connect() - bind() failed.");
        }

        this->logMessage("[INFO] Created socket, bound to port number 42069.");

        struct timeval timeout;
        timeout.tv_sec = SOCKET_READ_TIMEOUT_SEC;
        timeout.tv_usec = 0;
        setsockopt(this->myNetworkInfo.sockFD, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

        // this->logMessage(">>>> " << strlen(destination_ip) / sizeof(char));  // Change this also

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
            Packet synPacket(MESSAGE_TYPES::HELLO, 0, this->senderReceiveWindow, this->senderSequenceNumber, this->senderAcknowledgementNumber, nullptr);
            socklen_t sockLen = sizeof(this->peerNetworkInfo.servAddr);
            int n = sendto(this->myNetworkInfo.sockFD, synPacket.serializePacket(), synPacket.sizeofPakcet(), 0, (sockaddr*)&(this->peerNetworkInfo.servAddr), sockLen);
            if (n < 0) {
                this->logMessage("[ERROR] Handshake failed");
                exit(1);
            }
            this->logMessage("[INFO] Sent SYN Packet");

            /* Receive SYN-ACK Packet */
            char* buffer = (char*)malloc(Packet::maxPacketSize());  //  To check sender with peerNetworkInfo
            struct sockaddr_in servAddr;
            socklen_t servAddrSize = sizeof(servAddr);
            memset(&(servAddr), 0, servAddrSize);
            n = recvfrom(this->myNetworkInfo.sockFD, buffer, Packet::maxPacketSize(), 0, (sockaddr*)&(servAddr), &servAddrSize);
            if (n == -1) {
                if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
                    // Timeout
                    this->logMessage("[WARNING] SYN-ACK receiving timed out.");
                    ++numTimeoutsOrAttempts;
                    continue;
                } else {
                    // Some other error.
                    // deallocate `peerNetworkInfo` and exit
                    this->logMessage("[ERROR] While receiving SYN-ACK Packet, non timeout error.");
                    exit(1);
                }
            }
            Packet synAckPacket = Packet::parsePacket(buffer, false);
            if (synAckPacket.getMessageType() == (MESSAGE_TYPES::HELLO | MESSAGE_TYPES::ACCEP)) {
                /* Send final ACK */
                this->logMessage("[INFO] Received SYN-ACK Packet");
                Packet ackPacket(MESSAGE_TYPES::ACCEP, 0, this->senderReceiveWindow, this->senderSequenceNumber, this->senderAcknowledgementNumber, nullptr);
                socklen_t sockLen = sizeof(this->peerNetworkInfo.servAddr);
                int n = sendto(this->myNetworkInfo.sockFD, ackPacket.serializePacket(), synPacket.sizeofPakcet(), 0, (sockaddr*)&(this->peerNetworkInfo.servAddr), sockLen);
                if (n < 0) {
                    // deallocate all class members
                    this->logMessage("[ERROR] Handshake failed");
                    exit(1);
                }
                this->logMessage("[INFO] Sent ACK Packet");
                break;
            }
            free(buffer);
        }

        if (numTimeoutsOrAttempts >= 5) {
            this->logMessage("[INFO] Too many timeouts, can't handle this BS.");
            return false;
        }
        this->logMessage("[INFO] Been done that 3 way Handshake.");
        return true;
    }

    void listen() {
        this->currentApplicationType = RECEIVER;

        if ((this->myNetworkInfo.sockFD = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
            // deallocate everything ans exit
            this->logMessage("[ERROR] listen() - socket() failed.");
            exit(1);
        }

        // bind to port
        memset(&(this->myNetworkInfo.servAddr), 0, sizeof(this->myNetworkInfo.servAddr));
        this->myNetworkInfo.servAddr.sin_family = AF_INET;
        this->myNetworkInfo.servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        this->myNetworkInfo.servAddr.sin_port = htons(42070);
        int ret = bind(this->myNetworkInfo.sockFD, (struct sockaddr*)&(this->myNetworkInfo.servAddr), sizeof(this->myNetworkInfo.servAddr));
        if (ret < 0) {
            this->logMessage("[ERROR] listen() - bind() failed.");
        }

        this->logMessage("[INFO] Created socket, bound to port number 42070.");

        struct timeval timeout;
        timeout.tv_sec = 50 * SOCKET_READ_TIMEOUT_SEC;
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
                    this->logMessage("[WARNING] SYN receiving timed out.");
                    continue;
                } else {
                    // Some other error.
                    // deallocate `peerNetworkInfo` and exit
                    this->logMessage("[ERROR] While receiving SYN Packet, non timeout error.");
                    exit(1);
                }
            }
            Packet synPacket = Packet::parsePacket(buffer, false);
            if (synPacket.getMessageType() == MESSAGE_TYPES::HELLO) {
                /* Send SYN-ACK */
                this->logMessage("[INFO] Received SYN Packet.");
                Packet synAckPacket((MESSAGE_TYPES::HELLO | MESSAGE_TYPES::ACCEP), 0, this->receiverReceiveWindow, this->receiverSequenceNumber, synPacket.getSequenceNum(), nullptr);
                socklen_t sockLen = sizeof(this->peerNetworkInfo.servAddr);
                int n = sendto(this->myNetworkInfo.sockFD, synAckPacket.serializePacket(), synAckPacket.sizeofPakcet(), 0, (sockaddr*)&(this->peerNetworkInfo.servAddr), sockLen);
                if (n < 0) {
                    // deallocate all class members
                    this->logMessage("[ERROR] Handshake failed");
                    exit(1);
                }
                this->logMessage("[INFO] Sent SYN-ACK Packet.");
                /* Wait for ACK Packet */
                memset(buffer, 0, (int)synPacket.getApplnLaterDataSize());
                struct sockaddr_in servAddr;
                socklen_t servAddrSize = sizeof(servAddr);
                memset(&servAddr, 0, servAddrSize);
                n = recvfrom(this->myNetworkInfo.sockFD, buffer, Packet::maxPacketSize(), 0, (sockaddr*)&(servAddr), &servAddrSize);
                if (n == -1) {
                    if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
                        // Timeout
                        this->logMessage("[WARNING] ACK receiving timed out.");
                        continue;
                    } else {
                        // Some other error.
                        // deallocate `peerNetworkInfo` and exit
                        this->logMessage("[ERROR] While receiving ACK Packet, non timeout error.");
                        exit(1);
                    }
                }
                Packet ackPacket = Packet::parsePacket(buffer, false);
                if (ackPacket.getMessageType() == MESSAGE_TYPES::ACCEP) {
                    // Handshake Done
                    this->logMessage("[INFO] Received ACK Packet.");
                    connected = true;
                }
            }
            free(buffer);
        }
        this->logMessage("[INFO] Been done that 3 way Handshake.");
    }

    void updateWindowDetails() {
        ++this->window_base;
        this->window_end = (this->window_base + window_size - 1) % ((uint64_t)(1) << 32);  // automatic modulo 2^32
    }

    void send(char* data, size_t numBytes) {
        // this->logMessage("[SEND CALL]: \n"
        //      << data)
        //     );
        int dataSize = numBytes;
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
                // this->logMessage("[INFO] DataChunk created is:\n"
                //      << dataChunk << "\n"
                //      << ">>>>>> Size of dataChunk " << dataChunkSize << " bytes.")
                //     );
                packet = new Packet(MESSAGE_TYPES::DATA, dataChunkSize, this->senderReceiveWindow, seqNum, this->senderAcknowledgementNumber, dataChunk);
                packetList.push_back(packet);
                ++seqNum;
            }
        }
        // maybe FIN packet condition
        Packet* packet = new Packet(MESSAGE_TYPES::FIN, 0, this->senderReceiveWindow, seqNum, this->senderAcknowledgementNumber, nullptr);
        packetList.push_back(packet);
        ++seqNum;

        this->logMessage("[INFO] Packet List has " + to_string(packetList.size()) + " packets.");
        /* 1. Send as many packets as possible. */
        bool doneSending = false;
        bool finSent = false;
        unsigned int packetListIndex = 0;
        int numAttempts = 0;
        bool peerCut = false;
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
                        this->logMessage("[INFO] Sent a DATA packet with seq num: " + to_string(packet->getSequenceNum()) + " of size " + to_string(packet->sizeofPakcet()) + " bytes whose applnLayerData has size " + to_string((int)(packet->getApplnLaterDataSize())) + " bytes.");
                    } else if (packet->getMessageType() == MESSAGE_TYPES::FIN) {
                        finSent = true;
                        this->logMessage("[INFO] Sent a FIN packet with seq num: " + to_string(packet->getSequenceNum()));
                    }
                    if (n < 0) {
                        this->logMessage("[ERROR] Sending failed.");
                        exit(1);
                    }
                    ++packetListIndex;
                    ++(this->senderSequenceNumber);
                    this->logMessage("[INFO] Updated seq num to: " + to_string(this->senderSequenceNumber));
                    this->logMessage("[INFO] Updated packetIndex to: " + to_string(packetListIndex));
                } else {
                    break;
                }
            }

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
                    if (this->window_base == (ackPacketACKNum % (1 << 16))) {            //  base ------- end+1seq
                        this->sendWindow.pop_front();
                        this->updateWindowDetails();
                        this->logMessage("[INFO] Received ACK packet for seq num: " + to_string(ackPacketACKNum) + ", removed it.");
                        while (get<1>(this->sendWindow.front())) {
                            this->logMessage("[INFO] Removing from window packet with seq num: " + to_string(get<2>(this->sendWindow.front())->getSequenceNum()));
                            this->sendWindow.pop_front();
                            this->updateWindowDetails();
                        }
                    } else {
                        for (auto& packetDetails : this->sendWindow) {
                            if (get<2>(packetDetails)->getSequenceNum() == ackPacketACKNum) {
                                get<1>(packetDetails) = true;
                                this->logMessage("[INFO] Received ACK packet for seq num: " + to_string(ackPacketACKNum) + ", setting it and not removing it.");
                            }
                        }
                        // get<1>(this->sendWindow[(ackPacketACKNum % (1 << 16)) - this->window_base]) = true;
                    }
                }
                if (this->sendWindow.empty()) {
                    break;
                }
                // else {
                //     this->logMessage("[INFO] Front packet number: " + to_string(get<2>(this->sendWindow.front())->getSequenceNum()));
                //     while (get<1>(this->sendWindow.front())) {
                //         this->logMessage("[INFO] Removing from window packet with seq num: " + to_string(get<2>(this->sendWindow.front())->getSequenceNum()));
                //         this->sendWindow.pop_front();
                //         this->updateWindowDetails();
                //     }
                // }
                memset(&(servAddr), 0, servAddrSize);
                n = recvfrom(this->myNetworkInfo.sockFD, buffer, Packet::maxPacketSize(), 0, (sockaddr*)&(servAddr), &servAddrSize);
            }
            free(buffer);
            this->logMessage("Size of sendWindow: " + to_string(this->sendWindow.size()));
            this->logMessage("[INFO] Window ends: " + to_string(this->window_base) + " ----- " + to_string(this->window_end));
            if (n == -1) {
                if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
                    // Timeout
                    this->logMessage("[WARNING] ACK receiving timed out.");
                    // continue;
                    ++numAttempts;
                    if (numAttempts > SOCKET_READ_TIMEOUT_SEC) {
                        peerCut = true;
                        break;
                    }
                } else {
                    // Some other error.
                    // deallocate `peerNetworkInfo` and exit
                    this->logMessage("[ERROR] While receiving ACK Packet, non timeout error.");
                    exit(1);
                }
            }

            //third: receive chance over, resend unack'd
            /* 3. Resend un-ACK'd packets that have timeout. */
            for (auto& packetDetails : this->sendWindow) {
                time_t timeElapsed = time(0) - get<0>(packetDetails);
                this->logMessage("[CHECKING] Inside resend loop with packet of seq num " + to_string(get<2>(packetDetails)->getSequenceNum()) + " with time elapsed " + to_string(timeElapsed));
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
                    get<0>(packetDetails) = time(0);
                    this->logMessage("[INFO] Resent and Reset timer for packet with seq num: " + to_string(packet->getSequenceNum()));
                    if (n < 0) {
                        this->logMessage("[ERROR] Sending failed.");
                        exit(1);
                    }
                }
            }
            this->logMessage("[INFO] Done resending packets and resetting timeouts.");
        }
        if (peerCut) {
            this->logMessage("[INFO] The peer connection is lost.");
        }
    }

    int recv(char** userBuffer) {
        int dataSizeReceived = -1;
        bool doneReceiving = false;
        bool timeOutCondition = false;
        int recvAttempts = 0;
        while (!doneReceiving) {
            char* buffer = (char*)malloc(Packet::maxPacketSize());
            struct sockaddr_in servAddr;
            socklen_t servAddrSize = sizeof(servAddr);
            memset(&(servAddr), 0, servAddrSize);
            int n = recvfrom(this->myNetworkInfo.sockFD, buffer, Packet::maxPacketSize(), 0, (sockaddr*)&(servAddr), &servAddrSize);
            if (n == -1) {
                if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
                    // Timeout
                    this->logMessage("[WARNING] recvfrom() - timed out.");
                    // ++recvAttempts;
                    // this->logMessage("[WARNING] recvfrom() - timed out. Restarting recv.");
                    dataSizeReceived = -1;
                    timeOutCondition = true;
                    doneReceiving = true;
                } else {
                    // Some other error.
                    // deallocate `peerNetworkInfo` and exit
                    this->logMessage("[ERROR] While receiving SYN Packet, non timeout error.");
                    exit(1);
                }
            }
            Packet received_packet = Packet::parsePacket(buffer, false);
            MESSAGE_TYPES messageType = received_packet.getMessageType();
            if (messageType == MESSAGE_TYPES::DATA) {
                this->logMessage("[INFO] Received a DATA packet with seq num: " + to_string(received_packet.getSequenceNum()) + " of size: " + to_string(n) + " bytes.");
                this->receiveBuffer.push_back(make_pair(received_packet.getSequenceNum(), make_pair(received_packet.getApplnLaterDataSize(), received_packet.getApplnLaterDataPtr())));
                --this->receiverReceiveWindow;

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
                    this->logMessage("[ERROR] Sending failed.");
                    exit(1);
                }
                this->logMessage("[INFO] Sent ACCEP Packet in response to the DATA packet.");
            } else if (messageType == MESSAGE_TYPES::FIN) {
                this->logMessage("[INFO] Received a FIN packet with seq num: " + to_string(received_packet.getSequenceNum()) + " of size: " + to_string(n) + " bytes.");

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
                    this->logMessage("[ERROR] Sending failed.");
                    exit(1);
                }
                this->logMessage("[INFO] Sent ACCEP Packet in response to the FIN packet.");
                // Packet the data to send to user
                sort(this->receiveBuffer.begin(), this->receiveBuffer.end(), [](const pair<uint32_t, pair<size_t, char*>>& p1, const pair<uint32_t, pair<size_t, char*>>& p2) {
                    return p1.first < p2.first;
                });
                dataSizeReceived = 0;
                for (auto& Buffer : this->receiveBuffer) {
                    dataSizeReceived += Buffer.second.first;
                }
                // dataSizeReceived += 1;  // For the most irritating char* shit a.k.a null char at the end of the string
                this->allData = (char*)malloc(dataSizeReceived * sizeof(char));
                memset(this->allData, 0, dataSizeReceived * sizeof(char));
                char* allDataCopy = this->allData;
                for (auto& Buffer : this->receiveBuffer) {
                    memcpy(allDataCopy, Buffer.second.second, Buffer.second.first * sizeof(char));
                    allDataCopy += Buffer.second.first;
                }
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
            free(buffer);
        }

        if (!timeOutCondition) {
            if (*userBuffer != nullptr) {
                free(*userBuffer);
                *userBuffer = (char*)malloc(dataSizeReceived * sizeof(char));
            } else {
                *userBuffer = (char*)malloc(dataSizeReceived * sizeof(char));
            }
            memcpy(*userBuffer, this->allData, dataSizeReceived * sizeof(char));
            free(this->allData);
            this->allData = nullptr;
        }
        this->logMessage("[INFO] Data received is : " + to_string(dataSizeReceived));
        return dataSizeReceived;
    }
};