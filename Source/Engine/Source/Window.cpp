#include "EngineCore.h"
#include "Window.h"
#include "Input.h"
#include "Engine.h"
#include <Windows.h>
#include "InternalHelper.h"


#ifndef HID_USAGE_PAGE_GENERIC
#	define HID_USAGE_PAGE_GENERIC ((USHORT) 0x01)
#endif
#ifndef HID_USAGE_GENERIC_MOUSE
#	define HID_USAGE_GENERIC_MOUSE ((USHORT) 0x02)
#endif

const int WINDOWED_FLAGS = WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME;
const int FULLSCREEN_FLAGS = WS_POPUP;


Window::Window() : engine(nullptr), input(nullptr), hwnd(nullptr), title("Window"), fullscreen(false), size(1024, 768), active(false), cursor_locked(false),
cursor_visible(true), in_resize(false)
{
	client_size = size;
}

void Window::Init(Engine* engine, Input* input)
{
	assert(engine && input);
	this->engine = engine;
	this->input = input;

	RegisterWindowClass();
	AdjustWindowSize();
	CreateWindow();
	RegisterRawInput();
	CenterWindow();
	ShowWindow((HWND)hwnd, SW_SHOWNORMAL);
}

bool Window::Update()
{
	cursor_move = Int2::Zero;
	mouse_wheel = 0;

	MSG msg = { 0 };
	while(true)
	{
		if(PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if(msg.message == WM_QUIT)
				return false;
		}
		else
		{
			bool is_foreground = (GetForegroundWindow() == (HWND)hwnd);
			if(is_foreground != active)
				UpdateActivity(is_foreground);
			else if(active && cursor_locked)
				PlaceCursor();
			else
			{
				cursor_move = Int2::Zero;
				mouse_wheel = 0;
			}
			input->SetMouseDif(cursor_move);
			input->SetMouseWheel(mouse_wheel);
			return true;
		}
	}
}

void Window::RegisterWindowClass()
{
	HINSTANCE module = GetModuleHandle(nullptr);
	HICON icon = LoadIcon(module, "Icon");

	WNDCLASSEX wc = {
		sizeof(WNDCLASSEX), CS_HREDRAW | CS_VREDRAW,
		[](HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) -> LRESULT
		{
			if(msg == WM_NCCREATE)
			{
				CREATESTRUCT* cs = (CREATESTRUCT*)lParam;
				Window* window = (Window*)cs->lpCreateParams;
				window->hwnd = hwnd;
				SetWindowLongPtr((HWND)hwnd, GWLP_USERDATA, (IntPointer)cs->lpCreateParams);
			}
			else
			{
				Window* window = (Window*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
				if(window)
				{
					assert(hwnd == window->hwnd);
					return window->HandleMessage(msg, wParam, lParam);
				}
			}
			return DefWindowProc(hwnd, msg, wParam, lParam);
		},
		0, 0, module, icon, LoadCursor(nullptr, IDC_ARROW), (HBRUSH)GetStockObject(BLACK_BRUSH),
		nullptr, "MainWindow", nullptr
	};

	RegisterClassEx(&wc);
}

void Window::AdjustWindowSize()
{
	if(!fullscreen)
	{
		Rect rect = Rect::Create(Int2::Zero, size);
		AdjustWindowRect((RECT*)&rect, WINDOWED_FLAGS, false);
		real_size = rect.Size();
	}
	else
		real_size = size;
}

void Window::CreateWindow()
{
	CreateWindowEx(0, "MainWindow", title.c_str(), fullscreen ? FULLSCREEN_FLAGS : WINDOWED_FLAGS, 0, 0, real_size.x, real_size.y,
		nullptr, nullptr, GetModuleHandle(nullptr), this);
}

void Window::CenterWindow()
{
	if(!fullscreen)
		MoveWindow((HWND)hwnd,
			(GetSystemMetrics(SM_CXSCREEN) - real_size.x) / 2,
			(GetSystemMetrics(SM_CYSCREEN) - real_size.y) / 2,
			real_size.x, real_size.y, false);
}

void Window::RegisterRawInput()
{
	RAWINPUTDEVICE rid;
	rid.usUsagePage = HID_USAGE_PAGE_GENERIC;
	rid.usUsage = HID_USAGE_GENERIC_MOUSE;
	rid.dwFlags = RIDEV_INPUTSINK;
	rid.hwndTarget = (HWND)hwnd;
	RegisterRawInputDevices(&rid, 1, sizeof(rid));
}

IntPointer Window::HandleMessage(uint msg, IntPointer wParam, UIntPointer lParam)
{
	switch(msg)
	{
	case WM_CLOSE:
		PostQuitMessage(0);
		return 0;
	case WM_ACTIVATE:
		UpdateActivity(wParam != WA_INACTIVE);
		return 0;
	case WM_ACTIVATEAPP:
		UpdateActivity(wParam == TRUE);
		return 0;
	case WM_INPUT:
		{
			UINT dwSize = 40;
			static BYTE lpb[40];
			GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER));

			RAWINPUT* raw = (RAWINPUT*)lpb;
			if(raw->header.dwType == RIM_TYPEMOUSE)
			{
				cursor_move.x += raw->data.mouse.lLastX;
				cursor_move.y += raw->data.mouse.lLastY;
			}
		}
		break;
	case WM_SYSKEYDOWN:
	case WM_KEYDOWN:
		input->Process((Key)wParam, true);
		return 0;
	case WM_SYSKEYUP:
	case WM_KEYUP:
		input->Process((Key)wParam, false);
		return 0;
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_XBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
	case WM_MBUTTONUP:
	case WM_XBUTTONUP:
		return ProcessMouseButton(msg, wParam);
	case WM_MOUSEWHEEL:
		mouse_wheel += GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
		return 0;
	case WM_MENUCHAR:
		return MAKELRESULT(0, MNC_CLOSE);
	case WM_SIZE:
		if(wParam == SIZE_MAXIMIZED)
			SetFullscreen(true);
		else if(wParam != SIZE_MINIMIZED)
		{
			if(!in_resize)
			{
				RECT rect = {};
				GetWindowRect((HWND)hwnd, &rect);
				real_size = Int2(rect.right - rect.left, rect.bottom - rect.top);

				GetClientRect((HWND)hwnd, &rect);
				client_size = Int2(rect.right - rect.left, rect.bottom - rect.top);
			}

			engine->OnChangeResolution(client_size);
		}
		return 0;
	}

	return DefWindowProc((HWND)hwnd, msg, wParam, lParam);
}

int Window::ProcessMouseButton(uint msg, IntPointer wParam)
{
	int result;
	Key key;
	bool down;

	// translate msg to key
	switch(msg)
	{
	default:
		assert(0);
	case WM_LBUTTONDOWN:
		result = 0;
		key = Key::LeftButton;
		down = true;
		break;
	case WM_LBUTTONUP:
		result = 0;
		key = Key::LeftButton;
		down = false;
		break;
	case WM_RBUTTONDOWN:
		result = 0;
		key = Key::RightButton;
		down = true;
		break;
	case WM_RBUTTONUP:
		result = 0;
		key = Key::RightButton;
		down = false;
		break;
	case WM_MBUTTONDOWN:
		result = 0;
		key = Key::MiddleButton;
		down = true;
		break;
	case WM_MBUTTONUP:
		result = 0;
		key = Key::MiddleButton;
		down = false;
		break;
	case WM_XBUTTONDOWN:
		result = TRUE;
		key = (GET_XBUTTON_WPARAM(wParam) == XBUTTON1 ? Key::X1Button : Key::X2Button);
		down = true;
		break;
	case WM_XBUTTONUP:
		result = TRUE;
		key = (GET_XBUTTON_WPARAM(wParam) == XBUTTON1 ? Key::X1Button : Key::X2Button);
		down = false;
		break;
	}

	if(!cursor_locked)
	{
		if(down)
			SetCursorLock(true);
	}
	else
		input->Process(key, down);

	return result;
}

void Window::ShowError(cstring msg)
{
	assert(msg);
	ShowWindow((HWND)hwnd, SW_HIDE);
	ShowCursor(true);
	MessageBox(nullptr, msg, nullptr, MB_OK | MB_ICONERROR | MB_APPLMODAL);
}

void Window::SetTitle(Cstring title)
{
	this->title = title;
	if(hwnd)
		SetWindowText((HWND)hwnd, title);
}

void Window::UpdateActivity(bool new_active)
{
	if(new_active && GetForegroundWindow() != (HWND)hwnd)
		new_active = false;
	if(new_active == active)
		return;
	active = new_active;
	if(active && cursor_locked)
	{
		input->ReleaseKeys();
		ShowCursor(false);
		PlaceCursor();
	}
	else
		ShowCursor(true);
}

void Window::SetCursorLock(bool locked)
{
	if(locked == cursor_locked)
		return;
	cursor_locked = locked;
	if(active && cursor_locked)
	{
		ShowCursor(false);
		PlaceCursor();
	}
	else
		ShowCursor(true);
}

void Window::PlaceCursor()
{
	POINT dest_pt = { client_size.x / 2, client_size.y / 2 };
	ClientToScreen((HWND)hwnd, &dest_pt);
	SetCursorPos(dest_pt.x, dest_pt.y);
}

void Window::ShowCursor(bool show)
{
	if(show == cursor_visible)
		return;
	cursor_visible = show;
	::ShowCursor(show);
}

void Window::SetFullscreen(bool fullscreen)
{
	if(this->fullscreen == fullscreen)
		return;
	this->fullscreen = fullscreen;
	if(!hwnd)
		return;
	in_resize = true;
	if(fullscreen)
	{
		client_size = Int2(GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
		real_size = client_size;
		SetWindowLong((HWND)hwnd, GWL_STYLE, FULLSCREEN_FLAGS);
		SetWindowPos((HWND)hwnd, HWND_NOTOPMOST,
			0, 0,
			client_size.x, client_size.y,
			SWP_FRAMECHANGED | SWP_NOACTIVATE);
		ShowWindow((HWND)hwnd, SW_MAXIMIZE);
	}
	else
	{
		client_size = size;
		AdjustWindowSize();
		SetWindowLong((HWND)hwnd, GWL_STYLE, WINDOWED_FLAGS);
		SetWindowPos((HWND)hwnd, HWND_NOTOPMOST,
			(GetSystemMetrics(SM_CXSCREEN) - real_size.x) / 2,
			(GetSystemMetrics(SM_CYSCREEN) - real_size.y) / 2,
			real_size.x, real_size.y,
			SWP_FRAMECHANGED | SWP_NOACTIVATE);
		ShowWindow((HWND)hwnd, SW_NORMAL);
	}
	in_resize = false;
}

void Window::SetSize(const Int2& size)
{
	if(this->size == size)
		return;
	this->size = size;
	if(!hwnd)
		return;
	if(!fullscreen)
	{
		client_size = size;
		AdjustWindowSize();
		SetWindowPos((HWND)hwnd, HWND_NOTOPMOST,
			(GetSystemMetrics(SM_CXSCREEN) - real_size.x) / 2,
				(GetSystemMetrics(SM_CYSCREEN) - real_size.y) / 2,
			real_size.x, real_size.y,
			SWP_NOACTIVATE);
	}
}

void Window::StartFullscreen()
{
	if(!fullscreen)
		return;
	fullscreen = false;
	SetFullscreen(true);
}
