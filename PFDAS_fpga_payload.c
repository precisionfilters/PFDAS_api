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
        int samples = PFDAS_fpga_ch_data_unpack(offset, dma_capture_buf[i], DMA_CH_BUFF_SIZE, &dma_capture_sec[i], &dma_capture_nsec[i]);
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
void PFDAS_fpga_ch_data_gettime(const uint8_t* buffer_fpga_ch, uint32_t* sec, uint32_t* nsec){
    uint32_t* start = (uint32_t*)buffer_fpga_ch;
    nsec[0] = start[1];
    sec[0] = start[2];
}
//
int PFDAS_fpga_ch_data_unpack(const uint8_t* buffer_fpga_ch, int32_t* output_buffer_32, int output_buffer_size, uint32_t* sec, uint32_t* nsec){
    uint32_t* start = (uint32_t*)buffer_fpga_ch;
    nsec[0] = start[1];
    sec[0] = start[2];
    //
    int debug = 0;
    int32_t samplecount = 0;
    int32_t pack = 0;
	PFDAS_fpga_ch_data_sample_info(buffer_fpga_ch, &pack, &samplecount);
    int32_t samplebytes = 0;
    if(pack == 0) { samplebytes = samplecount * 4; }
    if(pack == 1) { samplebytes = samplecount * 3; }
    if(pack == 2) { samplebytes = samplecount * 2; }
    if(debug){ samplecount++; }
    //
  	int8_t* ds = (int8_t*)(start+=3);
    int32_t signext = 0;
    int samplecnt32 = 0;
    if(pack == 0) { // 32bit mode.
        for(int i=0; i<samplebytes; i+=4){
            if(samplecnt32 == output_buffer_size) { break; } // We fill the buffer until full then we are done.
            signext = 0;
            memcpy(&signext, &(ds[i]), 4);
            signext = signext << 8;
            signext /= 256;
            output_buffer_32[samplecnt32++] = signext;
        }
    }else if(pack == 1){ // 24 bit mode.
        uint8_t signext0 = 0;
        uint8_t signext1 = 0;
        uint8_t signext2 = 0;
        uint32_t signext0_32 = 0;
        uint32_t signext1_32 = 0;
        uint32_t signext2_32 = 0;
        for(int i=0; i<samplebytes; i+=3){
            if(samplecnt32 == output_buffer_size) { break; }
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
            output_buffer_32[samplecnt32++] = signext;
        }
    }else if(pack == 2){ // 16 bit mode.
        int16_t signext16 = 0;
        for(int i=0; i<samplebytes; i+=2){
            if(samplecnt32 == output_buffer_size) { break; } // We fill the buffer until full then we are done.
            memcpy(&signext16, &(ds[i]), 2);
            output_buffer_32[samplecnt32++] = signext16*256;
            //output_buffer_32[samplecnt32++] = signext16;
        }
    }
    return samplecnt32;
}
//
void PFDAS_fpga_ch_data_sample_info(const uint8_t* buffer_fpga_ch, int32_t* pack, int32_t* samplecnt){
    uint32_t* start = (uint32_t*)buffer_fpga_ch;
    uint32_t n32samplewords = (start[0] >> 8) &  0xfff;
    n32samplewords = (n32samplewords+1)-3;
    samplecnt[0] = (n32samplewords*4);
    pack[0] = start[0] >> 30 ; //0->32bits, 1->24bits, 2->16bits
    if(pack[0] == 0) { // 32bit mode.
        samplecnt[0] /= 4;
    }else if(pack[0] == 1){ // 24 bit mode.
        samplecnt[0] /= 3;
    }else if(pack[0] == 2){ // 16 bit mode.
        samplecnt[0] /= 2;
    }
}

void PFDAS_fpga_ch_data_overwrite(const int32_t* input_buffer_32, uint8_t* buffer_fpga_ch, int input_buffer_size){
    int32_t pack = 0;
    int32_t numsamples = 0;
    PFDAS_fpga_ch_data_sample_info(buffer_fpga_ch, &pack, &numsamples);
    uint32_t* start = (uint32_t*)buffer_fpga_ch;
  	int8_t* ds = (int8_t*)(start+=3);
    int32_t newdata = 0;
    if(pack == 0) { // 32bit mode.
    	int32_t numsamplebytes = numsamples*4;
    	int samplecnt = 0;
        for(int i=0; i<numsamplebytes; i+=4){
            if(samplecnt == input_buffer_size) { break; } // We fill the buffer until full then we are done.
            newdata = input_buffer_32[samplecnt++];
            memset(&(ds[i]), 0, 4); // Be sure the upper unused bytes is zero.
            memcpy(&(ds[i]), &newdata, 3); // Only copy the first three bytes, this ensures we produce the same as the ADC.
        }
    }else if(pack == 1){ // 24 bit mode.
    	int32_t numsamplebytes = numsamples*3;
        int samplecnt = 0;
        for(int i=0; i<numsamplebytes; i+=3){
            if(samplecnt == input_buffer_size) { break; } // We fill the buffer until full then we are done.
            newdata = input_buffer_32[samplecnt++];
            memcpy(&(ds[i]), &newdata, 3); // Only copy the first three bytes, this ensures we produce the same as the ADC.
        }
    }else if(pack == 2){ // 16 bit mode.
    	int32_t numsamplebytes = numsamples*2;
        int samplecnt = 0;
        for(int i=0; i<numsamplebytes; i+=2){
            if(samplecnt == input_buffer_size) { break; } // We fill the buffer until full then we are done.
            newdata = input_buffer_32[samplecnt++];
            newdata /= 256;
            memcpy(&(ds[i]), &newdata, 2); // Only copy the first three bytes, this ensures we produce the same as the ADC.
        }
    }
}
uint64_t PFDAS_ptp_uint32_2_uint64_secs(uint32_t sec, uint32_t nsec){
    return (uint64_t)nsec+(uint64_t)sec*1000000000;
}
double PFDAS_int64_nanosec2_double_secs(int64_t nanosecs){
    return ((double)nanosecs)/1000000000.0;
};
double PFDAS_cnt_to_volt_scalar(){
    static double fs_peak = 4.095; // Set by the ADC reference voltage.
    static double gain_factor =  2.44180; 
    static double two_to_the_24 = 16777216; // 24 bits.
    double twentyfourbitsto10v = (two_to_the_24/(fs_peak*2)/gain_factor);
    return twentyfourbitsto10v;
}
