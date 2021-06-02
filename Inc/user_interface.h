#ifndef STM_PLAYER_USER_INTERFACE_H
#define STM_PLAYER_USER_INTERFACE_H
#include "cmsis_os.h"
void choose_file(uint8_t *audio_buffer, uint8_t* buff_off, SemaphoreHandle_t syncSemaphore);

#endif //STM_PLAYER_USER_INTERFACE_H