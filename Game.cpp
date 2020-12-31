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
	int optionsNumber = 3;
	int levelIndex = _currentLevelIndex;
	bool keyPressed = false;

	auto printSelectionScreen = [&selection, &levelIndex]() {
		system("cls");
		std::cout << "//..........................................//" << std::endl;
		std::cout << "//.................................Level:" << levelIndex << "..//" << std::endl;
		std::cout << "//................." << (selection == 0 ? "[Start]" : ".Start.") << "..................//" << std::endl;
		std::cout << "//............." << (selection == 1l ? "[Change level]" : ".Change level.") << "...............//" << std::endl;
		std::cout << "//................." << (selection == 2 ? "[Exit]" : ".Exit.") << "...................//" << std::endl;
		std::cout << "//..........................................//" << std::endl;
		std::cout << "//..........................................//";
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
				_currentLevel->GetMap()->SetCharacterAt(tile.GetPosition(), newCharacter);
				_currentLevel->GetMap()->RemoveOptionAt(tile.GetPosition(), OPTION::ADD_GOLD);
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
	auto printGameOverScreen = [&selection]() {
		system("cls");
		std::cout << "//...........................................//" << std::endl;
		std::cout << "//.................Game Over.................//" << std::endl;
		std::cout << "//...........................................//" << std::endl;
		std::cout << "//................." << (selection == 0 ? "[Restart]" : ".Restart.") << ".................//" << std::endl;
		std::cout << "//.............." << (selection == 1 ? "[Start Screen]" : ".Start Screen.") << "...............//" << std::endl;
		std::cout << "//.................." << (selection == 2 ? "[Quit]" : ".Quit.") << "...................//" << std::endl;
		std::cout << "//...........................................//" << std::endl;
		std::cout << "//...........................................//";
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
