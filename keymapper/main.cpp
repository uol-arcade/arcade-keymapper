#include "main.h"

void __cdecl InitialiseSDL()
{
	//Initialise SDL & subsystems
	SDL_Init(SDL_INIT_EVERYTHING);
	SDL_InitSubSystem(SDL_INIT_JOYSTICK);

	//Initialise relevant libraries
	IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG);
	TTF_Init();

	//Initialise key util thing
	KeyUtil::Initialise();

	//Enable joypad events
	SDL_JoystickEventState(SDL_ENABLE);

	//Set up hints
	SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
	SDL_SetHint(SDL_HINT_ALLOW_TOPMOST, "1");
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 2);

	//Print out debug stuff
	printd("[info] Initialised SDL\n");
}

int SDL_main(int argc, char* argv[])
{
	//Set debug flag if --debug is passed
	if (argc > 1)
		keymapperIsDebug = !stricmp(argv[1], "--debug");

	//Initialise SDL
	InitialiseSDL();

	//Show / hide the window depending on debug
	ShowWindow(GetConsoleWindow(), SW_HIDE);

	//Create a new window
	keymapper::Window* window = new keymapper::Window();

	//Create a new mapper, set the window's mapper instance
	keymapper::Mapper* mapper = new keymapper::Mapper();
	window->SetMapperInstance(mapper);

	//Load in assets
	window->LoadWarningImage("assets/img/background-2.jpg");
	window->LoadMainFont("assets/fonts/press-start.ttf");

	//Load config
	mapper->LoadConfigFile("assets/json/mapper_config.json");

	//Set up joypad mappings: player 1 & player 2
	mapper->AddJoyButtonMap(keymapper::JOYPAD_DEVICE_IDX_P1, "assets/json/p1_map.json");
	mapper->AddJoyAxisMap(keymapper::JOYPAD_DEVICE_IDX_P1, "assets/json/p1_map.json");
	//--
	mapper->AddJoyButtonMap(keymapper::JOYPAD_DEVICE_IDX_P2, "assets/json/p2_map.json");
	mapper->AddJoyAxisMap(keymapper::JOYPAD_DEVICE_IDX_P2, "assets/json/p2_map.json");

	//Find virtual indices and hide the window when done
	bool result = mapper->EnumerateJoypads(window);

	if (result)
	{
		SDL_Delay(10000);
		window->Close();
		return 0;
	}

	if(!keymapperIsDebug)
		window->Hide();

	//Start chrome up
	if(!keymapperIsDebug)
		mapper->RestartChrome();

	while (!window->joypadThreadExited)
	{
		//Call onThreadIteration
		mapper->OnThreadIteration(window);
	}

	//Close the window
	window->Close();	

    return 0;
}

