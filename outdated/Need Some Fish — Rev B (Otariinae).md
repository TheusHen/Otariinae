# Need Some Fish — Rev. B (Otariinae)

A ideia aqui é construir um aquário automático e elétrico, do zero.

### Componentes Extra (mecânica / montagem)

- **Rolamento 608zz Código 210046-0** (2x)
- **Parafuso Pequeno M3 8 M3x8 M3 X 8 Mm Inox A2 - 20pçs**
- **Cola Silic. 50g Trans.aquario Tekb**

### Materiais e estrutura

- Vamos precisar de alguns sensores e componentes.
- Precisamos de **alta resistência à água** em todas as conexões.
- A estrutura será em peças **3D** (parte de baixo e parte de cima).
- A parte transparente do aquário, que fica no meio entre as duas partes 3D, será feita de **acrílico (PMMA)** para encaixar nas peças impressas.
- Também vamos usar bombas, servos e conexão à internet.

### Estrutura externa (fora da água)

- Na parte de baixo (peça 3D), atrás do aquário, vai existir uma **baia/compartimento 3D externo** para colocar todo o **sistema elétrico**.
- Essa baia vai servir para organizar:

#### ~~Baia removível (gaveta) + trilho impresso~~

- **~~Posição:** atrás, embaixo, **escondida**.~~
- **~~Remoção:** **100% removível** (toda a elétrica fica na baia).~~
- **~~Material sugerido:** **PETG** (mais resistente a temperatura e umidade do que PLA).~~
- **~~Largura alvo:** ~**40 cm** (altura pode ser a que couber, mantendo folga para cabos e conectores).~~

**~~Arquitetura do trilho~~**

- ~~Fazer **dois trilhos paralelos** (esquerda + direita) integrados na peça 3D do aquário.~~
- ~~A baia entra por trás e desliza até encostar no batente.~~
- ~~Perfil do trilho:~~
    - ~~Preferência: **dovetail leve (trapezoidal)** para não “cair” e reduzir folga.~~
    - ~~Alternativa mais simples: **canal em U + patim** (mais fácil, mas tende a ter mais folga).~~

**~~Travamento / fixação no final do curso (para não soltar sozinho)~~**

- ~~Batente mecânico no fim do trilho.~~
- ~~1 ponto de retenção (escolher 1):~~
    - **~~Trava por clip (snap-fit)** na frente da baia.~~
    - **~~1 parafuso** (M3/M4) de retenção.~~
    - **~~Ímãs + batente** (segura fechado e fica elegante).~~

**~~Detalhes importantes (pra não dar ruim)~~**

- **~~Folga de impressão:** começar com ~**0,4 mm por lado** e ajustar conforme a impressora (PETG costuma “engordar” um pouco).~~
- **~~Anti-torção:** usar os 2 trilhos + uma “aba” frontal (ou guia central) para a baia não virar quando puxar.~~
- **~~Puxador/pegador:** recorte ou alça para remover sem enroscar nos cabos.~~
- **~~Alívio de cabos:** zona interna para fazer “loop” e não puxar conector.~~
- **~~Passagem selada:** usar **prensa-cabos** na saída da baia.~~
- **~~Ventilação:** se a fonte e relés aquecerem, prever respiros na baia (sem ficar aberto para respingo).~~

**~~Layout interno da baia (peso médio)~~**

- ~~Separar em 2 “andares” ou 2 zonas:~~
    - **~~Potência/AC (entrada, fusível, fonte)** bem isolado e fixo.~~
    - **~~Baixa tensão (12 V, WAGO, distribuição, relés, conexões)** com acesso fácil.~~
    - ~~(Opcional) Uma “tampinha” interna para cobrir bornes e evitar toque acidental.~~
    - ~~a fonte AC → DC~~
    - ~~régua com fusível~~
    - ~~módulos (relés, conversores, etc.)~~
    - ~~conexões e distribuição de energia para bombas, LEDs e sensores~~

Completamente feito no 3D, Ficou 60cm de largura, e Dovetail somente.

### Passagem de cabos e sensores

- Na parte de cima, vamos deixar **furos/canais** para passagem de:
    - sensores que vão dentro da água
    - cabos dos LEDs da tampa
    - demais cabos que precisarem descer para a baia elétrica

### Placa de controle

- Vamos usar **duas placas**: **Flux (RP2040)** + **ESP32 (de fábrica)**.
- **Mudança importante:** o **Flux não tem conectividade**. Então **Wi‑Fi/BLE e tudo que for comunicação** fica por conta do **ESP32**.
- **RP2040 DevBoard (Flux ou compatível)**

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
- O **ESP32** será usado como módulo de conectividade e integração com a internet/app.

### Observações importantes (montagem)

- Já saiba que vamos soldar bastante coisa e usar bastante fita isolante, principalmente no **módulo de relé** etc.

Mas como todo o aquário trabalha em baixa tensão, a parte da fonte é separada, 110V não passa pelo nosso PCB

### Segurança elétrica / alimentação

- A parte que vem da tomada (110 V) vai entrar em uma **régua com fusível** (proteção).
- Vamos usar uma **fonte AC → DC** para transformar **110 V em 12 V,** que vai ser o padrão utilizado pelo aquário.
- **Todo o aquário trabalha em baixa tensão** (12 V), para reduzir riscos e evitar problemas.

### Componentes que serão controlados via GPIO

- Régua com fusível, antes da fonte, dá energia 110V para a Bomba por exemplo e passa para a fonte
    - **Filtro de linha com protetor contra surtos 
    (DPS) - 127/220 volts - 10 amperes - 5 tomadas - 3 pinos - iCLAMPER 
    Energia 5 - LCF - Transparente**
- Filtro interno (via **módulo de relé**).
    - **Filtro Interno Ipf 408 Aquario Pequeno 30 Litros  Aleas 127V**
- LED ou fita de LED (na tampa, **fora da água** e bem isolado).
    - **Fita LED MK 10W/m COB IP65**
    - *Aviso:* IP65 ajuda contra respingos/umidade, mas **não é para imersão**. Como fica perto de água, prefira sempre **isolação extra** (perfil/capa, boa vedação e prensa-cabos) e evitar emendas expostas.
- Sensor de temperatura (à prova d’água).
    - **DS18B20**
- Sensor de **pH** da água.
    - **Módulo Sensor + Ph Eletrodo Sonda Bnc**
- Sensor de **turbidez**.
    - **Sensor De Turbidez Arduino Para Monitoramento Dágua**
- Sensor de fluxo.
    - **YF-S201**
- **Sensor de nível de água** (para impedir a bomba de rodar a seco).
    - **Sensor De Nível De Água E Chuva Para Arduino - Minymix**
- Sensor de vazamento (na base, do lado de fora do aquário).
    - **Sensor de vazamento de água Velds (WiFi, Notificação Instantânea, Alarme de 85dBa, Baixo Consumo) - VDS SVAW**
- **Motor de passo** (aciona o dosador rotativo de comida).
    - **Motor De Passo Nema 17 1.3a 2,8 Kg Cnc Laser Impressora 3d**
- Chave geral (corta toda a energia, precisa ser colocada antes da bomba e da fonte, ou seja na 110V, precisa se boa isolação, e cuidado, 
operar com o polo negativo).
    - **Módulo Interruptor Bipolar Simples 25 A Inova Pro Class Alumbra**
- Fonte AC → DC
    - **FONTE BLINDADA FC FONTES 12V 10A 120W IP67 FC1210BN**
- Adaptador, tomada filtro de linha → Fonte
    - **Cabo De Força Rabicho 2p + T 2,50mm Plugue 10a 1,50 Metro 127/220v**
- Consumíveis e montagem (geral)
    - **Abraçadeira de nylon**
    - **Prensa-cabo**
    - **Terminais ilhós**
    - **Conectores WAGO**
    - **Fusíveis sobressalentes**
    - **Fios com bitola adequada**
    - **Tubo termo-retrátil**
    - **Silicone neutro próprio para aquário**

### Dispenser de comida (tampa superior)

- No topo, vou fazer em 3D uma **“bacia”/reservatório** que encaixa em um buraco acima da tampa do dosador.
- Esse reservatório vai ter uma **tampa própria**, para evitar que a ração fique úmida.
- A ração fica totalmente isolada até o **dosador rotativo (impresso em 3D)** liberar a porção, e então a ração cai para dentro do aquário.
- O dosador vai ser acionado por **motor de passo**, para ter controle fino da quantidade (passos) e evitar problemas de travar/fechar de “portinha”.

### PCB / conexões

- PCB já pronta, com vários **pin headers**.
- Resistores e proteções já feitos para conectar diretamente os pinos dos componentes, relés, etc., à **Flux DevBoard**.

#### Alimentação e proteção

- **Entrada principal:** 1x **borne 2 vias (12V_IN)**
- **Fusível/Polyfuse (PTC) na entrada 12V**
- **Proteção contra inversão de polaridade** (na entrada 12V)
- **2x portas USB-A** para alimentar as duas placas (Flux + ESP32)
    - Alimentação via **DC-DC buck 12V → 5V**
    - **Polyfuse (PTC)** em 5V (por porta ou geral)

#### Saídas 12V (alta corrente / fio mais grosso)

1. **1x borne 2 vias – LED 12V**
2. **1x borne 2 vias – AUX 12V** *(sobrando)*

#### Motor de passo (dosador)

- **Driver DRV8825** (no PCB, como componente)
- Conector do motor: **JST-XH 4 vias** (preferível)

#### Headers de sensores

- **DS18B20 (à prova d’água):** 1x header 3 vias (V/G/D)
- **YF-S201 (fluxo):** 1x header 3 vias
- **Sensor de nível (Minymix):** 1x header 3 vias
- **Turbidez:** 1x header 3 vias
- **Módulo de pH:** 1x header 3 ou 4 vias (depende do módulo: AO/DO)
- **I2C “expansão”:** 1x header 4 vias (V/G/SDA/SCL)

#### Conectores entre placas

- **Para Flux ↔ WhaleShark (controle):** 1x header **2x8 (16 pinos)**
- **Para ESP32 ↔ WhaleShark (comunicação):** 1x header **2x6 (12 pinos)**

Tomada Quarto(110V) → Filtro de Linha(Sistema 110V) → Fonte AC DC(Terminal block) → **12V** → Terminal block do PCB → Headers pins e Terminal Blocks → Sensores/Relay da Bomba/Fita de led/Motor de Passo

Fora isso, é só fazer um sistema de monitoramento conectado à Flux DevBoard, e pronto.

# Nomes

Projeto: **SeaLion**
Edition: **Otariinae**
PCB: **WhaleShark**

[Need Some Fish — Rev. B (Otariinae) — EN](https://www.notion.so/Need-Some-Fish-Rev-B-Otariinae-EN-74aa7f24469a47aab4f081518cf3b9b8?pvs=21)
