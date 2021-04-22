# MONKE Protocol
## Protocol
Since the underlying transport protocol is UDP, we have the guarantees of checksum validation and the assurance the data will reach the right destination port. The reliability that we have to provide is that the messages reach the receiver's end without any loss.

To satisfy the requirements of the reliability constraints, we use the Data Segment of the UDP segment structure to our full benefit.

The proposed structure is as follows:
```
|<------------32 bits------------>|
+----------------+----------------+
| MType |  Size  | Receive Window |
+----------------+----------------+
|         Sequence Number         |
+----------------+----------------+
|     Acknowledgement Number      |
+----------------+----------------+
|                                 |
|      Application Layer Data     |
|                                 |
+----------------+----------------+
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
- Application Layer Data Size
- Receive Window (16 bits)
- Acknowledgment Number
- Sequence Number
- Application Layer Data

## Application Programming Interface
- MONKE()
	- connect()
	- listen()
	- send()
	- recv()
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