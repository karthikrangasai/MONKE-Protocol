Client -> ( m=  MONKEClient()  )

m.connect() -> 3 way handshake (handle RDT here itself for handshake)
	- Window

m.send() -> Chunk of data : (Break into 512 size packets)
	- Common DS : Window <Index, SEQ> -> <Time, Bool>
		- Thread : Packet Handler ( Multiple Thread: Break into chunks and call thread to monitor each packet)
			- Create Chunk and transmit ( data and create packets)
		
		
		
		
		- Thread : ACK Handler (Waits fot I/O on socket, reads packet, marks ACKs accordingly in window)
			- while (socket is free) (Done)
				get handle and read packet and mark ack (Done)


10 MB

512KB : {Packet, ACK} handlers (threads)
	- Break into 512KB chunks
	- For each chunk all send and store in window

512B -> 


ACK = latest_SEQ + 1
SEQ = Prev SEQ + 1 (We aint caring about bytes)



(Index Num, Seq Num, Ack Num)
32 , 32 = 2^64 ( 2^27 )
  Side |   Packet #  | Type  |  Traid
Client |       1     | DATA  | (0,0,1)  ACK = 0 + 1
Client |       2     | DATA  | (0,1,1)  Is x still 1 ???
Client |       3     | DATA  | (0,2,1)

Server |      1      | ACK   | (0,0,1)  ACK = 0 + 1

Client |       4     | DATA  | (0,3,2)  What is x now ???

Server |      2      | ACK   | (0,1,2)  ACK = 1 + 1


Client Window:
	Index
	Seq
	

	transmission_flag

	empty()
	full()
	maxSequenceNumber()
	maxIndexNum()
	exists()
	next() -> uses Next Packet ???
	consume() -> Add to window
	start()
	stop()
	restart()
	start_time()
	unacked
	mark_acked()
	stop_transmission()
	transmitting()

Receiver Window:
	Window Size
	Max recv window
	Max Seq Num
	Max Index Num
	Look Up store

	next() -> give next data packet in order
	exists()
	store()
	expected() ->

ReceiverPacketHandler
	wait for I/O (select)

	recv packet
	
	parse packet

	if not exists:
		store

	if expected packet: (next packet in order)
		deliver to application -> write to the buffer



r = Receiver()
r.bind() -> socket.bind call
r.listen() -> take a client req, 3 way handshake, start ReceiverPacketHandler
r.send() -> (n = 4) * 512B output = 4 (2KB)

5 -> 0B

int n = recv(my life in hell);
while(n>0){

}


Sender -> Sends data, receives ACKs
Receiver -> Receives data, sends ACKs

MONKE()
	- Sender : connect and send
	- Receiver : bind, listen, and recv
