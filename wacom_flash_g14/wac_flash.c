/*
 * Wacom Penabled Driver for I2C
 *
 * Copyright (c) 2011-2014 Tatsunosuke Tobita, Wacom.
 * <tobita.tatsunosuke@wacom.co.jp>
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software
 * Foundation; either version of 2 of the License,
 * or (at your option) any later version.
 */

#include "wac_flash.h"
#include <unistd.h>

/*hex file read*/
int read_hex(FILE *fp, char *file_name, unsigned char *flash_data, size_t data_size, unsigned long *max_address)
{
  int s;
  int ret;
  int fd = -1;
  struct stat *stat = NULL;
  unsigned long expand_address = 0;
  unsigned long startLinearAddress = 0;
  unsigned long count = 0;
  unsigned long file_size = 0;

  fd = open(file_name, O_RDWR);
  if (fd < 0) {
	  fprintf(stderr, "cannot open the hex file\n");
  }

  stat = malloc(sizeof(struct stat));
  if (stat == NULL) {
	  printf("Error \n");
  }


  ret = fstat(fd, stat);
  if (ret < 0) {
	  printf("Cannot obtain stat \n");
	  return HEX_READ_ERR;
  }

  file_size = stat->st_size;
  printf("File size : %d \n", file_size);
  free(stat);
  close(fd);

  while (!feof(fp)) {
	  s = fgetc(fp);
	  if (ferror(fp)) {
		  printf("HEX_READ_ERR 1 \n ");
		  return HEX_READ_ERR;
	  }


	  if ( s == ':') {
		  s = fseek(fp, -1L, SEEK_CUR);
		  if (s) {
			  printf("HEX_READ_ERR 2 \n ");
			  return HEX_READ_ERR;
		  }
		  break;
	  }
  }

  while(count < file_size) {
	  unsigned long address = 0;
	  unsigned int byte_count;
	  unsigned int sum, total;
	  unsigned int record_type;
	  int cr = 0, lf = 0;
	  unsigned int i;
	  unsigned int data;


	  s = fgetc(fp);
	  count++;

	  if (s != ':') {
		  if (s == ASCII_EOF) {
			  printf("Reaching to EOF \n");
			  return count;
		  }

		  printf("HEX_READ_ERR 3 \n ");
		  return HEX_READ_ERR; /* header error */
	  }

	  fscanf(fp, "%2X", &byte_count);
	  count += 2;

	  fscanf(fp, "%4lX", &address);
	  count += 4;

	  fscanf(fp, "%2X", &record_type);
	  count += 2;

	  switch (record_type) {
	  case 0:
		  total = byte_count;
		  total += (unsigned char)(address);
		  total += (unsigned char)(address >> 8);
		  total += record_type;
		  address += expand_address;
		  if (address > *max_address) {
			  *max_address = address;
			  *max_address += (byte_count-1);
		  }

		  for (i = 0; i < byte_count; i++){
			  fscanf(fp, "%2X", &data);
			  count += 2;
			  total += data;

			  if (address + i < data_size){
				  flash_data[address + i] = (unsigned char)data;
			  }
		  }

		  fscanf(fp, "%2X", &sum);
		  count += 2;

		  total += sum;
		  if ((unsigned char)(total & 0xff) != 0x00) {
			  printf("HEX_READ_ERR 4 \n ");
			  return HEX_READ_ERR; /* check sum error */
		  }

		  cr = fgetc(fp);
		  count++;

		  lf = fgetc(fp);
		  count++;

		  if (cr != '\r' || lf != '\n') {
			  printf("HEX_READ_ERR 5 \n ");
			  return HEX_READ_ERR;
		  }

		  break;
	  case 1:
		  total = byte_count;
		  total += (unsigned char)(address);
		  total += (unsigned char)(address >> 8);
		  total += record_type;

		  fscanf(fp, "%2X", &sum);
		  count += 2;

		  total += sum;
		  if ((unsigned char)(total & 0xff) != 0x00) {
			  printf("HEX_READ_ERR 6 \n ");
			  return HEX_READ_ERR; /* check sum error */
		  }

		  cr = fgetc(fp);
		  count++;

		  lf = fgetc(fp);
		  count++;

		  if (cr != '\r' || lf != '\n') {
			  printf("HEX_READ_ERR 7 \n ");
			  return HEX_READ_ERR;
		  }

		  break;
	  case 2:
		  fscanf(fp, "%4lX", &expand_address);
		  count += 4;

		  total = byte_count;
		  total += (unsigned char)(address);
		  total += (unsigned char)(address >> 8);
		  total += record_type;
		  total += (unsigned char)(expand_address);
		  total += (unsigned char)(expand_address >> 8);

		  fscanf(fp, "%2X", &sum);
		  count += 2;

		  total += sum;
		  if ((unsigned char)(total & 0xff) != 0x00) {
			  printf("HEX_READ_ERR 8 \n ");
			  return HEX_READ_ERR; /* check sum error */
		  }

		  cr = fgetc(fp);
		  count++;

		  lf = fgetc(fp);
		  count++;

		  if (cr != '\r' || lf != '\n') {
			  printf("HEX_READ_ERR 9 \n ");
			  return HEX_READ_ERR;
		  }

		  expand_address <<= 4;

		  break;

	  case 3:
		  {
			  unsigned long cs=0, ip=0;

			  fscanf(fp, "%4lX", &cs);
			  count += 4;

			  fscanf(fp, "%4lX", &ip);
			  count += 4;

			  expand_address = (cs << 4) + ip;

			  total = byte_count;
			  total += (unsigned char)(address);
			  total += (unsigned char)(address >> 8);
			  total += record_type;
			  total += (unsigned char)(cs);
			  total += (unsigned char)(cs >> 8);
			  total += (unsigned char)(ip);
			  total += (unsigned char)(ip >> 8);

			  fscanf(fp, "%2lX", &sum);
			  count += 2;
			  total += sum;

			  if ((unsigned char)(total & 0x0f) != 0x00)
				  return HEX_READ_ERR;

			  cr = fgetc(fp);
			  count++;

			  lf = fgetc(fp);
			  count++;

			  if (cr != '\r' || lf != '\n')
				  return HEX_READ_ERR;

			  expand_address <<= 16;

			  break;
		  }

	  case 4:
		  fscanf(fp, "%4lX", &expand_address);
		  count += 4;

		  total = byte_count;
		  total += (unsigned char)(address);
		  total += (unsigned char)(address >> 8);
		  total += record_type;
		  total += (unsigned char)(expand_address);
		  total += (unsigned char)(expand_address >> 8);

		  fscanf(fp, "%2X", &sum);
		  count += 2;

		  total += sum;

		  if ((unsigned char)(total & 0xff) != 0x00) {
			  printf("HEX_READ_ERR 10 \n ");
			  return HEX_READ_ERR; /* check sum error */
		  }

		  cr = fgetc(fp);
		  count++;

		  lf = fgetc(fp);
		  count++;

		  if (cr != '\r' || lf != '\n') {
			  printf("HEX_READ_ERR 11 \n ");
			  return HEX_READ_ERR;
		  }

		  expand_address <<= 16;

		  break;

	  case 5:
		  fscanf(fp, "%8lX", &startLinearAddress);
		  count += 8;

		  total = byte_count;
		  total += (unsigned char)(address);
		  total += (unsigned char)(address >> 8);
		  total += record_type;
		  total += (unsigned char)(startLinearAddress);
		  total += (unsigned char)(startLinearAddress >> 8);
		  total += (unsigned char)(startLinearAddress >> 16);
		  total += (unsigned char)(startLinearAddress >> 24);

		  fscanf(fp, "%2lX", &sum);
		  count += 2;
		  total += sum;

#if 0
		  printf("byte_count: %d\n", byte_count);
		  printf("Address: %d\n", address);
		  printf("record_type %d\n", record_type);
		  printf("startLinearAddress: %d\n", startLinearAddress);
		  printf("total: %d \n\n\n", total);
#endif
		  if ((unsigned char)(total & 0x0f) != 0x00)
			  return HEX_READ_ERR; /* check sum error */

		  cr = fgetc(fp);
		  count++;

		  lf = fgetc(fp);
		  count++;

		  if (cr != '\r' || lf != '\n')
			  return HEX_READ_ERR;

		  break;

	  default:
		  printf("HEX_READ_ERR 12 \n ");
		  return HEX_READ_ERR;
	  }
  }

  return count;
}
/*********************************************************************************************************/


bool wacom_i2c_set_feature(int fd, u8 report_id, unsigned int buf_size, u8 *data,
			   u16 cmdreg, u16 datareg)
{
	int i, ret = -1;
	int total = SFEATURE_SIZE + buf_size;
	u8 *sFeature = NULL;
	bool bRet = false;

	sFeature = malloc(sizeof(u8) * total);
	if (!sFeature) {
		fprintf(stderr, "%s cannot preserve memory \n", __func__);
		goto out;
	}

	memset(sFeature, 0, sizeof(u8) * total);

	sFeature[0] = (u8)(cmdreg & 0x00ff);
	sFeature[1] = (u8)((cmdreg & 0xff00) >> 8);
	sFeature[2] = (RTYPE_FEATURE << 4) | report_id;
	sFeature[3] = CMD_SET_FEATURE;
	sFeature[4] = (u8)(datareg & 0x00ff);
	sFeature[5] = (u8)((datareg & 0xff00) >> 8);

	if ( (buf_size + 2) > 255) {
		sFeature[6] = (u8)((buf_size + 2) & 0x00ff);
		sFeature[7] = (u8)(( (buf_size + 2) & 0xff00) >> 8);
	} else {
		sFeature[6] = (u8)(buf_size + 2);
		sFeature[7] = (u8)(0x00);
	}

	for (i = 0; i < buf_size; i++)
		sFeature[i + SFEATURE_SIZE] = *(data + i);

	ret = write(fd, sFeature, total);
	if (ret != total) {
		fprintf(stderr, "Sending Set_Feature failed sent bytes: %d \n", ret);
		goto err;
	}

	bRet = true;
 err:
	free(sFeature);
	sFeature = NULL;

 out:
	return bRet;
}

/*get_feature uses ioctl for using I2C restart method to communicate*/
/*and for that i2c_msg requires "char" for buf rather than unsinged char,*/
/*so storing data should be back as "unsigned char".*/
bool wacom_i2c_get_feature(int fd, u8 report_id, unsigned int buf_size, u8 *data,
		 u16 cmdreg, u16 datareg, char addr)

{
	struct i2c_rdwr_ioctl_data packets;

	/*"+ 2", adding 2 more spaces for organizeing again later in the passed data, "data"*/
	unsigned int total = buf_size + 2;
	char *recv = NULL;
	bool bRet = false;
	u8 gFeature[] = {
		(u8)(cmdreg & 0x00ff),
		(u8)((cmdreg & 0xff00) >> 8),
		(RTYPE_FEATURE << 4) | report_id,
		CMD_GET_FEATURE,
		(u8)(datareg & 0x00ff),
		(u8)((datareg & 0xff00) >> 8)
	};

	recv = (char *)malloc(sizeof(char) * total);
	if (recv == NULL) {
		fprintf(stderr, "%s cannot preserve memory \n", __func__);
		goto out;
	}
	memset(recv, 0, sizeof(char) * total);

	{
		struct i2c_msg msgs[] = {
			{
				.addr = addr,
				.flags = 0,
				.len = GFEATURE_SIZE,
				.buf = (char *)gFeature,
			},
			{
				.addr = addr,
				.flags = I2C_M_RD,
				.len = total,
				.buf = recv,
			},
		};

		packets.msgs  = msgs;
		packets.nmsgs = 2;
		if (ioctl(fd, I2C_RDWR, &packets) < 0) {
			fprintf(stderr, "%s failed to send messages\n", __func__);
			goto err;
		}

		/*First two bytes in recv are length of
		  the report and data doesn't need them*/
		memcpy(data, (unsigned char *)(recv + 2), buf_size);
	}

#ifdef WACOM_DEBUG_LV3
	{
		int ret = -1;
		fprintf(stderr, "Recved bytes: %d \n", ret);
		fprintf(stderr, "Expected bytes: %d \n", buf_size);
		fprintf(stderr, "1: %x, 2: %x, 3:%x, 4:%x 5:%x\n", data[0], data[1], data[2], data[3], data[4]);
	}
#endif

	bRet = true;
 err:
	free(recv);
	recv = NULL;
 out:
	return bRet;

}

bool wacom_flash_cmd(int fd)
{
	int len = 0;
	u8 cmd[2] = {0};
	bool bRet = false;

	cmd[len++] = 0x02;
	cmd[len++] = 0x02;

	bRet = wacom_i2c_set_feature(fd, FLASH_CMD_REPORT_ID, len, cmd, COMM_REG, DATA_REG);
	if (!bRet){
		fprintf(stderr, "Sending flash command failed\n");
		return bRet;
	}

	msleep(300);

	return true;
}

bool flash_query_w9021(int fd)
{
	bool bRet = false;
	u8 command[BOOT_CMD_SIZE] = {0};
	u8 response[BOOT_RSP_SIZE] = {0};
	int ECH = 0, len = 0;

	command[len++] = BOOT_CMD_REPORT_ID;	                /* Report:ReportID */
	command[len++] = BOOT_QUERY;				/* Report:Boot Query command */
	command[len++] = ECH = 7;				/* Report:echo */

	bRet = wacom_i2c_set_feature(fd, REPORT_ID_1, len, command, COMM_REG, DATA_REG);
	if (!bRet) {
		fprintf(stderr, "%s failed to set feature \n", __func__);
		return bRet;
	}

	bRet = wacom_i2c_get_feature(fd, REPORT_ID_2, BOOT_RSP_SIZE, response, COMM_REG, DATA_REG, I2C_TARGET);
	if (!bRet) {
		fprintf(stderr, "%s failed to get feature \n", __func__);
		return bRet;
	}

	if ( (response[1] != BOOT_CMD_REPORT_ID) ||
	     (response[2] != ECH) ) {
		fprintf(stderr, "%s res1:%x res2:%x \n", __func__, response[1], response[2]);
		return false;
	}

	if (response[3] != QUERY_RSP) {
		fprintf(stderr, "%s res3:%x \n", __func__, response[3]);
		return false;
	}

	return true;
}

bool flash_blver_w9021(int fd, int *blver)
{
	bool bRet = false;
	u8 command[BOOT_CMD_SIZE] = {0};
	u8 response[BOOT_RSP_SIZE] = {0};
	int ECH = 0, len = 0;

	command[len++] = BOOT_CMD_REPORT_ID;	/* Report:ReportID */
	command[len++] = BOOT_BLVER;					/* Report:Boot Version command */
	command[len++] = ECH = 7;							/* Report:echo */

	bRet = wacom_i2c_set_feature(fd, REPORT_ID_1, len, command, COMM_REG, DATA_REG);
	if (!bRet) {
		fprintf(stderr, "%s failed to set feature1\n", __func__);
		return bRet;
	}

	bRet = wacom_i2c_get_feature(fd, REPORT_ID_2, BOOT_RSP_SIZE, response, COMM_REG, DATA_REG, I2C_TARGET);
	if (!bRet) {
		fprintf(stderr, "%s 2 failed to set feature\n", __func__);
		return bRet;
	}

	if ( (response[1] != BOOT_BLVER) ||
	     (response[2] != ECH) ) {
		fprintf(stderr, "%s res1:%x res2:%x \n", __func__, response[1], response[2]);
		return false;
	}

	*blver = (int)response[3];

	return true;
}

bool flash_mputype_w9021(int fd, int* pMpuType)
{
	bool bRet = false;
	u8 command[BOOT_CMD_SIZE] = {0};
	u8 response[BOOT_RSP_SIZE] = {0};
	int ECH = 0, len = 0;

	command[len++] = BOOT_CMD_REPORT_ID;	                        /* Report:ReportID */
	command[len++] = BOOT_MPU;					/* Report:Boot Query command */
	command[len++] = ECH = 7;					/* Report:echo */

	bRet = wacom_i2c_set_feature(fd, REPORT_ID_1, len, command, COMM_REG, DATA_REG);
	if (!bRet) {
		fprintf(stderr, "%s failed to set feature \n", __func__);
		return bRet;
	}

	bRet = wacom_i2c_get_feature(fd, REPORT_ID_2, BOOT_RSP_SIZE, response, COMM_REG, DATA_REG, I2C_TARGET);
	if (!bRet) {
		fprintf(stderr, "%s failed to get feature \n", __func__);
		return bRet;
	}

	if ( (response[1] != BOOT_MPU) ||
	     (response[2] != ECH) ) {
		fprintf(stderr, "%s res1:%x res2:%x \n", __func__, response[1], response[2]);
		return false;
	}

	*pMpuType = (int)response[3];
	return true;
}

bool flash_end_w9021(int fd)
{
	bool bRet = false;
	u8 command[BOOT_CMD_SIZE] = {0};
	int len = 0;

	command[len++] = BOOT_CMD_REPORT_ID;
	command[len++] = BOOT_EXIT;
	command[len++] = 0;

	bRet = wacom_i2c_set_feature(fd, REPORT_ID_1, len, command, COMM_REG, DATA_REG);
	if (!bRet) {
		fprintf(stderr, "%s failed to set feature 1\n", __func__);
		return bRet;
	}

	return true;
}

int check_progress(u8 *data, size_t size, u8 cmd, u8 ech)
{
	if (data[0] != cmd || data[1] != ech) {
		fprintf(stderr, "%s failed to erase \n", __func__);
		return -EXIT_FAIL;
	}

	switch (data[2]) {
	case PROCESS_CHKSUM1_ERR:
	case PROCESS_CHKSUM2_ERR:
	case PROCESS_TIMEOUT_ERR:
		fprintf(stderr, "%s error: %x \n", __func__, data[2]);
		return -EXIT_FAIL;
	}

	return data[2];
}

bool flash_erase_all(int fd)
{
	bool bRet = false;
	u8 command[BOOT_CMD_SIZE] = {0};
	u8 response[BOOT_RSP_SIZE] = {0};
	int i = 0, len = 0;
	int ECH = 0, sum = 0;
	int ret = -1;

	command[len++] = 7;
	command[len++] = ERS_ALL_CMD;
	command[len++] = ECH = 2;
	command[len++] = ERS_ECH2;

	//Preliminarily stored data that cannnot appear here, but in wacom_set_feature()
	sum += 0x05;
	sum += 0x07;
	for (i = 0; i < len; i++)
		sum += command[i];

	command[len++] = ~sum + 1;

	bRet = wacom_i2c_set_feature(fd, REPORT_ID_1, len, command, COMM_REG, DATA_REG);
	if (!bRet) {
		fprintf(stderr, "%s failed to set feature \n", __func__);
		return bRet;
	}

	do {
		bRet = wacom_i2c_get_feature(fd, REPORT_ID_2, BOOT_RSP_SIZE, response,
					     COMM_REG, DATA_REG, I2C_TARGET);
		if (!bRet) {
			fprintf(stderr, "%s failed to set feature \n", __func__);
			return bRet;
		}

		if ((ret = check_progress(&response[1], (BOOT_RSP_SIZE - 3), ERS_ALL_CMD, ECH)) < 0)
			return false;

	} while(ret == PROCESS_INPROGRESS);

	return true;
}

bool flash_write_block_w9021(int fd, char *flash_data,
				    unsigned long ulAddress, u8 *pcommand_id, int *ECH)
{
	const int MAX_COM_SIZE = (8 + FLASH_BLOCK_SIZE + 2); //8: num of command[0] to command[7]
                                                              //FLASH_BLOCK_SIZE: unit to erase the block
                                                              //Num of Last 2 checksums
	bool bRet = false;
	u8 command[300] = {0};
	unsigned char sum = 0;
	int i = 0;

	command[0] = BOOT_CMD_REPORT_ID;	                /* Report:ReportID */
	command[1] = BOOT_WRITE_FLASH;			        /* Report:program  command */
	command[2] = *ECH = ++(*pcommand_id);		        /* Report:echo */
	command[3] = ulAddress & 0x000000ff;
	command[4] = (ulAddress & 0x0000ff00) >> 8;
	command[5] = (ulAddress & 0x00ff0000) >> 16;
	command[6] = (ulAddress & 0xff000000) >> 24;			/* Report:address(4bytes) */
	command[7] = 0x20;

	/*Preliminarily stored data that cannnot appear here, but in wacom_set_feature()*/
	sum = 0;
	sum += 0x05; sum += 0x0c; sum += 0x01;
	for (i = 0; i < 8; i++)
		sum += command[i];
	command[MAX_COM_SIZE - 2] = ~sum + 1;					/* Report:command checksum */

	sum = 0;
	for (i = 8; i < (FLASH_BLOCK_SIZE + 8); i++){
		command[i] = flash_data[ulAddress+(i - 8)];
		sum += flash_data[ulAddress+(i - 8)];
	}

	command[MAX_COM_SIZE - 1] = ~sum+1;				/* Report:data checksum */

	/*Subtract 8 for the first 8 bytes*/
	bRet = wacom_i2c_set_feature(fd, REPORT_ID_1, (BOOT_CMD_SIZE + 4 - 8), command, COMM_REG, DATA_REG);
	if (!bRet) {
		fprintf(stderr, "%s failed to set feature at addr: %x\n", __func__, ulAddress);
		return bRet;
	}

	usleep(50);

	return true;
}

bool flash_write_w9021(int fd, unsigned char *flash_data,
			      unsigned long start_address, unsigned long *max_address)
{
	bool bRet = false;
	u8 command_id = 0;
	u8 response[BOOT_RSP_SIZE] = {0};
	int i = 0, j = 0, ECH = 0, ECH_len = 0;
	int ECH_ARRAY[3] = {0};
	int ret = -1;
	unsigned long ulAddress = 0;

	j = 0;
	for (ulAddress = start_address; ulAddress < *max_address; ulAddress += FLASH_BLOCK_SIZE) {
		for (i = 0; i < FLASH_BLOCK_SIZE; i++) {
			if (flash_data[ulAddress+i] != 0xFF)
				break;
		}

		if (i == (FLASH_BLOCK_SIZE))
			continue;

		bRet = flash_write_block_w9021(fd, flash_data, ulAddress, &command_id, &ECH);
		if(!bRet)
			return bRet;
		if (ECH_len == 3)
			ECH_len = 0;

		ECH_ARRAY[ECH_len++] = ECH;
		if (ECH_len == 3) {
			for (j = 0; j < 3; j++) {
				do {

					bRet = wacom_i2c_get_feature(fd, REPORT_ID_2, BOOT_RSP_SIZE, response, COMM_REG, DATA_REG, I2C_TARGET);
					if (!bRet) {
						fprintf(stderr, "%s failed to set feature \n", __func__);
						return bRet;
					}

					if ((ret = check_progress(&response[1], (BOOT_RSP_SIZE - 3), 0x01, ECH_ARRAY[j])) < 0) {
						fprintf(stderr, "addr: %x res:%x \n", ulAddress, response[1]);
						return false;
					}
				} while (ret == PROCESS_INPROGRESS);
			}
		}
	}

	return true;
}

bool wacom_i2c_flash_w9021(int fd,  unsigned char *f_data)
{
	bool bRet = false;
	int iBLVer = 0, iMpuType = 0;
	unsigned long max_address = W9021_END_ADDR;			/* Max.address of Load data */
	unsigned long start_address = W9021_START_ADDR;	        /* Start.address of Load data */

	/*Obtain boot loader version*/
	if (!flash_blver_w9021(fd, &iBLVer)) {
		fprintf(stderr, "%s failed to get Boot Loader version \n", __func__);
		return false;
	}
	fprintf(stderr, "BL version: %x \n", iBLVer);

	/*Obtain MPUtype: this can be manually done in user space*/
	if (!flash_mputype_w9021(fd, &iMpuType)) {
		fprintf(stderr, "%s failed to get MPU type \n", __func__);
		return false;
	}

	if (iMpuType != MPU_W9021) {
		fprintf(stderr, "MPU is not for W9021 : %x \n", iMpuType);
		return false;
	}
	fprintf(stderr, "MPU type: %x \n", iMpuType);

	/*-----------------------------------*/
	/*Flashing operation starts from here*/

	/*Erase the current loaded program*/
	fprintf(stderr, "%s erasing the current firmware \n", __func__);
	bRet = flash_erase_all(fd);
	if (!bRet) {
		fprintf(stderr, "%s failed to erase the user program \n", __func__);
		return bRet;
	}

	/*Write the new program*/
	fprintf(stderr, "%s writing new firmware \n", __func__);
	bRet = flash_write_w9021(fd, f_data, start_address, &max_address);
	if (!bRet) {
		fprintf(stderr, "%s failed to write firmware \n", __func__);
		return bRet;
	}

	printf("%s write and verify completed \n", __func__);

	return true;
}

bool wacom_i2c_flash(int fd, unsigned char *data)
{
	bool bRet = false;

	/*Even this fails, try query if it is boot-mode or not*/
	wacom_flash_cmd(fd);

	bRet = flash_query_w9021(fd);
	if(!bRet) {
		fprintf(stderr, "%s Error: cannot send query \n", __func__);
		goto err;
	}

	bRet = wacom_i2c_flash_w9021(fd, data);
	if (!bRet) {
		fprintf(stderr, "%s Error: flash failed \n", __func__);
		goto err;
	}

	bRet = true;
 err:
	/*Return to the user mode*/
	fprintf(stderr, "%s closing the boot mode \n", __func__);
	bRet = flash_end_w9021(fd);
	if (!bRet) {
		fprintf(stderr, "%s closing boot mode failed  \n", __func__);
	}

	return bRet;
}

void show_hid_descriptor(HID_DESC hid_descriptor)
{
	fprintf(stderr,  "Length:%d bcdVer:0x%x RPLength:0x%x \
RPRegister:%d InputReg:%d \n \
MaxInput:%d OutReg:%d MaxOut:%d \
ComReg:%d DataReg:%d \n VID:0x%x \
PID:0x%x wVer:0x%x ResvH:%d ResvL%d\n",
	       hid_descriptor.wHIDDescLength,
	       hid_descriptor.bcdVersion,
	       hid_descriptor.wReportDescLength,
	       hid_descriptor.wReportDescRegister,
	       hid_descriptor.wInputRegister,
	       hid_descriptor.wMaxInputLength,
	       hid_descriptor.wOutputRegister,
	       hid_descriptor.wMaxOutputLength,
	       hid_descriptor.wCommandRegister,
	       hid_descriptor.wDataRegister,
	       hid_descriptor.wVendorID,
	       hid_descriptor.wProductID,
	       hid_descriptor.wVersion,
	       hid_descriptor.RESERVED_HIGH,
	       hid_descriptor.RESERVED_LOW);
}

int get_hid_desc(int fd)
{
	struct i2c_rdwr_ioctl_data packets;
	HID_DESC hid_descriptor;
	int ret = -1;
	char cmd[] = {HID_DESC_REGISTER, 0x00};
	struct i2c_msg msgs[] = {
		{
			.addr = I2C_TARGET,
			.flags = 0,
			.len = sizeof(cmd),
			.buf = cmd,
		},
		{
			.addr = I2C_TARGET,
			.flags = I2C_M_RD,
			.len = sizeof(HID_DESC),
			.buf = (char *)&hid_descriptor,
		},
	};

	packets.msgs  = msgs;
	packets.nmsgs = 2;
	if (ioctl(fd, I2C_RDWR, &packets) < 0) {
		fprintf(stderr, "%s failed to send messages\n", __func__);
		ret = -EXIT_FAIL;
		goto out;
	}

#ifdef SHOW_DESC
	show_hid_descriptor(hid_descriptor);
#endif

	printf("******************************\n");
	printf("\n");
	printf("Current firmware vesrsion:\n");
	printf(" 0x%x\n", hid_descriptor.wVersion);
	printf("******************************\n");


	ret = 0;
 out:
	return ret;
}


int main(int argc, char *argv[])
{
	/*For read hex*/
	char *file_name;
	char device_num[64];
	unsigned long maxAddr;
	FILE *fp;

	/*For Flash*/
	bool bRet = false;
	bool only_fw_ver = false;

	unsigned char flash_data[DATA_SIZE];
	unsigned long start_address = W9021_START_ADDR;
	unsigned long max_address = W9021_END_ADDR;
	unsigned long max_range = max_address - start_address;
	int i, ret, fd, cnt;
	int bl_ver = 0, mpu_type = 0;
	int erase_block_num = 0;
	int erase_block[BLOCK_NUM + 1];

	int count = 0;

	if(argc == 1 || argc > 3) {
		printf("\n\n++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
		printf("@Usage:\n");
		printf("------------------------------------------------------\n");
		printf("$wac_flash [target file name] \n");
		printf("               OR                    \n");
		printf("$wac_flash [target file name] [i2c device file dir]\n");
		printf("Ex:\n");
		printf("$wac_flash W9021_056.hex i2c-1 \n\n\n");
		printf("++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
		printf("@If you want to check the current firmware version do:\n");
		printf("$wac_flash -v\n");
		printf("------------------------------------------------------\n\n");
		return 0;
	}

	if (argc == 3) {
		strcpy(device_num, "/dev/");
		strcat(device_num, argv[2]);
	} else {
		strcpy(device_num, I2C_DEVICE);
	}

	if (strcmp(argv[1], "-v") == 0) {
		only_fw_ver = true;
		goto fw_ver;

	}

	/**************************************/
	/**************************************/
	/*From here starts reading hex file****/
	/**************************************/
	/**************************************/
	file_name = argv[1];
	printf("Hex file name: %s \n", file_name);

	for (i = 0; i < DATA_SIZE; i++)
		flash_data[i] = 0xff;

	fp = fopen(argv[1], "rb");
	if (fp == NULL) {
		printf("the file name is invalid or does not exist\n");
		return -EXIT_FAIL;
	}

	cnt = read_hex(fp, file_name, flash_data, DATA_SIZE, &maxAddr);
	if (cnt == HEX_READ_ERR) {
		printf("reading the hex file failed\n");
		fclose(fp);
		return -EXIT_FAIL;
	} else
		printf("Reading hex-file is succeeded\n");
	fclose(fp);

	/****************************************/
	/*From here prepares for flash operation*/
	/****************************************/
 fw_ver:
	printf("Opening a device file %s \n",  device_num);
	fd = open(device_num, O_RDWR);
	if (fd < 0) {
		fprintf(stderr, "cannot open %s \n", device_num);
		return -EXIT_FAIL;
	}

	/*If I2C_SLAVE makes "Segmentation fault" or the error, use I2C_SLAVE_FORCE instead*/
	if (ioctl(fd, I2C_SLAVE_FORCE, I2C_TARGET)) {
		fprintf(stderr, "Falied to set the slave address: %s \n", I2C_TARGET);
		goto err;
	}


	ret = get_hid_desc(fd);
	if (ret < 0)
		fprintf(stderr, "Failed to get hid descriptor\n");
	if (only_fw_ver)
		goto err;

	for (count = 0; count < RETRY_COUNT; count++) {
		printf("\n %d st attempt\n", (count + 1));
		if (wacom_i2c_flash(fd, flash_data))
			break;
		else
			fprintf(stderr, "Flashing firmware failed\n");
	}

	ret = get_hid_desc(fd);
	if (ret < 0)
		fprintf(stderr, "Failed to get hid descriptor\n");

 err:
	close(fd);
	return ret;
}
