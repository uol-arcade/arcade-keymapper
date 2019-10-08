#include "main.h"

int SDL_main(int argc, char* argv[])
{
	//Create a new window
	keymapper::Window* window = new keymapper::Window();

	//Create a new mapper, set the window's mapper instance
	keymapper::Mapper* mapper = new keymapper::Mapper();
	window->SetMapperInstance(mapper);

	//Set up timeout values for automatic refresh (timeout)
	mapper->SetTimeoutSeconds(6);
	mapper->SetWarningSeconds(5);

	//Render the splash screen
	window->RenderSplashScreen("splash.jpg");

	//Start joypad detection
	//window->StartJoypadDetection();

	while (!window->joypadThreadExited)
	{
		//Call onThreadIteration
		mapper->OnThreadIteration(window);
	}

	//SDL_Delay(5000);
	window->Close();
	//SDL_Delay(3000);
	

    return 0;
}

