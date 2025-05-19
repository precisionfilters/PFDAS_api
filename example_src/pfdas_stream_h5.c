#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include "PFDAS_api.h"
#include "PFDAS_fpga_payload.h"
#include "include/hdf5.h"
#include "pfdas_hdf5.h"
char buf[65536] = {0};        // Temporary stoarge for a string.
int32_t samples[65536] = {0}; // Temporary storage for sample data.
uint32_t sec = 0;             // Temporary storage for seconds.
uint32_t nsec = 0;            // Temporary storage for nano seconds.

int main(int argc, char *argv[]) {
    // Check the number of arguments
    if (argc < 2) {
        printf("Usage: %s <connection_string> [h5_filename] [num_samples]\n", argv[0]);
        return 1;
    }
    // Connection string (required)
    char *connection_string = argv[1];
    char h5_filename[256];
    if (argc > 2) {
        strncpy_s(h5_filename, 256, argv[2], 252); // Leave room for.h5 extension
        strncat_s(h5_filename, 256, ".h5", 4); // Add.h5 extension
    } else {
        strncpy_s(h5_filename, 256, "data.h5", 8);
    }
    // Number of samples (optional, default to 65536*128)
    int num_samples = (argc > 3)? atoi(argv[3]) : 65536*128;

    // Print out API information.
    printf("build_info: %s", PFDAS_api_build_info() );
    printf("api version: %s", PFDAS_api_version() );
    //
    PFDAS_data_handle dhandle = PFDAS_handle_stream_tcp_create(); // Allocate 1 client handle.
    PFDAS_connect(connection_string, dhandle);  // Connect to ip at port.
    PFDAS_start(dhandle);                          // Start the client thread.
    pfds_hdf5_nonblocking_write_packet(1);
    if(pfdas_hdf5_open(h5_filename, 16, num_samples)){
        printf("%s file ready\n", h5_filename);
    }else{
        printf("%s file not ready\n", h5_filename);
        exit(0);
    }
    //
    int writing = 1;
    int have_eos = 0;
    while(have_eos == 0 && PFDAS_running(dhandle)){ // Try several times to be sure we read out all the header info.
        while(PFDAS_meta_has_pkt(dhandle)){ // While we have metadata.
            const struct PFDAS_base_packet_t* p = PFDAS_meta_get_pkt(dhandle); // Get the metadata packet.
            if(p->payload_type == PFDAS_PAYLOAD_LCS_CMD){ // Is it a LCS command?
                const struct PFDAS_packet_payload_lcs_cmd_t* pl =  PFDAS_meta_lcs_cmd_payload(p); // Extract payload which is a simple struct.
                char tmp[512] = {0};
                memcpy(tmp, pl+1, pl->text_size);
                //printf("lcs cmd: %s", tmp);
                fflush(stdout);
                if(strstr(tmp, "state eos")){
                    have_eos = 1;
                    printf("->>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> have eos!\n");
                }
                if(strstr(tmp, "Sample_Rate_")){
                    char* cmd = 0;
                    char* value = 0;
                    char* ch = 0;
                    PFDAS_lcs_parse(tmp, &cmd, &value, &ch);
                    if(cmd &&  value && ch){
                        double sr = atof(value);
                        pfds_hdf5_write_attr_sr(sr);
                    }
                }
                if(strstr(tmp, "Channel_Name_")){
                    char* cmd = 0;
                    char* value = 0;
                    char* ch = 0;
                    PFDAS_lcs_parse(tmp, &cmd, &value, &ch);
                    if(cmd &&  value && ch){
                        printf("cmd=%s value=%s ch=%s\n", cmd, value, ch);
                        pfdas_hdf5_set_channel_name(atoi(ch), value);
                    }
                }
                if(strstr(tmp, "MU_Name_")){
                    char* cmd = 0;
                    char* value = 0;
                    char* ch = 0;
                    PFDAS_lcs_parse(tmp, &cmd, &value, &ch);
                    if(cmd &&  value && ch){
                        printf("cmd=%s value=%s ch=%s\n", cmd, value, ch);
                        pfdas_hdf5_set_channel_mu_name(atoi(ch), value);
                    }
                }
                if(strstr(tmp, "MU_Per_Volt_")){
                    char* cmd = 0;
                    char* value = 0;
                    char* ch = 0;
                    PFDAS_lcs_parse(tmp, &cmd, &value, &ch);
                    if(cmd &&  value && ch){
                        printf("cmd=%s value=%s ch=%s\n", cmd, value, ch);
                        pfdas_hdf5_set_channel_mu_scalar(atoi(ch), atof(value));
                    }
                }


            }
        }
        Sleep(100);
    }
    pfds_hdf5_write_channel_names_attr();
    pfds_hdf5_write_channel_mu_attr();
    PFDAS_data_continue(dhandle); // Instruct reader to continue reading, the metadata has all been read out.
    while(PFDAS_data_has_pkt(dhandle)){ // Read out the buffer to be sure we start with it not full!
        const struct PFDAS_base_packet_t* p = PFDAS_data_get_pkt(dhandle); // Get the data packet.
        PFDAS_data_return_pkt(p, dhandle); // Important!, must return packet after it is done being used.
    }
    int did_something = 0;
    while(PFDAS_running(dhandle) && writing){ // While the connection is active.
        did_something = 0;
        while(PFDAS_data_has_pkt(dhandle)){ // While we have a data packet.
            const struct PFDAS_base_packet_t* p = PFDAS_data_get_pkt(dhandle); // Get the data packet.
            if(pfdas_hdf5_full()){
                pfdas_hdf5_close();
                writing = 0;
            }else{
                pfdas_hdf5_write_packet(p);
            }
            PFDAS_data_return_pkt(p, dhandle); // Important!, must return packet after it is done being used.
            did_something = 1;
        }
        while(PFDAS_meta_has_pkt(dhandle)){ // While we have metadata.
            const struct PFDAS_base_packet_t* p = PFDAS_meta_get_pkt(dhandle); // Get the metadata packet.
            if(p->payload_type == PFDAS_PAYLOAD_LCS_CMD){ // Is it a LCS command?
                const struct PFDAS_packet_payload_lcs_cmd_t* pl =  PFDAS_meta_lcs_cmd_payload(p); // Extract payload which is a simple struct.
                char tmp[512] = {0};
                memcpy(tmp, pl+1, pl->text_size);
                //printf("got lcs cmd %s\n", tmp);
            }
            if(p->payload_type == PFDAS_PAYLOAD_PTP4L_STATS){ // Is it a PTP4l stat?
                const struct PFDAS_packet_payload_ptp4l_t* pl =  PFDAS_meta_ptp4l_payload(p); // Extract payload which is a simple struct.
              //  printf("got ptp4 offset=%d\n", pl->offset);
            }
            if(p->payload_type == PFDAS_PAYLOAD_PHC2SYS_STATS){ // Is it a PHC2SYS stat?
                const struct PFDAS_packet_payload_phc2sys_t* pl =  PFDAS_meta_phy2sys_payload(p); // Extract payload which is a simple struct.
             //   printf("got phc2sys offset=%d\n", pl->offset);
            }
            if(p->payload_type == PFDAS_PAYLOAD_PPS_SYNC_STATS){ // Is it a PPS_SYNC stat?
                const struct PFDAS_packet_payload_pps_sync_t* pl =  PFDAS_meta_pps_sync_payload(p); // Extract payload which is a simple struct.
              //  printf("got pps_sync offset=%d\n", pl->error);
            }
            if(p->payload_type == PFDAS_PAYLOAD_TIME_STAMPS){
                const struct PFDAS_packet_payload_time_stamp_t* pl =  PFDAS_meta_time_stamp_payload(p);
               // printf("got time_stamp =%d\n", pl->sec_fpga);
            }
            PFDAS_meta_return_pkt(p, dhandle); // Important!, must return packet after it is done being used.
        }
        if(did_something == 1){
            Sleep(1); // Pace the loop some.
        }
    }
    return 0;
} // Main
