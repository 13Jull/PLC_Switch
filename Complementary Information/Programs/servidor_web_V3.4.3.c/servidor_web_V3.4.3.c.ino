/*
  Servidor Web y transreceptor central
  
//REVISAR DISPARO POR FLANCOS DEL ZERO CROSS//

Realizado segun lo detallado en el informe de avance 2 presentado

Proyecto:T.P. Final
Version:1
Fecha:22/1/20
Edito:Julio Esteban

Notas:
-El primer frame se corresponde con el 3 valor del array (Tanto para parte
alta como baja)
-Se envian tres frames y debe existir la señal brindada por el zero_cross
detector para que el modulo no se resete por watchdog

V3.4.1 Envia tres frames de datos que son comprendidos por el attiny13 
(YA PROBADO CON EL PRIMER FRAME)

V3.4.1 Se programa la lectura 
-Se deja en el punto que entra en el modo lectura luego de transmitir
-Debe esperarse la habilitacion de transmision por el attiny13

V3.4.2 Se retoma despues de las vacaciones y se analiza la lectura de datos 
-Se logra leer el mensaje pero no se interpreta (Se deja solo para el ACK)
pero se genera un flag para saber si se requiere leer mas de 1 un bite.
AUN ASI ESTO ULTIMO NO ESTA PROBADO A PESAR DE QUE DEBERIA FUNCIONAR
CORRECTAMENTE

V3.4.3 Se preve dejar el ACK funcionando

V3.4.3.b Se coloca el ESP8266 en la placa
-Se procede a cambiar los puertos segun corresponda
  >Se encuentra que los puertos 10 y 9 no pueden ser utilizados con facilidad
   y se busca su remplazo
-Se utiliza la libreria WifiManager para que el dispositivo arranque como 
Acces point y pase a conectarse a la red

V3.4.3.c  Se borran los serial para usar tx y rx como GPIO
 */
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>       
#include <Ticker.h>
#include "Comunicacion.h"
#include "Acciones.h"
Ticker blinker;

#define Zero_cross 12 //Zero Cross detector (Señal sincronizacion)
#define Rx 3 //Entrada de datos
#define Tx 1 //On board LED
#define OUT 13 //Lampara
#define AUX 14 //Aux PIN D7 //Tx
#define Tiempo_Bit 5715 //El correcto es 5715 que equivale a 1413us 





//=======================================================================
//                   Definicion variables globales
//=======================================================================

byte prueba=0xFF;
byte prueba_b=0xFF;
long int Pulse_Time;
bool Flag_time_interrupt;
bool Flag_end_frame=0;
bool Flag_end_Rframe=0;
bool Flag_Read=0;
bool Flag_ACK=0; //0=Se usa ACK; 1=Se espera mensaje de 3 bits;
int Condition=1;
bool IN[100];
bool Read_Write=1;
//SSID and Password to your ESP Access Point
const char* ssid = "Tesis Julio";
const char* password = "Dalequeaprobamos";
//Input Data del web server
const char* INPUT_1 = "input1";
const char* INPUT_2 = "input2";
const char* INPUT_3 = "input3";
//Especificacion del mensaje a enviar/recibido
static byte high_message[3];
static byte low_message[3];
//READ VARIABLES
byte READ;
//volatile byte aux;
byte READ_1;


ESP8266WebServer server(80); //Server on port 80


// Variable to store the HTTP request
String header;

// Auxiliar variables to store the current output state
String output5State = "off";
String output4State = "off";

// Assign output variables to GPIO pins
const int output5 = 2;
const int output4 = 2;


//=======================================================================
//                   HTML DE PAGINA PRINCIPAL
//=======================================================================
const char MAIN_page[] PROGMEM = R"=====(
       <!DOCTYPE html><html>
            <head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">
            <link rel=\"icon\" href=\"data:,\">            
            <style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}
            .button { background-color: #195B6A; border: none; color: black; padding: 16px 40px;
            text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}.button2 {background-color: #77878A;}
            </style></head>
            
            
            <body><h1>ESP8266 Web Server</h1>
            
            
            <p>GPIO 5 - State " + output5State + "</p>
            
            
              <p><a href="ledOn"><button class="button">ON</button></a></p>
            
              <p><a href="ledOff"><button class="button button2">OFF</button></a></p>                                                
                                  
            
        </body></html>
     
)=====";


//===============================================================
// This routine is executed when you open its IP in browser
//===============================================================
void returnFail(String msg)
{
  server.sendHeader("Connection", "close");
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(500, "text/plain", msg + "\r\n");
}


void handleRoot() {
 
 String s = MAIN_page; //Read HTML contents
 server.send(200, "text/html", s); //Send web page
}
 
void handleLEDon() { 
 
 Enviar_Frame(1,0,0);
 high_message[0]=0xAA;
 high_message[1]=0xAA;
 high_message[2]=0xAA;
 low_message[0]=0xAA;
 low_message[1]=0xAA;
 low_message[2]=0xAA;
 //digitalWrite(LED,LOW); //LED is connected in reverse
 server.send(200, "text/html", MAIN_page); //Send ADC value only to client ajax request
}
 
void handleLEDoff() { 
 
 Enviar_Frame(1,0,0);
  high_message[0]=0xAB;
 high_message[1]=0xAB;
 high_message[2]=0xAB;
 low_message[0]=0xAB;
 low_message[1]=0xAB;
 low_message[2]=0xAB;
 //Flag_Read=1;
 //Leer_Frame(1);
 //digitalWrite(LED,HIGH); //LED off
 server.send(200, "text/html", MAIN_page); //Send ADC value only to client ajax request
}

void handleSave() {
  String str = "Settings Saved ...\r\n";
  
  if (server.args() > 0 ) {
    for ( uint8_t i = 0; i < server.args(); i++ ) {
      str += server.argName(i) + " = " + server.arg(i) + "\r\n";
         
    }
  }
  server.send(200, "text/html", MAIN_page);
}


//===============================================================
//                  SETUP
//===============================================================
void setup(void){
/////// CHANGE PIN FUNCTION  TO GPIO ///////
//GPIO 1 (TX) swap the pin to a GPIO.
pinMode(1,FUNCTION_3); 
//GPIO 3 (RX) swap the pin to a GPIO.
pinMode(3,FUNCTION_3); 
////////////////////////////////////////////
  
  
  
  //Initialize Ticker every 0.5s
    timer1_attachInterrupt(onTimerISR);    
        
 
  
  // Initialize the output variables as outputs
  pinMode(Zero_cross,INPUT);
  pinMode(Rx,INPUT);
  pinMode(Tx, OUTPUT);  
  pinMode(OUT, OUTPUT); 
  pinMode(AUX, OUTPUT);  
  pinMode(output4, OUTPUT);

  
  // Set outputs to LOW
  digitalWrite(Tx, LOW);
  digitalWrite(OUT, LOW);
  digitalWrite(AUX, LOW);  
  digitalWrite(output4, LOW);

digitalWrite(LED_BUILTIN,0);  
  
/*
//WiFiManager
    //Local intialization. Once its business is done, there is no need to keep it around
    WiFiManager wifiManager;
    //reset saved settings
    //wifiManager.resetSettings();
    
    //set custom ip for portal
    wifiManager.setAPStaticIPConfig(IPAddress(192,168,4,1), IPAddress(192,168,4,1), IPAddress(255,255,255,0));
  
    //fetches ssid and pass from eeprom and tries to connect
    //if it does not connect it starts an access point with the specified name
    //here  "AutoConnectAP"
    //and goes into a blocking loop awaiting configuration
    wifiManager.autoConnect("AutoConnectAP","holaJulio");
    //or use this for auto generated name ESP + ChipID
    //wifiManager.autoConnect();

    
    //if you get here you have connected to the WiFi
   
*/
  
  WiFi.mode(WIFI_AP);           //Only Access point
  //WiFi.softAP(ssid, password);  //Start HOTspot removing password will disable security
   while(!WiFi.softAP(ssid, password))
  {
   
    delay(100);
  }
  IPAddress myIP = WiFi.softAPIP(); //Get IP address
    

 
  server.on("/", handleRoot);      //Which routine to handle at root location. This is display page
  server.on("/ledOn", handleLEDon); //as Per  <a href="ledOn">, Subroutine to be called
  server.on("/ledOff", handleLEDoff);
  server.on("/save", handleSave);
  
  server.begin();                  //Start server  
digitalWrite(LED_BUILTIN,1);
}



//=======================================================================
//                        Detect Transmision start
//
//Detecta si existe un uno en el canal durante 4ms y si es asi devuelve 
//un 1.
//=======================================================================

int Inicio() {
  int Counter=0;
  int Counter_1=0;
  short Flag=0;

  while(Flag==0){
    bool Input_Value = digitalRead(Rx);
    
    delay(2);    //Se debe tener cuidado ya que esto ya no representa 
                 //un mS debido al cambio de la base del reloj.
    Counter_1++; //Si pasan unos mS y no hay respuesta se 
                 //deja de escuchar
                              
      if (Input_Value==0){
      Counter++;      
        if (Counter==4){         
        Flag=1;
        Counter=0;      
        }
      }else{
      Counter=0;  
      }  
    if (Counter_1>=8){
         Flag=2;
         Counter_1=0;      
    }   
  }
return Flag;
}


//===============================================================
//                     LOOP
//===============================================================
void loop(void){
  static int Counter_1=3;
  static int Counter_2=2;
  static int Counter_3=2;  
  static short Flag_Inicio=0; //1 empieza una transmision; 2 no hay nada;
  static int aux=0;

/*
aux=digitalRead(Zero_cross);
digitalWrite(AUX,aux);
digitalWrite(LED_BUILTIN,aux);
*/
 
server.handleClient();          //Handle client requests          

  
 
////SEND 2 bytes restantes/////////////
    if(Flag_end_frame==1){      
      Flag_end_frame=0;
      digitalWrite(Tx,0);
     if (Counter_1>0){
      Counter_1--;      
     Enviar_Frame(2,high_message[Counter_1],low_message[Counter_1]); 
     }else{
      Flag_end_frame=0;
      Counter_1=3;
      
      //if (Flag_Read==1){ //Se activa si no se quiere usar el ack
        Flag_Inicio=Inicio();
        if (Flag_Inicio==1){  
        digitalWrite(LED_BUILTIN,0);        
        Leer_Frame(1);    
        digitalWrite(LED_BUILTIN,1);     
        }else{
        }
        //Flag_Read=0;
      //}
     
     }
    }



    
//////////////////////////////////////////////
//////////////READ 2 bytes restantes///////////////
if(Flag_ACK==1){
    if(Flag_end_Rframe==1){
      Flag_end_Rframe=0;
     if (Counter_3>0){
      Counter_3--;
     Leer_Frame(2); 
     }else{
      Flag_end_Rframe=0;
      Counter_3=2;
     }
    }
///////////////////////////////////////////////////
///Evalua el dato recibido si termino la lectura///
}else{ 
     if(Flag_end_Rframe==1){

        READ=READ&127;
        READ_1=READ_1&127;
        if((READ==42) && (READ_1==42)){     
        digitalWrite(LED_BUILTIN,0);  
        }
        
        Flag_end_Rframe=0;
        
     }

  
}
///////////////////////////////////////////////////
/*if(Counter_2>1){  

main_decition(0xAC,0xAB,0&00000000);
Counter_2--;
}
if(Counter_2>0){
  Serial.println("Por aca paso 1");
main_decition(0xAC,90,128);
Counter_2--;
}
*/
}
