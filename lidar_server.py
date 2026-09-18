import asyncio
import websockets
import json
import math
import time
from rplidar import RPLidar, RPLidarException

PORT_NAME = '/dev/ttyUSB0'

async def lidar_handler(websocket):
    print("Trình duyệt ROSLIB.js đã kết nối thành công!")
    lidar = None
    try:
        lidar = RPLidar(PORT_NAME, baudrate=115200, timeout=3)
        if hasattr(lidar, '_serial') and lidar._serial:
            lidar._serial.reset_input_buffer()
            
        lidar.start_motor()
        await asyncio.sleep(1)

        # 1. Đăng ký Topic /scan với hệ thống frontend
        await websocket.send(json.dumps({
            "op": "advertise",
            "topic": "/scan",
            "type": "sensor_msgs/msg/LaserScan"
        }))

        loop = asyncio.get_event_loop()
        iterator = lidar.iter_scans(max_buf_meas=1000)

        while True:
            try:
                scan = await loop.run_in_executor(None, next, iterator)
            except Exception:
                await asyncio.sleep(0.01)
                continue

            # Mảng 360 độ chứa khoảng cách (m)
            ranges = [0.0] * 360
            for item in scan:
                angle = int(round(item[1])) % 360
                dist_m = float(item[2]) / 1000.0  # mm -> m
                ranges[angle] = dist_m

            # 2. Đóng gói chuẩn ROS2 LaserScan message
            msg_payload = {
                "op": "publish",
                "topic": "/scan",
                "msg": {
                    "header": {
                        "stamp": {"sec": int(time.time()), "nanosec": 0},
                        "frame_id": "laser"
                    },
                    "angle_min": 0.0,
                    "angle_max": 6.283185307179586,  # 2 * PI
                    "angle_increment": 0.017453292519943295, # 1 độ sang rad
                    "time_increment": 0.0,
                    "scan_time": 0.1,
                    "range_min": 0.15,
                    "range_max": 12.0,
                    "ranges": ranges,
                    "intensities": [100.0] * 360
                }
            }

            await websocket.send(json.dumps(msg_payload))
            await asyncio.sleep(0.08)

    except Exception as e:
        print(f"Lỗi LiDAR: {e}")
    finally:
        if lidar:
            try:
                lidar.stop()
                lidar.stop_motor()
                lidar.disconnect()
            except:
                pass
        print("Đã ngắt kết nối client.")

async def main():
    async with websockets.serve(lidar_handler, "0.0.0.0", 9090):
        print("Server ROSBridge giả lập đang chạy tại ws://0.0.0.0:9090 ...")
        await asyncio.Future()

if __name__ == "__main__":
    asyncio.run(main())

