import os
import time
import serial
import cv2
import numpy as np

PORT = 'COM5'
BAUDRATE = 1500000
TIMEOUT = 0.05

SOI = b'\xff\xd8'
EOI = b'\xff\xd9'

MAX_DISPLAY_WIDTH = 1280
MAX_DISPLAY_HEIGHT = 720
BUFFER_LIMIT = 1024 * 1024
BUFFER_KEEP = 256 * 1024
AUTO_RECONNECT_INTERVAL = 2.0

WINDOW_NAME = 'OV2640 Professional Preview'

# ---------- UI ----------
BTN_H = 38
BTN_W = 120
BTN_GAP = 10
BTN_MARGIN = 10

mouse_state = {
    "clicked_action": None
}

def fit_size(width, height, max_width, max_height):
    scale = min(max_width / width, max_height / height, 1.0)
    return int(width * scale), int(height * scale)

def draw_button(img, rect, text, color_bg, color_fg=(255, 255, 255)):
    x1, y1, x2, y2 = rect
    cv2.rectangle(img, (x1, y1), (x2, y2), color_bg, -1)
    cv2.rectangle(img, (x1, y1), (x2, y2), (40, 40, 40), 1)
    text_size = cv2.getTextSize(text, cv2.FONT_HERSHEY_SIMPLEX, 0.6, 2)[0]
    tx = x1 + (x2 - x1 - text_size[0]) // 2
    ty = y1 + (y2 - y1 + text_size[1]) // 2
    cv2.putText(img, text, (tx, ty), cv2.FONT_HERSHEY_SIMPLEX, 0.6, color_fg, 2)

def build_buttons(img_w, img_h):
    y1 = img_h - BTN_MARGIN - BTN_H
    y2 = img_h - BTN_MARGIN

    names = ["SHOT", "REC", "RECONNECT", "QUIT"]
    widths = [100, 100, 150, 100]

    rects = {}
    x = BTN_MARGIN
    for name, w in zip(names, widths):
        rects[name] = (x, y1, x + w, y2)
        x += w + BTN_GAP
    return rects

def point_in_rect(x, y, rect):
    x1, y1, x2, y2 = rect
    return x1 <= x <= x2 and y1 <= y <= y2

def mouse_callback(event, x, y, flags, param):
    if event == cv2.EVENT_LBUTTONDOWN:
        buttons = param["buttons"]
        for name, rect in buttons.items():
            if point_in_rect(x, y, rect):
                mouse_state["clicked_action"] = name
                break

# ---------- Serial ----------
def open_serial():
    ser = serial.Serial(PORT, BAUDRATE, timeout=TIMEOUT)
    print(f'[INFO] Open serial: {PORT} @ {BAUDRATE}')
    return ser

def save_snapshot(frame):
    filename = time.strftime("capture_%Y%m%d_%H%M%S.jpg")
    ok = cv2.imwrite(filename, frame)
    if ok:
        print(f'[INFO] Saved image: {filename}')
    else:
        print('[ERR ] Failed to save image.')

def start_recording(frame, fps):
    h, w = frame.shape[:2]
    filename = time.strftime("record_%Y%m%d_%H%M%S.avi")
    fourcc = cv2.VideoWriter_fourcc(*'XVID')
    writer = cv2.VideoWriter(filename, fourcc, max(fps, 5.0), (w, h))
    if writer.isOpened():
        print(f'[INFO] Start recording: {filename}')
        return writer, filename
    print('[ERR ] Failed to start recording.')
    return None, None

def stop_recording(writer, filename):
    if writer is not None:
        writer.release()
    print(f'[INFO] Stop recording: {filename}')

def main():
    ser = None
    last_reconnect_try = 0.0

    buffer = bytearray()
    frame = None

    frame_count = 0
    total_frames = 0
    decode_fail_count = 0
    overflow_count = 0
    reconnect_count = 0

    last_fps_time = time.time()
    fps = 0.0

    bytes_count = 0
    last_speed_time = time.time()
    rx_kbps = 0.0

    recording = False
    writer = None
    video_filename = None
    record_start_time = None

    last_resolution = None

    cv2.namedWindow(WINDOW_NAME, cv2.WINDOW_NORMAL)
    buttons_for_mouse = {"buttons": {}}
    cv2.setMouseCallback(WINDOW_NAME, mouse_callback, buttons_for_mouse)

    print('[INFO] Hotkeys: q/ESC=Quit, s=Shot, r=Rec, c=Reconnect')

    try:
        while True:
            now = time.time()

            # 自动重连
            if ser is None:
                if now - last_reconnect_try >= AUTO_RECONNECT_INTERVAL:
                    last_reconnect_try = now
                    try:
                        ser = open_serial()
                        reconnect_count += 1
                    except Exception as e:
                        print(f'[WARN] Serial reconnect failed: {e}')

            # 串口读取
            if ser is not None:
                try:
                    data = ser.read(4096)
                    if data:
                        buffer.extend(data)
                        bytes_count += len(data)
                except Exception as e:
                    print(f'[WARN] Serial read failed: {e}')
                    try:
                        ser.close()
                    except:
                        pass
                    ser = None

            # RX速率统计
            speed_dt = now - last_speed_time
            if speed_dt >= 1.0:
                rx_kbps = bytes_count / 1024.0 / speed_dt
                bytes_count = 0
                last_speed_time = now

            # 缓冲区保护
            if len(buffer) > BUFFER_LIMIT:
                overflow_count += 1
                buffer = buffer[-BUFFER_KEEP:]
                print(f'[WARN] Buffer overflow trimmed. count={overflow_count}')

            # JPEG帧提取
            while True:
                start = buffer.find(SOI)
                if start < 0:
                    if len(buffer) > 1:
                        buffer = buffer[-1:]
                    break

                end = buffer.find(EOI, start + 2)
                if end < 0:
                    if start > 0:
                        buffer = buffer[start:]
                    break

                jpg = bytes(buffer[start:end + 2])
                buffer = buffer[end + 2:]

                img_array = np.frombuffer(jpg, dtype=np.uint8)
                decoded = cv2.imdecode(img_array, cv2.IMREAD_COLOR)

                if decoded is None:
                    decode_fail_count += 1
                    continue

                frame = decoded
                frame_count += 1
                total_frames += 1

                h, w = frame.shape[:2]
                current_resolution = (w, h)

                if current_resolution != last_resolution:
                    last_resolution = current_resolution
                    display_w, display_h = fit_size(w, h, MAX_DISPLAY_WIDTH, MAX_DISPLAY_HEIGHT)
                    # 额外给按钮和信息栏留空间
                    cv2.resizeWindow(WINDOW_NAME, display_w, display_h)
                    print(f'[INFO] Resolution: {w}x{h}, Window: {display_w}x{display_h}')

                fps_dt = now - last_fps_time
                if fps_dt >= 1.0:
                    fps = frame_count / fps_dt
                    frame_count = 0
                    last_fps_time = now
                    print(f'[STAT] FPS={fps:.2f} RX={rx_kbps:.2f}KB/s Frames={total_frames} DecodeFail={decode_fail_count} Overflow={overflow_count}')

                if recording and writer is not None:
                    writer.write(frame)

            # 显示
            if frame is not None:
                show = frame.copy()
                h, w = show.shape[:2]

                # 文字信息
                line_y = 30
                line_gap = 28
                cv2.putText(show, f'FPS: {fps:.2f}', (10, line_y),
                            cv2.FONT_HERSHEY_SIMPLEX, 0.75, (0, 255, 0), 2)
                cv2.putText(show, f'Resolution: {w}x{h}', (10, line_y + line_gap),
                            cv2.FONT_HERSHEY_SIMPLEX, 0.75, (0, 255, 255), 2)
                cv2.putText(show, f'RX: {rx_kbps:.2f} KB/s', (10, line_y + line_gap * 2),
                            cv2.FONT_HERSHEY_SIMPLEX, 0.75, (255, 255, 0), 2)
                cv2.putText(show, f'Frames: {total_frames}', (10, line_y + line_gap * 3),
                            cv2.FONT_HERSHEY_SIMPLEX, 0.75, (255, 220, 180), 2)
                cv2.putText(show, f'DecodeFail: {decode_fail_count}  Overflow: {overflow_count}  Reconnect: {max(reconnect_count-1,0)}',
                            (10, line_y + line_gap * 4),
                            cv2.FONT_HERSHEY_SIMPLEX, 0.6, (100, 180, 255), 2)

                if ser is None:
                    cv2.putText(show, 'SERIAL: DISCONNECTED', (10, line_y + line_gap * 5),
                                cv2.FONT_HERSHEY_SIMPLEX, 0.75, (0, 0, 255), 2)
                else:
                    cv2.putText(show, 'SERIAL: CONNECTED', (10, line_y + line_gap * 5),
                                cv2.FONT_HERSHEY_SIMPLEX, 0.75, (0, 200, 0), 2)

                if recording:
                    rec_elapsed = int(time.time() - record_start_time) if record_start_time else 0
                    rec_text = f'REC {rec_elapsed}s'
                    cv2.putText(show, rec_text, (10, line_y + line_gap * 6),
                                cv2.FONT_HERSHEY_SIMPLEX, 0.9, (0, 0, 255), 2)

                # 按钮
                buttons = build_buttons(w, h)
                buttons_for_mouse["buttons"] = buttons
                draw_button(show, buttons["SHOT"], "SHOT [S]", (60, 120, 220))
                draw_button(show, buttons["REC"], "REC [R]" if not recording else "STOP [R]", (50, 50, 220) if not recording else (0, 0, 255))
                draw_button(show, buttons["RECONNECT"], "RECONNECT [C]", (70, 160, 70))
                draw_button(show, buttons["QUIT"], "QUIT [Q]", (120, 120, 120))

                try:
                    status = 'CONNECTED' if ser is not None else 'DISCONNECTED'
                    rec_flag = ' [REC]' if recording else ''
                    cv2.setWindowTitle(
                        WINDOW_NAME,
                        f'OV2640 Professional - {w}x{h} - FPS:{fps:.2f} - RX:{rx_kbps:.2f}KB/s - {status}{rec_flag}'
                    )
                except:
                    pass

                cv2.imshow(WINDOW_NAME, show)

            # 处理按钮点击
            action = mouse_state.get("clicked_action")
            if action is not None:
                mouse_state["clicked_action"] = None
                if action == "SHOT" and frame is not None:
                    save_snapshot(frame)
                elif action == "REC" and frame is not None:
                    if not recording:
                        writer, video_filename = start_recording(frame, fps)
                        if writer is not None:
                            recording = True
                            record_start_time = time.time()
                    else:
                        recording = False
                        stop_recording(writer, video_filename)
                        writer = None
                        video_filename = None
                        record_start_time = None
                elif action == "RECONNECT":
                    print('[INFO] Manual reconnect requested.')
                    if ser is not None:
                        try:
                            ser.close()
                        except:
                            pass
                    ser = None
                    last_reconnect_try = 0.0
                elif action == "QUIT":
                    break

            # 键盘
            key = cv2.waitKey(1) & 0xFF
            if key == 27 or key == ord('q'):
                break
            elif key == ord('s') and frame is not None:
                save_snapshot(frame)
            elif key == ord('r') and frame is not None:
                if not recording:
                    writer, video_filename = start_recording(frame, fps)
                    if writer is not None:
                        recording = True
                        record_start_time = time.time()
                else:
                    recording = False
                    stop_recording(writer, video_filename)
                    writer = None
                    video_filename = None
                    record_start_time = None
            elif key == ord('c'):
                print('[INFO] Manual reconnect requested.')
                if ser is not None:
                    try:
                        ser.close()
                    except:
                        pass
                ser = None
                last_reconnect_try = 0.0

    finally:
        if writer is not None:
            writer.release()
        if ser is not None:
            try:
                ser.close()
            except:
                pass
        cv2.destroyAllWindows()
        print('[INFO] Closed.')

if __name__ == '__main__':
    main()