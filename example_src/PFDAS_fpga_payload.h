/* -------------------------------------------------------------------------- */
/* (C) Copyright 2022 Precision Filters Inc., Ithaca, NY.                     */
/* All Rights Reserved.                                                       */
/* --------------------------------------------------------------------------
 * -------------------------------------------------------------------------- */
#ifndef __PFDAS_H__
#define __PFDAS_H__
#include <stdint.h>
#include <string.h>
#include "PFDAS_packet_types.h"
#ifdef __cplusplus
extern "C" {
#endif
#ifndef _WIN32
void PFDAS_fpga_debug_dma_capture(const uint8_t* fpga_buffer_start, const char* filename);
#endif
#define PFDAS_CH_MAX_PER_BOX 16
#define PFDAS_API_NO_ERROR 0
#define PFDAS_API_ERROR1_POINTER_NULL_ARG -1
#define PFDAS_API_ERROR1_SIZE_ZERO_BUFFER_FPGA -2
#define PFDAS_API_ERROR1_BUFFER_SIZE_GT_PAYLOAD_SAMPLES -3
#define PFDAS_API_ERROR1_BUFFER_SIZE_LT_PAYLOAD_SAMPLES -4
#define PFDAS_API_ERROR1_INVALID_FPGA_PACK_MODE -5
#define PFDAS_API_ERROR1_BUFFER_SIZE_SMALLER_THAN_FPGA_HEADER -6
#define PFDAS_API_ERROR1_SIZE_ZERO_OUTPUT_BUFFER -7
#define PFDAS_API_ERROR1_BUFFER_SIZE_SMALLER_THAN_HEADER_TIME -8
#define PFDAS_API_ERROR1_INVALID_PACKET_TYPE -9
/**
 * \brief Convert Error to human readable string.
 *
 * \param error1 The error code.
 * \return Pointer to the string.
 */
const char* PFDAS_error1_to_string(int error1);
/**
 * \brief Extracts absolute time and samples from a fpga buffer (1 channel).
 *
 * Intended to be used with network packets which encapsulate 1 channel of ADC data as received from the FPGA.
 * The function will automatically detect the fpga buffer size and bits per sample and present them in the output buffer as signed 32 words.
 * The time value is referenced to the first sample in the buffer. By comparing time between buffers and number of samples, sample rate can be inferred.
 * 
 * \param buffer_fpga_ch Pointer to the 1 channels worth of ADC data.
 * \param output_buffer_32 Pointer to the destination of the extracted ADC samples.
 * \param output_buffer_size The allocated size of the output_buffer_32. To keep it simple make this buffer > 4096 as this is the maximum number of sample that can in a FPGA buffer.
 * \param sec Pointer to the memory location that will receive the seconds portion of the abs time.
 * \param nsec Pointer to the memory location that will receive the nano seconds portion of the abs time.
 * \param samples Pointer to the memory location that will receive the number of samples now in the output buffer.
 * \return Error if less than 0.
 */
int PFDAS_fpga_ch_data_unpack(const uint8_t* buffer_fpga_ch, int buffer_size, int32_t* output_buffer_32, int output_buffer_size, uint32_t* sec, uint32_t* nsec, uint32_t* samples);
/**
 * \brief Extracts absolute time fpga buffer (1 channel).
 *
 * Intended to be used with network packets which encapsulate 1 channel of ADC data as received from the FPGA.
 * Overlapping functionality with fpga_ch_data_unpack.
 * The time value is referenced to the first sample in the buffer. By comparing time between buffers and number of samples, sample rate can be inferred.
 * 
 * \param buffer_fpga_ch Pointer to the 1 channels worth of ADC data.
 * \param sec Pointer to the memory location that will receive the seconds portion of the abs time.
 * \param nsec Pointer to the memory location that will receive the nano seconds portion of the abs time.
 * \return Error if less than 0.
 */
int PFDAS_fpga_ch_data_gettime(const uint8_t* buffer_fpga_ch, int buffer_size, uint32_t* sec, uint32_t* nsec);
/**
 * \brief Extracts absolute time from system packet.
 *
 * \param packet Pointer to the system packet.
 * \param sec Pointer to the memory location that will receive the seconds portion of the abs time.
 * \param nsec Pointer to the memory location that will receive the nano seconds portion of the abs time.
 * \return Error if less than 0.
 */
int PFDAS_sys_packet_gettime(struct PFDAS_base_packet_t* packet, uint32_t* sec, uint32_t* nsec);
/**
 * \brief Extracts number of samples and pack mode (16,24 or 32 bit).
 *
 * Intended to be used with network packets which encapsulate 1 channel of ADC data as received from the FPGA.
 * pack == 0 -> 32 bit pack, pack == 1 -> 24 bit pack, pack == 2 -> 16 bit pack.
 * 
 * \param buffer_fpga_ch Pointer to the 1 channels worth of ADC data.
 * \param sec Pointer to the memory location that will receive pack value.
 * \param nsec Pointer to the memory location that will receive the sample count.
 * \param samples Pointer to the memory location that will receive the number of samples as described in the fpga buffer header.
 * \return Error if less than 0.
 */
int PFDAS_fpga_ch_data_sample_info(const uint8_t* buffer_fpga_ch, int buffer_size, int32_t* pack, int32_t* samples);
/**
 * \brief Optional usage function to overwrite the FPGA channel data with prepopulate values.
 *
 * This function can be used to simulate signals or inject known values for software testing purposes.
 * 
 * \param in_buf_32 Pointer to the sample data that will overwrite the ADC sample data.
 * \param in_buf_32_size Number of elements in the in_buf_32.
 * \param buf_fpga_ch Pointer to the 1 channels worth of ADC data.
 * \param buf_fpga_ch_size Size of buf_fpga_ch.
 * \return Error if less than 0.
 */
int PFDAS_fpga_ch_data_overwrite(const int32_t* in_buf_32, int in_buf_32_size, uint8_t* buf_fpga_ch, int buf_fpga_ch_size);
/**
 * \brief Function to convert sec and nsec as int32's to one int64 as nanoseconds.
 *
 * 
 * \param sec Seconds in absolute time.
 * \param nsec Fractional nano second portion of absolute time.
 * \return Nano seconds in absolute time.
 */
uint64_t PFDAS_ptp_uint32_2_uint64_secs(uint32_t sec, uint32_t nsec);
/**
 * \brief Function to convert nanosecond absolute time to a double type.
 *
 * 
 * \param sec Seconds in absolute time.
 * \param nsec Fractional nano second portion of absolute time.
 * \return Nano seconds in absolute time.
 */
double PFDAS_int64_nanosec2_double_secs(int64_t nanosecs);
/**
 * \brief Function to provide the scalar to convert sample counts to voltage.
 *
 * The ADC is calibrated within the FPGA at the sample level, therefore this function uses the nominal conversion.
 * 
 * \return Counts to voltage scalar in units of Volts.
 */
double PFDAS_cnt_to_volt_scalar();
#ifdef __cplusplus
}
#endif
//
#endif
