#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

#ifndef RTAPI
    #define RTAPI
#endif

#include "rtapi.h"
#include "rtapi_app.h"
#include "hal.h"




MODULE_AUTHOR("MX_Master");
MODULE_DESCRIPTION("CH32 Ethernet controller driver");
MODULE_LICENSE("GPL");




static int32_t comp_id;
static const uint8_t * comp_name = "CH32";

static char *IP = "10.10.10.10";
RTAPI_MP_STRING(IP, "IP addresses list, comma separated");

static char *PORT = "1000";
RTAPI_MP_STRING(PORT, "UDP ports list, comma separated");

static char *SIMULATION = "0";
RTAPI_MP_STRING(SIMULATION, "Enable simulation");

static int sock;
static char buf[512];
static uint32_t simulation = 0;




#define DEVICES_MAX_CNT 8
#define STEPDIR_CH_CNT 4
#define ENCODER_CH_CNT 4
#define GPIO_PORTS_CNT 5
#define GPIO_PINS_CNT 16
#define ETH_ANSWER_TIMEOUT_S 0 // seconds, max time to wait an answer
#define ETH_ANSWER_TIMEOUT_US 5000 // microseconds, max time to wait an answer
#define MSG_MAX_CNT 4

typedef struct
{
  uint8_t used;
  uint8_t IP[4];
  uint16_t port;
  struct sockaddr_in sock_addr;
  socklen_t sock_addr_size;
}
dev_cfg_t;

typedef struct
{
  hal_bit_t *connected; // out
}
dev_hal_t;

static dev_cfg_t dev[DEVICES_MAX_CNT] = {0};
static uint32_t dev_cnt = 0;
static dev_hal_t *devh;




typedef struct
{
  hal_u32_t *id;
  hal_bit_t *in;
  hal_bit_t *in_not;
  hal_bit_t *out;
  hal_bit_t *out_not;
  hal_s32_t *pull;
  hal_s32_t *type;
}
gpio_hal_t;

typedef struct
{
  hal_bit_t out;
  hal_bit_t out_not;
  hal_s32_t pull;
  hal_s32_t type;
}
gpio_param_t;

static gpio_hal_t *gph[DEVICES_MAX_CNT][GPIO_PORTS_CNT];
static gpio_param_t gpp[DEVICES_MAX_CNT][GPIO_PORTS_CNT][GPIO_PINS_CNT] = {{{0}}};




typedef struct
{
    hal_bit_t *enable; // in
    hal_u32_t *dir_offset; // in
    hal_u32_t *dir_pin; // in
    hal_u32_t *step_pin; // in
    hal_u32_t *step_len; // in
    hal_float_t *max_speed; // in
    hal_float_t *max_accel; // in
    hal_float_t *pos_scale; // in
    hal_float_t *pos_cmd; // in
    hal_float_t *pos_fb; // out
    hal_bit_t *pos_reset; // in
}
stepdir_ch_hal_t;

typedef struct
{
    hal_bit_t enable; // in
    hal_u32_t dir_offset; // in
    hal_u32_t dir_pin; // in
    hal_u32_t step_pin; // in
    hal_u32_t step_len; // in
    hal_float_t max_speed; // in
    hal_float_t max_accel; // in
    hal_float_t pos_scale; // in
    hal_float_t pos_cmd; // in
    hal_bit_t state;
}
stepdir_ch_param_t;

static stepdir_ch_hal_t *sdh[DEVICES_MAX_CNT];
static stepdir_ch_param_t sdp[DEVICES_MAX_CNT][STEPDIR_CH_CNT] = {{0}};




typedef struct
{
    hal_bit_t *enable; // in
    hal_bit_t *reset; // in
    hal_u32_t *A_pin; // in
    hal_u32_t *B_pin; // in
    hal_u32_t *Z_pin; // in
    hal_bit_t *find_Z; // io
    hal_float_t *pos_scale; // in
    hal_float_t *pos_fb; // out
}
encoder_ch_hal_t;

typedef struct
{
    hal_bit_t enable; // in
    hal_u32_t A_pin; // in
    hal_u32_t B_pin; // in
    hal_u32_t Z_pin; // in
    hal_bit_t find_Z; // io
    hal_bit_t find_Z_real;
    hal_float_t pos_scale; // in
}
encoder_ch_param_t;

static encoder_ch_hal_t *eh[DEVICES_MAX_CNT];
static encoder_ch_param_t ep[DEVICES_MAX_CNT][ENCODER_CH_CNT] = {{0}};




static void comp_read(void *arg, long period)
{
    static uint32_t d, state, port, pin, ch;
    static char *data, *c;
    static hal_float_t f;
    
    // send a request
    if (!simulation)
        for (d = dev_cnt; d--;)
            sendto(sock, "get", 4, 0, (struct sockaddr*)&dev[d].sock_addr, sizeof(dev[d].sock_addr));

    // wait for answer and do something with this answer
    for (d = dev_cnt; d--;)
    {
        // wait for answer
        buf[0] = 0;
        if (!simulation)
            recvfrom(sock, buf, sizeof(buf) - 1, 0, (struct sockaddr*)&dev[d].sock_addr, &dev[d].sock_addr_size);
        // or get a fake answer
        else
        {
            data = buf;
            // add to the answer fake gpio state
            strcpy(data, "gpio:"); 
            data += 5;
            for (port = 0; port < GPIO_PORTS_CNT; port++)
            {
                *data++ = 'A' + port;
                for (pin = 0; pin < GPIO_PINS_CNT; pin++, data++) *data = '0';
                if (port < GPIO_PORTS_CNT-1) *data++ = ',';
            }
            // add to the answer fake stepdir positions
            strcpy(data, " stepdir:"); 
            data += 9;
            for (ch = 0; ch < STEPDIR_CH_CNT; ch++)
            {
                sprintf(data, "%f|", *sdh[d][ch].pos_cmd);
                data = memchr(data, 0, sizeof(buf));
                if (ch < STEPDIR_CH_CNT-1) *data++ = ',';
            }
            // add to the answer fake encoder positions
            strcpy(data, " encoder:"); 
            data += 9;
            for (ch = 0; ch < ENCODER_CH_CNT; ch++)
            {
                sprintf(data, "%f|", 0.0);
                data = memchr(data, 0, sizeof(buf));
                if (ch < ENCODER_CH_CNT-1) *data++ = ',';
            }
            // add `end of the string`
            data = 0;
        }

        // if no answer received
        state = (buf[0] == 0) ? 0 : 1;
        *devh[d].connected = state;
        if (!state) continue;

        // parse answer for gpio state
        if (data = strstr(buf, "gpio:"))
        {
            for (port = 0; port < GPIO_PORTS_CNT; port++)
            {
                if (c = strchr(data, 'A' + port))
                {
                    for (data = c+1, pin = 0; pin < GPIO_PINS_CNT; pin++, data++)
                    {
                        if (*data == '1') state = 1;
                        else if (*data == '0') state = 0;
                        else break;
                        *gph[d][port][pin].in = state;
                        *gph[d][port][pin].in_not = !state;
                    }
                }
            }
        }

        // parse answer for stepdir positions and states
        if (data = strstr(buf, "stepdir:"))
        {
            for (data += 8, ch = 0; ch < STEPDIR_CH_CNT; ch++)
            {
                // get position
                if (sscanf(data, "%lf", &f) == 1) *sdh[d][ch].pos_fb = f;
                // get state
                data += (uint32_t)strcspn(data, ">|,");
                if (*data == '>') sdp[d][ch].state = 1;
                else if (*data == '|') sdp[d][ch].state = 0;
                else data++;
            }
        }

        // parse answer for encoder positions and findZ states
        if (data = strstr(buf, "encoder:"))
        {
            for (data += 8, ch = 0; ch < ENCODER_CH_CNT; ch++)
            {
                // get position
                if (sscanf(data, "%lf", &f) == 1) *eh[d][ch].pos_fb = f;
                // get findZ state
                data += (uint32_t)strcspn(data, ">|,");
                if (*data == '>' || *data == '|')
                {
                    // get real state
                    state = *data == '>' ? 1 : 0;
                    // reset HAL pin only if real findZ state was changed from 1 to 0
                    if (ep[d][ch].find_Z_real && !state)
                    {
                        ep[d][ch].find_Z = state; 
                        *eh[d][ch].find_Z = state;
                    }
                    // save real state
                    ep[d][ch].find_Z_real = state;
                }
                else data++;
            }
        }
    }
}

static void comp_write(void *arg, long period)
{
    static uint32_t d, port, pin, ch, send_set_cmd, msg_sent;
    static int32_t size;
    static char *set_cmd, set_cmd_buf[512];

    for (d = dev_cnt; d--;)
    {
        // setup number of messages
        msg_sent = 0;
        // setup set command string
        set_cmd = set_cmd_buf;
        strcpy(set_cmd, "!set ");
        set_cmd += 5;
        send_set_cmd = 0;
        // gpio setup
        strcpy(set_cmd, "gpio:");
        set_cmd += 5;
        for (port = 0; port < GPIO_PORTS_CNT; port++)
        {
            *set_cmd++ = 'A'+port;
            for (pin = 0; pin < GPIO_PINS_CNT; pin++)
            {
                // output pin type/pull check/set
                if (msg_sent < MSG_MAX_CNT && 
                    (gpp[d][port][pin].type != *gph[d][port][pin].type ||
                    gpp[d][port][pin].pull != *gph[d][port][pin].pull))
                {
                    gpp[d][port][pin].type = *gph[d][port][pin].type;
                    gpp[d][port][pin].pull = *gph[d][port][pin].pull;
                    if (simulation) continue;
                    size = sprintf(buf, "!gpio P%c%u type %d %d", 'A'+port, pin, 
                                   *gph[d][port][pin].type, *gph[d][port][pin].pull);
                    sendto(sock, buf, size+1, 0, 
                           (struct sockaddr*)&dev[d].sock_addr, sizeof(dev[d].sock_addr));
                    msg_sent++;
                }
                // pin state check/set
                if (msg_sent < MSG_MAX_CNT &&
                    (gpp[d][port][pin].out != *gph[d][port][pin].out ||
                    gpp[d][port][pin].out_not != *gph[d][port][pin].out_not))
                {
                    if (gpp[d][port][pin].out != *gph[d][port][pin].out)
                    {
                        gpp[d][port][pin].out = *gph[d][port][pin].out;
                        gpp[d][port][pin].out_not = !*gph[d][port][pin].out;
                        *gph[d][port][pin].out_not = !*gph[d][port][pin].out;
                    }
                    else
                    {
                        gpp[d][port][pin].out_not = *gph[d][port][pin].out_not;
                        gpp[d][port][pin].out = !*gph[d][port][pin].out_not;
                        *gph[d][port][pin].out = !*gph[d][port][pin].out_not;
                    }
                    *set_cmd++ = *gph[d][port][pin].out ? '1' : '0';
                    send_set_cmd = 1;
                } 
                else *set_cmd++ = '*';
            }
            if (port < GPIO_PORTS_CNT-1) *set_cmd++ = ',';
        }
        *set_cmd = 0;
        
        // stepdir setup
        strcpy(set_cmd, " goto:");
        set_cmd += 6;
        for (ch = 0; ch < STEPDIR_CH_CNT; ch++)
        {
            // enable check/set
            if (msg_sent < MSG_MAX_CNT && sdp[d][ch].enable != *sdh[d][ch].enable)
            {
                // disable channel
                if (!simulation && sdp[d][ch].state && !*sdh[d][ch].enable)
                {
                    size = sprintf(buf, "!stepdir %u stop", ch);
                    sendto(sock, buf, size+1, 0, 
                           (struct sockaddr*)&dev[d].sock_addr, sizeof(dev[d].sock_addr));
                    msg_sent++;
                }
                sdp[d][ch].enable = *sdh[d][ch].enable;
            }
            // direction signal offset check/set
            if (msg_sent < MSG_MAX_CNT && sdp[d][ch].dir_offset != *sdh[d][ch].dir_offset)
            {
                sdp[d][ch].dir_offset = *sdh[d][ch].dir_offset;
                if (!simulation)
                {
                    size = sprintf(buf, "!stepdir %u dirspace %u", ch, *sdh[d][ch].dir_offset);
                    sendto(sock, buf, size+1, 0, 
                           (struct sockaddr*)&dev[d].sock_addr, sizeof(dev[d].sock_addr));
                    msg_sent++;
                }
            }
            // step signal time check/set
            if (msg_sent < MSG_MAX_CNT && sdp[d][ch].step_len != *sdh[d][ch].step_len)
            {
                sdp[d][ch].step_len = *sdh[d][ch].step_len;
                if (!simulation)
                {
                    size = sprintf(buf, "!stepdir %u steplen %u", ch, *sdh[d][ch].step_len);
                    sendto(sock, buf, size+1, 0, 
                           (struct sockaddr*)&dev[d].sock_addr, sizeof(dev[d].sock_addr));
                    msg_sent++;
                }
            }
            // `steps per unit` value check/set
            if (msg_sent < MSG_MAX_CNT && sdp[d][ch].pos_scale != *sdh[d][ch].pos_scale)
            {
                if (*sdh[d][ch].pos_scale < 1.0) *sdh[d][ch].pos_scale = 1;
                sdp[d][ch].pos_scale = *sdh[d][ch].pos_scale;
                if (!simulation)
                {
                    size = sprintf(buf, "!stepdir %u stepmult %f", ch, *sdh[d][ch].pos_scale);
                    sendto(sock, buf, size+1, 0, 
                           (struct sockaddr*)&dev[d].sock_addr, sizeof(dev[d].sock_addr));
                    msg_sent++;
                }
            }
            // max speed check/set
            if (msg_sent < MSG_MAX_CNT && sdp[d][ch].max_speed != *sdh[d][ch].max_speed)
            {
                sdp[d][ch].max_speed = *sdh[d][ch].max_speed;
                if (!simulation)
                {
                    size = sprintf(buf, "!stepdir %u maxspeed %f", ch, *sdh[d][ch].max_speed);
                    sendto(sock, buf, size+1, 0, 
                           (struct sockaddr*)&dev[d].sock_addr, sizeof(dev[d].sock_addr));
                    msg_sent++;
                }
            }
            // max acceleration check/set
            if (msg_sent < MSG_MAX_CNT && sdp[d][ch].max_accel != *sdh[d][ch].max_accel)
            {
                sdp[d][ch].max_accel = *sdh[d][ch].max_accel;
                if (!simulation)
                {
                    size = sprintf(buf, "!stepdir %u accel %f", ch, *sdh[d][ch].max_accel);
                    sendto(sock, buf, size+1, 0, 
                           (struct sockaddr*)&dev[d].sock_addr, sizeof(dev[d].sock_addr));
                    msg_sent++;
                }
            }
            // step pin check/set
            if (msg_sent < MSG_MAX_CNT && sdp[d][ch].step_pin != *sdh[d][ch].step_pin)
            {
                port = *sdh[d][ch].step_pin / 100;
                pin = *sdh[d][ch].step_pin % 100;
                if (port >= GPIO_PORTS_CNT || pin >= GPIO_PINS_CNT)
                    *sdh[d][ch].step_pin = sdp[d][ch].step_pin;
                else
                {
                    sdp[d][ch].step_pin = *sdh[d][ch].step_pin;
                    if (!simulation)
                    {
                        size = sprintf(buf, "!stepdir %u steppin P%c%u", ch, 'A'+port, pin);
                        sendto(sock, buf, size+1, 0, 
                               (struct sockaddr*)&dev[d].sock_addr, sizeof(dev[d].sock_addr));
                        msg_sent++;
                    }
                }
            }
            // direction pin check/set
            if (msg_sent < MSG_MAX_CNT && sdp[d][ch].dir_pin != *sdh[d][ch].dir_pin)
            {
                port = *sdh[d][ch].dir_pin / 100;
                pin = *sdh[d][ch].dir_pin % 100;
                if (port >= GPIO_PORTS_CNT || pin >= GPIO_PINS_CNT)
                    *sdh[d][ch].dir_pin = sdp[d][ch].dir_pin;
                else
                {
                    sdp[d][ch].dir_pin = *sdh[d][ch].dir_pin;
                    if (!simulation)
                    {
                        size = sprintf(buf, "!stepdir %u dirpin P%c%u", ch, 'A'+port, pin);
                        sendto(sock, buf, size+1, 0, 
                               (struct sockaddr*)&dev[d].sock_addr, sizeof(dev[d].sock_addr));
                        msg_sent++;
                    }
                }
            }
            // reset check/set
            if (msg_sent < MSG_MAX_CNT && *sdh[d][ch].pos_reset)
            {
                *sdh[d][ch].pos_reset = 0;
                *sdh[d][ch].pos_fb = 0;
                if (!simulation)
                {
                    size = sprintf(buf, "!stepdir %u pos 0.0", ch);
                    sendto(sock, buf, size+1, 0, 
                           (struct sockaddr*)&dev[d].sock_addr, sizeof(dev[d].sock_addr));
                    msg_sent++;
                }
            }
            // position command check/set
            if (msg_sent < MSG_MAX_CNT && *sdh[d][ch].enable && sdp[d][ch].pos_cmd != *sdh[d][ch].pos_cmd)
            {
                sdp[d][ch].pos_cmd = *sdh[d][ch].pos_cmd;
                size = sprintf(set_cmd, "%f", *sdh[d][ch].pos_cmd);
                set_cmd += size;
                send_set_cmd = 1;
            }
            else *set_cmd++ = '*';
            if (ch < STEPDIR_CH_CNT-1) *set_cmd++ = ',';
        }
        *set_cmd = 0;

        // encoder setup
        for (ch = 0; ch < ENCODER_CH_CNT; ch++)
        {
            // A pin check/set
            if (msg_sent < MSG_MAX_CNT && ep[d][ch].A_pin != *eh[d][ch].A_pin)
            {
                port = *eh[d][ch].A_pin / 100;
                pin = *eh[d][ch].A_pin % 100;
                if (port >= GPIO_PORTS_CNT || pin >= GPIO_PINS_CNT)
                    *eh[d][ch].A_pin = ep[d][ch].A_pin;
                else
                {
                    ep[d][ch].A_pin = *eh[d][ch].A_pin;
                    if (!simulation)
                    {
                        size = sprintf(buf, "!encoder %u pinA P%c%u", ch, 'A'+port, pin);
                        sendto(sock, buf, size+1, 0, 
                               (struct sockaddr*)&dev[d].sock_addr, sizeof(dev[d].sock_addr));
                        msg_sent++;
                    }
                }
            }
            // B pin check/set
            if (msg_sent < MSG_MAX_CNT && ep[d][ch].B_pin != *eh[d][ch].B_pin)
            {
                port = *eh[d][ch].B_pin / 100;
                pin = *eh[d][ch].B_pin % 100;
                if (port >= GPIO_PORTS_CNT || pin >= GPIO_PINS_CNT)
                    *eh[d][ch].B_pin = ep[d][ch].B_pin;
                else
                {
                    ep[d][ch].B_pin = *eh[d][ch].B_pin;
                    if (!simulation)
                    {
                        size = sprintf(buf, "!encoder %u pinB P%c%u", ch, 'A'+port, pin);
                        sendto(sock, buf, size+1, 0, 
                               (struct sockaddr*)&dev[d].sock_addr, sizeof(dev[d].sock_addr));
                        msg_sent++;
                    }
                }
            }
            // Z pin check/set
            if (msg_sent < MSG_MAX_CNT && ep[d][ch].Z_pin != *eh[d][ch].Z_pin)
            {
                port = *eh[d][ch].Z_pin / 100;
                pin = *eh[d][ch].Z_pin % 100;
                if (port >= GPIO_PORTS_CNT || pin >= GPIO_PINS_CNT)
                    *eh[d][ch].Z_pin = ep[d][ch].Z_pin;
                else
                {
                    ep[d][ch].Z_pin = *eh[d][ch].Z_pin;
                    if (!simulation)
                    {
                        size = sprintf(buf, "!encoder %u pinZ P%c%u", ch, 'A'+port, pin);
                        sendto(sock, buf, size+1, 0, 
                               (struct sockaddr*)&dev[d].sock_addr, sizeof(dev[d].sock_addr));
                        msg_sent++;
                    }
                }
            }
            // `pulses per unit` value check/set
            if (msg_sent < MSG_MAX_CNT && ep[d][ch].pos_scale != *eh[d][ch].pos_scale)
            {
                if (*eh[d][ch].pos_scale < 1.0) *eh[d][ch].pos_scale = 1;
                ep[d][ch].pos_scale = *eh[d][ch].pos_scale;
                if (!simulation)
                {
                    size = sprintf(buf, "!encoder %u mult %f", ch, *eh[d][ch].pos_scale);
                    sendto(sock, buf, size+1, 0, 
                           (struct sockaddr*)&dev[d].sock_addr, sizeof(dev[d].sock_addr));
                    msg_sent++;
                }
            }
            // reset check/set
            if (msg_sent < MSG_MAX_CNT && *eh[d][ch].reset)
            {
                *eh[d][ch].reset = 0;
                *eh[d][ch].pos_fb = 0;
                if (!simulation)
                {
                    size = sprintf(buf, "!encoder %u pos 0.0", ch);
                    sendto(sock, buf, size+1, 0, 
                           (struct sockaddr*)&dev[d].sock_addr, sizeof(dev[d].sock_addr));
                    msg_sent++;
                }
            }
            // enable check/set
            if (msg_sent < MSG_MAX_CNT && ep[d][ch].find_Z != *eh[d][ch].find_Z)
            {
                ep[d][ch].find_Z = *eh[d][ch].find_Z;
                if (!simulation)
                {
                    size = sprintf(buf, "!encoder %u findZ %u", ch, *eh[d][ch].find_Z);
                    sendto(sock, buf, size+1, 0, 
                           (struct sockaddr*)&dev[d].sock_addr, sizeof(dev[d].sock_addr));
                    msg_sent++;
                }
            }
            // enable check/set
            if (msg_sent < MSG_MAX_CNT && ep[d][ch].enable != *eh[d][ch].enable)
            {
                ep[d][ch].enable = *eh[d][ch].enable;
                if (!simulation)
                {
                    size = sprintf(buf, "!encoder %u enable %u", ch, *eh[d][ch].enable);
                    sendto(sock, buf, size+1, 0, 
                           (struct sockaddr*)&dev[d].sock_addr, sizeof(dev[d].sock_addr));
                    msg_sent++;
                }
            }
        }
        
        // send `set` command if it really needs
        if (msg_sent < MSG_MAX_CNT && !simulation && send_set_cmd)
        {
            sendto(sock, set_cmd_buf, strlen(set_cmd_buf), 0, 
                   (struct sockaddr*)&dev[d].sock_addr, sizeof(dev[d].sock_addr));
            msg_sent++;
        }
    }
}

static int32_t comp_init()
{
    uint32_t d, size, i, n, ip[4], port, pin, ch;
    int32_t r;
    char name[HAL_NAME_LEN + 1];

    // parse IPs list
    size = strlen(IP);
    for (i = 0, d = 0; i < size && d < DEVICES_MAX_CNT;)
    {
        for (n = 0; n < 4; n++)
        {
            if (i >= size || sscanf(&IP[i], "%u", &ip[n]) < 1) break;
            if (n < 3) i += 1 + (uint32_t)strcspn(&IP[i], ".");
        }
        if (n >= 4)
        {
            for (n = 4; n--;) dev[d].IP[n] = (uint8_t)ip[n];
            dev[d].used = 1;
            d++;
            if (d > dev_cnt) dev_cnt = d;
        }
        i += 1 + (uint32_t)strcspn(&IP[i], ",");
    }

    // parse ports list
    size = strlen(PORT);
    for (i = 0, d = 0; i < size && d < DEVICES_MAX_CNT;)
    {
        if (sscanf(&PORT[i], "%u", &n) < 1) break;
        dev[d].port = (uint16_t)n;
        dev[d].used = 1;
        d++;
        if (d > dev_cnt) dev_cnt = d;
        i += 1 + (uint32_t)strcspn(&PORT[i], ",");
    }

    // parse sim state
    simulation = (SIMULATION[0] == '1') ? 1 : 0;

    // setup network
    if (!simulation)
    {
        sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock < 0) return -1;
        struct timeval tv = {ETH_ANSWER_TIMEOUT_S, ETH_ANSWER_TIMEOUT_US}; // {s,us}
        if ( setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv) < 0 )
        {
            rtapi_print_msg(RTAPI_MSG_ERR,
                "%s: ERROR: can't set socket options (SO_RCVTIMEO) [%s:%d]\n",
                comp_name, __FILE__, __LINE__);
            close(sock);
            return -1;
        }
    }

    // malloc for device HAL pins
    devh = hal_malloc(dev_cnt * sizeof(dev_hal_t));
    if ( !devh ) {
        rtapi_print_msg(RTAPI_MSG_ERR, "%s: hal_malloc() failed\n", comp_name);
        return -1;
    }

    // setup devices
    for (d = dev_cnt; d--;)
    {
        // setup socket address
        if (!simulation)
        {
            dev[d].sock_addr.sin_family = AF_INET;
            dev[d].sock_addr.sin_port = htons(dev[d].port);
            dev[d].sock_addr.sin_addr.s_addr = htonl(*(uint32_t *)&dev[d].IP);
            dev[d].sock_addr_size = sizeof(dev[d].sock_addr);
        }
        // malloc for device HAL pins
        for (port = GPIO_PORTS_CNT; port--;)
            gph[d][port] = hal_malloc(GPIO_PINS_CNT * sizeof(gpio_hal_t));
        sdh[d] = hal_malloc(STEPDIR_CH_CNT * sizeof(stepdir_ch_hal_t));
        eh[d] = hal_malloc(ENCODER_CH_CNT * sizeof(encoder_ch_hal_t));
        if ( !gph[d][0] || !sdh[d] || !eh[d] ) {
            rtapi_print_msg(RTAPI_MSG_ERR, "%s: hal_malloc() failed\n", comp_name);
            return -1;
        }
        // setup device hal pins
        r = 0;
        r += hal_pin_bit_newf(HAL_OUT, &devh[d].connected, comp_id, 
                              "%s.%u.connected", comp_name, d);
        if ( r ) {
            rtapi_print_msg(RTAPI_MSG_ERR, "%s: device HAL pins export failed \n", comp_name);
            return -1;
        }
        // setup gpio hal pins
        r = 0;
        for (port = GPIO_PORTS_CNT; port--;)
        {
            for (pin = GPIO_PINS_CNT; pin--;)
            {
                r += hal_pin_bit_newf(HAL_OUT, &gph[d][port][pin].in, comp_id, 
                                      "%s.%u.gpio.P%c%u.state", comp_name, d, 'A'+port, pin);
                r += hal_pin_bit_newf(HAL_OUT, &gph[d][port][pin].in_not, comp_id, 
                                      "%s.%u.gpio.P%c%u.state-inverted", comp_name, d, 'A'+port, pin);
                r += hal_pin_bit_newf(HAL_IN, &gph[d][port][pin].out, comp_id, 
                                      "%s.%u.gpio.P%c%u.control", comp_name, d, 'A'+port, pin);
                r += hal_pin_bit_newf(HAL_IN, &gph[d][port][pin].out_not, comp_id, 
                                      "%s.%u.gpio.P%c%u.control-inverted", comp_name, d, 'A'+port, pin);
                r += hal_pin_s32_newf(HAL_IN, &gph[d][port][pin].pull, comp_id, 
                                      "%s.%u.gpio.P%c%u.pull", comp_name, d, 'A'+port, pin);
                r += hal_pin_s32_newf(HAL_IN, &gph[d][port][pin].type, comp_id, 
                                      "%s.%u.gpio.P%c%u.type", comp_name, d, 'A'+port, pin);
                *gph[d][port][pin].out_not = !*gph[d][port][pin].out;
                gpp[d][port][pin].out_not = !*gph[d][port][pin].out;
            }
        }
        if ( r ) {
            rtapi_print_msg(RTAPI_MSG_ERR, "%s: gpio HAL pins export failed \n", comp_name);
            return -1;
        }
        // setup stepdir hal pins
        r = 0;
        for (ch = STEPDIR_CH_CNT; ch--;)
        {
            r += hal_pin_bit_newf(HAL_IN, &sdh[d][ch].enable, comp_id, 
                                  "%s.%u.stepdir.%u.enable", comp_name, d, ch);
            r += hal_pin_u32_newf(HAL_IN, &sdh[d][ch].dir_offset, comp_id, 
                                  "%s.%u.stepdir.%u.dir-offset", comp_name, d, ch);
            r += hal_pin_u32_newf(HAL_IN, &sdh[d][ch].dir_pin, comp_id, 
                                  "%s.%u.stepdir.%u.dir-pin", comp_name, d, ch);
            r += hal_pin_u32_newf(HAL_IN, &sdh[d][ch].step_pin, comp_id, 
                                  "%s.%u.stepdir.%u.step-pin", comp_name, d, ch);
            r += hal_pin_u32_newf(HAL_IN, &sdh[d][ch].step_len, comp_id, 
                                  "%s.%u.stepdir.%u.step-len", comp_name, d, ch);
            r += hal_pin_float_newf(HAL_IN, &sdh[d][ch].max_speed, comp_id, 
                                  "%s.%u.stepdir.%u.max-speed", comp_name, d, ch);
            r += hal_pin_float_newf(HAL_IN, &sdh[d][ch].max_accel, comp_id, 
                                  "%s.%u.stepdir.%u.max-accel", comp_name, d, ch);
            r += hal_pin_float_newf(HAL_IN, &sdh[d][ch].pos_scale, comp_id, 
                                  "%s.%u.stepdir.%u.pos-scale", comp_name, d, ch);
            r += hal_pin_float_newf(HAL_IN, &sdh[d][ch].pos_cmd, comp_id, 
                                  "%s.%u.stepdir.%u.pos-cmd", comp_name, d, ch);
            r += hal_pin_float_newf(HAL_IN, &sdh[d][ch].pos_fb, comp_id, 
                                  "%s.%u.stepdir.%u.pos-fb", comp_name, d, ch);
            r += hal_pin_bit_newf(HAL_IN, &sdh[d][ch].pos_reset, comp_id, 
                                  "%s.%u.stepdir.%u.pos-reset", comp_name, d, ch);
            *sdh[d][ch].dir_offset = 50000;
             sdp[d][ch].dir_offset = 50000;
            *sdh[d][ch].pos_scale = 1;
             sdp[d][ch].pos_scale = 1;
            *sdh[d][ch].step_pin = 0xFFFF;
             sdp[d][ch].step_pin = 0xFFFF;
            *sdh[d][ch].dir_pin = 0xFFFF;
             sdp[d][ch].dir_pin = 0xFFFF;
        }
        if ( r ) {
            rtapi_print_msg(RTAPI_MSG_ERR, "%s: stepdir HAL pins export failed \n", comp_name);
            return -1;
        }
        // setup encoder hal pins
        r = 0;
        for (ch = ENCODER_CH_CNT; ch--;)
        {
            r += hal_pin_bit_newf(HAL_IN, &eh[d][ch].enable, comp_id, 
                                  "%s.%u.encoder.%u.enable", comp_name, d, ch);
            r += hal_pin_bit_newf(HAL_IN, &eh[d][ch].reset, comp_id, 
                                  "%s.%u.encoder.%u.reset", comp_name, d, ch);
            r += hal_pin_u32_newf(HAL_IN, &eh[d][ch].A_pin, comp_id, 
                                  "%s.%u.encoder.%u.A-pin", comp_name, d, ch);
            r += hal_pin_u32_newf(HAL_IN, &eh[d][ch].B_pin, comp_id, 
                                  "%s.%u.encoder.%u.B-pin", comp_name, d, ch);
            r += hal_pin_u32_newf(HAL_IN, &eh[d][ch].Z_pin, comp_id, 
                                  "%s.%u.encoder.%u.Z-pin", comp_name, d, ch);
            r += hal_pin_bit_newf(HAL_IO, &eh[d][ch].find_Z, comp_id, 
                                  "%s.%u.encoder.%u.find-Z", comp_name, d, ch);
            r += hal_pin_float_newf(HAL_IN, &eh[d][ch].pos_scale, comp_id, 
                                  "%s.%u.encoder.%u.pos-scale", comp_name, d, ch);
            r += hal_pin_float_newf(HAL_IN, &eh[d][ch].pos_fb, comp_id, 
                                  "%s.%u.encoder.%u.pos-fb", comp_name, d, ch);
            *eh[d][ch].pos_scale = 1;
             ep[d][ch].pos_scale = 1;
            *eh[d][ch].A_pin = 0xFFFF;
             ep[d][ch].A_pin = 0xFFFF;
            *eh[d][ch].B_pin = 0xFFFF;
             ep[d][ch].B_pin = 0xFFFF;
            *eh[d][ch].Z_pin = 0xFFFF;
             ep[d][ch].Z_pin = 0xFFFF;
        }
        if ( r ) {
            rtapi_print_msg(RTAPI_MSG_ERR, "%s: encoder HAL pins export failed \n", comp_name);
            return -1;
        }
    }

    if ( dev_cnt ) {
        // export component HAL functions
        r = 0;
        rtapi_snprintf(name, sizeof(name), "%s.write", comp_name);
        r += hal_export_funct(name, comp_write, 0, 0, 0, comp_id);
        rtapi_snprintf(name, sizeof(name), "%s.read", comp_name);
        r += hal_export_funct(name, comp_read, 0, 0, 0, comp_id);
        if ( r )
        {
            rtapi_print_msg(RTAPI_MSG_ERR, "%s: real-time functions export failed\n", comp_name);
            return -1;
        }
        // setup gpio general hal pins
        for (port = GPIO_PORTS_CNT; port--;)
        {
            for (pin = GPIO_PINS_CNT; pin--;)
            {
                rtapi_snprintf(name, HAL_NAME_LEN, "P%c%u", 'A'+port, pin);
                rtapi_snprintf(buf, HAL_NAME_LEN, "%s.gpio.%s", comp_name, name);
                r += hal_signal_new(name, HAL_U32);
                r += hal_pin_u32_newf(HAL_OUT, &gph[0][port][pin].id, comp_id, "%s", buf);
                r += hal_link(buf, name);
                *gph[0][port][pin].id = port*100 + pin;
            }
        }
    }

    return 0;
}

static void comp_deinit()
{
    uint32_t d;

    // TODO - stop devices
    for (d = dev_cnt; d--;)
    {
        // TODO
    }

    // stop network
    if (!simulation && sock) close(sock);
}




int32_t rtapi_app_main(void)
{
    comp_id = hal_init(comp_name);

    if ( comp_id < 0 ) {
        rtapi_print_msg(RTAPI_MSG_ERR, "%s: ERROR: hal_init() failed\n", comp_name);
        return -1;
    }

    if ( comp_init() ) {
        hal_exit(comp_id);
        return -1;
    }

    hal_ready(comp_id);
    return 0;
}

void rtapi_app_exit(void)
{
    comp_deinit();
    hal_exit(comp_id);
}
