#ifndef __USBD_UVC_H
#define __USBD_UVC_H

#include <stdint.h>

/* probe/commit 控制数据长度（UVC 1.0） */
#define UVC_PROBE_COMMIT_LEN (26)

void uvc_control_init(void);
void uvc_commit_applied(void);
void uvc_stream_enable(void);
void uvc_stream_disable(void);
uint8_t uvc_is_streaming(void);
uint8_t uvc_frame_done(void);
void uvc_feed_frame(const uint8_t *frame, uint16_t len);
void uvc_ep1_in_callback(void);
uint8_t uvc_poll_resolution(uint16_t *width, uint16_t *height);

extern uint8_t uvc_probe_data[UVC_PROBE_COMMIT_LEN];
extern uint8_t uvc_commit_data[UVC_PROBE_COMMIT_LEN];

#endif /* __USBD_UVC_H */
