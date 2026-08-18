// WinapiPortfolio.cpp : 애플리케이션에 대한 진입점을 정의합니다.
//

#include <memory>
#include <filesystem>
#include "framework.h"
#include "WinapiPortfolio.h"
#include "D2DFramework/Graphics/include/Graphics.h"
#include "D2DFramework/Manager/include/SceneManager.h"
#include "D2DFramework/Manager/include/DataManager.h"
#include "D2DFramework/Manager/include/InputManager.h"
#include "D2DFramework/Manager/include/SpriteSheetManager.h"
#include "Level/Dungeon.h"
#include "Level/ObjectEditor.h"
#include "Level/D01/D01.h"
#include "Level/SpriteSetEditor.h"
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
// D01/Dungeon/ObjectEditor 등이 멤버로 갖는 ComPtr<IDWriteTextFormat> 등을 make_shared로
// 생성/해제하려면 이 번역 단위에서도 완전한 타입 정의가 필요하다 (FPS 표시 자체는 제거했지만
// include는 그래서 남겨둔다).
#include <dwrite.h>
#pragma comment(lib, "dwrite.lib")

#define MAX_LOADSTRING 100

// 전역 변수:
HINSTANCE hInst;                                // 현재 인스턴스입니다.
WCHAR szTitle[MAX_LOADSTRING];                  // 제목 표시줄 텍스트입니다.
WCHAR szWindowClass[MAX_LOADSTRING];            // 기본 창 클래스 이름입니다.
HWND gHwnd;

std::shared_ptr<Graphics> graphics;

// 이 코드 모듈에 포함된 함수의 선언을 전달합니다:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: 여기에 코드를 입력합니다.
    timeBeginPeriod(1);

    LARGE_INTEGER freq, prev, now;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&prev);

    // 전역 문자열을 초기화합니다.
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_WINAPIPORTFOLIO, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // 애플리케이션 초기화를 수행합니다:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_WINAPIPORTFOLIO));

    InputManager::GetInstance().Init(gHwnd);

    // 리소스 경로를 exe 자신의 위치 기준으로 잡는다. 상대 경로(예: L"Resources/Texture")를 그대로
    // 쓰면 현재 작업 디렉터리(exe를 실행한 위치) 기준으로 풀리는데, 탐색기에서 exe를 직접
    // 더블클릭하면 작업 디렉터리가 exe가 있는 폴더가 되어 리소스를 못 찾는다 (Visual Studio의
    // F5 실행만 작업 디렉터리를 프로젝트 폴더로 잡아줘서 우연히 동작했었다).
    // Resources는 빌드 후 PostBuildEvent가 exe와 같은 출력 폴더로 복사해두므로, exe 바로
    // 옆에서 찾는다 - 이렇게 해야 x64\Release 폴더 하나만 통째로 다른 환경에 배포할 수 있다.
    wchar_t exePathBuf[MAX_PATH];
    GetModuleFileNameW(nullptr, exePathBuf, MAX_PATH);
    std::filesystem::path exeDir = std::filesystem::path(exePathBuf).parent_path();

    SpriteSheetManager::GetInstance().Init(graphics, exeDir / L"Resources/Texture");
    DataManager::GetInstance().Init(exeDir / L"Resources/Data");
    GameLevel::Init(graphics);
    SceneManager::GetInstance().Init();
    SceneManager::GetInstance().LoadInitialLevel(std::make_shared<D01>());

    MSG msg;
    msg.message = WM_NULL;

    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        QueryPerformanceCounter(&now);
        double deltaTime = (double)(now.QuadPart - prev.QuadPart) / freq.QuadPart;

        //Update
        InputManager::GetInstance().Update();

        // 레벨 전환: F1 = Dungeon, F2 = ObjectEditor, F3 = D01, F4 = SpriteSetEditor
        /*
        if (InputManager::GetInstance().GetButtonDown(KeyType::F1))
        {
            SceneManager::GetInstance().SwitchLevel(std::make_shared<Dungeon>());
        }
        if (InputManager::GetInstance().GetButtonDown(KeyType::F2))
        {
            SceneManager::GetInstance().SwitchLevel(std::make_shared<ObjectEditor>());
        }
        if (InputManager::GetInstance().GetButtonDown(KeyType::F4))
        {
            SceneManager::GetInstance().SwitchLevel(std::make_shared<SpriteSetEditor>());
        }
        */
        if (InputManager::GetInstance().GetButtonDown(KeyType::F3))
        {
            SceneManager::GetInstance().SwitchLevel(std::make_shared<D01>());
        }

        SceneManager::GetInstance().Update(deltaTime);

        //PostUpdate
        SceneManager::GetInstance().PostUpdate();

        //Render
        graphics->BeginDraw();
        SceneManager::GetInstance().Render();
        graphics->EndDraw();

        prev = now;
    }

    return (int) msg.wParam;
}



//
//  함수: MyRegisterClass()
//
//  용도: 창 클래스를 등록합니다.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_WINAPIPORTFOLIO));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = 0;
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   함수: InitInstance(HINSTANCE, int)
//
//   용도: 인스턴스 핸들을 저장하고 주 창을 만듭니다.
//
//   주석:
//
//        이 함수를 통해 인스턴스 핸들을 전역 변수에 저장하고
//        주 프로그램 창을 만든 다음 표시합니다.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // 인스턴스 핸들을 전역 변수에 저장합니다.

   gHwnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!gHwnd)
   {
      return FALSE;
   }

   graphics = std::make_shared<Graphics>();
   if (!graphics->Init(gHwnd))
   {
       return false;
   }

   ShowWindow(gHwnd, nCmdShow);
   UpdateWindow(gHwnd);

   return TRUE;
}

//
//  함수: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  용도: 주 창의 메시지를 처리합니다.
//
//  WM_COMMAND  - 애플리케이션 메뉴를 처리합니다.
//  WM_PAINT    - 주 창을 그립니다.
//  WM_DESTROY  - 종료 메시지를 게시하고 반환합니다.
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // 메뉴 선택을 구문 분석합니다:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_SIZE:
        {
        UINT width = LOWORD(lParam);
        UINT height = HIWORD(lParam);
        if (graphics) {
            //graphics->Resize(width, height);
        }
        if (height > 0)
        {
            SceneManager::GetInstance().OnResize((float)width, (float)height);
        }
        }
        break;
    case WM_ERASEBKGND:
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// 정보 대화 상자의 메시지 처리기입니다.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
