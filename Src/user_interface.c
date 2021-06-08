#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "share/compat.h"
#include "user_interface.h"
#include "fatfs.h"
#include "cmsis_os.h"
#include "stm32f4_discovery_audio.h"
#include "flac_decoder_handler.h"
#include "dbgu.h"

/* Play a sound file and control its playback */
void play(char* path, char* file, uint8_t *audio_buffer, uint8_t* buf_offs, SemaphoreHandle_t syncSemaphore){
    /* Contruct full filepath */
    char full_path[256];
    for(int i = 0; i<256; i++) full_path[i] = 0;
    strcat(full_path, path);
    strcat(full_path, "/");
    strcat(full_path, file);
    xprintf("Now playing: %s\n", full_path);

    /* play file */
    int player_state = 1;
    int loaded_counter = 0;
    *buf_offs = 0;
    if(start_flac_decoding(full_path, audio_buffer, &loaded_counter, buf_offs, syncSemaphore)) return;
    xprintf("Use \'o\' to pause and resume, 'n' to stop playing and choose another song.\n");
    for(;;){
        char key = debug_inkey();
        switch(key){
            case 'o':
            {
                if(player_state==1) {
                    xprintf("PAUSED\n");
                    BSP_AUDIO_OUT_Pause();
                    player_state = 2;
                } else if(player_state==2) {
                    xprintf("RESUMED\n");
                    BSP_AUDIO_OUT_Resume();
                    player_state = 1;
                }
                break;
            }
            case 'n':
            {
                xprintf("STOPPING\n");
                BSP_AUDIO_OUT_Stop(CODEC_PDWN_HW);
                close_decoder();
                return;
            }
        }

        if(player_state == 1){
            if(load_flac_frame()){
                xprintf("Unexpected error while playing.\n");
                BSP_AUDIO_OUT_Stop(CODEC_PDWN_HW);
                break;
            } else {
                if(reached_eof()){ /* When playback finishes */
                    BSP_AUDIO_OUT_Stop(CODEC_PDWN_HW);
                    xprintf("Finished playing\n");
                    break;
                }
            }
        }
        vTaskDelay(2);
    }
    close_decoder();
}

/*
 * Goes through all files on a drive.
 * Source: fatfs documentation
 * http://elm-chan.org/fsw/ff/doc/readdir.html
 */
FRESULT scan_files (char* path, uint8_t *audio_buffer, uint8_t* buf_offs, SemaphoreHandle_t syncSemaphore){
    DIR dir;
    UINT i;
    static FILINFO fno;

    FRESULT res = f_opendir(&dir, path);
    if (res == FR_OK) {
        for (;;) {
            res = f_readdir(&dir, &fno);                   /* Read a directory item */
            if (res != FR_OK || fno.fname[0] == 0) break;  /* Break on error or end of dir */
            if (fno.fattrib & AM_DIR) {                    /* directory */
                i = strlen(path);
                sprintf(&path[i], "/%s", fno.fname);
                res = scan_files(path, audio_buffer, buf_offs, syncSemaphore);   /* Enter the directory */
                if (res != FR_OK) break;
                path[i] = 0;
            } else {                                       /* file. */
                xprintf("%s/%s\n", path, fno.fname);
                for(;;){
                    char key = debug_inkey();
                    if(key == 'n') break;
                    if(key == 'p') play(path, fno.fname, audio_buffer, buf_offs, syncSemaphore);
                    vTaskDelay(2);
                }
            }
        }
        f_closedir(&dir);
    }
    return res;
}
/* Go into a file choosing and playing mode - infinite loop */
void choose_file(uint8_t *audio_buffer, uint8_t* buf_offs, SemaphoreHandle_t syncSemaphore){
    xprintf("Scroll through files with \'n\'. Press 'p' to play chosen file.\n");
    char path_buff[256];
    for(;;) {
        for(int i = 0; i < 256; i++){
            path_buff[i] = 0;
        }
        strcpy(path_buff, "0:");
        scan_files(path_buff, audio_buffer, buf_offs, syncSemaphore);
    }
}

