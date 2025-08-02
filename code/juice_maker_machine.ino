#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <Stepper.h>
#include <Servo.h>
const int irFlavor1 = 8;
const int irFlavor2 = 9;
const int irSugar = 51;
const int waterPump = 49;
const int stepsPerRevolution = 2048;
unsigned long extraTime = 0;
Stepper stepperFlavor1(stepsPerRevolution, 22, 26, 24, 28);
Stepper stepperFlavor2(stepsPerRevolution, 30, 34, 32, 36);
Stepper stepperSugar(stepsPerRevolution, 38, 42, 40, 44);
Servo tankServo;
Servo supplyServo;
Servo drainServo;
LiquidCrystal_I2C lcd(0x27, 16, 2);
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}};
byte rowPins[ROWS] = {23, 25, 27, 29};
byte colPins[COLS] = {31, 33, 35, 37};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);
const int motorPin1 = 39;
const int motorPin2 = 41;
const int enablePin = 10;
const int trigPin = 43;
const int echoPin = 45;
enum State
{
    START,
    SELECT_FLAVOR,
    SELECT_SUGAR,
    SHOW_WARN_WATER,
    SHOW_WARN_ITEMS,
    SHOW_WARN_CUP,
};
unsigned long durationSugar = 0;
int count = 0;
State currentState = START;
String flavors[2] = {"Orange", "Mango"};
String sugars[2] = {"Low", "High"};
String alertMessage = "";
String previousFlavor = "";
String selectedFlavor;
String selectedSugar;
void setup()
{
    stepperFlavor1.setSpeed(15);
    stepperFlavor2.setSpeed(15);
    stepperSugar.setSpeed(15);
    pinMode(motorPin1, OUTPUT);
    pinMode(motorPin2, OUTPUT);
    pinMode(enablePin, OUTPUT);
    tankServo.attach(2);
    supplyServo.attach(4);
    drainServo.attach(5);
    tankServo.write(130);
    supplyServo.write(0);
    drainServo.write(180);
    pinMode(irFlavor1, INPUT);
    pinMode(irFlavor2, INPUT);
    pinMode(irSugar, INPUT);
    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);
    lcd.init();
    lcd.backlight();
    resetToStart();
    pinMode(11, OUTPUT);
    pinMode(waterPump, OUTPUT);
    digitalWrite(waterPump, HIGH);
    analogWrite(enablePin, 192);
}
void loop()
{
    char key = keypad.getKey();
    if (key)
    {
        keyBuzzer();
        if (key == '*')
        {
            count = 0;
            resetToStart();
        }
        switch (currentState)
        {
        case START:
            if (key == 'A')
            {
                showFlavor();
            }
            else if (key == 'C' && count == 0)
            {
                clean();
            }
            break;
        case SELECT_FLAVOR:
            if (key == '1')
            {
                selectedFlavor = flavors[0];
                showSugar();
            }
            else if (key == '2')
            {
                selectedFlavor = flavors[1];
                showSugar();
            }
            break;
        case SELECT_SUGAR:
            if (key == '1')
            {
                selectedSugar = sugars[0];
                durationSugar = 25000;
                checkItems();
            }
            else if (key == '2')
            {
                selectedSugar = sugars[1];
                durationSugar = 50000;
                checkItems();
            }
            break;
        case SHOW_WARN_WATER:
            if (key == 'A')
            {
                clean();
            }
            break;
        case SHOW_WARN_ITEMS:
            if (key == 'A')
            {
                checkItems();
            }
            break;
        case SHOW_WARN_CUP:
            if (key == 'A')
            {
                filling();
            }
            break;
        }
    }
}
int ultrasonic()
{
    long duration;
    int distance;
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    duration = pulseIn(echoPin, HIGH);
    distance = duration * 0.034 / 2;
    return distance;
}
void resetToStart()
{
    currentState = START;
    selectedFlavor = "";
    selectedSugar = "";
    durationSugar = 0;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Press A to start");
    if (count == 0)
    {
        lcd.setCursor(0, 1);
        lcd.print("Press C to clean");
    }
}
void showFlavor()
{
    currentState = SELECT_FLAVOR;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Select Flavor:");
    delay(1000);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("1:" + flavors[0]);
    lcd.setCursor(0, 1);
    lcd.print("2:" + flavors[1]);
}
void showSugar()
{
    currentState = SELECT_SUGAR;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Select Sugar:");
    delay(1000);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("1:" + sugars[0]);
    lcd.setCursor(0, 1);
    lcd.print("2:" + sugars[1]);
}
void showContinue()
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(alertMessage);
    blinkBuzzer(3000);
    lcd.setCursor(0, 1);
    lcd.print("Press A");
}
void clean()
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Cleaning...");
    if (waterLevel())
    {
        moveServoSmoothly(drainServo, 0, 10);
        digitalWrite(waterPump, LOW);
        delay(6000);
        digitalWrite(waterPump, HIGH);
        runMotorForTime(3);
        moveServoSmoothly(drainServo, 180, 10);
        delay(50000);
        previousFlavor = "";
        if (selectedFlavor == "")
        {
            showFlavor();
        }
        else
        {
            checkItems();
        }
    }
    else
    {
        currentState = SHOW_WARN_WATER;
        alertMessage = "No Water!";
        showContinue();
    }
}
void checkItems()
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Checking...");
    if ((previousFlavor != "") && (previousFlavor != selectedFlavor))
    {
        clean();
    }
    else
    {
        if (!(waterLevel()))
        {
            currentState = SHOW_WARN_ITEMS;
            alertMessage = "No Water!";
            showContinue();
        }
        else if (!(flavorLevel()))
        {
            currentState = SHOW_WARN_ITEMS;
            alertMessage = "No Flavor!";
            showContinue();
        }
        else if (!(sugarLevel()))
        {
            currentState = SHOW_WARN_ITEMS;
            alertMessage = "No Sugar!";
            showContinue();
        }
        else
        {
            makingJuice();
        }
    }
}
void makingJuice()
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Making...");
    moveServoSmoothly(drainServo, 0, 10);
    moveServoSmoothly(tankServo, 95, 25);
    if (selectedFlavor == flavors[0])
    {
        controlSteppers(stepperFlavor1, 30000);
    }
    else if (selectedFlavor == flavors[1])
    {
        controlSteppers(stepperFlavor2, 30000);
    }
    previousFlavor = selectedFlavor;
    digitalWrite(waterPump, LOW);
    delay(13000);
    digitalWrite(waterPump, HIGH);
    runMotorForTime(4);
    count++;
    filling();
}
void filling()
{
    if (placeCup())
    {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Filling...");
        moveServoSmoothly(supplyServo, 180, 5);
        delayWithCheck(48);
        moveServoSmoothly(supplyServo, 0, 5);
        thanks();
    }
    else
    {
        currentState = SHOW_WARN_CUP;
        alertMessage = "No Cup!";
        showContinue();
    }
}
void thanks()
{
    lcd.clear();
    lcd.setCursor(3, 0);
    lcd.print("Thank you!");
    moveServoSmoothly(tankServo, 130, 25);
    moveServoSmoothly(drainServo, 180, 10);
    delay(25000 + extraTime);
    extraTime = 0;
    resetToStart();
}
bool waterLevel()
{
    int sum = 0;
    for (int i = 0; i < 10; i++)
    {
        sum += analogRead(A0);
        delay(100);
    }
    int average = sum / 10;
    if (average > 450)
    {
        return true;
    }
    else
    {
        return false;
    }
}
bool flavorLevel()
{
    if (selectedFlavor == flavors[0])
    {
        if (digitalRead(irFlavor1) == LOW)
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    else if (selectedFlavor == flavors[1])
    {
        if (digitalRead(irFlavor2) == LOW)
        {
            return true;
        }
        else
        {
            return false;
        }
    }
}
bool sugarLevel()
{
    if (digitalRead(irSugar) == LOW)
    {
        return true;
    }
    else
    {
        return false;
    }
}
bool placeCup()
{
    int distance = ultrasonic();
    if (distance >= 6 && distance <= 9)
    {
        return true;
    }
    else
    {
        return false;
    }
}
void controlSteppers(Stepper &stepper, unsigned long durationFlavor)
{
    unsigned long startTime = millis();
    while (millis() - startTime < max(durationFlavor, durationSugar))
    {
        unsigned long currentTime = millis();
        if (currentTime - startTime < durationFlavor)
        {
            stepper.step(stepsPerRevolution / 2048);
        }
        if (currentTime - startTime < durationSugar)
        {
            stepperSugar.step(stepsPerRevolution / 2048);
        }
    }
}
void moveServoSmoothly(Servo &servo, int targetPosition, int delayTime)
{
    int currentPosition = servo.read();
    int step = 1;
    if (currentPosition < targetPosition)
    {
        for (int pos = currentPosition; pos <= targetPosition; pos += step)
        {
            servo.write(pos);
            delay(delayTime);
        }
    }
    else
    {
        for (int pos = currentPosition; pos >= targetPosition; pos -= step)
        {
            servo.write(pos);
            delay(delayTime);
        }
    }
}
void blinkBuzzer(int duration)
{
    unsigned long startTime = millis();
    while (millis() - startTime < duration)
    {
        digitalWrite(11, HIGH);
        delay(250);
        digitalWrite(11, LOW);
        delay(250);
    }
}
void runMotorForTime(int spin)
{
    for (int i = 0; i < spin; i++)
    {
        if (i % 2 == 0)
        {
            digitalWrite(motorPin1, LOW);
            digitalWrite(motorPin2, HIGH);
        }
        else
        {
            digitalWrite(motorPin1, HIGH);
            digitalWrite(motorPin2, LOW);
        }
        delay(3000);
        digitalWrite(motorPin1, LOW);
        digitalWrite(motorPin2, LOW);
        delay(3000);
    }
}
void delayWithCheck(int delayTime)
{
    for (int i = 0; i < delayTime; i++)
    {
        if (!placeCup())
        {
            extraTime = (delayTime - i) * 1000;
            break;
        }
        delay(1000);
    }
}
void keyBuzzer()
{
    digitalWrite(11, HIGH);
    delay(200);
    digitalWrite(11, LOW);
}