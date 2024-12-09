/**
******************************************************************************
* @file    Examples/BSP/Src/HSPI_nor.c
* @author  MCD Application Team
* @brief   This example code shows how to use the HSPI Driver
******************************************************************************
* @attention
*
* Copyright (c) 2022 STMicroelectronics.
* All rights reserved.
*
* This software is licensed under terms that can be found in the LICENSE file
* in the root directory of this software component.
* If no LICENSE file comes with this software, it is provided AS-IS.
*
******************************************************************************
*/

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32u5g9j_discovery_hspi.h"
#include "mx66uw1g45g.h"
#include "littlefs_test.h"

/** @addtogroup STM32U5xx_HAL_Examples
* @{
*/

/** @addtogroup BSP
* @{
*/

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
//#define TEST_HSPI_NOR_FLASH
#if defined TEST_HSPI_NOR_FLASH
#define TEST_HSPI_NOR_FLASH_HW 1
#else
#include "lfs.h"
// variables used by the filesystem
lfs_t lfs;
lfs_file_t file;
uint8_t doTest = 0;

int user_provided_block_device_sync(const struct lfs_config *c);
int user_provided_block_device_erase(const struct lfs_config *c, lfs_block_t block);
int user_provided_block_device_prog(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size);
int user_provided_block_device_read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size);

// configuration of the filesystem is provided by this struct
const struct lfs_config cfg = {
    // block device operations
    .read  = user_provided_block_device_read,
    .prog  = user_provided_block_device_prog,
    .erase = user_provided_block_device_erase,
    .sync  = user_provided_block_device_sync,

    // block device configuration
    .read_size = 4096,
    .prog_size = 4096,
    .block_size = 4096,
    .block_count = 32768,
    .cache_size = 4096,
    .lookahead_size = 4096,
    //.block_cycles = 512,
	.block_cycles = -1,
};

// Read a region in a block. Negative error codes are propagated to the user.
int user_provided_block_device_read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size)
{
	uint32_t readAddr;

	readAddr = (c->block_size * block) + off;
	//sFLASH_ReadBuffer(buffer, readAddr, size);
	//printf("LSF READ: addr=%d, size=%d\n\r", readAddr, size);
	if(BSP_HSPI_NOR_Read(0, buffer, readAddr, size) != BSP_ERROR_NONE)
	{
		printf("Read : Failed\r\n");
	}
	return 0;
}

// Program a region in a block. The block must have previously
// been erased. Negative error codes are propagated to the user.
// May return LFS_ERR_CORRUPT if the block should be considered bad.
int user_provided_block_device_prog(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size)
{
	uint32_t writeAddr;

	writeAddr = (c->block_size * block) + off;
	//sFLASH_WriteBuffer((uint8_t*)buffer, writeAddr, size);
	//printf("LSF WRITE: addr=%d, size=%d\n\r", writeAddr, size);
	if(BSP_HSPI_NOR_Write(0, (uint8_t*)buffer, writeAddr, size) != BSP_ERROR_NONE)
	{
		printf("Write : Failed\r\n");
	}
	return 0;
}

// Erase a block. A block must be erased before being programmed.
// The state of an erased block is undefined. Negative error codes
// are propagated to the user.
// May return LFS_ERR_CORRUPT if the block should be considered bad.
int user_provided_block_device_erase(const struct lfs_config *c, lfs_block_t block)
{
	uint32_t eraseAddr;

	eraseAddr = (c->block_size * block);

	//sFLASH_EraseSector(eraseAddr);
	//printf("LSF ERASE: addr=%d\n\r", eraseAddr);
	if(BSP_HSPI_NOR_Erase_Block(0, eraseAddr, MX66UW1G45G_ERASE_4K) != BSP_ERROR_NONE)
	{
		printf("Erase Sector 4K: Failed\r\n");
	}
	return 0;
}


// Sync the state of the underlying block device. Negative error codes
// are propagated to the user.
int user_provided_block_device_sync(const struct lfs_config *c)
{
	return 0;
}

int lfs_ls(lfs_t *lfs, const char *path) {
    lfs_dir_t dir;
    int err = lfs_dir_open(lfs, &dir, path);
    if (err) {
        return err;
    }

    struct lfs_info info;
    while (true) {
        int res = lfs_dir_read(lfs, &dir, &info);
        if (res < 0) {
            return res;
        }

        if (res == 0) {
            break;
        }

        switch (info.type) {
            case LFS_TYPE_REG: printf("reg "); break;
            case LFS_TYPE_DIR: printf("dir "); break;
            default:           printf("?   "); break;
        }

        static const char *prefixes[] = {"", "K", "M", "G"};
        for (int i = sizeof(prefixes)/sizeof(prefixes[0])-1; i >= 0; i--) {
            if (info.size >= (1 << 10*i)-1) {
                printf("%*u%sB ", 4-(i != 0), info.size >> 10*i, prefixes[i]);
                break;
            }
        }

        printf("%s\r\n", info.name);
    }

    err = lfs_dir_close(lfs, &dir);
    if (err) {
        return err;
    }

    return 0;
}
#endif

#define HSPI_NOR_BUFFER_SIZE     ((uint32_t)0x0200)
#define HSPI_NOR_WRITE_READ_ADDR ((uint32_t)0x0050)
#define HSPI_NOR_BASE_ADDR       ((uint32_t)0xA0000000)

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
uint8_t HSPI_nor_aTxBuffer[HSPI_NOR_BUFFER_SIZE];
uint8_t HSPI_nor_aRxBuffer[MX66UW1G45G_BLOCK_4K];

/* External variables --------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
static void     Fill_Buffer (uint8_t *pBuffer, uint32_t uwBufferLength, uint32_t uwOffset);
static uint8_t  Buffercmp   (uint8_t* pBuffer1, uint8_t* pBuffer2, uint32_t BufferLength);
static uint8_t  DataCmp     (uint8_t* pBuffer, uint8_t Pattern, uint32_t BufferLength);
BSP_HSPI_NOR_Info_t Hspi_NOR_Info = {0};
BSP_HSPI_NOR_Init_t Hspi_NOR_Init;
/* Private functions ---------------------------------------------------------*/

/**
* @brief  HSPI NOR Demo
* @param  None
* @retval None
*/
#if defined TEST_HSPI_NOR_FLASH_HW
int32_t littlefs_test(MX66UW1G45G_TestMode_t mymode)
{
	/* HSPI info structure */
	int32_t status;
	__IO uint8_t *data_ptr;
	uint32_t index;
	uint8_t mode;

	/* Demo Hint */
	printf("---------- TEST STARTED ----------\r\n");

	//for (mode = 0; mode < 3; mode++)
	for (mode = 0; mode < 1; mode++)
	{
		/********************************** PART 1 **********************************/

		/*##-1- Configure the HSPI NOR device ######################################*/
		/* HSPI NOR device configuration */
		if (mode == 0)
		{
			/*-------- SPI mode --------*/
			printf("     ***** SPI mode *****\r\n");
			Hspi_NOR_Init.InterfaceMode = BSP_HSPI_NOR_SPI_MODE;
			Hspi_NOR_Init.TransferRate  = BSP_HSPI_NOR_STR_TRANSFER;
		}
		else if (mode == 1)
		{
			/* -------- OPI STR mode -------- */
			printf("\r\n     ***** OPI STR mode *****\r\n");
			Hspi_NOR_Init.InterfaceMode = BSP_HSPI_NOR_OPI_MODE;
			Hspi_NOR_Init.TransferRate  = BSP_HSPI_NOR_STR_TRANSFER;
		}
		else
		{
			/*-------- OPI DTR mode --------*/
			printf("\r\n     ***** OPI DTR mode *****\r\n");
			Hspi_NOR_Init.InterfaceMode = BSP_HSPI_NOR_OPI_MODE;
			Hspi_NOR_Init.TransferRate  = BSP_HSPI_NOR_DTR_TRANSFER;
		}

		/*##-1- Read & check the HSPI NOR info ###################################*/
		/* Read the HSPI NOR memory info */
		if(BSP_HSPI_NOR_GetInfo(0,&Hspi_NOR_Info) != BSP_ERROR_NONE)
		{
			Error_Handler();
		}
		else if((Hspi_NOR_Info.FlashSize         != MX66UW1G45G_FLASH_SIZE)   ||
			(Hspi_NOR_Info.EraseSectorSize       != MX66UW1G45G_BLOCK_64K)    ||
			(Hspi_NOR_Info.EraseSectorsNumber    != (MX66UW1G45G_FLASH_SIZE / MX66UW1G45G_BLOCK_64K)) ||
			(Hspi_NOR_Info.EraseSubSectorSize    != MX66UW1G45G_BLOCK_4K) ||
			(Hspi_NOR_Info.EraseSubSectorNumber  != (MX66UW1G45G_FLASH_SIZE / MX66UW1G45G_BLOCK_4K))    ||
			(Hspi_NOR_Info.EraseSubSector1Size   != MX66UW1G45G_BLOCK_4K)   ||
			(Hspi_NOR_Info.EraseSubSector1Number != (MX66UW1G45G_FLASH_SIZE / MX66UW1G45G_BLOCK_4K)) ||
			(Hspi_NOR_Info.ProgPageSize          != MX66UW1G45G_PAGE_SIZE) ||
			(Hspi_NOR_Info.ProgPagesNumber       != (MX66UW1G45G_FLASH_SIZE / MX66UW1G45G_PAGE_SIZE)))
		{
			/* Test the correctness */
			printf("GET INFO : FAILED\r\n");
			printf("Test Aborted\r\n");
			break;
		}

		status = BSP_HSPI_NOR_Init(0, &Hspi_NOR_Init);
		printf("--> BSP_HSPI_NOR_Init #1\r\n");
		if (status != BSP_ERROR_NONE)
		{
			Error_Handler();
		}
		else
		{
			/*##-2- Erase HSPI NOR memory ##########################################*/
			if(BSP_HSPI_NOR_Erase_Block(0, HSPI_NOR_WRITE_READ_ADDR, MX66UW1G45G_ERASE_64K) != BSP_ERROR_NONE)
			{
				printf("Erase Block 64 K: Failed\r\n");
				printf("Test Aborted\r\n");
				break;
			}
			else
			{
				printf("Erase Block 64 K: OK\r\n");

				/*##-4- HSPI NOR memory read/write access  ###########################*/
				/* Fill the buffer to write */
				Fill_Buffer(HSPI_nor_aTxBuffer, HSPI_NOR_BUFFER_SIZE, 0xD20F);

				/* Write data to the HSPI NOR memory */
				if(BSP_HSPI_NOR_Write(0, HSPI_nor_aTxBuffer, HSPI_NOR_WRITE_READ_ADDR, HSPI_NOR_BUFFER_SIZE) != BSP_ERROR_NONE)
				{
					printf("Write : Failed\r\n");
					printf("Test Aborted\r\n");
					break;
				}
				else
				{
					/* Read back data from the HSPI NOR memory */
					if(BSP_HSPI_NOR_Read(0, HSPI_nor_aRxBuffer, HSPI_NOR_WRITE_READ_ADDR, HSPI_NOR_BUFFER_SIZE) != BSP_ERROR_NONE)
					{
						printf("Read : Failed\r\n");
						printf("Test Aborted\r\n");
						break;
					}
					else
					{
						printf("Read : OK\r\n");

						/*##-5- Checking data integrity ##################################*/
						uint16_t error = 0;
						for (uint16_t index = 0; index < HSPI_NOR_BUFFER_SIZE; index++)
						{
							if (HSPI_nor_aRxBuffer[index] != HSPI_nor_aTxBuffer[index])
							{
								error ++;
							}
						}

						if(error > 0)
						{
							if(Buffercmp(HSPI_nor_aRxBuffer, HSPI_nor_aTxBuffer, HSPI_NOR_BUFFER_SIZE) > 0)
							{
								printf("Compare : Failed\r\n");
								printf("Test Aborted\r\n");
								break;
							}
							else
							{
								printf("Compare : OK\r\n");

								/*##-6- HSPI NOR memory in memory-mapped mode###################*/
								if(BSP_HSPI_NOR_EnableMemoryMappedMode(0) != BSP_ERROR_NONE)
								{
									printf("Mem-Mapped Cfg : Failed\r\n");
									printf("Test Aborted\r\n");
									break;
								}
								else
								{
									for(index = 0, data_ptr = (__IO uint8_t *)(HSPI_NOR_BASE_ADDR + HSPI_NOR_WRITE_READ_ADDR);
													index < HSPI_NOR_BUFFER_SIZE; index++, data_ptr++)
									{
										if(*data_ptr != HSPI_nor_aTxBuffer[index])
										{
											printf("Mem-Mapped Access : Failed\r\n");
											printf("Test Aborted\r\n");
											break;
										}
										else
										{
											printf("Mem-Mapped Access : OK\r\n");
										}
									}
								}
							}
						}
					}
				}
			}
		}
		/********************************** PART 2 **********************************/

		/*##-1- Deconfigure the HSPI NOR device ####################################*/
		status = BSP_HSPI_NOR_DeInit(0);
		printf("--> BSP_HSPI_NOR_DeInit #1\r\n");
		if (status != BSP_ERROR_NONE)
		{
			Error_Handler();
		}
		else
		{
			/*##-2- Reconfigure the HSPI NOR device ##################################*/
			/* QSPI device configuration */
			status = BSP_HSPI_NOR_Init(0, &Hspi_NOR_Init);
			printf("--> BSP_HSPI_NOR_Init #2\r\n");
			if (status != BSP_ERROR_NONE)
			{
				Error_Handler();
			}
			else
			{
				/*##-3- Erase HSPI NOR memory ##########################################*/
				if(BSP_HSPI_NOR_Erase_Block(0, 0, MX66UW1G45G_ERASE_4K) != BSP_ERROR_NONE)
				{
					printf("Erase Sector 4K: Failed\r\n");
					printf("Test Aborted\r\n");
					break;
				}
				else
				{
					/*##-4- Suspend erase HSPI NOR memory ################################*/
					if(BSP_HSPI_NOR_SuspendErase(0) != BSP_ERROR_NONE)
					{
						printf("Erase Suspend : Failed\r\n");
						printf("Test Aborted\r\n");
						break;
					}
					else
					{
						printf("Erase Suspend : OK\r\n");

						/*##-6- Resume erase HSPI NOR memory ###############################*/
						if(BSP_HSPI_NOR_ResumeErase(0) != BSP_ERROR_NONE)
						{
							printf("Erase Resume : Failed\r\n");
							printf("Test Aborted\r\n");
							break;
						}
						else
						{
							printf("Erase Resume : OK\r\n");

							/*##-7- Check HSPI NOR memory status  ############################*/
							/* Wait the end of the current operation on memory side */
							do
							{
								status = BSP_HSPI_NOR_GetStatus(0);
							} while((status != BSP_ERROR_NONE) && (status != BSP_ERROR_COMPONENT_FAILURE));

							if(status != BSP_ERROR_NONE)
							{
								printf("Memory Status : Failed\r\n");
								printf("Test Aborted\r\n");
								break;
							}
							else
							{
								/*##-8- HSPI NOR memory read access  ###########################*/
								/* Read back data from the HSPI NOR memory */
								if(BSP_HSPI_NOR_Read(0, HSPI_nor_aRxBuffer, 0, MX66UW1G45G_BLOCK_4K) != BSP_ERROR_NONE)
								{
									printf("Read : Failed\r\n");
									printf("Test Aborted\r\n");
									break;
								}
								else
								{
									printf("Read : OK\r\n");

									/*##-9- Checking data integrity ##############################*/
									if(DataCmp(HSPI_nor_aRxBuffer, 0xFF, MX66UW1G45G_BLOCK_4K) > 0)
									{
										printf("Compare : Failed\r\n");
										printf("Test Aborted\r\n");
										break;
									}
									else
									{
										printf("Compare : OK\r\n");
									}
								}
							}
						}
					}
				}
			}
		}
		/********************************** PART 3 **********************************/

		/*##-1- Deconfigure the HSPI NOR device ####################################*/
		status = BSP_HSPI_NOR_DeInit(0);
		printf("--> BSP_HSPI_NOR_DeInit #2\r\n");
		if (status != BSP_ERROR_NONE)
		{
		  Error_Handler();
		}
		else
		{
			/*##-2- Reconfigure the HSPI NOR device ##################################*/
			/* HSPI NOR device configuration */
			status = BSP_HSPI_NOR_Init(0, &Hspi_NOR_Init);
			printf("--> BSP_HSPI_NOR_Init #3\r\n");
			if (status != BSP_ERROR_NONE)
			{
				Error_Handler();
			}
			else
			{
				/*##-3- Erase HSPI NOR memory ##########################################*/
				if(BSP_HSPI_NOR_Erase_Block(0, HSPI_NOR_WRITE_READ_ADDR, MX66UW1G45G_ERASE_4K) != BSP_ERROR_NONE)
				{
					printf("Erase Sector 4K: Failed\r\n");
					printf("Test Aborted\r\n");
					break;
				}
				else
				{
					printf("Erase Sector 4K: OK\r\n");

					/*##-4- HSPI NOR memory write access #################################*/
					/* Fill the buffer to write */
					Fill_Buffer(HSPI_nor_aTxBuffer, HSPI_NOR_BUFFER_SIZE, 0xD20F);

					/* Write data to the HSPI NOR memory */
					if(BSP_HSPI_NOR_Write(0, HSPI_nor_aTxBuffer, HSPI_NOR_WRITE_READ_ADDR, HSPI_NOR_BUFFER_SIZE) != BSP_ERROR_NONE)
					{
						printf("Write : Failed\r\n");
						printf("Test Aborted\r\n");
						break;
					}
					else
					{
						printf("Write : OK\r\n");

						/*##-5- HSPI NOR memory in memory-mapped mode#######################*/
						if(BSP_HSPI_NOR_EnableMemoryMappedMode(0) != BSP_ERROR_NONE)
						{
							printf("Mem-Mapped Cfg : Failed\r\n");
							printf("Test Aborted\r\n");
							break;
						}
						else
						{
							for(index = 0, data_ptr = (__IO uint8_t *)(HSPI_NOR_BASE_ADDR + HSPI_NOR_WRITE_READ_ADDR);
											index < HSPI_NOR_BUFFER_SIZE; index++, data_ptr++)
							{
								if(*data_ptr != HSPI_nor_aTxBuffer[index])
								{
									printf("Mem-Mapped Access : Failed\r\n");
									printf("Test Aborted\r\n");
									break;
								}
							}

							if(index == HSPI_NOR_BUFFER_SIZE)
							{
								printf("Mem-Mapped Access : OK\r\n");
							}
						}
					}
				}
			}
		}
		/* De-initialization in order to have correct configuration memory on next try */
		BSP_HSPI_NOR_DeInit(0);
		printf("--> BSP_HSPI_NOR_DeInit #3\r\n");
	}
	printf("---------- TEST ENDED ----------\r\n");
	return 0;
}
#else
int32_t littlefs_test(MX66UW1G45G_TestMode_t mymode)
{
	uint8_t flashID[3];
	uint8_t statiticFileName[20];
	uint8_t statiticData[1024];
	uint32_t file_count = 0;
	int32_t status;
	int err;
#if 0
	/* 1 - Verify that the identifier is correctly read */
	flashID = sFLASH_ReadID();

	if(flashID != sFLASH_IS25LP032D_JDEC_ID)
	{
		printf("WARNING: Statistic Flash IS25LP032D wrong ID 0x%06x\r\n", (unsigned int)flashID);
	}
#endif

	Hspi_NOR_Init.InterfaceMode = BSP_HSPI_NOR_SPI_MODE;
	Hspi_NOR_Init.TransferRate  = BSP_HSPI_NOR_STR_TRANSFER;
	if(BSP_HSPI_NOR_GetInfo(0,&Hspi_NOR_Info) != BSP_ERROR_NONE)
	{
		Error_Handler();
	}
	else if((Hspi_NOR_Info.FlashSize         != MX66UW1G45G_FLASH_SIZE)   ||
		(Hspi_NOR_Info.EraseSectorSize       != MX66UW1G45G_BLOCK_64K)    ||
		(Hspi_NOR_Info.EraseSectorsNumber    != (MX66UW1G45G_FLASH_SIZE / MX66UW1G45G_BLOCK_64K)) ||
		(Hspi_NOR_Info.EraseSubSectorSize    != MX66UW1G45G_BLOCK_4K) ||
		(Hspi_NOR_Info.EraseSubSectorNumber  != (MX66UW1G45G_FLASH_SIZE / MX66UW1G45G_BLOCK_4K))    ||
		(Hspi_NOR_Info.EraseSubSector1Size   != MX66UW1G45G_BLOCK_4K)   ||
		(Hspi_NOR_Info.EraseSubSector1Number != (MX66UW1G45G_FLASH_SIZE / MX66UW1G45G_BLOCK_4K)) ||
		(Hspi_NOR_Info.ProgPageSize          != MX66UW1G45G_PAGE_SIZE) ||
		(Hspi_NOR_Info.ProgPagesNumber       != (MX66UW1G45G_FLASH_SIZE / MX66UW1G45G_PAGE_SIZE)))
	{
		/* Test the correctness */
		printf("GET INFO : FAILED\r\n");
		printf("Test Aborted\r\n");
		return(-1);
	}

	status = BSP_HSPI_NOR_Init(0, &Hspi_NOR_Init);
	printf("--> BSP_HSPI_NOR_Init #1\r\n");
	if (status != BSP_ERROR_NONE)
	{
		Error_Handler();
	}

	if(BSP_HSPI_NOR_ReadID(0, flashID) != BSP_ERROR_NONE)
	{
		Error_Handler();
	}
	printf("Statistic Flash MX66LM1G45G [0x%06x]\r\n", (flashID[0] << 16) | (flashID[1] << 8) | flashID[2]);

	// mount the filesystem
	err = lfs_mount(&lfs, &cfg);

	// reformat if we can't mount the filesystem
	// this should only happen on the first boot
	if (err) {
		printf("lfs mount error!\r\n");
		lfs_format(&lfs, &cfg);
		printf("lfs format!\r\n");
		lfs_mount(&lfs, &cfg);
		printf("lfs mount!\r\n");
	}

	for(;;)
	{
		if(doTest == 1)
		{
			printf("StatisticFlashTestTask write file\r\n");

			lfs_file_open(&lfs, &file, "file_count", LFS_O_RDWR | LFS_O_CREAT);
			lfs_file_read(&lfs, &file, &file_count, sizeof(file_count));
			file_count++;
			lfs_file_rewind(&lfs, &file);
			lfs_file_write(&lfs, &file, &file_count, sizeof(file_count));
			lfs_file_close(&lfs, &file);

			sprintf(statiticFileName, "Statistic_%d", file_count);
			lfs_file_open(&lfs, &file, statiticFileName, LFS_O_RDWR | LFS_O_CREAT);
			lfs_file_read(&lfs, &file, &statiticData, sizeof(statiticData));
			memset(statiticData, file_count, sizeof(statiticData));
			lfs_file_rewind(&lfs, &file);
			lfs_file_write(&lfs, &file, &statiticData, sizeof(statiticData));
			lfs_file_close(&lfs, &file);

			printf("Wrote file %s\r\n", statiticFileName);

			//lfs_ls(&lfs, "/");

			doTest = 0;
		}
		else
		{
			printf("StatisticFlashTestTask idle\r\n");
			lfs_ls(&lfs, "/");
			//osDelay(2000);
			HAL_Delay(2000);
			doTest = 1;

		}
	}
}
#endif


/**
* @brief  Fills buffer with user predefined data.
* @param  pBuffer: pointer on the buffer to fill
* @param  uwBufferLenght: size of the buffer to fill
* @param  uwOffset: first value to fill on the buffer
* @retval None
*/
static void Fill_Buffer(uint8_t *pBuffer, uint32_t uwBufferLenght, uint32_t uwOffset)
{
  uint32_t tmpIndex = 0;

  /* Put in global buffer different values */
  for (tmpIndex = 0; tmpIndex < uwBufferLenght; tmpIndex++ )
  {
    pBuffer[tmpIndex] = tmpIndex + uwOffset;
  }
}

/**
* @brief  Compares two buffers.
* @param  pBuffer1, pBuffer2: buffers to be compared.
* @param  BufferLength: buffer's length
* @retval 1: pBuffer identical to pBuffer1
*         0: pBuffer differs from pBuffer1
*/
static uint8_t Buffercmp(uint8_t* pBuffer1, uint8_t* pBuffer2, uint32_t BufferLength)
{
  while (BufferLength--)
  {
    if (*pBuffer1 != *pBuffer2)
    {
      return 1;
    }

    pBuffer1++;
    pBuffer2++;
  }

  return 0;
}

/**
* @brief  Compares a buffer with data.
* @param  pBuffer, pBuffer2: buffers to be compared.
* @param  Pattern: data reference pattern.
* @param  BufferLength: buffer's length
* @retval 1: pBuffer identical to pBuffer1
*         0: pBuffer differs from pBuffer1
*/
static uint8_t DataCmp(uint8_t* pBuffer, uint8_t Pattern, uint32_t BufferLength)
{
  while (BufferLength--)
  {
    if (*pBuffer != Pattern)
    {
      return 1;
    }

    pBuffer++;
  }

  return 0;
}

/**
* @brief  Compares two buffers.
* @param  pBuffer1, pBuffer2: buffers to be compared.
* @param  BufferLength: buffer's length
* @retval 1: pBuffer identical to pBuffer1
*         0: pBuffer differs from pBuffer1
*/
void littlefs(void)
{

}
/**
* @}
*/

/**
* @}
*/
