#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

#include <string.h>
#include <sys/types.h>
#include <termios.h>

#include <pthread.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <curl/curl.h>
#include <wiringPi.h>

static int fd = 0, fd_obd;
#define MAX_BUF_SIZE 512
#define DEV_UART "/dev/ttyACM0"
#define OBD_UART "/dev/ttyUSB0"

//#define TRUE 1
//#define FALSE 0
#define BUZZER 	17
#define LED_RED	10	//#19
#define LED_YELLOW	9	//#21
#define LED_GREEN	11	//#23

#define L_LED		23	//16
#define R_LED		24	//18

#define uint8_t	unsigned char 
#define uint32_t unsigned int
#define uint16_t unsigned short
#define BYTE 	unsigned char

#define DATA_MAX   14

int httppost(char ch, double lat, double lon);
int httpget(double *x1, double *y1, double *x2, double *y2);
int httppost_obd(int speed, int batt, int rpm, int cnt);
int httppost_dist(float dist1, float dist2, char *dir);
int obd_cnt = 0;
char ipaddr[16] = "112.171.101.26";
//char ipaddr[16] = "192.168.0.115"; // test-server

typedef struct _GPS_PARSE_DEF_{
    uint8_t		flagRead;        // flag used by the parser, when a valid sentence has begun
    uint8_t	    flagTIMEDataReady;   // valid UTC time data available, user can call reader functions
    uint8_t	    flagUTCDataReady;   // valid UTC date data available
    int8_t    	words[20][15];  // hold parsed words for one given NMEA sentence
    uint8_t    	szChecksum[15];	// hold the received checksum for one given NMEA sentence

    // will be set to true for characters between $ and * only
    uint8_t		flagComputedCks;     // used to compute checksum and indicate valid checksum interval (between $ and * in a given sentence)
    uint8_t     checksum;            // numeric checksum, computed for a given sentence
    uint8_t    	flagReceivedCks;     // after getting  * we start cuttings the received checksum
    uint8_t     index_received_checksum;// used to parse received checksum

    // word cutting variables
    uint8_t     wordIdx;         // the current word in a sentence
    uint8_t     prevIdx;		// last character index where we did a cut
    uint8_t     nowIdx ;		// current character index

    // globals to store parser results
    double   	longitude;     // GPRMC and GPGGA
    double   	latitude;	// GPRMC and GPGGA
    uint8_t   	UTCHour, UTCMin, UTCSec, UTCDay, UTCMonth, UTCYear;
    uint8_t		flagDetect;		// flag NMEA sentence
}GPS_ParseTypeDef;

GPS_ParseTypeDef parser;

typedef enum
{
  LEN_1, 
  LEN_10, 
  LEN_100, 
  LEN_1000, 
  LEN_10000,
  LEN_100000,
  LEN_1000000,
  LEN_10000000,
  LEN_100000000,
  LEN_1000000000
}_LEN_Def;

typedef enum 
{
    OBD_ON_READY,
    OBD_OFF_READY,
    OBD_GET_GENDER_SPEC,
//    OBD_GET_GENDER_RDY,
    OBD_GET_GENDER_SPEC_CHK,
    OBD_GET_DATA_REQ,
    OBD_GET_DATA_CHK,
    OBD_SET_DATA_REQ
} _obd_at_status;

typedef enum
{
  obd_off,
  obd_on,
  obd_ready,
  obd_delay,
  obd_data_check,
  obd_reset,
  obd_get_data
}_obd_status;

typedef struct
{
  char    buffer[256];
  uint32_t orawcnt;
}_obd_raw;

typedef struct
{
    char    status;
    unsigned long   raw_speed;
    unsigned int     raw_rpm;
    unsigned int   raw_pedal;
    int                         raw_temp_oil;
    int                         raw_temp_indoor;
    int                         raw_temp_outdoor;
    int                         raw_temp_coolant;
    unsigned int    raw_maf;
    unsigned int    raw_amp;
    unsigned long raw_torque;
    unsigned int    raw_batt_volt;
//    char        odcnt;        // ?òÌîåÍ∞?àò Ï§ëÏóê ?†Ìö®??Í∞?àò 
}_obd_data;

typedef struct
{
    char    status;
    char    speed[3];
    char    engine_rpm[4];
    char    gas_pedal[3];
    char    temp_engine_oil[5];
    char    temp_intake_air[5];
    char    temp_outdoor_air[5];
    char    temp_coolant[5];
    char    air_volume[4];
    char    atmospheric_pressure[3];
    char    engine_torque[4];
    char    voltage_vehicle[3];
}_obd_tx_data; // sizeof(_obd_tx_data) = 45

typedef enum { mSEC, SEC }time_def;

typedef struct
{
	uint16_t   mSec;
	uint32_t   Sec;           // ?úÏä§???úÍ∞Ñ 
	uint32_t   Sec2;          // ?µÏã† ?úÏûë ?¥ÌõÑ ?ºÏã± Í≤ΩÍ≥º ?úÍ∞Ñ ?êÎã® Î≥Ä??
	uint32_t   delay;         // GPS ?ºÏã± ?ÄÍ∏?Î∞?Í≤ΩÍ≥º ?úÍ∞Ñ ?êÎã®Î≥Ä??
	uint8_t    delay_mode;
	uint32_t   timeout;
	uint8_t    timeout_mode;
	uint32_t   SystemTick;           // ?úÏä§???úÍ∞Ñ 
	BYTE       check;
}_time_t;

typedef struct {
  uint8_t previous;
  uint8_t now;
  uint8_t next;
} _Status_Def;

typedef struct
{
  _time_t     *time;
  _obd_raw    *oraw;
  _Status_Def *status;
 _obd_at_status at_exe;
 _obd_data  *odata;
 uint32_t      odata_cnt;       //obd ?òÌîå Ïπ¥Ïö¥????
 _obd_data *odata_backup;
 uint32_t odata_backup_cnt;
}_obd_def;

typedef struct _genderspec {
	BYTE 	serial_no[8];		// ?†Îçî ?úÎ¶¨??
	BYTE	sw_ver[2];			// ?†Îçî ?åÌîÑ?∏Ïõ®??Î≤ÑÏ†Ñ
	BYTE	vehicle_no[12];		// Ï∞®Îüâ Î≤àÌò∏
	BYTE	vin_info[17];		// Ï∞®Î? Î≤àÌò∏ 
	BYTE 	vehicle_type[8];		// Ï∞®Ï¢Ö 
	BYTE	oil_type;			// ?†Ï¢Ö 
	BYTE	made_year[2];			// Ï∞®Îüâ ?∞Ïãù
	BYTE	engine_displacement[2];	// Î∞∞Í∏∞??	BYTE	cylinder;				// Í∏∞ÌÜµ??
	BYTE	bt_mac[12];					// BT MAC
	BYTE 	accumulation_driving_distance[8];		// ?ÑÏ†Å Ï£ºÌñâ Í±∞Î¶¨
	BYTE	accumulation_fuel_quantity[8];		//?ÑÏ†Å?∞Î°ú?åÎ™®??	BYTE	gender_attached;						// ?àÎ?Ï∞??†Î¨¥
	BYTE	ignition_check;						// ?úÎèô ?®Ïò§??	BYTE	year[2];			//?∞ÎèÑ
	BYTE	month;				// ??
	BYTE 	day;				// ??
	BYTE	date;				// ?îÏùº
	BYTE	am_pm;				// ?§Ï†Ñ ??	BYTE	hour;				// ??	BYTE	min;				// Î∂?
	BYTE	sec;				// Ï¥?
}GENDER_SPEC;


typedef struct _rnudata {
	BYTE	accumulation_driving_distance[8];		// ?ÑÏ†Å ?¥Ìñâ Í±∞Î¶¨ 
	BYTE	speed[4];								// Ï∞®ÏÜç
	BYTE	acc_fuel_volume[4];						// ?ÑÏ†Å?∞Î£å?åÎ™®??
	BYTE	trip_efficiency[2];					//?∏Î¶Ω ?∞ÎπÑ
	BYTE	spot_fuel_spread[4];				// ?úÍ∞Ñ?∞Î£åÎ∂ÑÏÇ¨??
	BYTE	rpm[2];								// RPM
	BYTE	break_signal;						// Î∏åÎ†à?¥ÌÅ¨?†Ìò∏ 
	BYTE	key_plus;							// KEY+
	BYTE	gear_lever;							// Î≥Ä?çÎ†àÎ≤?
	BYTE	gear_box;							// Î≥Ä?çÎã® 
	BYTE	throttle_position;					// Í∞Ä?çÌéò???§Î°ú?Ä?¨Ï???
	BYTE	engine_oil_temperature[3];			// ?îÏßÑ?§Ïùº?®ÎèÑ 
	BYTE	inhalation_temperature[3];			// ?°Í∏∞?®ÎèÑ 
	BYTE	exhaust_temperature[3];				// ?∏Í∏∞?®ÎèÑ
	BYTE	coolant_temperature[3];				// ?âÍ∞Å?òÏò®??
	BYTE	maf[2];								// MAF (?°ÏûÖÍ≥µÍ∏∞??
	BYTE	amp[2];								// ?ÄÍ∏∞Ïïï 
	BYTE	engine_torque[4];					// ?îÏßÑ?†ÌÅ¨ 
	BYTE	battery_voltage[2];					// Î∞∞ÌÑ∞Î¶¨Ï†Ñ??
	BYTE	air_guage_volt[2];					// ?êÏñ¥Í≤åÏù¥ÏßÄ?ÑÏïï 
	BYTE	fuel_guage_volt[2];					// ?∞Î°úÍ≤åÏù¥ÏßÄ?ÑÏïï 
	BYTE	fuel_remain;							// ?∞Î£å?îÎüâ 
	BYTE	reserved[8];							// ?àÏïΩ 
}RNU_DATA;

char    Get_OBD_Data(_obd_data *obd_data, _obd_tx_data *obd_tx_data);
char    Get_OBD_Data_Null( _obd_tx_data *obd_tx_data);
void    Copy_OBD_Data(_obd_data *raw, _obd_data *backup);
//void    Control_OBD(_obd_def *obd, _event_flag_t *event);
void    Enable_OBD_Signal(void);

#define IDX_STX         0x00
#define IDX_ITEM       0x01
#define IDX_CMD          0x02
#define IDX_LEN          0x03
#define IDX_DATA       0x04
void obd_error(int error)
{
	if (error == 100) {
		printf("Not Ready Vehicle Initialize\r\n");
	}
	else if (error == 101) {
		printf("Not Ready OBD Initialize\r\n");
	}
	else if (error == 102) {
		printf("???úÎèô Í∞êÏ?\r\n");
	}
	else if (error == 200) {
		printf("?úÎèô On ?ÅÌÉú(busy)\r\n");
	}
	else{
		printf("error%d\r\n", error);
	}

	return;
}
char  GenderSpecParse(char *buffer)
{
	GENDER_SPEC _gender_spec;
	char        *temp_addr, length, retValue = 0;
	unsigned char   chksum = 0, temp_chksum = 0;

	//unsigned long       fuel=0;

	if ((temp_addr = strchr(buffer, '<')) != NULL)
	{
		if ((*(temp_addr + IDX_ITEM) == 'A') && (*(temp_addr + IDX_CMD) == 'G'))
		{
			if (*(temp_addr + IDX_LEN) == 0x5D)
			{
				length = *(temp_addr + IDX_LEN);
				chksum = *(temp_addr + IDX_LEN + length);

				printf("GENDER_SPEC=%ld\r\n", sizeof(GENDER_SPEC));
				printf("length = %d \r\n", length);

				for (int i = 0; i<length - 1; i++)  // Í∏∏Ïù¥ ?∞Ïù¥?∞Ïóê??Ï≤¥ÌÅ¨??Í∞?àòÍ∞Ä ?¨Ìï®?òÏñ¥ ?àÏùå 
					temp_chksum += *(temp_addr + IDX_DATA + i);

				if (temp_chksum != chksum)
				{
					printf("check sum error!!(temp=%d, chs %d)\r\n", temp_chksum, chksum);
					retValue = 0;
					return retValue;
				}

				printf("temp=%d, chs %d)\r\n", temp_chksum, chksum);

				memcpy(&_gender_spec, temp_addr + IDX_DATA, sizeof(_gender_spec));

				//printf("_gender_spec.accumulation_driving_distance=%ld\r\n", (unsigned long)_gender_spec.accumulation_driving_distance);
				//printf("_gender_spec.gender_attached=%ld\r\n", (unsigned long)_gender_spec.accumulation_fuel_quantity);
				//printf("_gender_spec.gender_attached=%x\r\n", _gender_spec.gender_attached);
				//printf("_gender_spec.gender_attached=%x\r\n", _gender_spec.ignition_check);

				//if(_gender_spec.gender_attached)
				//    obd->status = 0;                     //0:?∞Í≤∞ 1:Ï∞®Îã® 2:ÎØ∏Ï???

				//
				//if(_gender_spec.ignition_check)
				//    obd->odata->status = 2;

				//for(int i=0; i<length; i++) printf(" %02x ", *(temp_addr+i));

				retValue = 1;
			}

		}
		else if ((*(temp_addr + IDX_ITEM) == 'A') && (*(temp_addr + IDX_CMD) == 'F')){
			int error = 0;
			printf("error\r\n");
			error = (*(temp_addr + IDX_LEN) - 0x30) * 100 + (*(temp_addr + IDX_LEN + 1) - 0x30) * 10 + (*(temp_addr + IDX_LEN + 2) - 0x30) * 1;
			obd_error(error);
		}


	}
//for(int j=0;j<16;j++){ printf("%x ", buffer[j]);}
	return retValue;
}

void HexToAscii_Make(char *str, unsigned long val, unsigned char len)
{
	unsigned char i, loop;

	switch (len) {
	case LEN_1000000000:
		loop = 1;
		i = 0;
		while (loop) {
			if (val > 999999999) {
				val -= 1000000000;
				i++;
			}
			else {
				*str = i + 0x30;
				str++;
				loop = 0;
			}
		}
		// NO Break
	case LEN_100000000:
		loop = 1;
		i = 0;
		while (loop) {
			if (val > 99999999) {
				val -= 100000000;
				i++;
			}
			else {
				*str = i + 0x30;
				str++;
				loop = 0;
			}
		}
		// NO Break
	case LEN_10000000:
		loop = 1;
		i = 0;
		while (loop) {
			if (val > 9999999) {
				val -= 10000000;
				i++;
			}
			else {
				*str = i + 0x30;
				str++;
				loop = 0;
			}
		}
		// NO Break
	case LEN_1000000:
		loop = 1;
		i = 0;
		while (loop) {
			if (val > 999999) {
				val -= 1000000;
				i++;
			}
			else {
				*str = i + 0x30;
				str++;
				loop = 0;
			}
		}
		// NO Break
	case LEN_100000:
		loop = 1;
		i = 0;
		while (loop) {
			if (val > 99999) {
				val -= 100000;
				i++;
			}
			else {
				*str = i + 0x30;
				str++;
				loop = 0;
			}
		}
		// NO Break
	case LEN_10000:
		loop = 1;
		i = 0;
		while (loop) {
			if (val > 9999) {
				val -= 10000;
				i++;
			}
			else {
				*str = i + 0x30;
				str++;
				loop = 0;
			}
		}
		// NO Break
	case LEN_1000:
		loop = 1;
		i = 0;
		while (loop) {
			if (val > 999) {
				val -= 1000;
				i++;
			}
			else {
				*str = i + 0x30;
				str++;
				loop = 0;
			}
		}
		// NO Break
	case LEN_100:
		loop = 1;
		i = 0;
		while (loop) {
			if (val > 99) {
				val -= 100;
				i++;
			}
			else {
				*str = i + 0x30;
				str++;
				loop = 0;
			}
		}
		// NO Break
	case LEN_10:
		loop = 1;
		i = 0;
		while (loop) {
			if (val > 9) {
				val -= 10;
				i++;
			}
			else {
				*str = i + 0x30;
				str++;;
				loop = 0;
			}
		}
		// NO Break
	case LEN_1:
		*str = val + 0x30;
		break;
	}
}

#define ABS(x) ((x) < 0 ? (-(x)) : (x))

char Get_OBD_Data(_obd_data *obd_data, _obd_tx_data *obd_tx_data)
{
	// ?Ä?•Îêò???àÎäî OBD ?ïÎ≥¥Î•??ÑÏä§?§Ìòï?ºÎ°ú Î≥Ä?òÌïò??TxÎ≤ÑÌçº???Ä??	//uint8_t i;
	 int    speed, rpm, accel, maf, amp, torque, volt;
	int                         oil_temp, in_temp =0, out_temp =0, coolant_temp = 0;

	//?ÑÏä§?§Î°ú Î≥Ä?òÏùÑ ?ÑÌï¥ 
	obd_tx_data->status = obd_data->status + 0x30;

	//for(i=0; i<obd_data->cnt; i++):
	{
		// 1.Ï∞®ÏÜç 
		if (obd_data->raw_speed > 1000)
			speed = (unsigned int)(obd_data->raw_speed / 1000);
		else
			speed = 0; //(unsigned int)obd_data->raw_speed ;

		//printf("%d, %lu[km/h]", speed, obd_data->raw_speed);
		printf("%d[km/h]", speed);
		HexToAscii_Make(&obd_tx_data->speed[0], speed, LEN_100);
		//printf("%c %c %c [km/h]\r\n",obd_tx_data->speed[0], obd_tx_data->speed[1], obd_tx_data->speed[2]);

		// 2. RPM
		rpm = obd_data->raw_rpm;
		HexToAscii_Make(&obd_tx_data->engine_rpm[0], rpm, LEN_1000);
		printf("\t%4d [RPM]", rpm );

		// 3.Í∞Ä?çÌéò??
		accel = obd_data->raw_pedal;
		HexToAscii_Make(&obd_tx_data->gas_pedal[0], accel, LEN_100);
		//printf("\t%3d [%%]", accel);

		// 4.?îÏßÑ?§Ïùº?®ÎèÑ
		oil_temp = obd_data->raw_temp_oil;

		if (oil_temp >= 9999)
			oil_temp = 9999;

		if (oil_temp<0)
			obd_tx_data->temp_engine_oil[0] = '-';
		else
			obd_tx_data->temp_engine_oil[0] = '+';

		HexToAscii_Make(&obd_tx_data->temp_engine_oil[1], ABS(oil_temp), LEN_1000);
		//printf("\t%4d['C]", oil_temp);

		// 5.?°Í∏∞?®ÎèÑ 
		in_temp = obd_data->raw_temp_indoor;

		if (in_temp >= 9999)
			in_temp = 9999;

		if (in_temp<0)
			obd_tx_data->temp_intake_air[0] = '-';
		else
			obd_tx_data->temp_intake_air[0] = '+';

		HexToAscii_Make(&obd_tx_data->temp_intake_air[1], ABS(in_temp), LEN_1000);
		//printf("\t%4d['C]", in_temp);

		// 6.?∏Í∏∞?®ÎèÑ
		out_temp = obd_data->raw_temp_outdoor;

		if (out_temp >= 9999)
			out_temp = 9999;

		if (out_temp<0)
			obd_tx_data->temp_outdoor_air[0] = '-';
		else
			obd_tx_data->temp_outdoor_air[0] = '+';

		HexToAscii_Make(&obd_tx_data->temp_outdoor_air[1], ABS(out_temp), LEN_1000);
		//printf("\t%4d['C]", out_temp);

		// 7. ?âÍ∞Å?òÏò®??		coolant_temp = obd_data->raw_temp_coolant;

		if (coolant_temp >= 9999)
			coolant_temp = 9999;

		if (coolant_temp<0)
			obd_tx_data->temp_coolant[0] = '-';
		else
			obd_tx_data->temp_coolant[0] = '+';

		HexToAscii_Make(&obd_tx_data->temp_coolant[1], ABS(coolant_temp), LEN_1000);
		//printf("\t%4d['C]", coolant_temp);

		// 8. MAF(Í≥µÍ∏∞??
		maf = obd_data->raw_maf;
		if (maf >= 9999)
			maf = 9999;

		HexToAscii_Make(&obd_tx_data->air_volume[0], maf, LEN_1000);
		//printf("\t%d [mg/TDC]", maf);

		// 9. AMP(?ÄÍ∏∞Ïïï)
		amp = obd_data->raw_amp;
		if (amp >= 999)
			amp = 999;

		HexToAscii_Make(&obd_tx_data->atmospheric_pressure[0], amp, LEN_100);
		//printf("\t%4d [kPa]", amp);

		// 10. ?îÏßÑ?†ÌÅ¨
		torque = (unsigned int)obd_data->raw_torque;
		if (torque >= 9999)
			torque = 9999;

		HexToAscii_Make(&obd_tx_data->engine_torque[0], torque, LEN_1000);
		//printf("\t%4d [%%]", torque);

		// 11. Î∞∞ÌÑ∞Î¶¨Ï†Ñ??
		volt = obd_data->raw_batt_volt;
		if (volt >= 999)
			volt = 999;

		HexToAscii_Make(&obd_tx_data->voltage_vehicle[0], volt, LEN_100);
		printf("\t%3d [V]", volt);

		// ?ÑÏä§?§Î°ú Î≥Ä?òÎêú ?∞Ïù¥??
		//        printf("\t");
		//        for(int i=0; i<sizeof(_obd_tx_data); i++)
		//            printf("%c", *(obd_tx_data->speed + i));
		 printf("cnt=%d\r\n", obd_cnt++);

	}
	httppost_obd(speed, volt, rpm, obd_cnt);
	
	return 1;
}

char  Check_OBD_Data(char *buffer, _obd_data *obd_data)
{
	RNU_DATA _run_data;
	//char    *addr = (char *)obd_raw->buffer;
	char        *temp_addr;
	char        length, retValue = 0;
	unsigned long   temp_l;
	unsigned int    temp_i, rpm, maf, amp, volt;
	unsigned char   symbol;
	//int                         oil_temp, in_temp, out_temp, coolant_temp;
	unsigned char   chksum = 0, temp_chksum = 0;
	memset(obd_data, 0, sizeof(_obd_data));
	if ((temp_addr = strchr(buffer, '<')) != NULL)
	{
		if ((*(temp_addr + IDX_ITEM) == 'A') && (*(temp_addr + IDX_CMD) == 'C'))
		{
			if (*(temp_addr + IDX_LEN) == 0x41)
			{
				length = *(temp_addr + IDX_LEN);

				printf("length = %d \r\n", length);
				chksum = *(temp_addr + IDX_LEN + length);

				for (int i = 0; i<length - 1; i++)  // Í∏∏Ïù¥ ?∞Ïù¥?∞Ïóê??Ï≤¥ÌÅ¨??Í∞?àòÍ∞Ä ?¨Ìï®?òÏñ¥ ?àÏùå 
					temp_chksum += *(temp_addr + IDX_DATA + i);

				if (temp_chksum != chksum)
				{
					printf("check sum error!!(temp=%d, chs %d)\r\n", temp_chksum, chksum);
					retValue = 0;
					return retValue;
				}

				printf("temp=%d, chs %d\r\n", temp_chksum, chksum);

				memcpy(&_run_data, temp_addr + IDX_DATA, sizeof(RNU_DATA));

				//for(int i=0; i<length; i++)  printf(" %2x ", *(temp_addr+i));

				// 1. Ï∞®ÏÜç 
				temp_l = ((unsigned long)_run_data.speed[0]) << 24;
				temp_l |= ((unsigned long)_run_data.speed[1]) << 16;
				temp_l |= ((unsigned long)_run_data.speed[2]) << 8;
				temp_l |= ((unsigned long)_run_data.speed[3]) << 0;
				obd_data->raw_speed = temp_l;

				// 2. RPM              
				rpm = (unsigned int)_run_data.rpm[0] << 8;
				rpm |= (unsigned int)_run_data.rpm[1];
				obd_data->raw_rpm = rpm;

				// 3. Í∞Ä???òÎã¨ 
				obd_data->raw_pedal = (unsigned int)_run_data.throttle_position;

				// 4. ?îÏßÑ?§Ïùº?®ÎèÑ
				temp_i = 0;
				symbol = _run_data.engine_oil_temperature[0];
				temp_i = ((unsigned int)_run_data.engine_oil_temperature[1]) << 8;
				temp_i |= ((unsigned int)_run_data.engine_oil_temperature[2]) << 0;

				if (!symbol)
				{
					obd_data->raw_temp_oil = temp_i;
				}
				else
				{
					obd_data->raw_temp_oil = temp_i*(-1);
				}

				// 5. ?°Í∏∞?®ÎèÑ
				temp_i = 0;
				symbol = _run_data.inhalation_temperature[0];
				temp_i = ((unsigned int)(_run_data.inhalation_temperature[1]) << 8);
				temp_i |= ((unsigned int)(_run_data.inhalation_temperature[2]) << 0);

				if (!symbol)
				{
					obd_data->raw_temp_indoor = temp_i;
				}
				else
				{
					obd_data->raw_temp_indoor = temp_i*(-1);
				}

				// 6. ?∏Í∏∞?®ÎèÑ 
				temp_i = 0;
				symbol = _run_data.exhaust_temperature[0];
				temp_i |= ((unsigned int)_run_data.exhaust_temperature[1]) << 8;
				temp_i |= ((unsigned int)_run_data.exhaust_temperature[2]) << 0;

				if (!symbol)
				{
					obd_data->raw_temp_outdoor = temp_i;
				}
				else
				{
					obd_data->raw_temp_outdoor = temp_i*(-1);
				}

				// 7. ?âÍ∞Å???®ÎèÑ
				temp_i = 0;
				symbol = _run_data.coolant_temperature[0];
				temp_i |= ((unsigned int)(_run_data.coolant_temperature[1]) << 8);
				temp_i |= ((unsigned int)(_run_data.coolant_temperature[2]) << 0);

				if (!symbol)
				{
					obd_data->raw_temp_coolant = temp_i;
				}
				else
				{
					obd_data->raw_temp_coolant = temp_i*(-1);
				}

				// 8. MAF
				maf = (unsigned int)_run_data.maf[0] << 8;
				maf |= (unsigned int)_run_data.maf[1];
				obd_data->raw_maf = maf;

				// 9. ?ÄÍ∏∞Ïïï 
				amp = (unsigned int)_run_data.amp[0] << 8;
				amp |= (unsigned int)_run_data.amp[1];
				obd_data->raw_amp = amp;

				// 10. ?îÏßÑ?†ÌÅ¨
				temp_l = 0;
				temp_l = ((unsigned long)(_run_data.engine_torque[0]) << 24);
				temp_l |= ((unsigned long)(_run_data.engine_torque[1]) << 16);
				temp_l |= ((unsigned long)(_run_data.engine_torque[2]) << 8);
				temp_l |= ((unsigned long)(_run_data.engine_torque[3]) << 0);
				obd_data->raw_torque = temp_l;

				// 11. Î∞∞ÌÑ∞Î¶¨Ï†Ñ??
				volt = (unsigned int)_run_data.battery_voltage[0] << 8;
				volt |= (unsigned int)_run_data.battery_voltage[1];
				obd_data->raw_batt_volt = volt;


				// RPM?ºÎ°ú ?úÎèô ?†Î¨¥Î•??êÎã® 
				if ((obd_data->raw_rpm>100) && (obd_data->raw_rpm<0xFFFF))
				{
					//                    obd_data->odcnt++;      // obd ?òÌîå Ï§??†Ìö®??Í∞?àò Ï¶ùÍ? 
					//                    printf("obd cnt = %d \r\n", obd_data->odcnt);
					retValue = 1;
				}
			}
		}
		else if ((*(temp_addr + IDX_ITEM) == 'A') && (*(temp_addr + IDX_CMD) == 'F')) {
			int error = 0;
			error = (*(temp_addr + IDX_LEN) - 0x30) * 100 + (*(temp_addr + IDX_LEN + 1) - 0x30) * 10 + (*(temp_addr + IDX_LEN + 2) - 0x30) * 1;
			obd_error(error);
		}
		else{
			printf("error\r\n");
		}
	}
return retValue;
}

int init_uart(char * dev, int baud, int * fd)
{
    struct termios newtio;
    * fd = open(dev, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if ( * fd < 0) {
        printf("%s> uart dev '%s' open fail [%d]n", __func__, dev, * fd);
        return -1;
    }
    memset( & newtio, 0, sizeof(newtio));
    newtio.c_iflag = IGNPAR; // non-parity 
    newtio.c_oflag = 0;//OPOST | ONLCR;;
    newtio.c_cflag = CS8 | CLOCAL | CREAD; // NO-rts/cts 
    switch (baud)
    {
    case 115200:
      newtio.c_cflag |= B115200;
      break;
    case 57600:
      newtio.c_cflag |= B57600;
      break;
    case 38400:
      newtio.c_cflag |= B38400;
      break;
    case 19200:
      newtio.c_cflag |= B19200;
      break;
    case 9600:
      newtio.c_cflag |= B9600;
      break;
    case 4800:
      newtio.c_cflag |= B4800;
      break;
    case 2400:
      newtio.c_cflag |= B2400;
      break;
    default:
      newtio.c_cflag |= B115200;
      break;
    }

    newtio.c_lflag = 0;
    //newtio.c_cc[VTIME] = vtime; // timeout 0.1Ï¥??®ÏúÑ 
    //newtio.c_cc[VMIN] = vmin; // ÏµúÏÜå n Î¨∏Ïûê Î∞õÏùÑ ?åÍπåÏß??ÄÍ∏?
    newtio.c_cc[VTIME] = 0;
    newtio.c_cc[VMIN] = 0;
    tcflush( * fd, TCIFLUSH);
    tcsetattr( * fd, TCSANOW, & newtio);
    return 0;
}

void *test_read_loop(void * arg)
{
    int result;
    char buffer[MAX_BUF_SIZE];
    fd_set reads, temps;

    FD_ZERO( & reads);
    FD_SET(fd, & reads);

    while (1)
    {
        temps = reads;
        result = select(FD_SETSIZE, & temps, NULL, NULL, NULL);

        if (result < 0){
            exit(EXIT_FAILURE);
        }

        if (FD_ISSET(fd, & temps)){
            memset(buffer, 0, sizeof(buffer));
            
            if (read(fd, buffer, MAX_BUF_SIZE) == -1){
                continue;
            }
            
            printf("%s", buffer);
        }
    }
}
int charToInt(char c)
{
	return (c - 48);
}

char int2char(char ch)
{
  if( 0<= ch || ch <= 9){
    return ch + 0x30;
  }
  else{
    return ch + 0x31;
  }
}

double prvlat = 0.0, prvlon = 0.0;

static double haversine(double lat1, double lon1, double lat2, double lon2)
{
    // distance between latitudes
    // and longitudes
    double dLat = (lat2 - lat1) *  M_PI / 180.0;
    double dLon = (lon2 - lon1) *  M_PI / 180.0;

    // convert to radians
    lat1 = (lat1) * M_PI / 180.0;
    lat2 = (lat2) * M_PI / 180.0;

    // apply formulae
    double a = pow(sin(dLat / 2), 2) +
	       pow(sin(dLon / 2), 2) *
	       cos(lat1) * cos(lat2);
    double rad = 6371000;
    double c = 2 * asin(sqrt(a));
    
    printf("diff: %f\r\n", rad * c);
    
    return rad * c;
}
void searching()
{
    char upper, lower;

    upper = int2char((parser.checksum & 0xF0)>>4);
    lower = int2char((parser.checksum & 0x0F)>>0);

    if ((upper != parser.szChecksum[0]) && (lower != parser.szChecksum[1])) {
	return;
    }

    if (strcmp((char*)&parser.words[0], "$GPGGA") == 0) {
	parser.flagDetect = TRUE;
	// Check GPS Fix: 0=no fix, 1=GPS fix, 2=Dif. GPS fix
	if (parser.words[6][0] == '0') {
	    //_CLR_FLAG_(status, GPS_FIX_FLAG);
	    parser.flagTIMEDataReady = FALSE;
  	    digitalWrite(LED_RED, 1);
	    digitalWrite(LED_GREEN, 0);
	    digitalWrite(LED_YELLOW, 0);

	    return;
	}
	else if(parser.words[6][0] == '4') {
	//else{
	    // parse latitude and longitude in NMEA format
	    parser.latitude = strtod((char*)&parser.words[2], NULL);
	    parser.longitude = strtod((char*)&parser.words[4], NULL);

	    // get decimal format
	    if (parser.words[3][0] == 'S') parser.latitude  *= -1.0;
	    if (parser.words[5][0] == 'W') parser.longitude *= -1.0;
	    double degrees = trunc(parser.latitude / 100.0f);
	    double minutes = parser.latitude - (degrees * 100.0f);
	    parser.latitude = degrees + minutes / 60.0f;

	    degrees = trunc(parser.longitude / 100.0f);
	    minutes = parser.longitude - (degrees * 100.0f);
	    parser.longitude = degrees + minutes / 60.0f;

	    // data ready
	    parser.flagTIMEDataReady = TRUE;
	    
	    //printf("lat = %f, lon = %f\r\n", parser.latitude, parser.longitude);
	    
	    //haversine(parser.latitude, parser.longitude, prvlat, prvlon);
	    //httppost(ch, parser.latitude, parser.longitude);
	    
	    //prvlat = parser.latitude;
	    //prvlon = parser.longitude;
	    digitalWrite(LED_RED, 0);
	    digitalWrite(LED_GREEN, 1);
	    digitalWrite(LED_YELLOW, 0);
	}
	else{
	    digitalWrite(LED_RED, 0);
	    digitalWrite(LED_GREEN, 0);
	    digitalWrite(LED_YELLOW, 1);
	    printf("%c", parser.words[6][0]);
	}

	// parse time
	parser.UTCHour = charToInt(parser.words[1][0]) * 10 + charToInt(parser.words[1][1]);
	parser.UTCMin = charToInt(parser.words[1][2]) * 10 + charToInt(parser.words[1][3]);
	parser.UTCSec = charToInt(parser.words[1][4]) * 10 + charToInt(parser.words[1][5]);

	if(parser.flagTIMEDataReady == TRUE && parser.flagUTCDataReady == TRUE){
	    //_SET_FLAG_(status, GPS_FIX_FLAG);
	    printf("%d:%d:%d\r\n", parser.UTCHour, parser.UTCMin, parser.UTCSec);
	}
    }//$GPGGA

    if (strcmp((char*)&parser.words[0], "$GPRMC") == 0) {
	parser.flagDetect = TRUE;

	// Check data status: A-ok, V-invalid
	if ( parser.words[2][0] == 'V') {
	     parser.flagUTCDataReady = FALSE;
	    return;
	}

	// parse UTC date
	 parser.UTCDay = charToInt( parser.words[9][0]) * 10 + charToInt( parser.words[9][1]);
	 parser.UTCMonth = charToInt( parser.words[9][2]) * 10 + charToInt( parser.words[9][3]);
	 parser.UTCYear = charToInt( parser.words[9][4]) * 10 + charToInt( parser.words[9][5]);

	// data ready
	 parser.flagUTCDataReady = TRUE;
	 //printf("%d-%d-%d\r\n", parser.UTCDay, parser.UTCMonth, parser.UTCYear);
    }//$GPRMC
}
void parsing(uint8_t ch)
{
    if (ch == '$'){
	parser.flagRead = TRUE;
	// init parser vars
	parser.flagComputedCks = FALSE;
	parser.checksum = 0;
	// after getting  * we start cuttings the received m_nChecksum
	parser.flagReceivedCks = FALSE;
	parser.index_received_checksum = 0;
	// word cutting variables
	parser.wordIdx = 0; parser.prevIdx = 0; parser.nowIdx = 0;
    }

    if (parser.flagRead) {
	// check ending
	if (ch == '\r' || ch == '\n') {
		// catch last ending item too
		parser.words[parser.wordIdx][parser.nowIdx - parser.prevIdx] = 0;
		parser.wordIdx++;
		// cut received m_nChecksum
		parser.szChecksum[parser.index_received_checksum] = 0;
		// sentence complete, read done
		parser.flagRead = FALSE;
		// parse
		//GPS_setDrvParam(&GPS, GPS_SET_PARSER, 0);
		searching();
		
		
		
	} else {
		// computed m_nChecksum logic: count all chars between $ and * exclusively
		if (parser.flagComputedCks && ch == '*') parser.flagComputedCks = FALSE;
		if (parser.flagComputedCks) parser.checksum ^= ch;
		if (ch == '$') parser.flagComputedCks = TRUE;
		// received m_nChecksum
		if (parser.flagReceivedCks){
		    parser.szChecksum[parser.index_received_checksum] = ch;
		    parser.index_received_checksum++;
		}
		
		if (ch == '*'){
		    parser.flagReceivedCks = TRUE;
		}

		// build a word
		parser.words[parser.wordIdx][parser.nowIdx - parser.prevIdx] = ch;
		if (ch == ',') {
		    parser.words[parser.wordIdx][parser.nowIdx - parser.prevIdx] = 0;
		    parser.wordIdx++;
		    parser.prevIdx = parser.nowIdx;
		}
		else {
		    parser.nowIdx++;
		}
	}
    }    
}

void httpprint(char *str)
{
    printf("%s",str);
}

int httppost(char ch, double lat, double lon)
{
    CURL *curl;
    CURLcode res;
    char temp[64] = {0,};
#if 0
    httpprint("{\r\n    ");
    sprintf(temp, "\"lat2\":\"%f\",\r\n    ", lat);
    httpprint(temp);
    memset(temp, 0, 64);
    sprintf(temp, "\"lon2\":\"%f\"\r\n}", lon);
    httpprint(temp);
#else    
    curl = curl_easy_init();
    
    if(curl) {
	curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
	
	if(ch==1){
	    sprintf(temp, "http://%s:5000/pos1", ipaddr);
	    curl_easy_setopt(curl, CURLOPT_URL, temp);
	}
	else{
	    sprintf(temp, "http://%s:5000/pos2", ipaddr);
	    curl_easy_setopt(curl, CURLOPT_URL, temp);
	}
	
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_DEFAULT_PROTOCOL, "https");

	struct curl_slist *headers = NULL;
	headers = curl_slist_append(headers, "Content-Type: application/json");
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	//const char *data = "{\r\n    \"lat2\":\"1234.5678\",\r\n    \"lon2\":\"1234.5678\"\r\n}";
	sprintf(temp, "{\r\n    \"lat\":\"%f\",\r\n    \"lon\":\"%f\"\r\n}", lat,lon);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, temp);
	res = curl_easy_perform(curl);
    }
    curl_easy_cleanup(curl);
#endif
    return 1;
}

struct memory {
   char *response;
   size_t size;
 };
 
static size_t cb(void *data, size_t size, size_t nmemb, void *userp)
 {
   size_t realsize = size * nmemb;
   struct memory *mem = (struct memory *)userp;
 
   char *ptr = realloc(mem->response, mem->size + realsize + 1);
   if(ptr == NULL)
     return 0;  /* out of memory! */
 
   mem->response = ptr;
   memcpy(&(mem->response[mem->size]), data, realsize);
   mem->size += realsize;
   mem->response[mem->size] = 0;
 
   return realsize;
 }

int httpget(double *x1, double *y1, double *x2, double *y2)
{
    CURL *curl;
    CURLcode res;
    struct memory chunk = {0};
    int ret = 0;
    char url[36] = {0,};
    
    curl = curl_easy_init();

    if(curl) {
	curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "GET");
	
	sprintf(url, "http://%s:5000/getpos", ipaddr);
	//curl_easy_setopt(curl, CURLOPT_URL, "http://192.168.0.125:5000/getpos");
	curl_easy_setopt(curl, CURLOPT_URL, url);
	
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_DEFAULT_PROTOCOL, "https");

	/* send all data to this function  */
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, cb);
	 
	/* we pass our 'chunk' struct to the callback function */
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
	
	struct curl_slist *headers = NULL;
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	res = curl_easy_perform(curl);
	
	if(CURLE_OK == res) {
	    //char *ct;
	    long response_code;
	    /* ask for the content-type */
	    res = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
 
	    if((CURLE_OK == res)){
		printf("[%ld]\r\n", response_code);
	//	printf("%s, %d \r\n", chunk.response, chunk.size);

		char *addr = NULL;
		char comma[4] = {0,}, buffer[16] = {0,};

		for(int i=0,j=0; i<chunk.size; i++){
		    if(chunk.response[i] == ','){
			comma[j] = i;
			j++;
		    }
		}
		addr = strstr(chunk.response, "@");
		if(addr != NULL){
		    memcpy(buffer, (addr+1), comma[0]-1);
		    //printf("x1=%s,%f\r\n", buffer, atof(buffer));
		    *x1 = atof(buffer);
		    memset(buffer, 0, 16);
		    memcpy(buffer, (addr+comma[0]+1), comma[1]-comma[0]-1);
		    //printf("y1=%s,%f\r\n", buffer, atof(buffer));
		    *y1 = atof(buffer);
		
		
		
		    memset(buffer, 0, 16);
		    memcpy(buffer, (addr+comma[1]+1), comma[2]-comma[1]-1);
		    //printf("x2=%s,%f\r\n", buffer, atof(buffer));
		    *x2 = atof(buffer);
		    memset(buffer, 0, 16);
		    memcpy(buffer, (addr+comma[2]+1), comma[3]-comma[2]-1);
		    //printf("y2=%s,%f\r\n", buffer, atof(buffer));
		    *y2 = atof(buffer);
	    
		    ret = 1;
		    
		}
	    }
	}
    }
    curl_easy_cleanup(curl);

    return ret;
}

int httppost_obd(int speed, int batt, int rpm, int cnt)
{
    CURL *curl;
    CURLcode res;
    char temp[128] = {0,};
#if 0
    httpprint("{\r\n    ");
    sprintf(temp, "\"lat2\":\"%f\",\r\n    ", lat);
    httpprint(temp);
    memset(temp, 0, 64);
    sprintf(temp, "\"lon2\":\"%f\"\r\n}", lon);
    httpprint(temp);
#else    
    curl = curl_easy_init();
    
    if(curl) {
	curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
	
    	sprintf(temp, "http://%s:5000/obd", ipaddr);
	curl_easy_setopt(curl, CURLOPT_URL, temp);
	
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_DEFAULT_PROTOCOL, "https");

	struct curl_slist *headers = NULL;
	headers = curl_slist_append(headers, "Content-Type: application/json");
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	//const char *data = "{\r\n    \"lat2\":\"1234.5678\",\r\n    \"lon2\":\"1234.5678\"\r\n}";
	//sprintf(temp, "{\r\n	\"spd\":\"%d\",\r\n		\"bat\":\"%.1f\",\r\n	\"rpm\":\"%d\",r\n		\"cnt\":\"%d\"\r\n}", speed, (float)batt/10, rpm, cnt);
	sprintf(temp, "{\r\n	\"spd\": \"%d\",\r\n	\"bat\": \"%.1f\",\r\n	\"rpm\": \"%d\",\r\n	\"cnt\": \"%d\"\r\n}",speed, (float)batt/10.0, rpm, cnt);
	
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, temp);
	res = curl_easy_perform(curl);
    }
    curl_easy_cleanup(curl);
#endif
    return 1;
}

int httppost_dist(float dist1, float dist2, char *dir)
{
    CURL *curl;
    CURLcode res;
    char temp[256] = {0,};
printf("dir=%s\r\n", dir);
#if 0
    httpprint("{\r\n    ");
    sprintf(temp, "\"lat2\":\"%f\",\r\n    ", lat);
    httpprint(temp);
    memset(temp, 0, 64);
    sprintf(temp, "\"lon2\":\"%f\"\r\n}", lon);
    httpprint(temp);
#else    
    curl = curl_easy_init();
    
    if(curl) {
		curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
	
    	sprintf(temp, "http://%s:5000/dist", ipaddr);
		curl_easy_setopt(curl, CURLOPT_URL, temp);
		
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
		curl_easy_setopt(curl, CURLOPT_DEFAULT_PROTOCOL, "https");

		struct curl_slist *headers = NULL;
		headers = curl_slist_append(headers, "Content-Type: application/json");
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		//const char *data = "{\r\n    \"lat2\":\"1234.5678\",\r\n    \"lon2\":\"1234.5678\"\r\n}";
		//sprintf(temp, "{\r\n	\"spd\":\"%d\",\r\n		\"bat\":\"%.1f\",\r\n	\"rpm\":\"%d\",r\n		\"cnt\":\"%d\"\r\n}", speed, (float)batt/10, rpm, cnt);
		sprintf(temp, "{\r\n	\"dist1\": \"%.2f\",\r\n	\"dist2\": \"%.2f\",\r\n	\"diff\": \"%.2f\",\r\n 	\"dir\": \"%s\"\r\n}", dist1, dist2, fabs(dist1-dist2), dir);
		
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, temp);
		res = curl_easy_perform(curl);
    }
    curl_easy_cleanup(curl);
#endif
    return 1;
}


int main(int argc , char *argv[])
{
    double x1 = 0.0, y1 = 0.0, x2 = 0.0, y2 = 0.0, diff1 = 0.0, diff2 = 0.0;
    int result, ret = -1, i = 0, odata_cnt = 0;
    char toggle = 0, buffer[MAX_BUF_SIZE] = {0,}, buffer_obd[MAX_BUF_SIZE] = {0,}, obd_state = OBD_ON_READY, rtk_enable = 0, obd_enable = 0;
    
    fd_set reads, reads_obd, temps, temps_obd;

	//_obd_raw    obdraw;
	_obd_data   obddata;
	_obd_tx_data obd_tx_data;

	BYTE    GET_DATA_REQ[7] = { 0x3C, 0x41, 0x43, 0x01, 0x00, 0x0D, 0x0A };    // get gender spec
	//BYTE    GET_DTC_REQ[7] = { 0x3C, 0x41, 0x44, 0x01, 0x00, 0x0D, 0x0A };   // get data request
	BYTE    GET_GENDER_SPEC[7] = { 0x3C, 0x41, 0x47, 0x01, 0x00, 0x0D, 0x0A };
	//BYTE    GET_FW_UPDATE_REQ[] = { 0x3C, 0x41, 0x49 };        // firmware update request
	//BYTE    SET_FW_UPDATE_DATA[] = { 0x3C, 0x41, 0x55 };      //firmware update data
	//BYTE    SET_TIME[] = { 0x3C, 0x41, 0x54, 0x0A };

	struct timeval timeout_obd;

	timeout_obd.tv_sec = 1;
	timeout_obd.tv_usec = 0;

    if(argc>1){
		strcpy(ipaddr, argv[2]);
			//offset = atoi(argv[1]);
		printf("argc = %d , argv=%s, ipaddr=%s\r\n", argc, argv[1], ipaddr);
		
		if(!strcmp("all", argv[1])){
			obd_enable = 1;
			rtk_enable = 1;
		}
		else if(!strcmp("obd", argv[1])){
			obd_enable = 1;
			printf("obd enable\r\n");
		}
		else if(!strcmp(argv[1] , "rtk")){
			rtk_enable = 1;
			printf("rtk enable\r\n");
		}
		else{
	
		}
    }

    if(rtk_enable){
		ret = init_uart(DEV_UART, 115200, & fd);

		printf("init uart [%d], [%d]\r\n", ret, fd);

		FD_ZERO( & reads);
		FD_SET(fd, & reads);
	}
	
	if(obd_enable){
		ret = init_uart(OBD_UART, 115200, & fd_obd);

		printf("init OBD uart [%d], [%d]\r\n", ret, fd_obd);

		FD_ZERO( & reads_obd);
		FD_SET(fd_obd, & reads_obd);
	}

    if(wiringPiSetupGpio() <0){
		printf("Unabe to setup wiringPi\r\n");
		return 0;
    }
    
    pinMode(BUZZER, OUTPUT);
    digitalWrite(BUZZER, 0);
    
    pinMode(LED_GREEN, OUTPUT);
    digitalWrite(LED_GREEN, 0);
    
    pinMode(LED_RED, OUTPUT);
    digitalWrite(LED_RED, 0);
    
    pinMode(LED_YELLOW, OUTPUT);
    digitalWrite(LED_YELLOW, 0);
    
    pinMode(L_LED, OUTPUT);
    digitalWrite(L_LED, 0);

    pinMode(R_LED, OUTPUT);
    digitalWrite(R_LED, 0);
	
    /*
			printf("left= 7.280, right=7.183 \r\n");
			printf(" left right align !!\r\n");
			printf("left= 7.270, right=7.153 \r\n");
			printf(" left right align !!\r\n");
			printf("left= 7.270, right=7.163 \r\n");
			printf(" left right align !!\r\n");
			printf("left= 7.288, right=7.181 \r\n");
			printf(" left right align !!\r\n");
			printf("left= 7.280, right=7.179 \r\n");
			printf(" left right align !!\r\n");
			printf("left= 7.281, right=7.183 \r\n");
			printf(" left right align !!\r\n");
			printf("left= 7.287, right=7.163 \r\n");
			printf(" left right align !!\r\n");
			printf("left= 7.270, right=7.173 \r\n");
			printf(" left right align !!\r\n");
			printf("left= 7.288, right=7.186 \r\n");
			printf(" left right align !!\r\n");
			printf("left= 7.281, right=7.182 \r\n");
			printf(" left right align !!\r\n");
			

return ;
*/

    for(i=0;i<3;i++)
    {
		digitalWrite(BUZZER, 1);
		digitalWrite(R_LED, 1);
		digitalWrite(L_LED, 1);
		
		usleep(500000);

		digitalWrite(BUZZER, 0);
		digitalWrite(R_LED, 0);
		digitalWrite(L_LED, 0);
		
		usleep(500000);
    }


    while (1)
    {
		if(rtk_enable){
			temps = reads;
			result = select(FD_SETSIZE, & temps, NULL, NULL, NULL);

			if (result < 0){
				printf("EXIT_FAILURE\r\n");
				exit(EXIT_FAILURE);
			}

			if (FD_ISSET(fd, & temps)){
				memset(buffer, 0, sizeof(buffer));
            
				if (read(fd, buffer, MAX_BUF_SIZE) == -1){
					printf("continue\r\n");
					continue;
				}
				
				for(i = 0; i<strlen(buffer); i++){
					parsing(buffer[i]);
				}
				printf("%s", buffer);
	    
	
				if(parser.flagTIMEDataReady){
					ret = httpget(&x1, &y1, &x2, &y2);
					if(ret){
						printf("lat = %f, lon = %f\r\n", parser.latitude, parser.longitude);
						printf("x1 = %f, y1 = %f, x2 = %f, y2 = %f\r\n", x1, y1, x2, y2);
						
						if(x1 != 0.0 && y1 != 0.0 && x2 != 0.0 && y2 != 0.0){
							diff1 = haversine(parser.latitude, parser.longitude, x1, y1);
					
							diff2 = haversine(parser.latitude, parser.longitude, x2, y2);
		
				//printf("left= %f, right=%f \r\n", diff1, diff2);
				
							if(diff1 < 5.01){
								digitalWrite(BUZZER, 1);
							}
							else if(diff1 < 10.0){
								toggle ^= 1;
								digitalWrite(BUZZER, toggle);
							}
							else{
								digitalWrite(BUZZER, 0);
							}
			
					
							if(fabs(diff1 - diff2) < 0.05){
								digitalWrite(R_LED, 1);
								digitalWrite(L_LED, 1);
								httppost_dist(diff1, diff2, "ALIGN");
							}	   
							else if(diff1>diff2){
								printf("--> left > right\r\n");
								digitalWrite(R_LED, 1);
								digitalWrite(L_LED, 0);
								httppost_dist(diff1, diff2, "LEFT");
							}
							else if(diff1<diff2){
								printf("<-- left < right\r\n");
								digitalWrite(R_LED, 0);
								digitalWrite(L_LED, 1);
								httppost_dist(diff1, diff2, "RIGHT");
							}
							else{
								digitalWrite(R_LED,1);
								digitalWrite(L_LED,1);
								httppost_dist(diff1, diff2, "NONE");
							}
						
							printf("\r\nLEFT=%.2f, RIGHT=%.2f, DIFF=%.2f \r\n", diff1, diff2, fabs(diff1-diff2));
						}	
					}
				}
			}
		}
		
		if(obd_enable){

			switch (obd_state)
			{
				case OBD_ON_READY:
					printf("OBD_ON_READY\r\n");
					obd_state = OBD_GET_GENDER_SPEC;
					break;

				case OBD_GET_GENDER_SPEC:
					printf("OBD_GET_GENDER_SPEC\r\n");
					ret = write(fd_obd, GET_GENDER_SPEC, 7);
					obd_state = OBD_GET_GENDER_SPEC_CHK;
					break;

				case OBD_GET_GENDER_SPEC_CHK:
					printf("OBD_GET_GENDER_SPEC_CHK\r\n");
					//SP->ReadData(obdraw.buffer, 512);
					temps_obd = reads_obd;

					result = select(FD_SETSIZE, & temps_obd, NULL, NULL, &timeout_obd);

					if (result < 0){
						exit(EXIT_FAILURE);
					}
					else{
						if (FD_ISSET(fd_obd, & temps)){
							memset(buffer_obd, 0, sizeof(buffer_obd));

						if (read(fd_obd, buffer_obd, MAX_BUF_SIZE) == -1){
								continue;
							}
							else{
								if (GenderSpecParse(buffer_obd)) {
									obd_state = OBD_GET_DATA_REQ;
								}
							else {
									obd_state = OBD_GET_GENDER_SPEC;
								}
							}
						}
					}
					break;

				case OBD_GET_DATA_REQ:
					printf("OBD_GET_DATA_REQ\r\n");
					ret = write(fd_obd, GET_DATA_REQ, 7);
					obd_state = OBD_GET_DATA_CHK;
					break;

				case OBD_GET_DATA_CHK:
					printf("OBD_GET_DATA_CHK\r\n");
					
					temps_obd = reads_obd;
					result = select(FD_SETSIZE, & temps_obd, NULL, NULL, NULL);

					if (result < 0){
						exit(EXIT_FAILURE);
					}
					else{
						if (FD_ISSET(fd_obd, & temps_obd)){
							memset(buffer_obd, 0, sizeof(buffer_obd));
							
							if (read(fd_obd, buffer_obd, MAX_BUF_SIZE) == -1){
								continue;
							}
							else{
								if (Check_OBD_Data(buffer_obd, &obddata + (odata_cnt % DATA_MAX)))
								{
								printf("obd ok:: odata_cnt = %d \r\n", odata_cnt);
								obd_state = OBD_OFF_READY;
				
							}
							else
							{
								obd_state = OBD_GET_DATA_REQ;
							}
						}
					}
				}
				break;


			case OBD_OFF_READY:
				printf("OBD_OFF_READY\r\n");
				obd_state = OBD_GET_DATA_REQ;
				Get_OBD_Data(&obddata, &obd_tx_data);
				//__no_operation();
				break;
			}	
		}

//	printf(".");
//        test++;
//    httppost_obd(test, test, test);
	
        sleep(1);
    }
    digitalWrite(BUZZER, 0);
}
