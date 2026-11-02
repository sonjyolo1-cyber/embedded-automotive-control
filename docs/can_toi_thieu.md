# CAN tối thiểu

Ghi chú ngắn để đọc lại trước khi viết driver CAN cho Dự án 2.

**Ý quan trọng nhất:** mình chỉ quyết định *gửi cái gì* (ID, độ dài, dữ liệu). Còn đóng gói, tranh quyền gửi, kiểm tra lỗi, gửi lại… là việc của CAN controller trong chip, mình không phải tự làm từng bit.

```text
Code của mình  →  CAN controller (trong chip)  →  Transceiver  →  Dây CANH/CANL
"gửi lệnh gì"      "lo hết phần giao thức"         "đổi ra tín hiệu điện"
```

---

## 1. Dây và tín hiệu

- CAN dùng **2 dây CANH và CANL**. Thông tin nằm ở **chênh lệch điện áp** giữa hai dây, nên ít bị nhiễu.
- **Bit 0 (dominant) là bit "mạnh"**: chỉ cần một node gửi 0 là cả bus thành 0.
- **Bit 1 (recessive) là bit "yếu"**: chỉ là lúc không ai kéo bus.
- Mỗi MCU cần **một transceiver riêng** để nối ra bus.
- **Điện trở 120 Ω chỉ đặt ở 2 đầu bus**, không phải node nào cũng có. Tắt nguồn, đo giữa CANH và CANL sẽ ra khoảng **60 Ω**.

## 2. Một frame gồm gì

- **ID (11 bit):** tên của bản tin, đồng thời là độ ưu tiên.
- **DLC:** số byte dữ liệu (0–8).
- **Data:** nội dung. Ý nghĩa do mình tự quy định, CAN không hiểu nó.
- Các phần còn lại (SOF, CRC, ACK, EOF…) controller tự thêm vào.

> "11 bit" là độ dài của **ID**, không phải độ dài cả frame.

## 3. Ai được gửi trước (arbitration)

- Hai node gửi cùng lúc thì so ID **từng bit, từ trái sang**.
- Node nào gửi 1 mà đọc lại thấy 0 thì biết mình thua, tự dừng lại và gửi lại sau.
- **ID càng nhỏ thì càng được ưu tiên.**
- Bản tin thắng **không bị hỏng gì cả**.
- Ưu tiên chỉ quyết định ai gửi trước, **không chen ngang** một frame đang gửi dở.

## 4. Ưu tiên khác với lọc

- **Ưu tiên (ID):** ai được lên bus trước.
- **Bộ lọc (filter):** node nhận chọn bản tin nào mới đưa vào cho code xử lý.
- Mọi node đều "nghe thấy" mọi frame. Node động cơ bỏ qua lệnh đèn là nhờ **bộ lọc**, không phải nhờ ưu tiên.

## 5. ACK: "có người nghe thấy" chứ không phải "đã làm theo"

- Khi nhận đúng một frame, **bất kỳ node nào** cũng gửi bit ACK.
- Nên ACK chỉ có nghĩa là "có ai đó nhận đúng", **không** có nghĩa node mình muốn điều khiển đã nhận hay đã làm.
- Muốn biết node đích có làm theo không, node đó phải **gửi bản tin trạng thái ngược về**.
- Một node đứng một mình trên bus: không ai ACK nên nó **gửi lại mãi**.
- **Loopback:** chip tự gửi tự nhận, không cần node khác. Chạy được thì driver ổn, nhưng **chưa chứng minh** dây, transceiver và bit timing giữa hai board đúng.

## 6. Kiểm tra lỗi

- **CRC:** bên gửi tính một con số từ dữ liệu, bên nhận tính lại rồi so. Khác nhau là có lỗi. Controller tự làm.
- **Bit stuffing:** cứ 5 bit giống nhau liên tiếp thì controller chèn thêm 1 bit ngược lại, để bên nhận giữ được nhịp. Bên nhận tự bỏ bit này đi, dữ liệu không đổi.
- Có 5 loại lỗi: bit, stuff, CRC, form, ACK.

## 7. Khi node hay bị lỗi

- Mỗi node đếm lỗi khi gửi (**TEC**) và khi nhận (**REC**). Gửi/nhận thành công thì bộ đếm giảm dần.
- **Error-active** (dưới 128): bình thường.
- **Error-passive** (từ 128): vẫn gửi và nhận được, nhưng bị hạn chế cách báo lỗi.
- **Bus-off** (TEC từ 256): node tự rút khỏi bus, không gửi gì nữa.
- Vì vậy code của mình vẫn cần **timeout + trạng thái an toàn** riêng, không chỉ trông vào cơ chế lỗi của CAN.

## 8. Bit timing

- 500 kbit/s nghĩa là **mỗi bit dài 2 µs**.
- Mỗi bit được chia thành nhiều ô nhỏ gọi là **time quantum**. **Sample point** là thời điểm đọc bit, thường khoảng 87–89% thời gian của bit.
- Công thức: `Bitrate = f_CAN / (Prescaler × số quantum mỗi bit)`.
- Hai node **không cần cùng clock hay cùng prescaler**, chỉ cần **cùng bitrate** và sample point gần nhau.
- Ví dụ: TM4C 80 MHz, prescaler 10, 16 quantum; Blue Pill PCLK1 36 MHz, prescaler 4, 18 quantum. Cả hai đều ra 500 kbit/s.

> ⚠️ Ví dụ trên chỉ đúng khi đã bật PLL (TM4C 80 MHz, Blue Pill 72 MHz với APB1 chia 2). Để clock mặc định thì phải tính lại. Clock thật sẽ chốt ở buổi 2–4.

Bẫy hay gặp:
- bxCAN của STM32 chạy theo **PCLK1**, không phải clock CPU.
- Giá trị ghi vào thanh ghi thường là **giá trị thật trừ 1**.

---

## 9. Áp dụng vào dự án

| ID | Bản tin | Gửi → Nhận | Khi nào gửi |
|---|---|---|---|
| `0x050` | Lỗi động cơ (để dành) | Blue Pill 2 → Tiva C | Khi có lỗi |
| `0x100` | Lệnh tốc độ | Tiva C → Blue Pill 2 | ~20 ms |
| `0x110` | Trạng thái động cơ | Blue Pill 2 → Tiva C | ~20 ms |
| `0x200` | Lệnh xi-nhan | Tiva C → Blue Pill 1 | 100 ms + khi đổi |
| `0x210` | Trạng thái đèn | Blue Pill 1 → Tiva C | 100 ms |

Lệnh xi-nhan `0x200`: 1 byte, **0 OFF, 1 LEFT, 2 RIGHT, 3 HAZARD**, số khác là sai.

Những gì đã chốt:
- Tiva C chỉ gửi **trạng thái**, Blue Pill 1 **tự nháy**.
- Gửi **đều đặn 100 ms và ngay khi đổi**, nên mất một frame cũng không sao.
- Quá **500 ms** không nhận được bản tin đúng thì Blue Pill 1 **tắt hết đèn**, còn Tiva C **báo mất node**.
- Dữ liệu sai thì **bỏ qua**, không tính là "còn sống".
- Chân CAN: Tiva C **PE4 (RX) / PE5 (TX)**, Blue Pill **PB8 (RX) / PB9 (TX)**.
- Sơ đồ đấu dây: `docs/can_wiring_buoi1.svg`.

## Tự kiểm tra

- `0x200` và `0x210` gửi cùng lúc thì khác nhau ở bit …, bản tin … thắng.
