# Chạy ROS2 Jazzy trong Docker trên Debian 13

Tài liệu này hướng dẫn chạy ROS2 cho dự án Robot LiDAR trên Raspberry Pi 3B+, trong khi vẫn giữ hệ điều hành Debian 13. ROS2 chạy trong Docker container Ubuntu 24.04 arm64, không cài trực tiếp package ROS vào Debian host.

## 1. Kiến trúc hiện tại

```text
Debian 13 (host)
  +-- Docker
       +-- Ubuntu 24.04 arm64
            +-- ROS2 Jazzy
                 +-- rplidar_ros
                 +-- rosbridge_suite
                 +-- laser_filters
```

Luồng LiDAR sau khi chuyển sang ROS2:

```text
RPLiDAR A1
  | USB: /dev/ttyUSB0
  v
rplidar_ros (ROS2 node)
  |
  +-- /scan: sensor_msgs/msg/LaserScan
  v
rosbridge_server
  |
  +-- WebSocket :9090
  v
LiDar/lidar_view.html
```

## 2. Vì sao dùng ROS2 trong Docker?

Máy đang chạy Debian 13 (trixie), kiến trúc `arm64`/`aarch64`. Debian hiện tại không có package `ros-jazzy-ros-base` trong apt repository mặc định, vì vậy lệnh sau bị lỗi:

```text
Unable to locate package ros-jazzy-ros-base
```

Không nên thêm repository Ubuntu ROS trực tiếp vào Debian. Docker cho phép giữ nguyên Debian, nhưng chạy ROS2 Jazzy trong môi trường Ubuntu 24.04 tương thích.

ROS2 được chọn thay vì ROS1 vì:

- Dự án định hướng Nav2, SLAM Toolbox và hệ sinh thái ROS2.
- Frontend đang dùng message type ROS2: `sensor_msgs/msg/LaserScan`.
- ROS1 Noetic đã hết vòng đời hỗ trợ chính thức.

## 3. Kiểm tra hệ thống Debian host

Các lệnh dưới đây chạy trên Debian, bên ngoài container:

```bash
cat /etc/os-release
uname -m
ls -l /dev/ttyUSB0
id
```

Kết quả mong đợi:

```text
Debian GNU/Linux 13 (trixie)
aarch64
/dev/ttyUSB0 tồn tại
user thuộc group dialout
```

Nếu user chưa thuộc group `dialout`:

```bash
sudo usermod -aG dialout "$USER"
```

Sau đó đăng xuất rồi đăng nhập lại.

## 4. Cài và kiểm tra Docker

Cài Docker từ repository Debian:

```bash
sudo apt update
sudo apt install -y docker.io
sudo systemctl enable --now docker
sudo usermod -aG docker "$USER"
```

Đăng xuất và đăng nhập lại để group `docker` có hiệu lực. Kiểm tra:

```bash
docker version
systemctl is-active docker
id
```

Nếu shell hiện tại chưa nhận group mới, có thể dùng tạm:

```bash
sudo docker version
```

Không đưa mật khẩu sudo vào script, README hoặc lệnh tự động.

## 5. Tải image ROS2 Jazzy

Image chính thức có sẵn cho arm64:

```bash
docker pull ros:jazzy-ros-base
```

Kiểm tra image:

```bash
docker images ros
```

## 6. Tạo container ROS2

Chạy lệnh này tại thư mục gốc của project:

```bash
docker rm -f ros2-lidar 2>/dev/null || true

docker run -d \
  --name ros2-lidar \
  --network host \
  --device=/dev/ttyUSB0 \
  --group-add "$(getent group dialout | cut -d: -f3)" \
  -v "$PWD:/workspace/Project-Robotics-LiDar" \
  ros:jazzy-ros-base \
  sleep infinity
```

Giải thích các tùy chọn:

- `--name ros2-lidar`: đặt tên container.
- `--network host`: container dùng network của host, thuận tiện cho WebSocket port `9090`.
- `--device=/dev/ttyUSB0`: truyền thiết bị RPLiDAR vào container.
- `--group-add ...`: cấp group `dialout` để đọc serial.
- `-v ...`: mount project vào `/workspace/Project-Robotics-LiDar`.
- `sleep infinity`: giữ container chạy để có thể truy cập nhiều lần.

Kiểm tra container:

```bash
docker ps --filter name=ros2-lidar

docker exec ros2-lidar ls -l /dev/ttyUSB0
```

## 7. Truy cập container

Mở shell tương tác trong container:

```bash
docker exec -it ros2-lidar bash
```

Sau khi vào container, luôn source ROS2:

```bash
source /opt/ros/jazzy/setup.bash
```

Nếu muốn ROS2 được nạp tự động mỗi lần mở shell trong container:

```bash
echo 'source /opt/ros/jazzy/setup.bash' >> /root/.bashrc
source /root/.bashrc
```

Kiểm tra môi trường:

```bash
echo "$ROS_VERSION"
echo "$ROS_DISTRO"
command -v ros2
```

Kết quả mong đợi:

```text
2
jazzy
/opt/ros/jazzy/bin/ros2
```

Thoát container:

```bash
exit
```

Chạy một lệnh bên trong container mà không mở shell:

```bash
docker exec ros2-lidar bash -lc \
  'source /opt/ros/jazzy/setup.bash && ros2 node list'
```

## 8. Cài package ROS2 cần thiết

Trong container:

```bash
apt-get update
apt-get install -y \
  ros-jazzy-rplidar-ros \
  ros-jazzy-rosbridge-suite \
  ros-jazzy-laser-filters
```

Kiểm tra package:

```bash
source /opt/ros/jazzy/setup.bash

for p in rplidar_ros rosbridge_suite rosbridge_server laser_filters; do
  printf "%s: " "$p"
  ros2 pkg prefix "$p" 2>/dev/null || echo MISSING
done
```

Kết quả đúng là mỗi package trả về:

```text
/opt/ros/jazzy
```

## 9. Chạy driver RPLiDAR bằng ROS2

Trước khi chạy driver, cần đảm bảo không có chương trình khác đang truy cập `/dev/ttyUSB0`.

Trong một terminal host, dừng server Python cũ nếu nó đang chạy:

```bash
pkill -f '/home/lirrak/Project-Robotics-LiDar/LiDar/lidar_server.py' || true
sudo systemctl stop lidar-server.service 2>/dev/null || true
```

Mở shell ROS2:

```bash
docker exec -it ros2-lidar bash -lc \
  'source /opt/ros/jazzy/setup.bash && exec bash'
```

Chạy driver:

```bash
ros2 launch rplidar_ros rplidar.launch.py
```

Launch file `rplidar.launch.py` trong package Jazzy đã cấu hình sẵn cho RPLiDAR A1/A2:

- Serial port: `/dev/ttyUSB0`
- Baudrate: `115200`
- Frame: `laser`

Giữ terminal này mở. Nếu cần kiểm tra các launch file có sẵn:

```bash
find /opt/ros/jazzy/share/rplidar_ros -type f -name '*.launch.py' -o -name '*.launch.xml'
```

Lưu ý: package Jazzy hiện tại không có file `rplidar_a1_launch.py`. Không dùng tên file đó; dùng `rplidar.launch.py` cho RPLiDAR A1.

## 10. Kiểm tra node và topic `/scan`

Mở terminal thứ hai trên host:

```bash
docker exec -it ros2-lidar bash -lc \
  'source /opt/ros/jazzy/setup.bash && exec bash'
```

Kiểm tra node:

```bash
ros2 node list
```

Kiểm tra topic:

```bash
ros2 topic list
ros2 topic type /scan
ros2 topic hz /scan
```

Đọc thử một message:

```bash
ros2 topic echo /scan --once
```

Kết quả cần có:

```text
/scan
sensor_msgs/msg/LaserScan
```

Ở giai đoạn này `/scan` là topic ROS2 thật, được publish bởi `rplidar_ros`. Nó khác với server Python cũ, vốn chỉ gửi JSON có hình dạng ROS message qua WebSocket.

## 11. Chạy ROSBridge

Chỉ chạy ROSBridge sau khi `/scan` thật đã hoạt động.

Trong container, mở terminal mới hoặc dùng một shell khác:

```bash
docker exec -it ros2-lidar bash
source /opt/ros/jazzy/setup.bash
ros2 launch rosbridge_server rosbridge_websocket_launch.xml
```

ROSBridge thường mở WebSocket tại:

```text
ws://<IP-cua-Raspberry-Pi>:9090
```

Frontend hiện tại trong `LiDar/lidar_view.html` đang kết nối tới port `9090` và subscribe:

```text
/scan
sensor_msgs/msg/LaserScan
```

Do đó có thể giữ cách kết nối WebSocket hiện tại sau khi thay server Python bằng ROSBridge thật.

## 12. Chạy laser_filters

Chỉ thêm filter sau khi topic `/scan` thô đã ổn định. Kiểm tra package:

```bash
ros2 pkg prefix laser_filters
```

Tạo file cấu hình filter trong workspace, ví dụ:

```yaml
scan_filter_chain:
  - name: range_filter
    type: laser_filters/LaserScanRangeFilter
    params:
      lower_threshold: 0.15
      upper_threshold: 12.0
      lower_replacement_value: .inf
      upper_replacement_value: .inf
```

Nên publish kết quả ra `/scan_filtered` trong giai đoạn kiểm thử, giữ `/scan` nguyên bản để dễ so sánh và debug.

## 13. LiDAR server Python cũ

File cũ là:

```text
LiDar/lidar_server.py
```

File này:

- Đọc trực tiếp `/dev/ttyUSB0` bằng thư viện Python `rplidar`.
- Tự đóng gói JSON với topic `/scan`.
- Mở WebSocket port `9090`.
- Không tạo ROS2 node thật.

Service cũ:

```text
LiDar/lidar-server.service
```

Không chạy đồng thời server Python và ROSBridge vì cả hai đều dùng port `9090`, đồng thời cả hai có thể tranh quyền truy cập `/dev/ttyUSB0`.

Kiểm tra process và port:

```bash
ps auxww | grep -E '[l]idar|[r]os'
ss -lntup | grep -E ':9090|:8000'
```

## 14. Các lỗi thường gặp

### `Unable to locate package ros-jazzy-ros-base`

Đang chạy lệnh trên Debian host. Package ROS2 Jazzy được cài bên trong container Ubuntu, không phải apt của Debian:

```bash
docker exec -it ros2-lidar bash
apt-get update
apt-get install -y ros-jazzy-ros-base
```

Image `ros:jazzy-ros-base` đã có sẵn ROS2 base, nên thường chỉ cần cài các package bổ sung.

### `ros2: error: unrecognized arguments: --version`

`ros2` không hỗ trợ lệnh:

```bash
ros2 --version
```

Dùng:

```bash
echo "$ROS_DISTRO"
echo "$ROS_VERSION"
ros2 pkg prefix rclpy
ros2 doctor
```

### Port `9090` đã được sử dụng

Kiểm tra:

```bash
ss -lntup | grep ':9090'
```

Dừng server cũ trước khi chạy ROSBridge:

```bash
pkill -f lidar_server.py || true
sudo systemctl stop lidar-server.service
```

### Không thấy `/dev/ttyUSB0` trong container

Kiểm tra host:

```bash
ls -l /dev/ttyUSB0
```

Nếu thiết bị tồn tại, tạo lại container với:

```bash
--device=/dev/ttyUSB0
--group-add "$(getent group dialout | cut -d: -f3)"
```

### Container đã dừng

Kiểm tra:

```bash
docker ps -a --filter name=ros2-lidar
```

Khởi động lại:

```bash
docker start ros2-lidar
```

Vào lại:

```bash
docker exec -it ros2-lidar bash
```

### Xóa và tạo lại container

Dữ liệu trong workspace được mount từ host nên không mất khi xóa container:

```bash
docker rm -f ros2-lidar
```

Sau đó chạy lại lệnh `docker run` ở mục 6.

## 15. Lệnh quản lý nhanh

```bash
# Xem container đang chạy
docker ps

# Xem log container
docker logs ros2-lidar

# Vào container
docker exec -it ros2-lidar bash

# Dừng container
docker stop ros2-lidar

# Khởi động container
docker start ros2-lidar

# Xóa container
docker rm -f ros2-lidar
```

## 16. Trạng thái đã hoàn thành

Các bước sau đã được thực hiện và xác nhận:

- Debian 13 host vẫn được giữ nguyên.
- Docker daemon đã cài và đang chạy.
- Kiến trúc máy là `aarch64`/`arm64`.
- Image `ros:jazzy-ros-base` đã tải thành công.
- Container `ros2-lidar` đã được tạo.
- `/dev/ttyUSB0` đã được truyền vào container.
- ROS2 Jazzy đã hoạt động trong container.
- Các package đã được xác nhận:
  - `rplidar_ros`
  - `rosbridge_suite`
  - `rosbridge_server`
  - `laser_filters`

Bước tiếp theo là chạy `rplidar_ros`, xác nhận `/scan` bằng `ros2 topic echo /scan --once`, rồi mới chạy ROSBridge trên port `9090`.
