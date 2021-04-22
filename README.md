# MONKE-Protocol

# Team Members
- Varun Parthasarathy - 2017B3A70515H
- Siddarth Gopalakrishnan - 2017B3A71379H
- Sivaraman Karthik Rangasai - 2017B4A71499H
- Rohan Maheshwari - 2017B4A70965H
- Mythreyi S S - 2017B4A70969H

## Requirements:
Reliable Data Transfer in Application Layer using Selective Repeat Automatic Repeat ReQuest protocol using UDP sockets.

## Why MONKE Protocol?
We name this protocol the MONKE protocol as a reference to one of the currently trending jokes circulating on social media, which features the tagline “REJECT HUMANITY, RETURN TO MONKE”. The name is meant to be slightly ironic, as the circulation of such jokes (which suggest returning to a more primitive lifestyle) is heavily dependent on the underlying networks (and thus modern technology) that connect these users.

The name is also a homage to RFC 2795 - The Infinite Monkey Protocol Suite (IMPS) - which was published as part of the Internet Engineering Task Force’s yearly tradition of creating an RFC as a joke every April Fools’ Day.

## Protocol
Check the file [RFC.md](./rfc/RFC.md)

# How to run
- Directory structure.
```
.
├── client
│   └── client.cpp
├── README.md
├── rfc
│   └── RFC.md
├── server
│   └── server.cpp
└── src
    └── MONKE.hpp

```
- Running the code:
	- Copy some file into the `client` directory.
	- Change the filename variable to the name of the file that needs to be transferred.
	- In terminal 1
		```
		cd /client/
		g++ -o client.o client.cpp
		```
	
	- In terminal 2
		```
		cd /server/
		g++ -o server.o server.cpp
		```
	- First run `./server.o` and then run `./client.o`.
	- Voila, file tranferring happens.


# Changes to the Protocol
- Removed Index number field as it did not contribute much.
- Added the application layer data size field to specify the same.
- Reduced received window size to 16 bits.
- This protocol can theoritically send 1.13TB of data before exhausting all the sequence numbers

# File Details
The file that was used to test the protocol for working.
```
$ exiftool Demo.mp4 
ExifTool Version Number         : 11.88
File Name                       : Demo.mp4
File Size                       : 296 MB
File Modification Date/Time     : 2020:11:20 21:43:49+05:30
File Access Date/Time           : 2021:04:19 19:14:55+05:30
File Inode Change Date/Time     : 2021:04:19 19:14:55+05:30
File Permissions                : rw-rw-r--
File Type                       : MP4
File Type Extension             : mp4
MIME Type                       : video/mp4
Major Brand                     : MP4  Base Media v1 [IS0 14496-12:2003]
```

The file that was used to test the protocol on all fronts and plot them against the throughput.
```
$ exiftool assignment_2.pdf 
ExifTool Version Number         : 11.88
File Name                       : assignment_2.pdf
Directory                       : .
File Size                       : 61 kB
File Modification Date/Time     : 2021:03:15 15:00:44+05:30
File Access Date/Time           : 2021:04:19 22:01:45+05:30
File Inode Change Date/Time     : 2021:04:19 22:01:54+05:30
File Permissions                : rw-rw-r--
File Type                       : PDF
File Type Extension             : pdf
MIME Type                       : application/pdf
PDF Version                     : 1.4
```