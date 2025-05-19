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

void write_to_disk(const struct PFDAS_base_packet_t* p){
    double cnt2volts = PFDAS_cnt_to_volt_scalar(); // This is a constant used to scale counts to volts.
    const uint8_t* fpga = PFDAS_data_fpga_payload(p);
    printf("dump to file %g\n", cnt2volts);
    int samplecount = PFDAS_fpga_ch_data_unpack(fpga, samples, sizeof(samples), &sec, &nsec);
    FILE* fd = 0;
    fopen_s(&fd, "test.csv", "wb");
    sprintf(buf, "chan=%d\n", p->channel);
    fwrite(buf, 1, strlen(buf), fd);
    for(int i=0; i<samplecount; i++){
        sprintf(buf, "%g,", (double)samples[i]/cnt2volts);
        fwrite(buf, 1, strlen(buf), fd);
    }
    fclose(fd);
}
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
#ifdef FROM_TCP
        strncpy_s(h5_filename, 256, "data.h5", 8);
#else
        strncpy_s(h5_filename, 256, argv[1], 252); // Leave room for.h5 extension
        for(int i=0; i<strlen(argv[1]); i++){
            if(h5_filename[i] == '.'){
                h5_filename[i] = '-';
            }
        }
        strncat_s(h5_filename, 256, ".h5", 4); // Add.h5 extension
#endif
    }
    // Number of samples (optional, default to 65536*400)
    int num_samples = (argc > 3)? atoi(argv[3]) : -1;

    // Print out API information.
    printf("build_info: %s", PFDAS_api_build_info() );
    printf("change_log: %s", PFDAS_api_changelog() );
    printf("api version: %s", PFDAS_api_version() );

//#define FROM_TCP
#ifdef FROM_TCP
    PFDAS_data_handle dhandle = PFDAS_handle_stream_tcp_create(); // Allocate 1 client handle.
#else
    PFDAS_data_handle dhandle = PFDAS_handle_stream_file_create(); // Allocate 1 client handle.
#endif
    PFDAS_connect(connection_string, dhandle);  // Connect to ip at port.
    struct PFDAS_footer_packet_t footer = PFDAS_file_footer(dhandle);
    printf("footer:\n");
    printf("File Size: %llu Bytes\n", footer.file_size_bytes);
    printf("File Start: %llu Nano seconds\n", footer.file_start_time);
    printf("File End: %llu Nano seconds\n", footer.file_end_time);
    int sample_count = footer.sample_counts[0];
    int sample_count_uniform = true;
    uint64_t sample_count_min = INT64_MAX;
    for(int i=0; i<16; i++){
        if(sample_count != footer.sample_counts[i]){ sample_count_uniform = 0; }
        printf("Chan %d sample count: %llu\n", i, footer.sample_counts[i]);
        if(footer.sample_counts[i] < sample_count_min) {
            sample_count_min = footer.sample_counts[i];
        }
    }
    if(num_samples == -1){
        num_samples = sample_count;
        printf("Extracting all samples to h5 file (%d)\n", sample_count);
    }
    if(sample_count_uniform == 0 && num_samples > sample_count_min){
        printf("Sample count in file is not uniform.\n");
    }
    if(num_samples > sample_count_min){
        printf("Reduced requested sample count (%d) to samples available in file (%lld)\n", num_samples, sample_count_min);
        num_samples = sample_count_min;
    }
    PFDAS_start(dhandle);                          // Start the client thread.
    if(pfdas_hdf5_open(h5_filename, 16, num_samples)){
        printf("%s file ready\n", h5_filename);
    }else{
        printf("%s file not ready\n", h5_filename);
    }
    //
    int doprint = 0;
    int writing = 1;
    for(int i=0; i<10 && PFDAS_running(dhandle); i++){ // Try several times to be sure we read out all the header info.
        while(PFDAS_meta_has_pkt(dhandle)){ // While we have metadata.
            const struct PFDAS_base_packet_t* p = PFDAS_meta_get_pkt(dhandle); // Get the metadata packet.
            if(p->payload_type == PFDAS_PAYLOAD_LCS_CMD){ // Is it a LCS command?
                const struct PFDAS_packet_payload_lcs_cmd_t* pl =  PFDAS_meta_lcs_cmd_payload(p); // Extract payload which is a simple struct.
                char tmp[512] = {0};
                memcpy(tmp, pl+1, pl->text_size);
                printf("got lcs cmd %s\n", tmp);
            }
        }
        Sleep(10);
    }
    while(PFDAS_running(dhandle) && writing){ // While the connection is active.
        while(PFDAS_data_has_pkt(dhandle)){ // While we have a data packet.
            const struct PFDAS_base_packet_t* p = PFDAS_data_get_pkt(dhandle); // Get the data packet.
            const uint8_t* fpga = PFDAS_data_fpga_payload(p); // Extract pointer to fpga payload (adc samples).
            int samplecount = PFDAS_fpga_ch_data_unpack(fpga, samples, sizeof(samples), &sec, &nsec); // Extract samples to array from fpga payload.
              doprint++;
              if(doprint%1000 == 0){
                  printf("ch=%d %d sec=%d nsec=%d sample count=%d\n",p->channel, doprint, sec, nsec, samplecount);
              }
            if(pfdas_hdf5_full()){
                pfdas_hdf5_close();
                writing = 0;
            }else{
                pfdas_hdf5_write_packet(p);
            }
            //if(doprint%1000 == 0){
            //    write_to_disk(p); // Write contents of fpga buffer samples to a csv every so often.
           // }
            PFDAS_data_return_pkt(p, dhandle); // Important!, must return packet after it is done being used.
        }
        while(PFDAS_meta_has_pkt(dhandle)){ // While we have metadata.
            const struct PFDAS_base_packet_t* p = PFDAS_meta_get_pkt(dhandle); // Get the metadata packet.
            if(p->payload_type == PFDAS_PAYLOAD_LCS_CMD){ // Is it a LCS command?
                const struct PFDAS_packet_payload_lcs_cmd_t* pl =  PFDAS_meta_lcs_cmd_payload(p); // Extract payload which is a simple struct.
                char tmp[512] = {0};
                memcpy(tmp, pl+1, pl->text_size);
                printf("got lcs cmd %s\n", tmp);
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
        //Sleep(1); // Pace the loop some.
    }
    return 0;
} // Main
