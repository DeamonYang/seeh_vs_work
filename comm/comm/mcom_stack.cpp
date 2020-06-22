#include <stdio.h>
#include "mcom_stack.h"
#include <winsock2.h>
#include <Windows.h>

#pragma comment (lib,"ws2_32.lib") //����ws2_32.dll��̬���ӿ�
unsigned int state_map[DEV_TYP_NUM][DEV_ONE_TYPE_MAX_NUM];
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



/*share memory 4 state code */
/*
�������ܣ����IP��ַ
��ϸ���ܣ����IP��ַ�еĵ��Ƿ���3�����Լ�ÿ��IP����ֵ�Ƿ񳬹�255
*/
int CheckIP(char *cIP)
{
	char IPAddress[128];//IP��ַ�ַ���
	char IPNumber[4];//IP��ַ��ÿ�����ֵ
	int iSubIP = 0;//IP��ַ��4��֮һ
	int iDot = 0;//IP��ַ��'.'�ĸ���
	int iResult = 0;
	int iIPResult = 1;
	int i;//ѭ�����Ʊ���

	memset(IPNumber, 0, 4);
	strncpy(IPAddress, cIP, 128);

	for (i = 0; i<128; i++)
	{
		if (IPAddress[i] == '.')
		{
			iDot++;
			iSubIP = 0;
			if (atoi(IPNumber)>255) //���ÿ��IP����ֵ�Ƿ񳬹�255
				iIPResult = 0;
			memset(IPNumber, 0, 4);
		}
		else
		{
			IPNumber[iSubIP++] = IPAddress[i];
		}
		if (iDot == 3 && iIPResult != 0) //���IP��ַ�еĵ��Ƿ���3��
			iResult = 1;
	}

	return iResult;
}

/*
�������ܣ��˳�ϵͳ����,���ͷ��ļ�ָ���ws2_32.lib��̬���ӿ�
*/
void ExitSystem()
{
	//if (server_fp != NULL)
	//	fclose(server_fp);
	//if (client_fp != NULL)
	//	fclose(client_fp);
	WSACleanup(); //�ͷų�ʼ��ws2_32.lib��̬���ӿ����������Դ
	exit(0);
}


/*
�������ܣ���������˽�����Ϣ���߳�
*/
DWORD WINAPI threadproServer(LPVOID lpParam) // LPVOID lpParameterΪ�̲߳���
{
	SOCKET hsock = (SOCKET)lpParam;
	char cRecvBuffer[1024]; //������Ϣ����,�������ݱ�����cRecvBuff[]
	char cShowBuffer[1024]; //��ʾ��Ϣ����
	int iRecvResult = 0;

	if (hsock != INVALID_SOCKET)
	{
		printf("start:\n");
	}

	while (1)
	{
		iRecvResult = recv(hsock, cRecvBuffer, 1024, 0);
		if (iRecvResult >= 0)
		{
			//cRecvBuffer[iRecvResult] = '\0'; //��cRecvBuff[]��Ϊ�ַ���
			//sprintf(cShowBuffer, "to me:%s\n", cRecvBuffer); //sprintf: ��ʽ��������д��ĳ���ַ�����
			//printf("%s", cShowBuffer); //��ʾ���յ�������
			//fwrite(cShowBuffer, 1, strlen(cShowBuffer), server_fp); //�����յ������ݣ�д�뵽������ļ���
			//fflush(server_fp); //ˢ���ļ��� stream ����������� (�ļ�ָ�뱾��Ҳ��һ����stream)
			unpack_state(cRecvBuffer, iRecvResult);
			if (strcmp("exit", cRecvBuffer) == 0)
			{
				ExitSystem(); //�˳�ϵͳ����,���ͷ��ļ�ָ���ws2_32.lib��̬���ӿ�
				//�˳�ϵͳ
			}
		}
		else
		{
			ExitSystem(); //�˳�ϵͳ����,���ͷ��ļ�ָ���ws2_32.lib��̬���ӿ�
		}
	}
	return 0;
}

/*
�������ܣ�������Ե�����
��ϸ���ܣ�����˼����ͷ��˷������������󣬵��пͻ��˷�����������ʱ������������Ϣ���̲߳����뷢����Ϣ��ѭ����
*/
void createServer()
{
	SOCKET    server_listenSocket; //����˵ļ����׽���,socket()�����ģ������ͻ����Ƿ�����������
	SOCKET    server_communiSocket; //����˵�ͨ���׽���,accept()���ص�,��ͻ��˽���ͨ��
	struct sockaddr_in server_sockAddr; //��������˵ı��ؽӿںͶ˿ںŵ�sockaddr_in�ṹ��
	struct sockaddr_in client_sockAddr; //���������ӿͷ��˵ĽӿںͶ˿ںŵ�sockaddr_in�ṹ��
	struct hostent *localHost; //���������������������͵�ַ��Ϣ��hostent�ṹ��ָ��

	int iPort = 4600; //�趨Ϊ�̶��˿�
	char* localIP; //����������IP��ַ
	DWORD nThreadId = 0; //����ID
	int iBindResult = -1; //�󶨽��
	int ires;//���͵ķ���ֵ
	int iWhileCount_bind = 10; //�ܹ���������˿ںŰ󶨱��������Ļ������
	int iWhileCount_listen = 10; //�ܹ����¼����Ļ������
	char cWelcomBuffer[] = "Welcome to you\0"; //��ӭ��Ϣ���ַ���
	char cSendBuffer[1024];//������Ϣ����
	char cShowBuffer[1024];//������Ϣ����
	int len = sizeof(struct sockaddr);

	//server_fp = fopen("MessageServer.txt", "a");//�򿪼�¼��Ϣ���ļ�

	//����һ������˵ı��������׽���
	server_listenSocket = socket(AF_INET, SOCK_STREAM, 0); //TCP��ʽ����typeѡ��SOCK_STREAM��ʽ�׽���


	//��ȡ����������IP��ַ
	localHost = gethostbyname(""); //��ȡ���������������������͵�ַ��Ϣ��hostent�ṹ��ָ��
	localIP = "192.168.86.1";//inet_ntoa(*(struct in_addr *)*localHost->h_addr_list); //��ȡ����������IP��ַ

	//���ñ��������������ַ��Ϣ
	server_sockAddr.sin_family = AF_INET; //���õ�ַ����                    
	server_sockAddr.sin_port = htons(iPort); //���ñ��������Ķ˿ں�        
	server_sockAddr.sin_addr.S_un.S_addr = INADDR_ANY; //���ñ���������IP��ַ

	//���׽��ְ��ڱ���������
	iBindResult = bind(server_listenSocket, (struct sockaddr*)&server_sockAddr, sizeof(struct sockaddr));
	if (iBindResult != 0)
	{
		printf(" bind error %d \n", iBindResult);
		exit(0);
	}


	//�ظ�����
	while (1)
	{
		printf("start listen\n");
		listen(server_listenSocket, 0);//����ֵ�жϵ��������Ƿ�ʱ

		server_communiSocket = accept(server_listenSocket, (struct sockaddr*)&client_sockAddr, &len);
		if (server_communiSocket != INVALID_SOCKET)
		{
			////�����ӳɹ������ͻ�ӭ��Ϣ
			//send(server_communiSocket, cWelcomBuffer, sizeof(cWelcomBuffer), 0);
			//����������Ϣ���߳�
			CreateThread(NULL, 0, threadproServer, (LPVOID)server_communiSocket, 0, &nThreadId);
//			break;
		}
		printf(".");
	}
}


int main()
{
	//��ʼ��WSA
	WORD sockVersion = MAKEWORD(2, 2);
	WSADATA wsaData;
	if (WSAStartup(sockVersion, &wsaData) != 0)
	{
		return 0;
	}
	createServer();
	printf("OK");
	return 0;
}

///https://www.cnblogs.com/linuxAndMcu/p/9787473.html

