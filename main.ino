/**
 * @file Main.cpp
 * @brief Control del sistema basado en máquina de estados para monitoreo ambiental.
 *
 * Este código implementa una máquina de estados que incluye monitoreo ambiental,
 * detección de eventos, y alarmas utilizando sensores y un LCD.
 */

#include "StateMachineLib.h"
#include "AsyncTaskLib.h"
#include "DHT.h"
#include <LiquidCrystal.h>
#include <Keypad.h>

// ====================== Configuración de Hardware ======================

/**
 * @brief Configuración de pines y variables para los dispositivos conectados.
 * 
 * Se configura el LCD, el teclado matricial, y los sensores conectados a los pines 
 * definidos en el código, como el sensor DHT22 (para temperatura y humedad), 
 * sensor de luz LDR, sensor PIR para movimiento, y un sensor Hall para eventos magnéticos.
 */

const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

const byte ROWS = 4, COLS = 4; /**< Tamaño del teclado matricial */
char keys[ROWS][COLS] = { /**< Definición del mapa del teclado */
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}};
byte rowPins[ROWS] = {24, 26, 28, 30}; /**< Pines de las filas del teclado */
byte colPins[COLS] = {32, 34, 36, 38}; /**< Pines de las columnas del teclado */
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);


#define DHTPIN 46 /**< Pin para el sensor DHT22 */
#define DHTTYPE DHT22 /**< Tipo de sensor DHT */
DHT dht(DHTPIN, DHTTYPE);

#define TEMPERATURA_UMBRAL 30.0 /**< Umbral de temperature para activar la alarma */
#define PIN_SENSOR_HALL A1      /**< Pin del sensor Hall */
#define BUZZER_PIN 43           /**< Pin del buzzer */
#define LDR_PIN A0              /**< Pin del sensor de luz */
#define PIN_PIR 48              /**< Pin del sensor PIR */
#define LED_PIN 13              /**< Pin del LED */

/**
 * @brief Variables globales que gestionan el estado del sistema, la entrada de datos,
 *        los sensores y el control de la máquina de estados.
 */

int failedAttempts = 0; /**< Contador de intentos fallidos de contraseña */

enum State
{
    INICIO,                    /**< Estado inicial donde se solicita la contraseña */
    BLOQUEADO,                 /**< Estado bloqueado después de múltiples intentos fallidos */
    MONITOREO_AMBIENTAL,       /**< Estado de monitoreo de temperatura, humedad y luz */
    MONITOR_EVENTOS,           /**< Estado que detecta eventos como movimiento o campos magnéticos */
    ALARMA                     /**< Estado de alarma cuando se activa algún sensor crítico */
};

enum Input
{
    INPUT_T,       /**< Transición por tiempo */
    INPUT_P,       /**< Transición por sensor PIR */
    INPUT_S,       /**< Transición por otro sensor o evento */
    INPUT_UNKNOWN  /**< Entrada desconocida */
};

StateMachine stateMachine(5, 8); /**< Máquina de estados */
Input input = INPUT_UNKNOWN; /**< Variable que almacena el tipo de entrada que se recibe */


String inputPassword = "";        /**< Contraseña ingresada */
String correctPassword = "1234"; /**< Contraseña correcta */
float temperature = 0.0, humidity = 0.0; /**< Variables de sensores */
int luz = 0, pirEstado = 0; /**< Variables de sensores */

bool triggerTransaction = false; /**< Indicador de transición automática */

unsigned long lasTimeLed = 0; /**< Tiempo del último parpadeo de LED */
unsigned long lastTimeBuzzer = 0; /**< Tiempo del último cambio de estado del buzzer */

unsigned long ledInterval = 500; /**< Intervalo de parpadeo del LED */
bool ledState = false; /**< Estado del LED */
bool buzzerState = false; /**< Estado del buzzer */

// ====================== AsyncTasks ======================
/**
 * @brief Tarea asincrónica para leer la temperatura y humedad.
 * 
 * Esta tarea se ejecuta cada 500 ms y se encarga de leer los valores del sensor DHT 
 * para obtener la temperatura y la humedad. Si la temperatura supera un umbral
 * predefinido (definido por `TEMPERATURA_UMBRAL`), se activa una transición hacia
 * el estado de alarma (`input = INPUT_P`), indicando que la temperatura ha excedido
 * el límite permitido.
 */
AsyncTask TaskTemperatura(500, true, []() {
  temperature = dht.readTemperature();
    humidity = dht.readHumidity();
    if (temperature > TEMPERATURA_UMBRAL && stateMachine.GetState() == MONITOREO_AMBIENTAL)
    {
        input = INPUT_P;
        Serial.println("Temperatura excede el umbral. Activando alarma");
    }
});
/**
 * @brief Tarea asincrónica para leer el valor del sensor de luz.
 * 
 * Esta tarea se ejecuta cada 500 ms y lee el valor del sensor de luz (LDR) conectado
 * al pin `LDR_PIN`. El valor leído se almacena en la variable `luz`. Esta tarea no 
 * desencadena transiciones o cambios de estado, pero sirve para monitorear la luz 
 * ambiente.
 */
AsyncTask TaskLuz(500, true, []() {
    luz = analogRead(LDR_PIN);
});
/**
 * @brief Tarea asincrónica para leer el estado del sensor de movimiento (PIR).
 * 
 * Esta tarea se ejecuta cada 500 ms y lee el estado del sensor PIR para detectar movimiento.
 * Si el sensor detecta movimiento (valor `HIGH`), y el sistema está en el estado 
 * `MONITOR_EVENTOS`, se activa una transición hacia el estado de alarma (`input = INPUT_S`),
 * indicando que se ha detectado movimiento.
 */
AsyncTask TaskInfraRojo(500, true, []() {
  pirEstado = digitalRead(PIN_PIR);
  if (pirEstado == HIGH && stateMachine.GetState() == MONITOR_EVENTOS){
    input = INPUT_S;
    Serial.println("Movimiento detectado. Activando alarma");
  }
});
/**
 * @brief Tarea asincrónica para activar la transición del estado MONITOREO_AMBIENTAL.
 * 
 * Esta tarea se ejecuta cada 5000 ms y verifica si el sistema está en el estado
 * `MONITOREO_AMBIENTAL`. Si es así, activa una transacción (cambio de estado) mediante
 * la variable `triggerTransaction`. Esta transición puede ser usada para pasar de
 * monitoreo a un estado de alarma o para realizar otras acciones según sea necesario.
 */
AsyncTask TaskMonitoreoAmbiental(5000, false, []() {
  if (stateMachine.GetState() == MONITOREO_AMBIENTAL){
    triggerTransaction = true;
  }
});
/**
 * @brief Tarea asincrónica para activar la transición del estado MONITOR_EVENTOS.
 * 
 * Similar a la tarea de monitoreo ambiental, esta tarea se ejecuta cada 3000 ms y verifica
 * si el sistema está en el estado `MONITOR_EVENTOS`. Si es así, activa una transacción
 * mediante la variable `triggerTransaction`, que puede ser utilizada para procesar eventos
 * y posibles cambios de estado.
 */
AsyncTask TaskMonitorEventos(3000, false, []() {
  if (stateMachine.GetState() == MONITOR_EVENTOS){
    triggerTransaction = true;
  }
});
/**
 * @brief Tarea asincrónica para manejar el tiempo de bloqueo.
 * 
 * Esta tarea se ejecuta cada 7000 ms y verifica si el sistema está en el estado `BLOQUEADO`.
 * Si es así, después de 7 segundos de bloqueo, activa una transición de vuelta al estado
 * `INICIO` (input = INPUT_T), indicando que el tiempo de bloqueo ha finalizado. Esta tarea
 * simula un bloqueo temporal para evitar que un usuario ingrese contraseñas incorrectas
 * repetidamente sin consecuencias.
 */
AsyncTask TaskBloqueoTiempo(7000, false, []() {
	if (stateMachine.GetState() == BLOQUEADO){
    input = INPUT_T; // Transición al estado INICIO
    Serial.println("Tiempo de bloqueo terminado. Regresando al estado INICIO.");
  }
});

// ====================== Funciones Estados ======================
/**
 * @brief Función auxiliar que controla el parpadeo del LED en el sistema.
 * 
 * El LED se alterna entre encendido y apagado en base a un intervalo de tiempo.
 * 
 * @param interval Intervalo de tiempo en milisegundos entre cambios de estado del LED.
 */
void keepLed(unsigned long interval){
  if (millis() - lasTimeLed >= interval){
    lasTimeLed = millis();
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState);
  }
}
/**
 * @brief Función auxiliar que controla el comportamiento del buzzer en el sistema.
 * 
 * El buzzer alterna entre dos frecuencias para simular una alerta sonora en caso de
 * una alarma activa.
 */
void keepBuzzer(){

  if (millis() - lastTimeBuzzer >= 150){
    lastTimeBuzzer = millis();
    buzzerState = !buzzerState;
    if (buzzerState){
      tone(BUZZER_PIN, 1000); 
    }else{
      tone(BUZZER_PIN, 1500); 
    }
  }
}

// ====================== Funciones de Estados ======================
/**
 * @brief Acción que se ejecuta al entrar en el estado BLOQUEADO.
 * 
 * En este estado, se muestra un mensaje en el LCD que indica que el sistema está bloqueado,
 * se activa el parpadeo del LED y se emite un sonido constante del buzzer. Además, inicia
 * un temporizador que retornará al estado INICIO después de 7 segundos.
 */
void blockade(){
    lcd.clear();
    lcd.print("BLOQUEADO 7s");
    Serial.println("Entrando al estado BLOQUEADO");
    ledInterval = 500; 
    tone(BUZZER_PIN, 500); 
	TaskBloqueoTiempo.Start(); 
}
/**
 * @brief Acción que se ejecuta al salir del estado BLOQUEADO.
 * 
 * En este estado, se detienen el buzzer y el LED, y se limpia el LCD.
 */
void exitBlockade(){
    Serial.println("Saliendo del estado BLOQUEADO");
    noTone(BUZZER_PIN);
    digitalWrite(LED_PIN, LOW);
	TaskBloqueoTiempo.Stop();
}
/**
 * @brief Acción que se ejecuta al entrar en el estado INICIO.
 * 
 * En este estado, el usuario debe ingresar una contraseña para continuar. Se muestra un
 * mensaje en el LCD para que el usuario ingrese su contraseña. Si se ingresan tres intentos
 * fallidos consecutivos, el sistema cambiará al estado BLOQUEADO.
 */
void home(){
	pirEstado =0;
    input = INPUT_UNKNOWN;
    inputPassword = "";
    lcd.clear();
	lcd.print("Ingrese clave:");
	Serial.println("Entrando al estado INICIO");
}
/**
 * @brief Acción que se ejecuta al salir del estado INICIO.
 * 
 * Se limpia el LCD y se reinicia el estado de la contraseña.
 */
void exitHome(){
    Serial.println("Saliendo del estado INICIO");
    lcd.clear();
}
/**
 * @brief Acción que se ejecuta al entrar en el estado MONITOREO_AMBIENTAL.
 * 
 * En este estado, el sistema lee la temperatura, humedad y el nivel de luz ambiente,
 * y muestra estos valores en el LCD. Además, se inician las tareas asincrónicas para
 * realizar lecturas periódicas de los sensores.
 */
void monitoring(){
    lcd.clear();
    
    temperature = dht.readTemperature();
    humidity = dht.readHumidity();
    luz = analogRead(LDR_PIN);
    lcd.print("T: ");
    lcd.print(temperature, 1);
    lcd.print("C H:");
    lcd.print(humidity, 1);
    lcd.setCursor(0, 1);
    lcd.print("Luz: ");
    lcd.print(luz);
    Serial.println("Entrando al estado MONITOREO AMBIENTAL");
    TaskTemperatura.Start();
    TaskLuz.Start();
    TaskMonitoreoAmbiental.Start();
    triggerTransaction = false;
}
/**
 * @brief Acción que se ejecuta al salir del estado MONITOREO_AMBIENTAL.
 * 
 * Se detienen las tareas asincrónicas de monitoreo de sensores y se limpia el LCD.
 */
void exitMonitoring(){
    Serial.println("Saliendo del estado MONITOREO AMBIENTAL");
    TaskTemperatura.Stop();
    TaskLuz.Stop();
    TaskMonitoreoAmbiental.Stop();
    lcd.clear();
}
/**
 * @brief Acción que se ejecuta al entrar en el estado MONITOR_EVENTOS.
 * 
 * Este estado detecta eventos como el movimiento a través del sensor PIR y el campo
 * magnético a través del sensor Hall. Si alguno de estos eventos es activado, el
 * sistema entrará en el estado de ALARMA.
 */
void events(){
    lcd.clear();

    int estadoBotonHall = digitalRead(PIN_SENSOR_HALL);

    pirEstado = digitalRead(PIN_PIR);

    lcd.print("PIR: ");
    lcd.print(pirEstado ? "Activo" : "Inactivo");
    lcd.setCursor(0, 1);
    lcd.print("HALL: ");
    lcd.print(estadoBotonHall == HIGH ? "ALTO" : "BAJO");

    if (estadoBotonHall == HIGH)
    {
        input = INPUT_S;
        Serial.println("Campo magnético alto. Activando alarma");
    }
    Serial.print("Estado del botón Hall: ");
    Serial.println(estadoBotonHall == HIGH ? "ALTO" : "BAJO");
    Serial.println("Entrando al estado MONITOR EVENTOS");
    TaskInfraRojo.Start();
    TaskMonitorEventos.Start();
    triggerTransaction = false;
}
/**
 * @brief Acción que se ejecuta al salir del estado MONITOR_EVENTOS.
 * 
 * Se detienen las tareas de detección de eventos y se limpia el LCD.
 */
void exitEvents(){
    Serial.println("Saliendo del estado MONITOR EVENTOS");
    TaskInfraRojo.Stop();
    TaskMonitorEventos.Stop();
    lcd.clear();
}
/**
 * @brief Acción que se ejecuta al entrar en el estado ALARMA.
 * 
 * En este estado, el sistema activa el buzzer y el LED para alertar al usuario. Si la
 * alarma es activada por una temperatura alta, el mensaje en el LCD indicará "TEMP ALTA".
 * Si es por movimiento, el mensaje será "MOV DETECTADO".
 */
void alarm(){
    lcd.clear();
	if(temperature >TEMPERATURA_UMBRAL){
		lcd.print("TEMP ALTA");
	}else{
		lcd.print("MOV DETECTADO");
	}
	delay(1500);
	lcd.clear();
    lcd.print("ALARMA ACTIVADA");
    Serial.println("Entrando al estado ALARMA");
    ledInterval = 150; // Parpadeo cada 150 ms
}
/**
 * @brief Acción que se ejecuta al salir del estado ALARMA.
 * 
 * Se apagan el buzzer y el LED, y se limpia el LCD.
 */
void exitAlarm(){
    Serial.println("Saliendo del estado ALARMA");
    noTone(BUZZER_PIN);
    digitalWrite(LED_PIN, LOW);
    lcd.clear();
}
/**
 * @brief Lee el teclado y maneja las entradas según el estado actual.
 *
 * Esta función procesa las teclas ingresadas por el usuario para:
 * - Validar contraseñas ingresadas.
 * - Desactivar alarmas si se encuentra en estado de alarma.
 * - Reiniciar entradas según el estado actual del sistema.
 * 
 * @note Solo maneja la interacción en el estado INICIO o ALARMA.
 */
void readKeyboard(){
    char key = keypad.getKey();

    if (key) {
        if (stateMachine.GetState() == ALARMA && key == '#') {
            input = INPUT_T;
            Serial.println("Tecla # presionada. Desactivando alarma");
            return;
        }

        if (stateMachine.GetState() == INICIO) {
            if (key == '#') {
                if (inputPassword == correctPassword) {
                    input = INPUT_T;
                    failedAttempts = 0; // Reiniciar intentos fallidos
                    Serial.println("Clave correcta. Acceso concedido.");
                } else {
                    failedAttempts++;
                    inputPassword = "";
                    lcd.clear();
                    lcd.print("Clave incorrecta");
                    Serial.println("Clave incorrecta.");
                    delay(1000);
                    lcd.clear(); // Limpiar la segunda línea
                    lcd.print("Ingrese clave: ");

                    if (failedAttempts >= 3) {
                        input = INPUT_S; // Cambiar al estado BLOQUEADO
                        Serial.println("Demasiados intentos fallidos. Bloqueando sistema.");
                        return;
                    }
                }
            } else if (key == '*') {
                inputPassword = "";
                lcd.setCursor(0, 1);
                lcd.print("                "); // Limpiar la segunda línea
                Serial.println("Entrada reiniciada.");
            } else if (inputPassword.length() < 4) {
                inputPassword += key;
                lcd.setCursor(0, 1);
                lcd.print(inputPassword);
            }
        }
    }
}
// ====================== Configuración Inicial ======================
void setup()
{
  Serial.begin(9600);
  lcd.begin(16, 2);
  dht.begin();
  pinMode(PIN_PIR, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  noTone(BUZZER_PIN);

  stateMachine.AddTransition(INICIO, MONITOREO_AMBIENTAL, []() { return input == INPUT_T; });
  stateMachine.AddTransition(MONITOREO_AMBIENTAL, MONITOR_EVENTOS, []() { return triggerTransaction; });
  stateMachine.AddTransition(MONITOR_EVENTOS, MONITOREO_AMBIENTAL, []() { return triggerTransaction; });
  stateMachine.AddTransition(MONITOREO_AMBIENTAL, ALARMA, []() { return input == INPUT_P; });
  stateMachine.AddTransition(MONITOR_EVENTOS, ALARMA, []() { return input == INPUT_S; });
  stateMachine.AddTransition(ALARMA, INICIO, []() { return input == INPUT_T; });
  stateMachine.AddTransition(INICIO, BLOQUEADO, []() { return input == INPUT_S; });
  stateMachine.AddTransition(BLOQUEADO, INICIO, []() { return input == INPUT_T; });

  stateMachine.SetOnEntering(INICIO, home);
  stateMachine.SetOnLeaving(INICIO, exitHome);
  stateMachine.SetOnEntering(BLOQUEADO, blockade);
  stateMachine.SetOnLeaving(BLOQUEADO, exitBlockade);
  stateMachine.SetOnEntering(MONITOREO_AMBIENTAL, monitoring);
  stateMachine.SetOnLeaving(MONITOREO_AMBIENTAL, exitMonitoring);
  stateMachine.SetOnEntering(MONITOR_EVENTOS, events);
  stateMachine.SetOnLeaving(MONITOR_EVENTOS, exitEvents);
  stateMachine.SetOnEntering(ALARMA, alarm);
  stateMachine.SetOnLeaving(ALARMA, exitAlarm);

  stateMachine.SetState(INICIO, false, true);
}

// ====================== Bucle Principal ======================
void loop()
{
    readKeyboard();
    if (stateMachine.GetState() == ALARMA || stateMachine.GetState() == BLOQUEADO){
    keepLed(ledInterval);
    }
	if (stateMachine.GetState() == ALARMA){
    keepBuzzer();
    }
    TaskTemperatura.Update();
    TaskLuz.Update();
    TaskInfraRojo.Update();
    TaskMonitoreoAmbiental.Update();
    TaskMonitorEventos.Update();
    TaskBloqueoTiempo.Update();

    stateMachine.Update();
}
