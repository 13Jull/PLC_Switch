#ifndef Acciones.h

byte Home_code=0xAC;
byte Unit_code=0xAB;

void main_decition(byte Home_request,byte Command,byte Subfix){
//Variables
static bool Listen=0;  
//Codigo:

//Me hablan a mi?
if (Home_request==Home_code){

    Subfix=Subfix&192;

    if (Subfix==128){

        if (Listen=1){
;
          switch (Command){
            case 89: //Unit On
            //Salida On

            Listen=0;
            break;
            case 90: //Unit Off
            //Salida Off

            Listen=0;
            break;
            case 170: //Status Request
            //Status response

            Listen=0;
            break; 
          }       
        }
    }else{      
           if(Command==Unit_code){

            Listen=1;
           }else{
            Listen=0;
           }
     }
   }
}





  

bool getParity(unsigned int n) //Para mas informacion ver: https://www.geeksforgeeks.org/program-to-find-parity/
{ 
    bool parity = 0; 
    while (n) 
    { 
        parity = !parity; 
        n     = n & (n - 1); 
    }      
    return parity; 
} 



#endif
