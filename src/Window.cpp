/*!
  @file Window.cpp @author Joel Barrett @date 12/04/09 @brief Windows A window base class.
*/

#include <stdexcept>
#include "Window.h"

namespace OceanWaves
{
  void Window::SetTitle(LPCTSTR lpszTitle)
  {
    ZeroMemory(title_, sizeof(title_));
    strcpy(title_, lpszTitle);
  }

  void Window::SetClassID(LPCTSTR lpszClassID)
  {
    ZeroMemory(classID_, sizeof(classID_));
    strcpy(classID_, lpszClassID);
  }

  bool Window::Initialise(LPCTSTR lpszClassID, UINT style, HICON hIcon,
    HCURSOR hCursor, HBRUSH hbrBackground, LPCTSTR lpszMenuID)
  {
    WNDCLASSEX wcx = { 0 };

    wcx.lpszClassName = lpszClassID;
    wcx.cbSize = sizeof(WNDCLASSEX);
    wcx.style = style;
    wcx.lpfnWndProc = st_MsgRouter;
    wcx.hInstance = hInst_;
    wcx.hIcon = hIcon;
    wcx.hIconSm = hIcon;
    wcx.hCursor = hCursor;
    wcx.hbrBackground = hbrBackground;
    wcx.lpszMenuName = lpszMenuID;
    wcx.cbClsExtra = 0;
    wcx.cbWndExtra = 0;

    if (!RegisterClassEx(&wcx)) {
      throw std::runtime_error("The RegisterClassEx() function failed");
      return false;
    }
    strcpy(classID_, lpszClassID);
    return true;
  }

  bool Window::Display(LPCTSTR lpszTitle, DWORD dwStyles,
    UINT xPos, UINT yPos, UINT width, UINT height)
  {
    hWnd_ = CreateWindowEx(NULL, classID_, lpszTitle, dwStyles, xPos, yPos,
      width, height, NULL, NULL, hInst_, (void*)this);

    if (!hWnd_) {
      throw std::runtime_error("The CreateWindowEx() function failed");
      return false;
    }
    return true;
  }

  LRESULT CALLBACK Window::st_MsgRouter(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
  {
    if (uMsg == WM_NCCREATE)
    {
      // Get pointer to the window from lpCreateParams
      SetWindowLongPtr(hWnd, GWL_USERDATA, (long)((LPCREATESTRUCT(lParam))->lpCreateParams));
    }
    // Get pointer to the window
    Window* pWnd = (Window*)GetWindowLongPtr(hWnd, GWL_USERDATA);

    // If a window exists, use message router of window; otherwise, use DefWindowProc
    return pWnd ? pWnd->MsgRouter(hWnd, uMsg, wParam, lParam)
      : DefWindowProc(hWnd, uMsg, wParam, lParam);
  }
}
