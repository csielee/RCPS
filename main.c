
#include "io430.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STRING_SIZE 512


int rx0_f,rx0_b;
char rx0_str[STRING_SIZE];
int rx1_f,rx1_b;
char rx1_str[STRING_SIZE];



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

void UCA0clear()
{
  rx0_b = (rx0_f + 1) % STRING_SIZE;
}

int UCA0find(const char *str)
{
  int count=0,len = strlen(str);
  char ch;
  while(1)
  {
    while(UCA0available()==0);
    ch = UCA0read();
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

int ESP_OK()
{
  static char OK[]="OK\r\n";
  static char ERROR[]="ERROR\r\n";
  int OK_l=strlen(OK),ERROR_l=strlen(ERROR);
  int i=0,j=0;
  char ch;
  while(1)
  {
    while(!(ESPavailable()>0));
    ch = ESPread();
    if(ch == *(OK+i))
      i++;
    else
      i=0;
    if(ch == *(ERROR+j))
      j++;
    else
      j=0;
    if(i >= OK_l)
      return 1;
    if(j >= ERROR_l)
      return 0;
  }
}

int ESP_sendData(int id,const char *msg)
{
  int len = strlen(msg);
  char str[30];
  sprintf(str,"AT+CIPSEND=%d,%d\r\n",id,len);
  ESPprint(str);
  ESPfind(">");
  ESPprint(msg);
  //ESPfind("OK\r\n");
  
  return ESP_OK();
}

char temp1[100],temp2[100];

void ESP_begin()
{
  char str[200],temp[100];
  int re,id,length,connect=0;
  //檢查狀態
  ESPprint("AT+CIPSTATUS\r\n");
  ESPfind("STATUS:");
  ESPreadstr(str);
  ESPfind("OK\r\n");
  int status = str[0] - '0';//get status
  switch(status)
  {
  case 2://has IP
    UCA1print("has connect\n");
    break;
  case 5://No connect wifi
    UCA1print("No connect\r\n");
    ESPprint("AT+CIPMUX=1\r\n");
    re = ESP_OK();
    sprintf(str,"COPMUX:%d\r\n",re);
    UCA1print(str);
    ESPprint("AT+CIPSERVER=1,80\r\n");
    sprintf(str,"CIPSERVER:%d\r\n",re);
    UCA1print(str);
    UCA1print("wait SSID&PWD\r\n");
    while(!connect)
    {
      ESPfind("+IPD");
      ESPreadstr(str);
      sscanf(str,",%d,%d:GET %s",&id,&length,temp);
      re = sscanf(temp,"/?SSID=%[^&]&PWD=%s",temp1,temp2);
      sprintf(str,"id : %d,length : %d,str : [%s],re : %d\r\n",id,length,temp,re);
      UCA1print(str);
      //ESPfind("\r\n\r\n");
      if(1)
      {
        //while(ESP_sendData(id,"<html><head><title>RCPS wifi config</title></head><body><form style=text-align:center; method=get><br><h2>RCPS wifi 連線設定</h2><br>SSID : <input type=text name=SSID><br><br>PWD  : <input type=password name=PWD><br><br><input type=submit value=RCPS連線設定></form>	</body></html>")==0);
        while(ESP_sendData(id,"HTTP/1.1 200 OK\r\nLocation: http://192.168.4.1/\r\nContent-Type: text/html; charset=UTF-8\r\nContent-Length: 4\r\n\r\ntest")==0);
                //while(ESP_sendData(id,"<html>test<br>test</html>")==0);
        sprintf(str,"AT+CIPCLOSE=%d\r\n",id);
        ESPprint(str);
        ESP_OK();
      }
      else
      {
        //connect wifi
        sprintf(str,"AT+CWJAP_DEF=\"%s\",\"%s\"\r\n",temp1,temp2);
        ESPprint(str);
        ESPreadstr(str);
        if(strlen(str) > 4)
        {
          re = str[8] - '0';
          sprintf(temp,"error : %d\r\n",re);
          UCA1print(temp);
          __no_operation();
        }
        else
        {
          ESPreadstr(str);
          sprintf(str,"AT+CIFSR\r\n");
          ESPfind("CIFSR:");
          ESPfind("CIFSR:");
          ESPreadstr(str);
          re = strlen(str);
          str[re-2]='\0';
          sprintf(temp,"<div style=text-align:center;>connect success, IP = %s</div>",str);
          connect = 1;
        }
        ESPclear();
        while(ESP_sendData(id,temp)==0);
        sprintf(str,"AT+CIPCLOSE=%d\r\n",id);
        ESPprint(str);
        ESP_OK();
      }
    }
    break;
  default:
    break;
  }
  UCA0clear();
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
  
    
  
  //loop
  __bis_SR_register(GIE);       // Enter interrupts enabled
  __no_operation();                         // For debugger
  ESP_begin();
  while(1)
  {
    if(UCA1available()>0)
    {
      UCA1readstr(temp1);
      UCA0print(temp1);
    }
    if(UCA0available()>0)
    {
      UCA0readstr(temp1);
      UCA1print("[");
      UCA1print(temp1);
      UCA1print("]\n");
    }
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
