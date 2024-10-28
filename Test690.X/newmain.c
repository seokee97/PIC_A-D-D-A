/*
PIC Config

    OSCCON - OSCILLATOR CONTROL REGISTER
        100 = 1 MHz
        101 = 2 MHz
        110 = 4 MHz
        111 = 8 MHz

    A/D CONVERSION CLOCK
        000 = Fosc / 2
        100 = Fosc / 4
        001 = Fosc / 8
        101 = Fosc / 16
        010 = Fosc / 32
        110 = Fosc / 64

    SPI CLOCK
        0000 = Fosc / 4
        0001 = Fosc / 16
        0010 = Fosc / 64

 */

// CONFIG
#pragma config FOSC = INTRCIO 
#pragma config WDTE = OFF      
#pragma config PWRTE = OFF     
#pragma config MCLRE = ON      
#pragma config CP = OFF        
#pragma config CPD = OFF       
#pragma config BOREN = OFF     
#pragma config IESO = OFF      
#pragma config FCMEN = OFF     

#include <xc.h>
#include <stdio.h>
#include <stdlib.h>

#define _XTAL_FREQ 4000000     

//ADC
#define AN2PIN TRISAbits.TRISA2
#define ANALOGAN2 ANSELbits.ANS2

//SPI
#define SCK TRISBbits.TRISB6
#define SDI TRISBbits.TRISB4

#define SDO TRISCbits.TRISC7

#define CS1  TRISCbits.TRISC6
#define CS2  TRISCbits.TRISC3

#define CS_C1 PORTCbits.RC6
#define CS_C2 PORTCbits.RC3

//QUEUE
#define QUEUE_SIZE 16


// ADC Data Struct
typedef struct {
    unsigned short adreshData;
    unsigned short adreslData;
} adcData;

// ADC Data Queue Struct
typedef struct {
    adcData buffer[QUEUE_SIZE]; // ADC ???? ???? ??
    int head;
    int tail;
    int count;
} CircularQueue;

// QUEUE Functions
void initQueue(CircularQueue *q) {
    q->head = 0;
    q->tail = 0;
    q->count = 0;
}

int isQueueFull(CircularQueue *q) {
    return q->count == QUEUE_SIZE;
}

int isQueueEmpty(CircularQueue *q) {
    return q->count == 0;
}

void enqueue(CircularQueue *q, adcData data) {
    if (!isQueueFull(q)) {
        q->buffer[q->tail] = data;
        q->tail = (q->tail + 1) % QUEUE_SIZE;
        q->count++;
    }
}

adcData dequeue(CircularQueue *q) {
    adcData data = {0, 0}; // ????? ???
    if (!isQueueEmpty(q)) {
        data = q->buffer[q->head];
        q->head = (q->head + 1) % QUEUE_SIZE;
        q->count--;
    }
    return data;
}

// ADC
void ADC_Init() {
    ANSEL = 0x00;   
    ANSELH = 0x00;

    //AN2 PIN Init
    AN2PIN = 1;
    ANALOGAN2 = 1;  
    
    ADCON0bits.CHS = 2; 
    ADCON1bits.ADCS = 0b001; 
    ADCON0bits.ADFM = 1; 
    ADCON0bits.ADON = 1; 
    
    ADCON0bits.VCFG = 0;
}
adcData ADC_Read() {
    ADCON0bits.GO = 1;  
    while (ADCON0bits.GO_nDONE);  
    adcData result;
    result.adreshData = ADRESH;
    result.adreslData = ADRESL;
    return result;  
}

//SPI
void SPI_Init() {
    SCK = 0;
    SDI = 1;
    SDO = 0;
    
    CS1 = 0;
    CS2 = 0;
    
    SSPCON  = 0b00100010;
    SSPSTAT = 0b11000000;
}

void SPI_Write(unsigned char data) {
    SSPBUF = data; 
    while(!SSPSTATbits.BF);  
}

//MCP4921 ADC_Data -> DAC
void MCP4921_SetOutput(adcData data) {
    unsigned char configBit = 0b01010000;
    unsigned char highByte = configBit | (data.adreshData & 0x0F);
    unsigned char lowByte = (unsigned char)data.adreslData;
    unsigned char zeroByte = 0b00000000;
        
    CS_C1 = 0;
    SPI_Write(highByte);
    SPI_Write(lowByte);
    CS_C1 = 1;
}

void main(void) {
    ADC_Init();
    SPI_Init();
    
    CircularQueue adcQueue;
    initQueue(&adcQueue);  

    CS_C1 = 1;     
    CS_C2 = 1;
    
    while(1) {
        if (!isQueueFull(&adcQueue)) {
            adcData adcValue = ADC_Read();
            enqueue(&adcQueue, adcValue); 
        }

        if (!isQueueEmpty(&adcQueue)) {
            adcData data = dequeue(&adcQueue);
            MCP4921_SetOutput(data); 
        }
    }
}