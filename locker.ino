#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo servo;

const int PIN_BTN_EMERG  = 2;
const int PIN_BTN_UP     = 7;
const int PIN_BTN_OK     = 10;
const int PIN_BTN_DOWN   = 11;
const int PIN_BTN_CONF   = 12;
const int PIN_LED_GREEN  = 6;
const int PIN_LED_YELLOW = 5;
const int PIN_LED_RED    = 4;
const int PIN_BUZZER     = 8;
const int PIN_SERVO      = 9;
const int PIN_TEMP       = A0;

const int SENHA_CORRETA[3] = {3, 7, 1};
const int MAX_TENTATIVAS = 3;
const long TEMPO_BLOQUEIO_MS = 15000;
const int SERVO_FECHADO = 0;
const int SERVO_ABERTO = 90;
const long DEBOUNCE_MS = 200;
const float LIMITE_TEMP = 50.0;

enum Estado { TRANCADO, INSERINDO, ABERTO, BLOQUEADO, ALERTA_TEMP };
Estado estado = TRANCADO;

int senha[3] = {0,0,0};
int pos = 0;
int tentativas = 0;

unsigned long tBloq = 0;
unsigned long tLast = 0;

unsigned long tUp=0, tDown=0, tOk=0, tConf=0;
bool lastUp=LOW, lastDown=LOW, lastOk=LOW, lastConf=LOW;

volatile bool emergenciaAtiva = false;

int tempo = 15;
bool initBloq = false;

Estado estadoAntes = TRANCADO;
bool alertaTempAtivo = false;

void isrEmergencia(){ emergenciaAtiva = true; }

void ledVerde(bool s){ digitalWrite(PIN_LED_GREEN, s); }
void ledAmarelo(bool s){ digitalWrite(PIN_LED_YELLOW, s); }
void ledVermelho(bool s){ digitalWrite(PIN_LED_RED, s); }

void apagarTodosLeds(){
  ledVerde(LOW);
  ledAmarelo(LOW);
  ledVermelho(LOW);
}

bool pressed(int pin, bool &lastState, unsigned long &lastTime){
  bool r = digitalRead(pin);
  if(lastState == LOW && r == HIGH && millis() - lastTime > DEBOUNCE_MS){
    lastTime = millis();
    lastState = HIGH;
    return true;
  }
  if(r == LOW) lastState = LOW;
  return false;
}

void piscarAmarelo(int v){
  for(int i=0;i<v;i++){
    ledAmarelo(HIGH); delay(120);
    ledAmarelo(LOW); delay(120);
  }
}

float lerTemperatura(){
  int raw = analogRead(PIN_TEMP);
  float tensao = raw * (5.0 / 1023.0);
  return (tensao - 0.5) * 100.0;
}

void entrarTrancado();
void entrarInserindo();
void entrarAberto();
void entrarBloqueado();

void checarTemperatura(){
  float temp = lerTemperatura();
  if(temp >= LIMITE_TEMP && !alertaTempAtivo){
    alertaTempAtivo = true;
    estadoAntes = estado;
    estado = ALERTA_TEMP;
    servo.write(SERVO_FECHADO);
    apagarTodosLeds();
    ledVermelho(HIGH);
    tone(PIN_BUZZER, 2000, 1000);
    lcd.clear();
    lcd.print("Temperatura Alta");
    lcd.setCursor(0,1);
    lcd.print("Resfrie o Cofre!");
  }
  if(temp < LIMITE_TEMP && alertaTempAtivo){
    alertaTempAtivo = false;
    estado = estadoAntes;
    switch(estadoAntes){
      case TRANCADO: entrarTrancado(); break;
      case INSERINDO: entrarInserindo(); break;
      case ABERTO: entrarAberto(); break;
      case BLOQUEADO: entrarBloqueado(); break;
    }
  }
}

void setup(){
  pinMode(PIN_BTN_EMERG, INPUT);
  pinMode(PIN_BTN_UP, INPUT);
  pinMode(PIN_BTN_OK, INPUT);
  pinMode(PIN_BTN_DOWN, INPUT);
  pinMode(PIN_BTN_CONF, INPUT);

  pinMode(PIN_LED_GREEN, OUTPUT);
  pinMode(PIN_LED_YELLOW, OUTPUT);
  pinMode(PIN_LED_RED, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);

  attachInterrupt(digitalPinToInterrupt(PIN_BTN_EMERG), isrEmergencia, RISING);

  servo.attach(PIN_SERVO);
  servo.write(SERVO_FECHADO);

  lcd.init();
  lcd.backlight();

  lcd.print("Iniciando...");
  delay(1000);

  entrarTrancado();
}

void loop(){
  checarTemperatura();

  if(estado == ALERTA_TEMP) return;

  if(emergenciaAtiva){
    emergenciaAtiva = false;
    entrarBloqueado();
  }

  switch(estado){
    case TRANCADO: loopTrancado(); break;
    case INSERINDO: loopInserindo(); break;
    case ABERTO: loopAberto(); break;
    case BLOQUEADO: loopBloqueado(); break;
    case ALERTA_TEMP: break;
  }
}

void entrarTrancado(){
  estado = TRANCADO;
  tentativas = 0;
  pos = 0;

  for(int i=0;i<3;i++) senha[i]=0;

  servo.write(SERVO_FECHADO);
  apagarTodosLeds();

  lcd.clear();
  lcd.print("Cofre trancado");
  lcd.setCursor(0,1);
  lcd.print("Pressione OK");
}

void entrarInserindo(){
  estado = INSERINDO;
  pos = 0;

  for(int i=0;i<3;i++) senha[i]=0;

  ledAmarelo(LOW);
  atualizarLCD();
}

void entrarAberto(){
  estado = ABERTO;
  tentativas = 0;

  servo.write(SERVO_ABERTO);

  ledVerde(HIGH);
  ledAmarelo(LOW);
  ledVermelho(LOW);

  tone(PIN_BUZZER, 1000, 200);

  lcd.clear();
  lcd.print("Acesso liberado");
}

void entrarBloqueado(){
  estado = BLOQUEADO;

  tLast = millis();
  tempo = 15;
  initBloq = false;

  servo.write(SERVO_FECHADO);

  ledVermelho(HIGH);
  ledAmarelo(LOW);
  ledVerde(LOW);

  lcd.clear();
}

void loopTrancado(){
  if(pressed(PIN_BTN_OK,lastOk,tOk)) entrarInserindo();
}

void loopInserindo(){
  if(pressed(PIN_BTN_UP,lastUp,tUp)){
    senha[pos] = (senha[pos]+1)%10;
    atualizarLCD();
  }

  if(pressed(PIN_BTN_DOWN,lastDown,tDown)){
    senha[pos] = (senha[pos]+9)%10;
    atualizarLCD();
  }

  if(pressed(PIN_BTN_OK,lastOk,tOk)){
    if(pos < 2){ pos++; atualizarLCD(); }
  }

  if(pressed(PIN_BTN_CONF,lastConf,tConf)){
    bool ok = true;
    for(int i=0;i<3;i++) if(senha[i]!=SENHA_CORRETA[i]) ok=false;

    if(ok) entrarAberto();
    else{
      tentativas++;
      piscarAmarelo(3);

      if(tentativas>=MAX_TENTATIVAS) entrarBloqueado();
      else{
        tone(PIN_BUZZER,400,200);
        lcd.clear();
        lcd.print("Senha errada");
        delay(800);
        pos=0;
        atualizarLCD();
      }
    }
  }
}

void loopAberto(){
  if(pressed(PIN_BTN_CONF,lastConf,tConf)) entrarTrancado();
}

void loopBloqueado(){
  ledVermelho((millis()/300)%2);

  if(!initBloq){
    initBloq = true;
    lcd.setCursor(0,0);
    lcd.print("BLOQUEADO       ");
    lcd.setCursor(0,1);
    lcd.print("Tempo: 15s   ");
    delay(800);
  }

  if(millis() - tLast >= 1000){
    tLast = millis();

    if(tempo > 0) tempo--;

    lcd.setCursor(0,1);
    lcd.print("Tempo: ");

    if(tempo < 10) lcd.print("0");
    lcd.print(tempo);
    lcd.print("s   ");
  }

  if(tempo <= 0){
    entrarTrancado();
  }
}

void atualizarLCD(){
  lcd.clear();
  lcd.print("Senha:");

  for(int i=0;i<3;i++){
    lcd.setCursor(7+i*3,0);

    if(i==pos){
      lcd.print("[");
      lcd.print(senha[i]);
      lcd.print("]");
    } else {
      lcd.print(" ");
      lcd.print(senha[i]);
      lcd.print(" ");
    }
  }

  lcd.setCursor(0,1);
  lcd.print("UP/DW OK CONF");
}