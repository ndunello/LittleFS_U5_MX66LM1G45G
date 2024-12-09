/*
 * littlefs_test.h
 *
 *  Created on: Dec 9, 2024
 *      Author: alombardinilo
 */

#ifndef INC_LITTLEFS_TEST_H_
#define INC_LITTLEFS_TEST_H_

typedef enum
{
  MX66UW1G45G_TEST_SPI_MODE = 0,                 /*!< SPI Mode              */
  MX66UW1G45G_TEST_OPI_STR_MODE,                 /*!< Single Transfer Mode  */
  MX66UW1G45G_TEST_OPI_DTR_MODE,                 /*!< Dual Transfer Mode    */
} MX66UW1G45G_TestMode_t;
int32_t littlefs_test(MX66UW1G45G_TestMode_t mode);



#endif /* INC_LITTLEFS_TEST_H_ */
