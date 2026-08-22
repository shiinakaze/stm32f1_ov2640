#include "usb_lib.h"
#include "usb_conf.h"
#include "usbd_uvc.h"

/* UVC MJPEG 流式状态机（F1 ISO 双缓冲版）。
 *
 * RM0008 §23.4.4：F1 的 ISO 端点硬件即双缓冲——每次 IN 传输完成，
 * CTR_TX 置位且硬件自动翻转 DTOG_TX，下一个包从另一组缓冲描述符
 * （BUF0=TX 槽 / BUF1=RX 槽）发送；STAT_TX 启动后保持 VALID
 * （对 ISO 写 NAK 属未定义行为，绝不可写）。
 *
 * 发送引擎：每次 CTR_TX（一包刚发完）立即把下一包装入硬件即将
 * 使用的那个缓冲（按 DTOG_TX 判断），比主机下一次轮询提前约 1ms。
 *
 * 采集静默模式：一帧的 EOF 包发送完成后，把两个缓冲都装成空包
 * （2 字节载荷头，无数据无 EOF）。随后主循环锁 USB 中断采集期间，
 * 硬件持续交替发送这两个空包——总线上保持规整的 UVC 空包流，
 * 不需要任何 CPU 参与，主机只观察到一帧"慢速产生"的视频。
 */

/* 单包最大视频载荷（wMaxPacketSize 184 - 2 字节头） */
#define UVC_MAX_PAYLOAD (UVC_ISO_PACKET_SIZE - 2)

#define UVC_HDR_FID (0x01)
#define UVC_HDR_EOF (0x02)

/* probe/commit 数据（小端字节序，UVC 1.0 共 26 字节）：
 * bmHint(2) bFormatIndex(1) bFrameIndex(1) dwFrameInterval(4)
 * wKeyFrameRate(2) wPFrameRate(2) wCompQuality(2) wCompWindowSize(2)
 * wDelay(2) dwMaxVideoFrameSize(4) dwMaxPayloadTransferSize(4) */
uint8_t uvc_probe_data[UVC_PROBE_COMMIT_LEN];
uint8_t uvc_commit_data[UVC_PROBE_COMMIT_LEN];

/* ISO 包暂存区（先拼包再一次性拷入 PMA） */
static uint8_t uvc_packet[UVC_ISO_PACKET_SIZE];

/* 流式状态 */
static volatile uint8_t stream_enabled = 0;   /* 主机已选择 alt 1 */
static volatile uint8_t frame_active = 0;     /* 正在发送一帧 */
static volatile uint8_t frame_done_flag = 1;  /* 1=当前无帧在发，可采集 */
static volatile uint8_t eof_tx_pending = 0;   /* EOF 包已装入待发出 */
static const uint8_t *cur_frame = NULL;
static const uint8_t *next_frame = NULL;
static volatile uint16_t cur_len = 0;
static volatile uint16_t cur_pos = 0;
static volatile uint16_t next_len = 0;
static volatile uint8_t fid = 0;              /* 帧标识位，每帧翻转 */

/* 主机 commit 选择的分辨率（切换需在主循环中做 SCCB 写入） */
static volatile uint8_t res_pending = 0;
static volatile uint16_t res_width = 320;
static volatile uint16_t res_height = 240;
static volatile uint8_t applied_frame_index = 1;

/* 帧索引 → 分辨率（顺序与 usb_desc.c 的 VS 帧描述符一致） */
static const uint16_t res_table[][2] = {
    {320, 240}, /* bFrameIndex 1 */
    {160, 120}, /* bFrameIndex 2 */
    {640, 360}, /* bFrameIndex 3 */
};

static uint16_t payload_max = UVC_MAX_PAYLOAD;

static void load_next_packet(void);

/*---------------- 对主循环暴露的接口 ----------------*/

uint8_t uvc_is_streaming(void)
{
    return stream_enabled;
}

/* 1 = 上一帧的 EOF 包已发送完成（总线进入空包流），可以安全采集 */
uint8_t uvc_frame_done(void)
{
    return frame_done_flag;
}

/* 主循环在采集完成后调用；先写指针后写长度，与 CTR 中断无竞争 */
void uvc_feed_frame(const uint8_t *frame, uint16_t len)
{
    next_frame = frame;
    frame_done_flag = 0;
    next_len = len;
}

/* 分辨率切换请求（commit 改变 bFrameIndex 时置位），由主循环取出执行 */
uint8_t uvc_poll_resolution(uint16_t *width, uint16_t *height)
{
    if (res_pending)
    {
        res_pending = 0;
        *width = res_width;
        *height = res_height;
        return 1;
    }
    return 0;
}

/*---------------- 由 usb_prop.c 调用 ----------------*/

/* 主机 SET_INTERFACE(alt 1)：准备开始流式发送 */
void uvc_stream_enable(void)
{
    /* 应用主机最后一次 COMMIT 的协商结果（包载荷上限/分辨率） */
    uvc_commit_applied();

    frame_active = 0;
    next_len = 0;
    eof_tx_pending = 0;
    frame_done_flag = 1;

    /* DTOG_TX 清零 → 硬件从 BUF0 开始发送；随后装一个空包并
     * 置 VALID（ISO 端点此后硬件保持 VALID，不再操作 STAT） */
    ClearDTOG_RX(ENDP1);
    ClearDTOG_TX(ENDP1);
    stream_enabled = 1;
    load_next_packet();
    SetEPTxStatus(ENDP1, EP_TX_VALID);
}

/* 主机 SET_INTERFACE(alt 0) / USB 复位 / 挂起：停止发送 */
void uvc_stream_disable(void)
{
    stream_enabled = 0;
    frame_active = 0;
    eof_tx_pending = 0;
    cur_frame = NULL;
    next_len = 0;
    frame_done_flag = 1;
    SetEPTxStatus(ENDP1, EP_TX_DIS);
}

/* SET_CUR COMMIT 处理：解析主机协商结果 */
void uvc_commit_applied(void)
{
    uint32_t payload;
    uint8_t frame_index = uvc_commit_data[3];

    payload = (uint32_t)uvc_commit_data[22] |
              ((uint32_t)uvc_commit_data[23] << 8) |
              ((uint32_t)uvc_commit_data[24] << 16) |
              ((uint32_t)uvc_commit_data[25] << 24);
    if (payload >= UVC_ISO_PACKET_SIZE)
    {
        payload = UVC_ISO_PACKET_SIZE;
    }
    payload_max = (payload >= 2) ? (payload - 2) : UVC_MAX_PAYLOAD;
    if (payload_max == 0 || payload_max > UVC_MAX_PAYLOAD)
    {
        payload_max = UVC_MAX_PAYLOAD;
    }

    if (frame_index != applied_frame_index)
    {
        applied_frame_index = frame_index;
        if (frame_index >= 1 && frame_index <= sizeof(res_table) / sizeof(res_table[0]))
        {
            res_width = res_table[frame_index - 1][0];
            res_height = res_table[frame_index - 1][1];
        }
        else
        {
            /* 非法帧索引回退默认分辨率 */
            res_width = 320;
            res_height = 240;
        }
        res_pending = 1;
    }
}

/*---------------- 发送引擎（USB 中断上下文） ----------------*/

/* 装填下一包：拼包后按 DTOG_TX 写入硬件即将使用的缓冲。
 * DTOG_TX=1 → 硬件下一个使用 BUF1（RX 描述符槽），否则 BUF0。 */
static void load_next_packet(void)
{
    uint16_t payload;
    uint16_t buf_addr;
    uint16_t i;
    uint8_t last;

    /* 当前帧发完且新帧已就绪：开始发送新帧 */
    if (frame_active == 0 && next_len != 0)
    {
        cur_frame = next_frame;
        cur_len = next_len;
        cur_pos = 0;
        frame_active = 1;
    }

    if (frame_active)
    {
        payload = cur_len - cur_pos;
        if (payload > payload_max)
        {
            payload = payload_max;
        }
        last = (uint8_t)((cur_pos + payload) >= cur_len);

        uvc_packet[0] = 2;
        uvc_packet[1] = fid;
        for (i = 0; i < payload; i++)
        {
            uvc_packet[2 + i] = cur_frame[cur_pos + i];
        }

        if (last)
        {
            /* 最后一包：EOF 置位并翻转 FID。
             * 随后的空包与新帧数据使用同一新 FID（主机视角单次翻转） */
            uvc_packet[1] |= UVC_HDR_EOF;
            eof_tx_pending = 1;
            frame_active = 0;
            fid ^= 1;
        }
        cur_pos += payload;
    }
    else
    {
        /* 帧间空包：只有 2 字节载荷头，无数据无 EOF */
        payload = 0;
        uvc_packet[0] = 2;
        uvc_packet[1] = fid;
    }

    if (GetENDPOINT(ENDP1) & EP_DTOG_TX)
    {
        buf_addr = ENDP1_BUF1ADDR;
        UserToPMABufferCopy(uvc_packet, buf_addr, payload + 2);
        SetEPDblBuf1Count(ENDP1, EP_DBUF_IN, payload + 2);
    }
    else
    {
        buf_addr = ENDP1_BUF0ADDR;
        UserToPMABufferCopy(uvc_packet, buf_addr, payload + 2);
        SetEPDblBuf0Count(ENDP1, EP_DBUF_IN, payload + 2);
    }
}

/* 把一个空包（仅 2 字节头）装入指定缓冲 */
static void load_null_packet(uint16_t buf_addr, uint8_t buf_index)
{
    uvc_packet[0] = 2;
    uvc_packet[1] = fid;

    UserToPMABufferCopy(uvc_packet, buf_addr, 2);
    if (buf_index == 0)
    {
        SetEPDblBuf0Count(ENDP1, EP_DBUF_IN, 2);
    }
    else
    {
        SetEPDblBuf1Count(ENDP1, EP_DBUF_IN, 2);
    }
}

/* EP1 ISO IN 发送完成（CTR_TX）：立即装填下一包 */
void uvc_ep1_in_callback(void)
{
    if (!stream_enabled)
    {
        return;
    }

    if (eof_tx_pending)
    {
        /* EOF 包刚发送完成：一帧结束。两个缓冲此刻都归软件所有，
         * 全部装入空包——采集期间（USB 中断关闭）硬件将持续交替
         * 发送这两个空包，维持总线上的空包流 */
        eof_tx_pending = 0;
        frame_done_flag = 1;
        load_null_packet(ENDP1_BUF0ADDR, 0);
        load_null_packet(ENDP1_BUF1ADDR, 1);
        return;
    }

    load_next_packet();
}

/*---------------- probe/commit 控制数据 ----------------*/

static void probe_commit_set_default(uint8_t *buf)
{
    uint8_t i;

    for (i = 0; i < UVC_PROBE_COMMIT_LEN; i++)
    {
        buf[i] = 0;
    }
    buf[2] = 1;                                  /* bFormatIndex */
    buf[3] = applied_frame_index;                /* bFrameIndex */
    buf[4] = 0x15; buf[5] = 0x16; buf[6] = 0x05; buf[7] = 0x00; /* 333333 (30fps) */
    buf[18] = 0x00; buf[19] = 0x30; buf[20] = 0x00; buf[21] = 0x00; /* 12288 */
    buf[22] = 0xB8; buf[23] = 0x00; buf[24] = 0x00; buf[25] = 0x00; /* 184 */
}

/* 上电/复位时由 usb_prop.c 调用 */
void uvc_control_init(void)
{
    probe_commit_set_default(uvc_probe_data);
    probe_commit_set_default(uvc_commit_data);
}
