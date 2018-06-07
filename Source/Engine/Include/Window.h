#pragma once

class Window
{
public:
	typedef void* Handle;

	Window();
	void Init(Input* input, WindowHandler* handler);
	bool Update();
	void ShowError(cstring err);

	void SetCursorLock(bool locked);
	void SetFullscreen(bool fullscreen);
	void SetSize(const Int2& size);
	void SetTitle(Cstring title);

	bool IsActive() { return active; }
	bool IsCursorLocked() { return cursor_locked; }
	bool IsFullscreen() { return fullscreen; }
	Handle GetHandle() { return hwnd; }
	const Int2& GetSize() { return size; }
	const string& GetTitle() { return title; }

private:
	void RegisterWindowClass();
	void AdjustWindowSize();
	void CreateWindow();
	void CenterWindow();
	void RegisterRawInput();
	IntPointer HandleMessage(uint msg, IntPointer wParam, UIntPointer lParam);
	int ProcessMouseButton(uint msg, IntPointer wParam);
	void UpdateActivity(bool new_active);
	void PlaceCursor();
	void ShowCursor(bool show);

	Input* input;
	WindowHandler* handler;
	Handle hwnd;
	string title;
	Int2 size, real_size, prev_size, cursor_move;
	int mouse_wheel;
	bool fullscreen, active, cursor_locked, cursor_visible;
};
