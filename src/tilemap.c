//! Includes
#include "orx.h"


//! Defines

#if defined(__orxX86_64__) || defined(__orxPPC64__) || defined(__orxARM64__)
  #define CAST_HELPER (orxU64)
#else /* __orxX86_64__ || __orxPPC64__ || __orxARM64__ */
  #define CAST_HELPER (orxU32)
#endif /* __orxX86_64__ || __orxPPC64__ || __orxARM64__ */


//! Structures

typedef struct TileSet
{
  orxVECTOR     vSize;
  orxVECTOR     vTileSize;
  orxTEXTURE   *pstTexture;
  orxHASHTABLE *pstIndexMap;
} TileSet;


//! Variables

static orxBANK   *spstTileSetBank;
static orxCAMERA *spstCamera;
static orxVECTOR  svMousePos, svScrollSpeed, svCameraSize;


//! Code

TileSet *LoadTileSet(const orxSTRING _zSetName)
{
  TileSet *pstSet = orxNULL;

  // Allocates tile set
  pstSet = (TileSet *)orxBank_Allocate(spstTileSetBank);
  orxASSERT(pstSet);

  // Pushes its config section
  orxConfig_PushSection(_zSetName);

  // Loads its texture
  pstSet->pstTexture = orxTexture_CreateFromFile(orxConfig_GetString("Texture"), orxFALSE);

  // Success?
  if(pstSet->pstTexture != orxNULL)
  {
    orxVECTOR       vSetSize = {};
    const orxSTRING zSetName;
    orxU32          i, u32Counter;

    // Gets set size (pixels)
    orxTexture_GetSize(pstSet->pstTexture, &vSetSize.fX, &vSetSize.fY);

    // Gets tile size
    orxConfig_GetVector("TextureSize", &pstSet->vTileSize);
    pstSet->vTileSize.fZ = orxFLOAT_1;

    // Gets set size (tiles)
    orxVector_Div(&pstSet->vSize, &vSetSize, &pstSet->vTileSize);

    // Creates index map
    pstSet->pstIndexMap = orxHashTable_Create(orxF2U(pstSet->vSize.fX * pstSet->vSize.fY), orxHASHTABLE_KU32_FLAG_NONE, orxMEMORY_TYPE_MAIN);
    orxASSERT(pstSet->pstIndexMap);

    // Gets current section's name (for faster comparisons with orxConfig_GetParent() results)
    zSetName = orxConfig_GetCurrentSection();

    // For all config sections
    for(i = 0, u32Counter = orxConfig_GetSectionCount();
        i < u32Counter;
        i++)
    {
      const orxSTRING zSectionName;

      // Gets its name
      zSectionName = orxConfig_GetSection(i);

      // Is a tile that belong to this set?
      if(orxConfig_GetParent(zSectionName) == zSetName)
      {
        orxVECTOR vTileOrigin = {};
        orxU32    u32TileIndex;

        // Selects it
        orxConfig_SelectSection(zSectionName);

        // Gets tile's origin (pixels)
        orxConfig_GetVector("TextureOrigin", &vTileOrigin);

        // Gets tile's origin (tiles)
        orxVector_Round(&vTileOrigin, orxVector_Div(&vTileOrigin, &vTileOrigin, &pstSet->vTileSize));

        // Computes its index
        u32TileIndex = orxF2U(vTileOrigin.fX + (pstSet->vSize.fX * vTileOrigin.fY)) + 1;

        // Stores it
        orxHashTable_Add(pstSet->pstIndexMap, (orxU64)orxString_ToCRC(zSectionName), (void *) CAST_HELPER u32TileIndex);
      }
    }
  }
  else
  {
    // Frees tile set
    orxBank_Free(spstTileSetBank, pstSet);
    pstSet = orxNULL;
  }

  // Pops config section
  orxConfig_PopSection();

  // Done!
  return pstSet;
}

orxTEXTURE *LoadMap(const orxSTRING _zMapName, const TileSet *_pstTileSet)
{
  orxVECTOR   vSize, vMapSize = {}, vScreenSize = {};
  orxBITMAP  *pstBitmap;
  orxTEXTURE *pstTexture;
  orxU8      *pu8Data, *pu8Value;
  orxU32      i, j, u32BitmapWidth, u32BitmapHeight;

  // Pushes its config section
  orxConfig_PushSection(_zMapName);

  // Gets its size (tiles)
  orxConfig_GetVector("Size", &vSize);

  // Adjusts map size
  vMapSize.fX = vSize.fX;
  vMapSize.fY = vSize.fY;
  vMapSize.fZ = orxMath_Ceil(vSize.fX / orx2F(2.0f)) * orx2F(2.0f);

  // Computes texture size (using 2 bytes per index as we have less than 65536 tiles in the set)
  u32BitmapWidth  = (orxF2U(vSize.fX) + 1) / 2;
  u32BitmapHeight = orxF2U(vSize.fY);

  // Creates bitmap
  pstBitmap = orxDisplay_CreateBitmap(u32BitmapWidth, u32BitmapHeight);
  orxASSERT(pstBitmap);

  // Creates texture
  pstTexture = orxTexture_Create();
  orxASSERT(pstTexture);

  // Links them together
  orxTexture_LinkBitmap(pstTexture, pstBitmap, _zMapName, orxTRUE);

  // Upgrades map to become its own graphic
  orxConfig_SetString("Texture", orxTexture_GetName(_pstTileSet->pstTexture));
  orxConfig_SetString("Pivot", "center");

  // Setups the shader on the map itself, with all needed parameters
  orxConfig_SetString("Code", "@MapShader");
  orxConfig_SetString("ParamList", "@MapShader");
  orxConfig_SetVector("CameraSize", &svCameraSize);
  orxConfig_SetVector("MapSize", &vMapSize);
  orxConfig_SetVector("TileSize", &_pstTileSet->vTileSize);
  orxConfig_SetVector("SetSize", &_pstTileSet->vSize);
  orxConfig_SetString("Map", _zMapName);
  orxDisplay_GetScreenSize(&vScreenSize.fX, &vScreenSize.fY);
  orxConfig_SetVector("Resolution", &vScreenSize);
  orxConfig_SetVector("CameraPos", &orxVECTOR_0);
  orxConfig_SetVector("Highlight", &orxVECTOR_0);

  // Allocates bitmap data
  pu8Data = (orxU8 *)orxMemory_Allocate(u32BitmapWidth * u32BitmapHeight * sizeof(orxRGBA), orxMEMORY_TYPE_TEMP);
  orxASSERT(pu8Data);

  // For all rows
  for(j = 0, pu8Value = pu8Data; j < orxF2U(vSize.fY); j++)
  {
    orxCHAR acRow[32] = {};

    // Gets row's name
    orxString_NPrint(acRow, sizeof(acRow) - 1, "Row%u", j + 1);

    // For all columns
    for(i = 0; i < orxF2U(vSize.fX); i++)
    {
      const orxSTRING zTile;
      orxU32          u32Index;

      // Pushes tile's section
      orxConfig_PushSection(orxConfig_GetListString(acRow, i));

      // Gets its name
      zTile = orxConfig_GetCurrentSection();

      // Pops config section
      orxConfig_PopSection();

      // Gets matching tile index
      u32Index = (orxU32)CAST_HELPER orxHashTable_Get(_pstTileSet->pstIndexMap, (orxU64)orxString_ToCRC(zTile)) - 1;

      // Stores it over two bytes
      *pu8Value++ = (u32Index & 0xFF00) >> 8;
      *pu8Value++ = u32Index & 0xFF;
    }

    // Zeroes padding bytes
    if(orxF2U(vSize.fX) & 1)
    {
      *pu8Value++ = 0;
      *pu8Value++ = 0;
    }
  }

  // Updates texture with indices
  orxDisplay_SetBitmapData(pstBitmap, pu8Data, u32BitmapWidth * u32BitmapHeight * sizeof(orxRGBA));

  // Deletes bitmap data
  orxMemory_Free(pu8Data);
  pu8Data = orxNULL;

  // Pops config section
  orxConfig_PopSection();

  // Done!
  return pstTexture;
}

void orxFASTCALL Update(const orxCLOCK_INFO *_pstInfo, void *_pContext)
{
  orxVECTOR vMousePos, vCameraPos = {}, vScreenSize, vRatio;
  orxSHADER *pstShader;

  // Screenshot?
  if(orxInput_IsActive("Screenshot") && orxInput_HasNewStatus("Screenshot"))
  {
    // Captures it
    orxScreenshot_Capture();
  }

  // Gets mouse position
  orxMouse_GetPosition(&vMousePos);

  // Gets screen size
  orxDisplay_GetScreenSize(&vScreenSize.fX, &vScreenSize.fY);
  vScreenSize.fZ = orxFLOAT_1;

  // Gets rendering aspect ratio to correct scrolling speed in full screen, based on mouse movement
  orxVector_Div(&vRatio, &svCameraSize, &vScreenSize);

  // Should scroll?
  if(orxInput_IsActive("Scroll"))
  {
    // Just started?
    if(orxInput_HasNewStatus("Scroll"))
    {
      // Resets scroll speed
      orxVector_Copy(&svScrollSpeed, &orxVECTOR_0);

      // Grabs mouse
      orxMouse_Grab(orxTRUE);
    }
    else
    {
      // Computes speed
      orxVector_Mul(&svScrollSpeed, orxVector_Sub(&svScrollSpeed, &svMousePos, &vMousePos), &vRatio);
    }

    // Stores scroll position
    orxVector_Copy(&svMousePos, &vMousePos);
  }
  else
  {
    // Just stopped?
    if(orxInput_HasNewStatus("Scroll"))
    {
      // Computes speed
      orxVector_Mul(&svScrollSpeed, orxVector_Sub(&svScrollSpeed, &svMousePos, &vMousePos), &vRatio);

      // Releases mouse
      orxMouse_Grab(orxFALSE);

      // Restores its position
      orxMouse_SetPosition(&vMousePos);
    }
  }

  // Has camera?
  if(spstCamera != orxNULL)
  {
    // Updates its position
    orxCamera_GetPosition(spstCamera, &vCameraPos);
    orxVector_Add(&vCameraPos, &vCameraPos, &svScrollSpeed);
    orxCamera_SetPosition(spstCamera, &vCameraPos);
  }

  // For all shaders
  for(pstShader = orxSHADER(orxStructure_GetFirst(orxSTRUCTURE_ID_SHADER));
      pstShader != orxNULL;
      pstShader = orxSHADER(orxStructure_GetNext(pstShader)))
  {
    // Sets its camera position
    orxShader_SetVectorParam(pstShader, "CameraPos", 0, &vCameraPos);

    // Sets its highlight position
    orxShader_SetVectorParam(pstShader, "Highlight", 0, &vMousePos);
  }
}

orxSTATUS orxFASTCALL Init()
{
  orxAABOX      stFrustum;
  TileSet      *pstGreenTileSet;
  orxVIEWPORT  *pstViewport;
  orxFLOAT      fCorrectionRatio;
  orxSTATUS eResult = orxSTATUS_SUCCESS;

  // Creates TileSet memory bank
  spstTileSetBank = orxBank_Create(32, sizeof(TileSet), orxBANK_KU32_FLAG_NONE, orxMEMORY_TYPE_MAIN);
  orxASSERT(spstTileSetBank);

  // Creates viewport
  pstViewport = orxViewport_CreateFromConfig("Viewport");
  orxSTRUCTURE_ASSERT(pstViewport);

  // Gets associated camera
  spstCamera = orxViewport_GetCamera(pstViewport);
  orxSTRUCTURE_ASSERT(spstCamera);

  // Gets its size
  orxCamera_GetFrustum(spstCamera, &stFrustum);
  orxVector_Sub(&svCameraSize, &stFrustum.vBR, &stFrustum.vTL);

  // Gets viewport correction ratio
  fCorrectionRatio = orxViewport_GetCorrectionRatio(pstViewport);

  // Applies correction ratio (for precise mouse movement in fullscreen when the screen aspect ratio doesn't match the camera's one)
  if(fCorrectionRatio >= orxFLOAT_1)
  {
    svCameraSize.fY *= fCorrectionRatio;
  }
  else
  {
    svCameraSize.fX /= fCorrectionRatio;
  }

  // Loads tile set
  pstGreenTileSet = LoadTileSet("GreenTiles");

  // Loads map
  LoadMap("CliffMap", pstGreenTileSet);

  // Creates scene
  orxObject_CreateFromConfig("Scene");

  // Registers update
  orxClock_Register(orxClock_FindFirst(orx2F(-1.0f), orxCLOCK_TYPE_CORE), Update, orxNULL, orxMODULE_ID_MAIN, orxCLOCK_PRIORITY_HIGH);

  // Done!
  return eResult;
}

orxSTATUS orxFASTCALL Run()
{
  orxSTATUS eResult = orxSTATUS_SUCCESS;

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
  // We could delete everything we created here but orx will do it for us anyway as long as we didn't do any direct memory allocations =)
}

orxSTATUS orxFASTCALL Bootstrap()
{
  orxSTATUS eResult = orxSTATUS_SUCCESS;

  // Adds config resource storages
  orxResource_AddStorage(orxCONFIG_KZ_RESOURCE_GROUP, "../data/config", orxFALSE);
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


