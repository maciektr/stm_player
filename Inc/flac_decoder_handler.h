#include "cmsis_os.h"
#include "semphr.h"
#define AUDIO_BUFFER_SIZE 4096*4
int start_flac_decoding(char *path, uint8_t *buffer, int * loaded_counter, uint8_t* buf_off, SemaphoreHandle_t semaphore);
int load_flac_frame();
int close_decoder();
