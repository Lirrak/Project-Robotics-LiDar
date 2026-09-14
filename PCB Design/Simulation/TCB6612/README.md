# BÁO CÁO MÔ PHỎNG VÀ ĐÓNG GÓI MÔ HÌNH IC LÁI ĐỘNG CƠ TB6612 TRÊN KICAD SPICE

## 1. Tổng quan dự án & Mục tiêu

Dự án thực hiện xây dựng mô hình SPICE Subcircuit cho IC driver cầu H kép TB6612FNG và mô phỏng kiểm tra trên công cụ KiCad SPICE Simulator. Mục tiêu là xác minh chính xác tính đúng đắn của tầng logic điều khiển và tầng công suất MOSFET đầu ra (kênh A) trước khi đưa vào thiết kế phần cứng thực tế.

---

## 2. Quá trình thực hiện chi tiết

### Giai đoạn 1: Xây dựng Subcircuit SPICE cho TB6612

* Tạo file thư viện `TB6612.lib` định nghĩa khối điều khiển logic và các công tắc MOSFET mô phỏng (`SW_HI`, `SW_LO`).
* Thiết lập tầng logic điều khiển dựa trên nguồn phụ thuộc `E_LOGIC` để xử lý các điều kiện đầu vào: `STBY`, `PWMA`, `AIN1`, `AIN2`.

### Giai đoạn 2: Phát hiện và xử lý sự cố (Troubleshooting)

#### Lần 1: Điện áp đầu ra $V(AO1)$ bị đứng ở $6\,\text{V}$ ($VM/2$)

* **Hiện trạng:** Khi chạy mô phỏng Transient (`.TRAN`), đầu ra chân $AO1$ không phát xung mà giữ nguyên ở mức điện áp phẳng $6\,\text{V}$.
* **Nguyên nhân:**
1. Nguồn xung $PWMA$ đang được thiết lập ở $0\,\text{V}$ làm cho logic kích mở đầu ra bằng $0\,\text{V}$.
2. Mô hình cầu H ban đầu thiết lập khi $INA1 = 0\,\text{V}$ thì cả hai công tắc MOSFET trên (`S_H1`) và dưới (`S_L1`) đều **mở (High-Impedance / Hi-Z)**. Do đó, chân $AO1$ bị trôi nổi (floating) và phân áp qua điện trở rò $R_{\text{off}}$ của 2 công tắc:

$$V(AO1) = \frac{VM}{2} = \frac{12\,\text{V}}{2} = 6\,\text{V}$$




* **Khắc phục:**
* Cập nhật lại file `TB6612.lib`, cấu hình lại công tắc dưới (`S_L1`) kích theo logic ngược (`SW_LO` với $R_{\text{on}}$ nhỏ khi ngõ vào ở mức thấp) để tự động kéo chân $AO1$ xuống GND ($0\,\text{V}$) khi không có tín hiệu kích.
* Thiết lập lại tham số cho nguồn $VPULSE\_A1$ trên sơ đồ nguyên lý.



#### Lần 2: Lỗi không hiển thị đồ thị điện áp vi sai ($V_{\text{diff}}$)

* **Hiện trạng:** Thêm biểu thức vi sai $V(AO1) - V(AO2)$ trong cửa sổ **User-defined Signals** nhưng đồ thị không xuất tín hiệu.
* **Nguyên nhân:** Nhập sai cú pháp khai báo ngspice (sử dụng toán tử `:=` hoặc giữ vế gán tên `V_diff = ...` trong ô chứa biểu thức toán học).
* **Khắc phục:** Chuẩn hóa cú pháp biểu thức trong ô nhập liệu thành:
`V(Net-_U1-AO1-Pad1_) - V(Net-_U1-AO2-Pad5_)`
* **Kết quả:** Đồ thị xuất thành công dạng sóng điện áp vi sai $V_{\text{diff}}$ dao động vuông từ $0\,\text{V}$ đến $12\,\text{V}$, trùng với $V(AO1)$ (do chân $AO2$ đang được giữ ở mức $0\,\text{V}$).

---

## 3. Khó khăn, Thử thách & Bài học kỹ thuật thu được

### Bài học 1: Tại sao phải sử dụng tụ lọc nguồn $C_{\text{VCC}}$ và $C_5$ ($C_{\text{VM}}$)?

* **Lý do thiết kế:** Trong mạch lái động cơ, khi các MOSFET đóng cắt ở tần số cao (ví dụ $20\,\text{kHz}$), dòng điện bị dội đột ngột gây ra hiện tượng sụt áp dòng hoặc nhiễu sụt áp trên đường nguồn ($VCC$ và $VM$).
* **Bài học thu được:**
* Tụ $C_{\text{VCC}}$ ($100\,\text{nF}$ Ceramic) lọc nhiễu tần số cao cho tầng logic $5\,\text{V}/3.3\,\text{V}$.
* Tụ $C_5$ ($10\,\mu\text{F} - 100\,\mu\text{F}$) đóng vai trò làm tụ đệm năng lượng (Decoupling Capacitor) cho đường nguồn $VM = 12\,\text{V}$, tránh hiện tượng nhiễu dội ngược về nguồn cấp khi cuộn dây động cơ xả năng lượng.
* Trong SPICE, nếu không có các tụ này, các phép tính điểm làm việc tĩnh (OP) hoặc đáp ứng quá độ có thể xuất hiện các đỉnh sóng nhọn (spikes) giả lập gây mất ổn định bộ giải.



### Bài học 2: Kỹ thuật thiết lập tham số cho nguồn $VPULSE\_A1$

* **Lý do thiết lập:** Động cơ DC cần được điều khiển bằng xung PWM ở tần số nằm ngoài dải âm thanh nghe được ($>16\,\text{kHz}$) hoặc đủ cao để giảm độ gợn dòng điện.
* **Bài học thu được:**
* Chọn tần số PWM là $20\,\text{kHz}$ tương ứng với chu kỳ $T = 50\,\mu\text{s}$.
* Thiết lập cấu hình nguồn xung `PULSE(V1 V2 Tdelay Trise Tfall Twidth Tperiod)`:
* `V1 = 0V`, `V2 = 3.3V` (Phù hợp với mức logic của Vi điều khiển).
* `Trise = Tfall = 1ns` (Thời gian sườn lên/xuống nhỏ để hạn chế sai số tính toán của ngspice).
* `Twidth = 25us`, `Tperiod = 50us` (Tương ứng Duty Cycle $50\%$).





### Bài học 3: Quản lý trạng thái Floating (Trôi nổi) trong SPICE

* **Bài học thu được:** Trong mô phỏng SPICE, các điểm nối (Node) không được phép trôi nổi tuyệt đối (High-Impedance không có đường thoát về GND) vì bộ giải ma trận `SPARSE` sẽ tính ra giá trị lơ lửng ngẫu nhiên (như $6\,\text{V}$ đã gặp ở Lần 1). Luôn cần đảm bảo có điện trở thoát hoặc mô hình hóa công tắc kéo về GND/VCC đầy đủ.

---

## 4. Kết quả thu được

* **Mô phỏng hoàn toàn thành công:** Đã khắc phục triệt để các lỗi logic ngõ ra và lỗi cú pháp parser của ngspice.
* **Thông số dạng sóng đạt chuẩn:**
* Dạng sóng tại ngõ ra $AO1$ ($V_{\text{Net-\_U1-AO1-Pad1\_}}$) đạt chu kỳ $50\,\mu\text{s}$ ($20\,\text{kHz}$), biên độ vuông phẳng từ $0\,\text{V}$ lên $12\,\text{V}$ chuẩn xác theo xung điều khiển $PWM$.
* Điện áp vi sai đặt lên tải $V_{\text{diff}} = V(AO1) - V(AO2) = 12\,\text{V}$ đáp ứng đúng yêu cầu cấp nguồn cho động cơ chạy thuận.


* **Đóng góp dự án:** Hoàn thiện mô hình subcircuit TB6612 chuẩn cho thư viện KiCad, có thể tái sử dụng cho các bài toán thiết kế mạch nhúng điều khiển động cơ tiếp theo.
