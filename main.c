
#include "io430.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STRING_SIZE 512
#define FREQUENCY  1048576
#define LOOPBODY 8 
#define LOOPCNT 260 

int rx0_f,rx0_b;
char rx0_str[STRING_SIZE];
int rx1_f,rx1_b;
char rx1_str[STRING_SIZE];
char use_str[STRING_SIZE];


void DelayMs(unsigned int ms)
{
  unsigned int iq0, iq1;
  for (iq0=ms; iq0>0; iq0--)
  {
    for (iq1=LOOPCNT; iq1>0; iq1--);
      
  }
} 

int UCA0print(const char *str)
{
  int len = strlen(str);
  for(int i=0;i<len;i++)
  {
      while (!(UCA0IFG&UCTXIFG));             // USCI_A0 TX buffer ready?
      UCA0TXBUF = str[i];                  // TX -> RXed character
  }
  while (!(UCA0IFG&UCTXIFG));
  return len;
}

int UCA0available() {
  return rx0_b>rx0_f?rx0_b - rx0_f - 1:STRING_SIZE - rx0_f + rx0_b - 1;
}

char UCA0read() {
  if(UCA0available()>0)
  {
    rx0_f = (rx0_f + 1) % STRING_SIZE;
    return rx0_str[rx0_f];
  }
  else
    return 0;
}

void UCA0readstr(char * str) {
  char ch;
  int count=0;
  while(1)
  {
    while(!(UCA0available()>0));
    ch = UCA0read();
    str[count++] = ch;
    if(ch == '\n')
      break;
  }
  str[count] = 0;
}

void UCA0readwait()
{
  int temp = rx0_b,count=0;
  while(count < 5)
  {
    DelayMs(100);
    if(rx0_b==temp)
      count++;
    else
      count = 0;
    temp = rx0_b;
  }
}

void UCA0clear()
{
  rx0_b = (rx0_f + 1) % STRING_SIZE;
}

int UCA0find(const char *str,int timeout)
{
  int count=0,len = strlen(str);
  unsigned int t;
  char ch;
  while(1)
  {
    t = 0;
    while(UCA0available()==0)
    {
      if(timeout>0)
        t++;
      if(t>60000)
        return 0;
    }
    ch = UCA0read();

    if(ch == *(str+count))
    {
      count++;
      if(count>=len)
      {
        return 1;
      }
    }
    else
      count = 0;
  }
}

int UCA1print(const char *str)
{
  int len = strlen(str);
  for(int i=0;i<len;i++)
  {
      while (!(UCA1IFG&UCTXIFG));             // USCI_A0 TX buffer ready?
      UCA1TXBUF = str[i];                  // TX -> RXed character
  }
  while (!(UCA1IFG&UCTXIFG));
  return len;
}

int UCA1available() {
  return rx1_b>rx1_f?rx1_b - rx1_f - 1:STRING_SIZE - rx1_f + rx1_b - 1;
}

char UCA1read() {
  if(UCA1available()>0)
  {
    rx1_f = (rx1_f + 1) % STRING_SIZE;
    return rx1_str[rx1_f];
  }
  else
    return 0;
}

void UCA1readstr(char * str) {
  char ch;
  int count=0;
  while(1)
  {
    while(!(UCA1available()>0));
    ch = UCA1read();
    str[count++] = ch;
    if(ch == '\n')
      break;
  }
  str[count] = 0;
}

void UCA1clear()
{
  rx1_b = (rx1_f + 1) % STRING_SIZE;
}

int UCA1find(const char *str)
{
  int count=0,len = strlen(str);
  char ch;
  while(1)
  {
    while(UCA1available()==0);
    ch = UCA1read();
    //if(ch == '\n')
      //return 0;
    if(ch == *(str+count))
    {
      count++;
      if(count>=len)
      {
        return 1;
      }
    }
    else
      count = 0;
  }
}

//esp8266 wifi
#define ESPprint UCA0print
#define ESPfind UCA0find
#define ESPread UCA0read
#define ESPreadstr UCA0readstr
#define ESPavailable UCA0available
#define ESPclear UCA0clear
#define ESPreadwait UCA0readwait
#define ESPrxbuf rx0_str
#define ESPrx_f rx0_f
#define ESPrx_b rx0_b

int ESP_OK();
int ESP_receiveData(char* str);
int ESP_sendData(int id,char *msg);
int ESP_sendCommand(char *command);

char send_str[STRING_SIZE];

int ESP_receiveData(char* str)
{
  int id,length;

  ESPfind("+IPD",0);
  ESPreadstr(send_str);
  //debug
  UCA1print(send_str);
  //debug
  sscanf(send_str,",%d,%d:GET /%s",&id,&length,str);
  //debug
  sprintf(send_str,"id : %d,length : %d,str : [%s]\r\n",id,length,str);
  UCA1print(send_str);
  //debug
  ESPreadwait();
  ESPclear();
  return id;
}

int ESP_sendData(int id,char *msg)
{
  int len = strlen(msg),re,count;
  char command[30];
  sprintf(send_str,"HTTP/1.1 200 OK\r\nLocation: http://192.168.4.1/\r\nContent-Type: text/html; charset=UTF-8\r\nContent-Length: %d\r\n\r\n%s",len,msg);
  len = strlen(send_str);
  sprintf(command,"AT+CIPSEND=%d,%d\r\n",id,len);
  do {
    while(ESP_sendCommand(command)==0)
      DelayMs(100);
    count++;
    if(count > 100)
      return 3;
  }while(ESPfind(">",1)==0);
  
  ESPprint(send_str);
  DelayMs(500);
  re = ESP_OK();
  ESPclear();
  return re;
}

int ESP_sendCommand(char *command)
{
  if(command[strlen(command)-1]=='@')
  command[strlen(command)-1]='?';
  ESPprint(command);
  if(command[strlen(command)-1]!='\n')
    ESPprint("\r\n");
  
  return 1;
}

int ESP_OK()
{
  static char OK[]="OK\r\n";
  static char ERROR[]="ERROR\r\n";
  static char FAIL[]="FAIL\r\n";
  static int OK_l=4,ERROR_l=7,FAIL_l=6;
  int i=0,j=0,k=0,v=0;
  char ch;
  while(1>0)
  {
    while(ESPavailable()==0)
    { }
    v++;
    ch = ESPrxbuf[(ESPrx_f+v)%STRING_SIZE];
    if(ch == *(OK+i) )
      i++;
    else
    {
      i=0;
      if(ch == *(OK+i) )
        i++;
    }
    if(ch == *(ERROR+j) )
      j++;
    else
    {
      j=0;
      if(ch == *(ERROR+j) )
        j++;
    }
    if(ch == *(FAIL+k) )
      k++;
    else
    {
      k=0;
      if(ch == *(FAIL+k) )
        k++;
    }
    
    if(j == ERROR_l)
      return 0;
    if(k == FAIL_l)
      return -1;
    if(i == OK_l)
      return 1;
  }
  return 2;
}

int main( void )
{
  // Stop watchdog timer to prevent time out reset
  WDTCTL = WDTPW + WDTHOLD;

  //UCA0
  P3SEL = BIT3+BIT4;                        // P3.4,5 = USCI_A0 TXD/RXD
  UCA0CTL1 |= UCSWRST;                      // **Put state machine in reset**
  UCA0CTL1 |= UCSSEL_2;                     // SMCLK
  UCA0BR0 = 6;                              // 1MHz 9600 (see User's Guide)
  UCA0BR1 = 0;                              // 1MHz 9600
  UCA0MCTL = UCBRS_0 + UCBRF_13 + UCOS16;   // Modln UCBRSx=0, UCBRFx=0,
                                            // over sampling
  UCA0CTL1 &= ~UCSWRST;                     // **Initialize USCI state machine**
  UCA0IE |= UCRXIE;                         // Enable USCI_A0 RX interrupt
  //UCA1
  P4SEL = BIT4+BIT5;                        // P4.4,5 = USCI_A1 TXD/RXD
  UCA1CTL1 |= UCSWRST;                      // **Put state machine in reset**
  UCA1CTL1 |= UCSSEL_2;                     // SMCLK
  UCA1BR0 = 6;                              // 1MHz 9600 (see User's Guide)
  UCA1BR1 = 0;                              // 1MHz 9600
  UCA1MCTL = UCBRS_0 + UCBRF_13 + UCOS16;   // Modln UCBRSx=0, UCBRFx=0,
                                            // over sampling
  UCA1CTL1 &= ~UCSWRST;                     // **Initialize USCI state machine**
  UCA1IE |= UCRXIE;                         // Enable USCI_A1 RX interrupt
  
  //init
  rx0_f=0;
  rx0_b=1;
  rx1_f=0;
  rx1_b=1;
  int id,re,i;
  char command[50];  
  
  __bis_SR_register(GIE);       // Enter interrupts enabled
  __no_operation();                         // For debugger
  
  ESPprint("AT+CIPMUX=1\r\n");
  re = ESP_OK();
  ESPclear();
  sprintf(command,"COPMUX:%d\r\n",re);
  UCA1print(command);
  ESPprint("AT+CIPSERVER=1,80\r\n");
  re = ESP_OK();
  ESPclear();
  sprintf(command,"CIPSERVER:%d\r\n",re);
  UCA1print(command);
  
  //loop
  while(1)
  {
    id = ESP_receiveData(command);
    //execute command
    while(ESP_sendCommand(command)==0)
      DelayMs(100);
    //get respand
    //DelayMs(4000);
    //ESPreadwait();
    if(command[3]=='C' && command[4]=='W')
      DelayMs(4000);
    else
      DelayMs(500);
    re = ESP_OK();
    sprintf(use_str,"%s:%d\r\n",command,re);
    UCA1print(use_str);
    for(i=0;ESPavailable();i++)
    {
      use_str[i] = ESPread();
    }
    use_str[i] = '\0';
    while(ESP_sendData(id,use_str)!=1) // not OK
    {
      UCA1print("send fail,try again\r\n");
    }
    UCA1print("send OK\r\n");
    sprintf(command,"AT+CIPCLOSE=%d\r\n",id);
    ESPprint(command);
    ESP_OK();
    ESPclear();
  }
  //__bis_SR_register(LPM0_bits + GIE);       // Enter LPM0, interrupts enabled
  //__no_operation();                         // For debugger
  
  //return 0;
}

//UCA0
#pragma vector=USCI_A0_VECTOR
__interrupt void USCI_A0_ISR(void)
{
  switch(__even_in_range(UCA0IV,4))
  {
  case 0:break;                             // Vector 0 - no interrupt
  case 2:                                   // Vector 2 - RXIFG
    if(rx0_b == rx0_f)
      break;
    rx0_str[rx0_b++] = UCA0RXBUF;
    if(rx0_b == STRING_SIZE)
      rx0_b=0;
      
    break;
  case 4:break;                             // Vector 4 - TXIFG
  default: break;
  }
}
//UCA1
#pragma vector=USCI_A1_VECTOR
__interrupt void USCI_A1_ISR(void)
{
  switch(__even_in_range(UCA1IV,4))
  {
  case 0:break;                             // Vector 0 - no interrupt
  case 2:                                   // Vector 2 - RXIFG
    if(rx1_b == rx1_f)
      break;
    rx1_str[rx1_b++] = UCA1RXBUF;
    if(rx1_b == STRING_SIZE)
      rx1_b=0;
    break;
  case 4:break;                             // Vector 4 - TXIFG
  default: break;
  }
}
