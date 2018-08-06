#ifndef __MODE32_RAW_IMAGE_H
#define __MODE32_RAW_IMAGE_H

#include "mode32.h"
#include "kernel/image.h"

int process_raw_image(image_stream* is, kernel::image_header* sector0);

#endif /* __MODE32_RAW_IMAGE_H */