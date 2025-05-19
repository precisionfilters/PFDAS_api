/* -------------------------------------------------------------------------- */
/* (C) Copyright 2022 Precision Filters Inc., Ithaca, NY.                     */
/* All Rights Reserved.                                                       */
/* --------------------------------------------------------------------------
 * -------------------------------------------------------------------------- */
#include "PFDAS_fpga_payload.h"
#include <stdio.h>
#include <string.h>
#ifndef _WIN32
/* Not USED on windows. Ignore this. */
// Storage for dma_capture.
#define DMA_CH_BUFF_SIZE 4096 // WORDS
#define DMA_CH_CNT 16
static int32_t dma_capture_buf[DMA_CH_CNT][DMA_CH_BUFF_SIZE];
static uint32_t dma_capture_sec[DMA_CH_CNT];
static uint32_t dma_capture_nsec[DMA_CH_CNT];
static char dma_capture_buf_text[128] = {0};
void PFDAS_fpga_debug_dma_capture(const uint8_t* fpga_buffer_start, const char* filename){
    uint8_t* ds = (uint8_t*)(fpga_buffer_start);
    FILE* fd = fopen(filename, "wt");
    for(unsigned long i=0; i<DMA_CH_CNT; i++){
        uint8_t* offset = ds + DMA_CH_BUFF_SIZE*sizeof(uint32_t)*i;
        int32_t samples = 0;
        int error = PFDAS_fpga_ch_data_unpack(offset, DMA_CH_BUFF_SIZE*sizeof(uint32_t), dma_capture_buf[i], DMA_CH_BUFF_SIZE, &dma_capture_sec[i], &dma_capture_nsec[i], &samples);
        printf("%s samples=%d error=%d [EXPECT %d error]\n", __FUNCTION__, samples, error, PFDAS_API_ERROR1_BUFFER_SIZE_GT_PAYLOAD_SAMPLES);
        //
        if(i==0){
            for(int j=0; j<samples; j++){
                memset(dma_capture_buf_text, 0, sizeof(dma_capture_buf_text));
                sprintf(dma_capture_buf_text, "%d,", j);
                fwrite(dma_capture_buf_text, 1, strlen(dma_capture_buf_text), fd);
            }
            memset(dma_capture_buf_text, 0, sizeof(dma_capture_buf_text));
            sprintf(dma_capture_buf_text, "\n");
            fwrite(dma_capture_buf_text, 1, strlen(dma_capture_buf_text), fd);
        }
        for(int j=0; j<samples; j++){
            memset(dma_capture_buf_text, 0, sizeof(dma_capture_buf_text));
            sprintf(dma_capture_buf_text, "%d,", dma_capture_buf[i][j]);
            fwrite(dma_capture_buf_text, 1, strlen(dma_capture_buf_text), fd);
        }
        memset(dma_capture_buf_text, 0, sizeof(dma_capture_buf_text));
        sprintf(dma_capture_buf_text, "\n");
        fwrite(dma_capture_buf_text, 1, strlen(dma_capture_buf_text), fd);
    }
    fclose(fd);
}
#endif
#define FPGA_BUFFER_HEADER_SIZE_BYTES 12
#define FPGA_BUFFER_HEADER_TIME_SIZE_BYTES 3
#define FPGA_WORD_BYTES 4
#define FPGA_BUFFER_HEADER_SIZE_WORDS 3
//
struct error_t {
    int error;
    char error_t[64];
};
//
static const struct error_t ERROR1S[] = {
    { PFDAS_API_ERROR1_POINTER_NULL_ARG,    "PFDAS_API_ERROR1_POINTER_NULL_ARG" },
    { PFDAS_API_ERROR1_SIZE_ZERO_BUFFER_FPGA,    "PFDAS_API_ERROR1_SIZE_ZERO_BUFFER_FPGA" },
    { PFDAS_API_ERROR1_BUFFER_SIZE_LT_PAYLOAD_SAMPLES, "PFDAS_API_ERROR1_BUFFER_SIZE_LT_PAYLOAD_SAMPLES" },
    { PFDAS_API_ERROR1_BUFFER_SIZE_GT_PAYLOAD_SAMPLES, "PFDAS_API_ERROR1_BUFFER_SIZE_GT_PAYLOAD_SAMPLES" },
    { PFDAS_API_ERROR1_INVALID_FPGA_PACK_MODE,         "PFDAS_API_ERROR1_INVALID_FPGA_PACK_MODE" },
    { PFDAS_API_ERROR1_BUFFER_SIZE_SMALLER_THAN_FPGA_HEADER, "PFDAS_API_ERROR1_BUFFER_SIZE_SMALLER_THAN_FPGA_HEADER" },
    { PFDAS_API_ERROR1_SIZE_ZERO_OUTPUT_BUFFER,    "PFDAS_API_ERROR1_SIZE_ZERO_OUTPUT_BUFFER" },
    { PFDAS_API_ERROR1_BUFFER_SIZE_SMALLER_THAN_HEADER_TIME, "PFDAS_API_ERROR1_BUFFER_SIZE_SMALLER_THAN_FPGA_HEADER" },
    { PFDAS_API_ERROR1_INVALID_PACKET_TYPE, "PFDAS_API_ERROR1_INVALID_PACKET_TYPE" },
    { PFDAS_API_NO_ERROR, "PFDAS_API_NO_ERROR" }
};
//
const char* PFDAS_error1_to_string(int error1){
    if(error1 == 0) { return "NO ERROR"; }
    for(int i=0; ERROR1S[i].error < 0; i++){
        if(error1 == ERROR1S[i].error) { return ERROR1S[i].error_t; }
    }
    return "UNKNOWN ERROR";
}
//
int PFDAS_fpga_ch_data_gettime(const uint8_t* buffer_fpga_ch, int buffer_size, uint32_t* sec, uint32_t* nsec){
    if(sec == 0)  { return PFDAS_API_ERROR1_POINTER_NULL_ARG; }
    if(nsec == 0) { return PFDAS_API_ERROR1_POINTER_NULL_ARG; }
    if(buffer_size < FPGA_BUFFER_HEADER_TIME_SIZE_BYTES){ return PFDAS_API_ERROR1_BUFFER_SIZE_SMALLER_THAN_HEADER_TIME; }
    //
    uint32_t* start = (uint32_t*)buffer_fpga_ch;
    nsec[0] = start[1];
    sec[0] = start[2];
    return PFDAS_API_NO_ERROR;
}
//
int PFDAS_fpga_ch_data_unpack(const uint8_t* buffer_fpga_ch, int buffer_size, int32_t* out_buf_32, int out_buf_32_size, uint32_t* sec, uint32_t* nsec, uint32_t* samples){
    //
    if(out_buf_32 == 0) { return PFDAS_API_ERROR1_POINTER_NULL_ARG; }
    if(out_buf_32_size == 0) { return PFDAS_API_ERROR1_SIZE_ZERO_OUTPUT_BUFFER; }
    int error = PFDAS_fpga_ch_data_gettime(buffer_fpga_ch, buffer_size, sec, nsec);
    if(error != PFDAS_API_NO_ERROR) { return error; }
    //
    int32_t sample_cnt = 0;
    int32_t pack = 0;
    error = PFDAS_fpga_ch_data_sample_info(buffer_fpga_ch, buffer_size, &pack, &sample_cnt);
    if(error == PFDAS_API_ERROR1_BUFFER_SIZE_GT_PAYLOAD_SAMPLES){
        // We allow this since it is "safe" meaning our fpga buffer is larger than required
    }else if(error < PFDAS_API_NO_ERROR) { return error; } // Bail out and return the error code, now way to proceed.
    //
    uint32_t* start = (uint32_t*)buffer_fpga_ch;
    int32_t sample_bytes = 0;
    if(pack == 0) { sample_bytes = sample_cnt * 4; }
    if(pack == 1) { sample_bytes = sample_cnt * 3; }
    if(pack == 2) { sample_bytes = sample_cnt * 2; }
    //
    int8_t* ds = (int8_t*)(start+=3);
    int32_t signext = 0;
    int samplecnt32 = 0;
    if(pack == 0) { // 32bit mode.
        for(int i=0; i<sample_bytes; i+=4){
            if(samplecnt32 == out_buf_32_size) { break; } // We fill the buffer until full then we are done.
            signext = 0;
            memcpy(&signext, &(ds[i]), 4);
            signext = signext << 8;
            signext /= 256;
            out_buf_32[samplecnt32++] = signext;
        }
    }else if(pack == 1){ // 24 bit mode.
        uint8_t signext0 = 0;
        uint8_t signext1 = 0;
        uint8_t signext2 = 0;
        uint32_t signext0_32 = 0;
        uint32_t signext1_32 = 0;
        uint32_t signext2_32 = 0;
        for(int i=0; i<sample_bytes; i+=3){
            if(samplecnt32 == out_buf_32_size) { break; }
            memcpy(&signext0, &(ds[i]), 1);
            memcpy(&signext1, &(ds[i+1]), 1);
            memcpy(&signext2, &(ds[i+2]), 1);
            signext0_32 = signext0;
            signext1_32 = signext1;
            signext2_32 = signext2;
            signext1_32 <<= 8;
            signext2_32 <<= 16;
            signext = signext0_32 | signext1_32 | signext2_32;
            signext = signext << 8;
            signext /= 256;
            out_buf_32[samplecnt32++] = signext;
        }
    }else if(pack == 2){ // 16 bit mode.
        int16_t signext16 = 0;
        for(int i=0; i<sample_bytes; i+=2){
            if(samplecnt32 == out_buf_32_size) { break; } // We fill the buffer until full then we are done.
            memcpy(&signext16, &(ds[i]), 2);
            out_buf_32[samplecnt32++] = signext16*256;
        }
    }
    samples[0] = samplecnt32;
    return error;
}
//
int PFDAS_fpga_ch_data_sample_info(const uint8_t* buffer_fpga_ch, int buffer_size, int32_t* pack, int32_t* sample_cnt){
    if(pack == 0)           { return PFDAS_API_ERROR1_POINTER_NULL_ARG;  }
    if(sample_cnt == 0)     { return PFDAS_API_ERROR1_POINTER_NULL_ARG;  }
    if(buffer_fpga_ch == 0) { return PFDAS_API_ERROR1_POINTER_NULL_ARG;  }
    if(buffer_size == 0)    { return PFDAS_API_ERROR1_SIZE_ZERO_BUFFER_FPGA; }
    if(buffer_size < FPGA_BUFFER_HEADER_SIZE_BYTES){ return PFDAS_API_ERROR1_BUFFER_SIZE_SMALLER_THAN_FPGA_HEADER; }
    //
    int error = PFDAS_API_NO_ERROR;
    pack[0] = -1;
    sample_cnt[0] = 0;
    uint32_t* start = (uint32_t*)buffer_fpga_ch;
    pack[0] = start[0] >> 30 ; //0->32bits, 1->24bits, 2->16bits
    uint32_t n32_sample_address = (start[0] >> 8) &  0xfff; // The FPGA returns sample address NOT count of words.
    uint32_t n32_sample_words = n32_sample_address+1;       // Now +1 to make it count/length of samples words.
    n32_sample_words = (n32_sample_words)-FPGA_BUFFER_HEADER_SIZE_WORDS; // Do not count the header words.
    sample_cnt[0] = (n32_sample_words*FPGA_WORD_BYTES);                   // The total count of samples bytes we have.
    int32_t buffer_samples_bytes = buffer_size-FPGA_BUFFER_HEADER_SIZE_BYTES;
    if(buffer_samples_bytes != sample_cnt[0]){ // We cannot always be sure the user will pass in a full buffer make sure we do not exceed the bounds!
        if(buffer_samples_bytes < sample_cnt[0]) { error = PFDAS_API_ERROR1_BUFFER_SIZE_LT_PAYLOAD_SAMPLES; } // This condition is TRUE if we have a channel stream turned off, as in muted.
        if(buffer_samples_bytes > sample_cnt[0]) { error = PFDAS_API_ERROR1_BUFFER_SIZE_GT_PAYLOAD_SAMPLES; } // This should not happen.
    }
    if(pack[0] == 0) { // 32bit mode.
        sample_cnt[0] /= 4;
    }else if(pack[0] == 1){ // 24 bit mode.
        sample_cnt[0] /= 3;
    }else if(pack[0] == 2){ // 16 bit mode.
        sample_cnt[0] /= 2;
    }else{
        sample_cnt[0] = 0; // If the pack mode is invalid we have no way of knowing the sample count.
        return PFDAS_API_ERROR1_INVALID_FPGA_PACK_MODE;
    }
    return error;
}
//
int PFDAS_sys_packet_gettime(struct PFDAS_base_packet_t* packet, uint32_t* sec, uint32_t* nsec){
    if(packet == 0)           { return PFDAS_API_ERROR1_POINTER_NULL_ARG;  }
    if(sec == 0)           { return PFDAS_API_ERROR1_POINTER_NULL_ARG;  }
    if(nsec == 0)           { return PFDAS_API_ERROR1_POINTER_NULL_ARG;  }
    //
    sec[0] = 0; nsec[0] = 0;
    if(packet->payload_type == PFDAS_PAYLOAD_PTP4L_STATS){
        struct PFDAS_packet_payload_ptp4l_t* pl = (struct PFDAS_packet_payload_ptp4l_t*)(packet+1);
        sec[0] = pl->sec; nsec[0] = pl->nsec;
    }else
    if(packet->payload_type == PFDAS_PAYLOAD_PHC2SYS_STATS){
        struct PFDAS_packet_payload_phc2sys_t* pl = (struct PFDAS_packet_payload_phc2sys_t*)(packet+1);
        sec[0] = pl->sec; nsec[0] = pl->nsec;
    }else
    if(packet->payload_type == PFDAS_PAYLOAD_PPS_SYNC_STATS){
        struct PFDAS_packet_payload_pps_sync_t* pl = (struct PFDAS_packet_payload_pps_sync_t*)(packet+1);
        sec[0] = pl->sec; nsec[0] = pl->nsec;
    }else
    if(packet->payload_type == PFDAS_PAYLOAD_TIME_STAMPS){
        struct PFDAS_packet_payload_time_stamp_t* pl = (struct PFDAS_packet_payload_time_stamp_t*)(packet+1);
        sec[0] = pl->sec_linux; nsec[0] = 0; // We don't have anything but seconds for this packet.
    }else
    if(packet->payload_type == PFDAS_PAYLOAD_RECORD_EVENT){
        struct PFDAS_packet_payload_event_t* pl = (struct PFDAS_packet_payload_event_t*)(packet+1);
        sec[0] = pl->sec; nsec[0] = pl->nsec;
    }else
    if(packet->payload_type == PFDAS_PAYLOAD_LCS_CMD){
        struct PFDAS_packet_payload_lcs_cmd_t* pl = (struct PFDAS_packet_payload_lcs_cmd_t*)(packet+1);
        sec[0] = pl->sec; nsec[0] = pl->nsec;
    }else{
        return PFDAS_API_ERROR1_INVALID_PACKET_TYPE;
    }
    return PFDAS_API_NO_ERROR;
}
//
int PFDAS_fpga_ch_data_overwrite(const int32_t* in_buf_32, int in_buf_32_size, uint8_t* buf_fpga_ch, int buf_fpga_ch_size){
    if(in_buf_32 == 0)     { return PFDAS_API_ERROR1_POINTER_NULL_ARG;  }
    if(in_buf_32_size == 0)    { return PFDAS_API_ERROR1_SIZE_ZERO_BUFFER_FPGA; }
    int32_t pack = 0;
    int32_t numsamples = 0;
    int error = PFDAS_fpga_ch_data_sample_info(buf_fpga_ch, buf_fpga_ch_size, &pack, &numsamples);
    if(error != PFDAS_API_NO_ERROR) { return error; }
    uint32_t* start = (uint32_t*)buf_fpga_ch;
    int8_t* ds = (int8_t*)(start+=3);
    int32_t newdata = 0;
    if(pack == 0) { // 32bit mode.
        int32_t numsamplebytes = numsamples*4;
        int samplecnt = 0;
        for(int i=0; i<numsamplebytes; i+=4){
            if(samplecnt == in_buf_32_size) { break; } // We fill the buffer until full then we are done.
            newdata = in_buf_32[samplecnt++];
            memset(&(ds[i]), 0, 4); // Be sure the upper unused bytes is zero.
            memcpy(&(ds[i]), &newdata, 3); // Only copy the first three bytes, this ensures we produce the same as the ADC.
        }
    }else if(pack == 1){ // 24 bit mode.
        int32_t numsamplebytes = numsamples*3;
        int samplecnt = 0;
        for(int i=0; i<numsamplebytes; i+=3){
            if(samplecnt == in_buf_32_size) { break; } // We fill the buffer until full then we are done.
            newdata = in_buf_32[samplecnt++];
            memcpy(&(ds[i]), &newdata, 3); // Only copy the first three bytes, this ensures we produce the same as the ADC.
        }
    }else if(pack == 2){ // 16 bit mode.
        int32_t numsamplebytes = numsamples*2;
        int samplecnt = 0;
        for(int i=0; i<numsamplebytes; i+=2){
            if(samplecnt == in_buf_32_size) { break; } // We fill the buffer until full then we are done.
            newdata = in_buf_32[samplecnt++];
            newdata /= 256;
            memcpy(&(ds[i]), &newdata, 2); // Only copy the first three bytes, this ensures we produce the same as the ADC.
        }
    }
    return PFDAS_API_NO_ERROR;
}
//
uint64_t PFDAS_ptp_uint32_2_uint64_secs(uint32_t sec, uint32_t nsec){
    return (uint64_t)nsec+(uint64_t)sec*1000000000;
}
//
double PFDAS_int64_nanosec2_double_secs(int64_t nanosecs){
    return ((double)nanosecs)/1000000000.0;
};
//
double PFDAS_cnt_to_volt_scalar(){
    static double fs_peak = 4.095; // Set by the ADC reference voltage.
    static double gain_factor =  2.44180; 
    static double two_to_the_24 = 16777216; // 24 bits.
    double twentyfourbitsto10v = (two_to_the_24/(fs_peak*2)/gain_factor);
    return twentyfourbitsto10v;
}
