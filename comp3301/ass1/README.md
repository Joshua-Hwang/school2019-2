# The objective
The purpose of this assignment is to create a program called greu.
This program is designed to **listen** on filedescriptors for changes.
We then,
1. package the message with a GRE header
2. send the packaged message through a UDP socket to our `server`

Not only must it do this but also return messages
1. Listen for messages from the server
2. we strip the gre packet and look for a key (if any)
3. We pass it to the appropriate tunnel or tap
 * This is dependent on protocol type and key

```
The GRE packet header has the form:

0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|C| |K|S| Reserved0       | Ver |         Protocol Type         |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                         Key (optional)                        |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

```

Key | Definition
---|---
C | is the checksum flag
K | if a key exists
S | Sequence number which will be 0
Reserved0 | should be all 0s
Ver | in our case should be version 0
Protocol | type will either be IPv4(0x0800) IPv6(0x86dd) or Ethernet(0x6558)
key | will be a 32 bit integer

# GRE header
We're going to handle the GRE header by doing a bunch of memcopy and strcpy
style ideas. This is preferred to the struct idea because of annoying
alignment issues and whatnot. Doing this also requires thinking about the
endianess of the system. Apparently it needs to be swapped.

# DON'T FORGET
Please ensure the the options -4 and -6 are implemented
