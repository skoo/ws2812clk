#ifndef PROG_IF_H_
#define PROG_IF_H_

extern volatile uint32_t prog_if_set_time_trigger;

void prog_if_init(void);
const uint8_t* prog_if_get_prog_data(void);

#endif // PROG_IF_H_
