#pragma once

#pragma once

#include <vector>
#include <windows.h>

#include "../../../D2DFramework/Core/include/Singleton.h"

enum class KeyType
{
    LeftMouse = VK_LBUTTON,
    RightMouse = VK_RBUTTON,

    Up = VK_UP,
    Down = VK_DOWN,
    Left = VK_LEFT,
    Right = VK_RIGHT,
    SpaceBar = VK_SPACE,

    KEY_1 = '1',
    KEY_2 = '2',

    W = 'W',
    A = 'A',
    S = 'S',
    D = 'D',
    L = 'L',
    Q = 'Q',
    E = 'E',

    F1 = VK_F1,
    F2 = VK_F2,
    F3 = VK_F3,
    F4 = VK_F4,
};

enum class KeyState
{
    None,
    Press,
    Down,
    Up,

    End
};

constexpr int KEY_TYPE_COUNT = static_cast<int>(UINT8_MAX) + 1;

class InputManager : public Singleton<InputManager>
{
    friend Singleton<InputManager>;

public:
    void Init(HWND hwnd);
    void Update();

    bool GetButtonPressed(KeyType key) { return GetState(key) == KeyState::Press; }

    bool GetButtonDown(KeyType key) { return GetState(key) == KeyState::Down; }

    bool GetButtonUp(KeyType key) { return GetState(key) == KeyState::Up; }

private:
    KeyState GetState(KeyType key) { return _states[static_cast<unsigned char>(key)]; }

private:
    InputManager() = default;
    ~InputManager() = default;

    HWND _hwnd = 0;
    std::vector<KeyState> _states;
};
