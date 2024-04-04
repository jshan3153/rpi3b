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

static int fd = 0;
#define MAX_BUF_SIZE 512
#define DEV_UART "/dev/ttyUSB0"
#define TRUE 1
#define FALSE 0

#define uint8_t	unsigned char 
#define uint32_t unsigned int
#define uint16_t unsigned short
#define BYTE 	unsigned char

#define DATA_MAX   14

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
//    char        odcnt;        // 샘플갯수 중에 유효한 갯수 
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
	uint32_t   Sec;           // 시스템 시간 
	uint32_t   Sec2;          // 통신 시작 이후 센싱 경과 시간 판단 변수 
	uint32_t   delay;         // GPS 센싱 대기 및 경과 시간 판단변수 
	uint8_t    delay_mode;
	uint32_t   timeout;
	uint8_t    timeout_mode;
	uint32_t   SystemTick;           // 시스템 시간 
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
 uint32_t      odata_cnt;       //obd 샘플 카운트 수 
 _obd_data *odata_backup;
 uint32_t odata_backup_cnt;
}_obd_def;

typedef struct _genderspec {
	BYTE 	serial_no[8];		// 젠더 시리얼 
	BYTE	sw_ver[2];			// 젠더 소프트웨어 버전
	BYTE	vehicle_no[12];		// 차량 번호
	BYTE	vin_info[17];		// 차대 번호 
	BYTE 	vehicle_type[8];		// 차종 
	BYTE	oil_type;			// 유종 
	BYTE	made_year[2];			// 차량 연식
	BYTE	engine_displacement[2];	// 배기량
	BYTE	cylinder;				// 기통수 
	BYTE	bt_mac[12];					// BT MAC
	BYTE 	accumulation_driving_distance[8];		// 누적 주행 거리
	BYTE	accumulation_fuel_quantity[8];		//누적연로소모량
	BYTE	gender_attached;						// 탈부착 유무
	BYTE	ignition_check;						// 시동 온오프
	BYTE	year[2];			//연도
	BYTE	month;				// 월 
	BYTE 	day;				// 일 
	BYTE	date;				// 요일
	BYTE	am_pm;				// 오전 오
	BYTE	hour;				// 시
	BYTE	min;				// 분 
	BYTE	sec;				// 초
}GENDER_SPEC;

typedef struct _rnudata {
	BYTE	accumulation_driving_distance[8];		// 누적 운행 거리 
	BYTE	speed[4];								// 차속
	BYTE	acc_fuel_volume[4];						// 누적연료소모량 
	BYTE	trip_efficiency[2];					//트립 연비
	BYTE	spot_fuel_spread[4];				// 순간연료분사량 
	BYTE	rpm[2];								// RPM
	BYTE	break_signal;						// 브레이크신호 
	BYTE	key_plus;							// KEY+
	BYTE	gear_lever;							// 변속레버 
	BYTE	gear_box;							// 변속단 
	BYTE	throttle_position;					// 가속페탈(스로틀포지션)
	BYTE	engine_oil_temperature[3];			// 엔진오일온도 
	BYTE	inhalation_temperature[3];			// 흡기온도 
	BYTE	exhaust_temperature[3];				// 외기온도
	BYTE	coolant_temperature[3];				// 냉각수온도 
	BYTE	maf[2];								// MAF (흡입공기량)
	BYTE	amp[2];								// 대기압 
	BYTE	engine_torque[4];					// 엔진토크 
	BYTE	battery_voltage[2];					// 배터리전압 
	BYTE	air_guage_volt[2];					// 에어게이지전압 
	BYTE	fuel_guage_volt[2];					// 연로게이지전압 
	BYTE	fuel_remain;							// 연료잔량 
	BYTE	reserved[8];							// 예약 
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
char  GenderSpecParse(_obd_raw *obd)
{
	GENDER_SPEC _gender_spec;
	char        *temp_addr, length, retValue = 0;
	unsigned char   chksum = 0, temp_chksum = 0;

	//unsigned long       fuel=0;

	if ((temp_addr = strchr(obd->buffer, '<')) != NULL)
	{
		if ((*(temp_addr + IDX_ITEM) == 'A') && (*(temp_addr + IDX_CMD) == 'G'))
		{
			if (*(temp_addr + IDX_LEN) == 0x5D)
			{
				length = *(temp_addr + IDX_LEN);
				chksum = *(temp_addr + IDX_LEN + length);

				//printf("GENDER_SPEC=%d\r\n", sizeof(GENDER_SPEC));
				//printf("length = %d \r\n", length);

				for (int i = 0; i<length - 1; i++)  // 길이 데이터에는 체크섬 갯수가 포함되어 있음 
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
				//    obd->status = 0;                     //0:연결 1:차단 2:미지정 

				//
				//if(_gender_spec.ignition_check)
				//    obd->odata->status = 2;

				//for(int i=0; i<length; i++) printf(" %02x ", *(temp_addr+i));

				retValue = 1;
			}

		}


	}

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
	// 저장되어 있는 OBD 정보를 아스키형으로 변환하여 Tx버퍼에 저장
	//uint8_t i;
	unsigned int    speed, rpm, accel, maf, amp, torque, volt;
	int                         oil_temp, in_temp, out_temp, coolant_temp;

	//아스키로 변환을 위해 
	obd_tx_data->status = obd_data->status + 0x30;

	//for(i=0; i<obd_data->cnt; i++)
	{
		// 1.차속 
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

		// 3.가속페달 
		accel = obd_data->raw_pedal;
		HexToAscii_Make(&obd_tx_data->gas_pedal[0], accel, LEN_100);
		printf("\t%3d [%%]", accel);

		// 4.엔진오일온도
		oil_temp = obd_data->raw_temp_oil;

		if (oil_temp >= 9999)
			oil_temp = 9999;

		if (oil_temp<0)
			obd_tx_data->temp_engine_oil[0] = '-';
		else
			obd_tx_data->temp_engine_oil[0] = '+';

		HexToAscii_Make(&obd_tx_data->temp_engine_oil[1], ABS(oil_temp), LEN_1000);
		printf("\t%4d['C]", oil_temp);

		// 5.흡기온도 
		in_temp = obd_data->raw_temp_indoor;

		if (in_temp >= 9999)
			in_temp = 9999;

		if (in_temp<0)
			obd_tx_data->temp_intake_air[0] = '-';
		else
			obd_tx_data->temp_intake_air[0] = '+';

		HexToAscii_Make(&obd_tx_data->temp_intake_air[1], ABS(in_temp), LEN_1000);
		printf("\t%4d['C]", in_temp);

		// 6.외기온도
		out_temp = obd_data->raw_temp_outdoor;

		if (out_temp >= 9999)
			out_temp = 9999;

		if (out_temp<0)
			obd_tx_data->temp_outdoor_air[0] = '-';
		else
			obd_tx_data->temp_outdoor_air[0] = '+';

		HexToAscii_Make(&obd_tx_data->temp_outdoor_air[1], ABS(out_temp), LEN_1000);
		printf("\t%4d['C]", out_temp);

		// 7. 냉각수온도
		coolant_temp = obd_data->raw_temp_coolant;

		if (coolant_temp >= 9999)
			coolant_temp = 9999;

		if (coolant_temp<0)
			obd_tx_data->temp_coolant[0] = '-';
		else
			obd_tx_data->temp_coolant[0] = '+';

		HexToAscii_Make(&obd_tx_data->temp_coolant[1], ABS(coolant_temp), LEN_1000);
		printf("\t%4d['C]", coolant_temp);

		// 8. MAF(공기량)
		maf = obd_data->raw_maf;
		if (maf >= 9999)
			maf = 9999;

		HexToAscii_Make(&obd_tx_data->air_volume[0], maf, LEN_1000);
		printf("\t%d [mg/TDC]", maf);

		// 9. AMP(대기압)
		amp = obd_data->raw_amp;
		if (amp >= 999)
			amp = 999;

		HexToAscii_Make(&obd_tx_data->atmospheric_pressure[0], amp, LEN_100);
		printf("\t%4d [kPa]", amp);

		// 10. 엔진토크
		torque = (unsigned int)obd_data->raw_torque;
		if (torque >= 9999)
			torque = 9999;

		HexToAscii_Make(&obd_tx_data->engine_torque[0], torque, LEN_1000);
		printf("\t%4d [%%]", torque);

		// 11. 배터리전압 
		volt = obd_data->raw_batt_volt;
		if (volt >= 999)
			volt = 999;

		HexToAscii_Make(&obd_tx_data->voltage_vehicle[0], volt, LEN_100);
		printf("\t%3d [V]", volt);

		// 아스키로 변환된 데이터 
		//        printf("\t");
		//        for(int i=0; i<sizeof(_obd_tx_data); i++)
		//            printf("%c", *(obd_tx_data->speed + i));
		//        printf("\r\n");

	}

	return 1;
}

char  Check_OBD_Data(_obd_raw *obd_raw, _obd_data *obd_data)
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
	if ((temp_addr = strchr(obd_raw->buffer, '<')) != NULL)
	{
		if ((*(temp_addr + IDX_ITEM) == 'A') && (*(temp_addr + IDX_CMD) == 'C'))
		{
			if (*(temp_addr + IDX_LEN) == 0x41)
			{
				length = *(temp_addr + IDX_LEN);

				printf("length = %d \r\n", length);
				chksum = *(temp_addr + IDX_LEN + length);

				for (int i = 0; i<length - 1; i++)  // 길이 데이터에는 체크섬 갯수가 포함되어 있음 
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

				// 1. 차속 
				temp_l = ((unsigned long)_run_data.speed[0]) << 24;
				temp_l |= ((unsigned long)_run_data.speed[1]) << 16;
				temp_l |= ((unsigned long)_run_data.speed[2]) << 8;
				temp_l |= ((unsigned long)_run_data.speed[3]) << 0;
				obd_data->raw_speed = temp_l;

				// 2. RPM              
				rpm = (unsigned int)_run_data.rpm[0] << 8;
				rpm |= (unsigned int)_run_data.rpm[1];
				obd_data->raw_rpm = rpm;

				// 3. 가속 페달 
				obd_data->raw_pedal = (unsigned int)_run_data.throttle_position;

				// 4. 엔진오일온도
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

				// 5. 흡기온도
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

				// 6. 외기온도 
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

				// 7. 냉각수 온도
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

				// 9. 대기압 
				amp = (unsigned int)_run_data.amp[0] << 8;
				amp |= (unsigned int)_run_data.amp[1];
				obd_data->raw_amp = amp;

				// 10. 엔진토크
				temp_l = 0;
				temp_l = ((unsigned long)(_run_data.engine_torque[0]) << 24);
				temp_l |= ((unsigned long)(_run_data.engine_torque[1]) << 16);
				temp_l |= ((unsigned long)(_run_data.engine_torque[2]) << 8);
				temp_l |= ((unsigned long)(_run_data.engine_torque[3]) << 0);
				obd_data->raw_torque = temp_l;

				// 11. 배터리전압 
				volt = (unsigned int)_run_data.battery_voltage[0] << 8;
				volt |= (unsigned int)_run_data.battery_voltage[1];
				obd_data->raw_batt_volt = volt;


				// RPM으로 시동 유무를 판단 
				if ((obd_data->raw_rpm>100) && (obd_data->raw_rpm<0xFFFF))
				{
					//                    obd_data->odcnt++;      // obd 샘플 중 유효한 갯수 증가 
					//                    printf("obd cnt = %d \r\n", obd_data->odcnt);
					retValue = 1;
				}
			}
		}
		else if ((*(temp_addr + IDX_ITEM) == 'A') && (*(temp_addr + IDX_CMD) == 'F')) {
			int error = 0;
			error = (*(temp_addr + IDX_LEN) - 0x30) * 100 + (*(temp_addr + IDX_LEN + 1) - 0x30) * 10 + (*(temp_addr + IDX_LEN + 2) - 0x30) * 1;
			if (error == 100) {
				printf("Not Ready Vehicle Initialize\r\n");
			}
			else if (error == 101) {
				printf("Not Ready OBD Initialize\r\n");
			}
			else if (error == 102) {
				printf("재 시동 감지\r\n");
			}
			else if (error == 200) {
				printf("시동 On 상태(busy)\r\n");
			}
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
    newtio.c_oflag = OPOST | ONLCR;;
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
    //newtio.c_cc[VTIME] = vtime; // timeout 0.1초 단위 
    //newtio.c_cc[VMIN] = vmin; // 최소 n 문자 받을 때까진 대기 
    newtio.c_cc[VTIME] = 0;
    newtio.c_cc[VMIN] = 0;
    tcflush( * fd, TCIFLUSH);
    tcsetattr( * fd, TCSANOW, & newtio);
    return 0;
}

int httppost(unsigned int speed, unsigned int rpm, unsigned int acc)
{
    CURL *curl;
    CURLcode res;
    char temp[64] = {0,};
   
    curl = curl_easy_init();
    
    if(curl) {
		curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
	
		
	    curl_easy_setopt(curl, CURLOPT_URL, "http://192.168.0.115:5000/obd");
	
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
		curl_easy_setopt(curl, CURLOPT_DEFAULT_PROTOCOL, "https");

		struct curl_slist *headers = NULL;
		headers = curl_slist_append(headers, "Content-Type: application/json");
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		//const char *data = "{\r\n    \"lat2\":\"1234.5678\",\r\n    \"lon2\":\"1234.5678\"\r\n}";
		sprintf(temp, "{\r\n    \"speed\":\"%d\",\r\n    \"rpm\":\"%d\",\r\n    \"acc\":\"%d\"\r\n}", speed, rpm, acc);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, temp);
		res = curl_easy_perform(curl);
    }
    curl_easy_cleanup(curl);

}

int main()
{
	_obd_raw    obdraw;
	_obd_data   obddata;
	_obd_tx_data obd_tx_data;

	BYTE    GET_DATA_REQ[7] = { 0x3C, 0x41, 0x43, 0x01, 0x00, 0x0D, 0x0A };    // get gender spec
	//BYTE    GET_DTC_REQ[7] = { 0x3C, 0x41, 0x44, 0x01, 0x00, 0x0D, 0x0A };   // get data request
	BYTE    GET_GENDER_SPEC[7] = { 0x3C, 0x41, 0x47, 0x01, 0x00, 0x0D, 0x0A };
	//BYTE    GET_FW_UPDATE_REQ[] = { 0x3C, 0x41, 0x49 };        // firmware update request
	//BYTE    SET_FW_UPDATE_DATA[] = { 0x3C, 0x41, 0x55 };      //firmware update data
	//BYTE    SET_TIME[] = { 0x3C, 0x41, 0x54, 0x0A };

	int odata_cnt = 0, result, ret = -1;
	char obd_state = OBD_ON_READY;

    ret = init_uart(DEV_UART, 115200, & fd);

    printf("init uart [%d], [%d]\r\n", ret, fd);


    char buffer[MAX_BUF_SIZE];
    fd_set reads, temps;

    FD_ZERO( & reads);
    FD_SET(fd, & reads);


    while (1)
    {
		switch (obd_state)
		{
			case OBD_ON_READY:
				printf("OBD_ON_READY\r\n");
				obd_state = OBD_GET_GENDER_SPEC;
				break;

			case OBD_GET_GENDER_SPEC:
				printf("OBD_GET_GENDER_SPEC\r\n");
				ret = write(fd, GET_GENDER_SPEC, 7);
				obd_state = OBD_GET_GENDER_SPEC_CHK;
				break;

			case OBD_GET_GENDER_SPEC_CHK:
				printf("OBD_GET_GENDER_SPEC_CHK\r\n");
				//SP->ReadData(obdraw.buffer, 512);
				temps = reads;
				result = select(FD_SETSIZE, & temps, NULL, NULL, NULL);

				if (result < 0){
					exit(EXIT_FAILURE);
				}
				else{
					if (FD_ISSET(fd, & temps)){
						memset(buffer, 0, sizeof(buffer));

					if (read(fd, buffer, MAX_BUF_SIZE) == -1){
							continue;
						}
						else{
							if (GenderSpecParse(&obdraw)) {
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
				ret = write(fd, GET_DATA_REQ, 7);
				obd_state = OBD_GET_DATA_CHK;
				break;

			case OBD_GET_DATA_CHK:
				printf("OBD_GET_DATA_CHK\r\n");
				
				temps = reads;
				result = select(FD_SETSIZE, & temps, NULL, NULL, NULL);

				if (result < 0){
					exit(EXIT_FAILURE);
				}
				else{
					if (FD_ISSET(fd, & temps)){
						memset(buffer, 0, sizeof(buffer));
						
						if (read(fd, buffer, MAX_BUF_SIZE) == -1){
							continue;
						}
						else{
							if (Check_OBD_Data(&obdraw, &obddata + (odata_cnt % DATA_MAX)))
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
		
        sleep(1);
    }
		





	getchar();

    return 0;
}

