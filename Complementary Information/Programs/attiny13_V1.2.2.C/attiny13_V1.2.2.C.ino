/*
  Emision de mensajes del tranreceptor

Realizado segun lo detallado en el informe de avance 2 presentado

Proyecto:T.P. Final
Version:1
Fecha:22/1/20
Edito:Julio Esteban

Notas:  
SE DEBE ESRIBIR FUNCION PARA DETECTAR FLANCOS
Se define funcion para flanco de subida de zero_cross
Se suma tiempo al retraso de 6ms para leer cuando el bit este listo
Se debe tener en cuenta que el dato enviado fuerza un 1 al inicio y un cero al final
Se debe descartar un valor leido (El que cae en el tiempo muerto (Bit 8)
Revisar el segundo frame ya que el primero no se ve el primer momento de lectura
Se trabaja despues de un tiempo y se preve dejar la comunicacion lista
Tx=PB_0
Rx=PB_3
Zero_Cross=PB_4

V.1.2 SE LEE LA PARTE ALTA Y BAJA DEL FRAME SIN PROBLEMA (SE DEBE IGNORAR EL BIT
MAS SIGNIFICATIVO EN AMBAS PARTES)

V.1.2.2 Se programa el envio de datos
-Se logra realizar correctamente el ACK
La version a se genera para tener un backup
La version b reordena los pines para ser pasado a la placa
V.1.2.2.c
Funcionando
*/




#include <Arduino.h>
#define Rx 1
#define Zero_Cross 3                    
#define Tx 2 
#define Led 0
#define AUX 4
#define Button
#define Bit_time 115 //115 //1.1467ms



bool i=0;
int ledState = LOW; 
bool Input[100];
//Variales de la interrupcion del timer
volatile bool Flag_Time_Int=0;
volatile bool Flag_End_Read=0;
volatile bool Flag_End_Send=0;
int Multiplier=0;
//Variables de lectura de datos
byte READ;
byte aux;
byte READ_1;
//Variables de envio de datos
byte SEND;
byte SEND_1;
byte aux_s;

//=======================================================================
//                                Setup
//=======================================================================

void setup() {

//Inicializacion I/O
pinMode(Led, OUTPUT);
pinMode(Tx, OUTPUT);
pinMode(AUX, OUTPUT);
pinMode(Zero_Cross,INPUT);
pinMode(Rx, INPUT);
//pinMode(Button, INPUT);
//digitalWrite(Tx,0);
//delay(1);
digitalWrite(Tx,1);

//Calibracion TIM0
TCCR0A = 0;
TCCR0B = 0;
TCNT0=0;
OCR0A=85;           //1.1467ms
TIMSK=_BV(OCIE0A); 
TCCR0A=_BV(WGM01);  //CTC Mode
//TCCR0B=_BV(CS01) | _BV(CS00);//Prescaler x64
interrupts();
 
}


//=======================================================================
//                         Timer 0 interrupt
//=======================================================================
ISR(TIM0_COMPA_vect){
  Flag_Time_Int=1;
}



//=======================================================================
//                         Flank detection 
//
//Detecta flanco positivo.
//=======================================================================
void flanco(){
if(digitalRead(Zero_Cross)==1){
  while(digitalRead(Zero_Cross)==1);
  while(digitalRead(Zero_Cross)==0);
}else{
  while(digitalRead(Zero_Cross)==0);
}  
}



//=======================================================================
//                        Detect Transmision start
//
//Detecta si existe un uno en el canal durante 4ms y si es asi devuelve 
//un 1.
//=======================================================================

bool Inicio() {
  int Counter=0;
  bool Flag=0;
TCCR0B=0;
TCNT0=0;
OCR0A=115;//115; //163           //1.1467ms

  while(Flag==0){
    bool Input_Value = digitalRead(Rx);
    
    TCCR0B=_BV(CS01);//Prescaler x8
    while(Flag_Time_Int==0);   
    TCCR0B=0;
    Flag_Time_Int=0;
         
      if (Input_Value==1){
      Counter++;      
        if (Counter==6){         
        Flag=1;
        Counter=0;      
        }
      }else{
      Counter=0;  
      }  
  }
}




//=======================================================================
//                              Send Data
//=======================================================================

void Send_Data(){ //ANULADA POR AHORA
int Counter_1=0;      

//Seteo de aviso de transmision
TCCR0B=0;
TCNT0=0;
OCR0A=185;//111         //10ms
//Multiplier=1;
TCCR0B=_BV(CS01) | _BV(CS00);//Prescaler x64
digitalWrite(Tx,1); //Pongo el canal en 1 para avisar de una transmision
    while(Flag_Time_Int==0);
    Flag_Time_Int=0;   
digitalWrite(Tx,0);
flanco();

//Seteo el tiempo de espera desde el cruce por cero
TCCR0B=0;
TCNT0=0;
OCR0A=90;//111         //6ms
//Multiplier=1;

TCCR0B=_BV(CS01) | _BV(CS00);//Prescaler x64
    while(Flag_Time_Int==0);
    Flag_Time_Int=0;         
        aux_s=SEND&0x01;     
        digitalWrite(Tx,aux_s);
        SEND=SEND>>1; 
        
//Primera tanda de bits    
while(Counter_1<6){    
TCCR0B=0;
TCNT0=0;
OCR0A=Bit_time;//163           //1.1467ms
//Multiplier=1;
TCCR0B=_BV(CS01);//Prescaler x8
    while(Flag_Time_Int==0);   
    Flag_Time_Int=0;      
        aux_s=SEND&0x01;     
        digitalWrite(Tx,aux_s);
        SEND=SEND>>1;       
      Counter_1++;
}
      Counter_1=0;

//Seteo el Dead time
TCCR0B=0;
TCNT0=0;
OCR0A=35;//37;           //2ms
//Multiplier=1;
TCCR0B=_BV(CS01) | _BV(CS00);//Prescaler x64
    while(Flag_Time_Int==0);
    Flag_Time_Int=0;    
      
        aux_s=SEND_1&0x01;     
        digitalWrite(Tx,aux_s);
        SEND_1=SEND_1>>1;       

//Segunda tanda de bits
while(Counter_1<6){    
TCCR0B=0;
TCNT0=0;
OCR0A=Bit_time;//163           //1.1467ms
//Multiplier=1;
TCCR0B=_BV(CS01);//Prescaler x8
    while(Flag_Time_Int==0);   
    Flag_Time_Int=0;
        aux_s=SEND_1&0x01;     
        digitalWrite(Tx,aux_s);
        SEND_1=SEND_1>>1;       
        Counter_1++;
}
      Counter_1=0;            
TCCR0B=0;   
Flag_End_Send=1;

digitalWrite(Tx,1);
}




//=======================================================================
//                              Read Data
//=======================================================================

void Read_Data(){
int Counter_1=0;      
//Seteo el tiempo de espera desde el cruce por cero
TCCR0B=0;
TCNT0=0;
OCR0A=80;//72         //6ms
//Multiplier=1;

TCCR0B=_BV(CS01) | _BV(CS00);//Prescaler x64
    while(Flag_Time_Int==0);
    Flag_Time_Int=0;   
      if(digitalRead(Rx)==1){                
        READ=READ<<1;
        READ=READ+1;            
      }else{
        READ=READ<<1;
      }

digitalWrite(Led,ledState);
ledState= !ledState;

//Primera tanda de bits    
while(Counter_1<7){    
TCCR0B=0;
TCNT0=0;
OCR0A=Bit_time;//163           //1.1467ms
//Multiplier=1;
TCCR0B=_BV(CS01);//Prescaler x8
    while(Flag_Time_Int==0);   
    Flag_Time_Int=0;
      
      if(digitalRead(Rx)==1){                
        READ=READ<<1;
        READ=READ+1;    
      }else{
        READ=READ<<1;
      }

digitalWrite(Led,ledState);
ledState= !ledState;
      
      Counter_1++;
}
      Counter_1=0;

//Seteo el Dead time
TCCR0B=0;
TCNT0=0;
OCR0A=28;//28;           //2ms
//Multiplier=1;
TCCR0B=_BV(CS01) | _BV(CS00);//Prescaler x64
ledState = HIGH;
    while(Flag_Time_Int==0);
    Flag_Time_Int=0;    
      
      if(digitalRead(Rx)==1){                
        READ_1=READ_1<<1;
        READ_1=READ_1+1;    
      }else{
        READ_1=READ_1<<1;
      }

digitalWrite(Led,ledState);
ledState=!ledState;

//Segunda tanda de bits
while(Counter_1<6){    
TCCR0B=0;
TCNT0=0;
OCR0A=Bit_time;//163           //1.1467ms
//Multiplier=1;
TCCR0B=_BV(CS01);//Prescaler x8
    while(Flag_Time_Int==0);   
    Flag_Time_Int=0;
      if(digitalRead(Rx)==1){                
        READ_1=READ_1<<1;
        READ_1=READ_1+1;    
      }else{
        READ_1=READ_1<<1;
      }      Counter_1++;

digitalWrite(Led,ledState);
ledState=!ledState;

}
      Counter_1=0;      
      
TCCR0B=0;   
Flag_End_Read=1;
}


//=======================================================================
//                        Test Time
//=======================================================================
/*
void T_Time() {
while(Flag_Time_Int==0);   
    Flag_Time_Int=0;
      if (ledState == LOW) {
            ledState = HIGH;
          } else {
            ledState = LOW;
          }
      digitalWrite(Led,ledState);

}
*/
//=======================================================================
//                        Test INPUT
//=======================================================================
/*
void T_Input() {    
      if (digitalRead(Zero_Cross) == LOW) {
            ledState = LOW;
          } else {
            ledState = HIGH;
          }
      digitalWrite(Led,ledState);

}
*/
//=======================================================================
//                                 Main
//=======================================================================

void loop() {
//static bool Flag=0;
static int Counter=3;
static byte aux1;

/////// PRUEBA DE ENTRADA ////
//T_Input();
//////////////////////////////

/////// PRUEBA DE FLANCO ////
/*
digitalWrite(AUX,0);  
flanco();
digitalWrite(AUX,1);  
flanco();
*/
//////////////////////////////

/////// PRUEBA DE TIMER /////
/*
TCCR0B=0;
TCNT0=0;
OCR0A=37;           //2ms
//Multiplier=1;
TCCR0B=_BV(CS01) | _BV(CS00);//Prescaler x64
while(1){
  T_Time();
}
*/
////////////////////////////


//if (aux==106){
//  digitalWrite(Led,1);
//}




  
  Inicio();
  
  ledState=LOW;  
  //digitalWrite(AUX,0);  
  flanco();
  Read_Data();
  aux1=READ_1;
  if (Flag_End_Read==1){
      flanco();
      Read_Data(); //Byte 2
      flanco();
      Read_Data(); //Byte 3
      Flag_End_Read=0;   
    //  digitalWrite(AUX,0);     
  }

  
  aux1=aux1&127;
    if (aux1==122){
      digitalWrite(Led,1);   
      digitalWrite(AUX,0);   
      
      //digitalWrite(Tx,0);
      //SEND=0xAA;
      //SEND_1=0xAA;
      //Send_Data();
      //Flag_End_Send=0;   
     // digitalWrite(Led,0);
    }else if (aux1==106){
      //digitalWrite(Tx,0);
      digitalWrite(Led,0);
      digitalWrite(AUX,1);   
    }

}
 
