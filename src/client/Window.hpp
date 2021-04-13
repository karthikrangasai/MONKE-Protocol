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

/**
 * @class ReceiveWindow
 * @brief Handles the receipt of the MONKE Packets using the Selective Repeat Protocol
 */
class ReceiveWindow {
    //! What is this?
    uint32_t expectedPkt;
    //! What is this?
    uint32_t maxSequenceSpace;
    //! What is this?
    uint32_t maxWindowSize;
    //! What is this?
    uint32_t lastPkt;
    //! Ordered Hashmap here for recepit window
    map<uint32_t, Packet*> receiptWindow;
    //! What is this?
    bool isPacketReceipt;

   public:
    /**
    * @brief Construct a new Receive Window object
    * 
    * @param sequenceNumberBits 
    * @param windowSize 
    */
    ReceiveWindow(uint32_t sequenceNumberBits, uint32_t windowSize = 0);

    /**
     * @brief 
     * 
     * @return uint32_t 
     */
    uint32_t expectedPacket();

    /**
     * @brief 
     * 
     * @return uint32_t 
     */
    uint32_t lastPacket();

    /**
     * @brief 
     * 
     * @param key 
     * @return true 
     * @return false 
     */
    bool out_of_order(uint32_t key);

    /**
     * @brief 
     * 
     * @param key 
     * @return true 
     * @return false 
     */
    bool exist(uint32_t key);

    /**
     * @brief 
     * 
     * @param receivedPacket 
     */
    void store(Packet receivedPacket);

    /**
     * @brief 
     * 
     * @param sequenceNumber 
     * @return true 
     * @return false 
     */
    bool expected(uint32_t sequenceNumber);

    /**
     * @brief 
     * 
     * @return Packet* 
     */
    Packet* next();

    /**
     * @brief 
     * 
     * @return true 
     * @return false 
     */
    bool receipt();

    /**
     * @brief 
     * 
     */
    void start_receipt();
};