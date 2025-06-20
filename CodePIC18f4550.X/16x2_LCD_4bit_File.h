#ifndef XC_HEADER_TEMPLATE_H
#define XC_HEADER_TEMPLATE_H

#include <xc.h>
#include "Configuration_Header_File.h"

#define _XTAL_FREQ 8000000

#define RS LATD0
#define EN LATD1
#define ldata LATD
#define LCD_Port TRISD

void MSdelay(unsigned int);
void LCD_Init();
void LCD_Command(unsigned char);
void LCD_Char(unsigned char);
void LCD_String(const char *);
void LCD_String_xy(char, char, const char *);
void LCD_Clear();

#endif
