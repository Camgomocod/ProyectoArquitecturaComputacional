#include <LiquidCrystal.h> /** Manejo de pantallas LCD */
#include "StateMachineLib.h"/** Implementación de la máquina de estados */
#include <Keypad.h>/** Manejo de teclados matriciales */
#include "AsyncTaskLib.h" /** Manejo de tareas asíncronas */
#include <EasyBuzzer.h> /** Control fácil de zumbadores */
#include <LiquidMenu.h> /** Manejo de menús en pantallas LCD */
#include <DHT.h> /** Sensores de temperatura y humedad */ 
#include <OneButton.h>/** Boton de estados*/


#define ledR 7 /*Blocking/
#define ledG 8 /*Adorno/
#define ledB 9/*Alarma/

#define buttonPin 13/*Boton de cambio de estados/

#define rs 12/*Señal a arduino/
#define en 11/**Señal a arduino */
#define d4 5/**Señal a arduino */
#define d5 4/**Señal a arduino */
#define d6 3/**Señal a arduino */
#define d7 2/**Señal a arduino */

#define enter 1/**Presionar enter */

#define length 6/**Tamaño de la contraseña */
#define maxLength 10/**Tamaño maximo de la contraseña */

#define DHTPIN 10/*Temperatura/
#define DHTTYPE DHT11  /**Humedad */ 

#define photocellPin A0/*Luz/
#define photocellPinGas A1/*Gas/


#define intervals 9/*Intervalos para la medición del promedio/

DHT dht(DHTPIN, DHTTYPE);


OneButton btn = OneButton(
  buttonPin,  // Input pin for the button
  false,        // Button is active LOW
  false         // Enable internal pull-up resistor
);



size_t timer = 10;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

const byte ROWS = 4; // four rows
const byte COLS = 4; // three columns
char keys[ROWS][COLS] = { 
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}};
byte rowPins[ROWS] = {24, 26, 28, 30}; // connect to the row pinouts of the keypad
byte colPins[COLS] = {32, 34, 36, 38}; // connect to the column pinouts of the keypad
/**
 * @brief Keypad inicialización.
 * @param makeKeyMap(keys) teclas del teclado.
 * @param rowPins pines de fila.
 * @param colPins pines de columna.
 * @return La suma de a y b.
 */
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS); // create the keypad
/**
 * @brief Tarea asincrona apagar el led color rojo.
 */
AsyncTask TurnOffLedR(500, []()
                     { digitalWrite(ledR, LOW); }); // Turn off the led after 500ms
/*!
 * @brief Tarea asincrona apagar el led color azul.
 */
AsyncTask TurnOffLedB(800, []()
                    { digitalWrite(ledB,LOW);});



bool executeMenu;
bool executeAlarm;

bool flgCheckTLAlarm ;
bool flgCheckGasAlarm ;

const char password[length] = "A1234B";
char buffer[maxLength];
int attemptsCounter = 0;
int failedAttempts = 0;
int charCount = 0;



int tempHigh = 28;
int tempLow = -16;
int lightHigh = 200;
int lightLow = 800;
int humHigh = 100;
int humLow = 0;
int gasHigh = 1000;
int gasLow = 0;

int newValue = 0;
byte pos = 0;
bool isNegative = false;

int outputValue = 0;


// Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
float h;
// Read temperature as Celsius (the default)
float t;

int l;

int g;

float tempBuffer[intervals];
float lightBuffer[intervals];
int gasBuffer[intervals];

int tempLightIndex = 0;
int gasIndex = 0;

// These are the char arrays stored in flash
const char text1[] PROGMEM = "Env Monitoring";
const char text2[] PROGMEM = "Reset limits";
const char text3[] PROGMEM = "TH TEMP HIGH";
const char text4[] PROGMEM = "TH TEMP LOW";
const char text5[] PROGMEM = "TH LIGHT HIGH";
const char text6[] PROGMEM = "TH LIGHT LOW";
const char text7[] PROGMEM = "TH HUM HIGH";
const char text8[] PROGMEM = "TH HUM LOW";
const char text9[] PROGMEM = "TH GAS HIGH";
const char text10[] PROGMEM = "TH GAS LOW";



/**
 * @brief Resetear limites de controles ambientales.
 */
void resetLimits();
/**
 * @brief Dar valor al maximo de temperatura.
 */
void setTempHigh();
/**
 * @brief Dar valor al minimo de temperatura.
 */
void setTempLow();
/**
 * @brief Dar valor al maximo de luz.
 */
void setLightHigh();
/**
 * @brief Dar valor al minimo de luz.
 */
void setLightLow();
/**
 * @brief Dar valor al maximo de humedad.
 */
void setHumHigh();
/**
 * @brief Dar valor al minimo de humedad.
 */
void setHumLow();
/**
 * @brief Dar valor al maximo de gas.
 */
void setGasHigh();
/**
 * @brief Dar valor al minimo de gas.
 */
void setGasLow();





LiquidLine opt1(0, 0, text1);
LiquidLine opt2(0, 1, text2);
LiquidScreen scr1(opt1, opt2);

LiquidLine opt3(0, 0, text3);
LiquidLine opt4(0, 1, text4);
LiquidScreen scr2(opt3, opt4);

LiquidLine opt5(0, 0, text5);
LiquidLine opt6(0, 1, text6);
LiquidScreen scr3(opt5, opt6);

LiquidLine opt7(0, 0, text7);
LiquidLine opt8(0, 1, text8);
LiquidScreen scr4(opt7, opt8);

LiquidLine opt9(0, 0, text9);
LiquidLine opt10(0, 1, text10);
LiquidScreen scr5(opt9, opt10);

LiquidMenu menu(lcd);


// State Alias
/**
 * @brief Estados de la maquina.
 * @param S_INICIO estado de sesión.
 * @param S_BLOQUEADO estado bloqueado.
 * @param S_CONFIG estado configuración.
 * @param S_MEVENTOS estado de monitoreo de eventos.
 * @param S_MAMBIENTAL estado de monitoreo ambiental.
 * @param S_ALARMA estado de alarma.
 */
enum State
{
    S_INICIO = 0,
    S_BLOQUEADO = 1,
    S_CONFIG = 2,
    S_MEVENTOS = 3,
    S_MAMBIENTAL = 4,
    S_ALARMA = 5
};

// Input Alias
enum Input
{
    TimeOut10 = 0,
    SystBlock = 1,
    CorrectPwd = 2,
    BtnPrs = 3,
    TimeOut2 = 4,
    TimeOut5 = 5,
    GasHigh = 6,
    TempLightHigh = 7,
    TimeOut4 = 8,
    Unknown = 9
};

// Create new StateMachine
/**
 * @brief Estados de la maquina.
 * @param 6 estados.
 * @param 12 transiciones.
 */
StateMachine stateMachine(6, 13);

// Stores last user input
Input input;
/**
 * @brief BlockSquenceMsg es una tarea asincrona que bloquea el mensaje del lcd.
 */
static void handleClick() {
  input = Input::BtnPrs;
}
/**
 * @brief imprime dos renglones en el lcd.
 * @param line1 es un caracter de texto en el lcd.
 * @param line2 es un caracter de texto en el lcd.
 * @param waitTime el tiempo de delay para que acabe la función.
 */
void print2lines(const char *line1, const char *line2, const int waitTime)
{
    lcd.clear();
    lcd.print(line1);
    lcd.setCursor(0, 2);
    lcd.cursor();
    lcd.print(line2);
    delay(waitTime);
}

/**
 * @brief InactivityError es una tarea asincrona que dispara el mensaje y la alarma de error despues de los 10 segundos de inactividad.
 */
AsyncTask InactivityError(10000, true, []()
                    {
                      failedAttempts++;

                      if (failedAttempts >= 3)
                      {
                          lcd.clear();
                          input = Input::SystBlock;
                          return;
                      }

                        print2lines("Inactivity Error", "Try Again", 500);
                      attemptsCounter = 0;
                      charCount = 0;
                      lcd.clear();
                      lcd.print("Input Password:");
                    });
/**
 * @brief BlockSequenceMsg es una tarea asincrona que muestra el mensaje de error despues de los 10 segundos de inactividad.
 */
AsyncTask BlockSequenceMsg(1000, true, []()
                   { 
    timer--;
    if(timer == 0)
    {
        timer = 10;
        input = Input::TimeOut10;
        return;
    }
    lcd.setCursor(0, 2);
    lcd.cursor(); 
    lcd.print("0");     
    lcd.print(timer); });
/**
 * @brief updateMonAmbiental es una tarea asincrona que actualiza los datos del monitoreo cada 5 segundos.
 */
AsyncTask updateMonAmbiental(1000,true, []()
{
    timer++;

    if(timer == 5)
    {
        input = Input::TimeOut5;
        return;
    }

    h = dht.readHumidity();
    t = dht.readTemperature();
    g = analogRead(photocellPinGas);

    l = analogRead(photocellPin) / 4;

    lcd.setCursor(4,0);
    lcd.cursor();
    lcd.print("    ");
    lcd.setCursor(4,0);
    lcd.cursor();
    lcd.print((int)t);
    
    lcd.setCursor(12,0);
    lcd.cursor();
    lcd.print("    ");
    lcd.setCursor(12,0);
    lcd.cursor();
    lcd.print((int)h);
    
    lcd.setCursor(4,1);
    lcd.cursor();
    lcd.print("    ");
    lcd.setCursor(4,1);
    lcd.cursor();
    lcd.print(l);

    // Actualizar los buffers
    /**
    * @brief Actualizar el buffer de temperatura.
    */
    tempBuffer[tempLightIndex] = t;
    /**
    * @brief Actualizar el buffer de luz.
    */
    lightBuffer[tempLightIndex] = l;
    
    gasBuffer[gasIndex] = g;

    if(gasIndex+1 == intervals)
    {
        flgCheckGasAlarm = true;
    }

    gasIndex = (gasIndex + 1) % intervals;

    int gasSum = 0;

    for (int i = 0; i < intervals; i++) {
        gasSum += gasBuffer[i];
    }

    float gasAvg = gasSum / intervals;

    if (gasAvg >= gasHigh) {
        flgCheckGasAlarm = false;
        gasIndex = 0;
        input = Input::GasHigh;
        return;
    }

    if(tempLightIndex+1 == intervals)
    {
        flgCheckTLAlarm = true;
    }
    
    tempLightIndex = (tempLightIndex + 1) % intervals;

    if(!flgCheckTLAlarm && !flgCheckGasAlarm)
    {
        return;
    }

    // Calcular promedios
    float tempSum = 0;
    float lightSum = 0;

    for (int i = 0; i < intervals; i++) {
        tempSum += tempBuffer[i];
        lightSum += lightBuffer[i];
    }
    float tempAvg = tempSum / intervals;
    float lightAvg = lightSum / intervals;

    if (tempAvg >= tempHigh && lightAvg >= lightHigh) {
        tempLightIndex = 0;
        flgCheckTLAlarm = false;
        input = Input::TempLightHigh;
        return;
    }
});
/**
 * @brief updateMonEventos es una tarea asincrona que actualiza los datos del monitoreo de eventos cada 2 segundos.
 */
AsyncTask updateMonEventos(1000, true, []()
{
    timer++;

    if(timer == 2)
    {
        input = Input::TimeOut2;
        return;
    }

    h = dht.readHumidity();
    t = dht.readTemperature();
    g = analogRead(photocellPinGas);
    l = analogRead(photocellPin) / 4;

    lcd.setCursor(4,0);
    lcd.cursor();
    lcd.print("    ");
    lcd.setCursor(4,0);
    lcd.cursor();
    lcd.print(g);

     // Actualizar los buffers
    /**
    * @brief Actualizar el buffer de temperatura.
    */
    tempBuffer[tempLightIndex] = t;
    /**
    * @brief Actualizar el buffer de luz.
    */
    lightBuffer[tempLightIndex] = l;
    
    gasBuffer[gasIndex] = g;

    if(gasIndex+1 == intervals)
    {
        flgCheckGasAlarm = true;
    }

    gasIndex = (gasIndex + 1) % intervals;

    int gasSum = 0;

    for (int i = 0; i < intervals; i++) {
        gasSum += gasBuffer[i];
    }

    float gasAvg = gasSum / intervals;

    if (gasAvg >= gasHigh) {
        flgCheckGasAlarm = false;
        gasIndex = 0;
        input = Input::GasHigh;
        return;
    }

    if(tempLightIndex+1 == intervals)
    {
        flgCheckTLAlarm = true;
    }
    
    tempLightIndex = (tempLightIndex + 1) % intervals;

    if(!flgCheckTLAlarm && !flgCheckGasAlarm)
    {
        return;
    }

    // Calcular promedios
    float tempSum = 0;
    float lightSum = 0;

    for (int i = 0; i < intervals; i++) {
        tempSum += tempBuffer[i];
        lightSum += lightBuffer[i];
    }
    float tempAvg = tempSum / intervals;
    float lightAvg = lightSum / intervals;

    if (tempAvg >= tempHigh && lightAvg >= lightHigh) {
        tempLightIndex = 0;
        flgCheckTLAlarm = false;
        input = Input::TempLightHigh;
        return;
    }
});

AsyncTask TimeOutAlarm(4000, []()
{
  input = Input::TimeOut4;
});
// Setup the State Machine
/**
 * @brief setupStateMachine inicializa los estados y las transiciones de los estados, de la maquina de estados.
 */
// Setup the State Machine
void setupStateMachine()
{
    // Add transitions
    stateMachine.AddTransition(S_INICIO, S_BLOQUEADO, []()
                               { return input == SystBlock; });
    stateMachine.AddTransition(S_INICIO, S_CONFIG, []()
                               { return input == CorrectPwd; });

    stateMachine.AddTransition(S_BLOQUEADO, S_INICIO, []()
                               { return input == TimeOut10; });

    stateMachine.AddTransition(S_CONFIG, S_MAMBIENTAL, []()
                               { return input == BtnPrs; });

    stateMachine.AddTransition(S_MEVENTOS, S_CONFIG, []()
                               { return input == BtnPrs; });
    stateMachine.AddTransition(S_MEVENTOS, S_MAMBIENTAL, []()
                               { return input == TimeOut2; });
    stateMachine.AddTransition(S_MEVENTOS, S_ALARMA, []()
                               { return input == GasHigh; });

    stateMachine.AddTransition(S_MAMBIENTAL, S_CONFIG, []()
                               { return input == BtnPrs; });
    stateMachine.AddTransition(S_MAMBIENTAL, S_MEVENTOS, []()
                               { return input == TimeOut5; });
    stateMachine.AddTransition(S_MAMBIENTAL, S_ALARMA, []()
                               { return input == TempLightHigh; });


    stateMachine.AddTransition(S_ALARMA, S_INICIO, []()
                               { return input == BtnPrs; });
    stateMachine.AddTransition(S_ALARMA, S_MAMBIENTAL, []()
                               { return input == TimeOut4; });

    // Add actions
    stateMachine.SetOnEntering(S_INICIO, outputA);
    stateMachine.SetOnEntering(S_BLOQUEADO, outputB);
    stateMachine.SetOnEntering(S_CONFIG, outputC);
    stateMachine.SetOnEntering(S_MEVENTOS, outputD);
    stateMachine.SetOnEntering(S_MAMBIENTAL, outputE);
    stateMachine.SetOnEntering(S_ALARMA, outputF);

    stateMachine.SetOnLeaving(S_INICIO, []()
                              { Serial.println("Leaving A"); 
                                InactivityError.Stop();});
    stateMachine.SetOnLeaving(S_BLOQUEADO, []()
                              { Serial.println("Leaving B"); 
                                BlockSequenceMsg.Stop();
                                EasyBuzzer.stopBeep();
                                });

    stateMachine.SetOnLeaving(S_CONFIG, []()
                              { Serial.println("Leaving C");
                                  executeMenu = false; });
    stateMachine.SetOnLeaving(S_MEVENTOS, []()
                              { Serial.println("Leaving D"); 
                                updateMonEventos.Stop();});
    stateMachine.SetOnLeaving(S_MAMBIENTAL, []()
                              { Serial.println("Leaving E"); 
                                updateMonAmbiental.Stop();});
    stateMachine.SetOnLeaving(S_ALARMA, []()
                              { Serial.println("Leaving F"); 
                                EasyBuzzer.stopBeep();
                                TimeOutAlarm.Stop();});
}

void setup()
{
    lcd.begin(16, 2);
    Serial.begin(9600);
    dht.begin();

    pinMode(ledR, OUTPUT);
    pinMode(ledG, OUTPUT);
    pinMode(ledB, OUTPUT);

    digitalWrite(ledR, LOW);
    digitalWrite(ledG, LOW);
    digitalWrite(ledB, LOW);

    pinMode(buttonPin, INPUT);  // Button pin as an input.

    TurnOffLedR.Start();
    EasyBuzzer.setPin(14);

    Serial.println("Starting State Machine...");
    setupStateMachine();
    Serial.println("Start Machine Started");

    // Initial state
    stateMachine.SetState(S_INICIO, false, true);

    // Set the variables as PROGMEM. The parameter is the consecutive
    // number of the PROGMEM variable for that LiquidLine object.
    opt1.set_asProgmem(1);
    opt2.set_asProgmem(1);
    opt3.set_asProgmem(1);
    opt4.set_asProgmem(1);
    opt5.set_asProgmem(1);
    opt6.set_asProgmem(1);
    opt7.set_asProgmem(1);
    opt8.set_asProgmem(1);
    opt9.set_asProgmem(1);
    opt10.set_asProgmem(1);

    opt1.attach_function(enter, gotoMonAmbiental);
    opt2.attach_function(enter, resetLimits);
    opt3.attach_function(enter, setTempHigh);
    opt4.attach_function(enter, setTempLow);
    opt5.attach_function(enter, setLightHigh);
    opt6.attach_function(enter, setLightLow);
    opt7.attach_function(enter, setHumHigh);
    opt8.attach_function(enter, setHumLow);
    opt9.attach_function(enter, setGasHigh);
    opt10.attach_function(enter, setGasLow);


    menu.add_screen(scr1);
    menu.add_screen(scr2);
    menu.add_screen(scr3);
    menu.add_screen(scr4);
    menu.add_screen(scr5);

    // Single Click event attachment
    btn.attachClick(handleClick);

    btn.setDebounceMs(5);
}


void loop()
{

    // Update State Machine
    stateMachine.Update();

    TurnOffLedR.Update();
    TurnOffLedB.Update();


    if(updateMonAmbiental.IsActive())
    {
        updateMonAmbiental.Update();
    }

    if(updateMonEventos.IsActive())
    {
        updateMonEventos.Update();
    }

    if (BlockSequenceMsg.IsActive())
    {
        BlockSequenceMsg.Update();
    }

    if(TimeOutAlarm.IsActive())
    {
        TimeOutAlarm.Update();
    }

    btn.tick();

    if (executeMenu)
    {
      myMenu();
    }




}

// Auxiliar output functions that show the state debug
/**
 * @brief outputA muestra en el serial el estado A y en el lcd pide la contraseña.
 */
void outputA()
{
    Serial.println("A   B   C   D   E   F");
    Serial.println("X            ");
    Serial.println();
    lcd.clear();
    lcd.print("Input Password:");

    charCount=0;
    attemptsCounter=0;

    while (security() && input != SystBlock)
    {
        if(attemptsCounter == 0)
        {
            InactivityError.Stop();
        }

        if(attemptsCounter == 1 && InactivityError.IsActive() == false)
        {
          InactivityError.Start();
        }

        if(InactivityError.IsActive())
        {
            InactivityError.Update();
        }
    }
}
/**
 * @brief outputB muestra en el serial el estado B y bloquea el sistema.
 */
void outputB()
{
    Serial.println("A   B   C   D   E   F");
    Serial.println("    x            ");
    Serial.println();
    input = Input::Unknown;
    blocking();
}
/**
 * @brief outputC muestra en el serial el estado C y en el lcd muestra el menu principal.
 */
void outputC()
{
    Serial.println("A   B   C   D   E   F");
    Serial.println("        X    ");
    Serial.println();
    input = Input::Unknown;
    menu.update();
    executeMenu = true;
}
/**
 * @brief outputD muestra en el serial el estado D y en el lcd muestra el monitoreo de eventos.
 */
void outputD()
{
    Serial.println("A   B   C   D   E   F");
    Serial.println("            X    ");
    Serial.println();
    input = Input::Unknown;
    showMonEventos();
}
/**
 * @brief outputE muestra en el serial el estado E y en el lcd muestra el monitoreo ambiental.
 */
void outputE()
{
    Serial.println("A   B   C   D   E   F");
    Serial.println("                X   ");
    Serial.println();
    input = Input::Unknown;
    showMonAmbiental();
}
/**
 * @brief outputF muestra en el serial el estado E y tira la alarma.
 */
void outputF()
{
    Serial.println("A   B   C   D   E   F");
    Serial.println("                    X");
    Serial.println();

    digitalWrite(ledB, HIGH);
    TurnOffLedB.Start();
    executeAlarm=true;
    EasyBuzzer.beep(15);

    print2lines("Alarm Activated", "Press Button", 100);
    TimeOutAlarm.Start();
}
/**
 * @brief security valida que la contraseña sea correcta con 3 intentos para bloquearse .
 */
bool security()
{
    // Configuramos lcd para cada loop
    lcd.setCursor(charCount, 2);
    char key = keypad.getKey();
    lcd.cursor();

    if (!key)
    {
        return true;
    }

    InactivityError.Reset();


    if (attemptsCounter < maxLength-1 && key != '#')
    {
        buffer[attemptsCounter] = key;
        Serial.print(key);
        lcd.print("*");
        attemptsCounter++;
        charCount++;
        return true;
    }

    buffer[attemptsCounter] = '\0';

    if (strcmp(password, buffer) == 0)
    {
        print2lines("Access Granted", "Welcome!", 500);
        lcd.clear();
        input = Input::CorrectPwd;

        for(int i = 0; i < maxLength; i++)
        {
            buffer[i] = ' ';
        }
        return false;
    }

    failedAttempts++;

    if (failedAttempts >= 3)
    {
        lcd.clear();
        input = Input::SystBlock;
        return false;
    }

    print2lines("Access Denied", "Try Again", 500);
    attemptsCounter = 0;
    charCount = 0;
    lcd.clear();
    lcd.print("Input Password:");
    return true;
}
/**
 * @brief blocking bloquea el sistema.
 */
void blocking()
{
    digitalWrite(ledR, HIGH); // Enciende el led y lo apaga despues de 500ms
    TurnOffLedR.Start();

    EasyBuzzer.singleBeep(120, 10000);

    timer=10;
    BlockSequenceMsg.Start();

    lcd.clear();
    lcd.print("System Blocked");
    lcd.setCursor(0, 2);
    lcd.cursor();
    lcd.print(timer);
    lcd.setCursor(3, 2);
    lcd.cursor();
    lcd.print("Seconds left");

    failedAttempts = 0;
    attemptsCounter = 0;
    charCount = 0;
}


/**
 * @brief myMenu es el menú de estados.
 */
void myMenu()
{
    char key = keypad.getKey();


    if (key == 'B')
    {
        menu.next_screen();
        return;
    }

    if (key == 'A')
    {
        menu.previous_screen();
        return;
    }

    if (key == '#')
    {
        menu.call_function(enter);
        return;
    }

    if (key == '*')
    {
        menu.switch_focus();
        return;
    }
}
/**
 * @brief printLineValue imprime un valor en una linea en especifico.
 * @param spaces son los espacios que va a saltar el valor. 
 * @param msg es el mensaje o valor.
 * @param spacesValue cuantos spacios tenga el mensaje. 
 * @param value valor numerico.
 */
void printLineValue(const byte spaces, const char *msg, const byte spacesValue, const int value)
{
    lcd.clear();
    lcd.setCursor(spaces, 0);
    lcd.cursor();
    lcd.print(msg);
    lcd.setCursor(spacesValue, 1);
    lcd.cursor();
    lcd.print(value);
}
/**
 * @brief clearLine borra una linea entera del lcd.
 */
void clearLine(int line)
{
    lcd.setCursor(0, line);
    lcd.cursor();
    for (int i = 0; i < 16; i++)
    {
        lcd.print(" ");
    }
    lcd.setCursor(0, line); // Vuelve a colocar el cursor al inicio de la línea
}
/**
 * @brief Verifica y procesa la entrada de teclado para un sensor con límites y repetición.
 * 
 * Esta función permite la entrada de valores numéricos desde un teclado y los procesa
 * asegurándose de que estén dentro de ciertos límites, y puede repetir los valores si es necesario.
 * 
 * @param startNumber El número inicial para el sensor.
 * @param posStart La posición de inicio en la pantalla LCD para mostrar el valor.
 * @param inferiorLimit El límite inferior permitido para el valor del sensor.
 * @param superiorLimit El límite superior permitido para el valor del sensor.
 * @return true si la entrada fue válida y se debe seguir procesando, false si se debe finalizar la entrada.
 * 
 * @details La función interactúa con un teclado numérico y un LCD para permitir la entrada de valores numéricos
 * dentro de los límites especificados. También maneja la repetición de dígitos y la conversión de números negativos.
 * Los siguientes botones del teclado se utilizan para las operaciones:
 * - '0-9': Entrada de dígitos numéricos.
 * - 'A': Para ingresar números negativos si el límite inferior es menor que cero.
 * - 'B': Para retroceder y corregir la entrada.
 * - '#': Para confirmar la entrada y finalizar el proceso.
 * - '*': Para cancelar la entrada y volver al valor inicial.
 * 
 * @note Esta función asume la existencia de un objeto keypad que proporciona métodos para obtener la entrada del teclado,
 * y un objeto lcd que se utiliza para la visualización en la pantalla LCD.
 */
bool sensorLimitRepeatable(const int startNumber, const byte posStart, const int inferiorLimit, const int superiorLimit)
{
    byte digit = 0;

    char key = keypad.getKey();

    if (!key)
    {
        return true;
    }

    // Check if the key is 0 and the position is the start position
    if (key == '0' 
        && (pos == posStart || (isNegative && pos == posStart + 1) 
        || (isNegative && ((newValue * -10 ) < inferiorLimit)) 
        || (!isNegative && ((newValue * 10)) > superiorLimit)))
    {
        return true;
    }

    if (key == '0')
    {
        newValue = newValue * 10 ;

        lcd.setCursor(pos, 2);
        lcd.cursor();
        lcd.print("0");

        pos++;
        return true;
    }

    // Check if the key is A(-) and the position is the start position and the number is negative
    if (key == 'A' && pos == posStart && inferiorLimit < 0)
    {
        isNegative = true;
        clearLine(1);
        lcd.setCursor(pos, 2);
        lcd.cursor();
        lcd.print("-");
        pos++;
        return true;
    }

    // Check if the key is a number between 1 and 9
    if (key > '0' && key <= '9')
    {
        digit = key - '0';

        if (isNegative && (((newValue * -10) + digit) < inferiorLimit))
        {
            return true;
        }

        if (!isNegative && ((newValue * 10) + digit) > superiorLimit)
        {
            return true;
        }

        if (pos == posStart)
        {
            clearLine(1);
        }

        newValue = newValue * 10 + digit;

        lcd.setCursor(pos, 2);
        lcd.cursor();
        lcd.print(key);

        pos++;
        return true;
    }

    // Check if the key is B (Backspace) and the position is the start position (negative case)
    if (key == 'B' && isNegative && pos == posStart + 1)
    {
        isNegative = false;
        pos--;
        lcd.setCursor(pos, 2);
        lcd.cursor();
        lcd.print(" ");
        return true;
    }

    // Check if the key is B (Backspace) and the position is greater than the start position
    if (key == 'B' && pos > posStart)
    {
        pos--;

        newValue = newValue / 10;

        lcd.setCursor(pos, 2);
        lcd.cursor();
        lcd.print(" ");
        return true;
    }

    // Check if the key is # (Enter) and the position is greater than the start position
    if (key == '#' && ( (!isNegative && pos > posStart) || (isNegative && pos > posStart + 1) ))
    {
        menu.update();
        if (isNegative)
        {
            newValue *= -1;
        }
        return false;
    }

    // Check if the key is * (Back) and the position is greater than the start position
    if (key == '*')
    {
        menu.update();
        newValue = startNumber;
        return false;
    }

    return true;
}
/**
 * @brief Establece el valor de un sensor dentro de ciertos límites mediante entrada interactiva.
 * 
 * Esta función permite establecer el valor de un sensor dentro de ciertos límites especificados
 * mediante una entrada interactiva desde un teclado. El usuario puede ingresar valores numéricos
 * y confirmar la selección una vez que esté satisfecho.
 * 
 * @param sensorValue Referencia al valor del sensor que se actualizará con el valor seleccionado.
 * @param posStart La posición de inicio en la pantalla LCD para mostrar el valor seleccionado.
 * @param inferiorLimit El límite inferior permitido para el valor del sensor.
 * @param superiorLimit El límite superior permitido para el valor del sensor.
 * 
 * @details Esta función se encarga de coordinar la entrada interactiva del usuario y la actualización
 * del valor del sensor. Utiliza la función sensorLimitRepeatable para procesar la entrada del teclado
 * y asegurarse de que los valores estén dentro de los límites especificados. Una vez que se confirma un valor,
 * actualiza la referencia sensorValue con el nuevo valor seleccionado.
 * 
 * @note Esta función asume que la función sensorLimitRepeatable está definida y disponible para su uso.
 * 
 * @see sensorLimitRepeatable
 */
bool setSensorLimit(int &sensorValue, const byte posStart, const int inferiorLimit, const int superiorLimit)
{
    if (superiorLimit < inferiorLimit)
    {
        return;
    }

    pos = posStart;
    newValue = 0;
    isNegative = false;

    while (sensorLimitRepeatable(sensorValue, posStart, inferiorLimit, superiorLimit))
    {
    }

    sensorValue = newValue;
}





//Ya esta comentado en la declaración->


void setTempHigh()
{
    const byte posStart = 6;
    const byte spaces = 3;
    printLineValue(spaces, "Temp High", posStart, tempHigh);
    setSensorLimit(tempHigh, posStart, tempLow, 50);
}

void setTempLow()
{
    const byte posStart = 6;
    const byte spaces = 3;
    printLineValue(spaces, "Temp Low", posStart, tempLow);
    setSensorLimit(tempLow, posStart, -50, tempHigh);
}

void setLightHigh()
{
    const byte posStart = 6;
    const byte spaces = 3;
    printLineValue(spaces, "Light High", posStart, lightHigh);
    setSensorLimit(lightHigh, posStart, lightLow, 1000);
}

void setLightLow()
{
    const byte posStart = 6;
    const byte spaces = 3;
    printLineValue(spaces, "Light Low", posStart, lightLow);
    setSensorLimit(lightLow, posStart, 0, lightHigh);
}

void setHumHigh()
{
    const byte posStart = 7;
    const byte spaces = 3;
    printLineValue(spaces, "Hum High", posStart, humHigh);
    setSensorLimit(humHigh, posStart, humLow, 100);
}

void setHumLow()
{
    const byte posStart = 7;
    const byte spaces = 3;
    printLineValue(spaces, "Hum Low", posStart, humLow);
    setSensorLimit(humLow, posStart, 0, humHigh);
}

void setGasHigh()
{
    const byte posStart = 8;
    const byte spaces = 3;
    printLineValue(spaces, "Gas High", posStart, gasHigh);
    setSensorLimit(gasHigh, posStart, gasLow, 2000);
}

void setGasLow()
{
    const byte posStart = 8;
    const byte spaces = 3;
    printLineValue(spaces, "Gas Low", posStart, gasLow);
    setSensorLimit(gasLow, posStart, 0, gasHigh);
}

void resetLimits()
{
    tempHigh = 28;
    tempLow = -16;
    lightHigh = 200;
    lightLow = 800;
    humHigh = 100;
    humLow = 0;
    gasHigh = 1000;
    gasLow = 0;
}   

void gotoMonAmbiental()
{
    input = Input::BtnPrs;
}

void showMonAmbiental()
{
    lcd.clear();
    lcd.print(F("TEM:"));
    lcd.setCursor(8, 0);
    lcd.print(F("HUM:"));
    lcd.setCursor(0, 1);
    lcd.print(F("LUZ:"));
    
    h = dht.readHumidity();
    t = dht.readTemperature();

    lcd.setCursor(4,0);
    lcd.cursor();
    lcd.print((int)t);
    
    lcd.setCursor(12,0);
    lcd.cursor();
    lcd.print((int)h);
    
    lcd.setCursor(4,1);
    lcd.cursor();
    lcd.print(analogRead(photocellPin) / 4);

    Serial.print(analogRead(photocellPin));

    timer=0;
    updateMonAmbiental.Start();
}
/**
 * @brief Enciende la alarma.
*/
void showMonEventos()
{
    lcd.clear();
    lcd.print("Gas:");
    lcd.print(analogRead(photocellPinGas));

    timer=0;
    updateMonEventos.Start();
}
