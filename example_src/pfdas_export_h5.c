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

    char *exe_name = argv[0];
    char exe_path[MAX_PATH];
    DWORD len = GetModuleFileName(NULL, exe_path, MAX_PATH);
    if (len == 0) {
        perror("GetModuleFileName");
        exit(1);
    }
    exe_name = exe_path;

    char *info_name = malloc(strlen(exe_name) + 6); // +6 for ".info\0"
    strcpy_s(info_name, strlen(exe_name) + 6, exe_name);
    char *dot = strrchr(info_name, '.');
    if(dot != NULL) {
        *dot = '\0';
        strcat_s(info_name, strlen(exe_name) + 6, ".info");
    } else {
        strcat_s(info_name, strlen(exe_name) + 6, ".info");
    }

    FILE *fp;
    errno_t err = fopen_s(&fp, info_name, "w");
    if(err != 0) {
        perror("Error opening file");
        free(info_name);
        exit(1);
    }

    fprintf(fp, "fname=Export to HDF5 (my_dataset)\n");
    fprintf(fp, "version=0.1\n");
    fprintf(fp, "author=jpietzuch@pfinc.com\n");
    fprintf(fp, "date=%s %s\n", __DATE__, __TIME__);
    unsigned int majnum, minnum, relnum;
    H5get_libversion(&majnum, &minnum, &relnum);
    fprintf(fp, "info=HDF5 Library Version: %u.%u.%u\n", majnum, minnum, relnum);
    fprintf(fp, "info=Exporting time domain samples to dataset: my_dataset\n");

    fclose(fp);
    free(info_name);
    if (argc < 2) {
        printf("Usage: %s <connection_string> [h5_filename] [num_samples]\n", argv[0]);
        return 1;
    }
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
    printf("api version: %s", PFDAS_api_version() );

    PFDAS_data_handle dhandle = PFDAS_handle_stream_file_create(); // Allocate 1 client handle.
    PFDAS_connect(connection_string, dhandle);  // Connect to ip at port.
    struct PFDAS_footer_packet_t footer = PFDAS_file_footer(dhandle); // Footer is immediately available after connect (file open).
    printf("footer:\n");
    printf("File Size: %llu Bytes\n", footer.file_size_bytes);
    printf("File Start: %llu Nano seconds\n", footer.file_start_time);
    printf("File End: %llu Nano seconds\n", footer.file_end_time);
    int sample_count = footer.sample_counts[0];
    int sample_count_uniform = true;
    uint64_t sample_count_max = 0;
    // Using the sample counts at the footer of the file (written at end of recording).
    // Find out if all the sample counts are the same or not.
    for(int i=0; i<PFDAS_CH_MAX_PER_BOX; i++){
        if(sample_count != footer.sample_counts[i]){ sample_count_uniform = 0; }
        printf("Chan %d sample count: %llu\n", i, footer.sample_counts[i]);
        if(footer.sample_counts[i] > sample_count_max) {
            sample_count_max = footer.sample_counts[i];
        }
    }
    // This is checking if the user requested a reduced number of samples to export.
    if(num_samples == -1){
        num_samples = sample_count;
        printf("Extracting all samples to h5 file (%d)\n", sample_count);
    }
    // Alert that we have a variable number of samples for each channel.
    if(sample_count_uniform == 0){
        printf("Sample count in file is not uniform.\n");
    }
    // If we request more than is in the file, then set request to max.
    if(num_samples > sample_count_max){
        printf("Reduced requested sample count (%d) to samples available in file (%lld)\n", num_samples, sample_count_max);
        num_samples = sample_count_max;
    }

    PFDAS_start(dhandle);                          // Start the client thread,
    if(pfdas_hdf5_open(h5_filename, 16, num_samples)){
        printf("%s file ready\n", h5_filename);
    }else{
        printf("%s file not ready\n", h5_filename);
        exit(0);
    }
    //
    int doprint = 0;
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
    struct PFDAS_header_packet_t header = PFDAS_file_header(dhandle);
    printf("header=%s\n", header.keyword_value);
    PFDAS_data_continue(dhandle); // Instruct File reader to continue reading the file, the metadata has all been read out.
    int data_read_cnt = 16;
    while(PFDAS_running(dhandle) && writing){ // While the connection is active.
        data_read_cnt = 16; // Only read out so many data packets to allow meta data queue to be handled, this is ONLY for debug and information. This is not needed to for the h5 export.
        while(PFDAS_data_has_pkt(dhandle) && data_read_cnt-- > 0){ // While we have a data packet.
            const struct PFDAS_base_packet_t* p = PFDAS_data_get_pkt(dhandle); // Get the data packet.
            if(pfdas_hdf5_full()){
                pfdas_hdf5_close();
                writing = 0;
            }else{
                pfdas_hdf5_write_packet(p);
            }
            PFDAS_data_return_pkt(p, dhandle); // Important!, must return packet after it is done being used.
        }
        while(PFDAS_meta_has_pkt(dhandle)){ // While we have metadata.
            const struct PFDAS_base_packet_t* p = PFDAS_meta_get_pkt(dhandle); // Get the metadata packet.
            if(p->payload_type == PFDAS_PAYLOAD_LCS_CMD){ // Is it a LCS command?
                const struct PFDAS_packet_payload_lcs_cmd_t* pl =  PFDAS_meta_lcs_cmd_payload(p); // Extract payload which is a simple struct.
                char tmp[512] = {0};
                memcpy(tmp, pl+1, pl->text_size);
                //printf("got lcs cmd %s\n", tmp);
            }
            if(p->payload_type == PFDAS_PAYLOAD_RECORD_EVENT){ // Is it an event?
                const struct PFDAS_packet_payload_event_t* pl =  PFDAS_meta_event_payload(p); // Extract payload which is a simple struct.
                char tmp[512] = {0};
                memcpy(tmp, pl->event, sizeof(pl->event));
                printf("got event %s\n", tmp);
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
    }
    printf("\n");
    printf("exported_filepath=%s\n", h5_filename);
    return 0;
} // Main
