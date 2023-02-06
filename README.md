# M480BSP_USBD_Mass_Storage_SPI_Flash
 M480BSP_USBD_Mass_Storage_SPI_Flash

update @ 2023/02/06

1. Initial SPI0 , to drive W25Q32JV ( 32MBit = 4MB ) , with USB FS , to set MCU USB MSC with SPI flash storage 

	- FLASH_SS    D10     PA.3
		
	- FLASH_CLK   D13     PA.2
		
	- FLASH_MOSI  D11     PA.0
		
	- FLASH_MISO  D12     PA.1
		
	- FLASH_HOLD  D8      PA.5
		
	- FLASH_WP    D9      PA.4
		
2. depend on SPI flash size , need to modify define : SPI_FLASH_STORAGE_SIZE (SPI_Flash.h)

	- ex : W25Q32JV ( 32MBit = 4MB ) , modify SPI_FLASH_STORAGE_SIZE to (4*1024*1024)
	
3. below is LA READ MDID 

![image](https://github.com/released/M480BSP_USBD_Mass_Storage_SPI_Flash/main/LA_ReadMidDid.jpg)

below is log mesage 

![image](https://github.com/released/M480BSP_USBD_Mass_Storage_SPI_Flash/main/log.jpg)

below is WINDOWS file manager display , 

![image](https://github.com/released/M480BSP_USBD_Mass_Storage_SPI_Flash/main/windows.jpg)

