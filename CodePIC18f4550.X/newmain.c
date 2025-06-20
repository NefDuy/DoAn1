/*
 * Keypad Interfacing with PIC18F4550
 * http://www.electronicwings.com
 */

#include <xc.h>  // S?a l?i include
#include <string.h>  // S?a l?i dùng strcmp, memset
#include "Configuration_Header_File.h"
#include "16x2_LCD_4bit_File.h"
#include <stdio.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#define F_CPU 8000000/64 // value to calculate baud rate

#define BUZZER LATEbits.LATE0  // Buzzer k?t n?i v?i chân RC0
#define MAX_WRONG_ATTEMPTS 3   // S? l?n nh?p sai t?i ?a
int wrong_attempts = 0;        // Bi?n ??m s? l?n nh?p sai

#define MAX_LENGTH 6  
char input[MAX_LENGTH + 1];  
char storedData[] = "123456";
int index = 0;  

unsigned char keyfind();  // Hàm tìm phím nh?n

#define write_port LATB
#define read_port PORTB
#define Direction_Port TRISB
unsigned char col_loc, rowloc, temp_col;

void UART_Write(char data);  // Khai báo nguyên m?u

unsigned char keypad[4][4] = {
    {'7', '8', '9', '/'},
    {'4', '5', '6', '*'},
    {'1', '2', '3', '-'},
    {' ', '0', '=', '+'}
};

void UART_Initial(long baud_rate)
{
float bps; 
TRISCbits.RC6=1;
TRISCbits.RC7=1;
bps = (( (float) (F_CPU) / (float) baud_rate) - 1); // sets baud rate
SPBRG=(int)bps; // store value for 9600 baud rate
TXSTAbits.CSRC = 0; // Don't care for asynchronous mode
TXSTAbits.TX9 = 0; // Selects 8-bit data transmission
TXSTAbits.TXEN = 1; // Enable Data tranmission on RC6 pin
TXSTAbits.SYNC = 0; // Selects Asynchronous Serial Communication Mode
TXSTAbits.BRGH = 0; // Default Baud rate speed 
BAUDCONbits.BRG16 = 0; // selects only 8 bit register for baud rate 
RCSTA = 0x90; // Enables UART Communication PORT

}

char USART_Read()
{
while(RCIF==0); // see if data on RC7 pin is available 
if(RCSTAbits.OERR)
{ 
CREN = 0;
NOP();
CREN=1;
}
return(RCREG); //read the byte from recieve register and return value
}
void PWM_Init()
{
    TRISCbits.TRISC2 = 0;  // RC2 làm output cho servo
    CCP1CON = 0x0C;  // Ch? ?? PWM
}
int setPeriodTo(unsigned long FPWM)
{
    int TimerPrescaleBits, TimerPrescaleValue;
    float period;
    unsigned long FOSC = 8000000;  // PIC ch?y ? 8MHz

    if (FPWM < 8000) { TimerPrescaleBits = 2; TimerPrescaleValue = 16; }
    else { TimerPrescaleBits = 0; TimerPrescaleValue = 1; }

    period = ((float)FOSC / (4.0 * (float)TimerPrescaleValue * (float)FPWM)) - 1.0;
    period = round(period);

    PR2 = (int)period;
    T2CON = TimerPrescaleBits;
    TMR2 = 0;
    T2CONbits.TMR2ON = 1;  // B?t Timer2
    return (int)period;
}
void SetDutyCycleTo(float Duty_cycle, int Period)
{
    int PWM10BitValue = 4.0 * ((float)Period + 1.0) * (Duty_cycle / 100.0);
    CCPR1L = (PWM10BitValue >> 2);
    CCP1CON = ((PWM10BitValue & 0x03) << 4) | 0x0C;
}

void main() 
{   
    int doi_mat_khau = 0;  // 0: ch? ?? th??ng, 1: ?ang nh?p m?t kh?u m?i   
    char data;
    OSCCON=0x72; // Select internal oscillator with frequency = 8MHz
    TRISDbits.RD0=0; //Make RD0 pin as a output pin
    PORTDbits.RD0=0; //initialize RD0 pin to logic zeeo
    UART_Initial(9600);

    char key;
    OSCCON = 0x72;

    TRISEbits.TRISE0 = 0;  // RC0 làm output cho buzzer
    BUZZER = 0;  // T?t buzzer ban ??u

    PWM_Init();  // Kh?i t?o PWM cho servo
    int Period = setPeriodTo(50);  // ??t t?n s? PWM 50Hz
    SetDutyCycleTo(3.0, Period);  // Ban ??u servo ? v? trí khóa (0°)

    RBPU = 0;
    LCD_Init();
    LCD_String_xy(0, 0, "Nhap mat ma:");
    LCD_Command(0xC0);

    while (1)
    {
        key = keyfind(); // ??c phím

        if (key == '=')  // Nh?n '=' ?? xác nh?n
        {
            input[index] = '\0';

            if (doi_mat_khau == 1) {
                strcpy(storedData, input);  // C?p nh?t m?t kh?u m?i
                LCD_Clear();
                LCD_String_xy(0, 0, "MK da doi!");
                MSdelay(2000);
                
                LCD_Clear();
                LCD_String_xy(0, 0, "Nhap mat ma:");
                LCD_String_xy(1, 0, "");
                doi_mat_khau = 0;  // Quay l?i ch? ?? th??ng
            }
            else {
                
                LCD_Clear();

                if (strcmp(input, storedData) == 0) {
                    LCD_String_xy(0, 0, "Mo khoa!");
                    SetDutyCycleTo(7.0, Period);  
                    UART_Write('U');
                    wrong_attempts = 0;
                }
                else {
                    wrong_attempts++;
                    LCD_String_xy(0, 0, "Sai mat ma!");

                    if (wrong_attempts >= MAX_WRONG_ATTEMPTS) {
                        LCD_String_xy(1, 0, "Khoa he thong!");
                        for (int i = 0; i < 3; i++) {
                            BUZZER = 1;
                            MSdelay(500);
                            BUZZER = 0;
                            MSdelay(500);
                        }
                        wrong_attempts = 0;
                    }
                }

                MSdelay(2000);
                LCD_Clear();
                LCD_String_xy(0, 0, "Nhap mat ma:");
                LCD_String_xy(1, 0, "");
            }

            // Reset input
            index = 0;
            memset(input, 0, sizeof(input));
        }

        else if (key == '+')  // 
        {
            index = 0;
            memset(input, 0, sizeof(input));
            doi_mat_khau = 1;  // 
            LCD_Clear();
            LCD_String_xy(0, 0, "Nhap mk moi");
            
        }

        else if (key == '-')  // 
        {
            if (index > 0) {
                index--;
                input[index] = '\0';

                LCD_Command(0x10);  // Move cursor left
                LCD_Char(' ');
                LCD_Command(0x10);  // Move cursor left again
            }
        }

        else if (key == '*')  // ? Khóa l?i (servo v? v? trí c?)
        {
            LCD_Clear();
            LCD_String_xy(0, 0, "Khoa lai!");
            SetDutyCycleTo(3.0, Period);  // Servo ?óng khóa
            UART_Write('L');
            MSdelay(1000);
            LCD_Clear();
            LCD_String_xy(0, 0, "Nhap mat ma:");
            LCD_String_xy(1, 0, "");
        }

        else if (key == '/')  // nhan lenh tu Uart
        {   
            
            index = 0;
            for (int t = 0; t < 5000; t += 100)
            {   
                LCD_String_xy(0, 0, "Face scan");
                data = USART_Read();
                if (data == 'U') {
                    LCD_Clear();
                    LCD_String_xy(0, 0, "Mo khoa!");
                    SetDutyCycleTo(7.0, Period);
                    MSdelay(5000);
                    SetDutyCycleTo(3.0, Period);
                    LCD_Clear();
                    LCD_String_xy(0, 0, "dong lai");
                    MSdelay(2000);
                    LCD_Clear();
                    LCD_String_xy(0, 0, "Nhap mat ma:");
                    LCD_String_xy(1, 0, "");
                    break;
                }
            }
        }

        else if (index < MAX_LENGTH && key != 0)  
        {
            if (doi_mat_khau == 1 && index == 0) {
                LCD_Command(0xC0);  
            }

            input[index++] = key;
            LCD_Char(key);
        }
    }
}
void UART_Write(char data) {
    while (!PIR1bits.TXIF);  // ??i cho ??n khi TXREG s?n sàng
    TXREG = data;            // G?i ký t?
}



unsigned char keyfind()
{
    Direction_Port = 0xf0;
    unsigned char temp1;
 
    write_port = 0xf0;  
    do {
        do {
            col_loc = read_port & 0xf0;    
        } while (col_loc != 0xf0);
        col_loc = read_port & 0xf0;
    } while (col_loc != 0xf0);
    
    MSdelay(50);
    write_port = 0xf0;  
    do {
        do {
            col_loc = read_port & 0xf0;
        } while (col_loc == 0xf0);
        col_loc = read_port & 0xf0;
    } while (col_loc == 0xf0);
             
    MSdelay(20);
    col_loc = read_port & 0xf0;
             
    while (1)
    {
        write_port  = 0xfe; 
        col_loc = read_port & 0xf0;
        temp_col = col_loc;
        if (col_loc != 0xf0)
        {
            rowloc = 0;
            while (temp_col != 0xf0)  
                temp_col = read_port & 0xf0;
            break;
        }
        
        write_port = 0xfd;
        col_loc = read_port & 0xf0;
        temp_col = col_loc;
        if (col_loc != 0xf0)
        {
            rowloc = 1;
            while (temp_col != 0xf0)
                temp_col = read_port & 0xf0;
            break;
        }
        
        write_port = 0xfb;
        col_loc = read_port & 0xf0;
        temp_col = col_loc;
        if (col_loc != 0xf0)
        {
            rowloc = 2;
            while (temp_col != 0xf0)
                temp_col = read_port & 0xf0;
            break;
        }
        
        write_port = 0xf7;
        col_loc = read_port & 0xf0;
        temp_col = col_loc;
        if (col_loc != 0xf0)
        {
            rowloc = 3;
            while (temp_col != 0xf0)
                temp_col = read_port & 0xf0;
            break;
        }
    }
    
    while (1)
    {
        if (col_loc == 0xe0) return keypad[rowloc][0];
        else if (col_loc == 0xd0) return keypad[rowloc][1];
        else if (col_loc == 0xb0) return keypad[rowloc][2];
        else return keypad[rowloc][3];    
    }
    
    MSdelay(300);     
}

