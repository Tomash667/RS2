#pragma once

class Window
{
public:
	typedef void* Handle;

	Window();
	void Init(Input* input);
	bool Update();
	void ShowError(cstring err);

	bool IsActive() { return active; }
	bool IsCursorLocked() { return cursor_locked; }
	Handle GetHandle() { return hwnd; }
	const Int2& GetSize() { return size; }
	const string& GetTitle() { return title; }

	void SetTitle(Cstring title);
	void SetCursorLock(bool locked);

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
	Handle hwnd;
	string title;
	Int2 size, real_size, cursor_move;
	int mouse_wheel;
	bool fullscreen, active, cursor_locked, cursor_visible;
};
