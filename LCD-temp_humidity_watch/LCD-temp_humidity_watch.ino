#include <LiquidCrystal_I2C.h>
#include <dht.h>
#include <IRremote.h>
#include <virtuabotixRTC.h>
#include <avr/sleep.h>
dht DHT;
#define DHT11_PIN A0

decode_results results;
LiquidCrystal_I2C  lcd(0x27, 2, 1, 0, 4, 5, 6, 7);
bool on = true;
const int receivePin = 2;
volatile unsigned long lastRefresh = 0;
enum possibleStates {temperatur, klokke, meny, sleep};
volatile possibleStates state = meny;
IRrecv irReceiver(receivePin);
virtuabotixRTC mhRealTime(6, 7, 8);


void setup()
{
  /* sekunder, minuter, timer, ukedag, dag, måned, år, brukes for å kalibrere dato og klokke
    myRTC.setDS1302Time(0, 0, 0, 7, 10, 11, 2019); */
  attachInterrupt(digitalPinToInterrupt(receivePin), changeProgram, FALLING); /* Setter interrupt på IR mottakeren */
  irReceiver.enableIRIn(); /* Starter IR mottakker */
  lcd.begin (16, 2); /* Starter LCD */
  lcd.setBacklightPin(3, POSITIVE);
  lcd.setBacklight(HIGH);
}

void loop()
{
  switch (state)
  {
    case temperatur:
      lcd.clear();
      on = true;
      while (on) temp();
      break;
    case klokke:
      lcd.clear();
      on = true;
      while (on) time();
      break;
    case meny:
      {
        unsigned int ctr = 1;
        lcd.clear();
        on = true;
        while (on) menu(ctr);
        break;
      }
    case sleep:
      sleepMode();
      break;
  }
}

void temp()
{
  /* Skriver ut temperatur og fuktighet hvert 1080ms fordi sensoren gir feil verdier for kortere intervaller. */
  if (millis() - lastRefresh >= 1080)
  {
    DHT.read11(DHT11_PIN);
    lcd.home();
    lcd.print("Temperatur: " + String(int(DHT.temperature)) + (char)223 + 'C');
    lcd.setCursor(0, 1);
    if (int(DHT.humidity) > 9) lcd.print("Fuktighet : " + String(int(DHT.humidity)) + " %");
    else lcd.print("Fuktighet : " + String(int(DHT.humidity)) + "  %");
    lastRefresh = millis();
  }
}

void time()
{
  /* Skriver ut dato og klokkeslett hvert sekund. */
  if (millis() - lastRefresh >= 1000)
  {
    mhRealTime.updateTime();
    lcd.home();
    lcd.print("   " + String(mhRealTime.dayofmonth) + '.' + String(mhRealTime.month) + '.' + String(mhRealTime.year));
    lcd.setCursor(0, 1);
    if (int(mhRealTime.hours) > 9 && int(mhRealTime.minutes) > 9)
      lcd.print("Kl: " + String(mhRealTime.hours) + ':' + String(mhRealTime.minutes));
    if (int(mhRealTime.hours) > 9 && int(mhRealTime.minutes) <= 9)
      lcd.print("Kl: " + String(mhRealTime.hours) + ":0" + String(mhRealTime.minutes));
    if (int(mhRealTime.hours) <= 9 && int(mhRealTime.minutes) > 9)
      lcd.print("Kl: 0" + String(mhRealTime.hours) + ':' + String(mhRealTime.minutes));
    if (int(mhRealTime.hours) <= 9 && int(mhRealTime.minutes) <= 9)
      lcd.print("Kl: 0" + String(mhRealTime.hours) + ':0' + String(mhRealTime.minutes));
    lastRefresh = millis();
  }
}

void menu(unsigned int& ctr)
{
  /* Hovedmenyen. */
  lcd.home();
  lcd.print("      Meny");
  lcd.setCursor(0, 1);

  /* "Scroller" teksten på hovedmenyen. */
  if (millis() - lastRefresh >= 1200)
  {
    switch (ctr)
    {
      case 1:
        lcd.print("1.Temp 2.Klokke 3.Meny");
        ctr++;
        break;
      case 2:
        lcd.print(".Temp 2.Klokke 3.Meny");
        ctr++;
        break;
      case 3:
        lcd.print("Temp 2.Klokke 3.Meny");
        ctr++;
        break;
      case 4:
        lcd.print("emp 2.Klokke 3.Meny");
        ctr++;
        break;
      case 5:
        lcd.print("mp 2.Klokke 3.Meny");
        ctr++;
        break;
      case 6:
        lcd.print("p 2.Klokke 3.Meny");
        ctr++;
        break;
      case 7:
        lcd.print(" 2.Klokke 3.Meny");
        ctr++;
        break;
      default:
        lcd.print("1.Temp 2.Klokke 3.Meny");
        ctr = 1;
        break;
    }
    lastRefresh = millis();
  }
}

void sleepMode()
{
  /* Setter arduinoen i sleep mode for å spare strøm. */
  lcd.clear();
  lcd.setBacklight(LOW);
  detachInterrupt(digitalPinToInterrupt(receivePin));
  /* Gjør så arduino kan våkne av interrupt fra IR receiveren. */
  attachInterrupt(digitalPinToInterrupt(receivePin), wakeUp, LOW);
  sleep_enable();
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_cpu();
  lcd.setBacklight(HIGH);
  state = meny;
  /* Setter IR receiveren tilbake til sin opprinnelige funksjon (å bytte mellom programmer). */
  detachInterrupt(digitalPinToInterrupt(receivePin));
  attachInterrupt(digitalPinToInterrupt(receivePin), changeProgram, FALLING);
}

void wakeUp()
{
  sleep_disable();
}

void changeProgram()
{
  /* Tar imot signaler fra fjernkontrollen og dekoder dem. */
  if (irReceiver.decode(&results))
  {
    irReceiver.resume();
    on = false;
    if (results.value == 0xFF6897) state = sleep; /* Knapp 0 blir trykket. */
    if (results.value == 0xFF30CF) state = temperatur; /* Knapp 1 blir trykket. */
    if (results.value == 0xFF18E7) state = klokke; /* Knapp 2 blir trykket. */
    if (results.value == 0xFF7A85) state = meny; /* Knapp 3 blir trykket. */
    if (results.value == 0xFFFFFFFF) state = meny; /* Error knappen blir holdt inne/IR mottakkeren mottar feil signal. */
  }
}
