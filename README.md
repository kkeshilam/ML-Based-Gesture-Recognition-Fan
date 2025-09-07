# ML-Based Gesture Recognition Fan

Gesture-controlled portable fan using embedded ML (Edge Impulse) and BLE control.

This repository contains the **Data Acquisition course project** where hand/arm **gestures are recognized on the edge** and used to control a **portable fan** (power/speed, off) and a **lamp** over BLE.  
It uses two microcontrollers:
- **Seeed XIAO nRF52840 Sense** — runs IMU sampling + **Edge Impulse** gesture classifier (6‑axis features).
- **Arduino Nano 33 BLE** — exposes a **Nordic UART Service (NUS)** over BLE and **emulates fan button presses** via GPIO (through NPN transistors).

> Dataset, training scripts, embedded inference, and BLE control are all included in this repo.

---

## ✨ Features
- **On-device ML**: 62.5 Hz IMU windowing; gestures: `wave_on`, `wave_off`, `tap_on`, `tap_off`.
- **BLE control path** using NUS (RX characteristic) to deliver commands to the fan controller board.
- **Button emulation** (BC548 NPN) to trigger **Power/Speed** and **OFF** buttons; direct **lamp** GPIO control.
- Reproducible **training pipeline** (Keras/TensorFlow) + **TFLite conversion**.
- Ready-to-upload **Arduino sketches** for both boards.

---

## 🛠 Hardware
- Seeed **XIAO nRF52840 Sense** (onboard IMU) *(code uses LSM6DS3 driver interface; keep that or swap to your IMU driver as needed)*
- **Arduino Nano 33 BLE**
- **BC548** (or similar NPN) × **3** + base resistors (≈ 1 kΩ–10 kΩ) to emulate button presses & switch lamp
- LED/indicator lamp (or relay/mosfet for your lamp load)
- Breadboard, wires, USB cables

> ⚠️ **Safety**: If you interface with **mains-powered** equipment, use proper isolation (relay/optocoupler) and follow electrical safety standards.

---

## 🧩 System Architecture

```
[Gestures] -> IMU (ax,ay,az,gx,gy,gz) @62.5Hz
      | 
      v
Seeed XIAO nRF52840 Sense
  - Edge Impulse inference
  - Produces labels: wave_on / wave_off / tap_on / tap_off
      |
      |  (via BLE NUS or bridge)
      v
Arduino Nano 33 BLE
  - Receives command strings over NUS RX
  - Maps to actions:
       wave_on  -> on   (Power/Speed press)
       wave_off -> offs (Lamp OFF)
       tap_on   -> off  (Fan OFF press)
       tap_off  -> ons  (Lamp ON)
  - Drives GPIO:
       D2 -> Power/Speed button (through NPN)
       D3 -> OFF button (through NPN)
       D4 -> Lamp (LOW-side via NPN)
```

---

## 📁 Repository Structure

```
Seeed Fan Project/
├─ GestureData/
│  ├─ gesture_data.csv          # 25,200 rows, columns: session_id, ax, ay, az, gx, gy, gz
│  └─ gesture_labels.csv        # 504 rows, columns: session_id, label (wave_on/off, tap_on/off)
├─ Nano_BLE_updated_to_Seeed.ino  # Edge Impulse IMU classifier (XIAO nRF52840 Sense)
├─ fan_light_working.ino          # BLE NUS + fan/lamp controller (Nano 33 BLE)
├─ train_dense_model.py           # Keras training pipeline (dense model)
├─ convert_tfLite.py              # Convert gesture_model.h5 -> gesture_model.tflite
└─ ei-gesture_predictor-arduino-1.0.1.zip  # Edge Impulse inference library (Arduino)
```

---

## 🔧 Software Setup

### 1) Arduino IDE & Libraries
- **Board packages**:
  - *Seeed nRF52 Boards* (for XIAO nRF52840 Sense)
  - *Arduino Mbed OS Nano* (for Nano 33 BLE)
- **Libraries**:
  - **Edge Impulse inference**: install from `ei-gesture_predictor-arduino-1.0.1.zip` (Sketch → Include Library → Add .ZIP Library…)
  - **ArduinoBLE** (for NUS on Nano 33 BLE)
  - **LSM6DS3** (IMU driver; adjust if using another IMU)

### 2) Python Environment (Training)
```bash
python -m venv venv
source venv/bin/activate  # on Windows: venv\Scripts\activate
pip install -U tensorflow pandas numpy scikit-learn matplotlib seaborn
```

---

## 🧠 Training & Conversion

**Data**: `GestureData/gesture_data.csv` (ax, ay, az, gx, gy, gz) linked by `session_id` to `gesture_labels.csv` (labels: wave_on, wave_off, tap_on, tap_off).

**Steps**:
1. Edit hyperparameters in `train_dense_model.py` if needed (window size, test split, epochs).
2. Run training to produce `gesture_model.h5` and `label_encoder.pkl`:
   ```bash
   python Seeed\ Fan\ Project/train_dense_model.py
   ```
3. Convert to TFLite:
   ```bash
   python Seeed\ Fan\ Project/convert_tfLite.py
   ```
   This writes `gesture_model.tflite` in the working directory.

> Note: The embedded sketch `Nano_BLE_updated_to_Seeed.ino` is currently set up to use **Edge Impulse** (`Gesture_Predictor_inferencing.h`). If you want to switch to **TensorFlow Lite Micro** directly, embed `gesture_model.tflite` into your sketch with TF‑Micro and match the input scaling and sampling parameters.

---

## ⬆️ Upload & Run

### A) Seeed XIAO nRF52840 Sense (classifier)
1. Open `Nano_BLE_updated_to_Seeed.ino`
2. Verify IMU init in `setup()` (currently `LSM6DS3`), adjust if using a different IMU.
3. Upload to the XIAO Sense. Open Serial Monitor to confirm inference is running.

### B) Arduino Nano 33 BLE (fan/lamp controller)
1. Open `fan_light_working.ino`
2. Confirm pin mappings: `D2=Power/Speed`, `D3=OFF`, `D4=Lamp`
3. Upload. The board advertises a **NUS service** (`6E400001-...`); connect with a BLE central (e.g. *nRF Connect* app).

### C) Send Commands (manual test)
Use a BLE UART app to send any of:
```
wave_on
wave_off
tap_on
tap_off
```
The controller will internally map them to:
```
on   -> press Power/Speed
off  -> press OFF
ons  -> Lamp ON
offs -> Lamp OFF
```

> Debounce is applied (`ACTION_DEBOUNCE_MS=300`). Button press duration defaults to 200 ms.

---

## 🧪 Validation
- Verified default state and transitions:
  - **Fan ON/SPEED** via `wave_on`
  - **Fan OFF** via `tap_on`
  - **Lamp ON** via `tap_off`
  - **Lamp OFF** via `wave_off`
- Tested multiple cycles; ensured reliable debounce and stable BLE RX parsing.

---

## 🔁 Customization
- Update gesture→action mapping in `handleCommand()` inside `fan_light_working.ino`.
- Change pins or press durations (`pressButton(pin, holdMs)`).
- Extend gestures by retraining with more classes and updating the embedded model.

---

## 🧷 Known Limitations
- Current repo shows **two‑board architecture** (classifier and controller).  
  You can migrate to **one board** by making the XIAO a BLE central that writes into the Nano’s NUS, or by adding direct GPIO control hardware to the XIAO and removing the Nano.
- `train_dense_model.py` assumes a simple dense network; you may achieve better accuracy with CNN/RNN on windows.

---

## 📜 License
This project is licensed under the **MIT License** (see [LICENSE](LICENSE)).  
Third‑party components retain their own licenses (e.g., **Edge Impulse SDK**, **ArduinoBLE**, IMU drivers).

---

## 🙌 Acknowledgements
- **Edge Impulse** for the on‑device ML tooling and Arduino inference library.
- **Arduino** community libraries and examples for BLE and IMU.

---

## 📬 Contact
**Kalani Keshila** — open to feedback and collaboration.
