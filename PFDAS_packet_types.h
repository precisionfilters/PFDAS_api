/* -------------------------------------------------------------------------- */
/* (C) Copyright 2023 Precision Filters Inc., Ithaca, NY.                     */
/* All Rights Reserved.                                                       */
/* --------------------------------------------------------------------------
 * -------------------------------------------------------------------------- */
#ifndef __PFDAS_PACKET_TYPES_H__
#define __PFDAS_PACKET_TYPES_H__
#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
typedef unsigned char byte;
enum PFDAS_PACKET_IDS {
    PFDAS_PACKET_BASE = 170,           /* Used to indicate the first byte in the packet, this is a constant set in PFDAS_base_packet_t::packet_type.                                  */
    PFDAS_PACKET_BASE_ACK = 1,         /* The optional data ack packet, it is send from client -> server, this is a constant set in PFDAS_base_packet_t::packet_type. */
    PFDAS_PACKET_AUTH = 8,             /* Packet type to authenticate client, this is set in Rdas_auth_packet_t::packet_type. Sent from client -> server.  */
    PFDAS_PACKET_FOOTER = 171,         /* Packet type to indicate EOF file or the footer which has key stats about the file contents. */
    PFDAS_PACKET_HEADER = 172,         /* Packet type to indicate EOF file or the footer which has key stats about the file contents. */
    /**/
    /* Real payloads set in Rdas_data_packet_t::payload_type from server -> client. */
    PFDAS_PAYLOAD_CH_DATA = 2,         /* The adc payload data, this comprises the bulk of what is sent. */
    PFDAS_PAYLOAD_PTP4L_STATS = 3,     /* ptp4l timing stats, sent 1/sec.                                */
    PFDAS_PAYLOAD_PHC2SYS_STATS = 4,   /* phyc2sys timing stats, sent 1/sec.                             */
    PFDAS_PAYLOAD_PPS_SYNC_STATS = 5,  /* pps-sync timing stats, sent 1/sec.                             */
    PFDAS_PAYLOAD_LCS_CMD = 6,         /* lcs command (text) packet, sent on connect and on any LCS command. */
    PFDAS_PAYLOAD_TIME_STAMPS = 11,
    PFDAS_PAYLOAD_RECORD_EVENT = 12,
    /**/
    /* Alternates  */
    PFDAS_MESSAGE_CONNECT_REJECT = 7,  /* Packet message to reject a connection, send from server to client. This is set in Rdas_data_packet_t::payload_type. */
    PFDAS_MESSAGE_AUTH_FAIL = 9,       /* Packet message to inform client authentication failed. This is set in Rdas_data_packet_t::payload_type. If Authentication did not fail then the client will not get disconnected. */
    PFDAS_STREAM_TYPE_DYNAMIC_1 = 99,  /* Unique this is the first byte sent (unsigned char) when the socket is connected. It indicates the stream type. */
    PFDAS_FILE_VERSION_1 = 100,  /* Unique this is the first byte sent (unsigned char) when the socket is connected. It indicates the stream type. */
};
/**/
//struct PFDAS_base_packet_t { /* 16 bytes */
struct PFDAS_base_packet_t { /* 16 bytes */
    byte packet_type;       /* This has to be the first element in the struct, the client code uses the first byte to confirm it is at the beginning of a packet. */
    union{
        byte payload_type;  /* The type of payload that follows this packet. */
        byte message;       /* Alternate, the message type. */
    };
    byte system_id;         /* The system identifier, not used but needs to be. */
    byte channel;       /* Channel number 0-15. */
    uint32_t payload_size;  /* Size of the payload packet that follows this packet. */
    uint64_t packet_id;     /* For CH_DATA this is the dma counter, combined with channel number it is unique. Otherwise this is a global packet counter. */
};
struct PFDAS_footer_packet_t { /* 128*8 bytes = 1024 */
    byte packet_type;          /* PFDAS_PACKET_FOOTER */
    byte padding[7];
    uint64_t file_start_time;
    uint64_t file_end_time;
    uint64_t sample_counts[16];    /* counts per channel */
    uint64_t file_size_bytes;
    uint64_t reserved[32];
    char keyword_value[256];
};
struct PFDAS_attachment_packet_t {
    byte packet_type;
    byte padding[3];
    char filename[256];
    int32_t file_data_size; // After this
};

struct PFDAS_header_packet_t { // Rename to header packet.
    byte packet_type;              /* PFDAS_PACKET_RECORD_INFORMATION */
    char keyword_value[4096];
    char keyword_value_files[1024];
};
struct PFDAS_packet_payload_event_t {
    uint32_t sec;
    uint32_t nsec;
    uint32_t event_id;
    char event[128];
};

/* CH_DATA payload is right after the struct of size payload_size. */
/* CH_DATA payload is not readily definable via struct.            */
/**/
struct PFDAS_packet_payload_ptp4l_t {
    uint32_t sec;
    uint32_t nsec;
    int32_t offset;
    int32_t freq;
    int32_t path_delay;
    int32_t unused;
};
struct PFDAS_packet_payload_phc2sys_t {
    uint32_t sec;
    uint32_t nsec;
    int32_t offset;
    int32_t freq;
    int32_t path_delay;
    int32_t unused;
};
struct PFDAS_packet_payload_pps_sync_t {
    uint32_t sec;
    uint32_t nsec;
    int32_t error;
    int32_t addend;
};
/* */
enum {
    PFDAS_PTP_STS_UNKNOWN = 0,
    PFDAS_PTP_STS_GOOD = 1,
    PFDAS_PTP_STS_REDETECTED_GM = 2,
    PFDAS_PTP_STS_OFF = 3,
    PFDAS_PTP_STS_NO_GM = 4,
    PFDAS_PTP_STS_RESTARING = 5,
};
enum {
    PFDAS_IRIG_STS_UNKNOWN = 0,
    PFDAS_IRIG_STS_GOOD = 1,
    PFDAS_IRIG_STS_NO_SIGNAL = 2, // Not an update everyt second (stuck).
    PFDAS_IRIG_STS_SIG_ERROR = 3, // DOY == 0
};
enum {
    PFDAS_PPS_EXT_STS_UNKNOWN = 0,
    PFDAS_PPS_EXT_STS_ABS_TIME_SET = 1,
    PFDAS_PPS_EXT_STS_ABS_TIME_NOT_SET = 2,
    PFDAS_PPS_EXT_STS_PPS_LOST = 3 // Determined by PPS_SYNC no longer updating at 1/sec.
};
enum {
    PFDAS_PPS_SRC_UNKNOWN = 0,
    PFDAS_PPS_SRC_PTP = 1,
    PFDAS_PPS_SRC_IRIG_AM = 2,
    PFDAS_PPS_SRC_IRIG_DCLS = 3,
    PFDAS_PPS_SRC_EXTERNAL_PPS = 4
};
typedef union {
   struct {
      unsigned pps_src:3;    // D[2:0] 1588, IRIGB, IRIG_DCLS, PPS
      unsigned ptp_sts:3; // D[3:5] PFDAS_PTP_STS enums.
      unsigned irig_sts:3;    // D[6:8] PFDAS_PTP_STS enums.
      unsigned pps_ext_sts:3; // D[9:11] PFDAS_PTP_STS enums.
      unsigned unused:20;     // D[12:31] unused
   } BIT_VAL;
   uint32_t UINT32_VAL;
} time_status_t;
struct PFDAS_packet_payload_time_stamp_t {
    time_status_t time_status; // Bit fields for status
    uint32_t sec_emac1;
    uint32_t sec_fpga;
    uint32_t sec_irig;
    uint32_t sec_linux;
};
/* */
struct PFDAS_packet_payload_lcs_cmd_t {
    uint32_t sec;
    uint32_t nsec;
    uint16_t text_size;
    /* payload is right after the struct of size "text_size". */
};
struct PFDAS_auth_packet_t {
    byte packet_type;      /* This has to be the first element in the struct, the client code uses the first byte to confirm it is at the beginning of a packet. */
    char user[256];
    char pass[256];
};
#ifdef __cplusplus
}
#endif
//
#endif
