//! Includes
#include "orx.h"


//! Defines

#ifdef __orxWINDOWS__
  #ifdef __orxX86_64__
    #define CAST_HELPER (orxU64)
  #else /* __orxX86_64__ */
    #define CAST_HELPER (orxU32)
  #endif /* __orxX86_64__ */
#endif /* __orxWINDOWS__ */


//! Structures

typedef struct TileSet
{
  orxVECTOR     vSize;
  orxVECTOR     vTileSize;
  orxTEXTURE   *pstTexture;
  orxHASHTABLE *pstIndexTable;
  orxHASHTABLE *pstTileTable;
} TileSet;


//! Variables

static orxBANK  *spstTileSetBank;
static orxVECTOR svMousePos, svScrollSpeed, svScrollPos;


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

    // Creates index & tile tables
    pstSet->pstIndexTable = orxHashTable_Create(orxF2U(pstSet->vSize.fX * pstSet->vSize.fY), orxHASHTABLE_KU32_FLAG_NONE, orxMEMORY_TYPE_MAIN);
    pstSet->pstTileTable  = orxHashTable_Create(orxF2U(pstSet->vSize.fX * pstSet->vSize.fY), orxHASHTABLE_KU32_FLAG_NONE, orxMEMORY_TYPE_MAIN);
    orxASSERT(pstSet->pstIndexTable);
    orxASSERT(pstSet->pstTileTable);

    // Gets current section's name (for faster comparisons with orxConfig_GetParent() results)
    zSetName = orxConfig_GetCurrentSection();

    // For all config sections
    for(i = 0, u32Counter = orxConfig_GetSectionCounter();
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
        u32TileIndex = orxF2U(vTileOrigin.fX + (pstSet->vSize.fX * vTileOrigin.fY));

        // Stores it
        orxHashTable_Add(pstSet->pstIndexTable, (orxU64)zSectionName, (void *) CAST_HELPER u32TileIndex);
        orxHashTable_Add(pstSet->pstTileTable, u32TileIndex, (void *)zSectionName);
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
  orxVECTOR   vSize, vScreenSize = {};
  orxBITMAP  *pstBitmap;
  orxTEXTURE *pstTexture;
  orxU8      *pu8Data, *pu8Value;
  orxU32      i, j, u32BitmapSize;

  // Pushes its config section
  orxConfig_PushSection(_zMapName);

  // Gets its size (tiles)
  orxConfig_GetVector("Size", &vSize);

  // Computes texture size (using 2 bytes per index as we have less than 65536 tiles in the set)
  u32BitmapSize = (orxF2U(vSize.fX * vSize.fY) + 1) / 2;

  // Creates bitmap
  pstBitmap = orxDisplay_CreateBitmap(u32BitmapSize, 1);
  orxASSERT(pstBitmap);

  // Creates texture
  pstTexture = orxTexture_Create();
  orxASSERT(pstTexture);

  // Links them together
  orxTexture_LinkBitmap(pstTexture, pstBitmap, _zMapName, orxTRUE);

  // Upgrades map to become its own graphic
  orxConfig_SetString("Texture", _zMapName);
  orxConfig_SetString("Pivot", "center");

  // Setups the shader on the map itself, with all needed parameters
  orxConfig_SetString("Code", "@MapShader");
  orxConfig_SetString("ParamList", "@MapShader");
  orxConfig_SetVector("MapSize", &vSize);
  orxConfig_SetVector("TileSize", &_pstTileSet->vTileSize);
  orxConfig_SetVector("SetSize", &_pstTileSet->vSize);
  orxConfig_SetString("Set", orxTexture_GetName(_pstTileSet->pstTexture));
  orxDisplay_GetScreenSize(&vScreenSize.fX, &vScreenSize.fY);
  orxConfig_SetVector("Resolution", &vScreenSize);
  orxConfig_SetVector("CameraPos", &orxVECTOR_0);

  // Allocates bitmap data
  pu8Data = (orxU8 *)orxMemory_Allocate(u32BitmapSize * sizeof(orxRGBA), orxMEMORY_TYPE_TEMP);
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
      u32Index = (orxU32)CAST_HELPER orxHashTable_Get(_pstTileSet->pstIndexTable, (orxU64)zTile);

      // Stores it over two bytes
      *pu8Value++ = (u32Index & 0xFF00) >> 8;
      *pu8Value++ = u32Index & 0xFF;
    }
  }
  // Zeroes last remaining bytes
  while(pu8Value < pu8Data + (u32BitmapSize * sizeof(orxRGBA)))
  {
    *pu8Value++ = 0;
  }

  // Updates texture with indices
  orxDisplay_SetBitmapData(pstBitmap, pu8Data, u32BitmapSize * sizeof(orxRGBA));

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
  orxSHADER *pstShader;

  // Screenshot?
  if(orxInput_IsActive("Screenshot") && orxInput_HasNewStatus("Screenshot"))
  {
    // Captures it
    orxScreenshot_Capture();
  }

  // Should scroll?
  if(orxInput_IsActive("Scroll"))
  {
    orxVECTOR vMousePos;

    // Gets mouse world position
    orxRender_GetWorldPosition(orxMouse_GetPosition(&vMousePos), orxNULL, &vMousePos);

    // Just started?
    if(orxInput_HasNewStatus("Scroll"))
    {
      // Resets scroll speed
      orxVector_Copy(&svScrollSpeed, &orxVECTOR_0);
    }
    else
    {
      // Computes speed
      orxVector_Sub(&svScrollSpeed, &svMousePos, &vMousePos);
    }

    // Stores scroll position
    orxVector_Copy(&svMousePos, &vMousePos);
  }
  else
  {
    // Just stopped?
    if(orxInput_HasNewStatus("Scroll"))
    {
      orxVECTOR vMousePos;

      // Gets mouse world position
      orxRender_GetWorldPosition(orxMouse_GetPosition(&vMousePos), orxNULL, &vMousePos);

      // Computes speed
      orxVector_Sub(&svScrollSpeed, &svMousePos, &vMousePos);
    }
  }

  // Updates scroll position
  orxVector_Add(&svScrollPos, &svScrollPos, &svScrollSpeed);

  // For all shaders
  for(pstShader = orxSHADER(orxStructure_GetFirst(orxSTRUCTURE_ID_SHADER));
      pstShader != orxNULL;
      pstShader = orxSHADER(orxStructure_GetNext(pstShader)))
  {
    // Sets its camera position
    orxShader_SetVectorParam(pstShader, "CameraPos", 0, &svScrollPos);
  }
}

orxSTATUS orxFASTCALL Init()
{
  TileSet  *pstGreenTileSet;
  orxSTATUS eResult = orxSTATUS_SUCCESS;

  // Creates TileSet memory bank
  spstTileSetBank = orxBank_Create(32, sizeof(TileSet), orxBANK_KU32_FLAG_NONE, orxMEMORY_TYPE_MAIN);
  orxASSERT(spstTileSetBank);

  // Loads tile set
  pstGreenTileSet = LoadTileSet("GreenTiles");

  // Loads map
  LoadMap("CliffMap", pstGreenTileSet);

  // Creates viewport
  orxViewport_CreateFromConfig("Viewport");

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
