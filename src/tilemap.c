//! Includes
#include "orx.h"


//! Variables


//! Code

orxSTATUS orxFASTCALL Init()
{
  orxSTATUS eResult = orxSTATUS_SUCCESS;

  // Creates viewport
  orxViewport_CreateFromConfig("Viewport");

  // Creates scene
  orxObject_CreateFromConfig("Scene");

  // Done!
  return eResult;
}

orxSTATUS orxFASTCALL Run()
{
  orxSTATUS eResult = orxSTATUS_SUCCESS;

  // Screenshot?
  if(orxInput_IsActive("Screenshot") && orxInput_HasNewStatus("Screenshot"))
  {
    // Captures it
    orxScreenshot_Capture();
  }

  // Quitting?
  if(orxInput_IsActive("Quit"))
  {
    // Updates result
    eResult = orxSTATUS_FAILURE;
  }

  // Done!
  return eResult;
}

void orxFASTCALL Exit()
{
  // We could delete everything we created here but orx will do it for us anyway =)
}

orxSTATUS orxFASTCALL Bootstrap()
{
  orxSTATUS eResult = orxSTATUS_SUCCESS;

  // Adds config resource storages
  orxResource_AddStorage(orxCONFIG_KZ_RESOURCE_GROUP, "../../data/config", orxFALSE);
  orxResource_AddStorage(orxCONFIG_KZ_RESOURCE_GROUP, "../../../data/config", orxFALSE);

  // Done!
  return eResult;
}

int main(int argc, char **argv)
{
  // Sets config bootstrap
  orxConfig_SetBootstrap(Bootstrap);

  // Executes orx
  orx_Execute(argc, argv, Init, Run, Exit);

  // Done!
  return EXIT_SUCCESS;
}


#ifdef __orxWINDOWS__

#include "windows.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
  // Executes orx
  orx_WinExecute(Init, Run, Exit);

  // Done!
  return EXIT_SUCCESS;
}

#endif // __orxWINDOWS__
