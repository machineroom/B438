
/*
 *  Function declarations for the Link I/O interface to the link adaptor.
 *  Copyright (C) Computer System Architects, 1988
 */

#include <stdint.h>

// return 0 on OK, -1 on error
int byte_out(uint8_t);

// return 0 on OK, -1 on error
int word_in(uint32_t *word);

// return 0 on OK, -1 on error
int word_out(uint32_t word);

// return 0 on OK, -1 on error
int chan_in(uint8_t *, unsigned int count);

// return 0 on OK, -1 on error
int chan_out(uint8_t *, unsigned int count);

void rst_adpt(void);

// return 0 on OK, -1 on error
int  init_lkio();
