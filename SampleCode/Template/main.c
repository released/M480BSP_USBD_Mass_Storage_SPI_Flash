/*_____ I N C L U D E S ____________________________________________________*/
#include <stdio.h>
#include <string.h>
#include "NuMicro.h"

#include "project_config.h"

#include "massstorage.h"
#include "SPI_Flash.h"

/*_____ D E C L A R A T I O N S ____________________________________________*/

/*_____ D E F I N I T I O N S ______________________________________________*/
volatile uint32_t BitFlag = 0;
volatile uint32_t counter_tick = 0;

#define CRYSTAL_LESS        1
#define TRIM_INIT           (SYS_BASE+0x10C)
uint32_t u32TrimInit;
/*_____ M A C R O S ________________________________________________________*/


/*_____ F U N C T I O N S __________________________________________________*/

void tick_counter(void)
{
	counter_tick++;
}

uint32_t get_tick(void)
{
	return (counter_tick);
}

void set_tick(uint32_t t)
{
	counter_tick = t;
}

void compare_buffer(uint8_t *src, uint8_t *des, int nBytes)
{
    uint16_t i = 0;	
	
    for (i = 0; i < nBytes; i++)
    {
        if (src[i] != des[i])
        {
            printf("error idx : %4d : 0x%2X , 0x%2X\r\n", i , src[i],des[i]);
			set_flag(flag_error , ENABLE);
        }
    }

	if (!is_flag_set(flag_error))
	{
    	printf("%s finish \r\n" , __FUNCTION__);	
		set_flag(flag_error , DISABLE);
	}

}

void reset_buffer(void *dest, unsigned int val, unsigned int size)
{
    uint8_t *pu8Dest;
//    unsigned int i;
    
    pu8Dest = (uint8_t *)dest;

	#if 1
	while (size-- > 0)
		*pu8Dest++ = val;
	#else
	memset(pu8Dest, val, size * (sizeof(pu8Dest[0]) ));
	#endif
	
}

void copy_buffer(void *dest, void *src, unsigned int size)
{
    uint8_t *pu8Src, *pu8Dest;
    unsigned int i;
    
    pu8Dest = (uint8_t *)dest;
    pu8Src  = (uint8_t *)src;


	#if 0
	  while (size--)
	    *pu8Dest++ = *pu8Src++;
	#else
    for (i = 0; i < size; i++)
        pu8Dest[i] = pu8Src[i];
	#endif
}

void dump_buffer(uint8_t *pucBuff, int nBytes)
{
    uint16_t i = 0;
    
    printf("dump_buffer : %2d\r\n" , nBytes);    
    for (i = 0 ; i < nBytes ; i++)
    {
        printf("0x%2X," , pucBuff[i]);
        if ((i+1)%8 ==0)
        {
            printf("\r\n");
        }            
    }
    printf("\r\n\r\n");
}

void dump_buffer_hex(uint8_t *pucBuff, int nBytes)
{
    int     nIdx, i;

    nIdx = 0;
    while (nBytes > 0)
    {
        printf("0x%04X  ", nIdx);
        for (i = 0; i < 16; i++)
            printf("%02X ", pucBuff[nIdx + i]);
        printf("  ");
        for (i = 0; i < 16; i++)
        {
            if ((pucBuff[nIdx + i] >= 0x20) && (pucBuff[nIdx + i] < 127))
                printf("%c", pucBuff[nIdx + i]);
            else
                printf(".");
            nBytes--;
        }
        nIdx += 16;
        printf("\n");
    }
    printf("\n");
}


/*
	WP : PA4 (D9)
	HOLD : PA5 (D8)
*/
void SPIFlash_WP_HOLD_Init (void)
{
    SYS->GPA_MFPL = (SYS->GPA_MFPL & ~(SYS_GPA_MFPL_PA4MFP_Msk)) | (SYS_GPA_MFPL_PA4MFP_GPIO);
    SYS->GPA_MFPL = (SYS->GPA_MFPL & ~(SYS_GPA_MFPL_PA5MFP_Msk)) | (SYS_GPA_MFPL_PA5MFP_GPIO);
	
    GPIO_SetMode(PA, BIT4, GPIO_MODE_OUTPUT);
    GPIO_SetMode(PA, BIT5, GPIO_MODE_OUTPUT);	

	PA4 = 1;
	PA5 = 1;		
}

void usb_trim_process(void)
{
    if (((SYS->CSERVER & SYS_CSERVER_VERSION_Msk) == 0x1) && (CRYSTAL_LESS))
    {
        /* Start USB trim if it is not enabled. */
        if ((SYS->HIRCTCTL & SYS_HIRCTCTL_FREQSEL_Msk) != 1)
        {
            if(USBD->INTSTS & USBD_INTSTS_SOFIF_Msk)
            {
                /* Clear SOF */
                USBD->INTSTS = USBD_INTSTS_SOFIF_Msk;

                /* Re-enable crystal-less */
                SYS->HIRCTCTL = 0x1;
                SYS->HIRCTCTL |= SYS_HIRCTCTL_REFCKSEL_Msk;
            }
        }

        /* Disable USB Trim when error */
        if (SYS->HIRCTISTS & (SYS_HIRCTISTS_CLKERRIF_Msk | SYS_HIRCTISTS_TFAILIF_Msk))
        {
            /* Init TRIM */
            M32(TRIM_INIT) = u32TrimInit;

            /* Disable crystal-less */
            SYS->HIRCTCTL = 0;

            /* Clear error flags */
            SYS->HIRCTISTS = SYS_HIRCTISTS_CLKERRIF_Msk | SYS_HIRCTISTS_TFAILIF_Msk;

            /* Clear SOF */
            USBD->INTSTS = USBD_INTSTS_SOFIF_Msk;
        }
    }
}

void usb_msc_Init(void)
{
    /* initialize USBD */
    USBD_Open(&gsInfo, MSC_ClassRequest, NULL);
    USBD_SetConfigCallback(MSC_SetConfig);

    /* Endpoint configuration */
    MSC_Init();
    USBD_Start();

    SYS_UnlockReg();
    if (((SYS->CSERVER & SYS_CSERVER_VERSION_Msk) == 0x1) && (CRYSTAL_LESS))
    {
        /* Start USB trim */
        USBD->INTSTS = USBD_INTSTS_SOFIF_Msk;
        while((USBD->INTSTS & USBD_INTSTS_SOFIF_Msk) == 0);
        SYS->HIRCTCTL = 0x1;
        SYS->HIRCTCTL |= SYS_HIRCTCTL_REFCKSEL_Msk;
        /* Backup default trim */
        u32TrimInit = M32(TRIM_INIT);
    }

    NVIC_EnableIRQ(USBD_IRQn);

}

void masstorage_process(void)
{
    #if 1
    MSC_ProcessCmd();
    #else
    /* Start transaction */
    if (USBD_IS_ATTACHED()) 
    {
        USBD_Start();
        set_flag(flag_usb_attached  ,ENABLE);
        // printf("usb attached\r\n");
    }
    else
    {
        set_flag(flag_usb_attached  ,DISABLE);
        // printf("usb un-attached\r\n");

    }

    if (is_flag_set(flag_usb_attached) )
    {
        MSC_ProcessCmd(); 
        PH1 ^= 1;         
    }  
    #endif
}

void process(void)
{
    usb_trim_process();
    masstorage_process();

}

void TMR1_IRQHandler(void)
{
	// static uint32_t LOG = 0;

	
    if(TIMER_GetIntFlag(TIMER1) == 1)
    {
        TIMER_ClearIntFlag(TIMER1);
		tick_counter();

		if ((get_tick() % 1000) == 0)
		{
			// printf("%s : %4d\r\n",__FUNCTION__,LOG++);
			PH0 ^= 1;
		}

		if ((get_tick() % 50) == 0)
		{

		}	
    }
}


void TIMER1_Init(void)
{
    TIMER_Open(TIMER1, TIMER_PERIODIC_MODE, 1000);
    TIMER_EnableInt(TIMER1);
    NVIC_EnableIRQ(TMR1_IRQn);	
    TIMER_Start(TIMER1);
}

void UARTx_Process(void)
{
	uint8_t res = 0;
	res = UART_READ(UART0);

	if (res == 'x' || res == 'X')
	{
		NVIC_SystemReset();
	}

	if (res > 0x7F)
	{
		printf("invalid command\r\n");
	}
	else
	{
		switch(res)
		{
			case '1':
				break;

			case 'X':
			case 'x':
			case 'Z':
			case 'z':
				NVIC_SystemReset();		
				break;
		}
	}
}


void UART0_IRQHandler(void)
{
    if(UART_GET_INT_FLAG(UART0, UART_INTSTS_RDAINT_Msk | UART_INTSTS_RXTOINT_Msk))     /* UART receive data available flag */
    {
        while(UART_GET_RX_EMPTY(UART0) == 0)
        {
			UARTx_Process();
        }
    }

    if(UART0->FIFOSTS & (UART_FIFOSTS_BIF_Msk | UART_FIFOSTS_FEF_Msk | UART_FIFOSTS_PEF_Msk | UART_FIFOSTS_RXOVIF_Msk))
    {
        UART_ClearIntFlag(UART0, (UART_INTSTS_RLSINT_Msk| UART_INTSTS_BUFERRINT_Msk));
    }	
}

void UART0_Init(void)
{
    SYS_ResetModule(UART0_RST);

    /* Configure UART0 and set UART0 baud rate */
    UART_Open(UART0, 115200);

	/* Set UART receive time-out */
	UART_SetTimeoutCnt(UART0, 20);

	UART0->FIFO &= ~UART_FIFO_RFITL_4BYTES;
	UART0->FIFO |= UART_FIFO_RFITL_8BYTES;

	/* Enable UART Interrupt - */
	UART_ENABLE_INT(UART0, UART_INTEN_RDAIEN_Msk | UART_INTEN_TOCNTEN_Msk | UART_INTEN_RXTOIEN_Msk);
	
	NVIC_EnableIRQ(UART0_IRQn);

	#if (_debug_log_UART_ == 1)	//debug
	printf("\r\nCLK_GetCPUFreq : %8d\r\n",CLK_GetCPUFreq());
	printf("CLK_GetHXTFreq : %8d\r\n",CLK_GetHXTFreq());
	printf("CLK_GetLXTFreq : %8d\r\n",CLK_GetLXTFreq());	
	printf("CLK_GetPCLK0Freq : %8d\r\n",CLK_GetPCLK0Freq());
	printf("CLK_GetPCLK1Freq : %8d\r\n",CLK_GetPCLK1Freq());	

//    printf("Product ID 0x%8X\n", SYS->PDID);
	
	#endif
}

void Custom_Init(void)
{
	SYS->GPH_MFPL = (SYS->GPH_MFPL & ~(SYS_GPH_MFPL_PH0MFP_Msk)) | (SYS_GPH_MFPL_PH0MFP_GPIO);
	SYS->GPH_MFPL = (SYS->GPH_MFPL & ~(SYS_GPH_MFPL_PH1MFP_Msk)) | (SYS_GPH_MFPL_PH1MFP_GPIO);
	SYS->GPH_MFPL = (SYS->GPH_MFPL & ~(SYS_GPH_MFPL_PH2MFP_Msk)) | (SYS_GPH_MFPL_PH2MFP_GPIO);

	//EVM LED
	GPIO_SetMode(PH,BIT0,GPIO_MODE_OUTPUT);
	GPIO_SetMode(PH,BIT1,GPIO_MODE_OUTPUT);
	GPIO_SetMode(PH,BIT2,GPIO_MODE_OUTPUT);
	
}

void SYS_Init(void)
{
    uint32_t volatile i;
        
    /* Unlock protected registers */
    SYS_UnlockReg();

    /* Set XT1_OUT(PF.2) and XT1_IN(PF.3) to input mode */
    PF->MODE &= ~(GPIO_MODE_MODE2_Msk | GPIO_MODE_MODE3_Msk);

    CLK_EnableXtalRC(CLK_PWRCTL_HIRCEN_Msk);
    CLK_WaitClockReady(CLK_STATUS_HIRCSTB_Msk);

   CLK_EnableXtalRC(CLK_PWRCTL_HXTEN_Msk);
   CLK_WaitClockReady(CLK_STATUS_HXTSTB_Msk);

//    CLK_EnableXtalRC(CLK_PWRCTL_LIRCEN_Msk);
//    CLK_WaitClockReady(CLK_STATUS_LIRCSTB_Msk);

//    CLK_EnableXtalRC(CLK_PWRCTL_LXTEN_Msk);
//    CLK_WaitClockReady(CLK_STATUS_LXTSTB_Msk);

    /* Switch HCLK clock source to HXT */
    CLK_SetHCLK(CLK_CLKSEL0_HCLKSEL_HXT,CLK_CLKDIV0_HCLK(1));

    /* Set core clock as PLL_CLOCK from PLL */
    CLK_SetCoreClock(FREQ_192MHZ);

    /* Set PCLK0/PCLK1 to HCLK/2 */
    CLK->PCLKDIV = (CLK_PCLKDIV_APB0DIV_DIV2 | CLK_PCLKDIV_APB1DIV_DIV2);

    /* M480LD support crystal-less */
    if (((SYS->CSERVER & SYS_CSERVER_VERSION_Msk) == 0x1) && (CRYSTAL_LESS))
    {
        CLK->PWRCTL |= CLK_PWRCTL_HIRC48MEN_Msk;
        /* Select IP clock source */
        CLK->CLKSEL0 &= ~CLK_CLKSEL0_USBSEL_Msk;
    }
    else
    {
        /* Select IP clock source */
        CLK->CLKSEL0 |= CLK_CLKSEL0_USBSEL_Msk;
        CLK->CLKDIV0 = (CLK->CLKDIV0 & ~CLK_CLKDIV0_USBDIV_Msk) | CLK_CLKDIV0_USB(4);
    }

    /* Select USBD */
    SYS->USBPHY = (SYS->USBPHY & ~SYS_USBPHY_USBROLE_Msk) | SYS_USBPHY_USBEN_Msk | SYS_USBPHY_SBO_Msk;

    /* Enable IP clock */
    CLK_EnableModuleClock(USBD_MODULE);

    /* Set PA.12 ~ PA.14 to input mode */
    PA->MODE &= ~(GPIO_MODE_MODE12_Msk | GPIO_MODE_MODE13_Msk | GPIO_MODE_MODE14_Msk);
    SYS->GPA_MFPH &= ~(SYS_GPA_MFPH_PA12MFP_Msk|SYS_GPA_MFPH_PA13MFP_Msk|SYS_GPA_MFPH_PA14MFP_Msk|SYS_GPA_MFPH_PA15MFP_Msk);
    SYS->GPA_MFPH |= (SYS_GPA_MFPH_PA12MFP_USB_VBUS|SYS_GPA_MFPH_PA13MFP_USB_D_N|SYS_GPA_MFPH_PA14MFP_USB_D_P|SYS_GPA_MFPH_PA15MFP_USB_OTG_ID);

    /* Enable UART clock */
    CLK_EnableModuleClock(UART0_MODULE);
    /* Select UART clock source from HXT */
    CLK_SetModuleClock(UART0_MODULE, CLK_CLKSEL1_UART0SEL_HIRC, CLK_CLKDIV0_UART0(1));

    /* Set GPB multi-function pins for UART0 RXD and TXD */
    SYS->GPB_MFPH &= ~(SYS_GPB_MFPH_PB12MFP_Msk | SYS_GPB_MFPH_PB13MFP_Msk);
    SYS->GPB_MFPH |= (SYS_GPB_MFPH_PB12MFP_UART0_RXD | SYS_GPB_MFPH_PB13MFP_UART0_TXD);

    CLK_EnableModuleClock(TMR1_MODULE);
    CLK_SetModuleClock(TMR1_MODULE, CLK_CLKSEL1_TMR1SEL_HIRC, 0);


//    CLK_SetModuleClock(SPI0_MODULE, CLK_CLKSEL2_SPI0SEL_PCLK1, MODULE_NoMsk);
//    CLK_EnableModuleClock(SPI0_MODULE);

	/* 
		SPI0 flash , 
		SPI0_FLASH_CLK : PA.2
		SPI0_FLASH_MISO : PA.1
		SPI0_FLASH_MOSI : PA.0
		SPI0_FLAH_NSS0 : PA.3
	*/
//    SYS->GPA_MFPL &= ~(SYS_GPA_MFPL_PA3MFP_Msk | SYS_GPA_MFPL_PA2MFP_Msk| SYS_GPA_MFPL_PA1MFP_Msk| SYS_GPA_MFPL_PA0MFP_Msk);
//    SYS->GPA_MFPL |= SYS_GPA_MFPL_PA3MFP_SPI0_SS| SYS_GPA_MFPL_PA2MFP_SPI0_CLK| SYS_GPA_MFPL_PA1MFP_SPI0_MISO| SYS_GPA_MFPL_PA0MFP_SPI0_MOSI ;
	
    /* Update System Core Clock */
    /* User can use SystemCoreClockUpdate() to calculate SystemCoreClock. */
    SystemCoreClockUpdate();

    /* Lock protected registers */
    // SYS_LockReg();
}

/*
 * This is a template project for M480 series MCU. Users could based on this project to create their
 * own application without worry about the IAR/Keil project settings.
 *
 * This template application uses external crystal as HCLK source and configures UART0 to print out
 * "Hello World", users may need to do extra system configuration based on their system design.
 */

int main()
{
	unsigned int MidDid;
    uint8_t spi_temp[4]= {0xFF};

    SYS_Init();

	UART0_Init();
	Custom_Init();	
	TIMER1_Init();

    /*
        FLASH_SS    D10     PA.3
        FLASH_CLK   D13     PA.2
        FLASH_MOSI  D11     PA.0
        FLASH_MISO  D12     PA.1
        FLASH_HOLD  D8      PA.5
        FLASH_WP    D9      PA.4

    */

	SPIFlash_WP_HOLD_Init();
    SpiInit();

    /* Read MID & DID */
    MidDid = SpiReadMidDid();
    printf("\r\nMID and DID = 0x%4X\r\n", MidDid);

    if(SPI_FlashReadID(spi_temp)) 
    {
        printf("SPI FLASH Open success.(%X-%X-%X)\n",spi_temp[0],spi_temp[1],spi_temp[2]);
    } else 
    {
        printf("SPI FLASH Open failed. (%X-%X-%X)\n",spi_temp[0],spi_temp[1],spi_temp[2]);
        //        printf("SPI FLASH Open failed %X\n",u32SpiMidDid);
    }


    usb_msc_Init();

    /* Got no where to go, just loop forever */
    while(1)
    {
        process();

    }
}

/*** (C) COPYRIGHT 2016 Nuvoton Technology Corp. ***/
