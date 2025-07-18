# Quick Start

The SDAS hardware has two distinct TCP sockets. The control socket which is a telnet like text based socket and the streaming sockets which is a binary protocol using c structs. All sockets are single connection only. The control socket is on port 28000 and the streaming sockets are at 56000, 56001, 56002 and 56003. The streaming sockets can be interfaced using the C API. The Control socket has no explict programming library.   For purposes of exersizing the C API it is necessary to have basic knowledge of the LCS commands. The following information contains enough basic commands to inject test signals and control the basic features of a channel.

<u>Commands to set all channels to internal 100Hz sine wave:</u>

Mute_tst OFF

gain 1

testmode testbus

test_src sine

test_atten X1

<u>Commands to set all channels to external function generator (after sending above commands):</u>

test_src tbin

<u>Command to change the sample rate:</u>

Sample_Rate_ 100000

<u>Command to save the current state:</u>

state save
