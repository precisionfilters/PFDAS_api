#include <windows.h>
#include <stdio.h>
#include "PFDAS_api.h"
#include "PFDAS_fpga_payload.h"
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
    // Print out API information.
    printf("build_info: %s", PFDAS_api_build_info() );
    printf("change_log: %s", PFDAS_api_changelog() );
    printf("api version: %s", PFDAS_api_version() );
    PFDAS_data_handle dhandle = PFDAS_handle_create(); // Allocate 1 client handle.
    PFDAS_connect("192.168.1.34:56000", dhandle);  // Connect to ip at port. 
    PFDAS_start(dhandle);                          // Start the client thread.
    int doprint = 0;
    //
    while(PFDAS_running(dhandle)){ // While the connection is active.
        while(PFDAS_data_has_pkt(dhandle)){ // While we have a data packet.
            const struct PFDAS_base_packet_t* p = PFDAS_data_get_pkt(dhandle); // Get the data packet.
            const uint8_t* fpga = PFDAS_data_fpga_payload(p); // Extract pointer to fpga payload (adc samples).
            int samplecount = PFDAS_fpga_ch_data_unpack(fpga, samples, sizeof(samples), &sec, &nsec); // Extract samples to array from fpga payload.
            doprint++;
            if(doprint%100 == 0){
                printf("ch=%d %d sec=%d nsec=%d sample count=%d\n",p->channel, doprint, sec, nsec, samplecount);
            }
            if(doprint%1000 == 0){
				write_to_disk(p); // Write contents of fpga buffer samples to a csv every so often.
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
            if(p->payload_type == PFDAS_PAYLOAD_PTP4L_STATS){ // Is it a PTP4l stat?
                const struct PFDAS_packet_payload_ptp4l_t* pl =  PFDAS_meta_ptp4l_payload(p); // Extract payload which is a simple struct.
                printf("got ptp4 offset=%d\n", pl->offset);
            }
            if(p->payload_type == PFDAS_PAYLOAD_PHC2SYS_STATS){ // Is it a PHC2SYS stat?
                const struct PFDAS_packet_payload_phc2sys_t* pl =  PFDAS_meta_phy2sys_payload(p); // Extract payload which is a simple struct.
                printf("got phc2sys offset=%d\n", pl->offset);
            }
            if(p->payload_type == PFDAS_PAYLOAD_PPS_SYNC_STATS){ // Is it a PPS_SYNC stat?
                const struct PFDAS_packet_payload_pps_sync_t* pl =  PFDAS_meta_pps_sync_payload(p); // Extract payload which is a simple struct.
                printf("got pps_sync offset=%d\n", pl->error);
            }
			if(p->payload_type == PFDAS_PAYLOAD_TIME_STAMPS){
                const struct PFDAS_packet_payload_time_stamp_t* pl =  PFDAS_meta_time_stamp_payload(p);
                printf("got time_stamp =%d\n", pl->sec_fpga);
            }
            PFDAS_meta_return_pkt(p, dhandle); // Important!, must return packet after it is done being used.
        }
      	Sleep(1); // Pace the loop some.
    }
    return 0;
} // Main
