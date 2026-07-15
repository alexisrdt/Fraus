#ifndef FRAUS_FRAUS_H
#define FRAUS_FRAUS_H

#include "./arena.h"
#include "./camera.h"
#include "./fonts/fonts.h"
#include "./images/images.h"
#include "./math.h"
#include "./models/models.h"
#include "./utils.h"
#include "fraus/version.h"
#include "./vulkan/vulkan.h"
#include "./window.h"

FrResult frInitialize(void);
FrResult frFinish(void);

typedef void (*FrMouseMoveHandler)(int32_t dx, int32_t dy, void* userData);
typedef void (*FrKeyHandler)(FrKey key, FrKeyState state, void* userData);
typedef void (*FrResizeHandler)(uint16_t newWidth, uint16_t newHeight, void* userData);
typedef void (*FrUpdateHandler)(float elapsed, void* userData);

typedef struct FrApplication
{
	FrArena arena;

	FrWindow window;

	FrEngine engine;

	FrCamera camera;

	FrMouseMoveHandler mouseMoveHandler;
	void* mouseMoveHandlerUserData;
	FrKeyHandler keyHandler;
	void* keyHandlerUserData;
	FrResizeHandler resizeHandler;
	void* resizeHandlerUserData;
	FrUpdateHandler updateHandler;
	void* updateHandlerUserData;
} FrApplication;

/*
 * Create a Fraus application.
 *
 * Parameters:
 * - application: The application.
 * - name: The name of the application.
 * - version: The version of the application.
 * 
 * Returns:
 * - FR_SUCCESS if everything went well
 * - FR_ERROR_FILE_NOT_FOUND if the Vulkan library could not be found
 * - FR_ERROR_UNKNOWN if some other error occured
 */
FrResult frCreateApplication(FrApplication* application, const char* name, uint32_t version);

/*
 * Destroy a Fraus application.
 *
 * Returns:
 * - FR_SUCCESS if everything went well
 * - FR_ERROR_UNKNOWN if freeing the Vulkan library failed
 */
FrResult frDestroyApplication(FrApplication* application);

/*
 * Set the mouse move handler.
 * - handler: The handler.
 * - userData: The user data.
 */
void frSetMouseMoveHandler(FrApplication* application, FrMouseMoveHandler handler, void* userData);

/*
 * Set the key handler.
 * - handler: The handler.
 * - userData: The user data.
 */
void frSetKeyHandler(FrApplication* application, FrKeyHandler handler, void* userData);

/*
 * Set the resize handler.
 * - handler: The handler.
 * - userData: The user data.
 */
void frSetResizeHandler(FrApplication* application, FrResizeHandler handler, void* userData);

/*
 * Set the update handler.
 *
 * Parameters:
 * - handler: The handler.
 * - userData: The user data.
 */
void frSetUpdateHandler(FrApplication* application, FrUpdateHandler handler, void* userData);

/*
 * Main loop of the application.
 *
 * Returns:
 * - The exit value of the application.
 */
int frRunApplication(FrApplication* application);

#endif
