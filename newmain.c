#include <xc.h>
#include <pic16f877a.h>
#include <stdio.h>
#include <stdint.h>

// CONFIG
#pragma config FOSC = HS        // Oscillator Selection bits (HS oscillator)
#pragma config WDTE = ON       // Watchdog Timer Enable bit (WDT disabled)
#pragma config PWRTE = ON       // Power-up Timer Enable bit (PWRT enabled)
#pragma config BOREN = ON      // Brown-out Reset Enable bit (BOR disabled)
#pragma config LVP = OFF        // Low-Voltage (Single-Supply) In-Circuit Serial Programming Enable bit (RB3 is digital I/O, HV on MCLR must be used for programming)
#pragma config CPD = OFF        // Data EEPROM Memory Code Protection bit (Data EEPROM code protection off)
#pragma config WRT = OFF        // Flash Program Memory Write Enable bits (Write protection off; all program memory may be written to by EECON control)
#pragma config CP = OFF         // Flash Program Memory Code Protection bit (Code protection off)
#define _XTAL_FREQ 4000000

//Pinos LCD
#define RS RD2
#define EN RD3
#define D4 RD4
#define D5 RD5
#define D6 RD6
#define D7 RD7
#include "lcd.h"
char buffer[20];

//Pinos de Entradas
__bit LigDesl = 0;
__bit AftDias = 0;

//Pinos de Saídas
#define RH1 RC3
#define RH2 RC4
#define RH3 RC5

double temperatura = 0, umidade = 0; 
int conta = 0;
double TempMax, TempMin;

void Temperatura() //Calcula e mostra temperatura
{   
    ADCON0bits.CHS0 = 0; //AN0
    ADCON0bits.CHS1 = 0; 
    ADCON0bits.CHS2 = 0;
    
    ADCON0bits.GO = 1; // Inicia a conversão AD
    __delay_us(10);
    uint16_t adcValue = ADRESH;
    temperatura = (adcValue * 0.48828125)*4; //Transformar o LM35 em decimal
    sprintf(buffer, "Temp: %.0f C", temperatura);
    Lcd_Set_Cursor(1,1);
    Lcd_Write_String(buffer);
}
void Umidade() //Calcula e mostra umidade
{   
    ADCON0bits.CHS0 = 1; //AN1
    ADCON0bits.CHS1 = 0; 
    ADCON0bits.CHS2 = 0;
    
    ADCON0bits.GO = 1; // Inicia a conversão AD
    __delay_us(10);
    uint16_t adcValue = ADRESH;
    umidade = (adcValue * 0.48828125)*4; //Transformar o LM35 em decimal
    sprintf(buffer, "Umid: %.0f", umidade);
    Lcd_Set_Cursor(2,1);
    Lcd_Write_String(buffer);
}
void __interrupt() Interrupcao(void)
{
    if (TMR1IF)  //Passou 0,5 segundos
    { 
        PIR1bits.TMR1IF = 0;  //reseta o flag da interrup??o
        TMR1L = 0xDC;        //reinicia contagem com 3036
        TMR1H = 0x0B;  
        
        conta++;
        if(conta == 10)
        {  if(AftDias == 1){
           PORTCbits.RC6 = 1; //gira o ovo 45 graus
           __delay_ms(1000);  //fica girando por um tempo 
           PORTCbits.RC6 = 0; //desliga
           conta = 0;
        }
        }
    }
    PIR1bits.ADIF = 0;
    INTCONbits.INTF = 0;
    return;
}

void main(void) 
{
    OPTION_REG=0b00111111;
     
    //Configurar entrada/saida
    TRISA = 0b10000011; //1 = entrada / 0 = saida
    TRISB = 0b11111111;
    TRISC = 0b00000000;
    TRISD = 0b00000000;
    
    PORTC = 0;
    
    //Interrupções:
    INTCONbits.GIE=1;
    INTCONbits.INTE=1;
    INTCONbits.PEIE = 1; //Periféricos
    PIE1bits.TMR1IE = 1; //Timer 1
    
    //Configuração Timer 1
    T1CONbits.TMR1CS = 0;   //Define timer 1 como temporizador (Fosc/4)
    T1CONbits.T1CKPS0 = 1;  //bit pra configurar pre-escaler, nesta caso 1:8
    T1CONbits.T1CKPS1 = 1;  //bit pra configurar pre-escaler, nesta caso 1:8
    TMR1L = 0xDC;           //carga do valor inicial no contador (65536-62500) -> mexer depois
    TMR1H = 0x0B;           //3036. Quando estourar contou 62500, passou 0,5s 
    T1CONbits.TMR1ON = 1;   //Liga o timer
    
    //Conversor A/D
    //ficar com AN0 AN1 AN3 como entrada anal?gica.
    ADCON1bits.PCFG0   = 0;  //configura as entradas anal?gicas
    ADCON1bits.PCFG1   = 1;  //configura as entradas anal?gicas
    ADCON1bits.PCFG2   = 0;  //configura as entradas anal?gicas
    ADCON1bits.PCFG3   = 0;  //configura as entradas anal?gicaS
    ADCON0bits.ADCS0 = 0  ;  //confirmando default Fosc/2 -- Clock de Conversão
    ADCON0bits.ADCS1 = 0  ;  //confirmando default Fosc/2 /
    ADCON1bits.ADFM = 0   ;  //8 ou 10 bits 0 = 8 bits
    ADRESL = 0x00;          //inicializar valor anal?gico com 0
    ADRESH = 0x00;  
    ADCON0bits.ADON = 1;     //Liga AD

    //Configurar WDT 
    OPTION_REGbits.PSA = 1; //define pre-escalar para WDT
    
    OPTION_REGbits.PS0 = 0;
    OPTION_REGbits.PS1 = 1;
    OPTION_REGbits.PS2 = 1; //1152ms
    
    CLRWDT();
    
    Lcd_Init();
    Lcd_Clear();
    
    while(1)
    {       
        CLRWDT();
        
        Temperatura();
        Umidade();
        
        LigDesl = PORTBbits.RB0;
        AftDias = PORTBbits.RB1;
        
        if(AftDias == 1){ //alterar os parametros apos os 18 dias 
            TempMax = 37.8;
            TempMin = 37.4;
        }
        else if(AftDias == 0){
            TempMax = 37.4;
            TempMin = 36.9;
        }
        
        if(LigDesl == 0) 
        {   
            if(temperatura > TempMax && umidade < 50)
            {
                PORTCbits.RC3 = 0; //aquecedor
                PORTCbits.RC4 = 1; //umidificador
                PORTCbits.RC5 = 1; //ventilação
            }
            else if(temperatura > TempMax && umidade > 55)
            {
                PORTCbits.RC3 = 0;
                PORTCbits.RC4 = 0;
                PORTCbits.RC5 = 1;   
            }
            else if(temperatura < TempMin && umidade < 50)
            {
                PORTCbits.RC3 = 1;
                PORTCbits.RC4 = 1;
                PORTCbits.RC5 = 0;   
            }
            else if(temperatura < TempMin && umidade > 55)
            {
                PORTCbits.RC3 = 1;
                PORTCbits.RC4 = 0;
                PORTCbits.RC5 = 0;   
            }
        }
    }
}
    
