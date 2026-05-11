#include <DHT.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#include <IRremote.h>
#include <Adafruit_NeoPixel.h>

//pins definitions
#define DHTPIN 2
#define IR_PIN 3
#define NEO_PIN 5
#define SERVO_PIN 6
#define TRIG_PIN 9
#define ECHO_PIN 10
#define LDR_PIN A0

//parameters
#define NUMPIXELS 8
#define TEMP_SETPOINT 25.0
#define ZONA_MUERTA 3.0 //range from -3 to +3
#define DISTANCIA_PRESENCIA 200 // Umbral en cm para detectar usuario

//objects creation
DHT dht(DHTPIN, DHT22);
LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo ascensor;
Adafruit_NeoPixel tira(NUMPIXELS, NEO_PIN, NEO_GRB + NEO_KHZ800);

int plantaActual = 0; //current floor

void setup() {
  Serial.begin(9600);
  dht.begin();
  lcd.init();
  lcd.backlight();
  ascensor.attach(SERVO_PIN);
  IrReceiver.begin(IR_PIN);
  tira.begin();
  
  // Configuración pins ultrasonidos (Añadido del segundo código)
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  ascensor.write(0); //floor = 0
  lcd.print("Actividad 2");
  delay(2000);
  lcd.clear();
}

void loop() {
  //sensors readings
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  int valorLuz = analogRead(LDR_PIN);
  int luzPct = map(valorLuz, 1023, 0, 0, 100);

  //presence measurement
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  float distancia = pulseIn(ECHO_PIN, HIGH) * 0.034 / 2;

  //illumnination configuration
  int ledsActivos = 0;
  //leds will turn on only if there's someone less than 200 cm close and light level is low
  if (distancia > 0 && distancia < DISTANCIA_PRESENCIA) { 
    if (luzPct < 80) { 
      ledsActivos = map(luzPct, 80, 0, 0, 8);
    }
  } else {
    ledsActivos = 0; //turn off when no one is close
  }
  
  tira.clear();
  for(int i = 0; i < ledsActivos; i++) {
    tira.setPixelColor(i, tira.Color(200, 200, 200)); 
  }
  tira.show();

  //temperature configuration
  //T > 28 activa frio; T < 22 activa calor)
  String estadoClima = "NORMAL";
  if (t > (TEMP_SETPOINT + ZONA_MUERTA)) {
    estadoClima = "FRIO ON";
  } else if (t < (TEMP_SETPOINT - ZONA_MUERTA)) {
    estadoClima = "CALOR ON";
  }

  //IR remote configuration
  if (IrReceiver.decode()) {
    int comando = IrReceiver.decodedIRData.command;
    if (comando == 104) viajar(0); 
    if (comando == 48)  viajar(1); 
    if (comando == 24)  viajar(2); 
    if (comando == 122) viajar(3); 
    if (comando == 16)  viajar(4); 
    if (comando == 56)  viajar(5); 
    IrReceiver.resume();
  }

  //LCD interface
  actualizarLCD(t, h, estadoClima);
  delay(200);
}

void viajar(int planta) {
  int angulo = map(planta, 0, 5, 0, 180);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("MOVIENDO A P:"); lcd.print(planta);
  ascensor.write(angulo);
  delay(2000); //artificial time that it takes to go to a different floor
  plantaActual = planta;
}

void actualizarLCD(float t, float h, String clima) {
  lcd.setCursor(0, 0);
  lcd.print("P:"); lcd.print(plantaActual);
  lcd.print(" T:"); lcd.print(t, 1);
  lcd.print("C  ");
  
  lcd.setCursor(0, 1);
  lcd.print("H:"); lcd.print(h, 0);
  lcd.print("% ");
  lcd.print(clima);
}
