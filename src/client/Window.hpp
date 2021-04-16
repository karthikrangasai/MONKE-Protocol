/**
 * @file Window.hpp
 * @brief 
 */
#include <iostream>
#include <vector>
#include <map>
#include <thread>
#include <mutex>
#include <time.h>

#include "common/types.hpp"
using namespace std;

mutex transmissionLock;

class NewSendWindow {
   private:
    uint32_t sequenceNumber;
    uint32_t indexNumber;
    uint32_t windowSize;
    bool transmissionIsOn;
    uint32_t maxSequenceNumber;
    uint32_t maxIndexNumber;
    uint32_t maxWindowSize;
    map<pair<uint32_t, uint32_t>, pair<time_t, bool>> transmissionWindow;

   public:
    NewSendWindow() {
        this->sequenceNumber = 0;
        this->indexNumber = 0;
        this->transmissionIsOn = true;
        this->maxSequenceNumber = (1 << SEQUENCE_NUMBER_BITS) - 1;
        this->maxIndexNumber = (1 << INDEX_NUMBER_BITS) - 1;
        this->maxWindowSize = (1 << COOL_DIRMA) - 1;
        this->transmissionWindow = map<pair<uint32_t, uint32_t>, pair<time_t, bool>>();
    }

    bool empty() {
        return this->transmissionWindow.empty();
    }

    bool full() {
        return (this->transmissionWindow.size() >= this->maxWindowSize);
    }

    uint32_t maxSequenceNum() {
        return this->maxSequenceNumber;
    }

    uint32_t maxIndexNum() {
        return this->maxIndexNumber;
    }

    uint32_t maxWindowSize() {
        return this->maxWindowSize;
    }

    uint32_t currentFreeWindowSize() {
        uint32_t remainingWindowSize = 0;
        unique_lock<mutex> lock(transmissionLock);
        remainingWindowSize = (this->maxWindowSize - this->transmissionWindow.size());
        transmissionLock.unlock();
        return remainingWindowSize;
    }

    bool exists(pair<uint32_t, uint32_t> key) {
        return (this->transmissionWindow.find(key) != this->transmissionWindow.end()) ? true : false;
    }

    void next() {}  //-> uses Next Packet ???

    pair<uint32_t, uint32_t> getCurrentIndexAndSeqNum() {
        return make_pair(this->indexNumber, this->sequenceNumber);
    }

    //-> Add to window
    void addTo() {
        unique_lock<mutex> lock(transmissionLock);
        if (!(this->full())) {
            this->transmissionWindow[make_pair(this->indexNumber, this->sequenceNumber)] = make_pair(-1, false);
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
            // if (this->sequenceNumber >= this->maxSequenceNumber) {
            //     ++this->indexNumber;
            //     if (this->indexNumber >= this->maxIndexNumber) {
            //         this->indexNumber %= this->maxIndexNumber;
            //     }
            //     this->sequenceNumber %= this->maxSequenceNumber;
            // }
        }
        transmissionLock.unlock();
    }

    void start(pair<uint32_t, uint32_t> key) {
        unique_lock<mutex> lock(transmissionLock);
        this->transmissionWindow[key].first = time(NULL);
        transmissionLock.unlock();
    }

    void restart(pair<uint32_t, uint32_t> key) {
        unique_lock<mutex> lock(transmissionLock);
        this->transmissionWindow[key].first = time(NULL);
        transmissionLock.unlock();
    }

    void stop(pair<uint32_t, uint32_t> key) {
        unique_lock<mutex> lock(transmissionLock);
        if (this->exists(key)) {
            this->transmissionWindow[key].first = -1;
        }
        this->transmissionWindow.erase(key);
        transmissionLock.unlock();
    }

    time_t start_time(pair<uint32_t, uint32_t> key) {
        unique_lock<mutex> lock(transmissionLock);
        return this->transmissionWindow[key].first;
        transmissionLock.unlock();
    }

    bool unacked(pair<uint32_t, uint32_t> key) {
        // bool _unacked = false;
        return ((this->exists(key)) && (this->transmissionWindow[key].second == false));
    }

    void mark_acked(pair<uint32_t, uint32_t> key) {
        unique_lock<mutex> lock(transmissionLock);
        this->transmissionWindow[key].second = true;
        transmissionLock.unlock();
    }

    void stop_transmission() {
        unique_lock<mutex> lock(transmissionLock);
        this->transmissionIsOn = false;
        transmissionLock.unlock();
    }

    bool transmitting() {
        return this->transmissionIsOn;
    }
};

/**
 * @class SendWindow
 * @brief Handles the transmission of the MONKE Packets using the Selective Repeat Protocol
 */
class SendWindow {
   private:
    //! What is this?
    uint32_t expectedAck;
    //! What is this?
    uint32_t nextSequenceNumber;
    //! What is this?
    uint32_t nextPacket;
    //! What is this?
    uint32_t maxSequenceSpace;
    //! What is this?
    uint32_t maxWindowSize;
    //! Ordered Hashmap here for transmission window
    map<pair<uint32_t, uint32_t>, pair<time_t, bool>> transmissionWindow;
    //! What is this?
    bool isPacketTransmission;

   public:
    /**
     * @brief Construct a new Send Window object
     * 
     * @param sequenceNumberBits 
     * @param windowSize 
     */
    SendWindow(uint32_t windowSize = 0) {
        this->expectedAck = 0;
        this->nextSequenceNumber = 0;
        this->nextPacket = 0;
        this->maxSequenceSpace = 1 << SEQUENCE_NUMBER_BITS;
        if (windowSize > (1 << (SEQUENCE_NUMBER_BITS - 1))) {
            cout << "[WARNING] Window size is too big! It will be adjusted automatically" << endl;
            this->maxWindowSize = 1 << (SEQUENCE_NUMBER_BITS - 1);
        } else if (windowSize == 0) {
            this->maxWindowSize = 1 << (SEQUENCE_NUMBER_BITS - 1);
        } else {
            this->maxWindowSize = windowSize;
        }
        this->transmissionWindow = map<pair<uint32_t, uint32_t>, pair<time_t, bool>>();
        this->isPacketTransmission = true;
    }
    ~SendWindow() {}

    /**
     * @brief 
     * 
     * @return uint32_t 
     */
    uint32_t expectedACK() {
        return this->expectedAck;
    }

    /**
     * @brief 
     * 
     * @return uint32_t 
     */
    uint32_t maxSequenceNumber() {
        return this->maxSequenceSpace;
    }

    /**
     * @brief 
     * 
     * @return true 
     * @return false 
     */
    bool empty() {
        return (this->transmissionWindow.size() == 0);
    }

    /**
     * @brief 
     * 
     * @return true 
     * @return false 
     */
    bool full() {
        return (this->transmissionWindow.size() >= this->maxWindowSize);
    }

    /**
     * @brief 
     * 
     * @param key 
     * @return true 
     * @return false 
     */
    bool exist(pair<uint32_t, uint32_t> key) {
        return (this->transmissionWindow.find(key) != this->transmissionWindow.end()) ? true : false;
    }

    /**
     * @brief 
     * 
     * @return uint32_t 
     */
    uint32_t next() {
        return this->nextPacket;
    }

    /**
     * @brief Start the timer for a particular packet.
     * 
     * @param key 
     */
    void start(pair<uint32_t, uint32_t> key) {
        unique_lock<mutex> lock(transmissionLock);
        this->transmissionWindow[key].first = time(NULL);
        transmissionLock.unlock();
    }

    /**
     * @brief Index the current packet into the window.
     * 
     * @param key 
     */
    void consume(pair<uint32_t, uint32_t> key) {
        unique_lock<mutex> lock(transmissionLock);
        this->transmissionWindow[key] = make_pair(-1, false);
        this->nextSequenceNumber += 1;
        if (this->nextSequenceNumber >= this->maxSequenceSpace)
            this->nextSequenceNumber %= this->maxSequenceSpace;
        this->nextPacket += 1;
        transmissionLock.unlock();
    }

    /**
     * @brief Sets the current time after retransmission of packet has been done.
     * 
     * @param key 
     */
    void restart(pair<uint32_t, uint32_t> key) {
        unique_lock<mutex> lock(transmissionLock);
        this->transmissionWindow[key].first = time(NULL);
        transmissionLock.unlock();
    }

    /**
     * @brief When ack'd, stop the timer for the packet.
     * 
     * @param key 
     */
    void stop(pair<uint32_t, uint32_t> key) {
        if (exist(key))
            this->transmissionWindow[key].first = -1;

        vector<pair<uint32_t, uint32_t>> deleteAcks;
        if (key.second == expectedAck) {
            for (auto it = this->transmissionWindow.begin(); it != this->transmissionWindow.end(); it++) {
                if (it->second.first == -1 && it->second.second == true)
                    deleteAcks.push_back(it->first);
                else
                    break;
            }
            for (auto i : deleteAcks) {
                this->transmissionWindow.erase(i);
            }
            if (this->transmissionWindow.size() == 0) {
                this->expectedAck = this->nextSequenceNumber;
            } else {
                this->expectedAck = this->transmissionWindow.begin()->first.second;
            }
        }
    }

    /**
     * @brief Return start time of transmission.
     * 
     * @param key 
     * @return time_t 
     */
    time_t start_time(pair<uint32_t, uint32_t> key) {
        return this->transmissionWindow[key].first;
    }

    /**
     * @brief Check if packet is acked
     * 
     * @param key 
     * @return true 
     * @return false 
     */
    bool unacked(pair<uint32_t, uint32_t> key) {
        if (exist(key) && this->transmissionWindow[key].second == false)
            return true;
        return false;
    }

    /**
     * @brief Mark packet as acked.
     * 
     * @param key 
     */
    void mark_acked(pair<uint32_t, uint32_t> key) {
        unique_lock<mutex> lock(transmissionLock);
        this->transmissionWindow[key].second = true;
        transmissionLock.unlock();
    }

    /**
     * @brief stop transmittin packets
     * 
     */
    void stop_transmission() {
        unique_lock<mutex> lock(transmissionLock);
        this->isPacketTransmission = false;
        transmissionLock.unlock();
    }

    /**
     * @brief tells whehter transmission is on.
     * 
     * @return true 
     * @return false 
     */
    bool transmit() {
        return this->isPacketTransmission;
    }
};