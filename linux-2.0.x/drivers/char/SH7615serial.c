
#include <linux/errno.h>
#include <linux/signal.h>
#include <linux/sched.h>
#include <linux/timer.h>
#include <linux/interrupt.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/config.h>
#include <linux/major.h>
#include <linux/string.h>
#include <linux/fcntl.h>
#include <linux/mm.h>
#include <linux/kernel.h>

#include <asm/io.h>
#include <asm/irq.h>
#include <asm/system.h>
#include <asm/segment.h>
#include <asm/bitops.h>
#include <asm/delay.h>

#include <asm/SH7615serial.h>
#include <asm/SH7615.h>

char 	  pwflag =0;
char 	  read_buffer[30];
char 	  read_status=0;
char 	  read_count =0;
void    echo_char(char );
void	read_xmodemdata();
void console_print_SH7615( char *p)
{

	int count = 0,flag = 1;
	int ftillhere = 0;
	while ((count<=8) && ((*SD_STATUS1_REG & TDFE_CHECK) == TDFE_CHECK) && flag == 1)
	{
		if(count== 8  )
		{
			ftillhere+=8;
			count=0;
			*SD_STATUS1_REG &= ~TDFE_CHECK;
			while((*SD_STATUS1_REG & TDFE_CHECK) != TDFE_CHECK);
		}
		if(*(p + count + ftillhere)=='\0')
		{
			for (;;)
			{ if((*SD_STATUS1_REG & TEND_CHECK) == TEND_CHECK)
				break;
			}	
			flag = 0;  // End of buffer
			
		}
		
		if(flag)
			*SD_TRANSMIT_DATA =*(p + count + ftillhere);
		count++;
	}
}


void console_receive_SH7615()
{	
 if((*SD_STATUS1_REG &( ER_MASK|BRK_MASK|FER_MASK|PER_MASK|DR_MASK)) ||
		 (*SD_STATUS2_REG & ORER_MASK)) /*checking any error in receive*/

	 do_error_handler();
	
 else 
      if ((*SD_STATUS1_REG & RDF_MASK)==RDF_MASK)  /*data overflow interrupt recieived*/ 
  	  { 
	 
	    	read_xmodemdata();
	  	 
          }
                         
	  
}

void  read_data()
{
   char data;	
         data = *SD_RECEIVE_DATA; 
	 if(!read_status)
	    {
	 	if(read_count >29)
			read_count=0;
	 	read_buffer[read_count]=data;
		read_count++;
		if(data == 0x08)
		    echo_backspace();
		else
		    echo_char(data);	

			  
	     }	
		 
	 	*SD_STATUS1_REG &= RDF_CLR; 

}

void  read_xmodemdata()
{
   	 char data;	
         data = *SD_RECEIVE_DATA; 
	 if(!read_status)
	    {
		read_buffer[0]=data;
		read_status = 1;
			  
	     }	
		 
	 	*SD_STATUS1_REG &= RDF_CLR; 

}
int get_character()
{
    int a=0x5aa5;
        if (read_status)
	{	               
        	a = (int)read_buffer[0];
        	read_status=0;
	        
	}
	
	 return a;
	 
		   

}


void  put_character( char data)
{
	char p[2];
	int count = 0,flag = 1;
	int ftillhere = 0;
	p[0] =data;
	p[1] = '\0';
	while ((count<=8) && ((*SD_STATUS1_REG & TDFE_CHECK) == TDFE_CHECK) && flag == 1)
	{
		if(count== 8  )
		{
			ftillhere+=8;
			count=0;
			*SD_STATUS1_REG &= ~TDFE_CHECK;
			while((*SD_STATUS1_REG & TDFE_CHECK) != TDFE_CHECK);
		}
		if(*(p + count + ftillhere)=='\0')
		{
			for (;;)
			{ if((*SD_STATUS1_REG & TEND_CHECK) == TEND_CHECK)
				break;
			}	
			flag = 0;  // End of buffer
			
		}
		
		if(flag)
			*SD_TRANSMIT_DATA =*(p + count + ftillhere);
		count++;
	}
}
    
int charcount=0;
char  get_char()
{
   int  ch;

   while(0x5aa5 == (ch = get_character()))
           ;
   if(0x08 == ch)
   {

	   charcount--;
	   if(charcount< 0) //1
		charcount = -1; //0   
	   if(charcount>=0) // >0
	      echo_backspace();

   }
   else
	   echo_char((char)ch);

        return ch;

}

int  get_string(char *pstring)
{
    char  ch;
    char ccount=0;
    char *ptemp;
    ptemp = pstring;

    while(1)
    {
	   
		    
	       while(0x5aa5 == ( ch = get_char() ) )
	             ;
		      
              *ptemp++ =(char) ch;
	      ccount++;
   	      charcount++;
	      if(ccount >= 32) //buffer overflow
		      return 0;
	      if(0x0D == ch)
		{     
		  *(ptemp-1) = '\0';
		  charcount =0;
		  break;
		}
	      if(0x08 == ch)
	      {
		  ptemp= ptemp-2;
   	          charcount--;
	      }

    }
    return 1;
		    

}


void echo_char(char c)
{
  char p[3];	
  if (c == 0x0D)
     {
   	p[0]=0x0A;
	p[1]=0x0D;
	p[2]='\0';
        console_print_SH7615(p); /*End of read buffer transfer to application*/
     }
  else
     {
        if(1==pwflag)
          p[0] = '*';
	else	
	  p[0]= c;

        p[1]='\0';  
        console_print_SH7615(p);
     }	

}
void echo_backspace()
{
   char c;
   char p[2];
   p[0] = 0x08; // backspace
   p[1] = 0x20; //space
   p[2] = 0x08; //backspace
   p[3] = '\0';
   console_print_SH7615(p);
   
		
 
}
void do_error_handler()
{
	 if(*SD_STATUS1_REG & DR_MASK)  
		 {
		  read_data();
	         *SD_STATUS1_REG &= DR_CLR;
		 }
	 if(*SD_STATUS2_REG & ORER_MASK)
	         {
	       	 read_data();
	         *SD_STATUS2_REG &= ORER_CLR;
		 }

	 if (*SD_STATUS1_REG & BRK_MASK)
	         *SD_STATUS1_REG &= BRK_CLR;   
	 if (*SD_STATUS1_REG &(FER_MASK|PER_MASK))
	  	 *SCFCR1|=RFRST;
		 
	   *SD_STATUS1_REG &= ER_CLR;
          printk("\r\n receive error");
}


void console_init_SH7615()
{
	
	int i=0;

	/*	Clear TE (Transmit Enable) Bit and RE (Receive Enable) Bit           */
	*SD_CONTROL_REG &= (unsigned char)~(SD_TRANSMIT_ENABLE | SD_RECEIVE_ENABLE);
        
	/* 	Clear FIFO 							     */
		*SCFCR1 |= (unsigned char)SD_FIFO_CLR_ENABLE; 			     

	/* 	Set the Clock Mode  						     */
	*SD_CONTROL_REG |= (unsigned char)(SD_CLOCK_ENABLE_1 | SD_CLOCK_ENABLE_0);

	/* 	Set the Format 							     */
	*SD_MODE_REG |= (unsigned char)0x00;
 
	/* 	Set Baudrate 							     */
	*SD_BAUD_REG = (unsigned char)SD_BAUD_9600_25;

	/*	Wait for 1 bit interval 					     */
	for (i = 0; i <= 99; i++);

	/* 	Initalize FIFO							     */
		*SCFCR1 |= (unsigned char)SD_FIFO_INIT; 			     

    	/*	FIFO Clear Disable 						     */
	*SCFCR1 &= (unsigned char)SD_FIFO_CLR_DISABLE;

	*SD_PRIORITY_REG = SD_RXI_INTR_PRIORITY;
	*SD_VECTOR_L_REG = SD_RXERROR_VECTOR|SD_RXDATAFULL_VECTOR;
	*SD_VECTOR_M_REG = SD_RXBRK_VECTOR;


	/* 	Enable the transmission 					     */
	*SD_CONTROL_REG |= (unsigned char)(SD_TRANSMIT_ENABLE); 

	

}
void console_receive_init_SH7615()
{
	
	int i=0;

	/*	Clear TE (Transmit Enable) Bit and RE (Receive Enable) Bit           */
	*SD_CONTROL_REG &= (unsigned char)~(SD_TRANSMIT_ENABLE | SD_RECEIVE_ENABLE);
        
	/* 	Clear FIFO 							     */
		*SCFCR1 |= (unsigned char)SD_FIFO_CLR_ENABLE; 			     

	/* 	Set the Clock Mode  						     */
	*SD_CONTROL_REG |= (unsigned char)(SD_CLOCK_ENABLE_1 | SD_CLOCK_ENABLE_0);

	/* 	Set the Format 							     */
	*SD_MODE_REG |= (unsigned char)0x00;
 
	/* 	Set Baudrate 							     */
	*SD_BAUD_REG = (unsigned char)SD_BAUD_9600_25;

	/*	Wait for 1 bit interval 					     */
	for (i = 0; i <= 99; i++);

	/* 	Initalize FIFO							     */
		*SCFCR1 |= (unsigned char)SD_FIFO_INIT; 			     

    	/*	FIFO Clear Disable 						     */
	*SCFCR1 &= (unsigned char)SD_FIFO_CLR_DISABLE;

	*SD_PRIORITY_REG = SD_RXI_INTR_PRIORITY;
	*SD_VECTOR_L_REG = SD_RXERROR_VECTOR|SD_RXDATAFULL_VECTOR;
	*SD_VECTOR_M_REG = SD_RXBRK_VECTOR;


	/* 	Enable the transmission 					     */
	*SD_CONTROL_REG |= (unsigned char)(SD_TRANSMIT_ENABLE| SD_RECEIVE_ENABLE |
				SD_RECEIVE_INTR_ENABLE); 

	

}

