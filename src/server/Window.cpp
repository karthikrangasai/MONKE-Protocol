/**
 * @file Window.cpp
 * @brief 
 * 
 */
#include "Window.hpp"

SendWindow::SendWindow(uint32_t sequenceNumberBits, uint32_t windowSize = 0) {
    this->expectedAck = 0;
    this->nextSequenceNumber = 0;
    this->nextPacket = 0;
    this->maxSequenceSpace = 2 << sequenceNumberBits;
    if (windowSize > (2 << (sequenceNumberBits - 1))) {
        cout << "[WARNING] Window size is too big! It will be adjusted automatically" << endl;
        this->maxWindowSize = 2 << (sequenceNumberBits - 1);
    } else if (windowSize == 0) {
        this->maxWindowSize = 2 << (sequenceNumberBits - 1);
    } else {
        this->maxWindowSize = windowSize;
    }
    this->transmissionWindow = map<uint32_t, pair<time_t, bool>>();
    this->isPacketTransmission = true;
}

SendWindow::~SendWindow() {
}

uint32_t SendWindow::expectedACK() {
    return this->expectedAck;
}

uint32_t SendWindow::maxSequenceNumber() {
    return this->maxSequenceSpace;
}

bool SendWindow::empty() {
    return (this->transmissionWindow.size() == 0);
}

bool SendWindow::full() {
    return (this->transmissionWindow.size() >= this->maxWindowSize);
}

bool SendWindow::exist(uint32_t key) {
    return (this->transmissionWindow.find(key) != this->transmissionWindow.end()) ? true : false;
}

uint32_t SendWindow::next() {
    return this->nextPacket;
}

void SendWindow::start(uint32_t key) {
    unique_lock<mutex> lock(transmissionLock);
    this->transmissionWindow[key].first = time(NULL);
    transmissionLock.unlock();
}

void SendWindow::consume(uint32_t key) {
    unique_lock<mutex> lock(transmissionLock);
    this->transmissionWindow[key] = make_pair(-1, false);
    this->nextSequenceNumber += 1;
    if (this->nextSequenceNumber >= this->maxSequenceSpace)
        this->nextSequenceNumber %= this->maxSequenceSpace;
    this->nextPacket += 1;
    transmissionLock.unlock();
}

void SendWindow::restart(uint32_t key) {
    unique_lock<mutex> lock(transmissionLock);
    this->transmissionWindow[key].first = time(NULL);
    transmissionLock.unlock();
}

void SendWindow::stop(uint32_t key) {
    if (exist(key))
        this->transmissionWindow[key].first = -1;

    vector<uint32_t> deleteAcks;
    if (key == expectedAck) {
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
            this->expectedAck = this->transmissionWindow.begin()->first;
        }
    }
}

time_t SendWindow::start_time(uint32_t key) {
    return this->transmissionWindow[key].first;
}

bool SendWindow::unacked(uint32_t key) {
    if (exist(key) && this->transmissionWindow[key].second == false)
        return true;
    return false;
}

void SendWindow::mark_acked(uint32_t key) {
    unique_lock<mutex> lock(transmissionLock);
    this->transmissionWindow[key].second = true;
    transmissionLock.unlock();
}

void SendWindow::stop_transmission() {
    unique_lock<mutex> lock(transmissionLock);
    this->isPacketTransmission = false;
    transmissionLock.unlock();
}

bool SendWindow::transmit() {
    return this->isPacketTransmission;
}