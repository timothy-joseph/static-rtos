#ifndef MUTEX_H
#define MUTEX_X

void kmutex_create(void);
void kmutex_take(uint8_t *mutex_handle, uint16_t tick_timeout);
void kmutex_give(void);
void kmutex_is_available(void);

#endif

