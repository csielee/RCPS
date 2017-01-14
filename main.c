
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

//RCPS
#define RCPS_number 4
char Rcommand[10];
int currentA[RCPS_number];

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
  int len = strlen(msg),re,count=0;
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
  WDTCTL = WDTPW + WDTSSEL_1 + WDTTMSEL + WDTIS_4 + WDTCNTCL;
  SFRIE1 |= WDTIE;
  REFCTL0 &= ~REFMSTR;
  __bic_SR_register(GIE);
  
  //port
  //set 1.0 1.2 1.3 1.4 1.5 output
  P1DIR |= 0x3D; 
  P1SEL &= ~0x3D;
  // turn on LED1
  P1OUT = 0x01;
  //set 6.0 6.1 6.2 6.3(A0~A3)
  P6DIR &= ~0x0F;
  P6SEL |= 0x0F; 
  
  
  //ADC12
  ADC12CTL0 = 0;
  ADC12CTL0 |= ADC12SHT0_12 | ADC12MSC | ADC12ON ;
  ADC12CTL1 = ADC12SSEL_2 | ADC12SHP | ADC12CONSEQ_3 ;
  ADC12CTL2 = ADC12TCOFF | ADC12RES_2 | ADC12REFOUT;
  ADC12MCTL0 = ADC12SREF_0 | ADC12INCH_0;
  ADC12MCTL1 = ADC12INCH_11;
  ADC12MCTL2 = ADC12INCH_2;
  ADC12MCTL3 = ADC12EOS | ADC12INCH_3;
  ADC12IE = 0x0F;
  
  
  
  //timer
  TA0CCR0 = 16384;
  TA0CCTL0 = CCIE;
  TA0CTL = TASSEL_1 | MC_1 | TACLR;

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
  int Rcommand_number;
  
  DelayMs(1000);
  __bis_SR_register(GIE);       // Enter interrupts enabled
  __no_operation();                         // For debugger
  
  
  ESPclear();
  ESPprint("AT+CIPMUX=1\r\n");
  re = ESP_OK();
  ESPclear();
  sprintf(command,"CIPMUX:%d\r\n",re);
  UCA1print(command);
  ESPprint("AT+CIPSERVER=1,80\r\n");
  re = ESP_OK();
  ESPclear();
  sprintf(command,"CIPSERVER:%d\r\n",re);
  UCA1print(command);
  
  //turn off
  TA0CCR0 =  8192;
  
  //loop
  ADC12CTL0 |= ADC12ENC | ADC12ON | ADC12SC;
  while(1)
  {
    TA0CTL = TASSEL_1 | MC_0 | TACLR;
    P1OUT |= 0x01;
    id = ESP_receiveData(command);
    P1OUT &= ~0x01;
    TA0CTL = TASSEL_1 | MC_1 | TACLR; 
    //execute command
    if(command[0] == 'A')
    {
      while(ESP_sendCommand(command)==0)
        DelayMs(100);
      //get respand
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
    }
    else if(command[0] == 'R')
    {
      int use_str_t=0,query=0;
      sprintf((use_str+use_str_t),"RCPS command : %s\n",command);
      use_str_t = strlen(use_str);
      Rcommand_number = -1;
      //re = sscanf(command,"RCPS+%[^=]=%d",Rcommand,&Rcommand_number);
      //int i;
      for(i=5;command[i]!='\0';i++)
      {
          if(command[i]=='=' || command[i]=='@')
            break;
          Rcommand[i-5]=command[i];
      }
      for(;command[i]!='\0';i++)
      {
        if(command[i]=='=')
        {
          Rcommand_number = 0;
          continue;
        }
        if(command[i]=='@')
        {
          query=1;
          break;
        }
        
        if(command[i]>='0' && command[i]<='9')
        {
          Rcommand_number = Rcommand_number*10 + (command[i]-'0');
        }
        else
          break;
      }
      
      //debug
      UCA1print(Rcommand);
      UCA1print("\r\n");

      if(strcmp(Rcommand,"CP")==0)
      {
        //P1OUT.bit0;
        if(query==1)
        {
          Rcommand_number = (P1OUT & 0x3C)>>2;
          Rcommand_number = (~Rcommand_number) & 0x0F;
          sprintf((use_str+use_str_t),"%d\nOK\n",Rcommand_number);
        }
        else if(Rcommand_number > -1)
        {
          Rcommand_number = ~Rcommand_number;
          //set P1.2~1.5
          P1OUT &= ~0x3C;
          P1OUT |= (Rcommand_number*4) & 0x3C;
          __no_operation(); 
          sprintf((use_str+use_str_t),"OK\n");
          __no_operation(); 
        }
        else
        {
          sprintf((use_str+use_str_t),"ERROR ARGUMENT\n");
        }
      }
      else
      {
        sprintf((use_str+use_str_t),"ERROR\n");
      }
    }
    else
    {
      sprintf(use_str,"vaild command");
    }
    i=0;
    while(ESP_sendData(id,use_str)!=1) // not OK
    {
      i++;
      UCA1print("send fail,try again\r\n");
      if(i>10)
        break;
      DelayMs(500);
    }
    if(i>10)
      UCA1print("send ERROR\r\n");
    else
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
    
    //P1OUT = P1OUT ^ 0x01;
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

#pragma vector = TIMER0_A0_VECTOR
__interrupt void TIMER0_A0_ISR(void)
{
  P1OUT = P1OUT ^ 0x01;
}

#pragma vector = ADC12_VECTOR
__interrupt void ADC12_ISR(void)
{
  switch(__even_in_range(ADC12IV,36))
  {
  case 6:
    currentA[0] = ADC12MEM0;
    //sprintf(use_str,"A0 : %d \r\n",ADC12MEM0;);
    //UCA1print(use_str);
    break;//MEM0
  case 8: 
    currentA[1] = ADC12MEM1;
    //sprintf(use_str,"A1 : %d \r\n",ADC12MEM1);
    //UCA1print(use_str);
    //ADC12IFG = 0;
    break;
  case 10: 
    currentA[2] = ADC12MEM2;
    break;
  case 12: 
    currentA[3] = ADC12MEM3;
    break;
  default: break;
  }
}

#pragma vector = WDT_VECTOR
__interrupt void WDT_ISR(void)
{
  //ADC12CTL0 |= ADC12SC;
  sprintf(use_str,"A0 : %d \r\n",currentA[0]);
  UCA1print(use_str);
}
