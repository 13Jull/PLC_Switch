#ifndef Comunicacion.h

#define Zero_cross 12 //Zero Cross detector (Señal sincronizacion)
#define Rx 3 //Entrada de datos
#define Tx 1 //On board Tx
#define OUT 13 //Lampara
#define AUX 14 //Aux PIN D7 //Tx
#define Tiempo_Bit 5715 //El correcto es 5715 que equivale a 1413us 

extern byte READ;
byte aux;
extern byte READ_1;
extern byte prueba;
extern byte prueba_b;
extern long int Pulse_Time;
extern bool Flag_time_interrupt;
extern bool Flag_end_frame;
extern bool Flag_end_Rframe;
extern int Condition;
extern bool IN[100];
extern bool Read_Write;
int TxState = LOW; 


//=======================================================================
//                        Interrupcion timer_1
//=======================================================================

void ICACHE_RAM_ATTR onTimerISR(){    
Flag_time_interrupt=1;        
short val;
static int i=0;

if(Read_Write==0){
    /////////////////Lectura de datos [READ]/////////////////////  
    //Delay 6ms
    if(Condition==1){
      timer1_write(29000);   //Seteo el tiempo de espera antes del frame
                             //y le sumo un desfasaje para la lectura de datos.

      if(digitalRead(Rx)==1){                
        READ=READ<<1;
        READ=READ+1;    
      }else{
        READ=READ<<1;
      }
//digitalWrite(AUX,TxState);
//TxState= !TxState;
                 
      Condition=2;         
        
    //Frame Bit
    }else if(Condition==2){
      timer1_write(Tiempo_Bit);       //Seteo el tiempo de bit      
      if(digitalRead(Rx)==1){        
        READ=READ<<1;       
        READ=READ+1;        
      }else{
        READ=READ<<1;
      } 
//digitalWrite(AUX,TxState);
//TxState= !TxState;

      Condition=3;         
    
    //Cicle bit
    }else if(Condition==3){         
      timer1_write(Tiempo_Bit);        
      if(digitalRead(Rx)==1){
        READ=READ<<1;
        READ=READ+1;
      }else{
        READ=READ<<1;
      }
//digitalWrite(AUX,TxState);
//TxState= !TxState;

      Condition=4;

    //Bit [0:3]+paridad
    }else if(Condition==4){                
      if(i==4){ //DEAD TIME//        
        timer1_write(10000);      //DEAD TIME 2ms (10000)
      if(digitalRead(Rx)==1){
        READ=READ<<1;
        READ=READ+1;
      }else{
        READ=READ<<1;
      }
//digitalWrite(AUX,TxState);
//TxState= !TxState;

        Condition=5;
        i=0;              
      }else{   //BITS//     
        timer1_write(Tiempo_Bit);       //Primer envio datos 7 bits en 1.143ms cada uno
                                        //600000 units = 120000 us  
       if(digitalRead(Rx)==1){
        READ=READ<<1;
        READ=READ+1;
      }else{
        READ=READ<<1;
      }
//digitalWrite(AUX,TxState);
//TxState= !TxState;

        i++;        
      }
      
      //cicle bit
      }else if(Condition==5){         
      timer1_write(Tiempo_Bit);        
      if(digitalRead(Rx)==1){
        READ_1=READ_1<<1;
        READ_1=READ_1+1;
      }else{
        READ_1=READ_1<<1;
      }
//digitalWrite(AUX,TxState);
//TxState= !TxState;

      Condition=6;

    //bits[0:4]+paridad
    }else if(Condition==6){
      if(i==5){      
    if(digitalRead(Rx)==1){
        READ_1=READ_1<<1;
        READ_1=READ_1+1;
      }else{
        READ_1=READ_1<<1;
      }            
//digitalWrite(AUX,TxState);
//TxState= !TxState;

        timer1_disable();      //END FRAME
        Condition=1;
        i=0;
        Flag_end_Rframe=1;
      }else{      
        timer1_write(Tiempo_Bit);  //SEGUNDO CICLO DEL FRAME
      if(digitalRead(Rx)==1){
        READ_1=READ_1<<1;
        READ_1=READ_1+1;
      }else{
        READ_1=READ_1<<1;
      }
//digitalWrite(AUX,TxState);
//TxState= !TxState;

        i++;
      }
    }
    /////////////////////////////////////////////////////////////    
    
}else{
   
    /////////////////Frame de datos [SEND]/////////////////////      
    if (Condition==0){ //10ms delay
      Flag_end_frame=1;
      timer1_disable();
      
    }else if(Condition==1){ 
    //Delay 6ms    
      timer1_write(30000);   //Seteo el tiempo de espera antes del frame
      val=0;      
      digitalWrite(Tx,val);  //Toggle Tx Pin      
      Condition=2;         
        
    //Frame Bit
    }else if(Condition==2){
      timer1_write(Tiempo_Bit);       //Seteo el tiempo de bit      
      val=1;      
      digitalWrite(Tx,val);  //Toggle Tx Pin      
      Condition=3;         
    
    //Cicle bit
    }else if(Condition==3){         
      timer1_write(Tiempo_Bit);        
      val=1;//0         
      digitalWrite(Tx,val);  //Toggle Tx Pin      
      Condition=4;

    //Bit [0:3]+paridad
    }else if(Condition==4){                
      if(i==5){ //DEAD TIME//        
        timer1_write(10000);      //DEAD TIME 2ms (10000)
        val=1;//0
        digitalWrite(Tx,val);  //Toggle Tx Pin        
        Condition=5;
        i=0;              
      }else{   //BITS//     
        timer1_write(Tiempo_Bit);       //Primer envio datos 7 bits en 1.143ms cada uno
                                        //600000 units = 120000 us  
        val=prueba&0x01;     
        digitalWrite(Tx,val);  //Toggle Tx Pin
        prueba=prueba>>1;
        i++;        
      }
      
      //cicle bit
      }else if(Condition==5){         
      timer1_write(Tiempo_Bit);        
      val=1;         
      digitalWrite(Tx,val);  //Toggle Tx Pin      
      Condition=6;

    //bits[0:4]+paridad
    }else if(Condition==6){
      if(i==5){
        val=1;
        digitalWrite(Tx,val);  //Toggle Tx Pin        
        timer1_disable();      //END FRAME        
        Condition=1;
        i=0;
        Flag_end_frame=1;
      }else{      
        timer1_write(Tiempo_Bit);  //SEGUNDO CICLO DEL FRAME
        val=prueba_b&0x01;   
        digitalWrite(Tx,val);
        prueba_b=prueba_b>>1;            
        i++;
      }
    }
    
    /////////////////////////////////////////////////////////////

}

}



//=======================================================================
//                        Enviar frame de datos
//=======================================================================


void Enviar_Frame(int state,byte high_message,byte low_message){ //7 bits each
  short i=2;
  if (state==1){                
             //Señal de inicio de transmision
              Read_Write=1;
              Condition=0;
              //prueba=0x1F;  //prueba=0x15;    //Bits[0:3]+Paridad  [Se envia primero el ultimo bit]
             // prueba_b=0x1A; //;prueba_b=0x15;  //Bits[4:7]+Paridad   //Uso del byte=1A (Mas alla del 1 no se envia)
              Pulse_Time=35000;//50000;          
              timer1_write(Pulse_Time); //50000 units = 100000 us = 10ms 
              while(digitalRead(Zero_cross)==0);
              digitalWrite(Tx,0);
              timer1_enable(TIM_DIV16, TIM_EDGE, TIM_SINGLE);              
    
  }else if (state==2){    
  
    prueba=high_message;    //Bits[0:3]+Paridad  [Se envia primero el ultimo bit]
    prueba_b=low_message;
  
    Condition=1;

    while(digitalRead(Zero_cross)==1);  
    while(digitalRead(Zero_cross)==0);  
    timer1_write(1); //50000 units = 100000 us = 10ms        
    timer1_enable(TIM_DIV16, TIM_EDGE, TIM_SINGLE);                                                                            
  }else{
    digitalWrite(Tx,1);
    timer1_disable();    
    Condition=1;
  }  
return;
}

void Leer_Frame(int state){
  if (state==1){
              digitalWrite(Tx,0);
              Read_Write=0;
              Condition=1;
              //timer1_write(1); //50000 units = 100000 us = 10ms                      
              timer1_write(1);   //Seteo el tiempo de espera antes del frame
                                     //y le sumo un desfasaje para la lectura de d
              while(digitalRead(Zero_cross)==0);
              timer1_enable(TIM_DIV16, TIM_EDGE, TIM_SINGLE);              
}else if (state==2){    
    while(digitalRead(Zero_cross)==0);  
    timer1_write(1); //50000 units = 100000 us = 10ms        
    timer1_enable(TIM_DIV16, TIM_EDGE, TIM_SINGLE);

  }else{
    digitalWrite(Tx,0);
    timer1_disable();    
    Condition=1;
  }  
return;
}

#endif
