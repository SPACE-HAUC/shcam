/**
 * @file ucam3.c
 * @author Sunip K. Mukherjee (sunipkmukherjee@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2020-11-11
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#include <string.h>
#include <ucam.h>
#include <termios.h>
#include <unistd.h>
#include <shserial/shserial.h>
#include <gpiodev/gpiodev.h>

unsigned char ucam_baud_div1[] = {
    0x1f,
    0x1f,
    0x1f,
    0x1f,
    0x1f,
    0x1f,
    0x1f,
    0x7,
    0x7,
    0x7,
    0x1,
    0x2,
    0x1,
    0x0,
};
unsigned char ucam_baud_div2[] = {
    0x2f,
    0x17,
    0x0b,
    0x5,
    0x2,
    0x1,
    0x0,
    0x2,
    0x1,
    0x0,
    0x1,
    0x0,
    0x0,
    0x0,
};

static unsigned char UCAM_SYNC_CMD[] = {0xaa, UCAM_SYNC, 0x0, 0x0, 0x0, 0x0};
static unsigned char UCAM_SYNC_ACK[] = {0xaa, UCAM_ACK, UCAM_SYNC, 0x0, 0x0, 0x0};

int ucam_init(ucam *dev, const char *fname, int baud, int rst)
{
    int status = 0;
    // first check that the baud rate is supported
    switch (baud)
    {
    case B9600:
        dev->baud = UCAM_B9600;
        break;
    case B57600:
        dev->baud = UCAM_B57600;
        break;
    case B115200:
        dev->baud = UCAM_B115200;
        break;
    case B921600:
        dev->baud = UCAM_B921600;
        break;
    default:
        fprintf(stderr, "%s: Baud rate %d note supported, exiting...\n", __func__, baud);
        status = -1;
        break;
    }
    if (status < 0)
        return status;
    // Open the device
    if ((dev->fd = setup_serial(fname, baud, 0, 0)) < 0)
    {
        fprintf(stderr, "%s: Setup serial error %d, exiting...\n", __func__, dev->fd);
        return dev->fd;
    }
    // set up additional attributes
    struct termios tty;
    tcgetattr(dev->fd, &tty);
    tty.c_cflag = baud | CS8 | CLOCAL | CREAD;
    tty.c_iflag = IGNPAR;
    tty.c_oflag = 0;
    tty.c_iflag = 0;
    tcflush(dev->fd, TCIFLUSH);
    tcsetattr(dev->fd, TCSANOW, &tty);
    // Set up GPIO
    dev->rst = rst;
    dev->sync = 0;     // indicate lack of sync
    dev->pkg_sz = 512; // default package size
    if (rst > 0)
    {
        if (gpioSetMode(rst, GPIO_OUT) < 0)
        {
            fprintf(stderr, "%s: GPIO set mode error, exiting...\n", __func__);
            return -1;
        }
        gpioWrite(rst, GPIO_HIGH); // push the pin high as chip is reset on the negative edge
    }
    return 1;
}

int ucam_sync(ucam *dev)
{
    if (ucam_hard_rst(dev) < 0)
    {
        return -1;
    }
    // wait 1 second
    usleep(1 * 1000000);
    // start working on synchronization
    unsigned long slp = 5000; // 5 ms
    int count = 0;
    char inbuf[6];
    for (int i = 0; i < 60; i++)
    {
        count = write(dev->fd, UCAM_SYNC_CMD, 6);
        if (count < 0)
        {
            fprintf(stderr, "%s: Failed to write to stream\n", __func__);
            return -1;
        }
        count = read(dev->fd, (void *)inbuf, 6);
        if (count < 0)
        {
            fprintf(stderr, "%s: Failed to read from stream\n", __func__);
            return -1;
        }
#ifdef UCAM_DEBUG
        fprintf(stderr, "%s: Try %d: Received ", __func__, i + 1);
        for (int j = 0; j < 6; j++)
            fprintf(stderr, "0x%02x ", inbuf[j]);
        fprintf(stderr, "\n");
#endif

        if (inbuf[0] == UCAM_SYNC_ACK[0] && inbuf[1] == UCAM_SYNC_ACK[1] && inbuf[2] == UCAM_SYNC_ACK[2]) // we have ack
        {
#ifdef UCAM_DEBUG
            fprintf(stderr, "%s: Ack received, checking for sync\n", __func__);
#endif
            count = read(dev->fd, (void *)inbuf, 6);
            if (count < 0)
            {
                fprintf(stderr, "%s: Sync read error, returning...\n", __func__);
                return -1;
            }
            if (inbuf[0] == 0xaa && inbuf[1] == UCAM_SYNC) // sync received
            {
#ifdef UCAM_DEBUG
                fprintf(stderr, "%s: Sync received, sending ack\n", __func__);
#endif
                count = write(dev->fd, UCAM_SYNC_ACK, 6);
                if (count < 0)
                {
                    fprintf(stderr, "%s: Error sending ack after receiving sync\n", __func__);
                    return -1;
                }
                else
                { // successful sync
#ifdef UCAM_DEBUG
                    fprintf(stderr, "%s: Sync success\n", __func__);
#endif
                    i = 60;              // break loop
                    dev->sync = 1;       // indicate sync achieved
                    usleep(2 * 1000000); // sleep 2 seconds for settling down
                    return 1;
                }
            }
        }
        usleep(slp);
        slp += 1000; // add 1 ms for next iteration
    }
    return -1;
}

static int ucam_cmd_with_ack(ucam *dev, unsigned char cmd, unsigned char p1, unsigned char p2, unsigned char p3, unsigned char p4);
static int ucam_cmd_without_ack(ucam *dev, unsigned char cmd, unsigned char p1, unsigned char p2, unsigned char p3, unsigned char p4);

int ucam_config(ucam *dev, unsigned char cmd)
{
    switch (cmd)
    {
    case UCAM_INIT:
        return ucam_cmd_with_ack(dev, cmd, 0x0, dev->img_fmt, dev->raw_res, dev->jpg_res);
        break;
    case UCAM_SET_PACK_SZ:
        return ucam_cmd_with_ack(dev, cmd, 0x8, dev->pkg_sz, (dev->pkg_sz) >> 8, 0x0);
        break;
    case UCAM_RESET:
        return ucam_cmd_with_ack(dev, cmd, 0x1, 0x0, 0x0, 0x0);
        break;
    case UCAM_LIGHT:
        return ucam_cmd_with_ack(dev, cmd, dev->light & 0x1, 0x0, 0x0, 0x0);
        break;
    case UCAM_CBE:
        return ucam_cmd_with_ack(dev, cmd, dev->contrast, dev->brightness, dev->exposure, 0x0);
        break;
    default:
        return -UCAM_INVALID_CMD;
        break;
    }
}

int ucam_snap_picture(ucam *dev, ssize_t *len)
{
    /**
     * Order of operation:
     * 
     * 1. Snapshot command with dev->pic_type
     * 2. Receive ACK
     * 3. Get picture snapshot (0x1)
     * 4. Receive ACK
     * 5. Receive DATA (0xaa 0xa 0x1 24-bit image size)
     * 6. Send ACK (considered to contain packet ID from this point, start with 0x0 0x0)
     * 7. Receive first data package (starts counting at 1)
     * 8. ACK receiving of first data package 
     * .
     * .
     * .
     * 9. ACK receiving last data package using f0 f0 as ACK payload
     */
    int status = 0;
    if ((status = ucam_cmd_with_ack(dev, UCAM_SNAP, dev->pic_mode, (dev->skip_frames) & 0xff, (dev->skip_frames >> 8) & 0xff, 0x0)) < 0)
    {
        fprintf(stderr, "%s: Error getting a snapshot\n", __func__);
        return status;
    }
    // int mode = 0x5; // default = jpeg
    // if (dev->pic_mode == 0x0)
    //     mode = 0x5;
    // else if (dev->pic_mode == 0x1)
    //     mode = 0x2;
    if ((status = ucam_cmd_with_ack(dev, UCAM_GET_PIC, 0x1, 0x0, 0x0, 0x0)) < 0)
    {
        fprintf(stderr, "%s: Error getting a picture\n", __func__);
        return status;
    }
    int cond = 0;
    int counter = 0;
    unsigned char inbuf[6];
    do
    {
        counter++;
        int count = read(dev->fd, inbuf, 6);
        if (count < 6)
            continue;
        cond = (inbuf[0] == 0xaa) && (inbuf[1] == 0xa) && (inbuf[2] == 0x1);
    } while (counter < UCAM_MAX_TRIES_EXCEED || !cond);
    if (counter >= UCAM_CONFIG_MAX_RETRY)
        return -UCAM_MAX_TRIES_EXCEED;
    // now we know the package size
    int num_bytes = inbuf[5];
    num_bytes <<= 8;
    num_bytes |= inbuf[4];
    num_bytes <<= 8;
    num_bytes |= inbuf[3];
    *len = num_bytes;
    return num_bytes;
}

int ucam_get_data(ucam *dev, unsigned char *data, ssize_t len, unsigned char err_check)
{
    // send the acknowledgement to start receiving data
    if (data == NULL)
        return -1;
    int status = 0;
    int counter = 0;
    do
    {
        status = ucam_cmd_without_ack(dev, UCAM_ACK, 0x0, 0x0, 0x0, 0x0);
    } while (status != 1 || counter < UCAM_MAX_TRIES_EXCEED);
    if (dev->pic_mode == 0x0) // JPEG
    {
        int rcvd = 0, count = 0;
        while (rcvd < len) // still not received full image
        {
            // receive first four bytes
            char tmpbuf[4];
            while ((count = read(dev->fd, tmpbuf, 4)) != 4)
                ;
#ifdef UCAM_DEBUG
            fprintf(stderr, "%s, %d: 0x%02x 0x%02x 0x%02x 0x%02x\n", __func__, __LINE__, tmpbuf[0], tmpbuf[1], tmpbuf[2], tmpbuf[3]);
#endif
            // calculate size of data to be received
            ssize_t size = (ssize_t)tmpbuf[2] | ((ssize_t)tmpbuf[3]) << 8;
            while ((count = read(dev->fd, &(data[rcvd]), size)) != size)
                ; // receive the data
#ifdef UCAM_DEBUG
            fprintf(stderr, "%s, %d: 0x%02x 0x%02x 0x%02x 0x%02x\n", __func__, __LINE__, data[rcvd], data[rcvd + 1], data[rcvd + 2], data[rcvd + 3]);
#endif
            char ecc[2];
            while ((count = read(dev->fd, ecc, 2)) != 2)
                ; // receive the verify code
#ifdef UCAM_DEBUG
            fprintf(stderr, "%s, %d: 0x%02x 0x%02x\n", __func__, __LINE__, ecc[0], ecc[1]);
#endif
            if (err_check)
            {
                unsigned char rcvd_ecc = tmpbuf[0] + tmpbuf[1] + tmpbuf[2] + tmpbuf[3];
                for (int i = 0; i < size; i++)
                    rcvd_ecc += data[rcvd + i];
                fprintf(stderr, "%s: Received checksum = 0x%02x, Calculated checksum = 0x%02x, Checksum %s\n", __func__, ecc[1], rcvd_ecc, rcvd_ecc == ecc[1] ? "PASS" : "FAIL");
            }
            rcvd += size; // increment number of received bytes
            // send ack
            if (rcvd < len)
            {
                char cmdbuf[] = {0xaa, UCAM_ACK, 0x0, 0x0, tmpbuf[0], tmpbuf[1]};
                while ((count = write(dev->fd, cmdbuf, 6)) != 6)
                    ;
#ifdef UCAM_DEBUG
                fprintf(stderr, "%s, %d: Sent ACK for package 0x%02x%02x\n", __func__, __LINE__, tmpbuf[0], tmpbuf[1]);
#endif
            }
            else
            {
                char cmdbuf[] = {0xaa, UCAM_ACK, 0x0, 0x0, 0xf0, 0xf0};
                while ((count = write(dev->fd, cmdbuf, 6)) != 6)
                    ;
#ifdef UCAM_DEBUG
                fprintf(stderr, "%s, %d: Sent ACK for package 0x%02x%02x\n", __func__, __LINE__, tmpbuf[0], tmpbuf[1]);
#endif
            }
        }
        return len;
    }
    else if (dev->pic_mode == 0x1) // RAW
    {
        int count = 0;
        do
        {
            if (counter++ > 1)
                usleep(100000); // give time for data to be available
            count = read(dev->fd, data, len);
        } while (count != len || counter < UCAM_CONFIG_MAX_RETRY);
        if (counter >= UCAM_CONFIG_MAX_RETRY)
            return -UCAM_MAX_TRIES_EXCEED;
        if (ucam_cmd_without_ack(dev, UCAM_ACK, UCAM_DATA, 0x0, 0x1, 0x0) > 0)
            return len;
    }
    return 0; // will never reach this point
}

int ucam_soft_rst(ucam *dev, unsigned char rst_type)
{
    return ucam_cmd_with_ack(dev, UCAM_RESET, rst_type & 0x1, 0x0, 0x0, 0x0);
}

int ucam_hard_rst(ucam *dev)
{
    if (dev->rst < 0)
        return ucam_cmd_with_ack(dev, UCAM_RESET, 0x0, 0x0, 0x0, 0x0);
    else
    {
        gpioWrite(dev->rst, GPIO_LOW);
        usleep(10000);
        gpioWrite(dev->rst, GPIO_HIGH);
    }
    return 1;
}

void ucam_destroy(ucam *dev)
{
    close(dev->fd);
}

static int ucam_cmd_with_ack(ucam *dev, unsigned char cmd, unsigned char p1, unsigned char p2, unsigned char p3, unsigned char p4)
{
    unsigned char inbuf[6];
    unsigned char cmd_buf[6] = {0xaa, cmd, p1, p2, p3, p4};
    int count = 0;
    // initialize camera with given settings
    int counter = 0;
    int cond = 0;
    do
    {
        counter++;             // set maximum number of tries
        memset(inbuf, 0x0, 6); // clear out
        count = write(dev->fd, cmd_buf, 6);
        if (count < 6)
            continue;                    // try to write again if write failed
        count = read(dev->fd, inbuf, 6); // read 6 bytes
        if (count < 6)                   // read failed
            continue;
        // if we reach here we shall evaluate the condition
        cond = (inbuf[0] == 0xaa) && (inbuf[1] == UCAM_ACK) && (inbuf[2] == cmd);
        if (inbuf[1] == UCAM_NAC && inbuf[0] == 0xaa)
        {
            fprintf(stderr, "%s: NAC received while trying to send command 0x%02x with error code 0x%02x\n", __func__, cmd, inbuf[4]);
            return inbuf[4] > 0 ? -inbuf[4] : inbuf[4]; // errors are always negative
        }
    } while (counter < UCAM_CONFIG_MAX_RETRY || !cond); // check on inbuf and check if counter has exceeded
    if (counter >= UCAM_CONFIG_MAX_RETRY)
        return -UCAM_MAX_TRIES_EXCEED;
    return 1;
}

static int ucam_cmd_without_ack(ucam *dev, unsigned char cmd, unsigned char p1, unsigned char p2, unsigned char p3, unsigned char p4)
{
    unsigned char cmd_buf[6] = {0xaa, cmd, p1, p2, p3, p4};
    int count = 0;
    // initialize camera with given settings
    int counter = 0;
    do
    {
        counter++; // set maximum number of tries
        count = write(dev->fd, cmd_buf, 6);
        if (count < 6)
            continue;                          // try to write again if write failed
    } while (counter < UCAM_CONFIG_MAX_RETRY); // check on inbuf and check if counter has exceeded
    if (counter >= UCAM_CONFIG_MAX_RETRY)
        return -UCAM_MAX_TRIES_EXCEED;
    return 1;
}

#ifdef UNIT_TEST
int main()
{
    ucam cam;
    ucam_init(&cam, "/dev/ttyS0", B115200, -1);
    ucam_destroy(&cam);
    return 0;
}
#endif