#include "cmsis_os.h"
#include "semphr.h"
int start_flac_decoding(char *path, uint8_t *buffer, int * loaded_counter, uint8_t* buf_off, SemaphoreHandle_t semaphore);
int load_flac_frame();
int close_decoder();
