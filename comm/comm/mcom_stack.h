#ifndef _MCOM_STACK_H_
#define _MCOM_STACK_H_ 1

//communication protocol stack

typedef struct
{
	unsigned char	device_type;
	unsigned int	unique_id;
	char			ip_add[20];
}dev_contacts_t;


typedef struct
{
	unsigned short	frame_header;
	unsigned char	su_device_type;
	unsigned char	tg_device_type;
	unsigned int	su_unique_id;
	unsigned short	op_code;
	unsigned short	pay_len;
}comm_frame_hd_t;

typedef struct 
{
	comm_frame_hd_t	cfh;
	char*			pdata;
	unsigned char	checksum;
}comm_frame_t;


//unified state code struct
typedef struct 
{
	unsigned short	fsbs;
	unsigned short	lsbs;
}state_code_t;

//unified cmd val struct
typedef struct
{
	unsigned short	fsbc;
	unsigned short	lsbc;
}cmd_val_t;

#define COM_PRO_ST_MAX_LEN 200
#define MASTER_DEVICE_ID	0xFAFAFAFA
#define MAX_DEVICE_NUMBER	500

//seacher
#define CMD_SC_FI	0X01	// search drugs
#define CMD_SC_RST	0x02	//reset
#define CMD_SC_STP	0x03	//emergency stop
#define CMD_SC_NUL	0x00	// nothing

#define ST_SC_NTRD	0x01

#define DEV_TYP_NUM 5
#define DEV_ONE_TYPE_MAX_NUM 1024 //maximum number of one kind of device 


#endif