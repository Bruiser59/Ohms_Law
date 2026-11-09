// OhmsLaw.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "OhmsLaw.h"
#include "COhmsLaw.h"

#include <cstring>
#include <cwchar>
#include <exception>
#include <string>

// Use Common Controls v6 so the form gets the modern visual style.
#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' " \
    "version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
HWND hSolver;                                   // solver form hosted in the main window

// One row of the solver form, bound to the matching COhmsLaw setter/getter.
struct Field {
    int                 id;
    const wchar_t*      name;
    elec::COhmsLaw&     (elec::COhmsLaw::*set)(double);
    double              (elec::COhmsLaw::*get)() const;
};

const Field kFields[] = {
    { IDC_EDIT_VOLTAGE,    L"Voltage",    &elec::COhmsLaw::setVoltage,    &elec::COhmsLaw::voltage },
    { IDC_EDIT_CURRENT,    L"Current",    &elec::COhmsLaw::setCurrent,    &elec::COhmsLaw::current },
    { IDC_EDIT_RESISTANCE, L"Resistance", &elec::COhmsLaw::setResistance, &elec::COhmsLaw::resistance },
    { IDC_EDIT_POWER,      L"Power",      &elec::COhmsLaw::setPower,      &elec::COhmsLaw::power },
};
constexpr int kFieldCount = ARRAYSIZE(kFields);

bool computed[kFieldCount];                     // true for fields filled in by solve()
bool updatingFields;                            // suppresses EN_CHANGE while we write results

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    SolverProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_OHMSLAW, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_OHMSLAW));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg) &&
            !(hSolver && IsDialogMessage(hSolver, &msg)))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
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
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_OHMSLAW));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_BTNFACE+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_OHMSLAW);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   // Fixed-size window: the form has a fixed layout, so no resizing or maximizing.
   const DWORD style = WS_OVERLAPPEDWINDOW & ~(WS_THICKFRAME | WS_MAXIMIZEBOX);

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, style,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   hSolver = CreateDialogW(hInstance, MAKEINTRESOURCEW(IDD_OHMSLAW_DIALOG), hWnd, SolverProc);
   if (!hSolver)
   {
      DestroyWindow(hWnd);
      return FALSE;
   }

   // Size the main window so its client area exactly fits the solver form.
   RECT rc;
   GetWindowRect(hSolver, &rc);
   OffsetRect(&rc, -rc.left, -rc.top);
   AdjustWindowRectEx(&rc, style, TRUE, 0);
   SetWindowPos(hWnd, nullptr, 0, 0, rc.right - rc.left, rc.bottom - rc.top,
      SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
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
    case WM_SETFOCUS:
        // Hand focus to the form; the dialog manager restores its last focused control.
        if (hSolver)
            SetFocus(hSolver);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

static void SetStatus(HWND hDlg, const std::wstring& text)
{
    SetDlgItemTextW(hDlg, IDC_STATUS, text.c_str());
}

// Writes a field without triggering the "user edited a field" logic.
static void SetFieldText(HWND hDlg, int index, const wchar_t* text)
{
    updatingFields = true;
    SetDlgItemTextW(hDlg, kFields[index].id, text);
    updatingFields = false;
}

static void ClearComputedFields(HWND hDlg)
{
    for (int i = 0; i < kFieldCount; ++i)
    {
        if (computed[i])
        {
            computed[i] = false;
            SetFieldText(hDlg, i, L"");
        }
    }
}

static void ClearAll(HWND hDlg)
{
    for (int i = 0; i < kFieldCount; ++i)
    {
        computed[i] = false;
        SetFieldText(hDlg, i, L"");
    }
    SetStatus(hDlg, L"");
    SetFocus(GetDlgItem(hDlg, kFields[0].id));
}

static void ReportError(HWND hDlg, int focusId, const std::wstring& message)
{
    SetStatus(hDlg, message);
    MessageBeep(MB_ICONWARNING);
    if (focusId)
    {
        HWND hEdit = GetDlgItem(hDlg, focusId);
        SetFocus(hEdit);
        SendMessageW(hEdit, EM_SETSEL, 0, -1);
    }
}

static std::wstring Widen(const char* s)
{
    return std::wstring(s, s + std::strlen(s));    // COhmsLaw messages are plain ASCII
}

// Reads the two known values from the form, solves with COhmsLaw, and fills in the unknowns.
static void Solve(HWND hDlg)
{
    elec::COhmsLaw ohms;
    bool known[kFieldCount] = {};

    for (int i = 0; i < kFieldCount; ++i)
    {
        wchar_t text[64];
        GetDlgItemTextW(hDlg, kFields[i].id, text, ARRAYSIZE(text));
        const wchar_t* p = text;
        while (iswspace(*p)) ++p;
        if (*p == L'\0')
            continue;                               // blank = unknown

        wchar_t* end;
        const double value = wcstod(p, &end);
        while (iswspace(*end)) ++end;
        if (end == p || *end != L'\0')
        {
            ReportError(hDlg, kFields[i].id, std::wstring(kFields[i].name) + L" is not a valid number.");
            return;
        }

        try
        {
            (ohms.*kFields[i].set)(value);
        }
        catch (const std::exception& e)
        {
            ReportError(hDlg, kFields[i].id, Widen(e.what()));
            return;
        }
        known[i] = true;
    }

    try
    {
        ohms.solve();
    }
    catch (const std::exception& e)
    {
        ReportError(hDlg, 0, Widen(e.what()));
        return;
    }

    std::wstring solvedFor;
    for (int i = 0; i < kFieldCount; ++i)
    {
        if (known[i])
            continue;

        wchar_t text[64];
        swprintf_s(text, L"%.6g", (ohms.*kFields[i].get)());
        SetFieldText(hDlg, i, text);
        computed[i] = true;

        solvedFor += solvedFor.empty() ? L"" : L" and ";
        solvedFor += kFields[i].name;
    }
    SetStatus(hDlg, L"Solved for " + solvedFor + L".");
}

// Message handler for the solver form.
INT_PTR CALLBACK SolverProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_SOLVE:
            Solve(hDlg);
            return (INT_PTR)TRUE;

        case IDC_CLEAR:
            ClearAll(hDlg);
            return (INT_PTR)TRUE;

        case IDCANCEL:                              // Esc: nothing to cancel in an embedded form
            return (INT_PTR)TRUE;

        default:
            // Editing any value after a solve starts a new problem: drop the old results.
            if (HIWORD(wParam) == EN_CHANGE && !updatingFields)
            {
                for (int i = 0; i < kFieldCount; ++i)
                {
                    if (kFields[i].id == LOWORD(wParam))
                        computed[i] = false;        // the user now owns this value
                }
                ClearComputedFields(hDlg);
                SetStatus(hDlg, L"");
                return (INT_PTR)TRUE;
            }
            break;
        }
        break;

    case WM_CTLCOLOREDIT:
        // Show solved values in the highlight color so they stand out from user input.
        for (int i = 0; i < kFieldCount; ++i)
        {
            if (computed[i] && (HWND)lParam == GetDlgItem(hDlg, kFields[i].id))
            {
                HDC hdc = (HDC)wParam;
                SetTextColor(hdc, GetSysColor(COLOR_HOTLIGHT));
                SetBkColor(hdc, GetSysColor(COLOR_WINDOW));
                return (INT_PTR)GetSysColorBrush(COLOR_WINDOW);
            }
        }
        break;
    }
    return (INT_PTR)FALSE;
}

// Message handler for about box.
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
