# PFDAS API Package

* PFDAS_export_h5.exe - Compiled windows binary to export PFI native file format to HDF5. The source code and supporting files are in example_src/pfdas_export_h5.c

* PFDAS_stream_h5.exe - Compiled Windows binary to stream PFDAS data directly to HDF5. The source code and supporting files are in example_src/pfdas_stream_h5.c

* pfdas-api-1.dll - The actual API DLL that both the above PDAS .exe's use.

# HDF5

* The HDF5 format is converting all integer sample data to floating point numbers and writing them as one data set. There are a select few settings that are extracted and saved as metadata in the HDF5 file.

* The files: example_src/h5_peak_extract.py and example_src/h5_plot_4x4_gui.py are both simple example Python programs that read the the HDF5 files created by the .exe examples. Note: the HDF5 reading capability is native to Python and no special code is required aside from importing the module.

# Protocol:

The C API is the prefered method for connecting external programs to the PFDAS system. In theory a user could write there own client side reader to replace the API. This is not recommended but would be possible if required. The PFDAS_fpga_payload.c and PFDAS_packet_types.h contain the bulk of the non-trivial code and C struct types that would be needed.

# Native PFDAS file format (.pfr):

The PFDAS GUI has a file record mode where it redirects the TCP data stream to disk. The reback of a .pfr file is nearly the same as reading from the network the API provides methods for file read back as well.

# Basic overview of PFDAS TCP connections

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
