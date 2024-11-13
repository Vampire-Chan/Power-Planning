#include <windows.h>
#include <powrprof.h>
#include <guiddef.h>
#include <vector>
#include <string>
#include <commctrl.h>
#include <ctime>
#pragma comment(lib, "PowrProf.lib")
#pragma comment(lib, "Comctl32.lib")

// Global variables
HINSTANCE hInst;
std::vector<GUID> powerPlans;
std::vector<std::wstring> powerPlanNames;
GUID activePowerPlan;
HFONT hFont;
const std::vector<std::wstring> tips = {
    L"Tip 1: Use Alt + Tab to switch between applications.",
    L"Tip 2: Press Win + L to lock your computer.",
    L"Tip 3: Use Ctrl + Shift + Esc to open Task Manager.",
    // Add more tips as needed
};

// Function to retrieve all power plans
void RetrievePowerPlans() {
    GUID* pActivePlan;
    if (PowerGetActiveScheme(NULL, &pActivePlan) != ERROR_SUCCESS) {
        MessageBox(NULL, L"Failed to get active power plan.", L"Error", MB_OK | MB_ICONERROR);
        return;
    }
    activePowerPlan = *pActivePlan;
    LocalFree(pActivePlan);

    DWORD index = 0;
    DWORD schemeGuidSize = sizeof(GUID);
    WCHAR name[256];
    DWORD nameSize = sizeof(name);

    while (true) {
        GUID* pSchemeGuid = (GUID*)LocalAlloc(LPTR, schemeGuidSize);
        if (!pSchemeGuid) {
            MessageBox(NULL, L"Memory allocation failed.", L"Error", MB_OK | MB_ICONERROR);
            return;
        }

        DWORD result = PowerEnumerate(NULL, NULL, NULL, ACCESS_SCHEME, index++, (UCHAR*)pSchemeGuid, &schemeGuidSize);
        if (result == ERROR_NO_MORE_ITEMS) {
            LocalFree(pSchemeGuid);
            break;
        }
        if (result != ERROR_SUCCESS) {
            MessageBox(NULL, L"Failed to enumerate power schemes.", L"Error", MB_OK | MB_ICONERROR);
            LocalFree(pSchemeGuid);
            return;
        }

        nameSize = sizeof(name);
        if (PowerReadFriendlyName(NULL, pSchemeGuid, NULL, NULL, (UCHAR*)name, &nameSize) == ERROR_SUCCESS) {
            powerPlans.push_back(*pSchemeGuid);
            powerPlanNames.push_back(name);
        }
        LocalFree(pSchemeGuid);
    }
}

// Function to set a power plan
void SetPowerPlan(GUID planGuid) {
    if (PowerSetActiveScheme(NULL, &planGuid) != ERROR_SUCCESS) {
        MessageBox(NULL, L"Failed to set power plan.", L"Error", MB_OK | MB_ICONERROR);
    }
}

// Utility to randomly pick a tip
std::wstring GetRandomTip() {
    srand((unsigned)time(0));
    int randomIndex = rand() % tips.size();
    return tips[randomIndex];
}

// Window procedure
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        // Initialize power plans and UI components
        RetrievePowerPlans();

        // Font setup
        hFont = CreateFont(18, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
            VARIABLE_PITCH, TEXT("Segoe UI"));

        // Get window width and height
        RECT rect;
        GetClientRect(hwnd, &rect);
        int windowWidth = rect.right - rect.left;
        int windowHeight = rect.bottom - rect.top;

        // Calculate positions for buttons and text to center them
        int buttonWidth = 200;
        int buttonHeight = 30;
        int yOffset = 50;

        // Create buttons for each power plan
        for (size_t i = 0; i < powerPlanNames.size(); ++i) {
            int xPos = (windowWidth - buttonWidth) / 2;  // Center the button
            HWND btn = CreateWindowW(L"BUTTON", powerPlanNames[i].c_str(), WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                xPos, yOffset + (int)(i * 40), buttonWidth, buttonHeight, hwnd, (HMENU)(i + 1), hInst, NULL);
            SendMessage(btn, WM_SETFONT, (WPARAM)hFont, TRUE);
        }

        // Create Reset button
        HWND resetBtn = CreateWindowW(L"BUTTON", L"Reset to High Performance Plan", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            (windowWidth - buttonWidth) / 2, yOffset + (int)(powerPlanNames.size() * 40), buttonWidth, buttonHeight, hwnd, (HMENU)(100), hInst, NULL);
        SendMessage(resetBtn, WM_SETFONT, (WPARAM)hFont, TRUE);

        // Display random tip
        std::wstring tip = GetRandomTip();
        HWND tipText = CreateWindowW(L"STATIC", tip.c_str(), WS_VISIBLE | WS_CHILD | SS_CENTER,
            (windowWidth - 280) / 2, 20, 280, 20, hwnd, NULL, hInst, NULL);
        SendMessage(tipText, WM_SETFONT, (WPARAM)hFont, TRUE);
        break;
    }

    case WM_COMMAND:
        if (LOWORD(wParam) == 100) {  // Reset button clicked
            GUID balancedGUID = GUID_MIN_POWER_SAVINGS;  // GUID for Balanced Power Plan
            SetPowerPlan(balancedGUID);
            MessageBox(hwnd, L"Switched to High Performance Plan", L"Success", MB_OK);
        }
        else if (LOWORD(wParam) > 0 && LOWORD(wParam) <= powerPlans.size()) {
            GUID selectedPlan = powerPlans[LOWORD(wParam) - 1];
            SetPowerPlan(selectedPlan);
            MessageBox(hwnd, L"Power Plan Changed", L"Success", MB_OK);
        }
        break;

    case WM_DESTROY:
        DeleteObject(hFont);
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// WinMain - Entry point
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    hInst = hInstance;
    const wchar_t CLASS_NAME[] = L"PowerPlanWindow";

    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = CreateSolidBrush(RGB(240, 240, 240)); // Light grey background
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(0, CLASS_NAME, L"Power Plan Changer", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 300, 400, NULL, NULL, hInstance, NULL);

    if (hwnd == NULL) return 0;

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}
