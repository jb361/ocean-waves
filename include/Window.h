/*!
  @file Window.h @author Joel Barrett @date 12/04/09 @brief A window base class.
*/

#pragma once

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <windowsx.h>

namespace OceanWaves
{
  /*
    A window base class for Windows applications to derive from.
  */
  class Window
  {
  protected:
    Window() : windowVisible_(true),
      hInst_(GetModuleHandle(NULL)),
      hWnd_(NULL) {}

    void SetTitle(LPCTSTR lpszTitle);
    void SetClassID(LPCTSTR lpszClassID);

    const HWND& GetHandle() const { return hWnd_; }

    //! Set up and register the WNDCLASSEX structure
    bool Initialise(LPCTSTR lpszClassID = "WinClass",
      UINT style = CS_OWNDC | CS_VREDRAW | CS_HREDRAW,
      HICON hIcon = LoadIcon(NULL, IDI_APPLICATION),
      HCURSOR hCursor = LoadCursor(NULL, IDC_ARROW),
      HBRUSH hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH),
      LPCTSTR lpszMenuID = NULL);

    //! Set up some parameters and create the window for display
    bool Display(LPCTSTR lpszTitle = "Win32 Window",
      DWORD dwStyles = WS_OVERLAPPEDWINDOW | WS_VISIBLE,
      UINT xPos = 200, UINT yPos = 200,
      UINT width = 800, UINT height = 600);

    //! Virtual message router that the derived class supplies
    virtual LRESULT CALLBACK MsgRouter(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) = 0;

    //! Static message router that the WNDCLASSEX structure needs access to
    static LRESULT CALLBACK st_MsgRouter(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

  protected:
    CHAR title_[256];
    CHAR classID_[256];

    UINT width_, height_;
    bool windowVisible_;
    HINSTANCE hInst_;
    HWND hWnd_;
  };
}
