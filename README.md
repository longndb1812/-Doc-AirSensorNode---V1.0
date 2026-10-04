# AirSensorNode Firmware - User Guide

Tài liệu này dành cho người **sử dụng firmware AirSensorNode** qua giao tiếp **RS-485 / Modbus RTU**.

---

## 1. Modbus Register Map

| Register | Sensor | Giá trị | Dữ liệu trong Register | Quy đổi |
|---|---|---|---:|---|
| `0x0000` | BH1750 | Độ sáng | Lux × 10 | `Lux = Register / 10.0` |
| `0x0001` | BMP280 | Nhiệt độ | °C × 100 | `Temperature = Register / 100.0` °C |
| `0x0002` | BMP280 | Áp suất | hPa × 10 | `Pressure = Register / 10.0` hPa |
| `0x0003` | SCD4x | CO₂ | ppm | `CO2 = Register` ppm |
| `0x0004` | SCD4x | Nhiệt độ | °C × 100 | `Temperature = Register / 100.0` °C |
| `0x0005` | SCD4x | Độ ẩm tương đối | %RH × 100 | `Humidity = Register / 100.0` %RH |

Firmware cung cấp **6 Input Registers liên tiếp**, từ `0x0000` đến `0x0005`.

- Protocol: **Modbus RTU**
- Physical interface: **RS-485**
- Function đọc dữ liệu: **`0x04` - Read Input Registers**
- Có thể đọc từng thanh ghi hoặc đọc cả 6 thanh ghi trong một request.
- Gateway đọc dữ liệu gần nhất đã được firmware lưu trong Input Registers.
- Request từ Gateway **không kích hoạt một phép đo cảm biến mới**.

---

## 2. Đọc toàn bộ dữ liệu

Khuyến nghị Gateway đọc cả 6 thanh ghi trong một request.

```text
Function      : 0x04
Start Address : 0x0000
Quantity      : 6
```

Ví dụ với **Slave ID = 2**:

```text
02 04 00 00 00 06 70 3B
```

Giải thích:

```text
02          Slave ID = 2
04          Read Input Registers
00 00       Start Address = 0x0000
00 06       Quantity = 6 Registers
70 3B       CRC16 Modbus
```

Response có dạng:

```text
02 04 0C RR RR RR RR RR RR RR RR RR RR RR RR CRC CRC
```

Trong đó:

```text
02       Slave ID
04       Function
0C       12 Data Bytes

RR RR    Register 0 - BH1750 Lux
RR RR    Register 1 - BMP280 Temperature
RR RR    Register 2 - BMP280 Pressure
RR RR    Register 3 - SCD4x CO2
RR RR    Register 4 - SCD4x Temperature
RR RR    Register 5 - SCD4x Humidity

CRC CRC  CRC16 Modbus
```

---
# 3. Cấu hình Slave ID

Slave ID được cấu hình trực tiếp trên PCB tại 3 vị trí resistor **R1, R2, R3**.

<img width="214" height="149" alt="image" src="https://github.com/user-attachments/assets/66d97964-b132-46fc-b051-3427f45a6200" />

## Cách xác định HIGH / LOW

Nhìn board theo đúng chiều của hình minh họa:

| Cách hàn | Trạng thái | Giá trị |
|---|---|---:|
| Hàn resistor **theo chiều dọc** | HIGH | `1` |
| Hàn resistor **theo chiều ngang** | LOW | `0` |
| **Không hàn resistor** | LOW mặc định | `0` |

> Các chân Slave ID được cấu hình **GPIO Input + Pull-down**, vì vậy vị trí không hàn resistor sẽ được đọc là `0`.

---

## Mapping R1 / R2 / R3

| PCB | STM32 Pin | Firmware Signal | Bit | Trọng số |
|---|---|---|---|---:|
| `R1` | `PB2` | `SLAVE_ID_2` | Bit 2 - MSB | 4 |
| `R2` | `PB10` | `SLAVE_ID_1` | Bit 1 | 2 |
| `R3` | `PB11` | `SLAVE_ID_0` | Bit 0 - LSB | 1 |

Slave ID được xác định theo:

```text
Slave ID raw = (R1 × 4) + (R2 × 2) + R3
```

Trong đó:

```text
Dọc        = 1
Ngang      = 0
Không hàn  = 0
```

---

# 4. Bảng chọn Slave ID

Để cấu hình nhanh, chỉ cần nhìn trạng thái **R1 → R2 → R3** và tra bảng:

| Slave ID | R1 | R2 | R3 | Cách hàn R1 → R2 → R3 |
|---:|:---:|:---:|:---:|---|
| **1 (Default)** | NC | NC | NC | Không hàn cả 3 |
| **1** | 0 | 0 | 0 | Ngang - Ngang - Ngang |
| **1** | 0 | 0 | 1 | Ngang - Ngang - **Dọc** |
| **2** | 0 | 1 | 0 | Ngang - **Dọc** - Ngang |
| **3** | 0 | 1 | 1 | Ngang - **Dọc** - **Dọc** |
| **4** | 1 | 0 | 0 | **Dọc** - Ngang - Ngang |
| **5** | 1 | 0 | 1 | **Dọc** - Ngang - **Dọc** |
| **6** | 1 | 1 | 0 | **Dọc** - **Dọc** - Ngang |
| **7** | 1 | 1 | 1 | **Dọc** - **Dọc** - **Dọc** |

**Quy ước:**

```text
1  = HIGH = Hàn dọc
0  = LOW  = Hàn ngang
NC = Không hàn = LOW mặc định
```

---

## Slave ID mặc định

Nếu **không hàn R1, R2 và R3**:

```text
R1 = 0
R2 = 0
R3 = 0

→ 000
→ Slave ID raw = 0
→ Firmware sử dụng Slave ID = 1
```

Firmware **không sử dụng Slave ID 0**.

Bất cứ khi nào đọc được:

```text
R1 R2 R3 = 000
```

firmware sẽ tự chuyển:

```text
Slave ID 0 → Slave ID 1
```

Do đó Slave ID `1` có hai cấu hình bit:

```text
000 → ID 1
001 → ID 1
```

và khi không hàn bất kỳ resistor cấu hình nào:

```text
NC - NC - NC → 000 → ID 1 (Default)
```

---

## Ví dụ cấu hình

### Slave ID = 2

```text
ID = 010

R1 = 0 → Hàn ngang
R2 = 1 → Hàn dọc
R3 = 0 → Hàn ngang
```

### Slave ID = 5

```text
ID = 101

R1 = 1 → Hàn dọc
R2 = 0 → Hàn ngang
R3 = 1 → Hàn dọc
```

---

## Lưu ý

- Nên **tắt nguồn board** trước khi thay đổi resistor cấu hình Slave ID.
- Sau khi thay đổi Slave ID, **reset hoặc cấp nguồn lại board** để firmware đọc cấu hình mới.
- Không cấu hình hai node trên cùng bus RS-485 với cùng Slave ID.
- Slave ID hợp lệ của firmware hiện tại là **1 đến 7**.


## 5. Lưu ý khi cấu hình Slave ID

Không được hàn đồng thời resistor:

```text
3.3V → SLAVE_ID_x
```

và:

```text
SLAVE_ID_x → GND
```

trên **cùng một bit**.

Điều này có thể tạo đường dẫn dòng điện trực tiếp từ `3.3V` xuống `GND` qua các resistor.

Nên thay đổi resistor cấu hình Slave ID khi board **đã tắt nguồn**.

Sau khi thay đổi Slave ID:

```text
Thay đổi resistor
        ↓
Cấp nguồn lại / Reset board
        ↓
Firmware đọc Slave ID mới
```

---

# 6. Thời gian lấy mẫu cảm biến

Các cảm biến được firmware đọc **độc lập với request Modbus của Gateway**.

| Sensor | Giá trị | Chu kỳ hiện tại |
|---|---|---:|
| BH1750 | Light | ~500 ms |
| BMP280 | Temperature + Pressure | ~500 ms |
| SCD4x | Check Data Ready | ~1 s |
| SCD4x | CO₂ + Temperature + Humidity sample mới | ~5 s |

### BH1750

Giá trị ánh sáng được cập nhật khoảng:

```text
500 ms / lần
```

tương đương khoảng:

```text
2 samples/s
```

### BMP280

Nhiệt độ và áp suất được cập nhật khoảng:

```text
500 ms / lần
```

tương đương khoảng:

```text
2 samples/s
```

### SCD4x

SCD4x chạy ở:

```text
Periodic Measurement Mode
```

Firmware kiểm tra:

```text
Data Ready
```

khoảng:

```text
1 s / lần
```

Tuy nhiên, việc kiểm tra mỗi 1 giây **không có nghĩa SCD4x tạo sample mới mỗi 1 giây**.

Ở Periodic Measurement Mode, dữ liệu mới của SCD4x được cập nhật khoảng:

```text
5 s / sample
```

Do đó có thể hình dung:

```text
0 s      Check
1 s      Check
2 s      Check
3 s      Check
4 s      Check
5 s      Data Ready → Read new sample

6 s      Check
7 s      Check
8 s      Check
9 s      Check
10 s     Data Ready → Read new sample
```

---

# 7. Lưu ý SCD4x sau khi cấp nguồn

SCD4x **không có sample mới ngay lập tức sau khi board vừa được cấp nguồn**.

Khi firmware khởi động:

```text
Power ON
    ↓
Initialize I2C
    ↓
Start SCD4x Periodic Measurement
    ↓
SCD4x bắt đầu chu kỳ đo
    ↓
Chờ sample đầu tiên
    ↓
Update Modbus Registers
```

SCD4x ở Periodic Measurement Mode có chu kỳ cập nhật tín hiệu khoảng:

```text
5 giây
```

Do đó sau khi vừa cấp nguồn, cần chờ cảm biến tạo sample đầu tiên trước khi coi các thanh ghi:

```text
0x0003  SCD4x CO2
0x0004  SCD4x Temperature
0x0005  SCD4x Humidity
```

là dữ liệu đo mới.

Ngoài thời gian để có sample đầu tiên, giá trị CO₂ cũng có thể cần thêm thời gian để ổn định sau khi cảm biến vừa được cấp nguồn hoặc khi điều kiện môi trường thay đổi mạnh.

Nếu ứng dụng yêu cầu độ chính xác CO₂ cao ngay sau khi khởi động, không nên sử dụng ngay những giá trị đầu tiên cho các quyết định quan trọng.

---

# 8. Cách Gateway đọc dữ liệu

Gateway có thể gửi request Modbus **bất kỳ lúc nào**.

Gateway không cần chờ đúng thời điểm cảm biến thực hiện phép đo.

Kiến trúc hoạt động:

```text
BH1750 ──┐
         │
BMP280 ──┼──> STM32 ──> Input Registers
         │                  ↑
SCD4x ───┘                  │
                            │
Gateway <── RS485/Modbus ───┘
```

Firmware liên tục cập nhật:

```text
Sensor
   ↓
Read Measurement
   ↓
Input Registers
```

Gateway chỉ thực hiện:

```text
Modbus Request
      ↓
Read Input Registers
      ↓
Modbus Response
```

Gateway **không thực hiện**:

```text
Modbus Request
      ↓
Trigger Sensor
      ↓
Wait Measurement
      ↓
Response
```

Điều này giúp thời gian phản hồi Modbus không phụ thuộc trực tiếp vào chu kỳ đo của cảm biến.

---

## 9. Dữ liệu lặp lại giữa các request

Nếu Gateway đọc nhanh hơn tốc độ cập nhật cảm biến, nhiều response liên tiếp có thể chứa cùng một giá trị.

Ví dụ Gateway đọc SCD4x mỗi:

```text
1 giây
```

nhưng SCD4x chỉ có sample mới khoảng:

```text
5 giây
```

thì có thể nhận:

```text
t = 5 s     CO2 = 668 ppm
t = 6 s     CO2 = 668 ppm
t = 7 s     CO2 = 668 ppm
t = 8 s     CO2 = 668 ppm
t = 9 s     CO2 = 668 ppm
t = 10 s    CO2 = 671 ppm
```

Đây là **hoạt động bình thường**, không phải lỗi cảm biến hoặc lỗi Modbus.

---

# 10. Ví dụ quy đổi dữ liệu

Giả sử Gateway nhận được:

```text
Register 0 = 1550
Register 1 = 2688
Register 2 = 10046
Register 3 = 668
Register 4 = 2388
Register 5 = 5980
```

### BH1750

```text
Lux = 1550 / 10
    = 155.0 lux
```

### BMP280 Temperature

```text
Temperature = 2688 / 100
            = 26.88 °C
```

### BMP280 Pressure

```text
Pressure = 10046 / 10
         = 1004.6 hPa
```

### SCD4x CO₂

```text
CO2 = 668 ppm
```

### SCD4x Temperature

```text
Temperature = 2388 / 100
            = 23.88 °C
```

### SCD4x Humidity

```text
Humidity = 5980 / 100
         = 59.80 %RH
```

---

# 11. Các lưu ý quan trọng

- Giao tiếp với board bằng **Modbus RTU qua RS-485**.
- Đọc dữ liệu cảm biến bằng **Function `0x04` - Read Input Registers**.
- Các thanh ghi `0x0000` → `0x0005` là một block liên tục.
- Có thể đọc toàn bộ 6 thanh ghi bằng một request.
- Kiểm tra đúng Slave ID trước khi giao tiếp.
- Không đặt hai board trên cùng RS-485 bus có cùng Slave ID.
- Slave ID `0` không được firmware sử dụng.
- Cấu hình `000` được firmware chuyển thành Slave ID `1`.
- Không hàn đồng thời pull-up và pull-down trên cùng một bit Slave ID.
- Sau khi thay đổi resistor Slave ID nên reset hoặc cấp nguồn lại board.
- Gateway đọc dữ liệu đã cache, không kích hoạt phép đo cảm biến mới.
- Gateway có thể đọc nhanh hơn chu kỳ cảm biến.
- Giá trị lặp lại trong nhiều request liên tiếp là bình thường.
- SCD4x cần thời gian để có sample đầu tiên sau khi bắt đầu Periodic Measurement.
- SCD4x có chu kỳ sample mới khoảng 5 giây.
- Nhiệt độ BMP280 và nhiệt độ SCD4x không nhất thiết giống nhau do vị trí cảm biến, tự gia nhiệt và ảnh hưởng nhiệt từ PCB.

---

# 12. Quick Reference

```text
Protocol        : Modbus RTU
Physical Layer  : RS-485
Read Function   : 0x04

Start Address   : 0x0000
Register Count  : 6

0x0000 : BH1750 Lux          / 10       [lux]
0x0001 : BMP280 Temperature  / 100      [°C]
0x0002 : BMP280 Pressure     / 10       [hPa]

0x0003 : SCD4x CO2           Direct     [ppm]
0x0004 : SCD4x Temperature   / 100      [°C]
0x0005 : SCD4x Humidity      / 100      [%RH]

Default Slave ID : 1
Slave ID Range   : 1 ... 7

BH1750 Update     : ~500 ms
BMP280 Update     : ~500 ms
SCD4x Ready Check : ~1 s
SCD4x New Sample  : ~5 s
```
