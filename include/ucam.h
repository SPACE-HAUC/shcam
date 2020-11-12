/**
 * @file ucam3.h
 * @author Sunip K. Mukherjee (sunipkmukherjee@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2020-11-11
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#ifndef __UCAM_III_H
#define __UCAM_III_H

#include <stdio.h>

/** 
 * @brief Custom assert function to check if struct sizes are accurate.
 */
#define CASSERT(predicate) _impl_CASSERT_LINE(predicate, __LINE__)

#define _impl_PASTE(a, b) a##b
#define _impl_CASSERT_LINE(predicate, line) \
    typedef char _impl_PASTE(assertion_failed, line)[2 * !!(predicate)-1];

/// Ref: https://4dsystems.com.au/mwdownloads/download/link/id/420/ | page 8 of 23

#define UCAM_CONFIG_MAX_RETRY 11 // 10 retries

/**
 * @brief Enumeration of command ids for the UCAM III
 * 
 */
typedef enum
{
    UCAM_GET_DATA,    /// Get data packets
    UCAM_INIT = 0x01, /// p1 = 0x0, p2 = ucam_img_fmt, p3 = ucam_raw_res, p4 = ucam_jpg_res, responds with ack

    UCAM_GET_PIC = 0x04, /// p1 = ucam_pic_type, p2 = 0x0, p3 = 0x0, p4 = 0x0, responds with ack

    UCAM_SNAP = 0x05, /// p1 = ucam_snap_type, p2 = ucam_skip_fr_l, p3 = ucam_skip_fr_h, p4 = 0x0, responds with ack

    UCAM_SET_PACK_SZ = 0x06, /// p1 = 0x8, p2 = ucam_pack_sz_l, p3 = ucam_pack_sz_h, 0x0, responds with ack

    UCAM_SET_BAUD, /// p1 = ucam_baud_div1, p2 = ucam_baud_div2, p3 = 0x0, p4 = 0x0

    UCAM_RESET, /// p1 = ucam_reset_type (0x0 resets whole system, 0x1 resets state machine), responds with ack
                /// p2 = 0x0, p3 = 0x0, p4 = 0x0 or 0xff
                /// (for 0xff, the module responds immediately to the reset)

    UCAM_DATA = 0xa, /// p1 = ucam_data_type, p2 = ucam_data_len_0, p3 = ucam_data_len_1, p3 = ucam_data_len_2

    UCAM_SYNC = 0xd, /// p1 = 0x0, p2 = 0x0, p3 = 0x0, p4 = 0x0

    UCAM_ACK = 0xe, /// p1 = ucam_cmd_set, p2 = ack_ctr, p3 = 0x0 or pkg_id_0, p4 = 0x0 or pkg_id_1

    UCAM_NAC = 0xf, /// p1 = 0x0, p2 = nak_ctr, p3 = ucam_errno, p4 = 0x0

    UCAM_LIGHT = 0x13, /// p1 = ucam_freq_type, p2 = 0x0, p3 = 0x0, p4 = 0x0
                       /// ucam_freq_type = 0x0 => 50 Hz hum minimization
                       /// ucam_freq_type = 0x1 => 60 Hz hum minimization

    UCAM_CBE, /// p1 = contrast (0--4, 2 is nominal)
              /// p1 = brightness (0--4, 2 is nominal)
              /// p3 = exposure (0--4, 2 is `0', 0x0 is -2, 0x4 is +2)
              /// p4 = 0x0

    UCAM_SLEEP, /// p1 = timeout (0--255 s), p2 = 0x0, p3 = 0x0, p4 = 0x0
} ucam_cmd_set;

/**
 * @brief Data type representing a single UCAM III command
 * 
 */
typedef union
{
    struct __attribute__((packed))
    {
        unsigned char hdr; /// always 0xaa
        unsigned char cmd; /// belongs to ucam_cmd_set
        unsigned char p1;  /// parameter 1
        unsigned char p2;  /// parameter 2
        unsigned char p3;  /// parameter 3
        unsigned char p4;  /// parameter 4
    };
    unsigned char bytes[6];
} ucam_cmd;

#ifdef UCAM_DEBUG
CASSERT(sizeof(ucam_cmd) == 6);
#endif // UCAM_DEBUG

typedef enum
{
    GRAY8 = 0x3,          /// Raw, 8-bit for Y only
    RAW_COL_CRYCBY = 0x8, /// Raw, 16 bit color, YCbCr data with bytes reversed, 4:2:2 chroma subsample
    RAW_COL_RGB = 0x6,    /// Raw, 16 bit color, 565 RGB (5 bits for red, 6 bits for green, 5 bits for blue)
    COL_JPEG = 0x7,       /// Compressed JPEG
} ucam_img_fmt;

typedef enum
{
    UCAM_RAW_W80H60 = 0x1,   /// 80 x 60 pixels raw image
    UCAM_RAW_W160H120 = 0x3, /// 160 x 120 pixels raw image
    UCAM_RAW_W128H128 = 0x9, /// 128 x 128 pixels raw image
    UCAM_RAW_W128H96 = 0xb,  /// 128 x 96 pixels raw image
} ucam_raw_res;

typedef enum
{
    UCAM_JPG_128p = 0x3, /// 160 x 128 JPEG image
    UCAM_JPG_240p = 0x5, /// 320 x 240 JPEG image
    UCAM_JPG_480p = 0x7, /// 640 x 480 JPEG image
} ucam_jpg_res;

typedef enum
{
    UCAM_SNAPSHOT = 0x1, /// snapshot picture mode
    UCAM_RAW = 0x2,      /// raw picture mode
    UCAM_JPG = 0x5,      /// jpeg picture mode
} ucam_pic_type;

/**
 * @brief The uCAM-III will hold a single frame of still picture data in its buffer
 * after receiving this command. This snapshot can then be retrieved from the 
 * buffer multiple times if required.
 * 
 */
typedef enum
{
    UCAM_SNAP_JPG = 0x0,
    UCAM_SNAP_RAW = 0x1,
} ucam_snap_type;

/**
 * @brief The number of dropped frames can be defined before capture occurs. 
 * “0” keeps the current frame, “1” captures the next frame, and so on.
 * 
 */
typedef union
{
    struct __attribute__((packed))
    {
        unsigned char ucam_skip_fr_l;
        unsigned char ucam_skip_fr_h;
    };
    unsigned short ucam_skip_fr_d;
} ucam_skip_fr;
#ifdef UCAM_DEBUG
CASSERT(sizeof(ucam_skip_fr) == 2);
#endif

/**
 * @brief The host issues package size command to change the size of the data 
 * package which is used to transmit the compressed JPEG image data from the 
 * uCAM-III to the host. This command should be issued before sending SNAPSHOT 
 * or GET PICTURE commands to the uCAM-III.
 * 
 * Note: The size of the last package varies for different JPEG image sizes.
 * 
 * The default size is 64 bytes and the maximum size is 512 bytes.
 * 
 * Package format:
 * 
 * +----------+-----------+------------------------+---------------+
 * |    ID    |  Data Sz  |       Image Data       |  Verify Code  |
 * | 2 bytes  |  2 bytes  | (package sz - 6 bytes) |    2 bytes    |
 * +----------+-----------+------------------------+---------------+
 * 
 * ID: Package ID, starts from 1 for an image
 * Data size: Size of image data in the package
 * Verify Code: Error detection code, equals to the lower byte of the sum
 * of the whole package data except the verify code field. The higher byte
 * of this code is always zero, i.e. verify code = lowbyte(sum(byte[0] -> byte[n-2]))
 * 
 * Note 1:Once the host receives the image size from the uCAM-III, the following 
 * simple equation can be used to calculate the number of packages that will be 
 * received according to the package size set. The package settings only apply for 
 * compressed JPEG images.
 * 
 * Number of packages = Image size / (Package size – 6)
 * 
 * Note 2: As the transmission of an uncompressed (RAW) image does not require the 
 * package mode, it is not necessary to set the package size for an uncompressed 
 * image. All of the pixel data for the RAW image will be sent continuously until 
 * completion.
 */
typedef union
{
    struct __attribute__((packed))
    {
        unsigned char ucam_pkg_sz_l;
        unsigned char ucam_pkg_sz_h;
    };
    unsigned short ucam_pkg_sz_d;
} ucam_pkg_sz;
#ifdef UCAM_DEBUG
CASSERT(sizeof(ucam_pkg_sz) == 2);
#endif

/**
 * @brief Baud rates available for the UCAM III.
 * 
 * Note: 3686400 baud is not achievable using the 4D programming cable or the PA5 
 * due to the USB to Serial IC’s used. To utilise this high speed baud rate, please 
 * check your serial port/device can handle this baud rate.
 * 
 * Note 2: The following baud rates are natively supported by the camera:
 *     9600
 *     14400
 *     56000
 *     57600
 *     115200
 *     921600 
 */
typedef enum
{
    UCAM_B2400,
    UCAM_B4800,
    UCAM_B9600,
    UCAM_B19200,
    UCAM_B38400,
    UCAM_B57600,
    UCAM_B115200,
    UCAM_B153600,
    UCAM_B230400,
    UCAM_B460800,
    UCAM_B921600,
    UCAM_B1228800,
    UCAM_B1843200,
    UCAM_B3686400
} ucam_baud;

/**
 * @brief Lower byte of the baud rate divider
 * 
 */
extern unsigned char ucam_baud_div1[];
/**
 * @brief Upper byte of the baud rate divider
 * 
 */
extern unsigned char ucam_baud_div2[];

typedef enum
{
    UCAM_PIC_TYPE_ERR = 0x1,
    UCAM_PIC_UPSCALE_ERR,
    UCAM_PIC_SCALE_ERR,
    UCAM_UNEXPECTED_RPLY,
    UCAM_SEND_PIC_TIMEOUT,
    UCAM_UNEXPECTED_CMD,
    UCAM_SRAM_JPG_TYPE_ERR,
    UCAM_SRAM_JPG_SZ_ERR,
    UCAM_PIC_FMT_ERR,
    UCAM_PIC_SZ_ERR,
    UCAM_PARAM_ERR,
    UCAM_SEND_REG_TIMEOUT,
    UCAM_CMD_ID_ERR,
    UCAM_PIC_NOT_RDY = 0xf,
    UCAM_XFER_PKG_NUM_ERR,
    UCAM_SET_XFER_PKG_SZ_ERR,
    UCAM_MAX_TRIES_EXCEED,
    UCAM_INVALID_CMD,
    UCAM_CMD_HDR_ERR = 0xf0,
    UCAM_CMD_LEN_ERR,
    UCAM_SEND_PIC_ERR = 0xf5,
    UCAM_SEND_CMD_ERR = 0xff,
} ucam_errno;

/**
 * @brief This structure contains the serial interface, reset GPIO pin and 
 * operational parameters of the UCAM-III camera.
 * 
 */
typedef struct
{
    int fd;                     /// Serial port descriptor
    int rst;                    /// Reset pin associated with the device (of type gpiodev_lut_pin index)
    int baud;                   /// baud rate
    unsigned char img_fmt;      /// ucam_img_fmt
    unsigned char raw_res;      /// ucam_raw_res
    unsigned char jpg_res;      /// ucam_jpg_res
    unsigned char pic_mode;     /// ucam_pic_type
    unsigned char snap_type;    /// ucam_snap_type
    unsigned short pkg_sz;      /// ucam_pkg_sz
    unsigned short skip_frames; /// ucam_skip_fr
    char sync;                  /// whether we have sync
    unsigned char contrast;     /// contrast (0--4, 2 is nominal)
    unsigned char brightness;   /// brightness (0--4, 2 is nominal)
    unsigned char exposure;     /// exposure (0--4, goes -2 to 2)
    unsigned char light;        /// 0x0 => 50 Hz hum, 0x1 => 60 Hz hum
} ucam;
const int x = sizeof(ucam);
/**
 * @brief Initialize serial port connection to an UCAM-III camera at serial port
 * specified.
 * 
 * @param dev ucam struct where serial port is opened
 * @param fname File name of serial port
 * @param baud Baud rate of serial port
 * @param rst Reset GPIO pin (-1 for unused)
 * @return int non-negative on success, negative on error
 */
int ucam_init(ucam *dev, const char *fname, int baud, int rst);
/**
 * @brief Synchronize the UCAM. Must be performed following a power up.
 * 
 * @param dev ucam device descriptor
 * @return int Non-negative on success, negative on error
 */
int ucam_sync(ucam *dev);
/**
 * @brief Configure the UCAM device with the parameters specified in the device struct
 * 
 * @param dev ucam device descriptor
 * @param cmd Command that is being executed (of type ucam_cmd_set)
 * @return int Non-negative on success, negative on error
 */
int ucam_config(ucam *dev, unsigned char cmd);
/**
 * @brief Snap a picture of the type specified in the device config
 * 
 * @param dev ucam device descriptor
 * @param len length of snapped image stored in this variable
 * @return int length of the snapped image
 */
int ucam_snap_picture(ucam *dev, ssize_t *len);
/**
 * @brief Get data after snapping the picture
 * 
 * @param dev ucam device descriptor
 * @param data Pointer to where image data will be stored. Memory has to be allocated by the caller.
 * @param len Length of image data to obtain
 * @param err_check Enable error checking for JPEG data
 * @return int length on success
 */
int ucam_get_data(ucam *dev, unsigned char *data, ssize_t len, unsigned char err_check);
/**
 * @brief Soft reset the camera.
 * 
 * @param dev ucam device descriptor
 * @param rst_type 0x0 to reset the whole system, 0x1 to reset state machine
 * @return int Non-negative on success, negative on error
 */
int ucam_soft_rst(ucam *dev, unsigned char rst_type);
/**
 * @brief Hard reset the camera.
 * 
 * @param dev ucam device descriptor
 * @return int Non-negative on success, negative on error
 */
int ucam_hard_rst(ucam *dev);
/**
 * @brief Hard reset and close the serial port. Note that memory for dev is not
 * freed, and it is up to the caller to perform relevant memory management.
 * 
 * @param dev ucam device descriptor
 */
void ucam_destroy(ucam *dev);

#endif // __UCAM_III_H