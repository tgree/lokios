#ifndef __MODE32_RAW_IMAGE_H
#define __MODE32_RAW_IMAGE_H

#include "mode32.h"
#include "kern/image.h"

int process_raw_image(image_stream* is, kernel::image_header* sector0,
                      uintptr_t* image_end);

#endif /* __MODE32_RAW_IMAGE_H */
