/* -------------------------------------------------------------------------- */
/* (C) Copyright 2023 Precision Filters Inc., Ithaca, NY.                     */
/* All Rights Reserved.                                                       */
/* --------------------------------------------------------------------------
 * -------------------------------------------------------------------------- */
#ifndef __PFDAS_API_H__
#define __PFDAS_API_H__
#ifdef __cplusplus
extern "C" {
#endif
#include "PFDAS_packet_types.h"
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
__declspec(dllexport) PFDAS_data_handle __cdecl PFDAS_handle_create();
/****************************************************************************************************************/
/**
 * \brief Connect to a PFDAS hardware using the connect string.
 *
 * Connect string format is ip:port, where port is 1 of 56000, 56001, 56002 or 56003.
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
#ifdef __cplusplus
}
#endif
//
#endif
