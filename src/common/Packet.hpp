/**
 * @file Packet.hpp
 * @brief 
 */
#include <iostream>
#include <vector>
#include <map>
#include <thread>
#include <mutex>
#include <time.h>
#include <cstring>
#include <cassert>

#include "types.hpp"
using namespace std;

/**
 * @class Packet
 * @brief Class definition for the Packet structure of MONKE Protocol.
 * 
 * @details
 * |<------------32 bits------------>|
 * +----------------+----------------+
 * | Message Type |  Receive Window  |
 * +----------------+----------------+
 * |     Acknowledgement Number      |
 * +----------------+----------------+
 * |            Index Number         |
 * +----------------+----------------+
 * |         Sequence Number         |
 * +----------------+----------------+
 * |                                 |
 * |      Application Layer Data     |
 * |                                 |
 * +----------------+----------------+
 * 
 * Message Type: This section is of size 5 bits
 * Receive Window: This section is of size 27 bits
 */
class Packet {
   private:
    uint32_t msgType_RecvWindow;
    uint32_t acknowledgementNum;
    uint32_t indexNum;
    uint32_t sequenceNum;
    char applnLayerData[512];  // = nullptr;  // 560

   public:
    /**
    * @brief Construct a new Packet object
    * 
    * @param msg_type 
    * @param recv_window 
    * @param acknowledgementNum 
    * @param indexNum 
    * @param sequenceNum 
    * @param applnLayerData 
    */
    Packet(uint32_t msg_type, uint32_t recv_window, uint32_t acknowledgementNum, uint32_t indexNum, uint32_t sequenceNum, char applnLayerData[]) {
        if (msg_type != MESSAGE_TYPES::DATA) {
            this->msgType_RecvWindow = msg_type >> RECEIVE_WINDOW_BITS;
            if (recv_window > (1 << RECEIVE_WINDOW_BITS) - 1)
                recv_window = 0 ^ ((1 << RECEIVE_WINDOW_BITS) - 1);
            this->msgType_RecvWindow = this->msgType_RecvWindow ^ recv_window;
            this->acknowledgementNum = acknowledgementNum;
            this->indexNum = indexNum;
            this->sequenceNum = sequenceNum;
            memset(this->applnLayerData, 0, sizeof(this->applnLayerData));
        } else {
            assert(sizeof(applnLayerData) <= MAXIMUM_PAYLOAD);
            this->msgType_RecvWindow = msg_type >> RECEIVE_WINDOW_BITS;
            if (recv_window > (1 << RECEIVE_WINDOW_BITS) - 1)
                recv_window = 0 ^ ((1 << RECEIVE_WINDOW_BITS) - 1);
            this->msgType_RecvWindow = this->msgType_RecvWindow ^ recv_window;
            this->acknowledgementNum = acknowledgementNum;
            this->indexNum = indexNum;
            this->sequenceNum = sequenceNum;
            memset(this->applnLayerData, 0, sizeof(this->applnLayerData));
            // this->applnLayerData = (char *)malloc(sizeof(applnLayerData));
            memcpy(this->applnLayerData, applnLayerData, sizeof(applnLayerData));
        }
    }

    ~Packet() {}

    uint32_t getSequenceNum() {
        return this->sequenceNum;
    }

    uint32_t getIndexNum() {
        return this->indexNum;
    }

    uint32_t getMessageType() {
        return (this->msgType_RecvWindow >> RECEIVE_WINDOW_BITS);
    }

    static Packet parsePacket(char* data) {
        uint32_t values[4];
        char _applnLayerData[512];  // = nullptr;  // 560

        char* data_copy = data;

        char num[4];
        for (int i = 0; i < 4; ++i) {
            memcpy(num, data_copy, 4 * sizeof(char));
            values[i] = (uint32_t)atoi(num);
            data_copy = data_copy + 4 * sizeof(char);
        }
        memcpy(_applnLayerData, data_copy, sizeof(data_copy));
        uint32_t msgType = values[0] >> RECEIVE_WINDOW_BITS;
        uint32_t recvWindow = values[0] & ((1 << RECEIVE_WINDOW_BITS) - 1);
        return Packet(msgType, recvWindow, values[1], values[2], values[3], _applnLayerData);
    }
};
