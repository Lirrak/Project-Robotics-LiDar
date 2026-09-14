### Tại sao cần tạo thư mục mô phỏng mạch (PCB Simulation)?

1. **Xác minh thiết kế trước khi sản xuất (Shift-Left Testing):**
   - Giúp kiểm tra tính đúng đắn của sơ đồ nguyên lý (Schematic) và mô hình linh kiện (SPICE subcircuit) ngay từ giai đoạn đầu.
   - Phát hiện sớm các lỗi về điện áp, phân áp trôi nổi (floating node) hoặc sai logic điều khiển trước khi đặt làm mạch in thực tế, tránh lãng phí chi phí và thời gian làm lại PCB (re-spin).

2. **Tối ưu hóa thông số và dải hoạt động:**
   - Dễ dàng kiểm tra các dạng sóng điện áp, dòng điện quá độ (Transient analysis) tại các chân ngõ ra (như $AO1$, $AO2$).
   - Phân tích đáp ứng tần số đóng cắt (PWM), tính toán công suất tiêu hao và kiểm tra khả năng đáp ứng của tải (như động cơ DC, cuộn cảm $RL$).

3. **Lưu trữ và Quản lý tài nguyên dự án tập trung:**
   - Quản lý riêng biệt các file thư viện linh kiện tùy chỉnh (`.lib`, `.subckt`), file netlist và cấu hình nguồn tín hiệu kích ($VPULSE$).
   - Đảm bảo tính đóng gói (modularity), giúp dễ dàng tái sử dụng mô hình mô phỏng cho các dự án phát triển phần cứng hoặc firmware tiếp theo.
