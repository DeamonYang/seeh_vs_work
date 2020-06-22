#include <stdio.h>
#include "mcom_stack.h"
#include <winsock2.h>
#include <Windows.h>
#include <WS2tcpip.h>


unsigned int state_map[DEV_TYP_NUM][DEV_ONE_TYPE_MAX_NUM];

#pragma comment (lib,"ws2_32.lib") //链接ws2_32.dll动态链接库

dev_contacts_t dev_map_tab[MAX_DEVICE_NUMBER] = {
{0x00,0x000000,"192.168.1.25"},
};

unsigned char check_out_device_type(unsigned device_id)
{
	for (int i = 0; i < MAX_DEVICE_NUMBER; i++)
	{
		if (device_id == dev_map_tab[i].unique_id)
		{
			return  dev_map_tab[i].device_type;
		}
	}
	return 255;
}

char pro_send_pak(SOCKET sck, comm_frame_t frd)
{
	char res = 0;
	unsigned char dbuf[COM_PRO_ST_MAX_LEN];
	int len = 0;
	if (frd.cfh.pay_len >= COM_PRO_ST_MAX_LEN - 30)
	{
		exit(-1);
	}
	len = sizeof(frd.cfh);

	memcpy(dbuf,&frd.cfh,len);
	memcpy(&dbuf[len], frd.pdata, frd.cfh.pay_len);
	len += frd.cfh.pay_len;
	res = send(sck, (const char *)dbuf, len, 0);
	return res;
}

char send_command(unsigned target_device_id, cmd_val_t cmd_val, bool mode, SOCKET sck)
{
	comm_frame_t frd;
	char res = 0;
	cmd_val_t temp_cmd_val = cmd_val;
	frd.cfh.frame_header = 0x55AA;
	frd.cfh.su_device_type = 0x00;
	frd.cfh.tg_device_type = check_out_device_type(target_device_id);
	frd.cfh.op_code = 0x01;
	frd.cfh.su_unique_id = MASTER_DEVICE_ID;
 
	frd.cfh.pay_len = sizeof(temp_cmd_val);
	frd.pdata = (char*)&temp_cmd_val;
	res = pro_send_pak(sck, frd);
	return res;
}


char send_state(unsigned int target_device_id, unsigned char tg_device_type, unsigned short state_cm, unsigned short state_cl, SOCKET sck)
{

//	comm_state_t frd;
	comm_frame_t frd;
	char res = 0;
	unsigned int data_code = ((state_cm << 16) & 0xFFFF0000) | (state_cl & 0xFFFF);
	frd.cfh.frame_header = 0x55AA;
	frd.cfh.su_device_type = 0x01;
	frd.cfh.tg_device_type = tg_device_type;
	frd.cfh.op_code = 0x03;
	frd.cfh.su_unique_id = 0x01;

	frd.cfh.pay_len = 4;
	frd.pdata = (char *)&data_code;
	//frd.state_cm = state_cm;
	//frd.state_cl = state_cl;
	res = pro_send_pak(sck, frd);
	return res;
}

unsigned char	su_device_type;
unsigned char	tg_device_type;
unsigned int	su_unique_id;

int unpack_state(char* pdata, int len)
{
	comm_frame_t* pfrd;
	pfrd = (comm_frame_t*)pdata;
	int * state = (int*)pfrd->pdata;
	if (pfrd->cfh.op_code == 0x03)
	{
		if (pfrd->cfh.su_device_type < DEV_TYP_NUM)
		{
			if (pfrd->cfh.su_unique_id < DEV_ONE_TYPE_MAX_NUM)
			{
				state_map[pfrd->cfh.su_device_type][pfrd->cfh.su_unique_id] = *state;
			}
		}
	}
	else
	{
		return -1;
	}

	return *state;
}


char create_client()
{
	SOCKET sclient = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sclient == INVALID_SOCKET)
	{
		printf("invalid socket !");
		return 0;
	}

	sockaddr_in serAddr;
	serAddr.sin_family = AF_INET;
	serAddr.sin_port = htons(4600);
	inet_pton(AF_INET, "192.168.186.1", &serAddr.sin_addr);
//	serAddr.sin_addr.S_un.S_addr = inet_addr("192.168.186.1");
	if (connect(sclient, (sockaddr *)&serAddr, sizeof(serAddr)) == SOCKET_ERROR)
	{
		printf("connect error !");
		closesocket(sclient);
		return 0;
	}

	while (1)
	{
		send_state(1,0, 0x06, 0x01, sclient);
		Sleep(1000);
		printf("send state\r\n");
	}

	//char * sendData = "你好，TCP服务端，我是客户端!\n";
	//send(sclient, sendData, strlen(sendData), 0);

	//char recData[255];
	//int ret = recv(sclient, recData, 255, 0);
	//if (ret > 0)
	//{
	//	recData[ret] = 0x00;
	//	printf(recData);
	//}
	closesocket(sclient);
	WSACleanup();
	return 0;
}


int main()
{
	//初始化WSA
	WORD sockVersion = MAKEWORD(2, 2);
	WSADATA wsaData;
	if (WSAStartup(sockVersion, &wsaData) != 0)
	{
		return 0;
	}
	create_client();
	printf("OK");
	return 0;
}


