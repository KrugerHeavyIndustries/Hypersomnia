#include <algorithm>

#include "augs/log.h"

#include "augs/templates/string_templates.h"
#include "augs/templates/corresponding_field.h"
#include "augs/templates/algorithm_templates.h"

#include "augs/window_framework/window.h"
#include "augs/window_framework/platform_utils.h"

#include "augs/window_framework/translate_windows_enums.h"

namespace augs {
	LRESULT CALLBACK wndproc(HWND hwnd, UINT umsg, WPARAM wParam, LPARAM lParam) {
		if (umsg == WM_GETMINMAXINFO || umsg == WM_INPUT) {
			return DefWindowProc(hwnd, umsg, wParam, lParam);
		}

		if (umsg == WM_CREATE) {
			AddClipboardFormatListener(hwnd);
		}

		if (umsg == WM_DESTROY) {
			RemoveClipboardFormatListener(hwnd);
		}
		else if (umsg == WM_SYSCOMMAND || umsg == WM_ACTIVATE || umsg == WM_INPUT) {
			(PostMessage(hwnd, umsg, wParam, lParam));
			return 0;
		}
		else if (umsg == WM_SIZE) {
			LRESULT res = DefWindowProc(hwnd, umsg, wParam, lParam);
			return res;
		}

		return DefWindowProc(hwnd, umsg, wParam, lParam);
	}

	event::change window::do_raw_motion(const basic_vec2<short> motion) {
		event::change change;
		
		change.data.mouse.rel = motion;

		if (!mouse_pos_frozen) {
			last_mouse_pos += vec2i(change.data.mouse.rel);
		}

		const auto screen_size = current_settings.get_screen_size() - vec2i(1, 1);
		last_mouse_pos.clamp_from_zero_to(screen_size);
		change.data.mouse.pos = basic_vec2<short>(last_mouse_pos);
		change.msg = event::message::mousemotion;

		return change;
	}
	
	std::optional<event::change> window::sync_mouse_on_click_activate(const event::change& new_change) {
		if (const bool should_sync_mouse = new_change.msg == event::message::click_activate
			&& get_current_settings().raw_mouse_input
		) {
			if (const auto screen_space = get_cursor_pos()) {
				const auto window_pos = get_window_rect().get_position();
				const auto rel_v = basic_vec2<short>((*screen_space - window_pos) - last_mouse_pos);
				return do_raw_motion(rel_v);
			}
		}

		return std::nullopt;
	}

	std::optional<event::change> window::handle_event(const UINT m, const WPARAM wParam, const LPARAM lParam) {
		using namespace event::keys;

		event::change change;
		change.msg = translate_enum(m);

		auto default_proc = [&]() {
			DefWindowProc(hwnd, m, wParam, lParam);
		};

		switch (m) {
		case WM_CHAR:
			change.data.character.utf16 = wchar_t(wParam);
			if (change.data.character.utf16 > 255) {
				break;
			}
			//change.utf32 = unsigned(wParam);
			if (const bool repeated = ((lParam & (1 << 30)) != 0)) {
				return std::nullopt;
			}

			break;
		case WM_UNICHAR:
			//change.utf32 = unsigned(wParam);
			//change.repeated = ((lParam & (1 << 30)) != 0);
			break;

		case SC_CLOSE:
			break;

		case WM_KEYUP:
			change.data.key.key = translate_key_with_lparam(lParam, wParam);
			break;
		case WM_KEYDOWN:
			change.data.key.key = translate_key_with_lparam(lParam, wParam);

			if (const bool repeated = ((lParam & (1 << 30)) != 0)) {
				return std::nullopt;
			}

			break;

		case WM_SYSKEYUP:
			change.data.key.key = translate_key_with_lparam(lParam, wParam);
			break;
		case WM_SYSKEYDOWN:
			change.data.key.key = translate_key_with_lparam(lParam, wParam);

			if (const bool repeated = ((lParam & (1 << 30)) != 0)) {
				return std::nullopt;
			}

			break;

		case WM_MOUSEWHEEL:
			change.data.scroll.amount = GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
			break;
		case WM_LBUTTONDBLCLK:
		case WM_LBUTTONDOWN:
			change.data.key.key = key::LMOUSE;
			SetCapture(hwnd);

			if (m == WM_LBUTTONDOWN) {
				if (double_click_occured && triple_click_timer.extract<std::chrono::milliseconds>() < triple_click_delay) {
					change.msg = event::message::ltripleclick;
					double_click_occured = false;
				}
			}
			else {
				triple_click_timer.extract<std::chrono::microseconds>();
				double_click_occured = true;
			}

			break;
		case WM_RBUTTONDBLCLK:
		case WM_RBUTTONDOWN:
			change.data.key.key = key::RMOUSE;
			break;
		case WM_MBUTTONDBLCLK:
		case WM_MBUTTONDOWN:
			change.data.key.key = key::MMOUSE;
			break;
		case WM_XBUTTONDBLCLK:
		case WM_XBUTTONDOWN:
			change.data.key.key = key::MOUSE4;
			change.msg = event::message::keydown;
			break;
		case WM_XBUTTONUP:
			change.data.key.key = key::MOUSE4;
			change.msg = event::message::keyup;
			break;
		case WM_LBUTTONUP:
			change.data.key.key = key::LMOUSE;
			if (GetCapture() == hwnd) ReleaseCapture(); break;
		case WM_RBUTTONUP:
			change.data.key.key = key::RMOUSE;
			break;
		case WM_MBUTTONUP:
			change.data.key.key = key::MMOUSE;
			break;
		case WM_MOUSEHOVER:
			cursor_in_client_area = true;
			default_proc();
			break;
		case WM_MOUSELEAVE:
			cursor_in_client_area = false;
			default_proc();
			break;
		case WM_MOUSEMOVE:
			if (!current_settings.raw_mouse_input && !mouse_pos_frozen) {
				basic_vec2<short> new_pos;

				{
					const auto p = MAKEPOINTS(lParam);
					new_pos = { p.x, p.y };
				}

				change.data.mouse.rel = new_pos - basic_vec2<short>(last_mouse_pos);
				
				if (change.data.mouse.rel.non_zero()) {
					double_click_occured = false;
				}

				last_mouse_pos = new_pos;

				change.data.mouse.pos = basic_vec2<short>(last_mouse_pos);
				change.msg = event::message::mousemotion;
			}
			else {
				return std::nullopt;
			}

			break;

		case WM_INPUT:
			if (active && (current_settings.raw_mouse_input || mouse_pos_frozen)) {
				thread_local BYTE lpb[sizeof(RAWINPUT)];
				thread_local UINT dwSize = sizeof(RAWINPUT);

				GetRawInputData(
					reinterpret_cast<HRAWINPUT>(lParam), 
					RID_INPUT,
					lpb, 
					&dwSize, 
					sizeof(RAWINPUTHEADER)
				);

				const auto* const raw = reinterpret_cast<RAWINPUT*>(lpb);

				if (raw->header.dwType == RIM_TYPEMOUSE) {
					change = do_raw_motion({
						static_cast<short>(raw->data.mouse.lLastX),
						static_cast<short>(raw->data.mouse.lLastY)
					});
				}
			}
			else {
				return std::nullopt;
			}

			break;

		case WM_ACTIVATE:
			{
				const auto type = LOWORD(wParam);

				active = type != WA_INACTIVE;

				switch (type) {
				case WA_INACTIVE: change.msg = event::message::deactivate; break;
				case WA_ACTIVE: change.msg = event::message::activate; break;
				case WA_CLICKACTIVE: change.msg = event::message::click_activate; break;
				default: change.msg = event::message::unknown; break;
				}

				if (!active && current_settings.raw_mouse_input) {
					augs::set_cursor_pos(current_settings.position + last_mouse_pos);
				}
			}

			break;

		case WM_GETMINMAXINFO:
			{
				auto* const mi = reinterpret_cast<MINMAXINFO*>(lParam);
				mi->ptMinTrackSize.x = min_window_size.x;
				mi->ptMinTrackSize.y = min_window_size.y;
				mi->ptMaxTrackSize.x = max_window_size.x;
				mi->ptMaxTrackSize.y = max_window_size.y;
			}
			break;

		case WM_SYSCOMMAND:
			default_proc();
			change.msg = translate_enum(static_cast<UINT>(wParam));
			break;

		default: default_proc(); break;
		}

		return change;
	}

	void window::set_window_name(const std::string& name) {
		SetWindowText(hwnd, to_wstring(name).c_str());
	}

	window::window(
		const window_settings& settings
	) {
		// TODO: throw an exception instead of ensuring
		static bool register_once = [](){
			WNDCLASSEX wcl = { 0 };
			wcl.cbSize = sizeof(wcl);
			wcl.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
			wcl.lpfnWndProc = wndproc;
			wcl.cbClsExtra = 0;
			wcl.cbWndExtra = 0;
			wcl.hInstance = GetModuleHandle(NULL);
#if ADD_APPLICATION_ICON
			wcl.hIcon = 0;
			wcl.hIconSm = 0;
#else
			wcl.hIcon = LoadIcon(0, IDI_APPLICATION);
			wcl.hIconSm = 0;
#endif
			wcl.hCursor = LoadCursor(0, IDC_ARROW);
			wcl.hbrBackground = 0;
			wcl.lpszMenuName = 0;
			wcl.lpszClassName = L"augwin";

			const auto register_success = RegisterClassEx(&wcl) != 0;

			ensure(register_success && "class registering");
			return register_success;
		}();

		triple_click_delay = GetDoubleClickTime();

		ensure((hwnd = CreateWindowEx(0, L"augwin", L"invalid_name", 0, 0, 0, 0, 0, 0, 0, GetModuleHandle(NULL), this)));

		PIXELFORMATDESCRIPTOR p;
		ZeroMemory(&p, sizeof(p));

		p.nSize = sizeof(p);
		p.nVersion = 1;
		p.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
		p.iPixelType = PFD_TYPE_RGBA;
		p.cColorBits = settings.bpp;
		p.cAlphaBits = 8;
		p.cDepthBits = 0;
		p.iLayerType = PFD_MAIN_PLANE;
		ensure(hdc = GetDC(hwnd));

		const auto pf = ChoosePixelFormat(hdc, &p);
		
		ensure(pf);
		ensure(SetPixelFormat(hdc, pf, &p));

#if BUILD_OPENGL
		ensure(hglrc = wglCreateContext(hdc));
#endif

		ensure(set_as_current());
		show();

		SetLastError(0);
		ensure(!(SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)this) == 0 && GetLastError() != 0));

#ifndef HID_USAGE_PAGE_GENERIC
#define HID_USAGE_PAGE_GENERIC         ((USHORT) 0x01)
#endif
#ifndef HID_USAGE_GENERIC_MOUSE
#define HID_USAGE_GENERIC_MOUSE        ((USHORT) 0x02)
#endif

		RAWINPUTDEVICE Rid[1];
		Rid[0].usUsagePage = HID_USAGE_PAGE_GENERIC;
		Rid[0].usUsage = HID_USAGE_GENERIC_MOUSE;
		Rid[0].dwFlags = RIDEV_INPUTSINK;
		Rid[0].hwndTarget = hwnd;
		RegisterRawInputDevices(Rid, 1, sizeof(Rid[0]));

		apply(settings, true);
		last_mouse_pos = settings.get_screen_size() / 2;

		const auto app_icon_path = settings.app_icon_path.wstring();
		
		if (const bool icon_was_specified = !app_icon_path.empty()) {
			const auto app_icon = (HICON)LoadImage(NULL,
				app_icon_path.c_str(),
				IMAGE_ICON,       
				0,                
				0,                
				LR_LOADFROMFILE |  
				LR_DEFAULTSIZE |   
				LR_SHARED         
			);

			SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)app_icon);
			SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)app_icon);
		}
	}

	void window::destroy() {
		if (hwnd) {
			unset_if_current();

#if BUILD_OPENGL
			wglDeleteContext(hglrc);
#endif
			ReleaseDC(hwnd, hdc);
			DestroyWindow(hwnd);

			hwnd = nullptr;
		}
	}

	void window::set_window_border_enabled(const bool enabled) {
		if (enabled) {
			style = WS_OVERLAPPEDWINDOW;
			exstyle = WS_EX_WINDOWEDGE;
		}
		else {
			style = WS_POPUP;
			exstyle = WS_EX_APPWINDOW;
		}

		SetWindowLongPtr(hwnd, GWL_EXSTYLE, exstyle);
		SetWindowLongPtr(hwnd, GWL_STYLE, style);

		SetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOZORDER | SWP_NOOWNERZORDER);
		set_window_rect(get_window_rect());
		show();
	}

	bool window::swap_buffers() {
		return SwapBuffers(hdc) != FALSE;
	}

	void window::show() {
		ShowWindow(hwnd, SW_SHOW);
	}
	
	void window::set_mouse_pos_frozen(const bool flag) {
		if (mouse_pos_frozen && !flag) {
			augs::set_cursor_pos(current_settings.position + last_mouse_pos);
		}

		mouse_pos_frozen = flag;
	}
	
	bool window::is_mouse_pos_frozen() const {
		return mouse_pos_frozen;
	}

	bool window::set_as_current_impl() {
#if BUILD_OPENGL
		return wglMakeCurrent(hdc, hglrc);
#else
		return true;
#endif
	}

	void window::set_current_to_none_impl() {
#if BUILD_OPENGL
		wglMakeCurrent(NULL, NULL);
#endif
	}

	void window::collect_entropy(local_entropy& output) {
		ensure(is_current());

		{
			MSG wmsg;

			while (PeekMessageW(&wmsg, hwnd, 0, 0, PM_REMOVE)) {
				const auto new_change = handle_event(
					wmsg.message, 
					wmsg.wParam, 
					wmsg.lParam
				);

				TranslateMessage(&wmsg);

				if (new_change.has_value()) {
					if (const auto mouse_change = sync_mouse_on_click_activate(*new_change)) {
						output.push_back(*mouse_change);
					}

					output.push_back(*new_change);
				}
			}
		}
	}

	void window::set_window_rect(const xywhi r) {
		static RECT wr = { 0 };
		ensure(SetRect(&wr, r.x, r.y, r.r(), r.b()));
		ensure(AdjustWindowRectEx(&wr, style, FALSE, exstyle));
		ensure(MoveWindow(hwnd, wr.left, wr.top, wr.right - wr.left, wr.bottom - wr.top, TRUE));
	}

	xywhi window::get_window_rect() const {
		static RECT r;
		GetClientRect(hwnd, &r);
		ClientToScreen(hwnd, (POINT*)&r);
		ClientToScreen(hwnd, (POINT*)&r + 1);
		return ltrbi(r.left, r.top, r.right, r.bottom);
	}

	bool window::is_active() const {
		return active;
	}
	
	bool window::is_cursor_in_client_area() const {
		return cursor_in_client_area; 
	}

	static auto get_filter(const std::vector<window::file_dialog_filter>& filters) {
		std::wstring filter;

		auto to_reserve = std::size_t{ 0 };

		for (const auto& f : filters) {
			to_reserve += f.description.length();
			to_reserve += f.extension.length() + 1;
			to_reserve += 2;
		}

		filter.reserve(to_reserve);

		for (const auto& f : filters) {
			const auto description = to_wstring(f.description);
			const auto extension = to_wstring(f.extension);

			filter += to_wstring(f.description);
			filter.push_back(L'\0');
			filter += L'*';
			filter += to_wstring(f.extension);
			filter.push_back(L'\0');
		}

		return filter;
	}

	std::optional<std::string> window::open_file_dialog(
		const std::vector<file_dialog_filter>& filters,
		std::string custom_title
	) const {
		const auto filter = get_filter(filters);
		const auto title = to_wstring(custom_title);
		
		OPENFILENAME ofn;       // common dialog box structure
		std::array<wchar_t, 400> szFile;
		fill_range(szFile, 0);

		ZeroMemory(&ofn, sizeof(ofn));
		ofn.lStructSize = sizeof(ofn);
		ofn.lpstrFile = szFile.data();
		ofn.hwndOwner = hwnd;
		ofn.lpstrFile[0] = '\0';
		ofn.nMaxFile = static_cast<DWORD>(szFile.size());
		ofn.lpstrFilter = filter.data();
		ofn.lpstrTitle = title.data();
		ofn.nFilterIndex = 1;
		ofn.lpstrFileTitle = NULL;
		ofn.nMaxFileTitle = 0;
		ofn.lpstrInitialDir = NULL;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER | OFN_NOCHANGEDIR;

		// Display the Open dialog box. 

		if (GetOpenFileName(&ofn) == TRUE) {
			return str_ops(to_string(ofn.lpstrFile)).replace_all("\\", "/");
		}
		else {
			return std::nullopt;
		}
	}

	std::optional<std::string> window::save_file_dialog(
		const std::vector<file_dialog_filter>& filters,
		std::string custom_title
	) const {
		const auto filter = get_filter(filters);
		const auto title = to_wstring(custom_title);

		OPENFILENAME ofn;       // common dialog box structure
		std::array<wchar_t, 400> szFile;
		fill_range(szFile, 0);

		ZeroMemory(&ofn, sizeof(ofn));
		ofn.lStructSize = sizeof(ofn);
		ofn.lpstrFile = szFile.data();
		ofn.hwndOwner = hwnd;
		ofn.lpstrFile[0] = '\0';
		ofn.nMaxFile = static_cast<DWORD>(szFile.size());
		ofn.lpstrFilter = filter.data();
		ofn.lpstrTitle = title.data();
		ofn.nFilterIndex = 1;
		ofn.lpstrFileTitle = NULL;
		ofn.nMaxFileTitle = 0;
		ofn.lpstrInitialDir = NULL;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;

		// Display the Open dialog box. 

		if (GetSaveFileName(&ofn) == TRUE) {
			auto result = augs::path_type(ofn.lpstrFile);
			const auto supposed_extension = filters[ofn.nFilterIndex - 1].extension;

			if (supposed_extension != ".*") {
				if (result.extension() != supposed_extension) {
					result += filters[ofn.nFilterIndex - 1].extension;
				}
			}

			return str_ops(result.string()).replace_all("\\", "/");
		}
		else {
			return std::nullopt;
		}
	}
}
