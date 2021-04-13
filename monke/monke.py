import time
import struct
import pickle
import random
import socket
import builtins
from copy import deepcopy
from threading import Thread, Lock
import queue

from utils import NoSocketCreated, ConnectionNotEstablished, HandshakeFailure, CONNECTION_STATES, MESSAGE_TYPES


class Packet:
    def __init__(self, message_type, receive_window, ack_num, index_num, seq_num, data):
        """
            Message type : 5 bits
            Receive windoe : 27 bits
            Ack : 32 bits
            Index : 32 bits
            seq : 32 bits
            application data : n bits (encode)
            +----------------+----------------+
            | Message Type |  Receive Window  |
            +----------------+----------------+
            |     Acknowledgement Number      |
            +----------------+----------------+
            |            Index Number         |
            +----------------+----------------+
            |         Sequence Number         |
            +----------------+----------------+
            |                                 |
            |      Application Layer Data     |
            |                                 |
            +----------------+----------------+
        """
        assert isinstance(message_type, int) and message_type <= (
            1 << 5) - 1, '[ERROR] Invalid message type'
        self.message_type = message_type

        assert isinstance(receive_window, int) and receive_window <= (
            1 << 27) - 1, '[ERROR] Invalid receive window size'
        self.receive_window = receive_window

        assert isinstance(ack_num, int) and ack_num <= (
            1 << 32) - 1, '[ERROR] Invalid ack_num size'
        self.ack_num = ack_num

        assert isinstance(index_num, int) and index_num <= (
            1 << 32) - 1, '[ERROR] Invalid index_num size'
        self.index_num = index_num

        assert isinstance(seq_num, int) and seq_num <= (
            1 << 32) - 1, '[ERROR] Invalid seq_num size'
        self.seq_num = seq_num

        self.data = data

    def bundle_packet(self):
        """
        Convert the entire packet structure into a byte stream, that will be sent with UDP
        ! -> follow network big endian style
        4I -> 4 unsigned integers
        s -> data. character array
        """
        packet = struct.pack(
            '!4Is',
            (self.message_type << 27) ^ self.receive_window,
            self.ack_num,
            self.index_num,
            self.seq_num,
            self.data
        )

        return packet


class MONKE:
    """
    Discussions:

    Main Thread -> actual packets

    Thread -> check time and retransmit
        keep send time in packet structure
        Every time a new packet is sent, run O(n) search to see un-ackd packets. Throw un-ackd in retransmission queue.


    """

    def __init__(self, interface='localhost', port=42069, backlog=2):
        self.addr = (interface, port)
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

        self.backlog_queue_size = backlog
        self.backlog_queue = queue.Queue(maxsize=self.backlog_queue_size)
        self.shook_hands = False
        self.CONNECTION_STATE = CONNECTION_STATES.NO_CONNECTION
        self.SERVER_STATE = None

        self.SR_window_size = 1024
        self.SR_window = {}  # Map: (index, seq) -> data
        self.un_ackd_buffer = {}  # Map: (index, seq) -> data

        self.seq_num = 0
        self.ack_num = 0
        self.index_num = 0
        self.mutex = Lock()
        self.TIMEOUT = 5

    def _listen(self):
        # Receive SYN, or in our case HELLO
        while True:
            if self.backlog_queue.qsize > self.backlog_queue_size:
                continue
            syn_packet = self.sock.recv(32*4 + 1)
            try:
                syn_packet = struct.unpack('!4Is', syn_packet)
                assert len(
                    syn_packet) == 4, '[ERROR] Handshake failed - packet format not as expected!'
            except:
                continue

            self.CONNECTION_STATE = CONNECTION_STATES.HANDSHAKE
            message_type = syn_packet[0] >> 27
            if message_type != MESSAGE_TYPES.HELLO:
                raise HandshakeFailure(
                    '[ERROR] Handshake failed - unidentified message type received')

            # receive_window = syn_packet[0] & int('00000111111111111111111111111111', 2)

            # Send a SYN_ACK
            self.ack_num = syn_packet[3] + 1
            syn_ack_packet_obj = Packet(MESSAGE_TYPES.HELLO | MESSAGE_TYPES.ACCEP,
                                        self.SR_window_size,
                                        self.ack_num,
                                        self.index_num,
                                        self.seq_num,
                                        b'')
            syn_ack_packet = syn_ack_packet_obj.bundle_packet()
            self.sock.send(syn_ack_packet)

            # Receive an ACK
            ack_packet = self.sock.recv(32*4 + 1)
            ack_packet = struct.unpack('!4Is', ack_packet)
            assert len(
                ack_packet) == 4, '[ERROR] Handshake failed - packet format not as expected!'
            message_type = ack_packet[0] >> 27
            if message_type != MESSAGE_TYPES.ACCEP:
                raise HandshakeFailure(
                    '[ERROR] Handshake failed - unidentified message type received')

            # receive_window = ack_packet[0] & int('00000111111111111111111111111111', 2)
            self.CONNECTION_STATE = CONNECTION_STATES.CONNECTED

            # Throw current socket into the backlog(int: queue size) sized queue so that accept can do its job
            self.backlog_queue.put(self.sock.dup())
            # sockfd = socket(AF_INET, SOCK_STREAM, 0);
            # listen(sockfd, 5)
            # connfd = accept(sockfd, (SA*)&cli, &len);
            """
            accept creates a new file desc in non-listening sate. (In our case, a new socket obj)
            The newly created socket is not in the listening state.
                - listening state means waiting for connection
                - non-listening state means connection has been made

            The accept function can block the caller until a connection is present if no pending connections are present on the queue, and the socket is marked as blocking

            monke.listen()
            new_socket = monke.accept() # Blocking call
            do whatever with new_socket

            monke.listen() # Go to a new thread

            while (new_socket = monke.accept()) is True: # Blocking call
                new Thread(do whatever with new_socket)
            """

    def listen(self):
        listening_thread = Thread(target=self._listen)
        listening_thread.start()

    def accept(self):
        while self.backlog_queue.qsize < 0:
            pass
        return self.backlog_queue.get()

    def connect(self):
        if self.sock:
            self.sock.connect(self.addr)
        else:
            raise NoSocketCreated('[ERROR] Socket connection failed')

        syn_packet_obj = Packet(MESSAGE_TYPES.HELLO,
                                self.SR_window_size,
                                self.ack_num,
                                self.index_num,
                                self.seq_num,
                                b'')
        syn_packet = syn_packet_obj.bundle_packet()
        self.sock.send(syn_packet)
        self.CONNECTION_STATE = CONNECTION_STATES.HANDSHAKE

        # Receiving SYN_ACK, or in our terms, HELLO-ACCEP
        syn_ack_packet = self.sock.recv(32*4 + 1)
        syn_ack_packet = struct.unpack('!4Is', syn_ack_packet)
        assert len(
            syn_packet) == 5, '[ERROR] Handshake failed - packet format not as expected!'

        message_type = syn_ack_packet[0] >> 27
        if message_type != (MESSAGE_TYPES.HELLO | MESSAGE_TYPES.ACCEP):
            raise HandshakeFailure(
                '[ERROR] Handshake failed - unidentified message type received')

        # Sending ACK, or in our case ACCEP
        self.ack_num = syn_ack_packet[3] + 1
        ack_packet_obj = Packet(MESSAGE_TYPES.ACCEP,
                                self.SR_window_size,
                                self.ack_num,
                                self.index_num,
                                self.seq_num,
                                b'')
        ack_packet = ack_packet_obj.bundle_packet()
        self.sock.send(ack_packet)
        self.CONNECTION_STATE = CONNECTION_STATES.CONNECTED

    def bind(self, interface='localhost', port=42069):
        if self.sock:
            self.sock.bind(self.addr)
        else:
            raise NoSocketCreated('[ERROR] Socket binding failed.')

    def _break_into_packets(self, data):
        # This function returns a generator that yields a 520-byte
        # chunk of data in order
        if not isinstance(data, bytes):
            data = bytes(data, 'utf-8')
        broken_data = None
        start_idx = 0
        while start_idx < len(data):
            broken_data = data[start_idx:start_idx+520]
            start_idx += 520
            yield broken_data

    def send(self, data):
        # Send 520 bytes of data every packet
        data_packets = self._break_into_packets(data)
        window = 0
        for index, data_packet in enumerate(data_packets):
            window += 1
            packet_obj = Packet(MESSAGE_TYPES.DATA,
                                self.SR_window_size,
                                self.ack_num,
                                index,
                                self.seq_num,
                                data_packet)
            packet = packet_obj.bundle_packet()
            self.un_ackd_buffer[(index, self.seq_num)] = [packet, None]
            # The None is meant to hold the time of packet transmission
            self.seq_num += len(packet)
            if window == 10:
                # 10 packets in windows
                send_helper = Thread(target=self._send_helper)
                recv_ack_thread = Thread(target=self._recv_acks)
                recv_ack_thread.start()
                send_helper.start()
                send_helper.join()
                recv_ack_thread.join()
                window = 0
        if window > 0:
            send_helper = Thread(target=self._send_helper)
            recv_ack_thread = Thread(target=self._recv_acks)
            recv_ack_thread.start()
            send_helper.start()
            send_helper.join()
            recv_ack_thread.join()
            window = 0

    def _send_helper(self):
        pass

    def _recv_acks(self):
        while self.un_ackd_buffer:
            ack_packet = self.sock.recv(32*4 + 1)
            ack_packet = struct.unpack('!4Is', ack_packet)
            assert len(
                ack_packet) == 5, '[ERROR] Packet error - packet format not as expected!'
            message_type = ack_packet[0] >> 27
            index = ack_packet[2]
            seq_num = ack_packet[3]
            if (index, seq_num) in self.un_ackd_buffer:
                del self.un_ackd_buffer[(index, seq_num)]

    def recv(self):
        pass
