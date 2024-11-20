

#define DEBUG(a) Serial.print(millis()); Serial.print(": "); Serial.println(a);

#define analogPin A0 //the thermistor attach to
#define beta 4090 //the beta of the thermistor

float tempC = 0.0;

// State Alias
enum State
{
	INICIO = 0,
	BLOQUEADO = 1,
	MONITOREO = 2,
	EVENTOS = 3,
  ALARMA = 4
};

// Input Alias
enum Input
{
	Sign_T = 0,
	Sign_P = 1,
	Sign_S = 2,
	Unknown = 3,
};

// Create new StateMachine
StateMachine stateMachine(5, 8);

// Stores last user input
Input input;


void read_Temperatura(void){

#if defined TEMP_DIGITAL
	Serial.println("TEMP_DIGITAL");
	// Read temperature as Celsius (the default)
  tempC = dht.readTemperature();
#else
	Serial.println("TEMP_ANALOG");
	//read thermistor value
  long a =1023 - analogRead(analogPin);
  //the calculating formula of temperature
  tempC = beta /(log((1025.0 * 10 / a - 10) / 10) + beta / 298.0) - 273.0;
#endif

	Serial.print("TEMP: ");
	Serial.println(tempC);
	if(tempC > 28){
    input = Input::Sign_P;
  }
}

void funct_Timeout(void){
  input = Input::Sign_T; 
}


AsyncTask TaskTemperatura(1000, true, read_Temperatura);
AsyncTask TaskTimeout(1000, false, funct_Timeout);


// Setup the State Machine
void setupStateMachine()
{
	// Add transitions
	stateMachine.AddTransition(INICIO, MONITOREO, []() { return input == Sign_P; });

	stateMachine.AddTransition(INICIO, BLOQUEADO, []() { return input == Sign_S; });
	stateMachine.AddTransition(BLOQUEADO, INICIO, []() { return input == Sign_T; });
	stateMachine.AddTransition(MONITOREO, EVENTOS, []() { return input == Sign_T; });

	stateMachine.AddTransition(EVENTOS, MONITOREO, []() { return input == Sign_T; });
	stateMachine.AddTransition(EVENTOS, ALARMA, []() { return input == Sign_S; });
	stateMachine.AddTransition(MONITOREO, ALARMA, []() { return input == Sign_P; });

	stateMachine.AddTransition(ALARMA, INICIO, []() { return input == Sign_T; });

	// Add actions
	stateMachine.SetOnEntering(INICIO, funct_Inicio);
	stateMachine.SetOnEntering(BLOQUEADO, funct_Bloqueado);
	stateMachine.SetOnEntering(MONITOREO, funct_Monitoreo);
	stateMachine.SetOnEntering(EVENTOS, funct_Eventos);
  stateMachine.SetOnEntering(ALARMA, funct_Alarma);

	stateMachine.SetOnLeaving(INICIO, funct_out_Inicio);
	stateMachine.SetOnLeaving(BLOQUEADO, funct_out_Bloqueado);
	stateMachine.SetOnLeaving(MONITOREO, funt_out_Monitoreo);
	stateMachine.SetOnLeaving(EVENTOS, funct_out_Eventos);
  stateMachine.SetOnLeaving(ALARMA, funct_out_Alarma);
}

void funt_out_Monitoreo(void){
	Serial.println("Leaving MONITOREO");
	TaskTemperatura.Stop();
}

void funct_Inicio(void){
	Serial.println("INICIO");
}

void funct_Bloqueado(void){
	Serial.println("BLOQUEADO");
}

void funct_Monitoreo(void){
	Serial.println("MONITOREO");
	TaskTemperatura.Start();
}

void funct_Eventos(void){
	Serial.println("EVENTOS");
	
}
void funct_Alarma(void){
	Serial.println("ALARMA");
}

void funct_out_Inicio(void){
	Serial.println("Leaving INICIO");
}

void funct_out_Bloqueado(void){
	Serial.println("Leaving BLOQUEADO");
}

void funct_out_Eventos(void){
	Serial.println("Leaving EVENTOS");
	
}
void funct_out_Alarma(void){
	Serial.println("Leaving ALARMA");
}


void setup() 
{
  
	Serial.begin(9600);

	Serial.println("Starting State Machine...");
	setupStateMachine();	
	Serial.println("Start Machine Started");

	dht.begin();
	// Initial state
	stateMachine.SetState(INICIO, false, true);
}

void loop() 
{
	
	// Read user input
	input = static_cast<Input>(readInput());
	TaskTemperatura.Update();
	// Update State Machine
	stateMachine.Update();
  input = Input::Unknown;
}

// Auxiliar function that reads the user input
int readInput()
{
	Input currentInput = Input::Unknown;
	if (Serial.available())
	{
		char incomingChar = Serial.read();

		switch (incomingChar)
		{
			case 'P': currentInput = Input::Sign_P; break;
			case 'T': currentInput = Input::Sign_T; break;
			case 'S': currentInput = Input::Sign_S; break;
			default: break;
		}
	}

	return currentInput;
}
