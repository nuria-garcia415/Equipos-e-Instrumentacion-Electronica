#include <DHT.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_NeoPixel.h>
#include <Servo.h>
#include <IRremote.hpp>

// ─── PINES ───────────────────────────────────────────────
#define DHTPIN        2
#define DHTTYPE       DHT22
#define IR_PIN        3
#define NEOPIXEL_PIN  5
#define SERVO_PIN     6
#define TRIG_PIN      9
#define ECHO_PIN      10
#define LDR_PIN       A0

// ─── CONSTANTES ──────────────────────────────────────────
#define NUM_LEDS            8
#define DISTANCIA_PRESENCIA 200   // cm

// Códigos IR del mando estándar (ajusta si tu mando es diferente)
#define IR_0    0x68
#define IR_1    0x30
#define IR_2    0x18
#define IR_3    0x7A
#define IR_4    0x10
#define IR_5    0x38
#define IR_PLUS 0x2   // tecla +
#define IR_MINUS 0x98  // tecla -
#define IR_NEXT 0x43   // tecla >>
#define IR_PREV 0x44   // tecla 
#define IR_MENU 0x40   // tecla MENU

// ─── OBJETOS ─────────────────────────────────────────────
DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);
Adafruit_NeoPixel strip(NUM_LEDS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
Servo miServo;

// ─── VARIABLES DE ESTADO ────────────────────────────────
int plantaActual = 0;

// --- Actividad 3: setpoints ajustables remotamente ---
int setpointTemp = 25;   // °C, rango 15–35
int umbralLuz    = 80;   // %, rango 0–100

// --- Actividad 3: autodiagnóstico ---
float ultimaTempValida = 25.0;
float ultimaHumValida  = 50.0;
bool  errorDHT         = false;
bool  errorLDR         = false;

// ─── SETUP ───────────────────────────────────────────────
void setup() {
  Serial.begin(9600);

  dht.begin();
  lcd.init();
  lcd.backlight();
  strip.begin();
  strip.show();
  miServo.attach(SERVO_PIN);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  IrReceiver.begin(IR_PIN, DISABLE_LED_FEEDBACK);

  lcd.setCursor(0, 0);
  lcd.print("ACME S.A. Ready");
  delay(2000);
  lcd.clear();
}

// ─── LOOP ────────────────────────────────────────────────
void loop() {
  // 1. Leer sensores
  float tempRaw = dht.readTemperature();
  float humRaw  = dht.readHumidity();
  int   rawLDR  = analogRead(LDR_PIN);
  float distancia = medirDistancia();

  // 2. Autodiagnóstico
  diagnosticarSensores(tempRaw, humRaw, rawLDR);

  // Usar valores válidos (reserva si hay error)
  float temp = errorDHT ? ultimaTempValida : tempRaw;
  float hum  = errorDHT ? ultimaHumValida  : humRaw;
  float luzPct = errorLDR ? -1 : map(rawLDR, 0, 1023, 0, 100);

  // 3. Comprobar IR
  if (IrReceiver.decode()) {
    uint32_t codigo = IrReceiver.decodedIRData.command;
    procesarIR(codigo, temp, hum, luzPct);
    IrReceiver.resume();
  }

  // 4. Control HVAC (usa setpointTemp ajustable)
  String estadoHVAC = "NORMAL";
  int banda = 3;
  if (temp > setpointTemp + banda) estadoHVAC = "FRIO ON";
  else if (temp < setpointTemp - banda) estadoHVAC = "CALOR ON";

  // 5. Control de iluminación
  bool presencia = (distancia > 0 && distancia < DISTANCIA_PRESENCIA);
  if (!errorLDR && presencia && luzPct < umbralLuz) {
    int ledsEncendidos = map((int)luzPct, 0, umbralLuz, 0, NUM_LEDS);
    ledsEncendidos = constrain(ledsEncendidos, 0, NUM_LEDS);
    for (int i = 0; i < NUM_LEDS; i++) {
      strip.setPixelColor(i, i < ledsEncendidos ? strip.Color(255, 200, 100) : strip.Color(0, 0, 0));
    }
  } else {
    for (int i = 0; i < NUM_LEDS; i++) strip.setPixelColor(i, 0);
  }
  strip.show();

  // 6. Actualizar LCD
  actualizarLCD(temp, hum, luzPct, estadoHVAC);

  delay(500);
}

// ─── MEDIR DISTANCIA ─────────────────────────────────────
float medirDistancia() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duracion = pulseIn(ECHO_PIN, HIGH, 30000);
  if (duracion == 0) return -1;
  return duracion * 0.034 / 2.0;
}

// ─── VIAJAR A PLANTA ─────────────────────────────────────
void viajar(int planta) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("SUBIENDO A P:");
  lcd.print(planta);
  int angulo = map(planta, 0, 5, 0, 180);
  miServo.write(angulo);
  plantaActual = planta;
  delay(2000);
  lcd.clear();
}

// ─── AUTODIAGNÓSTICO ─────────────────────────────────────
void diagnosticarSensores(float t, float h, int rawLDR) {
  if (isnan(t) || t < -40 || t >= 80 || isnan(h) || h < 0 || h > 100) {
    errorDHT = true;
  } else {
    errorDHT = false;
    ultimaTempValida = t;
    ultimaHumValida  = h;
  }
  errorLDR = (rawLDR <= 0 || rawLDR >= 1023);
}

// ─── MOSTRAR CONFIGURACIÓN EN LCD ────────────────────────
void mostrarConfigLCD() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("SP:");
  lcd.print(setpointTemp);
  lcd.print("C");
  lcd.setCursor(0, 1);
  lcd.print("UMB.LUZ:");
  lcd.print(umbralLuz);
  lcd.print("%");
  delay(2500);
  lcd.clear();
}

// ─── PROCESAR IR ─────────────────────────────────────────
void procesarIR(uint32_t codigo, float temp, float hum, float luzPct) {
  switch (codigo) {
    // Selección de planta
    case IR_0: viajar(0); break;
    case IR_1: viajar(1); break;
    case IR_2: viajar(2); break;
    case IR_3: viajar(3); break;
    case IR_4: viajar(4); break;
    case IR_5: viajar(5); break;

    // Ajuste setpoint temperatura
    case IR_PLUS:
      if (setpointTemp < 35) setpointTemp++;
      mostrarConfigLCD();
      break;
    case IR_MINUS:
      if (setpointTemp > 15) setpointTemp--;
      mostrarConfigLCD();
      break;

    // Ajuste umbral de iluminación
    case IR_NEXT:
      if (umbralLuz < 100) umbralLuz += 5;
      mostrarConfigLCD();
      break;
    case IR_PREV:
      if (umbralLuz > 0) umbralLuz -= 5;
      mostrarConfigLCD();
      break;

    // Mostrar configuración actual
    case IR_MENU:
      mostrarConfigLCD();
      break;

    default:
      // Código no asignado, imprimir en Serial para identificarlo
      Serial.print("IR no asignado: 0x");
      Serial.println(codigo, HEX);
      break;
  }
}

// ─── ACTUALIZAR LCD ──────────────────────────────────────
void actualizarLCD(float temp, float hum, float luzPct, String hvac) {
  lcd.setCursor(0, 0);

  if (errorDHT) {
    lcd.print("ERR:DHT22       ");
  } else {
    lcd.print("P:");
    lcd.print(plantaActual);
    lcd.print(" T:");
    lcd.print(temp, 1);
    lcd.print("C    ");
  }

  lcd.setCursor(0, 1);
  if (errorLDR) {
    lcd.print("ERR:LDR         ");
  } else {
    lcd.print("H:");
    lcd.print((int)hum);
    lcd.print("% ");
    lcd.print(hvac);
    lcd.print("    ");
  }
}