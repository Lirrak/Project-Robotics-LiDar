# README — Quá trình thực hiện schematic STM32F103C8T6 + TB6612FNG (điều khiển động cơ GA25)

Tài liệu này ghi lại toàn bộ quá trình thiết kế, review, và mô phỏng schematic cho module điều khiển động cơ GA25 dùng STM32F103C8T6 + driver TB6612FNG — dùng làm nhật ký kỹ thuật (design log) để tra cứu lại sau này.

---

## 1. Mục tiêu dự án

Thiết kế 1 mạch PCB tích hợp:
- MCU: **STM32F103C8T6** (LQFP48)
- Driver động cơ: **TB6612FNG** (SSOP24)
- Tải: động cơ DC giảm tốc **GA25** (tối đa 2 động cơ, kênh A và B)
- Đầy đủ các khối phụ trợ chuẩn: nguồn, reset, boot, decoupling — không chỉ đấu dây tín hiệu.

---

## 2. Kiến trúc tổng thể

```
Header SWD ──▶ STM32F103C8T6 ──▶ (PWMA/PWMB, AIN1/AIN2, BIN1/BIN2, STBY) ──▶ TB6612FNG ──▶ Động cơ GA25 (A & B)
                    ▲                                                            ▲
             Nguồn logic 3.3V                                          Nguồn VM 6–12V
             + Reset/Boot                                               + nguồn VCC 5V
```

7 tín hiệu điều khiển và mapping chân đã chốt:

| STM32 | Chân | → | TB6612 | Chân |
|---|---|---|---|---|
| PA6 | 16 | → | PWMA | 23 |
| PA0 | 10 | → | AIN1 | 21 |
| PA1 | 11 | → | AIN2 | 22 |
| PA7 | 17 | → | PWMB | 15 |
| PA2 | 12 | → | BIN1 | 17 |
| PA3 | 13 | → | BIN2 | 16 |
| PB0 | 18 | → | STBY | 19 |

---

## 3. Các khối mạch và giá trị linh kiện đã chốt

| Khối | Linh kiện | Giá trị | Lý do chính |
|---|---|---|---|
| Nguồn logic STM32 | C1, C2, C3 | 100nF (mỗi chân VDD/VBAT/VDDA một tụ riêng) | Mỗi chân VDD nuôi 1 vùng die riêng — không dùng chung 1 tụ được |
| Reset (NRST) | R1 | 10k pull-up lên 3.3V | NRST không có pull nội đủ mạnh, tránh floating |
| Reset (NRST) | SW1 | Nút nhấn | Reset thủ công |
| Reset (NRST) | CNRST | 100nF | Chống dội nút nhấn + trễ reset khi cấp nguồn (τ = R1×C ≈ 1ms) |
| Boot mode | R-Boot | 10k pull-down | BOOT0 không có pull nội — bắt buộc phải có để mặc định boot từ Flash |
| Nguồn logic TB6612 | C_VCC | 100nF | Lọc nhiễu logic tần số cao, mắc song song VCC–GND (không phải nối tiếp) |
| Nguồn logic TB6612 | VCC | +5V | Trong khoảng cho phép 2.7–5.8V theo datasheet TB6612 |
| Nguồn động cơ TB6612 | C5 | 100µF | Bulk cap chống sụt áp khi motor kéo dòng lớn lúc khởi động/đảo chiều |
| Nguồn động cơ TB6612 | VM | +12V | Nguồn cấp riêng cho cầu H, tách hoàn toàn khỏi VCC |

**Nguyên tắc cốt lõi rút ra:** tách nguồn logic (VCC) và nguồn công suất (VM) hoàn toàn; tách GND logic và GND công suất (GND vs PGND) nhưng gộp về đúng 1 điểm chung (star grounding).

---

## 4. Các lỗi thực tế đã phát hiện trong quá trình review schematic

Đây là các lỗi thật đã bắt được khi review từng bản vẽ của người dùng qua nhiều vòng — ghi lại để tránh lặp lại:

1. **VCC vô tình gần net +12V** — nguy cơ nối nhầm VCC (logic) vào VM (12V), sẽ làm cháy TB6612 ngay khi cấp nguồn vì vượt quá điện áp tối đa cho phép.
2. **STBY bị thả nổi** — không nối dây, khiến TB6612 ở trạng thái không xác định.
3. **R-Boot chưa gán giá trị** — hiển thị "R" thay vì "10k", dễ bị bỏ sót khi ra file BOM.
4. **Tụ C_VCC mắc nối tiếp thay vì song song** — lỗi nguyên lý cơ bản (tụ chặn DC, mắc nối tiếp thì chân VCC không bao giờ nhận được điện áp DC thật).
5. **Điện áp thử đầu tiên cho VCC là +7V** — vượt/sát giới hạn tuyệt đối tối đa của TB6612, đã đổi về +5V.
6. **VM3 (chân 14) thả nổi** — chỉ VM1/VM2 được nối, quên nối VM3 chung.
7. **PGND1, PGND2 thả nổi** — chỉ nối GND (18), quên 2 chân hồi dòng công suất PGND1 (3) và PGND2 (9).

Tất cả các lỗi trên đã được xác nhận sửa đúng qua các vòng review tiếp theo (kiểm tra bằng mắt + khuyến nghị chạy ERC trong KiCad để xác nhận lần cuối).

---

## 5. Mô phỏng SPICE (ngspice) — xác nhận bằng số liệu

Ba mạch con đã được dựng netlist SPICE và mô phỏng thực tế (không phải minh họa) để kiểm chứng lý do chọn giá trị linh kiện:

1. **Reset RC (R1=10k, CNRST=100nF):** xác nhận NRST trễ theo VDD với τ ≈ 1ms, đạt ~3.3V sau khoảng 5ms.
2. **Tụ lọc VDD (C1/C2/C3):** với cùng 1 xung dòng chuyển mạch — có tụ 100nF thì VDD dao động nhẹ (3.24–3.31V); không có tụ thì dao động dữ dội (2.1V–4.8V), đủ gây reset ngẫu nhiên.
3. **Tụ C5 lọc VM:** với xung dòng motor 1.5A — có C5 100µF thì điện áp giảm êm về ~11.55V; không có C5 thì sụt xuống 11.29V kèm dao động ngược trước khi ổn định.

File `.cir` gốc và hướng dẫn tự viết lại từ đầu (không dùng file dựng sẵn) đã được cung cấp riêng trong quá trình trao đổi.

---

## 6. Nỗ lực dựng file `.kicad_sch` bằng script (tự động hoá) — phát hiện kỹ thuật quan trọng

Trong quá trình thử dựng file schematic KiCad thật bằng Python (đọc trực tiếp thư viện symbol chuẩn của KiCad rồi ghép thành file `.kicad_sch`), đã gặp và xử lý các lỗi sau — đáng chú ý vì đây là các lỗi *không tài liệu hoá rõ ràng* của định dạng `.kicad_sch`:

### Lỗi 1: `extends` không được hỗ trợ trong cache `lib_symbols` của file `.kicad_sch`
Symbol `STM32F103C8Tx` trong thư viện gốc dùng cơ chế kế thừa (`extends "STM32F103C_8-B_Tx"`) để tái sử dụng hình vẽ/chân từ 1 symbol gốc khác. Khi copy nguyên cả 2 block (cha + con) vào `lib_symbols` của file `.kicad_sch`, `kicad-cli` báo "Failed to load schematic file" ngay lập tức.
**Cách khắc phục:** phải "làm phẳng" (flatten) — hợp nhất phần đồ hoạ/chân của symbol cha với phần thuộc tính (properties) của symbol con thành 1 symbol độc lập, không còn `extends`.

### Lỗi 2: Tên symbol trong cache `lib_symbols` phải có đầy đủ tiền tố thư viện
Đây là phát hiện quan trọng nhất, gây ra lỗi **Segmentation fault** (không phải lỗi parse thông thường) rất khó truy — đã tốn nhiều vòng bisect mới tìm ra:

- Khi copy symbol trực tiếp từ file thư viện gốc (ví dụ `power.kicad_sym`), symbol được khai báo là `(symbol "GND" (power) ...)` — chỉ có tên bare, không có tiền tố.
- Nhưng khi symbol đó được dùng làm cache trong 1 file `.kicad_sch` thật (đối chiếu với các file demo chính thức của KiCad), tên cache **bắt buộc phải là `"power:GND"`** — tức là đúng bằng chuỗi `lib_id` mà symbol instance tham chiếu tới (`(symbol (lib_id "power:GND") ...)`), **bao gồm cả tiền tố thư viện**.
- Nếu tên cache không khớp chính xác với `lib_id` (thiếu tiền tố), `kicad-cli` không báo lỗi cú pháp rõ ràng mà **crash thẳng (segfault)** — đây là hành vi dễ gây nhầm lẫn vì trông như file bị hỏng nặng, trong khi thực ra chỉ sai đúng 1 chuỗi tên.

**Quy tắc rút ra:** mọi symbol trong `lib_symbols` của 1 file `.kicad_sch` phải được đặt tên đúng bằng `"<TênThưViện>:<TênSymbol>"`, kể cả khi các sub-unit đi kèm (ví dụ `_0_1`, `_1_1` cho các symbol nhiều style/unit) — các sub-unit cũng phải đổi tên theo tiền tố mới tương ứng.

### Trạng thái tại thời điểm ghi tài liệu này
Đã xác nhận sửa đúng và test thành công với các symbol đơn giản (`power:GND`, và đang áp dụng lại cho toàn bộ symbol còn lại: `Device:R`, `Device:C`, `Simulation_SPICE:VPWL`, `Driver_Motor:TB6612FNG`, `MCU_ST_STM32F1:STM32F103C8Tx`...). Việc dựng lại toàn bộ file `.kicad_sch` chính (schematic đầy đủ 2 IC) bằng script với fix này **chưa hoàn tất tại thời điểm viết tài liệu** — đây là việc cần tiếp tục ở phiên làm việc sau nếu muốn có file `.kicad_sch` sinh tự động hoàn chỉnh.

---

## 7. Trạng thái hiện tại & việc cần làm tiếp

- [x] Sơ đồ khối tổng thể
- [x] Mapping chân điều khiển STM32 ↔ TB6612
- [x] Schematic chi tiết (vẽ tay trong KiCad bởi người dùng, đã qua nhiều vòng review và sửa lỗi)
- [x] Xác nhận nguyên lý bằng mô phỏng SPICE cho 3 mạch con quan trọng (reset, decoupling VDD, decoupling VM)
- [ ] Chạy ERC (Electrical Rules Checker) lần cuối trên toàn bộ schematic thật trong KiCad để xác nhận không còn pin nào thả nổi
- [ ] Hoàn thiện script tự động sinh file `.kicad_sch` (áp dụng fix tiền tố tên symbol ở mục 6 cho toàn bộ linh kiện còn lại)
- [ ] Layout PCB: bố trí linh kiện theo floorplan đã đề xuất (khối nguồn tách biệt, decoupling cap sát chân IC, GND/PGND star-grounding), đi dây, chạy DRC
- [ ] Xuất Gerber / BOM / Pick-and-place cho sản xuất

---

*Tài liệu này được tổng hợp lại từ quá trình trao đổi thiết kế thực tế — dùng làm tài liệu tham chiếu nội bộ, không thay thế cho datasheet chính thức của STM32F103C8T6 hay TB6612FNG.*
