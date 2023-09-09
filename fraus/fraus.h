#ifndef FRAUS_FRAUS_H
#define FRAUS_FRAUS_H

#if __STDC_VERSION__ < 199901L
#error C99 required
#endif

#include "camera.h"
#include "images/images.h"
#include "math.h"
#include "models/models.h"
#include "threads.h"
#include "utils.h"
#include "vulkan/vulkan.h"
#include "window.h"

typedef struct FrApplication
{
	FrWindow window;
	FrCamera camera;
	FrVulkanData vulkanData;
} FrApplication;

/*
 * Create a Fraus application
 * Returns:
 * - FR_SUCCESS if everything went well
 * - FR_ERROR_FILE_NOT_FOUND if the Vulkan library could not be found
 * - FR_ERROR_UNKNOWN if some other error occured
 */
FrResult frCreateApplication(const char* pName, uint32_t version, FrApplication* pApplication);

/*
 * Destroy a Fraus application
 * Returns:
 * - FR_SUCCESS if everything went well
 * - FR_ERROR_UNKNOWN if freeing the Vulkan library failed
 */
FrResult frDestroyApplication(FrApplication* pApplication);

/*
 * Main loop of the application
 * - pApplication: pointer to an application
 * Returns the exit value of the application
 */
int frMainLoop(FrApplication* pApplication);

#endif
