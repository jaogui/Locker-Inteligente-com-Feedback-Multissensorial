# 🔐 Locker Inteligente com Feedback Multissensorial

**Disciplina:** CCM520 – Internet das Coisas  
**Instituição:** Centro Universitário FEI  
**Componentes:** Arduino Uno, LCD I2C, Servomotor, Buzzer, LEDs, LDR, 4 Botões

---

## 📋 Descrição do Projeto

Sistema de controle de acesso para um cofre inteligente baseado em Arduino Uno. O usuário digita uma senha de 4 dígitos usando botões físicos; o LCD guia cada etapa, o servomotor representa a trava e um sensor LDR monitora o ambiente internamente ao cofre.

---

## 🔧 Hardware Utilizado

| Componente         | Quantidade | Pino Arduino       |
|--------------------|------------|--------------------|
| Arduino Uno        | 1          | —                  |
| Display LCD 16x2 I2C | 1        | SDA (A4), SCL (A5) |
| Servomotor SG90    | 1          | D9                 |
| Buzzer Ativo       | 1          | D8                 |
| LED Verde          | 1          | D7                 |
| LED Amarelo        | 1          | D6                 |
| LED Vermelho       | 1          | D5                 |
| Botão UP (↑)       | 1          | D2 (INT0)          |
| Botão OK (→)       | 1          | D3 (INT1)          |
| Botão DOWN (↓)     | 1          | D4                 |
| Botão CONFIRMAR    | 1          | D10                |
| Botão RESET        | 1          | D11 (PCINT3)       |
| Sensor LDR         | 1          | A0                 |
| Resistor 10kΩ (LDR)| 1          | —                  |
| Resistores 220Ω    | 3          | (um por LED)       |

---

## ⚙️ Funcionamento do Sistema

### Lógica de Senha

A senha é composta por 4 dígitos (0–9). Os botões funcionam da seguinte forma:

| Botão     | Função                                              |
|-----------|-----------------------------------------------------|
| **UP ↑**  | Incrementa o dígito selecionado (0→1→...→9→0)      |
| **OK →**  | Avança para o próximo dígito (posição 1→2→3→4)     |
| **DOWN ↓**| Decrementa o dígito selecionado (0→9→8→...→1→0)   |
| **CONF**  | Confirma a senha completa / tranca o cofre quando aberto |
| **RESET** | Interrupção de emergência – reinicia o sistema instantaneamente |

O LCD exibe os dígitos em tempo real, destacando com `[n]` o dígito sendo editado:

```
Senha:  2 [5] 1  3
UP/DW edita | OK->
```

### Senha padrão

```
3 - 7 - 1 - 5
```

Para alterar, edite o array no início do código:
```cpp
const int SENHA_CORRETA[4] = {3, 7, 1, 5};
```

---

## 🔄 Máquina de Estados Finita (FSM)

O sistema é gerenciado por uma FSM com 5 estados:

```
┌─────────────┐   qualquer botão    ┌──────────────────┐
│   TRANCADO  │ ──────────────────► │  INSERINDO_SENHA │
│ (estado ini)│                     │ (edição de senha)│
└─────────────┘                     └──────────────────┘
       ▲                               │             │
       │ B4: trancar                   │ B4 correto  │ B4 errado
       │ (servo 0°)                    ▼             ▼ 3 erros
       │                         ┌─────────┐   ┌──────────┐
       │◄────────────────────────│  ABERTO │   │BLOQUEADO │
       │ timeout 30s / reset     │servo 90°│   │espera 30s│
       │                         └─────────┘   └──────────┘
       │
       │◄────── reset (PCINT) ─────────────────────────────
       │
   ┌───────────────┐
   │ ALERTA_SENSOR │ (LDR detecta luz)
   │ buzzer + LCD  │
   └───────────────┘
```

### Descrição dos estados

| Estado           | Servo | LED Ativo | Condição de saída                     |
|------------------|-------|-----------|---------------------------------------|
| TRANCADO         | 0°    | —         | Qualquer botão de edição              |
| INSERINDO_SENHA  | 0°    | Amarelo   | B4 correto → ABERTO; 3 erros → BLOQUEADO |
| ABERTO           | 90°   | Verde     | B4 → TRANCADO                         |
| BLOQUEADO        | 0°    | Vermelho  | Timeout 30s → TRANCADO                |
| ALERTA_SENSOR    | —     | Vermelho  | LDR normal + B4 → TRANCADO            |

---

## 🚨 Funcionalidades de Segurança

- **Bloqueio após 3 tentativas erradas:** sistema trava por 30 segundos, LED vermelho pisca, contagem regressiva exibida no LCD.
- **Monitoramento por LDR:** sensor detecta incidência de luz dentro do cofre. Se ativado, dispara alarme sonoro e visual independentemente do estado atual.
- **Interrupção de emergência (PCINT):** o botão RESET usa interrupção de hardware (`PCINT0_vect`) para reiniciar o sistema instantaneamente a partir de qualquer estado.

---

## 📦 Bibliotecas Necessárias

Instalar pelo **Library Manager** da Arduino IDE:

- `LiquidCrystal_I2C` (Frank de Brabander)
- `Servo` (já inclusa na IDE)

---

## 🔌 Diagrama de Conexão Simplificado

```
Arduino Uno
│
├─ D2  ── Botão UP (com pull-up interno)
├─ D3  ── Botão OK (com pull-up interno)
├─ D4  ── Botão DOWN (com pull-up interno)
├─ D5  ── Resistor 220Ω ── LED Vermelho ── GND
├─ D6  ── Resistor 220Ω ── LED Amarelo ── GND
├─ D7  ── Resistor 220Ω ── LED Verde   ── GND
├─ D8  ── Buzzer (+) / GND ao negativo
├─ D9  ── Servo (sinal laranja)
├─ D10 ── Botão CONFIRMAR (com pull-up interno)
├─ D11 ── Botão RESET (com pull-up interno)
├─ A0  ── LDR ── 5V  /  LDR ── Resistor 10kΩ ── GND
├─ A4 (SDA) ── LCD I2C SDA
└─ A5 (SCL) ── LCD I2C SCL
```

> O endereço padrão do LCD I2C é `0x27`. Se não funcionar, tente `0x3F`.

---

## 🛠️ Como Reproduzir

1. Monte o circuito conforme o diagrama acima no protoboard.
2. Instale as bibliotecas necessárias na Arduino IDE.
3. Abra `locker.ino` e, se necessário, ajuste o endereço I2C do LCD (linha `LiquidCrystal_I2C lcd(0x27, 16, 2)`).
4. Faça o upload para o Arduino Uno.
5. Abra o Monitor Serial (9600 baud) para acompanhar os logs de estado.
6. Senha padrão: **3715**

---

## 🗂️ Estrutura do Repositório

```
/
├── locker.ino       # Firmware principal (FSM + lógica completa)
└── README.md        # Esta documentação
```
