/*
 * Copyright 2011 Joakim Sindholt <opensource@zhasha.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHOR(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE. */

#ifndef _NINE_DEVICE9_H_
#define _NINE_DEVICE9_H_

#include "d3dadapter/d3dadapter9.h"

#include "iunknown.h"
#include "adapter9.h"

#include "nine_helpers.h"
#include "nine_state.h"

struct gen_mipmap_state;
struct util_hash_table;
struct pipe_screen;
struct pipe_context;
struct cso_context;
struct hud_context;
struct u_upload_mgr;

struct NineSwapChain9;
struct NineStateBlock9;

#include "util/u_double_list.h"

struct NineDevice9
{
    struct NineUnknown base;
    boolean ex;

    /* G3D context */
    struct pipe_screen *screen;
    struct pipe_context *pipe;
    struct cso_context *cso;

    /* creation parameters */
    D3DCAPS9 caps;
    D3DDEVICE_CREATION_PARAMETERS params;
    IDirect3D9 *d3d9;

    /* swapchain stuff */
    ID3DPresentGroup *present;
    struct NineSwapChain9 **swapchains;
    unsigned nswapchains;

    struct NineStateBlock9 *record;
    struct nine_state *update; /* state to update (&state / &record->state) */
    struct nine_state state;   /* device state */

    struct list_head update_textures;

    boolean is_recording;
    boolean in_scene;

    boolean prefer_user_constbuf;

    struct pipe_resource *constbuf_vs;
    struct pipe_resource *constbuf_ps;
    uint16_t vs_const_size;
    uint16_t ps_const_size;
    uint16_t max_vs_const_f;
    uint16_t max_ps_const_f;

    struct gen_mipmap_state *gen_mipmap;

    struct {
        struct util_hash_table *ht_vs;
        struct util_hash_table *ht_ps;
        struct NineVertexShader9 *vs;
        struct NinePixelShader9 *ps;
        unsigned num_vs;
        unsigned num_ps;
        float *vs_const;
        float *ps_const;

        struct util_hash_table *ht_fvf;
    } ff;

    struct {
        struct pipe_resource *image;
        unsigned w;
        unsigned h;
        POINT hotspot; /* -1, -1 if no cursor image set */
        POINT pos;
        BOOL visible;
        boolean software;
    } cursor;

    struct {
        boolean user_vbufs;
        boolean user_ibufs;
        boolean window_space_position_support;
        boolean vs_integer;
        boolean ps_integer;
    } driver_caps;

    struct u_upload_mgr *upload;

    struct nine_range_pool range_pool;

    struct hud_context *hud; /* NULL if hud is disabled */
};
static INLINE struct NineDevice9 *
NineDevice9( void *data )
{
    return (struct NineDevice9 *)data;
}

HRESULT
NineDevice9_new( struct pipe_screen *pScreen,
                 D3DDEVICE_CREATION_PARAMETERS *pCreationParameters,
                 D3DCAPS9 *pCaps,
                 D3DPRESENT_PARAMETERS *pPresentationParameters,
                 IDirect3D9 *pD3D9,
                 ID3DPresentGroup *pPresentationGroup,
                 struct d3dadapter9_context *pCTX,
                 boolean ex,
                 D3DDISPLAYMODEEX *pFullscreenDisplayMode,
                 struct NineDevice9 **ppOut );

HRESULT
NineDevice9_ctor( struct NineDevice9 *This,
                  struct NineUnknownParams *pParams,
                  struct pipe_screen *pScreen,
                  D3DDEVICE_CREATION_PARAMETERS *pCreationParameters,
                  D3DCAPS9 *pCaps,
                  D3DPRESENT_PARAMETERS *pPresentationParameters,
                  IDirect3D9 *pD3D9,
                  ID3DPresentGroup *pPresentationGroup,
                  struct d3dadapter9_context *pCTX,
                  boolean ex,
                  D3DDISPLAYMODEEX *pFullscreenDisplayMode );

void
NineDevice9_dtor( struct NineDevice9 *This );

/*** Nine private ***/

struct pipe_screen *
NineDevice9_GetScreen( struct NineDevice9 *This );

struct pipe_context *
NineDevice9_GetPipe( struct NineDevice9 *This );

struct cso_context *
NineDevice9_GetCSO( struct NineDevice9 *This );

const D3DCAPS9 *
NineDevice9_GetCaps( struct NineDevice9 *This );

/* Mask: 0x1 = constant buffers, 0x2 = stipple */
void
NineDevice9_RestoreNonCSOState( struct NineDevice9 *This, unsigned mask );

/*** Direct3D public ***/

HRESULT WINAPI
NineDevice9_TestCooperativeLevel( struct NineDevice9 *This );

UINT WINAPI
NineDevice9_GetAvailableTextureMem( struct NineDevice9 *This );

HRESULT WINAPI
NineDevice9_EvictManagedResources( struct NineDevice9 *This );

HRESULT WINAPI
NineDevice9_GetDirect3D( struct NineDevice9 *This,
                         IDirect3D9 **ppD3D9 );

HRESULT WINAPI
NineDevice9_GetDeviceCaps( struct NineDevice9 *This,
                           D3DCAPS9 *pCaps );

HRESULT WINAPI
NineDevice9_GetDisplayMode( struct NineDevice9 *This,
                            UINT iSwapChain,
                            D3DDISPLAYMODE *pMode );

HRESULT WINAPI
NineDevice9_GetCreationParameters( struct NineDevice9 *This,
                                   D3DDEVICE_CREATION_PARAMETERS *pParameters );

HRESULT WINAPI
NineDevice9_SetCursorProperties( struct NineDevice9 *This,
                                 UINT XHotSpot,
                                 UINT YHotSpot,
                                 IDirect3DSurface9 *pCursorBitmap );

void WINAPI
NineDevice9_SetCursorPosition( struct NineDevice9 *This,
                               int X,
                               int Y,
                               DWORD Flags );

BOOL WINAPI
NineDevice9_ShowCursor( struct NineDevice9 *This,
                        BOOL bShow );

HRESULT WINAPI
NineDevice9_CreateAdditionalSwapChain( struct NineDevice9 *This,
                                       D3DPRESENT_PARAMETERS *pPresentationParameters,
                                       IDirect3DSwapChain9 **pSwapChain );

HRESULT WINAPI
NineDevice9_GetSwapChain( struct NineDevice9 *This,
                          UINT iSwapChain,
                          IDirect3DSwapChain9 **pSwapChain );

UINT WINAPI
NineDevice9_GetNumberOfSwapChains( struct NineDevice9 *This );

HRESULT WINAPI
NineDevice9_Reset( struct NineDevice9 *This,
                   D3DPRESENT_PARAMETERS *pPresentationParameters );

HRESULT WINAPI
NineDevice9_Present( struct NineDevice9 *This,
                     const RECT *pSourceRect,
                     const RECT *pDestRect,
                     HWND hDestWindowOverride,
                     const RGNDATA *pDirtyRegion );

HRESULT WINAPI
NineDevice9_GetBackBuffer( struct NineDevice9 *This,
                           UINT iSwapChain,
                           UINT iBackBuffer,
                           D3DBACKBUFFER_TYPE Type,
                           IDirect3DSurface9 **ppBackBuffer );

HRESULT WINAPI
NineDevice9_GetRasterStatus( struct NineDevice9 *This,
                             UINT iSwapChain,
                             D3DRASTER_STATUS *pRasterStatus );

HRESULT WINAPI
NineDevice9_SetDialogBoxMode( struct NineDevice9 *This,
                              BOOL bEnableDialogs );

void WINAPI
NineDevice9_SetGammaRamp( struct NineDevice9 *This,
                          UINT iSwapChain,
                          DWORD Flags,
                          const D3DGAMMARAMP *pRamp );

void WINAPI
NineDevice9_GetGammaRamp( struct NineDevice9 *This,
                          UINT iSwapChain,
                          D3DGAMMARAMP *pRamp );

HRESULT WINAPI
NineDevice9_CreateTexture( struct NineDevice9 *This,
                           UINT Width,
                           UINT Height,
                           UINT Levels,
                           DWORD Usage,
                           D3DFORMAT Format,
                           D3DPOOL Pool,
                           IDirect3DTexture9 **ppTexture,
                           HANDLE *pSharedHandle );

HRESULT WINAPI
NineDevice9_CreateVolumeTexture( struct NineDevice9 *This,
                                 UINT Width,
                                 UINT Height,
                                 UINT Depth,
                                 UINT Levels,
                                 DWORD Usage,
                                 D3DFORMAT Format,
                                 D3DPOOL Pool,
                                 IDirect3DVolumeTexture9 **ppVolumeTexture,
                                 HANDLE *pSharedHandle );

HRESULT WINAPI
NineDevice9_CreateCubeTexture( struct NineDevice9 *This,
                               UINT EdgeLength,
                               UINT Levels,
                               DWORD Usage,
                               D3DFORMAT Format,
                               D3DPOOL Pool,
                               IDirect3DCubeTexture9 **ppCubeTexture,
                               HANDLE *pSharedHandle );

HRESULT WINAPI
NineDevice9_CreateVertexBuffer( struct NineDevice9 *This,
                                UINT Length,
                                DWORD Usage,
                                DWORD FVF,
                                D3DPOOL Pool,
                                IDirect3DVertexBuffer9 **ppVertexBuffer,
                                HANDLE *pSharedHandle );

HRESULT WINAPI
NineDevice9_CreateIndexBuffer( struct NineDevice9 *This,
                               UINT Length,
                               DWORD Usage,
                               D3DFORMAT Format,
                               D3DPOOL Pool,
                               IDirect3DIndexBuffer9 **ppIndexBuffer,
                               HANDLE *pSharedHandle );

HRESULT WINAPI
NineDevice9_CreateRenderTarget( struct NineDevice9 *This,
                                UINT Width,
                                UINT Height,
                                D3DFORMAT Format,
                                D3DMULTISAMPLE_TYPE MultiSample,
                                DWORD MultisampleQuality,
                                BOOL Lockable,
                                IDirect3DSurface9 **ppSurface,
                                HANDLE *pSharedHandle );

HRESULT WINAPI
NineDevice9_CreateDepthStencilSurface( struct NineDevice9 *This,
                                       UINT Width,
                                       UINT Height,
                                       D3DFORMAT Format,
                                       D3DMULTISAMPLE_TYPE MultiSample,
                                       DWORD MultisampleQuality,
                                       BOOL Discard,
                                       IDirect3DSurface9 **ppSurface,
                                       HANDLE *pSharedHandle );

HRESULT WINAPI
NineDevice9_UpdateSurface( struct NineDevice9 *This,
                           IDirect3DSurface9 *pSourceSurface,
                           const RECT *pSourceRect,
                           IDirect3DSurface9 *pDestinationSurface,
                           const POINT *pDestPoint );

HRESULT WINAPI
NineDevice9_UpdateTexture( struct NineDevice9 *This,
                           IDirect3DBaseTexture9 *pSourceTexture,
                           IDirect3DBaseTexture9 *pDestinationTexture );

HRESULT WINAPI
NineDevice9_GetRenderTargetData( struct NineDevice9 *This,
                                 IDirect3DSurface9 *pRenderTarget,
                                 IDirect3DSurface9 *pDestSurface );

HRESULT WINAPI
NineDevice9_GetFrontBufferData( struct NineDevice9 *This,
                                UINT iSwapChain,
                                IDirect3DSurface9 *pDestSurface );

HRESULT WINAPI
NineDevice9_StretchRect( struct NineDevice9 *This,
                         IDirect3DSurface9 *pSourceSurface,
                         const RECT *pSourceRect,
                         IDirect3DSurface9 *pDestSurface,
                         const RECT *pDestRect,
                         D3DTEXTUREFILTERTYPE Filter );

HRESULT WINAPI
NineDevice9_ColorFill( struct NineDevice9 *This,
                       IDirect3DSurface9 *pSurface,
                       const RECT *pRect,
                       D3DCOLOR color );

HRESULT WINAPI
NineDevice9_CreateOffscreenPlainSurface( struct NineDevice9 *This,
                                         UINT Width,
                                         UINT Height,
                                         D3DFORMAT Format,
                                         D3DPOOL Pool,
                                         IDirect3DSurface9 **ppSurface,
                                         HANDLE *pSharedHandle );

HRESULT WINAPI
NineDevice9_SetRenderTarget( struct NineDevice9 *This,
                             DWORD RenderTargetIndex,
                             IDirect3DSurface9 *pRenderTarget );

HRESULT WINAPI
NineDevice9_GetRenderTarget( struct NineDevice9 *This,
                             DWORD RenderTargetIndex,
                             IDirect3DSurface9 **ppRenderTarget );

HRESULT WINAPI
NineDevice9_SetDepthStencilSurface( struct NineDevice9 *This,
                                    IDirect3DSurface9 *pNewZStencil );

HRESULT WINAPI
NineDevice9_GetDepthStencilSurface( struct NineDevice9 *This,
                                    IDirect3DSurface9 **ppZStencilSurface );

HRESULT WINAPI
NineDevice9_BeginScene( struct NineDevice9 *This );

HRESULT WINAPI
NineDevice9_EndScene( struct NineDevice9 *This );

HRESULT WINAPI
NineDevice9_Clear( struct NineDevice9 *This,
                   DWORD Count,
                   const D3DRECT *pRects,
                   DWORD Flags,
                   D3DCOLOR Color,
                   float Z,
                   DWORD Stencil );

HRESULT WINAPI
NineDevice9_SetTransform( struct NineDevice9 *This,
                          D3DTRANSFORMSTATETYPE State,
                          const D3DMATRIX *pMatrix );

HRESULT WINAPI
NineDevice9_GetTransform( struct NineDevice9 *This,
                          D3DTRANSFORMSTATETYPE State,
                          D3DMATRIX *pMatrix );

HRESULT WINAPI
NineDevice9_MultiplyTransform( struct NineDevice9 *This,
                               D3DTRANSFORMSTATETYPE State,
                               const D3DMATRIX *pMatrix );

HRESULT WINAPI
NineDevice9_SetViewport( struct NineDevice9 *This,
                         const D3DVIEWPORT9 *pViewport );

HRESULT WINAPI
NineDevice9_GetViewport( struct NineDevice9 *This,
                         D3DVIEWPORT9 *pViewport );

HRESULT WINAPI
NineDevice9_SetMaterial( struct NineDevice9 *This,
                         const D3DMATERIAL9 *pMaterial );

HRESULT WINAPI
NineDevice9_GetMaterial( struct NineDevice9 *This,
                         D3DMATERIAL9 *pMaterial );

HRESULT WINAPI
NineDevice9_SetLight( struct NineDevice9 *This,
                      DWORD Index,
                      const D3DLIGHT9 *pLight );

HRESULT WINAPI
NineDevice9_GetLight( struct NineDevice9 *This,
                      DWORD Index,
                      D3DLIGHT9 *pLight );

HRESULT WINAPI
NineDevice9_LightEnable( struct NineDevice9 *This,
                         DWORD Index,
                         BOOL Enable );

HRESULT WINAPI
NineDevice9_GetLightEnable( struct NineDevice9 *This,
                            DWORD Index,
                            BOOL *pEnable );

HRESULT WINAPI
NineDevice9_SetClipPlane( struct NineDevice9 *This,
                          DWORD Index,
                          const float *pPlane );

HRESULT WINAPI
NineDevice9_GetClipPlane( struct NineDevice9 *This,
                          DWORD Index,
                          float *pPlane );

HRESULT WINAPI
NineDevice9_SetRenderState( struct NineDevice9 *This,
                            D3DRENDERSTATETYPE State,
                            DWORD Value );

HRESULT WINAPI
NineDevice9_GetRenderState( struct NineDevice9 *This,
                            D3DRENDERSTATETYPE State,
                            DWORD *pValue );

HRESULT WINAPI
NineDevice9_CreateStateBlock( struct NineDevice9 *This,
                              D3DSTATEBLOCKTYPE Type,
                              IDirect3DStateBlock9 **ppSB );

HRESULT WINAPI
NineDevice9_BeginStateBlock( struct NineDevice9 *This );

HRESULT WINAPI
NineDevice9_EndStateBlock( struct NineDevice9 *This,
                           IDirect3DStateBlock9 **ppSB );

HRESULT WINAPI
NineDevice9_SetClipStatus( struct NineDevice9 *This,
                           const D3DCLIPSTATUS9 *pClipStatus );

HRESULT WINAPI
NineDevice9_GetClipStatus( struct NineDevice9 *This,
                           D3DCLIPSTATUS9 *pClipStatus );

HRESULT WINAPI
NineDevice9_GetTexture( struct NineDevice9 *This,
                        DWORD Stage,
                        IDirect3DBaseTexture9 **ppTexture );

HRESULT WINAPI
NineDevice9_SetTexture( struct NineDevice9 *This,
                        DWORD Stage,
                        IDirect3DBaseTexture9 *pTexture );

HRESULT WINAPI
NineDevice9_GetTextureStageState( struct NineDevice9 *This,
                                  DWORD Stage,
                                  D3DTEXTURESTAGESTATETYPE Type,
                                  DWORD *pValue );

HRESULT WINAPI
NineDevice9_SetTextureStageState( struct NineDevice9 *This,
                                  DWORD Stage,
                                  D3DTEXTURESTAGESTATETYPE Type,
                                  DWORD Value );

HRESULT WINAPI
NineDevice9_GetSamplerState( struct NineDevice9 *This,
                             DWORD Sampler,
                             D3DSAMPLERSTATETYPE Type,
                             DWORD *pValue );

HRESULT WINAPI
NineDevice9_SetSamplerState( struct NineDevice9 *This,
                             DWORD Sampler,
                             D3DSAMPLERSTATETYPE Type,
                             DWORD Value );

HRESULT WINAPI
NineDevice9_ValidateDevice( struct NineDevice9 *This,
                            DWORD *pNumPasses );

HRESULT WINAPI
NineDevice9_SetPaletteEntries( struct NineDevice9 *This,
                               UINT PaletteNumber,
                               const PALETTEENTRY *pEntries );

HRESULT WINAPI
NineDevice9_GetPaletteEntries( struct NineDevice9 *This,
                               UINT PaletteNumber,
                               PALETTEENTRY *pEntries );

HRESULT WINAPI
NineDevice9_SetCurrentTexturePalette( struct NineDevice9 *This,
                                      UINT PaletteNumber );

HRESULT WINAPI
NineDevice9_GetCurrentTexturePalette( struct NineDevice9 *This,
                                      UINT *PaletteNumber );

HRESULT WINAPI
NineDevice9_SetScissorRect( struct NineDevice9 *This,
                            const RECT *pRect );

HRESULT WINAPI
NineDevice9_GetScissorRect( struct NineDevice9 *This,
                            RECT *pRect );

HRESULT WINAPI
NineDevice9_SetSoftwareVertexProcessing( struct NineDevice9 *This,
                                         BOOL bSoftware );

BOOL WINAPI
NineDevice9_GetSoftwareVertexProcessing( struct NineDevice9 *This );

HRESULT WINAPI
NineDevice9_SetNPatchMode( struct NineDevice9 *This,
                           float nSegments );

float WINAPI
NineDevice9_GetNPatchMode( struct NineDevice9 *This );

HRESULT WINAPI
NineDevice9_DrawPrimitive( struct NineDevice9 *This,
                           D3DPRIMITIVETYPE PrimitiveType,
                           UINT StartVertex,
                           UINT PrimitiveCount );

HRESULT WINAPI
NineDevice9_DrawIndexedPrimitive( struct NineDevice9 *This,
                                  D3DPRIMITIVETYPE PrimitiveType,
                                  INT BaseVertexIndex,
                                  UINT MinVertexIndex,
                                  UINT NumVertices,
                                  UINT startIndex,
                                  UINT primCount );

HRESULT WINAPI
NineDevice9_DrawPrimitiveUP( struct NineDevice9 *This,
                             D3DPRIMITIVETYPE PrimitiveType,
                             UINT PrimitiveCount,
                             const void *pVertexStreamZeroData,
                             UINT VertexStreamZeroStride );

HRESULT WINAPI
NineDevice9_DrawIndexedPrimitiveUP( struct NineDevice9 *This,
                                    D3DPRIMITIVETYPE PrimitiveType,
                                    UINT MinVertexIndex,
                                    UINT NumVertices,
                                    UINT PrimitiveCount,
                                    const void *pIndexData,
                                    D3DFORMAT IndexDataFormat,
                                    const void *pVertexStreamZeroData,
                                    UINT VertexStreamZeroStride );

HRESULT WINAPI
NineDevice9_ProcessVertices( struct NineDevice9 *This,
                             UINT SrcStartIndex,
                             UINT DestIndex,
                             UINT VertexCount,
                             IDirect3DVertexBuffer9 *pDestBuffer,
                             IDirect3DVertexDeclaration9 *pVertexDecl,
                             DWORD Flags );

HRESULT WINAPI
NineDevice9_CreateVertexDeclaration( struct NineDevice9 *This,
                                     const D3DVERTEXELEMENT9 *pVertexElements,
                                     IDirect3DVertexDeclaration9 **ppDecl );

HRESULT WINAPI
NineDevice9_SetVertexDeclaration( struct NineDevice9 *This,
                                  IDirect3DVertexDeclaration9 *pDecl );

HRESULT WINAPI
NineDevice9_GetVertexDeclaration( struct NineDevice9 *This,
                                  IDirect3DVertexDeclaration9 **ppDecl );

HRESULT WINAPI
NineDevice9_SetFVF( struct NineDevice9 *This,
                    DWORD FVF );

HRESULT WINAPI
NineDevice9_GetFVF( struct NineDevice9 *This,
                    DWORD *pFVF );

HRESULT WINAPI
NineDevice9_CreateVertexShader( struct NineDevice9 *This,
                                const DWORD *pFunction,
                                IDirect3DVertexShader9 **ppShader );

HRESULT WINAPI
NineDevice9_SetVertexShader( struct NineDevice9 *This,
                             IDirect3DVertexShader9 *pShader );

HRESULT WINAPI
NineDevice9_GetVertexShader( struct NineDevice9 *This,
                             IDirect3DVertexShader9 **ppShader );

HRESULT WINAPI
NineDevice9_SetVertexShaderConstantF( struct NineDevice9 *This,
                                      UINT StartRegister,
                                      const float *pConstantData,
                                      UINT Vector4fCount );

HRESULT WINAPI
NineDevice9_GetVertexShaderConstantF( struct NineDevice9 *This,
                                      UINT StartRegister,
                                      float *pConstantData,
                                      UINT Vector4fCount );

HRESULT WINAPI
NineDevice9_SetVertexShaderConstantI( struct NineDevice9 *This,
                                      UINT StartRegister,
                                      const int *pConstantData,
                                      UINT Vector4iCount );

HRESULT WINAPI
NineDevice9_GetVertexShaderConstantI( struct NineDevice9 *This,
                                      UINT StartRegister,
                                      int *pConstantData,
                                      UINT Vector4iCount );

HRESULT WINAPI
NineDevice9_SetVertexShaderConstantB( struct NineDevice9 *This,
                                      UINT StartRegister,
                                      const BOOL *pConstantData,
                                      UINT BoolCount );

HRESULT WINAPI
NineDevice9_GetVertexShaderConstantB( struct NineDevice9 *This,
                                      UINT StartRegister,
                                      BOOL *pConstantData,
                                      UINT BoolCount );

HRESULT WINAPI
NineDevice9_SetStreamSource( struct NineDevice9 *This,
                             UINT StreamNumber,
                             IDirect3DVertexBuffer9 *pStreamData,
                             UINT OffsetInBytes,
                             UINT Stride );

HRESULT WINAPI
NineDevice9_GetStreamSource( struct NineDevice9 *This,
                             UINT StreamNumber,
                             IDirect3DVertexBuffer9 **ppStreamData,
                             UINT *pOffsetInBytes,
                             UINT *pStride );

HRESULT WINAPI
NineDevice9_SetStreamSourceFreq( struct NineDevice9 *This,
                                 UINT StreamNumber,
                                 UINT Setting );

HRESULT WINAPI
NineDevice9_GetStreamSourceFreq( struct NineDevice9 *This,
                                 UINT StreamNumber,
                                 UINT *pSetting );

HRESULT WINAPI
NineDevice9_SetIndices( struct NineDevice9 *This,
                        IDirect3DIndexBuffer9 *pIndexData );

HRESULT WINAPI
NineDevice9_GetIndices( struct NineDevice9 *This,
                        IDirect3DIndexBuffer9 **ppIndexData /*,
                        UINT *pBaseVertexIndex */ );

HRESULT WINAPI
NineDevice9_CreatePixelShader( struct NineDevice9 *This,
                               const DWORD *pFunction,
                               IDirect3DPixelShader9 **ppShader );

HRESULT WINAPI
NineDevice9_SetPixelShader( struct NineDevice9 *This,
                            IDirect3DPixelShader9 *pShader );

HRESULT WINAPI
NineDevice9_GetPixelShader( struct NineDevice9 *This,
                            IDirect3DPixelShader9 **ppShader );

HRESULT WINAPI
NineDevice9_SetPixelShaderConstantF( struct NineDevice9 *This,
                                     UINT StartRegister,
                                     const float *pConstantData,
                                     UINT Vector4fCount );

HRESULT WINAPI
NineDevice9_GetPixelShaderConstantF( struct NineDevice9 *This,
                                     UINT StartRegister,
                                     float *pConstantData,
                                     UINT Vector4fCount );

HRESULT WINAPI
NineDevice9_SetPixelShaderConstantI( struct NineDevice9 *This,
                                     UINT StartRegister,
                                     const int *pConstantData,
                                     UINT Vector4iCount );

HRESULT WINAPI
NineDevice9_GetPixelShaderConstantI( struct NineDevice9 *This,
                                     UINT StartRegister,
                                     int *pConstantData,
                                     UINT Vector4iCount );

HRESULT WINAPI
NineDevice9_SetPixelShaderConstantB( struct NineDevice9 *This,
                                     UINT StartRegister,
                                     const BOOL *pConstantData,
                                     UINT BoolCount );

HRESULT WINAPI
NineDevice9_GetPixelShaderConstantB( struct NineDevice9 *This,
                                     UINT StartRegister,
                                     BOOL *pConstantData,
                                     UINT BoolCount );

HRESULT WINAPI
NineDevice9_DrawRectPatch( struct NineDevice9 *This,
                           UINT Handle,
                           const float *pNumSegs,
                           const D3DRECTPATCH_INFO *pRectPatchInfo );

HRESULT WINAPI
NineDevice9_DrawTriPatch( struct NineDevice9 *This,
                          UINT Handle,
                          const float *pNumSegs,
                          const D3DTRIPATCH_INFO *pTriPatchInfo );

HRESULT WINAPI
NineDevice9_DeletePatch( struct NineDevice9 *This,
                         UINT Handle );

HRESULT WINAPI
NineDevice9_CreateQuery( struct NineDevice9 *This,
                         D3DQUERYTYPE Type,
                         IDirect3DQuery9 **ppQuery );

#endif /* _NINE_DEVICE9_H_ */
