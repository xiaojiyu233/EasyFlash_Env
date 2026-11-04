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
 * Function: Portable interface for STM32F407VET6 (SPL, bare metal).
 * Created on: 2015-01-16
 */

#include <easyflash.h>
#include <string.h>

#include "stm32f4xx.h"

#define EF_SECTOR_CNT 8U

static const uint32_t ef_sector_addr[EF_SECTOR_CNT] = {
    0x08000000U, /* Sector 0, 16KB  */
    0x08004000U, /* Sector 1, 16KB  */
    0x08008000U, /* Sector 2, 16KB  */
    0x0800C000U, /* Sector 3, 16KB  */
    0x08010000U, /* Sector 4, 64KB  */
    0x08020000U, /* Sector 5, 128KB */
    0x08040000U, /* Sector 6, 128KB */
    0x08060000U, /* Sector 7, 128KB */
};

static const uint32_t ef_sector_num[EF_SECTOR_CNT] = {
    FLASH_Sector_0, FLASH_Sector_1, FLASH_Sector_2, FLASH_Sector_3,
    FLASH_Sector_4, FLASH_Sector_5, FLASH_Sector_6, FLASH_Sector_7,
};

static const uint32_t ef_sector_size[EF_SECTOR_CNT] = {
    16U * 1024U, 16U * 1024U,  16U * 1024U,  16U * 1024U,
    64U * 1024U, 128U * 1024U, 128U * 1024U, 128U * 1024U,
};

// TIPS: 定义在rsim_tasks.c
extern uint8_t device_channel_id;
extern uint8_t device_rterm;

static const ef_env default_env_set[] = {
    {"env1", (void *)&device_channel_id, 1},
    {"env2", (void *)&device_rterm, 1},
};

// extern default_env_set

/* Private function prototypes -----------------------------------------------*/
static int32_t ef_get_sector_index(uint32_t addr);

/**
 * Flash port for hardware initialize.
 *
 * @param default_env default ENV set for user
 * @param default_env_size default ENV size
 *
 * @return result
 */
EfErrCode ef_port_init(ef_env const **default_env, size_t *default_env_size) {
  EfErrCode result = EF_NO_ERR;

  *default_env = default_env_set;
  *default_env_size = sizeof(default_env_set) / sizeof(default_env_set[0]);

  return result;
}

/**
 * Read data from flash.
 * @note This operation's units is word.
 *
 * @param addr flash address
 * @param buf buffer to store read data
 * @param size read bytes size
 *
 * @return result
 */
EfErrCode ef_port_read(uint32_t addr, uint32_t *buf, size_t size) {
  EfErrCode result = EF_NO_ERR;

  /* flash 映射在地址空间里，直接按字节拷贝 */
  memcpy((void *)buf, (const void *)addr, size);

  return result;
}

/**
 * Erase data on flash.
 * @note This operation is irreversible.
 * @note This operation's units is different which on many chips.
 *
 * @param addr flash address
 * @param size erase bytes size
 *
 * @return result
 */
EfErrCode ef_port_erase(uint32_t addr, size_t size) {
  EfErrCode result = EF_NO_ERR;
  size_t erased_size = 0;
  int32_t sector;

  /* make sure the start address is a multiple of EF_ERASE_MIN_SIZE */
  EF_ASSERT(addr % EF_ERASE_MIN_SIZE == 0);

  FLASH_Unlock();

  /* it will stop when erased size is greater than setting size */
  while (erased_size < size) {
    sector = ef_get_sector_index(addr + erased_size);
    if (sector < 0) {
      result = EF_ERASE_ERR;
      break;
    }

    FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR |
                    FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);

    if (FLASH_EraseSector(ef_sector_num[sector], VoltageRange_3) !=
        FLASH_COMPLETE) {
      result = EF_ERASE_ERR;
      break;
    }

    /* 按实际扇区大小累加，单次擦除可能覆盖多个扇区 */
    erased_size += ef_sector_size[sector];
  }

  FLASH_Lock();

  return result;
}

/**
 * Write data on flash.
 * @note This operation's units is word.
 * @note This operation must after erase. @see flash_erase.
 *
 * @param addr flash address
 * @param buf the write data buffer
 * @param size write bytes size
 *
 * @return result
 */
EfErrCode ef_port_write(uint32_t addr, const uint32_t *buf, size_t size) {
  EfErrCode result = EF_NO_ERR;
  const uint8_t *buf_8 = (const uint8_t *)buf;
  size_t i;

  FLASH_Unlock();
  FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR |
                  FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);

  for (i = 0; i < size; i++, buf_8++, addr++) {
    /* EF_WRITE_GRAN 为 8bit，按字节编程 */
    if (FLASH_ProgramByte(addr, *buf_8) != FLASH_COMPLETE) {
      result = EF_WRITE_ERR;
      break;
    }
    /* check data */
    if (*(const volatile uint8_t *)addr != *buf_8) {
      result = EF_WRITE_ERR;
      break;
    }
  }

  FLASH_Lock();

  return result;
}

/**
 * lock the ENV ram cache
 * @note 只在主循环/任务上下文中调用 EasyFlash，不使用中断，此处无需加锁。
 * @note 若以后要在中断里调用，这里才需要补关中断/嵌套计数实现。
 */
void ef_port_env_lock(void) {}

/**
 * unlock the ENV ram cache
 */
void ef_port_env_unlock(void) {}

/**
 * This function is print flash debug info.
 *
 * @param file the file which has call this function
 * @param line the line number which has call this function
 * @param format output format
 * @param ... args
 *
 */
void ef_log_debug(const char *file, const long line, const char *format, ...) {

#ifdef PRINT_DEBUG

  /* 需要时接到工程自己的日志接口，打印内容为：
   *   "[Flash](%s:%ld) " + format
   */
  // va_list args;
  // va_start(args, format);
  // printf("[Flash](%s:%ld) ", file, line);
  // vprintf(format, args);
  // va_end(args);

#endif
}

/**
 * This function is print flash routine info.
 *
 * @param format output format
 * @param ... args
 */
void ef_log_info(const char *format, ...) {
  /* 需要时接到工程自己的日志接口，打印内容为：
   *   "[Flash]" + format
   */
  // va_list args;
  // va_start(args, format);
  // printf("[Flash]");
  // vprintf(format, args);
  // va_end(args);
}

/**
 * This function is print flash non-package info.
 *
 * @param format output format
 * @param ... args
 */
void ef_print(const char *format, ...) {
  /* 需要时接到工程自己的日志接口，打印内容为：
   *   format
   */
  // va_list args;
  // va_start(args, format);
  // vprintf(format, args);
  // va_end(args);
}

/**
 * Get the sector index of a given address
 *
 * @param addr flash address
 *
 * @return sector index (0~EF_SECTOR_CNT-1), -1 表示地址超出 Flash 范围
 */
static int32_t ef_get_sector_index(uint32_t addr) {
  int32_t i;

  for (i = (int32_t)EF_SECTOR_CNT - 1; i >= 0; i--) {
    if (addr >= ef_sector_addr[i]) {
      return i;
    }
  }

  return -1;
}
