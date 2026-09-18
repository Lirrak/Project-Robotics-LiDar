# 🤖 Project Autonomous Robot (Raspberry Pi 3B+ & ESP32 & LiDAR SLAM)

![License](https://img.shields.io/badge/License-MIT-blue.svg)
![Platform](https://img.shields.io/badge/Platform-Raspberry%20Pi%203B%2B%20%7C%20ESP32-green)
![LiDAR](https://img.shields.io/badge/Sensor-RPLiDAR%20A1M8-red)
![3D%20Print](https://img.shields.io/badge/3D%20CAD-STL%20Ready-orange)

Dự án mở thiết kế và chế tạo **Robot Tự Hành Hai Bánh Vi Sai (Differential Drive Robot)** kết hợp hệ thống đầu não xử lý **Raspberry Pi 3 Model B+** (chạy SLAM, quét bản đồ 2D, lập đường đi tối ưu AI/Nav2) và sub-controller **ESP32** (điều khiển PID tốc độ động cơ thời gian thực, đọc xung Encoder, giao tiếp UART).

---

## 🌟 Tính Năng Nổi Bật (Key Features)

- 📐 **Khung Xe 3D Chuẩn Cơ Khí (Dual-Deck 200mm)**:
  - Thiết kế 2 tầng (tầng đáy chứa động cơ, pin 12V, driver; tầng đỉnh chứa Pi 3B+ và LiDAR A1).
  - Khe khoét bánh xe rộng **76mm x 50mm** loại bỏ 100% tình trạng cấn bánh xe 65mm.
  - Sẵn sàng xuất file in 3D STL (`stl_files/`).
- 🌐 **Giao Diện Web 3D Tương Tác (`LiDar/chassis_3d_view.html`)**:
  - Trình xem 3D xoay 360° bằng Three.js, bật/tắt từng lớp linh kiện và tải file STL trực tiếp.
- ⚡ **ESP32 Real-Time Firmware (`esp32_firmware/`)**:
  - Đọc ngắt Encoder chính xác, điều khiển PID vòng kín 20Hz cho động cơ DC JGA25-370.
  - Mạch cầu H TB6612FNG và giao tiếp UART Serial chuẩn hóa gói tin `CMD,v,w` và `TELE`.
- 🐍 **Raspberry Pi Serial Bridge (`rpi_esp32_bridge.py`)**:
  - Chuyển đổi lệnh vận tốc ROS/Web UI thành xung động cơ và tính toán Odometry dead-reckoning $(x, y, \theta)$.
- 📡 **Web Radar LiDAR 2D Visualizer (`LiDar/lidar_view.html`)**:
  - Hiển thị đám mây điểm 2D từ RPLiDAR A1 thời gian thực qua WebSocket ROSBridge (`ws://<IP-Pi>:9090`).
- 🧠 **Mô Phỏng Học Máy AI / Path Simulator (`LiDar/robot_sim.html`)**:
  - Thuật toán Reinforcement Learning (Q-Learning / TensorFlow.js) huấn luyện robot né vật cản & chọn đường đi ngắn nhất.

---

## 🏗️ Kiến Trúc Hệ Thống (System Architecture)

```
                       +---------------------------------------+
                       |        RPLiDAR A1 / A2 (2D LiDAR)     |
                       +-------------------+-------------------+
                                           | USB / Serial
                                           v
                       +-------------------+-------------------+
                       |    Raspberry Pi 3 Model B+ (Brain)    |
                       |  - ROS / ROS2 (Nav2 / Cartographer)   |
                       |  - AI Path Optimization / RL Planner  |
                       |  - ROSBridge Server (ws://:9090)      |
                       +-------------------+-------------------+
                                           | UART / Serial (Command v, w)
                                           v
                       +-------------------+-------------------+
                       |           ESP32 (Sub-Controller)      |
                       |  - Motor Speed Control (PID Loop)     |
                       |  - Quadrature Encoder Interrupts      |
                       |  - MPU6050 / BNO055 IMU Reading       |
                       +-------------------+-------------------+
                                           | Dual PWM Signal
                                           v
                       +-------------------+-------------------+
                       | Motor Driver (TB6612FNG / L298N)      |
                       +-------------------+-------------------+
                                           | Power Out
                                           v
                       +-------------------+-------------------+
                       | DC Gear Motors with Encoders (2x)     |
                       +---------------------------------------+
```

---

## 📊 Bảng Vật Tư Dự Toán (Bill of Materials - BOM)

| STT | Tên Linh Kiện | Thông Số Kỹ Thuật | Số Lượng | Đơn Giá Ước Tính (VND) | Thành Tiền (VND) |
|:---|:---|:---|:---:|:---:|:---:|
| **1** | Raspberry Pi 3 Model B+ | 1.4GHz 64-bit, 1GB RAM, Wi-Fi | 1 | (Có sẵn) | 0 |
| **2** | RPLiDAR A1M8 | Bán kính quét 12m, 360 độ | 1 | (Có sẵn) | 0 |
| **3** | ESP32 DevKit V1 | Dual core 240MHz, 30-pin | 1 | 95,000 | 95,000 |
| **4** | Động Cơ DC JGA25-370 | 12V 330RPM kèm Hall Encoder | 2 | 165,000 | 330,000 |
| **5** | Mạch TB6612FNG | Dual 1.2A Motor Driver | 1 | 55,000 | 55,000 |
| **6** | Pin Li-ion 3S 12V | Pack 3S 18650 (12.6V 4400mAh) | 1 | 220,000 | 220,000 |
| **7** | Mạch Hạ Áp XL4015 | Step-down 12V -> 5V 3A | 1 | 35,000 | 35,000 |
| **8** | Bánh Xe Chính | Đường kính 65mm cao su | 2 | 25,000 | 50,000 |
| **9** | Bánh Xe Hướng | Caster wheel 15mm | 2 | 15,000 | 30,000 |
| **10**| Khung Xe In 3D (STL) | Nhựa PLA / PETG | 1 | 150,000 | 150,000 |
| **11**| Cột Đệm Brass & Vít | Hex Standoff M3x40mm + Vít M3 | 1 bộ | 40,000 | 40,000 |
| **Tổng**| | | | | **~1,040,000 VNĐ** |

---

## 📁 Cấu Trúc Thư Mục Dự Án (Project Structure)

```
Project Robot/
├── stl_files/                     # Danh sách file STL 3D dùng để in khung xe
│   ├── chassis_bottom.stl         # Tầng đáy (có khe cắt khoét bánh xe 76x50mm & khay pin)
│   ├── chassis_top.stl            # Tầng đỉnh (có lỗ chuẩn RPi 3B+ & LiDAR A1)
│   ├── motor_holder.stl           # Gá kẹp giữ động cơ JGA25 (25mm)
│   ├── lidar_mount.stl            # Chân đệm nâng cao LiDAR A1
│   └── full_robot_chassis.stl     # Mô hình 3D tổng thể hoàn chỉnh
├── esp32_firmware/
│   └── esp32_firmware.ino         # Mã nguồn Arduino C++ nạp cho ESP32
├── LiDar/
│   ├── chassis_3d_view.html       # Trình xem mô hình robot 3D tương tác 360° (Three.js)
│   ├── lidar_server.py            # WebSocket server đọc dữ liệu RPLiDAR
│   ├── lidar_view.html             # Trình quan sát 2D Radar LiDAR thời gian thực
│   └── robot_sim.html              # Trình mô phỏng huấn luyện AI Reinforcement Learning
├── generate_stl.py                # Script Python tự động tạo các file mesh STL chuẩn 3D
├── rpi_esp32_bridge.py            # Bridge Python giao tiếp Serial UART Pi <-> ESP32
└── README.md                      # Tài liệu hướng dẫn dự án
```

---

## 🚀 Hướng Dẫn Nạp & Chạy Dự Án

### 1. In 3D Khung Xe
- Tải các file `.stl` trong thư mục `stl_files/` (hoặc nhấn nút Download trực tiếp từ `LiDar/chassis_3d_view.html`).
- Sử dụng phần mềm Cura / PrusaSlicer với thiết lập khuyến nghị:
  - Độ dày lớp (Layer Height): `0.2mm`
  - Mật độ in (Infill): `25% - 30%` (Grid / Gyroid)
  - Vật liệu: PLA hoặc PETG.

### 2. Nạp Code Firmware Cho ESP32
- Mở file `esp32_firmware/esp32_firmware.ino` bằng **Arduino IDE**.
- Chọn Board: `ESP32 Dev Module`.
- Kết nối ESP32 qua cáp Micro-USB và nhấn **Upload**.

### 3. Chạy Bridge Trên Raspberry Pi 3B+
- Kết nối ESP32 với cổng USB của Raspberry Pi.
- Cài đặt thư viện Python: `pip install pyserial`
- Khởi chạy cầu nối Serial:
  ```bash
  python3 rpi_esp32_bridge.py
  ```

### 4. Quan Sát Mô Hình 3D & LiDAR Trên Web
- **Xem khung xe 3D tương tác**: Mở file `LiDar/chassis_3d_view.html` trên trình duyệt.
- **Xem quét 2D Radar LiDAR**: Mở file `LiDar/lidar_view.html` và nhập IP của Raspberry Pi (Chạy ROSBridge server `roslaunch rosbridge_server rosbridge_websocket.launch`).

---

## 📝 Giấy Phép & Tác Giả (License & Author)

- **Tác giả**: [Lirrak](https://github.com/Lirrak)
- **Repository**: [Project-Robotics-LiDar](https://github.com/Lirrak/Project-Robotics-LiDar)
- Được phát hành dưới giấy phép **MIT License**.
