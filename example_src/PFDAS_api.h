/* -------------------------------------------------------------------------- */
/* (C) Copyright 2023 Precision Filters Inc., Ithaca, NY.                     */
/* All Rights Reserved.                                                       */
/* --------------------------------------------------------------------------
 * -------------------------------------------------------------------------- */
#ifndef __PFDAS_API_H__
#define __PFDAS_API_H__
#include "PFDAS_packet_types.h"
#ifdef __cplusplus
extern "C" {
#endif
/* .pfr file layout */
/*
___________________________________________________________________________________________________________________________________________________________________________
| 1 Byte      |               |                                   ~ |                                                                 ~ | (read after stream byte by seek) |
| Stream type | Header Packet | LCS state (variable # of packets) ~ | Variable number of base packet types each with varying payload  ~ | Footer Packet                    |
---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
                                                                    ^
                                                       (File Reader waits here until told to proceed)

*/
/****************************************************************************************************************/
typedef void (*callback_t)(void);
/****************************************************************************************************************/
/**
 * @brief Utility to parse an lcs command into it's 3 parts.
 * @param The full string of the lcs command unparsed.
 * @param Pointer to a pointer that will accept the internal memory location of the cmd.
 * @param Pointer to a pointer that will accept the internal memory location of the value.
 * @param Pointer to a pointer that will accept the internal memory location of the card number.
 */
_declspec(dllexport) void PFDAS_lcs_parse(const char* lcs_cmd, char** cmd, char** value, char** card);
/****************************************************************************************************************/
/**
 * @brief Waits on a semaphore.
 */
__declspec(dllexport) void PFDAS_semaphore_wait();
/****************************************************************************************************************/
/**
 * @brief Posts a semaphore.
 */
__declspec(dllexport) void PFDAS_semaphore_post();
/****************************************************************************************************************/
/**
 * @brief Locks a mutex.
 */
__declspec(dllexport) void PFDAS_mutex_lock();
/****************************************************************************************************************/
/**
 * @brief Unlocks a mutex.
 */
__declspec(dllexport) void PFDAS_mutex_unlock();
/****************************************************************************************************************/
/**
 * @brief Runs a thread with a given callback function.
 * @param it The callback function to run in the thread.
 */
__declspec(dllexport) void PFDAS_thread_run(callback_t it);
/****************************************************************************************************************/
/**
 * @brief Checks if a thread has finished execution.
 * @return 1 if the thread is done, 0 otherwise.
 */
__declspec(dllexport) int PFDAS_thread_done();
/****************************************************************************************************************/
/**
 * @brief Initializes the reblock module with a given reblock size and number of channels.
 * @param reblock_size The size of the reblock buffer.
 * @param channels The number of channels to reblock.
 * @return 1 on success, 0 on error.
 */
__declspec(dllexport) int PFDAS_data_reblock_init(int reblock_size, int channels);
/****************************************************************************************************************/
/**
 * @brief Appends a packet of data to the reblock buffer.
 * @param pkt A pointer to a PFDAS_base_packet_t structure containing the data to append.
 * @return Samples appended or if less than 0, error.
 */
__declspec(dllexport) int PFDAS_data_reblock_append(const struct PFDAS_base_packet_t* pkt);
/****************************************************************************************************************/
/**
 * @brief Gets the length of the reblock buffer for a given channel.
 * @param channel The channel to get the length for.
 * @return The length of the reblock buffer for the given channel.
 */
__declspec(dllexport) int PFDAS_data_reblock_length(int channel);
/****************************************************************************************************************/
/**
 * @brief Gets a pointer to the reblock data for a given channel.
 * @param channel The channel to get the data for.
 * @return A pointer to the reblock data for the given channel.
 */
/****************************************************************************************************************/
__declspec(dllexport) int32_t* PFDAS_data_reblock_data(int channel);
/**
 * @brief Shifts the reblock buffer by a given number of samples.
 * @param samples The number of samples to shift the buffer by.
 */
__declspec(dllexport) void PFDAS_data_reblock_shift(int samples);
/****************************************************************************************************************/
typedef void* PFDAS_data_handle; /* Opaque pointer to C++ object. */
/****************************************************************************************************************/
/**
 * \brief Allocate one instance of a network client.
 *
 * This must be the first function a called when using the API. Multiple instances can be created.
 * 
 * \return Pointer to network client instance.
 */
__declspec(dllexport) PFDAS_data_handle __cdecl PFDAS_handle_stream_tcp_create();
/****************************************************************************************************************/
/**
 * \brief Allocate one instance of a file client.
 *
 * This must be the first function a called when using the API. Multiple instances can be created.
 *
 * \return Pointer to file client instance.
 */
__declspec(dllexport) PFDAS_data_handle __cdecl PFDAS_handle_stream_file_create();
/****************************************************************************************************************/
/**
 * \brief Is the handle a file type stream.
 * \param h Handle allocated by PFDAS_handle_create().
 * \return One if is file, zero if tcp.
 */
__declspec(dllexport) int PFDAS_is_file(PFDAS_data_handle h);
/****************************************************************************************************************/
/**
 * \brief Obtain the file footer packet.
 *
 * This function is only meaningful in the reading of a file stream.
 *
 * \param h Handle allocated by PFDAS_handle_create().
 * \return PFDAS_footer_packet_t structure.
 */
__declspec(dllexport) struct PFDAS_footer_packet_t PFDAS_file_footer(PFDAS_data_handle h);
/****************************************************************************************************************/
/**
 * \brief Return the file header packet.
 *
 * User generated text regarding file record context.
 *
 * \return Header packet.
 */
__declspec(dllexport) struct PFDAS_header_packet_t PFDAS_file_header(PFDAS_data_handle h);
/****************************************************************************************************************/
/**
 * \brief Instructs the reader to continue reading after the initial read of the metadata.
 *
 * This should be called after "state eos".
 *
 * \param h Handle allocated by PFDAS_handle_create().
 */

__declspec(dllexport) void PFDAS_data_continue(PFDAS_data_handle h);
/****************************************************************************************************************/
/**
 * \brief Query if the end of file has been reached.
 *
 * This function is only meaningful in the reading of a file stream.
 *
 * \param h Handle allocated by PFDAS_handle_create().
 * \return 0 indicating eof has not been reached, 1 indicating eof has been reached.
 */
__declspec(dllexport) int PFDAS_file_eof_reached(PFDAS_data_handle h);
/****************************************************************************************************************/
/**
 * \brief Instruct the file reader to restart looping back to the beginning of the file.
 *
 * This function is only meaningful in the reading of a file stream.
 *
 * \param h Handle allocated by PFDAS_handle_create().
 */
__declspec(dllexport) void PFDAS_file_restart(PFDAS_data_handle h);
/****************************************************************************************************************/
/**
 * \brief Connect to a PFDAS hardware using the connect string.
 *
 * Connect string format for tcp stream is ip:port, where port is 1 of 56000, 56001, 56002 or 56003.
 * Connect string format for file stream is file path.
 * All four ports can be used simultaneously with restrictions. Each port is a single connection TCP/IP.
 * At sample rates below 50kHz all four can be used.
 * At sample rates below 100kHz three ports can be used.
 * At sample rates below 200kHz two ports can be used.
 * At or above 200kHz only one port can be used.
 * Exceeding this limits will result in data loss due to processor limitations.
 * 
 * \param connect_string String 
 * \param h Handle allocated by PFDAS_handle_create().
 */
__declspec(dllexport) void __cdecl PFDAS_connect(const char* connect_string, PFDAS_data_handle);
/****************************************************************************************************************/
/**
 * \brief Start the network client thread.
 *
 * This must be called after PFDAS_connect(...).
 * 
 * \param h Handle allocated by PFDAS_handle_create().
 * \return One if running, Zero if not running.
 */
__declspec(dllexport) void __cdecl PFDAS_start(PFDAS_data_handle h);
/****************************************************************************************************************/
/**
 * \brief Check if the network client thread is running.
 * 
 * \param h Handle allocated by PFDAS_handle_create().
 * \return One if running, Zero if not running.
 */
__declspec(dllexport) int __cdecl PFDAS_running(PFDAS_data_handle h);
/****************************************************************************************************************/
/**
 * \brief Stop the network client thread.
 * 
 * \param h Handle allocated by PFDAS_handle_create().
 * \return void
 */
__declspec(dllexport) void __cdecl PFDAS_stop(PFDAS_data_handle h);
/****************************************************************************************************************/
/**
 * \brief Release allocated memory from network client.
 * 
 * PFDAS_data_handle is no longer usaable.
 * 
 * \param h Handle allocated by PFDAS_handle_create().
 * \return void
 */
__declspec(dllexport) void __cdecl PFDAS_destroy(PFDAS_data_handle h);
/****************************************************************************************************************/
/**
 * \brief Query the data queue size.
 * 
 * This is intended to be used as metric to determine if the queue is being serviced fast enough.
 * It should not be used to determine if the queue is empty().
 * 
 * \param h Handle allocated by PFDAS_handle_create().
 * \return void
 */
__declspec(dllexport) int __cdecl PFDAS_data_queue_size(PFDAS_data_handle h);
/****************************************************************************************************************/
/**
 * \brief Check if the data queue has a packet to get.
 * 
 * This function should not used to determine if the queue is empty().
 * 
 * \param h Handle allocated by PFDAS_handle_create().
 * \return Number of packets in the data queue.
 */
__declspec(dllexport) int __cdecl PFDAS_data_has_pkt(PFDAS_data_handle h);
/****************************************************************************************************************/
/**
 * \brief Get one data packet from the network thread queue.
 * 
 * \param h Handle allocated by PFDAS_handle_create().
 * \return Pointer to PFDAS_base_packet_t.
 */
__declspec(dllexport) const __cdecl struct PFDAS_base_packet_t* PFDAS_data_get_pkt(PFDAS_data_handle h);
/****************************************************************************************************************/
/**
 * \brief Return one data packet that was obtaind by PFDAS_data_get_pkt(...).
 * 
 * Once this function is called the packet p no longer is valid to use.
 * 
 * \param p Pointer to PFDAS_base_packet_t previously obtained by PFDAS_data_get_pkt(...).
 * \return void
 */
__declspec(dllexport) void __cdecl PFDAS_data_return_pkt(const struct PFDAS_base_packet_t* p, PFDAS_data_handle h);
/****************************************************************************************************************/
/**
 * \brief Check if metadata queue has a packet to get.
 * 
 * This function should not used to determine if the queue is empty().
 * 
 * \param h Handle allocated by PFDAS_handle_create().
 * \return void
 */
__declspec(dllexport) int __cdecl PFDAS_meta_queue_size(PFDAS_data_handle h);
/****************************************************************************************************************/
/**
 * \brief Check if the metadata queue has a packet to get.
 * 
 * This function should not used to determine if the queue is empty().
 * 
 * \param h Handle allocated by PFDAS_handle_create().
 * \return Number of packets in the metadata queue.
 */
__declspec(dllexport) int __cdecl PFDAS_meta_has_pkt(PFDAS_data_handle h);
/****************************************************************************************************************/
/**
 * \brief Get one metadata packet from the network thread queue.
 * 
 * \param h Handle allocated by PFDAS_handle_create().
 * \return Pointer to PFDAS_base_packet_t which has a metadata type payload.
 */
__declspec(dllexport) const __cdecl struct PFDAS_base_packet_t* PFDAS_meta_get_pkt(PFDAS_data_handle);
/****************************************************************************************************************/
/**
 * \brief Return one data packet that was obtaind by PFDAS_meta_get_pkt(...).
 * 
 * Once this function is called the packet p no longer is valid to use.
 * 
 * \param p Pointer to PFDAS_base_packet_t previously obtained by PFDAS_meta_get_pkt(...).
 * \return void
 */
__declspec(dllexport) void __cdecl PFDAS_meta_return_pkt(const struct PFDAS_base_packet_t*, PFDAS_data_handle);
/****************************************************************************************************************/
/**
 * \brief Return the payload lcs command from the PFDAS_base_packet_t.
 * 
 * \param p Pointer to PFDAS_base_packet_t previously obtained by PFDAS_meta_get_pkt(...).
 * \return Pointer to the payload structure.
 */
__declspec(dllexport) const struct PFDAS_packet_payload_lcs_cmd_t* __cdecl PFDAS_meta_lcs_cmd_payload(const struct PFDAS_base_packet_t*);
/****************************************************************************************************************/
/**
 * \brief Return the payload phy2sys servo measurements from the PFDAS_base_packet_t.
 * 
 * \param p Pointer to PFDAS_base_packet_t previously obtained by PFDAS_meta_get_pkt(...).
 * \return Pointer to the payload structure.
 */
__declspec(dllexport) const struct PFDAS_packet_payload_phc2sys_t* __cdecl PFDAS_meta_phy2sys_payload(const struct PFDAS_base_packet_t*);
/****************************************************************************************************************/
/**
 * \brief Return the payload ptp4l servo measurements from the PFDAS_base_packet_t.
 * 
 * \param p Pointer to PFDAS_base_packet_t previously obtained by PFDAS_meta_get_pkt(...).
 * \return Pointer to the payload structure.
 */
__declspec(dllexport) const struct PFDAS_packet_payload_ptp4l_t* __cdecl PFDAS_meta_ptp4l_payload(const struct PFDAS_base_packet_t*);
/****************************************************************************************************************/
/**
 * \brief Return the payload pps_sync servo measurements from the PFDAS_base_packet_t.
 * 
 * This servo loop synchronizes the ethernet PPS to the FPGA's 10Mhz source clock PPS.
 * 
 * \param p Pointer to PFDAS_base_packet_t previously obtained by PFDAS_meta_get_pkt(...).
 * \return Pointer to the payload structure.
 */
__declspec(dllexport) const struct PFDAS_packet_payload_pps_sync_t* __cdecl PFDAS_meta_pps_sync_payload(const struct PFDAS_base_packet_t*);
/****************************************************************************************************************/
/**
 * \brief Return the payload system time stamp and status from the PFDAS_base_packet_t.
 *
 * Time stamp and status of current timing source.
 *
 * \param p Pointer to PFDAS_base_packet_t previously obtained by PFDAS_meta_get_pkt(...).
 * \return Pointer to the payload structure.
 */
__declspec(dllexport) const struct PFDAS_packet_payload_time_stamp_t* __cdecl PFDAS_meta_time_stamp_payload(const struct PFDAS_base_packet_t*);
/****************************************************************************************************************/
/**
 * \brief Return the payload event from the PFDAS_base_packet_t.
 *
 * User generated text with system id and channel id.
 *
 * \param p Pointer to PFDAS_base_packet_t previously obtained by PFDAS_meta_get_pkt(...).
 * \return Pointer to the payload structure.
 */
__declspec(dllexport) const struct PFDAS_packet_payload_event_t* __cdecl PFDAS_meta_event_payload(const struct PFDAS_base_packet_t*);
/****************************************************************************************************************/
/**
 * \brief Return the payload fpga payload from the PFDAS_base_packet_t.
 * 
 * The payload contains one channel of the FPGA formatted header and sample adc counts.
 * 
 * \param p Pointer to PFDAS_base_packet_t previously obtained by PFDAS_data_get_pkt(...).
 * \return Pointer to the payload structure.
 */
__declspec(dllexport) const uint8_t* __cdecl PFDAS_data_fpga_payload(const struct PFDAS_base_packet_t*);
/****************************************************************************************************************/
/**
 * \brief Description of the build information including data and environment.
 * 
 * \return Pointer to the null terminated text.
 */
__declspec(dllexport) const char* PFDAS_api_build_info();
/****************************************************************************************************************/
/**
 * \brief Description of the API's change log.
 * 
 * \return Pointer to the null terminated text.
 */
__declspec(dllexport) const char* PFDAS_api_changelog();
/****************************************************************************************************************/
/**
 * \brief The API's version string.
 * 
 * \return Pointer to the null terminated text.
 */
__declspec(dllexport) const char* PFDAS_api_version();
/****************************************************************************************************************/
/****************************************************************************************************************/
/****************************************************************************************************************/
/****************************************************************************************************************/

#ifdef __cplusplus
}
#endif
//
#endif
