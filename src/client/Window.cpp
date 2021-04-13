/**
 * @file Window.cpp
 * @brief 
 * 
 */

#include "Window.hpp"

ReceiveWindow::ReceiveWindow(uint32_t sequenceNumberBits, uint32_t windowSize = 0) {
    expectedPkt = 0;
    maxSequenceSpace = 2 << sequenceNumberBits;
    if (windowSize == 0) {
        maxWindowSize = 2 << (sequenceNumberBits - 1);
    } else {
        if (windowSize > (2 << (sequenceNumberBits - 1))) {
            cout << "[WARNING] Window size is too big! It will be adjusted automatically" << endl;
            maxWindowSize = 2 << (sequenceNumberBits - 1);
        } else {
            maxWindowSize = windowSize;
        }
    }
    lastPkt = maxWindowSize - 1;
    receiptWindow = map<uint32_t, Packet*>();
    isPacketReceipt = false;
}

uint32_t ReceiveWindow::expectedPacket() {
    return this->expectedPkt;
}

uint32_t ReceiveWindow::lastPacket() {
    return this->lastPkt;
}

bool ReceiveWindow::out_of_order(uint32_t key) {
    if (this->expectedPacket() > this->lastPacket()) {
        if (key < this->expectedPacket() && key > this->lastPacket()) {
            return true;
        }
    } else {
        if (key < this->expectedPacket() or key > this->lastPacket()) {
            return true;
        }
    }
    return false;
}

bool ReceiveWindow::exist(uint32_t key) {
    return (this->receiptWindow.find(key) != this->receiptWindow.end());
}

void ReceiveWindow::store(Packet receivedPacket) {
    if (!(this->expected(receivedPacket.getSequenceNum()))) {
        uint32_t sequenceNumber = this->expectedPkt;

        while (sequenceNumber != receivedPacket.getSequenceNum()) {
            if (this->receiptWindow.find(sequenceNumber) == this->receiptWindow.end()) {
                this->receiptWindow[sequenceNumber] = nullptr;
            }
            sequenceNumber += 1;
            if (sequenceNumber >= this->maxSequenceSpace) {
                sequenceNumber %= this->maxSequenceSpace;
            }
        }
    }

    this->receiptWindow[receivedPacket.getSequenceNum()] = &receivedPacket;
}

bool ReceiveWindow::expected(uint32_t sequenceNumber) {
    return (sequenceNumber == this->expectedPkt);
}

Packet* ReceiveWindow::next() {
    Packet* packet = nullptr;
    if (this->receiptWindow.size() > 0) {
        uint32_t nextPtr_key = receiptWindow.begin()->first;
        Packet* nextPtr_Packet = receiptWindow.begin()->second;
        // map<uint32_t, Packet*> nextPkt = receiptWindow.begin();

        if (nextPtr_Packet != nullptr) {
            packet = (Packet*)malloc(sizeof(Packet));
            packet = nextPtr_Packet;

            this->receiptWindow.erase(nextPtr_key);

            this->expectedPkt = nextPtr_key + 1;

            if (this->expectedPkt >= this->maxSequenceSpace) {
                this->expectedPkt %= this->maxSequenceSpace;
            }

            this->lastPkt = this->expectedPkt + this->maxWindowSize - 1;
            if (this->lastPkt >= this->maxSequenceSpace) {
                this->lastPkt %= this->maxSequenceSpace;
            }
        }
    }
    return packet;
}

bool ReceiveWindow::receipt() {
    return this->isPacketReceipt;
}

void ReceiveWindow::start_receipt() {
    this->isPacketReceipt = true;
}