/*
Copyright (c) 2012-2015 Ben Croston

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "c_gpio.h"
#include "common.h"


static int  mem_fd0;
static void* gpio_map0[GPIO_BANK];
static volatile unsigned* gpio0[GPIO_BANK];

static int  mem_fd4;
static void *grf_map;
static volatile unsigned *grf;

void short_wait(void)
{
    int i;

    for (i=0; i<150; i++) // wait 150 cycles
	{   
        asm volatile("nop");
    }
}

int setup(void)
{
	int i;
	for(i=0;i<GPIO_BANK;i++)
	{
		if ((mem_fd0 = open("/dev/mem", O_RDWR|O_SYNC) ) < 0) 
		{
		printf("can't open /dev/mem \n");
		return SETUP_DEVMEM_FAIL;
		}

		// mmap GPIO 
		gpio_map0[i] = mmap(
		NULL,             // Any adddress in our space will do 
		PAGE_SIZE,       // Map length 
		PROT_READ|PROT_WRITE, // Enable reading & writting to mapped memory 
		MAP_SHARED,       // Shared with other processes 
		mem_fd0,           // File to map 
		RK3128_GPIO(i)         //Offset to GPIO peripheral 
		);

		if ((uint32_t)gpio_map0[i] < 0)
		return SETUP_MMAP_FAIL;
		close(mem_fd0); // No need to keep mem_fd open after mmap 
		gpio0[i] = (volatile unsigned *)gpio_map0[i];
	}


	if((mem_fd4 = open("/dev/mem", O_RDWR|O_SYNC) ) < 0) 
	{
		printf("can't open /dev/mem \n");
		return SETUP_DEVMEM_FAIL;
	}

	// mmap GPIO 
	grf_map = mmap(
		NULL,             // Any adddress in our space will do 
		PAGE_SIZE,       // Map length 
		PROT_READ|PROT_WRITE, // Enable reading & writting to mapped memory 
		MAP_SHARED,       // Shared with other processes 
		mem_fd4,           // File to map 
		RK3128_GRF_PHYS         //Offset to GPIO peripheral 
		);
	
	if ((uint32_t)grf_map < 0)
		return SETUP_MMAP_FAIL;
	close(mem_fd4); // No need to keep mem_fd open after mmap
	grf = (volatile unsigned *)grf_map;
	return SETUP_OK;
}

void set_pullupdn(int gpio, int pud)
{

/*rk3368
	if(pud==PUD_DOWN)
	{
		*(grf+RK3368_GRF_GPIO1A_P/4+gpio/8) = (0x02<<((gpio%8)*2)) | (0x03<<(16+(gpio%8)*2));
	}
	else if (pud == PUD_UP)
	{
		*(grf+RK3368_GRF_GPIO1A_P/4+gpio/8) = (0x01<<((gpio%8)*2)) | (0x03<<(16+(gpio%8)*2));
	}
	else //pub==PUD_OFF
	{
		*(grf+RK3368_GRF_GPIO1A_P/4+gpio/8) = (0x03<<((gpio%8)*2)) | (0x03<<(16+(gpio%8)*2));
	}
*/		


}

int setup_gpio(int gpio, int direction, int pud)
{

	FILE * fd;
	int ret;
	char buf[200]={0};
	char gpiobuf[5]={0};
	sprintf(buf,"/sys/class/gpio/export");
	sprintf(gpiobuf,"%d",gpio);		
	fd = fopen(buf,"w");
	if(fd==0)
	{
		printf("failed to open /sys/class/gpio/export\n");
		return 0;
	}
	ret = fputs(gpiobuf,fd);
	fclose(fd);


	sprintf(buf,"/sys/class/gpio/gpio%d/direction",gpio);
	fd = fopen(buf,"w");
	if(fd==0)
	{
		printf("this gpio is set to other function\n");
		return 0;
	}
	if(direction==INPUT)
	{
		fputs("in",fd);
	}
	else
	{
		fputs("out",fd);
	}
	fclose(fd);
	return 1;
}

 
int gpio_function(int gpio)
{
	int value;
	int func=-1;
	switch(gpio)
	{
		//GPIO0
		//case 17 : value = ((*(grf+GRF_GPIO0A_IOMUX/4+gpio/8))>>((gpio%8)*2)) & 0x00000003;  break;
		case 17 : value =  0x00000003;  break;
		//GPIO5B
		case 160 : 
		case 161 :
		case 162 :
		case 163 :
			//printf("hjptestfor:value=0x%x\n",*(grf+GRF_GPIO5B_IOMUX/4));
			value = ((*(grf+GRF_GPIO5B_IOMUX/4))>>((gpio%8)*2)) & 0x00000003;
			switch(value)
			{
				case 0: func=GPIO;		break;
				case 1: func=SERIAL;	break;
				case 2: func=TS_XXXX;	break;
				case 3: func=RESERVED;	break;
				default: func=-1;		break;
			}
			break;
		case 164 :
		case 165 :
		case 166 :
		case 167 :
			//printf("hjptestfor:value=0x%x\n",*(grf+GRF_GPIO5B_IOMUX/4));
			value = ((*(grf+GRF_GPIO5B_IOMUX/4))>>((gpio%8)*2)) & 0x00000003;
			switch(value)
			{
				case 0: func=GPIO;		break;
				case 1: func=SPI;		break;
				case 2: func=TS_XXXX;	break;
				case 3: func=SERIAL;	break;
				default: func=-1;		break;
			}
			break;
		
		//GPIO5C
		case 168 : 
			//printf("hjptestfor:value=0x%x\n",*(grf+GRF_GPIO5C_IOMUX/4));
			value = ((*(grf+GRF_GPIO5C_IOMUX/4))>>((gpio%8)*2)) & 0x00000003;
			switch(value)
			{
				case 0: func=GPIO;		break;
				case 1: func=SPI;		break;
				case 2: func=TS_XXXX;	break;
				case 3: func=RESERVED;	break;
				default: func=-1;		break;
			}
			break;
		case 169 :
		case 170 :
		case 171 :
			//printf("hjptestfor:value=0x%x\n",*(grf+GRF_GPIO5C_IOMUX/4));
			value = ((*(grf+GRF_GPIO5C_IOMUX/4))>>((gpio%8)*2)) & 0x00000001;
			switch(value)
			{
				case 0: func=GPIO;		break;
				case 1: func=TS_XXXX;		break;
				default: func=-1;		break;
			}
			break;

		//GPIO6A
		case 184 : 
		case 185 :
		case 187 :
		case 188 :
			//printf("hjptestfor:value=0x%x\n",*(grf+GRF_GPIO6A_IOMUX/4));
			value = ((*(grf+GRF_GPIO6A_IOMUX/4))>>((gpio%8)*2)) & 0x00000001;	
			switch(value)
			{
				case 0: func=GPIO;		break;
				case 1: func=I2S;		break;
				default: func=-1;		break;
			}
			break;

		//GPIO7A7
		case 223 : 
			//printf("hjptestfor:value=0x%x\n",*(grf+GRF_GPIO7A_IOMUX/4));
			value = ((*(grf+GRF_GPIO7A_IOMUX/4))>>((gpio%8)*2)) & 0x00000003; 
			switch(value)
			{
				case 0: func=GPIO;		break;
				case 1: func=SERIAL;		break;
				case 2: func=GPS_MAG;	break;
				case 3: func=HSADCT;	break;
				default: func=-1;		break;
			}
			break;

		//GPIO7B
		case 224 : 
		case 225 : 
			//printf("hjptestfor:value=0x%x\n",*(grf+GRF_GPIO7B_IOMUX/4));
			value = ((*(grf+GRF_GPIO7B_IOMUX/4))>>((gpio%8)*2)) & 0x00000003;
			switch(value)
			{
				case 0: func=GPIO;		break;
				case 1: func=SERIAL;	break;
				case 2: func=GPS_MAG;	break;
				case 3: func=HSADCT;	break;
				default: func=-1;		break;
			}
			break;
		case 226 : 
			//printf("hjptestfor:value=0x%x\n",*(grf+GRF_GPIO7B_IOMUX/4));
			value = ((*(grf+GRF_GPIO7B_IOMUX/4))>>((gpio%8)*2)) & 0x00000003;
			switch(value)
			{
				case 0: func=GPIO;		break;
				case 1: func=SERIAL;	break;
				case 2: func=USB;		break;
				case 3: func=RESERVED;	break;
				default: func=-1;		break;
			}
			break;
		//GPIO7C
		case 233 : 
		case 234 : 
			//printf("hjptestfor:value=0x%x\n",*(grf+GRF_GPIO7CL_IOMUX/4));
			value = ((*(grf+GRF_GPIO7CL_IOMUX/4))>>((gpio%8)*2)) & 0x00000001;
			switch(value)
			{
				case 0: func=GPIO;		break;
				case 1: func=I2C;		break;
				default: func=-1;		break;
			}
			break;
		case 238 : 
			//printf("hjptestfor:value=0x%x\n",*(grf+GRF_GPIO7CH_IOMUX/4));
			value = ((*(grf+GRF_GPIO7CH_IOMUX/4))>>((gpio%8)*2)) & 0x00000003;
			switch(value)
			{
				case 0: func=GPIO;		break;
				case 1: func=SERIAL;	break;
				case 2: func=SERIAL;	break;
				case 3: func=PWM;		break;
				default: func=-1;		break;
			}
			break;
		case 239 : 
			//printf("hjptestfor:value=0x%x\n",*(grf+GRF_GPIO7CH_IOMUX/4));
			value = ((*(grf+GRF_GPIO7CH_IOMUX/4))>>((gpio%8)*2)) & 0x00000007;
			switch(value)
			{
				case 0:
					func=GPIO;		break;
				case 1: 
				case 2:
					func=SERIAL;		break;
				case 3:
					func=PWM;			break;
				case 4:
					func=HDMI;			break;
				case 5:
				case 6:
				case 7:
					func=RESERVED;		break;
				default: 
					func=-1;		break;
			}
			break;

		//GPIO8A
		case 251 : 
		case 254 :
		case 255 :
			//printf("hjptestfor:value=0x%x\n",*(grf+GRF_GPIO8A_IOMUX/4));
			value = ((*(grf+GRF_GPIO8A_IOMUX/4))>>((gpio%8)*2)) & 0x00000003;
			switch(value)
			{
				case 0: func=GPIO;		break;
				case 1: func=SPI;		break;
				case 2: func=SC_XXX;	break;
				case 3: func=RESERVED;	break;
				default: func=-1;		break;
			}
			break;
		case 252 : 
		case 253 :
			//printf("hjptestfor:value=0x%x\n",*(grf+GRF_GPIO8A_IOMUX/4));
			value = ((*(grf+GRF_GPIO8A_IOMUX/4))>>((gpio%8)*2)) & 0x00000003;  
			switch(value)
			{
				case 0: func=GPIO;		break;
				case 1: func=I2C;		break;
				case 2: func=SC_XXX;	break;
				case 3: func=RESERVED;	break;
				default: func=-1;		break;
			}
			break;

		//GPIO8B
		case 256 : 
		case 257 :
			//printf("hjptestfor:value=0x%x\n",*(grf+GRF_GPIO8B_IOMUX/4));
			value = ((*(grf+GRF_GPIO8B_IOMUX/4))>>((gpio%8)*2)) & 0x00000003;
			switch(value)
			{
				case 0: func=GPIO;		break;
				case 1: func=SPI;		break;
				case 2: func=SC_XXX;	break;
				case 3: func=RESERVED;	break;
				default: func=-1;		break;
			}
			break;
		default:
			func=-1; break;
	}
	//printf("hjptestfor:gpio=%d,func=%d\n",gpio,func);
	return func;
}

void output_gpio(int gpio, int value)
{

	if(0<gpio&&gpio<=24)
	{
		if(value==1)
		{
			*(gpio0[gpio/32]+GPIO_SWPORTA_DR_OFFSET/4) |= (1<<(gpio%32));
		}
		else
		{
			*(gpio0[gpio/32]+GPIO_SWPORTA_DR_OFFSET/4) &= ~(1<<(gpio%32));
		}
	}
	else
	{
		if(value==1)
		{
			*(gpio0[(gpio+8)/32]+GPIO_SWPORTA_DR_OFFSET/4) |= (1<<((gpio+8)%32));	
		}
		else
		{
			*(gpio0[(gpio+8)/32]+GPIO_SWPORTA_DR_OFFSET/4) &= ~(1<<((gpio+8)%32));
		}
	}
}

int input_gpio(int gpio)
{
	int value, mask;
	
	if(0<gpio&&gpio<=24)
	{
		mask = (1 << gpio%32);
		value = ((*(gpio0[gpio/32]+GPIO_EXT_PORTA_OFFSET/4)) & mask)>>(gpio%32);  
	}
	else
	{
		mask = (1 << (gpio+8)%32);
		value = ((*(gpio0[(gpio+8)/32]+GPIO_EXT_PORTA_OFFSET/4)) & mask)>>((gpio+8)%32);
	}

	//printf("input_gpio = %d,input value = %x\n",gpio,value);
	return value;
}

void cleanup(void)
{
	int i;
	for(i=0;i<GPIO_BANK;i++)
	{
    	munmap((caddr_t)gpio_map0[i], BLOCK_SIZE);
	}
	munmap((caddr_t)grf_map, BLOCK_SIZE);
	printf("cleanup\n");
}



