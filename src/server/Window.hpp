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
using namespace std;

mutex transmissionLock;

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
    map<uint32_t, pair<time_t, bool>> transmissionWindow;
    //! What is this?
    bool isPacketTransmission;

   public:
    /**
     * @brief Construct a new Send Window object
     * 
     * @param sequenceNumberBits 
     * @param windowSize 
     */
    SendWindow(uint32_t sequenceNumberBits, uint32_t windowSize = 0);
    ~SendWindow();

    /**
     * @brief 
     * 
     * @return uint32_t 
     */
    uint32_t expectedACK();

    /**
     * @brief 
     * 
     * @return uint32_t 
     */
    uint32_t maxSequenceNumber();

    /**
     * @brief 
     * 
     * @return true 
     * @return false 
     */
    bool empty();

    /**
     * @brief 
     * 
     * @return true 
     * @return false 
     */
    bool full();

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
     * @return uint32_t 
     */
    uint32_t next();

    /**
     * @brief 
     * 
     * @param key 
     */
    void start(uint32_t key);

    /**
     * @brief 
     * 
     * @param key 
     */
    void consume(uint32_t key);

    /**
     * @brief 
     * 
     * @param key 
     */
    void restart(uint32_t key);

    /**
     * @brief 
     * 
     * @param key 
     */
    void stop(uint32_t key);

    /**
     * @brief 
     * 
     * @param key 
     * @return time_t 
     */
    time_t start_time(uint32_t key);

    /**
     * @brief 
     * 
     * @param key 
     * @return true 
     * @return false 
     */
    bool unacked(uint32_t key);

    /**
     * @brief 
     * 
     * @param key 
     */
    void mark_acked(uint32_t key);

    /**
     * @brief 
     * 
     */
    void stop_transmission();

    /**
     * @brief 
     * 
     * @return true 
     * @return false 
     */
    bool transmit();
};