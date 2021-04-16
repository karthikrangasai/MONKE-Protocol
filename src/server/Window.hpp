/**
 * @file Window.hpp
 * @brief 
 * 
 */
#include <iostream>
#include <vector>
#include <map>
#include <thread>
#include <mutex>
#include <time.h>

#include "common/Packet.hpp"
using namespace std;

#define usl uint32_t

mutex receiptLock;

class ReceiveWindow {
   private:
    uint32_t sequenceNumber;
    uint32_t indexNumber;
    uint32_t windowSize;
    uint32_t maxSequenceNumber;
    uint32_t maxIndexNumber;
    uint32_t maxWindowSize;
    bool receiptIsOn;
    pair<uint32_t, uint32_t> expectedPkt;
    map<pair<uint32_t, uint32_t>, Packet> receiptWindow;

   public:
    ReceiveWindow() {
        this->sequenceNumber = 0;
        this->indexNumber = 0;
        this->receiptIsOn = false;
        this->maxSequenceNumber = (1 << SEQUENCE_NUMBER_BITS) - 1;
        this->maxIndexNumber = (1 << INDEX_NUMBER_BITS) - 1;
        this->maxWindowSize = (1 << COOL_DIRMA) - 1;
        this->expectedPkt = make_pair(this->indexNumber, this->sequenceNumber);
        this->receiptWindow = map<pair<uint32_t, uint32_t>, Packet>();
    }

    pair<uint32_t, uint32_t> expectedPacket() {
        return this->expectedPkt;
    }

    void updateExpectedPacket() {
        uint32_t seqNumBoundCheck = this->sequenceNumber + 1;
        if (seqNumBoundCheck == 0) {
            this->sequenceNumber = 0;
            uint32_t indexNumBoundCheck = this->indexNumber + 1;
            if (indexNumBoundCheck == 0) {
                this->indexNumber = 0;
            } else {
                ++this->indexNumber;
            }
        } else {
            ++this->sequenceNumber;
        }
        this->expectedPkt = make_pair(this->indexNumber, this->sequenceNumber);
    }

    uint32_t currentFreeWindowSize() {
        unique_lock<mutex> lock(receiptLock);
        assert(this->maxWindowSize >= this->receiptWindow.size());
        return (this->maxWindowSize - this->receiptWindow.size());
        receiptLock.unlock();
    }

    bool exists(pair<uint32_t, uint32_t> key) {
        unique_lock<mutex> lock(receiptLock);
        return (this->receiptWindow.find(key) != this->receiptWindow.end());
        receiptLock.unlock();
    }

    void store(Packet receivedPacket) {
        unique_lock<mutex> lock(receiptLock);
        // Blindly store
        pair<uint32_t, uint32_t> key(receivedPacket.getIndexNum(), receivedPacket.getSequenceNum());
        this->receiptWindow[key] = receivedPacket;
        receiptLock.unlock();
    }

    bool expected(pair<uint32_t, uint32_t> key) {
        return (key == this->expectedPkt);
    }

    Packet* next() {
        Packet* nextPacket = nullptr;
        if (this->exists(this->expectedPkt)) {
            // copy it, destroy it and return it
            unique_lock<mutex> lock(receiptLock);
            nextPacket = new Packet(this->receiptWindow[this->expectedPkt]);
            this->receiptWindow.erase(this->expectedPacket());
            receiptLock.unlock();
        }

        this->updateExpectedPacket();
        return nextPacket;
    }

    bool receipt() {
        return this->receiptIsOn;
    }

    void start_receipt() {
        this->receiptIsOn = true;
    }
};

/**
 * @class ReceiveWindow
 * @brief Handles the receipt of the MONKE Packets using the Selective Repeat Protocol
 */
// class ReceiveWindow {
//     //! What is this?
//     uint32_t expectedPkt;
//     //! What is this?
//     uint32_t maxSequenceSpace;
//     //! What is this?
//     uint32_t maxWindowSize;
//     //! What is this?
//     uint32_t lastPkt;
//     //! Ordered Hashmap here for recepit window
//     map<pair<uint32_t, uint32_t>, Packet*> receiptWindow;
//     //! What is this?
//     bool isPacketReceipt;

//    public:
//     /**
//     * @brief Construct a new Receive Window object
//     *
//     * @param sequenceNumberBits
//     * @param windowSize
//     */
//     ReceiveWindow(uint32_t sequenceNumberBits, uint32_t windowSize = 0) {
//         expectedPkt = 0;
//         maxSequenceSpace = 2 << sequenceNumberBits;
//         if (windowSize == 0) {
//             maxWindowSize = 2 << (sequenceNumberBits - 1);
//         } else {
//             if (windowSize > (2 << (sequenceNumberBits - 1))) {
//                 cout << "[WARNING] Window size is too big! It will be adjusted automatically" << endl;
//                 maxWindowSize = 2 << (sequenceNumberBits - 1);
//             } else {
//                 maxWindowSize = windowSize;
//             }
//         }
//         lastPkt = maxWindowSize - 1;
//         receiptWindow = map<pair<uint32_t, uint32_t>, Packet*>();
//         isPacketReceipt = false;
//     }

//     /**
//      * @brief
//      *
//      * @return uint32_t
//      */
//     uint32_t expectedPacket() {
//         return this->expectedPkt;
//     }

//     /**
//      * @brief
//      *
//      * @return uint32_t
//      */
//     uint32_t lastPacket() {
//         return this->lastPkt;
//     }

//     /**
//      * @brief
//      *
//      * @param key
//      * @return true
//      * @return false
//      */
//     bool out_of_order(pair<uint32_t, uint32_t> key) {
//         if (this->expectedPacket() > this->lastPacket()) {
//             if (key.second < this->expectedPacket() && key.second > this->lastPacket()) {
//                 return true;
//             }
//         } else {
//             if (key.second < this->expectedPacket() or key.second > this->lastPacket()) {
//                 return true;
//             }
//         }
//         return false;
//     }

//     /**
//      * @brief
//      *
//      * @param key
//      * @return true
//      * @return false
//      */
//     bool exist(pair<uint32_t, uint32_t> key) {
//         return (this->receiptWindow.find(key) != this->receiptWindow.end());
//     }

//     /**
//      * @brief
//      *
//      * @param receivedPacket
//      */
//     void store(Packet receivedPacket) {
//         if (!(this->expected(receivedPacket.getSequenceNum()))) {
//             uint32_t sequenceNumber = this->expectedPkt;
//             pair<uint32_t, uint32_t> key(receivedPacket.getIndexNum(), sequenceNumber);
//             while (sequenceNumber != receivedPacket.getSequenceNum()) {
//                 if (this->receiptWindow.find(key) == this->receiptWindow.end()) {
//                     this->receiptWindow[key] = nullptr;
//                 }
//                 sequenceNumber += 1;
//                 if (sequenceNumber >= this->maxSequenceSpace) {
//                     sequenceNumber %= this->maxSequenceSpace;
//                 }
//             }
//         }

//         this->receiptWindow[make_pair(receivedPacket.getIndexNum(), receivedPacket.getSequenceNum())] = &receivedPacket;
//     }

//     /**
//      * @brief
//      *
//      * @param sequenceNumber
//      * @return true
//      * @return false
//      */
//     bool expected(uint32_t sequenceNumber) {
//         return (sequenceNumber == this->expectedPkt);
//     }

//     /**
//      * @brief
//      *
//      * @return Packet*
//      */
//     Packet* next() {
//         Packet* packet = nullptr;
//         if (this->receiptWindow.size() > 0) {
//             pair<uint32_t, uint32_t> nextPtr_key = receiptWindow.begin()->first;
//             Packet* nextPtr_Packet = receiptWindow.begin()->second;
//             // map<uint32_t, Packet*> nextPkt = receiptWindow.begin();

//             if (nextPtr_Packet != nullptr) {
//                 packet = (Packet*)malloc(sizeof(Packet));
//                 packet = nextPtr_Packet;

//                 this->receiptWindow.erase(nextPtr_key);

//                 this->expectedPkt = nextPtr_key.second + 1;

//                 if (this->expectedPkt >= this->maxSequenceSpace) {
//                     this->expectedPkt %= this->maxSequenceSpace;
//                 }

//                 this->lastPkt = this->expectedPkt + this->maxWindowSize - 1;
//                 if (this->lastPkt >= this->maxSequenceSpace) {
//                     this->lastPkt %= this->maxSequenceSpace;
//                 }
//             }
//         }
//         return packet;
//     }

//     /**
//      * @brief
//      *
//      * @return true
//      * @return false
//      */
//     bool receipt() {
//         return this->isPacketReceipt;
//     }

//     /**
//      * @brief
//      *
//      */
//     void start_receipt() {
//         this->isPacketReceipt = true;
//     }
// };