/*!
  @file Main.cpp @author Joel Barrett @date 11/03/12 @brief Main entry point of the application.
*/

#include "Scene.h"

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR cmdLine, int cmdShow)
{
  // Enable run-time memory checking in debug builds
#if defined(DEBUG) || defined(_DEBUG)
  _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

  OceanWaves::Scene oceanWaves;
  try {
    oceanWaves.Init();
    oceanWaves.Execute();
  }
  catch (std::exception& e) {
    MessageBox(NULL, e.what(), "An exception occurred!",
      MB_OK | MB_ICONERROR | MB_TASKMODAL);
  }

  return oceanWaves.GetExitCode();
}
