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
  
  ascensor.write(0); //floor = 0
  lcd.print("SISTEMA ACME v2");
  delay(2000);
  lcd.clear();
}

void loop() {
  //sensors readings
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  int valorLuz = analogRead(LDR_PIN);
  int luzPct = map(valorLuz, 1023, 0, 0, 100);

  //illumnination configuration
  int ledsActivos = 0;
  if (luzPct < 80) { // Si hay poca luz natural
    ledsActivos = map(luzPct, 80, 0, 0, 8);
  }
  
  tira.clear();
  for(int i = 0; i < ledsActivos; i++) {
    tira.setPixelColor(i, tira.Color(200, 200, 200)); //white light
  }
  tira.show();

  //temperature configuration
  //T > 28 activa frio; T < 22 activa calor
  String estadoClima = "NORMAL";
  if (t > (TEMP_SETPOINT + ZONA_MUERTA)) {
    estadoClima = "FRIO ON";
  } else if (t < (TEMP_SETPOINT - ZONA_MUERTA)) {
    estadoClima = "CALOR ON";
  }

  //IR remote configuration
  if (IrReceiver.decode()) {
    int comando = IrReceiver.decodedIRData.command;
    // Mapeo estándar de botones Wokwi
    if (comando == 104) viajar(0); //0
    if (comando == 48)  viajar(1); //1
    if (comando == 24)  viajar(2); //2
    if (comando == 122) viajar(3); //3
    if (comando == 16)  viajar(4); //4
    if (comando == 56)  viajar(5); //5
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
  lcd.print("SUBIENDO A P:"); lcd.print(planta);
  ascensor.write(angulo);
  delay(2000); //artificial time that it takes to go to a different floor
  plantaActual = planta;
}

//update LCD
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