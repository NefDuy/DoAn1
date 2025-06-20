#include "16x2_LCD_4bit_File.h"

void LCD_Init()
{
    LCD_Port = 0;
    MSdelay(15);
    LCD_Command(0x02);
    LCD_Command(0x28);
    LCD_Command(0x01);
    LCD_Command(0x0c);
    LCD_Command(0x06);
}

void LCD_Command(unsigned char cmd)
{
    ldata = (ldata & 0x0f) | (cmd & 0xF0);  
    RS = 0;
    EN = 1;
    NOP();
    EN = 0;
    MSdelay(1);
    ldata = (ldata & 0x0f) | ((cmd << 4) & 0xF0);  
    EN = 1;
    NOP();
    EN = 0;
    MSdelay(3);
}

void LCD_Char(unsigned char dat)
{
    ldata = (ldata & 0x0f) | (dat & 0xF0);  
    RS = 1;
    EN = 1;
    NOP();
    EN = 0;
    MSdelay(1);
    ldata = (ldata & 0x0f) | ((dat << 4) & 0xF0);  
    EN = 1;
    NOP();
    EN = 0;
    MSdelay(3);
}

void LCD_String(const char *msg)
{
    while((*msg) != 0)
    {		
        LCD_Char(*msg);
        msg++;	
    }
}

void LCD_String_xy(char row, char pos, const char *msg)
{
    char location = 0;
    if (row == 0)
    {
        location = (0x80) | (pos & 0x0f);
        LCD_Command(location);
    }
    else
    {
        location = (0xC0) | (pos & 0x0f);
        LCD_Command(location);
    }
    LCD_String(msg);
}

void LCD_Clear()
{
    LCD_Command(0x01);
}

void MSdelay(unsigned int val)
{
    unsigned int i, j;
    for (i = 0; i < val; i++)
        for (j = 0; j < 165; j++);
}
