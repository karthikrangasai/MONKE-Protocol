/**
 * @file Packet.cpp
 * @brief 
 * 
 */
#include "Packet.hpp"

Packet::Packet(uint32_t msg_type, uint32_t recv_window, uint32_t acknowledgementNum, uint32_t indexNum, uint32_t sequenceNum, char applnLayerData[]) {
    this->msgType_RecvWindow = 0 ^ (msg_type << 27);
    if (recv_window > (2 << 27) - 1)
        recv_window = 0 ^ ((2 << 27) - 1);
    this->msgType_RecvWindow = this->msgType_RecvWindow ^ recv_window;
    this->acknowledgementNum = acknowledgementNum;
    this->indexNum = indexNum;
    this->sequenceNum = sequenceNum;
    this->applnLayerData = (char *)malloc(sizeof(applnLayerData));
    memcpy(this->applnLayerData, applnLayerData, sizeof(applnLayerData));
}

Packet::~Packet() {
}

uint32_t Packet::getSequenceNum() {
    return this->sequenceNum;
}