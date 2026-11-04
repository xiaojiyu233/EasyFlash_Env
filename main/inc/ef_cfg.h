/*
 * This file is part of the EasyFlash Library.
 *
 * Copyright (c) 2015-2019, Armink, <armink.ztl@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * 'Software'), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Function: It is the configure head file for this library.
 * Created on: 2015-07-14
 */

#ifndef EF_CFG_H_
#define EF_CFG_H_

/* using ENV function, default is NG (Next Generation) mode start from V4.0 */
#define EF_USING_ENV 1

#ifdef EF_USING_ENV
/* Auto update ENV to latest default when current ENV version number is changed.
 */
#define EF_ENV_AUTO_UPDATE 1
/**
 * ENV version number defined by user.
 * Please change it when your firmware add a new ENV to default_env_set.
 */
// TIPS: 老设备升级后就会自动补上新增的默认项，不会覆盖用户已经改过的值
#define EF_ENV_VER_NUM 0

/* MCU Endian Configuration, default is Little Endian Order. */
/* #define EF_BIG_ENDIAN  */

#endif /* EF_USING_ENV */

/* using IAP function */
// #define EF_USING_IAP

/* using save log function */
// #define EF_USING_LOG

/* The minimum size of flash erasure. May be a flash sector size. */
#define EF_ERASE_MIN_SIZE (16 * 1024)

/* the flash write granularity, unit: bit
 * only support 1(nor flash)/ 8(stm32f4)/ 32(stm32f1) */
#define EF_WRITE_GRAN 8

/* The size of read_env and continue_ff_addr function used*/
#define EF_READ_BUF_SIZE 32

/* backup area start address */
#define EF_START_ADDR 0x08008000

/* ENV area size. It's at least one empty sector for GC. So it's definition must
 * more then or equal 2 flash sector size. */
#define ENV_AREA_SIZE (32 * 1024)

/* saved log area size */
// #define LOG_AREA_SIZE

/* print debug information of flash */
// #define PRINT_DEBUG

#endif /* EF_CFG_H_ */
