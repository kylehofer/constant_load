// Visual Micro is in vMicro>General>Tutorial Mode0
// 
/*
    Name:       constant_load.ino
    Created:	27-Feb-21 9:36:33 AM
    Author:     AzureAD\KyleHofer
*/

// Define User Types below here or use a .h file
//

#include <Wire.h> 
#include <LCD_I2C.h>
#include <Encoder.h>

#define BATTERY_TITLE F("Measure Battery")
#define CURRENT_TITLE F("Current Load")
#define POWER_TITLE F("Power Load")

#define CURRENT_CONFIGURE F("Current:")
#define POWER_CONFIGURE F("Power:")
#define VOLTAGE_LIMIT_CONFIGURE F("Voltage:")

#define DRAW_LINE_1 F("S       C")
#define DRAW_LINE_2 F("Amp:        Stop")

#define TOTAL_TITLE F("Total Capacity")
#define MAH_SYMBOL F("mAh")
#define WA_SYMBOL F("wA")

#define CURRENT_INPUT F("Current")
#define POWER_INPUT F("Power")
#define VOLTAGE_LIMIT_INPUT F("V Limit")
#define DONE_INPUT F("Done")
#define INPUT_TEMPLATE F(":00.00")

#define V_FORMAT F("%2.2f")

LCD_I2C lcd(0x3F); // Default address of most PCF8574 modules, change according


constexpr auto ENCODER_A = 0;
constexpr auto ENCODER_B = 1;
constexpr auto ENCODER_C = 7;

constexpr auto FAN_OUT = 9;
constexpr auto CURRENT_OUT = 10;
constexpr auto CURRENT_IN = 20;
constexpr auto POWER_IN = 19;

constexpr auto CURRENT_MAX = 9.06;
constexpr auto CURRENT_MIN = 0.1;

constexpr auto RAW_TO_VOLTAGE = 12.380 / 1024.0;
constexpr auto RAW_TO_CURRENT_OUTPUT = CURRENT_MAX / 400.0;
constexpr auto CURRENT_TO_RAW_OUTPUT = 400.0 / CURRENT_MAX;
constexpr auto RAW_TO_CURRENT_INPUT = (CURRENT_MAX / 1024.0) / 0.974;

constexpr auto MAX_CURRENT = 10.0;
constexpr auto MILLIS_TO_HOURS = 3600000UL;

constexpr auto PWM_FREQ_HZ = 20000U; //Adjust this value to adjust the frequency
constexpr auto TCNT1_TOP = 16000000U / (PWM_FREQ_HZ << 1);

byte customChar1[8] = {
  0b10000,
  0b11000,
  0b11100,
  0b11110,
  0b11110,
  0b11100,
  0b11000,
  0b10000
};

byte customChar2[8] = {
  0b00100,
  0b01110,
  0b11111,
  0b00000,
  0b00000,
  0b11111,
  0b01110,
  0b00100,
};

enum EncoderEvent { LEFT, RIGHT, CLICK, NONE };

enum MainMenuState { BATTERY, CURRENT, POWER };

enum ValueInputState { TEN, ONE, TENTH, HUNDREDTH, VALUE_BACK };

Encoder ENCODER(0, 1);

double read_voltage()
{
    return ((double)analogRead(POWER_IN)) * RAW_TO_VOLTAGE;
}

double read_current()
{
    return ((double)analogRead(CURRENT_IN)) * RAW_TO_CURRENT_INPUT;
}

void set_current(double value)
{
    OCR1B = (value > CURRENT_MAX) ? 400 : value * CURRENT_TO_RAW_OUTPUT;
}

void build_draw_menu(char first_symbol, char second_symbol)
{
    lcd.clear();
    lcd.print(DRAW_LINE_1);
    lcd.setCursor(6, 0);
    lcd.print(first_symbol);
    lcd.setCursor(14, 0);
    lcd.print(second_symbol);
    lcd.setCursor(0, 1);
    lcd.print(DRAW_LINE_2);
    lcd.setCursor(9, 1);
    lcd.print('A');
    lcd.setCursor(11, 1);
    lcd.write(byte(0));
}

void display_total(long total)
{
    lcd.clear();
    lcd.print(TOTAL_TITLE);
    lcd.setCursor(0, 1);
    lcd.print(total);
    lcd.print(MAH_SYMBOL);
    while (digitalRead(ENCODER_C));
    while (!digitalRead(ENCODER_C));
}

void match_current(double current_in, double target) {
    double new_target = ((((target / current_in) - 1.0) * 3) + 1.0) * current_in;
    
    set_current(new_target > 0 ? new_target : target);
}

void battery_mode()
{
    double current = value_input(CURRENT_INPUT, 8, 'A'), target = value_input(VOLTAGE_LIMIT_INPUT, 8, 'V');

    build_draw_menu('V', 'V');
    double voltage_in, current_in, current_ratio;
    double conversion = 1 / target;

    char buffer[6];

    dtostrf(target, 2, 2, buffer);

    lcd.setCursor(1, 0);
    lcd.print(buffer);

    set_current(current);

    unsigned long start = millis();
    unsigned long milli_amps = 0;
    unsigned long now = start;

    do
    {
        voltage_in = read_voltage();
        dtostrf(voltage_in, 2, 2, buffer);
        lcd.setCursor(9, 0);
        lcd.print(buffer);

        current_in = read_current();
        dtostrf(current_in, 2, 2, buffer);
        lcd.setCursor(4, 1);
        lcd.print(buffer);

        
        if (voltage_in < target) {
            current -= 0.01;
        }

        if (millis() > now) {
            milli_amps += (long)(current_in * 1000) * (millis() - now);
        }

        match_current(current_in, current);
        
    } while (digitalRead(ENCODER_C) && (voltage_in >= target || current >= CURRENT_MIN));

    set_current(0);

    delete buffer;
    while (!digitalRead(ENCODER_C));

    //display_total(((unsigned long)milli_amps * (millis() - start)) / MILLIS_TO_HOURS);
    display_total(milli_amps / MILLIS_TO_HOURS);
}

void current_mode()
{
    double target = value_input(CURRENT_INPUT, 8, 'A');

    build_draw_menu('A', 'V');
    double voltage_in, current_in;
    double conversion = 1 / target;

    char buffer[6];

    dtostrf(target, 2, 2, buffer);

    lcd.setCursor(1, 0);
    lcd.print(buffer);

    set_current(target);

    do
    {
        voltage_in = read_voltage();
        dtostrf(voltage_in, 2, 2, buffer);
        lcd.setCursor(9, 0);
        lcd.print(buffer);

        current_in = read_current();
        dtostrf(current_in, 2, 2, buffer);
        lcd.setCursor(4, 1);
        lcd.print(buffer);

        match_current(current_in, target);
    } while (digitalRead(ENCODER_C));

    set_current(0);

    delete buffer;

    while (!digitalRead(ENCODER_C));
}

void power_mode()
{
    double target = value_input(POWER_INPUT, 6, 'W');

    build_draw_menu('W', 'W');
    double voltage_in, current_in, requested_current;

    char buffer[6];

    dtostrf(target, 2, 2, buffer);

    lcd.setCursor(1, 0);
    lcd.print(buffer);

    do
    {
        voltage_in = read_voltage();
        requested_current = target / voltage_in;

        current_in = read_current();
        dtostrf(current_in, 2, 2, buffer);
        lcd.setCursor(4, 1);
        lcd.print(buffer);

        dtostrf((current_in * voltage_in), 2, 2, buffer);
        lcd.setCursor(9, 0);
        lcd.print(buffer);

        match_current(current_in, requested_current);
    } while (digitalRead(ENCODER_C));

    set_current(0);

    delete buffer;

    while (!digitalRead(ENCODER_C));    
}

void main_menu()
{
    MainMenuState state = BATTERY;
    EncoderEvent input;

    do
    {
        lcd.clear();
        lcd.write((byte)0);
        switch (state)
        {
        case BATTERY:
            lcd.print(BATTERY_TITLE);
            lcd.setCursor(0, 1);
            lcd.print(CURRENT_TITLE);
            break;
        case CURRENT:
            lcd.print(CURRENT_TITLE);
            lcd.setCursor(0, 1);
            lcd.print(POWER_TITLE);
            break;
        default:
            lcd.print(POWER_TITLE);
            lcd.setCursor(0, 1);
            lcd.print(BATTERY_TITLE);
            break;
        }

        input = user_input();

        if (input == LEFT)
        {
            switch (state)
            {
            case BATTERY:
                state = POWER;
                break;
            case CURRENT:
                state = BATTERY;
                break;
            default:
                state = CURRENT;
                break;
            }
            
        }
        else if (input == RIGHT)
        {
            switch (state)
            {
            case BATTERY:
                state = CURRENT;
                break;
            case CURRENT:
                state = POWER;
                break;
            default:
                state = BATTERY;
                break;
            }
        }
    } while (input != CLICK);

    switch (state)
    {
    case BATTERY:
        battery_mode();
        break;
    case CURRENT:
        current_mode();
        break;
    default:
        power_mode();
        break;
    }
}

/**
* .
*/
double value_input(const __FlashStringHelper* name, int start, char symbol)
{
    //int start;// = name.length() + 1;
    ValueInputState state = TEN;

    lcd.clear();

    lcd.print(name);
    lcd.print(INPUT_TEMPLATE);
    lcd.print(symbol);
    lcd.write((byte)1);
    lcd.setCursor(0, 1);
    lcd.print(DONE_INPUT);
    lcd.cursor();

    lcd.setCursor(start, 0);

    int ten = 0, one = 0, tenth = 0, hundredth = 0;
    int position = start;

    const int ten_start = start, one_start = start + 1, tenth_start = start + 3, hundredth_start = start + 4;

    bool done = false;

    EncoderEvent input;

    do
    {
        input = user_input();

        switch (input)
        {
        case LEFT:
            switch (state)
            {
            case TEN:
                state = VALUE_BACK;
                lcd.setCursor(0, 1);
                break;
            case ONE:
                state = TEN;
                lcd.setCursor(ten_start, 0);
                break;
            case TENTH:
                state = ONE;
                lcd.setCursor(one_start, 0);
                break;
            case HUNDREDTH:
                state = TENTH;
                lcd.setCursor(tenth_start, 0);
                break;
            default:
                state = HUNDREDTH;
                lcd.setCursor(hundredth_start, 0);
                break;
            }
            break;
        case RIGHT:
            switch (state)
            {
            case TEN:
                state = ONE;
                lcd.setCursor(one_start, 0);
                break;
            case ONE:
                state = TENTH;
                lcd.setCursor(tenth_start, 0);
                break;
            case TENTH:
                state = HUNDREDTH;
                lcd.setCursor(hundredth_start, 0);
                break;
            case HUNDREDTH:
                state = VALUE_BACK;
                lcd.setCursor(0, 1);
                break;
            default:
                state = TEN;
                lcd.setCursor(ten_start, 0);
                break;
            }
            break;
        default:
            switch (state)
            {
            case TEN:
                ten = digit_input(ten_start, ten);
                break;
            case ONE:
                one = digit_input(one_start, one);
                break;
            case TENTH:
                tenth = digit_input(tenth_start, tenth);
                break;
            case HUNDREDTH:
                hundredth = digit_input(hundredth_start, hundredth);
                break;
            default:
                done = true;
                break;
            }
            break;
        }

    } while (!done);
    lcd.noCursor();
    return (float)ten * 10 + (float)one + (float)tenth * 0.1 + (float)hundredth * 0.01;
}

int digit_input(int position, int value)
{
    EncoderEvent input;
    do
    {
        input = user_input();
        if (input != CLICK) {
            value += (input == RIGHT) ? 1 : -1;
            if (value > 9) {
                value = 0;
            }
            else if (value < 0) {
                value = 9;
            }
            lcd.print(value);
            lcd.setCursor(position, 0);
        }
    } while (input != CLICK);

    return value;
}

/** 
* Waits for a user input, returning the result.
*/
EncoderEvent user_input() {
    bool hasInput = false;
    long currentPosition = ENCODER.read();

    EncoderEvent input = NONE;

    while (input == NONE)
    {
        if (!digitalRead(ENCODER_C))
        {
            input = CLICK;
            while (!digitalRead(ENCODER_C));
        }
        else {
            long newPosition = ENCODER.read();
            if (newPosition > currentPosition)
            {
                input = RIGHT;
            }
            else if (newPosition < currentPosition)
            {
                input = LEFT;
            }

            delay(50);
        }
    };
    return input;
}




// Define Functions below here or use other .ino or cpp files
//

// The setup() function runs once each time the micro-controller starts
void setup()
{

    // Clear Timer1 control and count registers
    TCCR1A = 0;
    TCCR1B = 0;
    TCNT1 = 0;

    // Set Timer1 configuration
    // COM1A(1:0) = 0b10   (Output A clear rising/set falling)
    // COM1B(1:0) = 0b00   (Output B normal operation)
    // WGM(13:10) = 0b1010 (Phase correct PWM)
    // ICNC1      = 0b0    (Input capture noise canceler disabled)
    // ICES1      = 0b0    (Input capture edge select disabled)
    // CS(12:10)  = 0b001  (Input clock select = clock/1)

    TCCR1A |= (1 << COM1A1) | (1 << COM1B1) | (1 << WGM11);
    TCCR1B |= (1 << WGM13) | (1 << CS10);
    ICR1 = TCNT1_TOP;

    pinMode(FAN_OUT, OUTPUT);  // sets the pin as output
    pinMode(CURRENT_OUT, OUTPUT);  // sets the pin as output

    pinMode(CURRENT_IN, INPUT);  // sets the pin as output
    pinMode(POWER_IN, INPUT);  // sets the pin as output

    pinMode(ENCODER_C, INPUT);  // sets the pin as output

    lcd.begin(); // If you are using more I2C devices using the Wire library use lcd.begin(false)
             // this stop the library(LCD_I2C) from calling Wire.begin()
    lcd.backlight();

    lcd.createChar(0, customChar1);
    lcd.createChar(1, customChar2);

    OCR1A = (word)TCNT1_TOP;
}



void loop()
{
    main_menu();
}

/*
if (watts > 40) {
    OCR1A = TCNT1_TOP;
}
else if (watts > 35) {
    OCR1A = 350;
}
else if (watts > 30) {
    OCR1A = 300;
}
else if (watts > 25) {
    OCR1A = 250;
}
else if (watts > 20) {
    OCR1A = 200;
}
*/