#ifndef PCF8523_H_
#define PCF8523_H_

typedef struct _RTC_TIME
{
	uint8_t sec;
	uint8_t min;
	uint8_t hour;
	uint8_t day;
	uint8_t weekday;
	uint8_t month;
	uint8_t year;
} rtc_time_t;

#define RTC_CONTROL_REGS_OFFSET 0x00
#define RTC_TIME_REG_OFFSET 0x03
#define RTC_TIME_REG_OFFSET_SEC     (RTC_TIME_REG_OFFSET + 0)

void pcf8523_init(void);
int pcf8523_get_time(rtc_time_t* t);

int pcf8523_read_regs(uint8_t reg, uint8_t* buffer, uint8_t num);
int pcf8523_write_regs(uint8_t reg, const uint8_t* buffer, uint8_t num);

#endif // PCF8523_H_
