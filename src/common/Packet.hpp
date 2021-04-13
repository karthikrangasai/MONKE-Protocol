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
using namespace std;

#define usl uint32_t
// Note: The above was originally meant to define unsigned long, however
//       we want strictly 32 bits to be allocated.
/* 
	"Then shalt thou count to 32, no more, no less. 32 shall be the
	number thou shalt count, and the number of the counting shall be 32.
	33 shalt thou not count, neither count thou 31, excepting that thou 
	then proceed to 32. 34 is right out. Once the number 32, being the 
	32nd number, be reached, then loadest thou thy values, which being 
	32 bits, shall serve you well "
*/

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
    char *applnLayerData = nullptr;

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
    Packet(uint32_t msg_type, uint32_t recv_window, uint32_t acknowledgementNum, uint32_t indexNum, uint32_t sequenceNum, char applnLayerData[]);

    ~Packet();

    uint32_t getSequenceNum();
};
