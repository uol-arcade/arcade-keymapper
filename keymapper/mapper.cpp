#include "mapper.h"

keymapper::Mapper::Mapper(void)
{

}

void keymapper::Mapper::SetTimeoutSeconds(int seconds)
{
	
	//Just set ticks to the amount of seconds times by 1000 (because this is in ms and we're doing s -> ms)
	this->timeoutTicks = seconds * 1000;
}

void keymapper::Mapper::SetWarningSeconds(int seconds)
{
	//Just set ticks to the amount of seconds times by 1000 (because this is in ms and we're doing s -> ms)
	this->warningTicks = seconds * 1000;
}

void keymapper::Mapper::CloseProcess(const wchar_t* name) const
{
	// god bless stackoverflow
	// <https://stackoverflow.com/questions/865152/how-can-i-get-a-process-handle-by-its-name-in-c>
	//

	PROCESSENTRY32 entry;
	entry.dwSize = sizeof(PROCESSENTRY32);

	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

	//----
	//Run through all processes and kill chrome
	//----

	if (Process32First(snapshot, &entry) == TRUE)
	{
		while (Process32Next(snapshot, &entry) == TRUE)
		{
			//wcsicmp, yikes what a name: wide compare strings with case insensitivity
			//..

			if (_wcsicmp(entry.szExeFile, name) == 0)
			{
				//Open that process
				HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, entry.th32ProcessID);

				//Terminate it
				TerminateProcess(hProcess, -1);

				//Close that process
				CloseHandle(hProcess);
			}
		}
	}

	CloseHandle(snapshot);
}

void keymapper::Mapper::StartProcess(const std::string& path, const std::string& args) const
{
	//Startup info
	STARTUPINFOA info = { sizeof(info) };
	
	//Process info for startup
	PROCESS_INFORMATION processInfo;

	LPSTR lpPath = const_cast<LPSTR>(path.c_str());
	LPSTR lpArgs = const_cast<LPSTR>(args.c_str());

	if (CreateProcessA(lpPath, lpArgs, NULL, NULL, TRUE, CREATE_NEW_PROCESS_GROUP, NULL, NULL, &info, &processInfo))
	{
		//It's created it so detach from parent process
		//..

		//Close handle
		CloseHandle(processInfo.hProcess);
		CloseHandle(processInfo.hThread);
	}
	else
	{
		DWORD error = GetLastError();
		std::cout << std::endl << error << std::endl;
	}
}

void keymapper::Mapper::RestartChrome(void) const
{
	//Close chrome
	this->CloseProcess(L"chrome.exe");

	//Sleep for a second
	Sleep(MAPPER_WAIT_START_PROCESS_TIME * 1000);

	//Start chrome
	this->StartProcess(this->chromePath, this->chromeFlags);
}

void keymapper::Mapper::RefreshChrome(void) const
{
	HWND hwnd = NULL;

	while (true)
	{
		hwnd = FindWindowEx(0, hwnd, L"Chrome_WidgetWin_1", NULL);

		if (!hwnd)
			break;
		
		if (!IsWindowVisible(hwnd))
			continue;

		SetForegroundWindow(hwnd);

		KeyUtil::SendKeyDown(VK_CONTROL);
		KeyUtil::SendKeyDown(VK_F5);
		KeyUtil::SendKeyUp(VK_CONTROL);
		KeyUtil::SendKeyUp(VK_F5);
	}

}

bool keymapper::Mapper::EnumerateJoypads(Window* window)
{
	//Check if any joypad input is pressed
	//..

	//Event to save into
	SDL_Event event;

	//Get the number of joypads open
	int joypadTotalCount = SDL_NumJoysticks();
	int joypadCount = 0;

	if (joypadTotalCount == 0)
	{
		window->RenderError("No joypads detected.");
		return false;
	}
		

	//Render the splash screen, draw some initial text
	window->RenderSplashScreen("assets/img/start-menu.png");
	window->RenderText("Player 1, press start!", window->DEFAULT_WINDOW_WIDTH / 2, 400, window->ALIGN_CENTRE);
	window->RenderPresent();

	while (joypadCount < joypadTotalCount)
	{
		while (SDL_PollEvent(&event))
		{
			//While there are events
			if (event.type == SDL_JOYBUTTONDOWN)
			{
				//Virtual indexing -- this will indicate which
				//pad is set.

				//Get which joypad was pressed
				int joypadID = event.jbutton.which;

				//This joypad is already mapped.. so they can't map it again
				if (enumerationMap.find(joypadID) != enumerationMap.end())
					continue;
				
				//Map this joypad id to the "player id"
				enumerationMap[joypadID] = joypadCount++;

				//Make the text
				std::string text = "Player " + std::to_string(joypadCount + 1) + ", press start!";

				//Render for next time
				window->RenderSplashScreen("assets/img/start-menu.png");
				window->RenderText(text, window->DEFAULT_WINDOW_WIDTH / 2, 400, window->ALIGN_CENTRE);
				window->RenderPresent();
			}
		}
	}

	//Clear window
	window->RenderClear();
	window->RenderPresent();

	//Set flag to false
	this->waitingForVirtualEnumeration = false;

	return true;
}

void __fastcall keymapper::Mapper::OnThreadIteration(keymapper::Window* window)
{
	//Get the event
	SDL_Event event;

	if (!this->waitingForUserStart)
	{
		//It's already been started by the user..
		//..

		if (SDL_TICKS_PASSED(SDL_GetTicks(), this->lastKeyPressed + this->timeoutTicks - this->warningTicks))
		{
			//Have they passed the last key press + timeout? If so.. we need to render a warning
			//..

			//Figure out how many seconds are left
			float diff = ((this->lastKeyPressed + this->timeoutTicks) - SDL_GetTicks()) / 1000.0f;

			//Render the warning
			window->RenderWarning(diff);
		}

		if (SDL_TICKS_PASSED(SDL_GetTicks(), this->lastKeyPressed + this->timeoutTicks))
		{
			//They've passed the warning!
			//..

			//Hide the window
			window->Hide();

			//Refresh chrome, somehow
			this->RestartChrome();

			//Set waiting for user to true
			this->waitingForUserStart = true;
		}
	}	

	while (SDL_PollEvent(&event))
	{
		//Escape pressed? rip in peace
		if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)
			window->Close();

		//Key pressed? Update key time
		if (event.type == SDL_KEYDOWN || event.type == SDL_JOYBUTTONDOWN || event.type == SDL_JOYAXISMOTION)
		{
			//Set last key press to current ticks
			this->lastKeyPressed = SDL_GetTicks();

			//Is waiting for user key press true? Set to false
			if (this->waitingForUserStart)
				this->waitingForUserStart = false;

			//Hide the window otherwise
			else 
				window->Hide();
		}

		//Joy button down?
		if (event.type == SDL_JOYBUTTONDOWN)
			this->MapJoyInputDown(event);

		//Joy button up?
		else if (event.type == SDL_JOYBUTTONUP)
			this->MapJoyInputUp(event);

		//Joy axis motion?
		if (event.type == SDL_JOYAXISMOTION)
			this->MapJoyAxisMotion(event);
	}
}


void keymapper::Mapper::MapJoyAxisMotion(SDL_Event event)
{
	//Firstly, what mapped joypad id is this?
	int mappedJoypadIndex = this->enumerationMap[event.jbutton.which];

	//Use this as an index to the axes mapper
	axisMap[mappedJoypadIndex]->Map(event);
}

void keymapper::Mapper::MapJoyInputDown(SDL_Event event)
{
	//joy button is down, find which button
	//..

	//Firstly, what joypad id is this?
	int mappedJoypadIndex = this->enumerationMap[event.jbutton.which];

	//Get the button
	uint8_t button = event.jbutton.button;

	//Find the mapping
	char mappedKey = (*this->joypadMappings[mappedJoypadIndex])[button];

	//Press that key
	KeyUtil::SendKeyDown(mappedKey);
}

void keymapper::Mapper::MapJoyInputUp(SDL_Event event)
{
	//joy button is up, find which button
	//..

	//Firstly, what joypad id is this?
	int mappedJoypadIndex = this->enumerationMap[event.jbutton.which];

	//Get the button
	uint8_t button = event.jbutton.button;

	//Find the mapping
	char mappedKey = (*this->joypadMappings[mappedJoypadIndex])[button];

	//Press that key
	KeyUtil::SendKeyUp(mappedKey);
}

void keymapper::Mapper::LoadConfigFile(const std::string& path)
{
	//Open the file, not the best way because its been opened already, but whatever
	std::ifstream stream(path);

	if (stream.fail())
	{
		//Send error message
		std::cout << "error: couldn't open file " << path << std::endl;

		//Close the file and return
		stream.close();
		return;
	}

	//Parse the JSON
	json j = json::parse(stream);

	//Parse the variables
	this->warningTicks = 1000 * j["warningSeconds"].get<int>();
	this->timeoutTicks = 1000 * j["timeoutSeconds"].get<int>();
	this->chromePath = j["chromePath"].get<std::string>();
	this->chromeFlags = j["chromeFlags"].get<std::string>();

	//Close the stream
	stream.close();
}

void keymapper::Mapper::AddJoyAxisMap(unsigned int joypadIndex, const char* file)
{
	//Open the joystick at this index (if it hasn't been already)
	SDL_JoystickOpen(joypadIndex);

	//Assign a new key, set to pointer so we don't have to type it in a lot (im so lazy)
	AxisMapper* map = axisMap[joypadIndex] = new AxisMapper();

	//Open the file, not the best way because its been opened already, but whatever
	std::ifstream stream(file);

	if (stream.fail())
	{
		//Send error message
		std::cout << "error: couldn't open file " << file << std::endl;

		//Close the file and return
		stream.close();
		return;
	}

	//Parse the JSON
	json j = json::parse(stream);

	//Get the axes
	std::vector<json> axes = j["axes"];

	for (int i = 0; i < axes.size(); i++)
	{
		//Get the axis
		const json& axis = axes[i];

		//Run through each axis.. get the pos/neg/thresh values
		char positive = axis["positive"].get<char>();
		char negative = axis["negative"].get<char>();
		float threshold = axis["threshold"].get<float>();
		float scale = axis["scale"].get<float>();

		//Add to the axis map
		map->AddMap(positive, negative, threshold, scale);
	}

	//Close the stream
	stream.close();
}


void keymapper::Mapper::AddJoyButtonMap(unsigned int joypadIndex, const char* file)
{
	//Open the joystick at this index
	SDL_JoystickOpen(joypadIndex);

	//Assign a new key in the outer map
	joypadMappings[joypadIndex] = new std::map<int, char>();
	
	//Point to the map
	std::map<int, char>* map = joypadMappings[joypadIndex];

	//Load in json file, set up mappings for joypad mappings
	std::ifstream stream(file);
	
	if (stream.fail())
	{
		//Send error message
		std::cout << "error: couldn't open file " << file << std::endl;

		//Close the file and return
		stream.close();
		return;
	}

	//Parse the JSON
	json j = json::parse(stream);

	//Find the exit/start buttons
	char exitButton  = j["exit"].get<char>();
	char startButton = j["start"].get<char>();

	//Get the buttons
	std::vector<char> buttons = j["buttons"].get<std::vector<char>>();

	//Now add everything to the map! We need to map exit/start first
	(*map)[JOYPAD_BUTTON_IDX_EXIT]  = exitButton;
	(*map)[JOYPAD_BUTTON_IDX_START] = startButton;

	//Then shove the buttons into the map
	for (int i = 0; i < buttons.size(); i++)
		(*map)[JOYPAD_BUTTON_IDX_A + i] = buttons[i];
	
	//Close the stream
	stream.close();
}


