#undef  interface
#define interface struct
#include <d3d9.h>
#include <d3dx9.h>
#include <dwmapi.h>
#undef  interface

namespace{
  DWORD const VertexFVF = D3DFVF_XYZRHW | D3DFVF_TEX1;
  D3DCOLOR const BLACK = D3DCOLOR_XRGB(0, 0, 0);
  unsigned CPOT(unsigned n){ //round up to power of two
    n--;
    n |= n >>  1;
    n |= n >>  2;
    n |= n >>  4;
    n |= n >>  8;
    n |= n >> 16;
    return n + 1;
  }
  unsigned get_OS_version(){
    OSVERSIONINFO versionInfo = {0};
    versionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&versionInfo);
    return (versionInfo.dwMajorVersion > 5 ? (versionInfo.dwMinorVersion > 0 ? 7 : 6) : 5);
  }
};

namespace ruby{

class pVideoD3D{
  typedef IDirect3D9 * (__stdcall * d3d9_create_t)(UINT);
  typedef MMRESULT (__stdcall * timer_res_init_t)(UINT);
  typedef MMRESULT (__stdcall * timer_res_unit_t)(UINT);
  typedef HRESULT (__stdcall * create_effect_t)(IDirect3DDevice9 *, void const *, UINT, D3DXMACRO const *, ID3DXInclude *, DWORD, ID3DXEffectPool *, ID3DXEffect * *, ID3DXBuffer * *);
  typedef HRESULT (__stdcall * DwmEnableMMCSSType)(BOOL);
  typedef HRESULT (__stdcall * DwmFlushType)();
  typedef HRESULT (__stdcall * DwmIsCompositionEnabledType)(BOOL *);
  bool d3d9_available;
  bool winmm_available;
  bool shaders_available;
  bool dwm_available;
  HINSTANCE d3d9_dll;
  HINSTANCE winmm_dll;
  HINSTANCE shader_dll;
  HINSTANCE dwm_dll;
  d3d9_create_t    d3d9_create;
  timer_res_init_t timer_res_init;
  timer_res_unit_t timer_res_unit;
  create_effect_t  create_effect;
  struct{
    DwmEnableMMCSSType          DwmEnableMMCSSProc;
    DwmFlushType                DwmFlushProc;
    DwmIsCompositionEnabledType DwmIsCompositionEnabledProc;
  } DWM;
  unsigned OS_version;
  bool     dwm_enabled;
  unsigned shader_dll_version;
  IDirect3D9             * pD3D9;
  IDirect3DDevice9       * pDevice;
  IDirect3DSwapChain9    * pSwapChain;
  IDirect3DTexture9      * pSystemTexture;
  IDirect3DTexture9      * pDeviceTexture;
  IDirect3DVertexBuffer9 * pVertexBuffer;
  ID3DXEffect            * pEffect;
  D3DPRESENT_PARAMETERS presentation;
  D3DLOCKED_RECT        locked_rect;
  D3DSURFACE_DESC       texture_desc;
  D3DCAPS9              d3d9_caps;
  D3DRASTER_STATUS      raster_status;
  struct{
    HWND     handle;
    bool     synchronize;
    unsigned filter;
  } settings;
  struct{
    bool NonSquare; //device supports non-square textures
    bool NPOT;      //device supports non-power-of-two textures
    bool shader;    //device supports pixel shaders
  } caps;
  struct D3DVertex{
    float x, y;   //screen coordinates
    float z, rhw; //unused
    float u, v;   //texture coordinates
  } * pVertices;
  unsigned iw, ih, //input height/width
           tw, th, //texture height/width
           dw, dh; //destination height/width
  bool device_lost;
  string shader_source_xml;
public:
  bool cap(nall::string const & name){
    if(name == Video::Handle)      return true;
    if(name == Video::Synchronize) return true;
    if(name == Video::Filter)      return true;
    if(name == Video::Shader)      return true;
    return false;
  }

  nall::any get(nall::string const & name){
    if(name == Video::Handle)      return (uintptr_t)settings.handle;
    if(name == Video::Synchronize) return settings.synchronize;
    if(name == Video::Filter)      return settings.filter;
    return false;
  }
  
  bool set(nall::string const & name, nall::any const & value){
    if(name == Video::Handle)     { update_handle(any_cast<uintptr_t>   (value)); return true; }
    if(name == Video::Synchronize){ update_sync_v(any_cast<bool>        (value)); return true; }
    if(name == Video::Filter)     { update_filter(any_cast<unsigned>    (value)); return true; }
    if(name == Video::Shader)     { update_shader(any_cast<char const *>(value)); return true; }
    return false;
  }

  pVideoD3D(){
    init_dlls();    
    if(!d3d9_available){ unit_dlls(); return; }
    timer_res_init(1);
    BOOL tBOOL;
    DWM.DwmIsCompositionEnabledProc(&tBOOL);
    dwm_enabled = (tBOOL != FALSE);
    pDevice        = NULL;
    pSwapChain     = NULL;
    pSystemTexture = NULL;
    pDeviceTexture = NULL;
    pVertexBuffer  = NULL;
    pEffect        = NULL;
    settings.handle      = NULL;
    settings.synchronize = true;
    settings.filter      = Video::FilterLinear;
    caps.NonSquare = false;
    caps.NPOT      = false;
    caps.shader    = false;
    iw = 256; ih = 224;
    device_lost = true;
    shader_source_xml = "";
  }

  ~pVideoD3D(){
    if(d3d9_available) term();
    if(pD3D9) pD3D9->Release();
    timer_res_unit(1);
    unit_dlls();
  }

  bool init(){
    if(!d3d9_available) return false;
    term();
    presentation = {0};
    presentation.SwapEffect           = D3DSWAPEFFECT_DISCARD;
    presentation.hDeviceWindow        = settings.handle;
    presentation.Windowed             = TRUE;
    presentation.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
    if     (pD3D9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, settings.handle,
                                D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_FPU_PRESERVE,
                                &presentation, &pDevice) == D3D_OK);
    else if(pD3D9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, settings.handle,
                                D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_FPU_PRESERVE,
                                &presentation, &pDevice) == D3D_OK);
    else if(pD3D9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_REF, settings.handle,
                                D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_FPU_PRESERVE,
                                &presentation, &pDevice) != D3D_OK) return false;
    if(dwm_enabled) DWM.DwmEnableMMCSSProc(TRUE);
    pDevice->GetSwapChain(0, &pSwapChain);
    pDevice->GetDeviceCaps(&d3d9_caps);
    caps.NonSquare = !(d3d9_caps.TextureCaps & D3DPTEXTURECAPS_SQUAREONLY);
    caps.NPOT      = !(d3d9_caps.TextureCaps & D3DPTEXTURECAPS_POW2) &&
                     !(d3d9_caps.TextureCaps & D3DPTEXTURECAPS_NONPOW2CONDITIONAL);
    caps.shader    = !(d3d9_caps.PixelShaderVersion < D3DPS_VERSION(2, 0));
    tw = caps.NPOT ? iw : CPOT(iw);
    th = caps.NPOT ? ih : CPOT(ih);
    if(!caps.NonSquare) tw = th = max(tw, th);
    if(pDevice->CreateTexture(tw, th, 1, 0, D3DFMT_X8R8G8B8, D3DPOOL_SYSTEMMEM, &pSystemTexture, NULL) != D3D_OK){ term(); return false; }
    device_lost = false;
    return recover();
  }

  void term(){
    release_resources();
    shader_term();
    if(pSystemTexture){ pSystemTexture->Release(); pSystemTexture = NULL; }
    if(pSwapChain){ pSwapChain->Release(); pSwapChain = NULL; }
    if(pDevice){ pDevice->Release(); pDevice = NULL; }
  }

  void clear(){
    if(device_lost && !recover()) return;
    IDirect3DSurface9 * pSurface = NULL;
    pSystemTexture->GetSurfaceLevel(0, &pSurface);
    pDevice->ColorFill(pSurface, 0, BLACK);
    pSurface->Release();
    pDevice->Clear(0, 0, D3DCLEAR_TARGET, BLACK, 1.0f, 0);
    pSwapChain->Present(0, 0, 0, 0, 0);
    pDevice->Clear(0, 0, D3DCLEAR_TARGET, BLACK, 1.0f, 0);
    pSwapChain->Present(0, 0, 0, 0, 0);
  }

  bool lock(uint32_t * & data, unsigned & pitch, unsigned width, unsigned height){
    if(device_lost && !recover()) return false;
    if((iw != width || ih != height) && !resize(width, height)) return false;
    if(pSystemTexture->LockRect(0, &locked_rect, NULL, D3DLOCK_NOSYSLOCK) != D3D_OK) return false;
    pitch = locked_rect.Pitch;
    data  = (uint32_t*)locked_rect.pBits;
    return true;
  }

  void unlock(){
    pSystemTexture->UnlockRect(0);
    pDevice->UpdateTexture(pSystemTexture, pDeviceTexture);
  }

  void refresh(){
    if(device_lost && !recover()) return;
    RECT rd;
    GetClientRect(settings.handle, &rd);
    if(dw != rd.right || dh != rd.bottom){ init(); return; }
    render();
    if(dwm_enabled){
      if(pSwapChain->Present(0, 0, 0, 0, 0) != D3D_OK) device_lost = true;
      if(settings.synchronize) DWM.DwmFlushProc();
    }else{
      if(settings.synchronize) do pSwapChain->GetRasterStatus(&raster_status); while(!raster_status.InVBlank);
      if(pSwapChain->Present(0, 0, 0, 0, 0) != D3D_OK) device_lost = true;
    }
  }
private:
  void render(){
    if(pEffect){
      UINT pass, passes;
      D3DXVECTOR4 rubyInputSize  (iw, ih, 1.0/ih, 1.0/iw);
      D3DXVECTOR4 rubyTextureSize(tw, th, 1.0/th, 1.0/tw);
      D3DXVECTOR4 rubyOutputSize (dw, dh, 1.0/dh, 1.0/dw);
      pEffect->SetVector("rubyInputSize",   &rubyInputSize);
      pEffect->SetVector("rubyTextureSize", &rubyTextureSize);
      pEffect->SetVector("rubyOutputSize",  &rubyOutputSize);
      pDevice->BeginScene();
      pEffect->Begin(&passes, 0);
      pEffect->SetTexture("rubyTexture", pDeviceTexture);
      for(pass = 0; pass < passes; pass++){
        pEffect->BeginPass(pass);
        pEffect->CommitChanges();
        pDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
        pEffect->EndPass();
      }
      pEffect->End();
      pDevice->EndScene();
    }else{
      pDevice->BeginScene();
      pDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
      pDevice->EndScene();
    }
  }

  bool recover(){
    if(device_lost){
      release_resources();
      if(pDevice->TestCooperativeLevel() == D3DERR_DEVICELOST) return false;
      else if(pDevice->Reset(&presentation) != D3D_OK) return init();
      device_lost = false;
    }
    pDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    pDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
    pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    pDevice->SetVertexShader(NULL);
    pDevice->SetFVF(VertexFVF);
    RECT rd;
    GetClientRect(settings.handle, &rd);
    dw = rd.right; dh = rd.bottom;
    if(!allocate_textures()) return false;
    if(!allocate_vertices()) return false;
    assign_filter();
    assign_shader();
    clear();
    return true;
  }

  void release_resources(){
    if(pVertexBuffer){ pVertexBuffer->Release(); pVertexBuffer = NULL; }
    if(pDeviceTexture){ pDeviceTexture->Release(); pDeviceTexture = NULL; }
  }

  bool resize(unsigned width, unsigned height){
    iw = width; ih = height;
    width  = (caps.NPOT ? iw : CPOT(iw));
    height = (caps.NPOT ? ih : CPOT(ih));
    if(!caps.NonSquare) width = height = max(width, height);
    if(tw != width || th != height){
      tw = width; th = height;
      if(pSystemTexture){ pSystemTexture->Release(); pSystemTexture = NULL; }
      if(pDevice->CreateTexture(tw, th, 1, 0, D3DFMT_X8R8G8B8, D3DPOOL_SYSTEMMEM, &pSystemTexture, NULL) != D3D_OK) return false;
      if(!allocate_textures()) return false;
    }
    if(!adjust_vertices()) return false;
    return true;
  }

  bool allocate_textures(){
    if(pDeviceTexture){ pDeviceTexture->Release(); pDeviceTexture = NULL; }
    if(pDevice->CreateTexture(tw, th, 1, 0, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &pDeviceTexture, NULL) != D3D_OK) return false;
    if(pDevice->SetTexture(0, pDeviceTexture) != D3D_OK) return false;
    return true;
  }
  
  bool allocate_vertices(){
    if(pVertexBuffer){ pVertexBuffer->Release(); pVertexBuffer = NULL; }
    if(pDevice->CreateVertexBuffer(4 * sizeof(D3DVertex), D3DUSAGE_WRITEONLY, VertexFVF, D3DPOOL_DEFAULT, &pVertexBuffer, NULL) != D3D_OK) return false;
    if(!adjust_vertices()) return false;
    if(pDevice->SetStreamSource(0, pVertexBuffer, 0, sizeof(D3DVertex)) != D3D_OK) return false;
    return true;
  }

  // Vertex format:
  // 0----------1
  // |         /|
  // |       /  |
  // |     /    |
  // |   /      |
  // | /        |
  // 2----------3
  // (x,y) screen coords, in pixels
  // (u,v) texture coords, betweeen 0.0 (top, left) to 1.0 (bottom, right)
  bool adjust_vertices(){
    if(pVertexBuffer->Lock(0, 0, (void**)&pVertices, D3DLOCK_NOSYSLOCK) != D3D_OK)  return false;
    set_vertices(pVertices, (double)dw-0.5D, (double)dh-0.5D, (double)iw/(double)tw, (double)ih/(double)th);
    pVertexBuffer->Unlock();
    return true;
  }

  void set_vertices(D3DVertex * const p, const float x, const float y, const float u, const float v){
    p[0].x   = p[2].x   = p[0].y   = p[1].y   = -0.5f;
    p[0].z   = p[1].z   = p[2].z   = p[3].z   =  0.0f;
    p[0].u   = p[2].u   = p[0].v   = p[1].v   =  0.0f;
    p[0].rhw = p[1].rhw = p[2].rhw = p[3].rhw =  1.0f;
    p[1].x = p[3].x = x;
    p[2].y = p[3].y = y;
    p[1].u = p[3].u = u;
    p[2].v = p[3].v = v;
  }

  void assign_filter(){
    if(device_lost) return;
    pDevice->SetSamplerState(0, D3DSAMP_MINFILTER, settings.filter);
    pDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, settings.filter);
    pDevice->SetSamplerState(0, D3DSAMP_ADDRESSU,  D3DTADDRESS_MIRROR);
    pDevice->SetSamplerState(0, D3DSAMP_ADDRESSV,  D3DTADDRESS_MIRROR);
  }

  void update_handle(uintptr_t const value){
    settings.handle = (HWND)value;
  }

  void update_sync_v(bool const value){
    settings.synchronize = value;
  }
  
  void update_filter(unsigned const value){
    switch(value){
      default: break;
      case Video::FilterPoint:  settings.filter = D3DTEXF_POINT;  break;
      case Video::FilterLinear: settings.filter = D3DTEXF_LINEAR; break;
    }
    assign_filter();
  }

  void update_shader(char const * const value){
    shader_source_xml = !value ? "" : value;
    assign_shader();
  }

  void init_dlls(){
    d3d9_available    = init_d3d9_dll();
    winmm_available   = init_winmm_dll();
    shaders_available = init_shader_dll();
    dwm_available     = init_dwm_dll();
  }
  
  void unit_dlls(){
    if(dwm_dll)   { FreeLibrary(dwm_dll);    dwm_dll    = NULL; }
    if(shader_dll){ FreeLibrary(shader_dll); shader_dll = NULL; }
    if(winmm_dll) { FreeLibrary(winmm_dll);  winmm_dll  = NULL; }
    if(d3d9_dll)  { FreeLibrary(d3d9_dll);   d3d9_dll   = NULL; }
  }

  bool init_d3d9_dll(){
    if((d3d9_dll    = LoadLibraryW(L"d3d9.dll"))                                  == NULL) return false;
    if((d3d9_create = (d3d9_create_t)GetProcAddress(d3d9_dll, "Direct3DCreate9")) == NULL) return false;
    if((pD3D9       = d3d9_create(D3D_SDK_VERSION))                               == NULL) return false;
    return true;
  }
  
  bool init_winmm_dll(){
    if((winmm_dll      = LoadLibraryW(L"winmm.dll"))                                     == NULL) return false;
    if((timer_res_init = (timer_res_init_t)GetProcAddress(winmm_dll, "timeBeginPeriod")) == NULL) return false;
    if((timer_res_unit = (timer_res_unit_t)GetProcAddress(winmm_dll, "timeEndPeriod")  ) == NULL) return false;
    return true;
  }

  bool init_dwm_dll(){
    OS_version = get_OS_version();
    if(OS_version <= 5) return false;
    if((dwm_dll = LoadLibraryW(L"dwmapi.dll"))                                                                             == NULL) return false;
    if((DWM.DwmEnableMMCSSProc          = (DwmEnableMMCSSType)         GetProcAddress(dwm_dll, "DwmEnableMMCSS")         ) == NULL) return false;
    if((DWM.DwmFlushProc                = (DwmFlushType)               GetProcAddress(dwm_dll, "DwmFlush")               ) == NULL) return false;
    if((DWM.DwmIsCompositionEnabledProc = (DwmIsCompositionEnabledType)GetProcAddress(dwm_dll, "DwmIsCompositionEnabled")) == NULL) return false;
    return true;
  }

  bool init_shader_dll(){
    char t[256];
    shader_dll = NULL;
    for(shader_dll_version = 255; shader_dll_version > 0; shader_dll_version--){ 
      sprintf(t, "d3dx9_%u.dll", shader_dll_version);
      shader_dll = LoadLibraryW(utf16_t(t));
      if(shader_dll != NULL) break;
    }
    if(shader_dll == NULL) shader_dll = LoadLibraryW(L"d3dx9.dll");
    if(shader_dll == NULL) return false;
    if((create_effect = (create_effect_t)GetProcAddress(shader_dll, "D3DXCreateEffect")) == NULL) return false;
    return true;
  }
  
  //Shader-specific section:
  
  void shader_term(){
    if(pEffect){ pEffect->Release(); pEffect = NULL; }
  }
  
  void assign_shader(){
    if(!shaders_available || device_lost || !caps.shader) return;
    shader_term();
    bool is_hlsl = false;
    string shader_source;
    xml_element document = xml_parse(shader_source_xml);
    foreach(head, document.element){
      if(head.name == "shader"){
        foreach(attribute, head.attribute){
          if(attribute.name == "language" && attribute.content == "HLSL") is_hlsl = true;
        }
        foreach(element, head.element){
          if(element.name == "source" && is_hlsl) shader_source = element.parse();
        }
      }
    }
    if(shader_source == "") return;
    DWORD compatibility = (shader_dll_version > 31) ? D3DXSHADER_ENABLE_BACKWARDS_COMPATIBILITY : 0;
    ID3DXBuffer * pBufferErrors = NULL;
    if(create_effect(pDevice, shader_source, lstrlenA(shader_source), NULL, NULL,
                    D3DXSHADER_OPTIMIZATION_LEVEL3 | compatibility,
                    NULL, &pEffect, &pBufferErrors) != D3D_OK){
      if(pBufferErrors && pBufferErrors->GetBufferSize() > 1){
        char * t = new char[pBufferErrors->GetBufferSize() + lstrlenA("Effect compilation failed with the following errors: ") + 1];
        sprintf(t, "Effect compilation failed with the following errors: %s", (char const *)pBufferErrors->GetBufferPointer());
        MessageBoxW(settings.handle, utf16_t(t), L"HLSL effect compilation error", MB_OK);
        delete [] t;
      }
      return;
    }
    D3DXHANDLE hTech = NULL;
    if(pEffect->FindNextValidTechnique(NULL, &hTech) != D3D_OK){
      MessageBoxW(settings.handle, L"Unable to select a technique from the currently loaded effect; your hardware appears to be incompatible.", L"Error loading effect", MB_OK);
      return;
    }
    pEffect->SetTechnique(hTech);
  }
}; //class pVideoD3D

DeclareVideo(D3D)

}; //namespace ruby
