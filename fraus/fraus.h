#ifndef FRAUS_FRAUS_H
#define FRAUS_FRAUS_H

#if __STDC_VERSION__ < 199901L
#error C99 required
#endif

#include "images/images.h"
#include "math.h"
#include "models/models.h"
#include "threads.h"
#include "utils.h"
#include "vulkan/vulkan.h"
#include "window.h"

FrResult frInit();

/*
 * Main loop of the program
 * Returns the exit value of the program
 */
int frMainLoop(FrVulkanData* pVulkanData);

FrResult frFinish();


#endif
