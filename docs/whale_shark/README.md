# WhaleShark PCB

A **WhaleShark** é a PCB de distribuição elétrica/regulação/proteção e interconexão do SeaLion (Otariinae).

Arquivos principais:
- Esquemático: [sch/WhaleShark.kicad_sch](../../sch/WhaleShark.kicad_sch)
- Layout: [pcb/WhaleShark.kicad_pcb](../../pcb/WhaleShark.kicad_pcb)
- iBOM: [ibom/ibom.html](../../ibom/ibom.html)

Fabricação:
- Gerbers: [gerbers](../../gerbers)
- Drill: [drl](../../drl)

Imagens:
- [assets/whale_front.png](../../assets/whale_front.png)
- [assets/whale_back.png](../../assets/whale_back.png)

---

## Parâmetros do PCB (do layout)

- Espessura: **1.6 mm** (ver [pcb/WhaleShark.kicad_pcb](../../pcb/WhaleShark.kicad_pcb))

---

## Blocos/ICs relevantes (confirmados no esquemático)

- Buck 12V→5V: **LM2596S-5**
- LDO 3.3V: **AP2112K-3.3**
- Driver LED: **MAX7219**
- Proteção: TVS (ex.: **D1 = D_TVS**)
- Polyfuses (PTC): **F1, F2, F3**

---

## Conectores de potência (bornes) — nomes no esquemático

- **J4 = 12V_IN** (entrada)
- **J1 = LED 12V** (saída 12V para LEDs)
- **J2 = AUX 12V**
- **J3 = AUX 12V**
- **J5 = 5V_OUT**

> Nota: a Rev. C descreve 2 saídas 12V (LED e AUX). O esquemático atual tem **duas** saídas AUX (J2 e J3). Confirme qual revisão você vai fabricar.

---

## USB-A (alimentação de devboards)

- **Flux1** (USB_A)
- **ESP1** (USB_A)

D+/D− aparecem como “unconnected” no layout (USB usado como alimentação, sem dados).

---

## Nets úteis (do layout)

Veja [pcb/WhaleShark.kicad_pcb](../../pcb/WhaleShark.kicad_pcb) para a lista completa; alguns nomes:

- Alimentação: **+12V**, **+5V**, **+3V3**, **GND**
- Stepper/driver: **/DRV_STEP**, **/DRV_DIR**, **/DRV_EN**, **/SW**
- MAX7219: **/MAX_CLK**, **/MAX_DIN**, **/MAX_LOAD**, **/ROW0..7**, **/COL0..7**
- I2C: **/I2C_SDA**, **/I2C_SCL**
- Aquaponia: **/AQUAPONIC_SIGNAL**
- ESP UART: **/ESP_RX**, **/ESP_TX**
- USB VBUS: **/USB1_VBUS**, **/USB2_VBUS**

---

## Gerbers e drill

- Gerbers: [gerbers](../../gerbers)
- Drill: [drl](../../drl)
- iBOM: [ibom/ibom.html](../../ibom/ibom.html)
