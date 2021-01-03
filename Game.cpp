#include "Game.h"

Game::Game(const std::vector<std::string>& filenames, const float& frameRate)
{
	_levels = filenames;
	_frameRate = frameRate;
	_timer = new Timer();
	_playerJumping = true;
	_jumpingMaxFrame = 4;
	_jumpingFrame = _jumpingMaxFrame + 1;
}

Game::~Game()
{
	delete _currentLevel;
	delete _timer;
}

void Game::LoadLevel(const int& levelIndex)
{
	_currentLevelIndex = levelIndex;
	
	if (_currentLevelIndex >= int(_levels.size()) or _currentLevelIndex < 0)
	{
		throw new Exception(3, "[LEVEL] level index out of size.");
	}

	std::ifstream levelStream(_levels[_currentLevelIndex]);

	if (levelStream.good())
	{
		_currentLevel = new Level(levelStream);
		_playerJumping = true;
		_jumpingFrame = _jumpingMaxFrame + 1; // to prevent jumping if spawned in the air
	}
	else
	{
		throw new Exception(1, "[LEVEL] file open error");
	}

	levelStream.close();
}

bool Game::SelectionScreen()
{
	int selection = 0; // selection: 0=start; 1=change leve; 2=exit
	int optionsNumber = 4;
	int levelIndex = _currentLevelIndex;
	bool keyPressed = false;

	auto printSelectionScreen = [&selection, &levelIndex]() {
		std::string backgroundColor = "\u001b[30m\u001b[40m"; // black background, black text
		std::string textColor = "\u001b[37m\u001b[40m"; //white text, black background
		std::string selectedColor = "\u001b[32m\u001b[40m"; //green text, black background
		std::string borderColor = "\u001b[37m\u001b[40m"; //white text, black background
		std::string exitColor = "\u001b[31m\u001b[40m"; //red text, black bakcground
		
		system("cls");

		std::cout << borderColor << "//////////////////////////////////////////////" << std::endl;
		std::cout << borderColor << "//" << backgroundColor << ".........................................." << borderColor << "//" << std::endl;
		std::cout << borderColor << "//" << backgroundColor << "................................." << textColor << "Level:" << levelIndex << backgroundColor << ".." << borderColor << "//" << std::endl;
		std::cout << borderColor << "//" << backgroundColor << "................." << (selection == 0 ? selectedColor + "[Start]" : "." + textColor + "Start" + backgroundColor + ".") << backgroundColor << ".................." << borderColor << "//" << std::endl;
		std::cout << borderColor << "//" << backgroundColor << "............." << (selection == 1 ? selectedColor + "[Change level]" : "." + textColor + "Change level" + backgroundColor + ".") << backgroundColor << "..............." << borderColor << "//" << std::endl;
		std::cout << borderColor << "//" << backgroundColor << "............." << (selection == 2 ? selectedColor + "[How to play?]" : "."+ textColor + "How to play?" + backgroundColor + ".") << backgroundColor << "..............." << borderColor << "//" << std::endl;
		std::cout << borderColor << "//" << backgroundColor << "................." << (selection == 3 ? exitColor + "[Exit]" : "." + textColor + "Exit" + backgroundColor + ".") << backgroundColor << "..................." << borderColor << "//" << std::endl;
		std::cout << borderColor << "//" << backgroundColor << ".........................................." << borderColor << "//" << std::endl;
		std::cout << borderColor << "//" << backgroundColor << ".........................................." << borderColor << "//" << std::endl;
		std::cout << borderColor << "//////////////////////////////////////////////" << /* reset colors */ "\u001b[0m" << std::endl;
	};
	printSelectionScreen();

	while (true)
	{
		_timer->Tick();
		if (_timer->DeltaTime() >= 1.0f / _frameRate)
		{
			// up arrow
			if (GetAsyncKeyState(VK_UP) and 0x26)
			{
				keyPressed = true;
				selection = ((selection - 1) % optionsNumber) < 0 ? optionsNumber - 1 : selection - 1;
				printSelectionScreen();
			}

			// down arrow
			else if (GetAsyncKeyState(VK_DOWN) and 0x28)
			{
				keyPressed = true;
				selection = (selection + 1) % optionsNumber;
				printSelectionScreen();
			}

			// enter
			else if (GetAsyncKeyState(VK_RETURN) and 0x0D)
			{
				keyPressed = true;
				switch (selection)
				{
				case 0:
					LoadLevel(levelIndex);
					return true;

				case 1:
					levelIndex = (levelIndex + 1) % _levels.size();
					printSelectionScreen();
					break;

				case 2:
					HowToPlayScreen();
					printSelectionScreen();
					break;

				case 3:
					return false;
				}
			}

			if (keyPressed)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
				keyPressed = false;
			}
		}
	}

	return false;
}

bool Game::MovePossible(std::vector<Position>& positions, const Position& direction)
{
	for (Position& position : positions)
	{
		position += direction;
	}

	return !_currentLevel->GetMap()->CollidingWith(positions);
}

void Game::CheckOptions()
{
	for (EntityTile& tile : _currentLevel->GetOptionTiles())
	{
		if (_currentLevel->GetPlayer()->CollidingWith(tile))
		{
			Option option;

			option = tile.GetOption(OPTION::SWITCH_MAP);
			if (option.optionName != OPTION::OPTION_ERROR)
			{
				Position newPlayerPosition = Position({ option.arguments[1], option.arguments[2] });
				int newMapIndex = option.arguments[0];
				_currentLevel->LoadMap(newMapIndex); //option.arguments[0]
				_currentLevel->GetPlayer()->SetPosition(newPlayerPosition);
				Update({0,0});
				_currentLevel->GetMap()->Show();
				HUD();
				//std::this_thread::sleep_for(std::chrono::milliseconds(100)); //so it doesn't bug
			}

			option = tile.GetOption(OPTION::DEAL_DMG);
			if (option.optionName != OPTION::OPTION_ERROR)
			{
				_currentLevel->GetPlayer()->LoseHp(option.arguments[0]);
				HUD();
			}

			option = tile.GetOption(OPTION::ADD_GOLD);
			if (option.optionName != OPTION::OPTION_ERROR)
			{
				_currentLevel->AddGold(option.arguments[0]);
				HUD();

				//change to different tile in original map & remove gold option
				char newCharacter = static_cast<char>(option.arguments[1]);
				int newTileColor = option.arguments[2];
				int newBackgroundColor = option.arguments[3];
				_currentLevel->GetMap()->SetCharacterAt(tile.GetPosition(), newCharacter);
				_currentLevel->GetMap()->RemoveOptionAt(tile.GetPosition(), OPTION::ADD_GOLD);
				_currentLevel->GetMap()->SetTileColorAt(tile.GetPosition(), newTileColor);
				_currentLevel->GetMap()->SetTileBackgroundColorAt(tile.GetPosition(), newBackgroundColor);

				_currentLevel->AssignOptionTiles();
			}
		}
	}
}

void Game::ApplyGravity()
{
	//check if a block below player {or any gravity-object} is collidable then move down if isn't
	Position direction = { 0,1 };
	_currentLevel->GetPlayer()->SetDirection(direction);
	std::vector<Position> playerPositions = _currentLevel->GetPlayer()->GetCollidingPositions();

	if (MovePossible(playerPositions, direction))
	{
		Update(direction);
	}
	else
	{
		_playerJumping = false;
		_jumpingFrame = 0;
	}
}

void Game::Update(const Position& direction)
{
	//set player direction
	_currentLevel->GetPlayer()->SetDirection(direction);

	std::vector<EntityTile> oldState = _currentLevel->GetPlayer()->GetBody();
	_currentLevel->GetPlayer()->Update();
	std::vector<EntityTile> newState = _currentLevel->GetPlayer()->GetBody();

	_currentLevel->GetMap()->UpdateMap(oldState, newState);
}

void Game::KeyboardInput(Position& direction)
{
	direction = { 0,0 };

	if (GetAsyncKeyState(VK_UP) and 0x26)
	{
		//up arrow
		if (!_playerJumping)
		{
			_playerJumping = true;
		}
	}

	if (GetAsyncKeyState(VK_DOWN) and 0x28)
	{
		//down arrow
		direction.y = 1;
	}

	if (GetAsyncKeyState(VK_RIGHT) and 0x27)
	{
		//right arrow
		direction.x = 1;
	}

	if (GetAsyncKeyState(VK_LEFT) and 0x25)
	{
		//left arrow
		direction.x = -1;
	}
}

void Game::Jump()
{
	if (_playerJumping)
	{
		_jumpingFrame++;
		std::vector<Position> playerPositions = _currentLevel->GetPlayer()->GetCollidingPositions();

		if (_jumpingFrame < _jumpingMaxFrame)
		{
			Position topLeft = _currentLevel->GetPlayer()->TopLeft() + Position({0, -2});
			Position bottomRight = _currentLevel->GetPlayer()->BottomRight() + Position({0, -2});
			if (MovePossible(playerPositions, { 0, -1 }) and _currentLevel->GetMap()->InBoundings(topLeft) and _currentLevel->GetMap()->InBoundings(bottomRight))
			{
				//gravity forces 1 down so you need to go 2 up
				_currentLevel->GetPlayer()->SetDirection({ 0, -2 });
				Update({ 0,-2 }); 
			}
			else
			{
				//end jumping
				_jumpingFrame = _jumpingMaxFrame;
				//make it look more realistic
				_currentLevel->GetPlayer()->SetDirection({ 0,-1 });
				Update({ 0,-1 }); 
			}
		}
		
		else if (_jumpingFrame == _jumpingMaxFrame) //hung in the air on the last frame so it looks more realistic
		{
			if (MovePossible(playerPositions, { 0, -1 }))
			{
				_currentLevel->GetPlayer()->SetDirection({ 0,-1 });
				Update({ 0,-1 });
			}
		}
	}
}

void Game::Move(const Position& direction)
{
	if (direction != Position{ 0, 0 })
	{
		std::vector<Position> playerPositions = _currentLevel->GetPlayer()->GetCollidingPositions();

		if (MovePossible(playerPositions, direction))
		{
			_currentLevel->GetPlayer()->SetDirection(direction);
			Update(direction);
		}
	}
}

void Game::GameOver()
{
	int selection = 0; // selection: 0=restart; 1=quit
	int optionsNumber = 3;
	bool keyPressed = false;
	bool selected = false;
	
	//displaying
	auto printGameOverScreen = [this, &selection]() {
		std::string backgroundColor = "\u001b[30m\u001b[40m"; // black background, black text
		std::string textColor = "\u001b[37m\u001b[40m"; //white text, black background
		std::string selectedColor = "\u001b[32m\u001b[40m"; //green text, black background
		std::string borderColor = "\u001b[37m\u001b[40m"; //white text, black background
		std::string gameoverColor = "\u001b[31m\u001b[40m"; //red text, black bakcground
		std::string exitColor = "\u001b[31m\u001b[40m"; //red text, black bakcground

		system("cls");

		std::cout << borderColor << "///////////////////////////////////////////////" << std::endl;
		std::cout << borderColor << "//" << backgroundColor << "..........................................." << borderColor << "//" << std::endl;
		std::cout << borderColor << "//" << backgroundColor << "................."<< gameoverColor  << "Game Over" << backgroundColor << "................." << borderColor << "//" << std::endl;
		std::cout << borderColor << "//" << backgroundColor << "..........................................." << borderColor << "//" << std::endl;
		std::cout << borderColor << "//" << backgroundColor << "................." << (selection == 0 ? selectedColor + "[Restart]" : "." + textColor + "Restart" + backgroundColor + ".") << backgroundColor << "................." << borderColor << "//" << std::endl;
		std::cout << borderColor << "//" << backgroundColor << ".............." << (selection == 1 ? selectedColor + "[Start Screen]" : "." + textColor + "Start Screen" + backgroundColor + ".") << backgroundColor << "..............." << borderColor << "//" << std::endl;
		std::cout << borderColor << "//" << backgroundColor << ".................." << (selection == 2 ? exitColor + "[Exit]" : "." + textColor + "Exit" + backgroundColor + ".") << backgroundColor << "..................." << borderColor << "//" << std::endl;
		std::cout << borderColor << "//" << backgroundColor << "..........................................." << borderColor << "//" << std::endl;
		std::cout << borderColor << "//" << backgroundColor << "..........................................." << borderColor << "//" << std::endl;
		std::cout << borderColor << "///////////////////////////////////////////////" << /* reset colors */ "\u001b[0m" << std::endl;
	};

	printGameOverScreen();
	while (!selected)
	{
		_timer->Tick();
		if (_timer->DeltaTime() >= 1.0f / _frameRate)
		{
			// up arrow
			if (GetAsyncKeyState(VK_UP) and 0x26)
			{
				keyPressed = true;
				selection = ((selection - 1) % optionsNumber) < 0 ? optionsNumber - 1 : selection - 1;
				printGameOverScreen();
			}

			// down arrow
			else if (GetAsyncKeyState(VK_DOWN) and 0x28)
			{
				keyPressed = true;
				selection = (selection + 1) % optionsNumber;
				printGameOverScreen();
			}

			// enter
			else if (GetAsyncKeyState(VK_RETURN) and 0x0D)
			{
				selected = true;
			}

			if (keyPressed)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
				keyPressed = false;
			}
		}
	}

	if (selection == 0)
	{
		RestartLevel();
	}
	else if (selection == 1)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100)); // to prevent accidental game start
		Start();
	}
	else if (selection == 2)
	{
		return;
	}
}

void Game::GameLoop()
{
	while (!_currentLevel->GetPlayer()->Dead())
	{
		_timer->Tick();
		
		if (_timer->DeltaTime() >= 1.0f / _frameRate)
		{
			Position direction;
			KeyboardInput(direction);
			Jump();
			Move(direction);
			CheckOptions();
			ApplyGravity();
			std::this_thread::sleep_for(std::chrono::milliseconds(35));
		}
	}

	GameOver();
}

void Game::HUD()
{
	int mapHeight = _currentLevel->GetMap()->GetHeight();
	int mapWidth = _currentLevel->GetMap()->GetWidth();
	int maxHp = _currentLevel->GetPlayer()->MaxHp();
	int Hp = _currentLevel->GetPlayer()->Hp();
	int gold = _currentLevel->GetGold();

	//clear HUD
	_currentLevel->GetMap()->GotoPosition({ 0, mapHeight + 1 });

	for (int i = 0; i < mapWidth + 2 * maxHp; i++)
	{
		std::cout << " ";
	}

	//draw new HUD data
	_currentLevel->GetMap()->GotoPosition({ 0, mapHeight + 1 });

	//gold
	std::cout << "\u001b[37mGold: \u001b[33m" << gold << " "; //white and yellow colors

	auto digits = [](int number) {
		int numberOfDigits = 0;

		if (number < 0)
		{
			number = -number;
			numberOfDigits++;
		}

		while (number > 0)
		{
			number /= 10;
			numberOfDigits++;
		}
		return numberOfDigits;
	};

	//hearts
	for (int i = 0; i < mapWidth - 2 * maxHp - 7 /* Hearts: - 7 chars */- 6 /* Gold: - 6 chars */ - digits(gold) - 1 /* 1 minimum space char */; i++)
	{
		std::cout << " ";
	}

	std::cout << "\u001b[37mHearts:\u001b[31m"; //red color
	for (int i = 0; i < Hp; i++)
	{
		std::cout << " *";
	}
	std::cout << "\u001b[37m"; //white color
	for (int i = 0; i < maxHp - Hp; i++)
	{
		std::cout << " _";
	}

	//level info
	std::cout << "\n\nLevel: (" << _currentLevelIndex << ") [" << _levels[_currentLevelIndex] << "]";
}

void Game::RestartLevel()
{
	Update({ 0,0 });
	LoadLevel(_currentLevelIndex);
	_currentLevel->GetMap()->Show();
	HUD();
	GameLoop();
}

void Game::Start()
{
	if (!SelectionScreen()) // false if exit was selected;
	{
		return;
	}

	RestartLevel();
}

void Game::HowToPlayScreen()
{
	for (int i = 15; i > 0; i--)
	{
		std::string backgroundColor = "\u001b[30m\u001b[40m"; // black background, black text
		std::string textColor = "\u001b[37m\u001b[40m"; //white text, black background
		std::string borderColor = "\u001b[37m\u001b[40m"; //white text, black background
		std::string highlightColor = "\u001b[36m\u001b[40m"; //cyan text, black background

		system("cls");

		std::cout << borderColor << "//////////////////////////////////////////////////////" << std::endl;
		std::cout << borderColor << "//" << backgroundColor << ".................................................." << borderColor << "//" << std::endl;
		std::cout << borderColor << "//" << backgroundColor << "................." << textColor << "Welcome to " << highlightColor << "2dcg!" << backgroundColor << "................." << borderColor << "//" << std::endl;
		std::cout << borderColor << "//" << backgroundColor << ".................................................." << borderColor << "//" << std::endl;
		std::cout << borderColor << "//" << backgroundColor << "............" << textColor << "Use arrow buttons to navigate" << backgroundColor << "........." << borderColor << "//" << std::endl;
		std::cout << borderColor << "//" << backgroundColor << "..................." << textColor << "around the map" << backgroundColor << "................." << borderColor << "//" << std::endl;
		std::cout << borderColor << "//" << backgroundColor << ".................................................." << borderColor << "//" << std::endl;
		std::cout << borderColor << "//" << backgroundColor << "............" << textColor << "There are some special blocks" << backgroundColor << "........." << borderColor << "//" << std::endl;
		std::cout << borderColor << "//" << backgroundColor << "........." << textColor << "that may damage your, give you gold" << backgroundColor << "......" << borderColor << "//" << std::endl;
		std::cout << borderColor << "//" << backgroundColor << "........" << textColor << "or even teleport you to another room" << backgroundColor << "......" << borderColor << "//" << std::endl;
		std::cout << borderColor << "//" << backgroundColor << ".................................................." << borderColor << "//" << std::endl;
		std::cout << borderColor << "//" << backgroundColor << "..............." << textColor << "You will be redirected" << backgroundColor << "............." << borderColor << "//" << std::endl;
		std::cout << borderColor << "//" << backgroundColor << ".............." << textColor << "to the main screen in: " << highlightColor << i << ( i >= 10 ? "" : backgroundColor + "." ) << backgroundColor << "..........." << borderColor << "//" << std::endl; //11,41
		std::cout << borderColor << "//" << backgroundColor << ".................................................." << borderColor << "//" << std::endl;
		std::cout << borderColor << "//" << backgroundColor << ".................................................." << borderColor << "//" << std::endl;
		std::cout << borderColor << "//////////////////////////////////////////////////////" << /* reset colors */ "\u001b[0m" << std::endl;
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}
}