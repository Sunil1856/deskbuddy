#include "app_games.h"
#include "game_dino.h"
#include "game_flappy.h"
#include "game_stack.h"
#include "game_reaction.h"
#include "game_toss.h"
#include "game_oracle.h"

AppGames::AppGames() : _dm(nullptr), _storage(nullptr), _face(nullptr), _selectedGame(GameID::NONE), _menuIndex(0) {
}

void AppGames::begin(DisplayManager* dm, Storage* storage, FaceRenderer* face) {
    _dm = dm;
    _storage = storage;
    _face = face;
}

AppGames::~AppGames() {
    _exitCurrentGame();
}

void AppGames::onEnter() {
    _selectedGame = GameID::NONE;
    _menuIndex = 0;
}

void AppGames::onExit() {
    _exitCurrentGame();
}

void AppGames::update() {
    switch (_selectedGame) {
        case GameID::DINO: if (_dino) _dino->update(); break;
        case GameID::FLAPPY: if (_flappy) _flappy->update(); break;
        case GameID::STACK: if (_stack) _stack->update(); break;
        case GameID::REACTION: if (_reaction) _reaction->update(); break;
        case GameID::TOSS: if (_toss) _toss->update(); break;
        case GameID::ORACLE: if (_oracle) _oracle->update(); break;
        default: break;
    }
}

void AppGames::handleTouch(TouchEvent event) {
    if (_selectedGame == GameID::NONE) {
        // Menu navigation: Tap to cycle, Long press to select
        if (event == TouchEvent::TAP) {
            _menuIndex = (_menuIndex + 1) % (int)GameID::COUNT;
        } else if (event == TouchEvent::LONG_PRESS) {
            _startGame((GameID)_menuIndex);
        }
    } else {
        // Active gameplay:
        // Hold/Long Press exits to game selection menu (no accidental exit during rapid taps!)
        if (event == TouchEvent::LONG_PRESS || event == TouchEvent::HOLD) {
            _exitCurrentGame();
            if (_face) _face->setExpression(Expression::NEUTRAL, 300);
            return;
        }
        
        // Fast 0ms latency input for games:
        // Route TOUCH_DOWN to active game immediately. Suppress delayed TAP to avoid double actions.
        if (event == TouchEvent::TOUCH_DOWN) {
            switch (_selectedGame) {
                case GameID::DINO: if (_dino) _dino->handleTouch(event); break;
                case GameID::FLAPPY: if (_flappy) _flappy->handleTouch(event); break;
                case GameID::STACK: if (_stack) _stack->handleTouch(event); break;
                case GameID::REACTION: if (_reaction) _reaction->handleTouch(event); break;
                case GameID::TOSS: if (_toss) _toss->handleTouch(event); break;
                case GameID::ORACLE: if (_oracle) _oracle->handleTouch(event); break;
                default: break;
            }
        }
    }
}

void AppGames::_startGame(GameID id) {
    _exitCurrentGame();
    _selectedGame = id;
    
    switch (id) {
        case GameID::DINO: 
            _dino = new GameDino(_dm, _storage, _face); 
            _dino->onEnter();
            break;
        case GameID::FLAPPY: 
            _flappy = new GameFlappy(_dm, _storage, _face); 
            _flappy->onEnter();
            break;
        case GameID::STACK: 
            _stack = new GameStack(_dm, _storage, _face); 
            _stack->onEnter();
            break;
        case GameID::REACTION: 
            _reaction = new GameReaction(_dm, _storage, _face); 
            _reaction->onEnter();
            break;
        case GameID::TOSS:
            _toss = new GameToss(_dm, _storage, _face);
            _toss->onEnter();
            break;
        case GameID::ORACLE:
            _oracle = new GameOracle(_dm, _storage, _face);
            _oracle->onEnter();
            break;
        default: break;
    }
}

void AppGames::_exitCurrentGame() {
    if (_dino) { delete _dino; _dino = nullptr; }
    if (_flappy) { delete _flappy; _flappy = nullptr; }
    if (_stack) { delete _stack; _stack = nullptr; }
    if (_reaction) { delete _reaction; _reaction = nullptr; }
    if (_toss) { delete _toss; _toss = nullptr; }
    if (_oracle) { delete _oracle; _oracle = nullptr; }
    
    _selectedGame = GameID::NONE;
    
    if (_dm) {
        _dm->display().invertDisplay(false);
    }
}

void AppGames::render() {
    if (!_dm) return;
    
    if (_selectedGame == GameID::NONE) {
        _renderMenu();
    } else {
        switch (_selectedGame) {
            case GameID::DINO: if (_dino) _dino->render(); break;
            case GameID::FLAPPY: if (_flappy) _flappy->render(); break;
            case GameID::STACK: if (_stack) _stack->render(); break;
            case GameID::REACTION: if (_reaction) _reaction->render(); break;
            case GameID::TOSS: if (_toss) _toss->render(); break;
            case GameID::ORACLE: if (_oracle) _oracle->render(); break;
            default: break;
        }
    }
}

void AppGames::_renderMenu() {
    Adafruit_SSD1306& d = _dm->display();
    
    d.setTextSize(1);
    d.setTextColor(SSD1306_WHITE);
    _dm->drawCenteredText("1-TAP GAMES", 2, 1);
    d.drawLine(0, 12, SCREEN_W, 12, SSD1306_WHITE);
    
    const char* gameNames[] = {
        "DINO RUN",
        "FLAPPY BIRD",
        "STACK TOWER",
        "REACTION TEST",
        "COIN TOSS",
        "MAGIC ORACLE"
    };
    
    d.setTextSize(1);
    _dm->drawCenteredText(gameNames[_menuIndex], 30, 2);
    
    // Centered Page Dots for 6 games
    int startX = SCREEN_W / 2 - ((int)GameID::COUNT * 9) / 2 + 2;
    for (int i = 0; i < (int)GameID::COUNT; i++) {
        int x = startX + i * 9;
        if (i == _menuIndex) {
            d.fillCircle(x, 58, 2, SSD1306_WHITE);
        } else {
            d.drawPixel(x, 58, SSD1306_WHITE);
        }
    }
}
