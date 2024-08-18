#pragma once

void blimp(uint32_t count, uint32_t tiks_to_delay);
void pcm_setup(int hz);
void pcm_cleanup(void);

typedef char* (*pcm_end_callback_t)(size_t* size);
void pcm_set_buffer(int16_t* buff, uint8_t channels, size_t size, pcm_end_callback_t cb);
