#ifndef FRAUS_FRAUS_H
#define FRAUS_FRAUS_H

#include "./camera.h"
#include "./fonts/fonts.h"
#include "./images/images.h"
#include "./input.h"
#include "./math.h"
#include "./models/models.h"
#include "./utils.h"
#include "./vulkan/vulkan.h"
#include "./window.h"

typedef void (*FrUpdateHandler)(float elapsed, void* pUserData);

/*
 * Create a Fraus application.
 *
 * Parameters:
 * - name: The name of the application.
 * - version: The version of the application.
 * 
 * Returns:
 * - FR_SUCCESS if everything went well
 * - FR_ERROR_FILE_NOT_FOUND if the Vulkan library could not be found
 * - FR_ERROR_UNKNOWN if some other error occured
 */
FrResult frCreateApplication(const char* name, uint32_t version);

/*
 * Destroy a Fraus application.
 *
 * Returns:
 * - FR_SUCCESS if everything went well
 * - FR_ERROR_UNKNOWN if freeing the Vulkan library failed
 */
FrResult frDestroyApplication(void);

/*
 * Set the update handler.
 *
 * Parameters:
 * - handler: The handler.
 * - pUserData: The user data.
 */
void frSetUpdateHandler(FrUpdateHandler handler, void* pUserData);

/*
 * Main loop of the application.
 *
 * Returns:
 * - The exit value of the application.
 */
int frRunApplication(void);

#endif
