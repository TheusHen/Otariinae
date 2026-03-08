# SeaLion — Otariinae (Visão geral do projeto)

SeaLion (edição **Otariinae**) é um aquário inteligente/autônomo com arquitetura embarcada **distribuída**, telemetria completa da água, automação e um módulo de **aquaponia** integrado.

**Fontes neste repositório**
- Escopo (Rev. C): [Need Some Fish — Rev C (Otariinae).md](../../Need%20Some%20Fish%20%E2%80%94%20Rev%20C%20(Otariinae).md)
- Devlog / histórico: [journal.md](../../journal.md)

---

## Nomes

| Item | Nome |
|---|---|
| Projeto | **SeaLion** |
| Edição | **Otariinae** |
| PCB | **WhaleShark** |

---

## Status / histórico (datas absolutas)

- **2026-03-03**: planejamento e “DataBook” Rev. A (ver [journal.md](../../journal.md)).
- **2026-03-04**: 3D (Fusion 360) e release Rev. B (ver [journal.md](../../journal.md)).
- **2026-03-05**: pesquisa e integração de **aquaponia** (ver [journal.md](../../journal.md)).
- **2026-03-06**: esquema e PCB WhaleShark (ver [journal.md](../../journal.md)).
- **2026-03-07**: TODOs / melhorias de power/segurança do PCB (ver [journal.md](../../journal.md)).

---

## Arquitetura eletrônica (distribuída)

A Rev. C adota 3 “blocos” independentes:

- **RP2040 DevBoard (Flux ou compatível)**: controle/IO, sensores, atuadores, **RTC** e **MicroSD**.
- **WhaleShark (PCB)**: distribuição elétrica, regulação, proteção, conectores/headers, drivers.
- **ESP32**: conectividade (Wi‐Fi / BLE / app), comunicação (ex.: UART) com o resto do sistema.

**DevBoard de controle (Flux)**

O projeto foi pensado para utilizar minha devboard custom chamada **Flux**, baseada no **RP2040**, que inclui:

- **Slot Micro SD**
- **Headers de IO expandidos**
- **Interface dedicada com a PCB WhaleShark**

Entretanto, o sistema não depende exclusivamente do **Flux**.

Durante desenvolvimento ou replicação do projeto, qualquer devboard compatível com **RP2040** pode ser utilizada, por exemplo:

- **Raspberry Pi Pico**
- **Pico W**
- **RP2040-Zero**
- outras placas **RP2040** compatíveis

Nesse caso, basta conectar a devboard ao header de controle da **WhaleShark PCB**.

---

## Sistema elétrico (alto nível)

Resumo do caminho de energia:

- Rede **110V** (fora do PCB) → filtro de linha com DPS → fonte **12V/10A (IP67)** → **WhaleShark PCB**
- No WhaleShark:
  - Buck **12V → 5V**
  - Regulador **3.3V**
  - Saídas 12V (LED, AUX, etc.)
  - Alimentação/headers para sensores

---

## Sensores (telemetria)

| Sensor | Função | Modelo/Referência |
|---|---|---|
| DS18B20 | Temperatura | DS18B20 à prova d’água |
| pH | Qualidade química | Módulo sensor + sonda BNC |
| Turbidez | Partículas | Sensor de turbidez (referência Arduino) |
| Fluxo | Circulação | YF‑S201 |
| Nível | Segurança de bomba | Sensor de nível (Minymix) |
| Vazamento | Proteção externa | Sensor Velds (Wi‑Fi, alarme 85 dBa) |

---

## Aquaponia (expansão)

Módulo de aquaponia com irrigação/circulação entre aquário e cultivo, e dosagem (bomba peristáltica).

Mitigações citadas: anti‑sifão, sensores de fluxo e nível, manutenção periódica.

---

## Dosador automático de comida

- Dosador rotativo impresso em 3D acionado por **motor NEMA17**
- Driver **DRV8825** (no WhaleShark)
- Controle de porção por passos do motor + agendamento via RTC (Flux)

---

## Estrutura mecânica (alto nível)

- Peças **3D** (parte superior e inferior)
- Seção transparente em **acrílico (PMMA)** encaixado entre as peças
- Compartimento elétrico integrado (mais rígido e simples)

Detalhes: [3D.md](3D.md)

---

## Onde está cada coisa (repo)

- Escopo (Rev. C): [Need Some Fish — Rev C (Otariinae).md](../../Need%20Some%20Fish%20%E2%80%94%20Rev%20C%20(Otariinae).md)
- Devlog: [journal.md](../../journal.md)
- 3D: pasta [3d](../../3d)
- PCB: [sch/WhaleShark.kicad_sch](../../sch/WhaleShark.kicad_sch) e [pcb/WhaleShark.kicad_pcb](../../pcb/WhaleShark.kicad_pcb)
- Gerbers/drill: [gerbers](../../gerbers) e [drl](../../drl)
- iBOM: [ibom/ibom.html](../../ibom/ibom.html)
- Imagens/renders: [assets](../../assets)
