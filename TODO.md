- [ ] Packet Drops (For 3-way handshake)
- [ ] Test Connect 1 client and 2 clients

- [ ] RDT 3.0 based send and receive




- [x] Make the `parsePacket` have different sized arrays for each of the for variables.
- [x] check that `recvfrom()` error as well.



- [ ] After SEQ_NUM = 126/128, it blows up. Don't know reason.
- Wireshark filter: `(udp.srcport == 42069 or udp.dstport == 42070) or (udp.srcport == 42070 or udp.dstport == 42069)`