#ifndef FRAUS_FRAUS_H
#define FRAUS_FRAUS_H

#if __STDC_VERSION__ < 199901L
#error C99 required
#endif

#include "images.h"
#include "threads.h"
#include "utils.h"
#include "vulkan/vulkan.h"
#include "window.h"

FrResult frInit();
FrResult frFinish();

#endif
