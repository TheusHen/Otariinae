# Need Some Fish — Rev. C (Otariinae)

Esse projeto virou praticamente um sistema embarcado completo para aquário inteligente. A ideia original era construir um aquário automático e elétrico do zero, e agora na Rev. C ele conta com: eletrônica dedicada, PCB custom, microcontroladores separados, múltiplos sensores, automação, aquaponia integrada, dosagem automática e conectividade.

---

## Visão geral da evolução

Desde a Rev. B, meu projeto evoluiu bastante. Saí de um conceito mais simples com Arduino ou microcontrolador único + relés + sensores com controle central, para uma arquitetura distribuída profissional com separação clara de responsabilidades, proteção elétrica completa, PCB custom roteada, telemetria da água e um módulo de aquaponia integrado.

---

## Arquitetura eletrônica (distribuída)

A maior mudança desse projeto foi sair de um controle central único para uma **arquitetura distribuída** com três sistemas independentes:

```
RP2040 DevBoard (Flux ou compatível)
        │
        │ IO / sensores / controle
        │
        ▼
WhaleShark PCB (distribuição elétrica)
        │
        │ UART / comunicação
        ▼
      ESP32
  WiFi / BLE / app
```

Separação clara de responsabilidades:

| Sistema        | Função                |
| -------------- | --------------------- |
| **RP2040**     | Controle de hardware  |
| **ESP32**      | Conectividade         |
| **WhaleShark** | Distribuição elétrica |

### RP2040 DevBoard (Flux ou compatível)

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

O **ESP32** cuida exclusivamente de conectividade: Wi-Fi, BLE e integração com app/internet.

A **WhaleShark PCB** faz toda a distribuição elétrica, regulação de tensão e proteção entre os sistemas.

---

## Sistema elétrico (proteção completa)

Saí de um esquema simples de 110V → fonte → sensores para um sistema com proteção completa em cascata:

```
Tomada 110V
     │
Filtro de linha / DPS
     │
Fonte 12V 10A (IP67)
     │
PCB WhaleShark
     │
Buck 12V → 5V
     │
Regulador 3.3V
```

### Proteções implementadas

- **Fusível** na entrada
- **Polyfuse (PTC)** nas saídas
- **Proteção de polaridade** (contra inversão)
- **TVS** (contra surtos)
- **Separação física de tensões** no PCB
- **DPS** no filtro de linha (antes da fonte)

Isso reduz muito o risco de falha elétrica, especialmente considerando que esse projeto trabalha perto de água.

### Caminho completo da energia

```
Tomada Quarto (110V)
  → Chave geral bipolar
    → Filtro de Linha com DPS
      → Fonte AC-DC 12V 10A (IP67)
        → Terminal block do PCB WhaleShark
          → Buck 12V→5V (USBs para Flux + ESP32)
          → Saídas 12V (LED, AUX, Motor)
          → Headers de sensores (3.3V/5V)
```

### Componentes de alimentação

- **Filtro de linha com DPS** — iCLAMPER Energia 5 (127/220V, 10A, 5 tomadas)
- **Fonte AC→DC** — FONTE BLINDADA FC FONTES 12V 10A 120W IP67 FC1210BN
- **Cabo de força** — Rabicho 2p + T 2,50mm Plugue 10A 1,50m 127/220V
- **Chave geral** — Módulo Interruptor Bipolar Simples 25A Inova Pro Class Alumbra

---

## PCB WhaleShark (custom)

Esse é um dos maiores upgrades do projeto. Saí de fios soltos, módulos avulsos e jumpers para uma **PCB custom** com roteamento adequado.

### O que a PCB tem

- Reguladores de tensão integrados
- Headers organizados para cada sensor/atuador
- Drivers integrados (DRV8825 para motor de passo)
- Buck converter com layout correto
- Divisores de tensão seguros
- Pull-ups I2C
- Trilhas de potência grossas
- Distribuição de energia separada por função
- **Test points** para facilitar depuração e manutenção

### Alimentação e proteção (no PCB)

- **Entrada principal:** 1x borne 2 vias (12V_IN)
- **Fusível/Polyfuse (PTC)** na entrada 12V
- **Proteção contra inversão de polaridade** na entrada
- **2x portas USB-A** para alimentar Flux + ESP32 (via DC-DC buck 12V→5V)
- **Polyfuse (PTC)** em 5V por porta

### Saídas 12V (alta corrente)

1. **1x borne 2 vias** — LED 12V
2. **1x borne 2 vias** — AUX 12V (reserva)

### Motor de passo (dosador)

- **Driver DRV8825** (soldado no PCB)
- Conector do motor: **JST-XH 4 vias**

### Headers de sensores

| Sensor            | Header            |
| ----------------- | ----------------- |
| DS18B20           | 1x 3 vias (V/G/D) |
| YF-S201 (fluxo)  | 1x 3 vias         |
| Nível (Minymix)   | 1x 3 vias         |
| Turbidez          | 1x 3 vias         |
| Módulo de pH      | 1x 3 ou 4 vias    |
| I2C (expansão)    | 1x 4 vias (V/G/SDA/SCL) |

### Conectores entre placas

- **Flux ↔ WhaleShark (controle):** 1x header 2x8 (16 pinos)
- **ESP32 ↔ WhaleShark (comunicação):** 1x header 2x6 (12 pinos)

Isso coloca meu projeto num nível de engenharia muito mais alto do que módulos soltos com jumper.

---

## Sistema de sensores (telemetria completa)

Saí de monitoramento básico para **telemetria completa da água**. Agora consigo monitorar em tempo real tudo que importa.

| Sensor     | Função               | Modelo/Referência                              |
| ---------- | -------------------- | ---------------------------------------------- |
| DS18B20    | Temperatura          | DS18B20 à prova d'água                         |
| pH         | Qualidade química    | Módulo Sensor + pH Eletrodo Sonda BNC          |
| Turbidez   | Partículas na água   | Sensor De Turbidez Arduino                     |
| Fluxo      | Circulação           | YF-S201                                        |
| Nível      | Segurança da bomba   | Sensor De Nível De Água (Minymix)              |
| Vazamento  | Proteção externa     | Sensor Velds (WiFi, Alarme 85dBa)              |

Com isso consigo:
- **Monitoramento em tempo real** de todos os parâmetros
- **Alarmes** automáticos (vazamento, nível baixo, pH fora da faixa)
- **Histórico** de dados (via SD no Flux)
- **Automação** baseada nas leituras (ex: desligar bomba se nível cair)

---

## Aquaponia (nova adição — Rev. C)

Essa foi uma grande expansão do meu projeto. Agora o sistema também controla um módulo de **aquaponia integrado**:

- **Irrigação** das plantas
- **Circulação de água** entre aquário e área de cultivo
- **Dosagem de nutrientes** (via bomba peristáltica)

### Benefícios

- Reciclagem de nutrientes da água do aquário
- Crescimento de plantas como filtragem natural
- Filtragem biológica complementar

### Riscos que estou tratando

| Problema     | Risco                          | Mitigação                |
| ------------ | ------------------------------ | ------------------------ |
| Sifão        | Drenagem reversa               | Anti-sifão implementado  |
| Entupimento  | Bloqueio de fluxo              | Sensor de fluxo (YF-S201) |
| Evaporação   | Queda de nível                 | Sensor de nível          |
| Biofilme     | Interferência nos sensores     | Manutenção periódica     |

Já comecei a tratar o problema de sifão com válvula anti-sifão, e os sensores de fluxo e nível ajudam a detectar os outros problemas automaticamente.

---

## Dosador automático de comida

Meu sistema de dosagem usa um **dosador rotativo impresso em 3D** acionado por motor de passo, muito mais confiável que soluções com servo.

```
Reservatório (tampa vedada)
     │
Dosador rotativo (3D)
     │
Motor NEMA 17
     │
Driver DRV8825 (no PCB)
```

### Vantagens

- **Controle fino de porção** (por passos do motor)
- **Alimentação programada** (via Internet, ESP32)
- **Confiabilidade** muito maior que servo simples (sem travar/fechar portinha)

### Estrutura mecânica do dosador

- No topo, fiz em 3D uma **"bacia"/reservatório** que encaixa num buraco acima da tampa.
- Esse reservatório tem **tampa própria** para isolar a ração da umidade.
- A ração fica totalmente isolada até o dosador rotativo liberar a porção, e então cai para dentro do aquário.

### Componentes

- **Motor** — NEMA 17 1.3A 2,8kg (CNC/Laser/Impressora 3D)
- **Driver** — DRV8825 (integrado no PCB WhaleShark)

---

## Estrutura mecânica

### Materiais e estrutura geral

- Preciso de **alta resistência à água** em todas as conexões.
- A estrutura é em peças **3D** (parte de baixo e parte de cima).
- A parte transparente do aquário, no meio entre as duas partes 3D, é feita de **acrílico (PMMA)** para encaixar nas peças impressas.

### Compartimento elétrico (evolução)

Inicialmente eu planejava uma baia removível com trilhos e sistema de gaveta (detalhes na Rev. B). Depois, simplifiquei para:

- **Dovetail fixo**
- **Compartimento integrado** na peça 3D
- **60 cm de largura**

Essa mudança foi boa porque:
- Aumenta a **rigidez** do conjunto
- Reduz **vibração**
- Evita **desalinhamento** mecânico
- Menos peças = menos pontos de falha

### Passagem de cabos e sensores

Na parte de cima, deixo **furos/canais** para passagem de:
- Sensores que vão dentro da água
- Cabos dos LEDs da tampa
- Demais cabos que precisarem descer para o compartimento elétrico

---

## Componentes controlados via GPIO

- **Filtro interno** (via módulo de relé) — Filtro Interno Ipf 408 Aquario Pequeno 30 Litros Aleas 127V
- **Fita de LED** (na tampa, fora da água, bem isolada) — Fita LED MK 10W/m COB IP65
  - *Aviso:* IP65 ajuda contra respingos/umidade, mas não é para imersão. Como fica perto de água, uso isolação extra (perfil/capa, boa vedação e prensa-cabos) e evito emendas expostas.
- **Motor de passo** (dosador) — NEMA 17 via DRV8825

---

## Segurança

Apliquei várias boas práticas de segurança, especialmente considerando que é um sistema com água:

- **Separar 110V do PCB** — a tensão de rede não passa pelo meu PCB
- **Usar 12V como padrão** — todo o aquário trabalha em baixa tensão
- **Fonte isolada (IP67)** — proteção contra respingos
- **Proteção contra inversão de polaridade** — no PCB
- **Sensores de vazamento** — na base, fora do aquário (com alarme 85dBa + WiFi)
- **Fusíveis e polyfuses** — em múltiplos pontos do circuito
- **TVS** — proteção contra surtos
- **Chave geral bipolar** — corta tudo antes da fonte

---

## Manutenção e expansibilidade

Meu design atual permite:

- **Trocar sensores** facilmente (headers padronizados)
- **Expandir via I2C** (header de expansão dedicado)
- **Adicionar módulos** sem refazer o PCB
- **Testar facilmente** (test points no PCB)
- **Acessar o compartimento elétrico** (dovetail fixo, abertura traseira)

Os test points que adicionei no PCB ajudam muito na depuração e manutenção do sistema.

---

## Consumíveis e montagem (geral)

- Abraçadeira de nylon
- Prensa-cabo
- Terminais ilhós
- Conectores WAGO
- Fusíveis sobressalentes
- Fios com bitola adequada
- Tubo termo-retrátil
- Silicone neutro próprio para aquário

### Componentes extra (mecânica / montagem)

- **Rolamento 608zz Código 210046-0** (2x)
- **Parafuso Pequeno M3 8 M3x8 M3 X 8 Mm Inox A2 — 20pçs**
- **Cola Silic. 50g Trans. aquário Tekb**

---

## Nomes

| Nome    | Referência          |
| ------- | ------------------- |
| Projeto | **SeaLion**         |
| Edition | **Otariinae**       |
| PCB     | **WhaleShark**      |

---

*Need Some Fish — Rev. C (Otariinae)*
