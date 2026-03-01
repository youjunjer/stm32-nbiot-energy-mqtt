# 01_helloworld (NUCLEO-L053R8)

目標：先確認板子、時鐘、UART、USB 虛擬序列埠都正常。

## 1. 建立專案
1. 開啟 STM32CubeIDE -> `File` -> `New STM32 Project`
2. Board Selector 選 `NUCLEO-L053R8`
3. Project Name 可用：`01_helloworld`
4. Toolchain / IDE 選 CubeIDE 預設

## 2. Pin / Peripheral 設定
1. 啟用 `USART2` 為 `Asynchronous`
2. 參數設定：
   - Baud rate: `115200`
   - Word length: `8 Bits`
   - Parity: `None`
   - Stop bits: `1`
3. `SYS` -> Debug 選 `Serial Wire`
4. 產生程式碼（Generate Code）

說明：NUCLEO 板上通常 `USART2` 會透過 ST-LINK 的 USB Virtual COM Port 連到 PC。

## 3. 貼入 Hello World 程式
把同資料夾的 `hello_patch_for_main.c` 內容，對應貼到 CubeIDE 產生的 `Core/Src/main.c` 使用者區塊。

## 4. 編譯與燒錄
1. 接上 NUCLEO USB
2. `Build`
3. `Run` (或 Debug)

## 5. 開啟序列埠觀察
在終端機工具（TeraTerm / PuTTY / MobaXterm / CubeIDE Terminal）設定：
- COM Port: 你的 ST-LINK VCP
- 115200-8-N-1
- No flow control

預期每秒看到：
- `hello world 1`
- `hello world 2`
- ...

## 6. 若沒有輸出，快速檢查
1. Windows 裝置管理員是否看到 ST-LINK COM Port
2. 終端機是否選對 COM 編號
3. 波特率是否 115200
4. 是否真的使用 `USART2`（不是別的 UART）
5. 板子是否成功下載（LED 有無閃爍/程式有跑）

---
如果這步成功，下一步就能接你的「用電資訊 UART 讀取 + NB-IoT MQTT 發送」骨架。
