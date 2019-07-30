/* Wacom I2C Firmware Flash Program*/
/* Copyright (c) 2013 Tatsunosuke Tobita, Wacom. */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "i2c-dev.h"

#define msleep(time)({usleep(time * 1000);})

#define ASCII_EOF 0x1A
 
/*-----------------------------------*/
/*-----------------------------------*/
/*------Wacom specific items---------*/
/*-----------------------------------*/
/*-----------------------------------*/
#define I2C_DEVICE          "/dev/i2c-1"
#define I2C_TARGET          0x09

#define MPU_W9021            0x45
#define FLASH_BLOCK_SIZE     256
#define DATA_SIZE            (65536 * 5)
#define BLOCK_NUM            63
#define W9021_START_ADDR     0x3000
#define W9021_END_ADDR       0x3efff

#define BOOT_CMD_SIZE	     (0x010c + 0x02)//78
#define BOOT_RSP_SIZE	     6

#define BOOT_WRITE_FLASH     1
#define BOOT_EXIT	     3
#define BOOT_BLVER	     4
#define BOOT_MPU	     5
#define BOOT_QUERY	     7
#define ERS_ALL_CMD           0x10

#define ERS_ECH2              0x03
#define QUERY_RSP             0x06

#define PROCESS_INPROGRESS    0xff
#define PROCESS_COMPLETED     0x00
#define PROCESS_CHKSUM1_ERR   0x81
#define PROCESS_CHKSUM2_ERR   0x82
#define PROCESS_TIMEOUT_ERR   0x87
#define RETRY_COUNT           5
#define EXIT_FAIL	      1
#define HEX_READ_ERR          -1

/*-----------------------------------*/
/*-----------------------------------*/
/*------ HID requiring items---------*/
/*-----------------------------------*/
/*-----------------------------------*/
#define RTYPE_FEATURE          0x03 /*: Report type -> feature(11b)*/
#define CMD_GET_FEATURE	       2
#define CMD_SET_FEATURE	       3

#define GFEATURE_SIZE          6
#define SFEATURE_SIZE          8

/*HID specific register*/
#define HID_DESC_REGISTER       1
#define COMM_REG                0x04
#define DATA_REG                0x05

/*Report IDs for Wacom device*/
#define REPORT_ID_1             0x07
#define REPORT_ID_2             0x08
#define FLASH_CMD_REPORT_ID     2
#define BOOT_CMD_REPORT_ID      7

typedef __u8 u8;
typedef __u16 u16;
typedef struct hid_descriptor {
	u16 wHIDDescLength;
	u16 bcdVersion;
	u16 wReportDescLength;
	u16 wReportDescRegister;
	u16 wInputRegister;
	u16 wMaxInputLength;
	u16 wOutputRegister;
	u16 wMaxOutputLength;
	u16 wCommandRegister;
	u16 wDataRegister;
	u16 wVendorID;
	u16 wProductID;
	u16 wVersion;
	u16 RESERVED_HIGH;
	u16 RESERVED_LOW;
} HID_DESC;
