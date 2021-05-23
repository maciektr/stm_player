#include <stdio.h>
#include <stdlib.h>
#include "share/compat.h"
#include "FLAC/stream_decoder.h"
#include "flac_decoder_handler.h"
#include "fatfs.h"

static FLAC__uint64 total_samples = 0;
static unsigned sample_rate = 0;
static unsigned channels = 0;
static unsigned bps = 0;

void metadata_callback(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data);
void error_callback(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data);
FLAC__StreamDecoderWriteStatus write_callback(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data);
FLAC__StreamDecoderReadStatus read_callback(const FLAC__StreamDecoder *decoder, FLAC__byte buffer[], size_t *bytes, void *client_data);
FLAC__StreamDecoderSeekStatus seek_callback(const FLAC__StreamDecoder *decoder, FLAC__uint64 absolute_byte_offset, void *client_data);
FLAC__StreamDecoderTellStatus tell_callback(const FLAC__StreamDecoder *decoder, FLAC__uint64 *absolute_byte_offset, void *client_data);
FLAC__StreamDecoderLengthStatus length_callback(const FLAC__StreamDecoder *decoder, FLAC__uint64 *stream_length, void *client_data);
FLAC__bool eof_callback(const FLAC__StreamDecoder *decoder, void *client_data);


typedef struct MyFileData{
    FIL file;
    char *path;
    uint8_t *buffer;
    int loaded_counter;
} MyFileData;

// TODO repeated from main
static void f_disp_res(FRESULT r)
{
    switch(r)
    {
        case FR_OK: printf("FR_OK\n"); break;
        case FR_DISK_ERR: printf("FR_DISK_ERR\n"); break;
        case FR_INT_ERR: printf("FR_INT_ERR\n"); break;
        case FR_NOT_READY: printf("FR_NOT_READY\n"); break;
        case FR_NO_FILE: printf("FR_NO_FILE\n"); break;
        case FR_NO_PATH: printf("FR_NO_PATH\n"); break;
        case FR_INVALID_NAME: printf("FR_INVALID_NAME\n"); break;
        case FR_DENIED: printf("FR_DENIED\n"); break;
        case FR_EXIST: printf("FR_EXIST\n"); break;
        case FR_INVALID_OBJECT: printf("FR_INVALID_OBJECT\n"); break;
        case FR_WRITE_PROTECTED: printf("FR_WRITE_PROTECTED\n"); break;
        case FR_INVALID_DRIVE: printf("FR_INVALID_DRIVE\n"); break;
        case FR_NOT_ENABLED: printf("FR_NOT_ENABLED\n"); break;
        case FR_NO_FILESYSTEM: printf("FR_NO_FILESYSTEM\n"); break;
        case FR_MKFS_ABORTED: printf("FR_MKFS_ABORTED\n"); break;
        case FR_TIMEOUT: printf("FR_TIMEOUT\n"); break;
        case FR_LOCKED: printf("FR_LOCKED\n"); break;
        case FR_NOT_ENOUGH_CORE: printf("FR_NOT_ENOUGH_CORE\n"); break;
        case FR_TOO_MANY_OPEN_FILES: printf("FR_TOO_MANY_OPEN_FILES\n"); break;
        case FR_INVALID_PARAMETER: printf("FR_INVALID_PARAMETER\n"); break;
        default: printf("result code unknown (%d = 0x%X)\n",r,r);
    }
}

FLAC__StreamDecoder *decoder = 0;
MyFileData filedata;

int start_flac_decoding(char *path, uint8_t *buffer, int * loaded_counter)
{
    FLAC__bool ok = true;
    FLAC__StreamDecoderInitStatus init_status;

    if((decoder = FLAC__stream_decoder_new()) == NULL) {
        xprintf(stderr, "ERROR: allocating decoder\n");
        return 1;
    }

    FRESULT res = f_open(&filedata.file, path, FA_READ);
    f_disp_res(res);
    filedata.path = path;
    filedata.buffer = buffer;
    filedata.loaded_counter = loaded_counter;


    init_status = FLAC__stream_decoder_init_stream(
            decoder, read_callback, seek_callback, tell_callback, length_callback, eof_callback,
            write_callback, metadata_callback, error_callback, &filedata);

    if(init_status != FLAC__STREAM_DECODER_INIT_STATUS_OK) {
        xprintf("ERROR: initializing decoder: %s\n", FLAC__StreamDecoderInitStatusString[init_status]);
        ok = false;
    }

    if(ok) {
        ok = FLAC__stream_decoder_process_single(decoder);
        xprintf("decoding: %s\n", ok? "succeeded" : "FAILED");
        xprintf("   state: %s\n", FLAC__StreamDecoderStateString[FLAC__stream_decoder_get_state(decoder)]);
    }
    return 0;
}

int close_decoder(){
    FLAC__stream_decoder_delete(decoder);
    f_close(&filedata.file);
}

int load_flac_frame(){
    //seek test
    //xprintf(" seekstart\n");
    //FLAC__bool ok1 = FLAC__stream_decoder_seek_absolute(decoder, 0);
    //xprintf(" seekend: %s\n", ok1? "succeeded" : "FAILED");

    xprintf("Run process single\n");
    FLAC__bool ok;
    ok = FLAC__stream_decoder_process_single(decoder);
    xprintf("decoding: %s\n", ok? "succeeded" : "FAILED");
    xprintf("   state: %s\n", FLAC__StreamDecoderStateString[FLAC__stream_decoder_get_state(decoder)]);
    return 0;
}

FLAC__StreamDecoderReadStatus read_callback(const FLAC__StreamDecoder *decoder, FLAC__byte buffer[], size_t *bytes, void *client_data)
{
    xprintf("reading\n");
    FIL* file = &((MyFileData*)client_data)->file;
    //xprintf("reading position: %d, %d, %d\n", f_tell(file), buffer, f_size(file));
    if(*bytes > 0) {
        size_t readbytes;
        FRESULT res = f_read(file, buffer, *bytes, &readbytes);
        *bytes = readbytes;
        if(f_error(file)) {
            return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
        }
        else if(*bytes == 0)
            return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
        else
            return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
    }
   else
       return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
}

FLAC__StreamDecoderSeekStatus seek_callback(const FLAC__StreamDecoder *decoder, FLAC__uint64 absolute_byte_offset, void *client_data)
{
    xprintf("seek\n");
    FIL* file = &((MyFileData *)client_data)->file;
    if(&file == stdin)
      return FLAC__STREAM_DECODER_SEEK_STATUS_UNSUPPORTED;
    FRESULT res = f_lseek(file, (off_t)absolute_byte_offset);
    f_disp_res(res);
    if(res < 0)
       return FLAC__STREAM_DECODER_SEEK_STATUS_ERROR;
    else
       return FLAC__STREAM_DECODER_SEEK_STATUS_OK;
}

FLAC__StreamDecoderTellStatus tell_callback(const FLAC__StreamDecoder *decoder, FLAC__uint64 *absolute_byte_offset, void *client_data)
{
   xprintf("tell\n");
   FIL* file = &((MyFileData*)client_data)->file;
   FSIZE_t pos;
   if(file == stdin)
       return FLAC__STREAM_DECODER_TELL_STATUS_UNSUPPORTED;
   else if((pos = f_tell(file)) < 0)
       return FLAC__STREAM_DECODER_TELL_STATUS_ERROR;
   else {
       *absolute_byte_offset = (FLAC__uint64)pos;
       return FLAC__STREAM_DECODER_TELL_STATUS_OK;
   }
}

FLAC__StreamDecoderLengthStatus length_callback(const FLAC__StreamDecoder *decoder, FLAC__uint64 *stream_length, void *client_data) {
    xprintf("length\n");
    FIL* file = &((MyFileData *) client_data)->file;
    *stream_length = (FLAC__uint64) f_size(file);
    if (file == stdin)
        return FLAC__STREAM_DECODER_LENGTH_STATUS_UNSUPPORTED;
    else if (*stream_length <= 0)
        return FLAC__STREAM_DECODER_LENGTH_STATUS_ERROR;
    else {
        return FLAC__STREAM_DECODER_LENGTH_STATUS_OK;
    }
}

FLAC__bool eof_callback(const FLAC__StreamDecoder *decoder, void *client_data){
    FIL* file = &((MyFileData*)client_data)->file;
    return f_eof(file)? true : false;
}


void write_little_endian(uint8_t *buff, FLAC__uint16 val){
    buff[0] = val & 0xff;
    buff[1] = (val >> 8) & 0xff;
}

FLAC__StreamDecoderWriteStatus write_callback(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data)
{
    xprintf("writing\n");
    const FLAC__uint32 total_size = (FLAC__uint32)(total_samples * channels * (bps/8));
    /*
    if(channels != 2 || bps != 16) {
        xprintf("ERROR: this example only supports 16bit stereo streams\n");
        return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
    }*/
    if(frame->header.channels != 2) {
        xprintf("ERROR: This frame contains %u channels (should be 2)\n", frame->header.channels);
        return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
    }
    if(buffer [0] == NULL) {
        xprintf("ERROR: buffer [0] is NULL\n");
        return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
    }
    if(buffer [1] == NULL) {
        xprintf("ERROR: buffer [1] is NULL\n");
        return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
    }

    MyFileData *myData = (MyFileData*)client_data;
    myData->loaded_counter = 0;

	/* write WAVE header before we write the first frame */
	if(frame->header.number.sample_number == 0) {
        memcpy(myData->buffer, "RIFF", sizeof(char) * 4);
        write_little_endian(myData->buffer + 4, total_size + 36);
        memcpy(myData->buffer + 8, "WAVEfmt ", sizeof(char) * 8);
        write_little_endian(myData->buffer + 16, 16);
        write_little_endian(myData->buffer + 20, 1);
        write_little_endian(myData->buffer + 24, (FLAC__uint16)channels);
        write_little_endian(myData->buffer + 28, sample_rate);
        write_little_endian(myData->buffer + 32, sample_rate * channels * (bps/8));
        write_little_endian(myData->buffer + 36, (FLAC__uint16)(channels * (bps/8)));
        write_little_endian(myData->buffer + 40, (FLAC__uint16)bps);
        memcpy(myData->buffer + 44, "data", sizeof(char) * 4);
        write_little_endian(myData->buffer + 48, total_size);
        myData->loaded_counter += 52;
	}
	/* write decoded PCM samples */
    int buffer_offset = 0;
    xprintf("Blocksize: %d\n", frame->header.blocksize);
	for(int i = 0; i < frame->header.blocksize; i++) {
	    if(i==0){
	        xprintf("first sample: %d\n", (FLAC__int16)buffer[0][i]);
	    }

		write_little_endian(myData->buffer + myData->loaded_counter, (FLAC__int16)buffer[0][i]);  /* left channel */
        buffer_offset+=2;
        myData->loaded_counter+=2;
		write_little_endian(myData->buffer + myData->loaded_counter, (FLAC__int16)buffer[1][i]);  /* right channel */
        buffer_offset+=2;
        myData->loaded_counter+=2;
        if(myData->loaded_counter>=4096*4) return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
	}

	return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

void metadata_callback(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data)
{
    xprintf("metadata\n");
    (void)decoder, (void)client_data;
    xprintf(metadata -> type);
    /* print some stats */
    if(metadata->type == FLAC__METADATA_TYPE_STREAMINFO) {
        /* save for later */
        total_samples = metadata->data.stream_info.total_samples;
        sample_rate = metadata->data.stream_info.sample_rate;
        channels = metadata->data.stream_info.channels;
        bps = metadata->data.stream_info.bits_per_sample;

        xprintf("sample rate    : %u Hz\n", sample_rate);
        xprintf("channels       : %u\n", channels);
        xprintf("bits per sample: %u\n", bps);
        //xprintf("total samples  : " PRIu64 "\n", total_samples); // Does not work
    }else{
        xprintf("other type\n");
    }
}

void error_callback(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data)
{
    (void)decoder, (void)client_data;
    xprintf("Got error callback: %s\n", FLAC__StreamDecoderErrorStatusString[status]);
}
