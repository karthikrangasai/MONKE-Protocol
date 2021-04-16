# MONKE Protocol
## Protocol
Since the underlying transport protocol is UDP, we have the guarantees of checksum validation and the assurance the data will reach the right destination port. The reliability that we have to provide is that the messages reach the receiver's end without any loss.

To satisfy the requirements of the reliability constraints, we use the Data Segment of the UDP segment structure to our full benefit.

The proposed structure is as follows:
```
|<------------32 bits------------>|
+----------------+----------------+
| Message Type |  Receive Window  |
+----------------+----------------+
|     Appln Layer Data Size       |
+----------------+----------------+
|            Index Number         |
+----------------+----------------+
|         Sequence Number         |
+----------------+----------------+
|                                 |
|      Application Layer Data     |
|                                 |
+----------------+----------------+

|<------------32 bits------------>|
+----------------+----------------+
| Message Type |  Receive Window  |
+----------------+----------------+
|     Appln Layer Data Size = 0   |
+----------------+----------------+
|            Index Number         |
+----------------+----------------+
|         Sequence Number         |
+----------------+----------------+


Client: Packet = MONKE Packet(DATA, 15, 512, 0, 0, HTTP Req)
Server: Packet = MONKE Packet(ACK, 14, 0, 0, 0) => after this, pampi HTTP req to actual server
Server: Packet = MONKE Packet(DATA, 15, 512, 0, 1, HTTP Response)
Server: Packet = MONKE Packet(DATA, 15, 512, 0, 2, HTTP Response)
Server: Packet = MONKE Packet(DATA, 15, 512, 0, 3, HTTP Response)
Server: Packet = MONKE Packet(DATA, 15, 512, 0, 4, HTTP Response)
Server: Packet = MONKE Packet(DATA, 15, 512, 0, 5, HTTP Response)
Server: Packet = MONKE Packet(DATA, 15, 512, 0, 6, HTTP Response)
Client: Packet = MONKE Packet(ACK, 12, 0, 0, 1) -> ACK for first HTTP Resp Msg => after this, pampi HTTP resp to browser
Server: Packet = MONKE Packet(DATA, 15, 512, 0, 7, HTTP Response)
Server: Packet = MONKE Packet(DATA, 15, 512, 0, 8, HTTP Response)
Client: Packet = MONKE Packet(ACK, 10, 0, 0, 2) -> ACK for second HTTP Resp Msg => after this, pampi HTTP resp to browser
Client: Packet = MONKE Packet(ACK, 10, 0, 0, 3) -> ACK for third HTTP Resp Msg => after this, pampi HTTP resp to browser
Client: Packet = MONKE Packet(ACK, 10, 0, 0, 4) -> ACK for fourth HTTP Resp Msg => after this, pampi HTTP resp to browser








|<------------32 bits------------>|
+----------------+----------------+
| MType |  Size  | Receive Window |
+----------------+----------------+
|         Sequence Number         |
+----------------+----------------+
|                                 |
|      Application Layer Data     |
|                                 |
+----------------+----------------+

- Unused = 3 bits
- MsgType = 5 bits
- Size = 8 bits (Payload size)
- Rec_win = uint16_t (Remaining packets I can accomodate) = (max_window_size - curr_window_size)
- Seq_num = uint32_t


262143 MONKE Packets and wait for acks
From seq num perspctive: (0, 262143)
As soon as one packet (assuming 1th packet) is ACK'd seq_num will inc to 262144 => recv window (1, 262144)
As soon as an another packet (assuming 2th packet) is ACK'd seq_num will inc to 262145 => recv window (2, 262145)
so on.................
When seq_num is 4294705152, I have sent 4294705152 packets succesfully.


SEQ Num = [0, 4294967295] intersection W

At seq_num = 4294967295, new packet will be registered to seq_num=0.

if map[seq_num].exists():
	find next free key
	seq_num = 

Send:
[0 1 2 3 4 5 6 7 8 9.....................................2^32
[_ _ _ _

Recv:
[0 1 2 3 4 5 6 7 8 9...... 2^18] ...........[................... 2^32] 
[- x x x x x x x x ____________]
xxxxxxxxxx[                            ]

[- - - - - - - - - -] - - - - - - - - -
x [- x - - x - - - - -] - - - - - - - -

window_base = 0
window_size = 16

window_end = window_base + window_size - 1

if map[window_base].ack == true:
	map.erase(window_base)
	++window_base

window = map<uint32_t, pair<time_t, bool>>;

0 : <00:00.0, false>
1 : <00:01.0, false>
2 : <00:02.0, false>
3 : <00:03.0, false>

Window  :  x [_ x x  ]
Window_d:     ^     ^
seq_num :     ^

Packet p[4];
        1                             2^32
Data  : 0 - - - - - - - - - - 0 0 0 0 0
SEQNUM: ^
BASE  :                       ^
MAX   :     ^


SendPacketHandler:
	while true:
		window[seq_num] = packet;
		SendSinglePacketHandler(packet); // Send the packet and monitor ack'd-ness for resending
		if seq_num == window_max:
			break;
		++seq_num;

ACKHandler:
	recv(packet);
	_seq_num = packet.seq_num
	if window[packet].exists():
		window.erase(packet);
	
	if(_seq_num == window_base):
		while(window[window_base] == acked):
			++window_base
		
		window_max = (window_base + max_window_size - 1) % max_seq_num

5MB audio file
1MB

file f = open("love_charger.mp3"); // load 5MB
buffer = file.read(1MB)

while(!f.eof()){
	MONKE.send(buffer);
	buffer = file.read(1MB)
}

MONKE {

	void send(){
		make 4000 packets
		O(4000) send them
		loop in all O(4000) to check time and send
	}
}

256B -> 4000 packets for 1 send call

SendSinglePacketHandler:
	Approach 1:
		Make thread for each packet in window
	
	Approach 2:
		One thread that loops through the window, checks time and resends

{  3 }
   _
map[seq_num % window_size] = new Packet();

3
6
0

Hence, at any given point of time, I have sent 262144 packets amounting to 134.217728 MB to the network core.


- If recv_window <= seq_num, then don't require index_num
- If msg_type == DATA, then read 512B more

computer -> proxy vpn -> internet

size = 512*n+32
n DATA packets -> 
32 B packet -> 

spotify 64B

Theoritical: map.size() = 2^27-1

Our: map.size() = 2^4 - 1

[                                                               ]
Client: Packet = MONKE Packet(DATA, 15, 512, 0, 0, HTTP Req)
Server: Packet = MONKE Packet(ACK, 14, 0, 0, 0) => after this, pampi HTTP req to actual server
Server: Packet = MONKE Packet(DATA, 15, 512, 0, 1, HTTP Response)
Server: Packet = MONKE Packet(DATA, 15, 512, 0, 2, HTTP Response)
Server: Packet = MONKE Packet(DATA, 15, 512, 0, 3, HTTP Response)
Server: Packet = MONKE Packet(DATA, 15, 512, 0, 4, HTTP Response)
Server: Packet = MONKE Packet(DATA, 15, 512, 0, 5, HTTP Response)
Server: Packet = MONKE Packet(DATA, 15, 512, 0, 6, HTTP Response)
Client: Packet = MONKE Packet(ACK, 12, 0, 0, 1) -> ACK for first HTTP Resp Msg => after this, pampi HTTP resp to browser
Server: Packet = MONKE Packet(DATA, 15, 512, 0, 7, HTTP Response)
Server: Packet = MONKE Packet(DATA, 15, 512, 0, 8, HTTP Response)
Client: Packet = MONKE Packet(ACK, 10, 0, 0, 2) -> ACK for second HTTP Resp Msg => after this, pampi HTTP resp to browser
Client: Packet = MONKE Packet(ACK, 10, 0, 0, 3) -> ACK for third HTTP Resp Msg => after this, pampi HTTP resp to browser
Client: Packet = MONKE Packet(ACK, 10, 0, 0, 4) -> ACK for fourth HTTP Resp Msg => after this, pampi HTTP resp to browser
```

- Two Possibilities:
	- Four 32bit integers without any appln layer data => Size = 16Bytes
	- Four 32bit integers with appln layer data => Size = 16Bytes + 512B = 528B

### Structure definitions:
- Message Type: (5 bits)
	- Type of message that can be transmitted from the client to the server or vice-versa.
		- HELLO  : 00001
		- DATA   : 00010 = I will send some victory screech.
		- ACCEP  : 00100
		- REJECC : 01000
		- PING   : 10000
- Receive Window (27 bits)
- Index Number
	- A unique number used to uniquely identify data from a given source.
- Acknowledgment Number
- Sequence Number
- Application Layer Data

## Application Programming Interface
- MONKEClient()		: Instantiate the Client object.
	- monke_send()	: Used to send data to other monke.
	- monke_recv()	: Used to receive data from the other monke.
- MONKEServer()		: Instantiate the Server object.
	- monke_send()	: Used to send data to other monke.
	- monke_recv()	: Used to receive data from the other monke.
	- monke_listen(): Used by the server to wait for connections from clients.
	- monke_accept()	: Used to serve a successful connection.

## Quick Run of the Protocol
- 3-way Handshake between client and server:
	- The client sends a ‘HELLO’ message to the server
	- The server responds to the client with a ‘HELLO-ACCEP’ message.
	- The client finally sends an ‘ACCEP’ MESSAGE to the server.
- Exchange of Data:
	- The sender monke will break the incoming application layer data into manageable chunks and send them to the receiver monke with the appropriate Index number and Sequence Numbers with the ‘DATA’ message.
	- For every ‘DATA’ type message received, the receiver monke must send an ‘ACCEP’ message to the sender monke.
	- In the case that the client and server don’t have any data getting exchanged and if the client wishes to keep the connection with the server alive, it has to send ‘PING’ messages to realize the same.
	- If there is no data exchange or exchange of ‘PING’ messages between both the client and server for a ‘time_out’ period of time, the server will initiate connection close protocol.
- Destroy Connection:
	- A monke sends a ‘REJECC’ message to the other monke.
	- The receiver monke sends an ‘ACCEP’ message to the sender monke.
	- The sender monke sends an ‘ACCEP’ message to the receiver monke and closes its connection.
	- The receiver monke receives the ‘ACCEP’ message and closes its connection.

## Consistency
- Error Conditions:
	- Packet Corruption: Packet corruption is not a major concern for our protocol since the underlying UDP protocol handles this issue.
	- Packet Loss or Packet Delay: However, packet loss is still a concern. This is handled by having the sender of data wait for an ‘ACCEP’ message, but not indefinitely; instead, the sender waits for a reasonable amount of time. If no ‘ACCEP’ is received, the sender retransmits the packet.
	- Packet Duplication or Packet Reorder: Duplicate and out-of-order packets will be handled by the receiver, which requires that the receiver specify the packet number for which the ‘ACCEP’ is being sent.
- Selective Repeat Protocol

## Assumptions:
- The application does basic file transfer from the client to the server. (One way)
- The metadata and the actual file are sent separately to the server to confirm that the file is transferable using the application protocol.

## Notes
- Index number is same for all chunks belonging to a single send call from the application
- Sequence number (SEQ in TCP) varies for all MONKE packets irrespective of who calls sens at which layer.
- Same index number
	- 0100 0100 1001 0000 0000 1010 1010 1011 1111 1111

## Application
### Assumptions
- Both Client and Server can upload and download files.
- Headers and Meta are always sent sepatately from the actual file.
	- Upon recieving the headers/meta and after required processing only the receiver shall signal the sender to send the file.
- User auth can be added (optional).

### Protocol
```
Types of Messages (Assume 8 types)
- HEADER_BEGIN :00000001
- HEADER_END   :00000010
- HEADER       :00000100
- FILE_ACCEPT  :00001000
- FILE_REJECT  :00010000
- FILE_BEGIN   :00100000
- FILE_END     :01000000
- FILE         :10000000
```

```
Sender Packet Structure
Line 1: 8 bit Header
Line 2: Data
```

```
Reciever Packet Structure
Line 1: FILE_ACCEPT / FILE_REJECT
```

### Order of working (HTTP type)
- Send HEADER_BEGIN
- Send HEADER
- Send HEADER_END
- Wait for FILE_ACCEPT / FILE_REJECT
- Send FILE_BEGIN
- Spam with FILE
- Send FILE_END



```

class MONKE {
	// window, socket, handler everything
	receiver_details;
	sender_details;

	bool senderBusy, receiverBusy;

	enum type;

	connect() {
		type= client;
		start 3 way handshake
		check 5 timeouts and cut

		if(success){
			instantiate Sender Window;
			instantiate Receiver Window;
			instantiate Handlers;
		}
	}

	bind() {
		type = server;
	}

	listen(){
		if (type != server){
			cut
		}
		wait for a packet (SYN Packet)
		start 3 way handshake

		if(success){
			instantiate Sender Window;
			instantiate Receiver Window;
			instantiate Handlers;
		}
	}

	bool pinger(){
		while true:
			while !senderBusy:
				keep sending ping packets
	}

	send(data) {
		senderBusy = true;
		senderhandler.send(data); // wont finish until all acks are received
		senderBusy = false;

		// send window_size packets

		// recv all acks possible

		// check for window variable update

		loopback

		ACK ?????
	}

	char* recv() {
		receiverhandler.recv(buffer);

		// recv each packet and send ack

		// where am i sending the data to the user ????

		C++.recv(sockFD, buffer, sizeof(buffer));

		// read until finish

		Packet p = convertToPacket(buffer);
		C++.send(sockFD, Packet(ACK, p.seq_num), sizeof(packet));

		buffer.write(packet.applicationLayerData);
		return &buffer;
	}
}

MONKE.connect(); 

MONKE.send(data);


MONKE.recv(buffer);

class SenderHandler {};
class SenderWindow {}; // circular buffer type implementation

class ReceiverHandler{};
class ReceiverWindow {}; // map type buffer

recv() {

}
0 1 2 3 4 5 6 7

0 1 

0 1 2 3 4 5 6 7 

send DATA by sender:
	if no ACK, he will keep sending DATA

not DATA


file f = open("love_charger.mp3"); // load 5MB
buffer = file.read(1MB)

while(!f.eof()){
	MONKE.send(buffer); // n*DATA + FINISH
	buffer = file.read(1MB)
}


while(recv(buffer) > 0){ // n*DATA - n*ACK + FINISH
	f.write(buffer);
}
```