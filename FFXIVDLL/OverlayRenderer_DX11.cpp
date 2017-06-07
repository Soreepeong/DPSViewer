#include "OverlayRenderer_DX11.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_dx11.h"
#include "FFXIVDLL.h"
#include <assert.h>

#pragma warning(push)
#pragma warning(disable : 4005)
#include <wincodec.h>
#pragma warning(pop)

HRESULT CreateWICTextureFromMemory(_In_ ID3D11Device* d3dDevice,
	_In_opt_ ID3D11DeviceContext* d3dContext,
	_In_bytecount_(wicDataSize) const uint8_t* wicData,
	_In_ size_t wicDataSize,
	_Out_opt_ ID3D11Resource** texture,
	_Out_opt_ ID3D11ShaderResourceView** textureView,
	_In_ size_t maxsize = 0
);

HRESULT CreateWICTextureFromFile(_In_ ID3D11Device* d3dDevice,
	_In_opt_ ID3D11DeviceContext* d3dContext,
	_In_z_ const wchar_t* szFileName,
	_Out_opt_ ID3D11Resource** texture,
	_Out_opt_ ID3D11ShaderResourceView** textureView,
	_In_ size_t maxsize = 0
);



OverlayRendererDX11::OverlayRendererDX11(FFXIVDLL *dll, ID3D11Device* device, ID3D11DeviceContext *context, IDXGISwapChain *sc) :
	OverlayRenderer(dll),
	pDevice(device),
	pContext(context),
	pFW1Factory(nullptr),
	pFW(nullptr),
	pSwapChain(sc) {
	ImGui_ImplDX11_Init(dll->ffxiv(), device, context);
	FW1CreateFactory(FW1_VERSION, &pFW1Factory);
	ReloadFromConfig();
}

OverlayRendererDX11::~OverlayRendererDX11() {
	OnLostDevice();
	pFW1Factory->Release();
	ImGui_ImplDX11_Shutdown();
}

PDXTEXTURETYPE OverlayRendererDX11::GetTextureFromResource(TCHAR *resName) {
	PDXTEXTURETYPE tex = nullptr;

	if (mResourceTextures.find(resName) != mResourceTextures.end())
		return mResourceTextures[resName];

	HRSRC hResource = FindResource(dll->instance(), resName, MAKEINTRESOURCE(RT_RCDATA));
	if (hResource) {
		HGLOBAL hLoadedResource = LoadResource(dll->instance(), hResource);
		if (hLoadedResource) {
			LPVOID pLockedResource = LockResource(hLoadedResource);
			if (pLockedResource) {
				DWORD dwResourceSize = SizeofResource(dll->instance(), hResource);
				if (0 != dwResourceSize) {
					CreateWICTextureFromMemory(pDevice, pContext, (uint8_t*)pLockedResource, dwResourceSize, nullptr, &tex);
				}
			}
		}
	}

	mResourceTextures[resName] = tex;

	return tex;
}

PDXTEXTURETYPE OverlayRendererDX11::GetTextureFromFile(TCHAR *resName) {
	PDXTEXTURETYPE tex = nullptr;

	if (mFileTextures.find(resName) != mResourceTextures.end())
		return mFileTextures[resName];
	
	CreateWICTextureFromFile(pDevice, pContext, resName, nullptr, &tex);

	mFileTextures[resName] = tex;

	return tex;
}

void OverlayRendererDX11::ReloadFromConfig() {
	std::lock_guard<std::recursive_mutex> lock(mWindows.layoutLock);
	MultiByteToWideChar(CP_UTF8, 0, mConfig.fontName, -1, mFontName, 512);
	FW1_FONTWRAPPERCREATEPARAMS createParams;
	ZeroMemory(&createParams, sizeof(createParams));

	createParams.GlyphSheetWidth = 512;
	createParams.GlyphSheetHeight = 512;
	createParams.MaxGlyphCountPerSheet = 2048;
	createParams.SheetMipLevels = 1;
	createParams.AnisotropicFiltering = FALSE;
	createParams.MaxGlyphWidth = 384;
	createParams.MaxGlyphHeight = 384;
	createParams.DisableGeometryShader = FALSE;
	createParams.VertexBufferSize = 0;
	createParams.DefaultFontParams.pszFontFamily = mFontName;
	createParams.DefaultFontParams.FontWeight = mConfig.bold ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL;
	createParams.DefaultFontParams.FontStyle = DWRITE_FONT_STYLE_NORMAL;
	createParams.DefaultFontParams.FontStretch = DWRITE_FONT_STRETCH_NORMAL;
	createParams.DefaultFontParams.pszLocale = L"";
	pFW1Factory->CreateFontWrapper(pDevice, nullptr, &createParams, &pFW);
}

void OverlayRendererDX11::OnLostDevice() {
	std::lock_guard<std::recursive_mutex> lock(mWindows.layoutLock);
	ImGui_ImplDX11_InvalidateDeviceObjects();
	if (pFW != nullptr) {
		pFW->Release();
		pFW = nullptr;
	}
}

void OverlayRendererDX11::OnResetDevice() {
	std::lock_guard<std::recursive_mutex> lock(mWindows.layoutLock);
	ImGui_ImplDX11_CreateDeviceObjects();


	FW1_FONTWRAPPERCREATEPARAMS createParams;
	ZeroMemory(&createParams, sizeof(createParams));

	createParams.GlyphSheetWidth = 512;
	createParams.GlyphSheetHeight = 512;
	createParams.MaxGlyphCountPerSheet = 2048;
	createParams.SheetMipLevels = 1;
	createParams.AnisotropicFiltering = FALSE;
	createParams.MaxGlyphWidth = 384;
	createParams.MaxGlyphHeight = 384;
	createParams.DisableGeometryShader = FALSE;
	createParams.VertexBufferSize = 0;
	createParams.DefaultFontParams.pszFontFamily = mFontName;
	createParams.DefaultFontParams.FontWeight = mConfig.bold ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL;
	createParams.DefaultFontParams.FontStyle = DWRITE_FONT_STYLE_NORMAL;
	createParams.DefaultFontParams.FontStretch = DWRITE_FONT_STRETCH_NORMAL;
	createParams.DefaultFontParams.pszLocale = L"";
	pFW1Factory->CreateFontWrapper(pDevice, nullptr, &createParams, &pFW);
}

void OverlayRendererDX11::DrawBox(int x, int y, int w, int h, DWORD Color) {
	ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(x, y), ImVec2(x + w, y + h), Color);
}

std::vector<OverlayRendererDX11::RenderInfo> OverlayRendererDX11::textRenderList;

void OverlayRendererDX11::TextDrawCallbackExternal(const ImDrawList* parent_list, const ImDrawCmd* cmd) {
	OverlayRendererDX11::RenderInfo ri = OverlayRendererDX11::textRenderList[(int) cmd->UserCallbackData];
	ri.rend->TextDrawCallback(ri);
}
void OverlayRendererDX11::TextDrawCallback(RenderInfo& ri) {
	if(ri.w == -1)
		pFW->DrawString(pContext, ri.text, mConfig.fontSize, ri.x, ri.y, ri.color, ri.flags | FW1_RESTORESTATE);
	else {
		FW1_RECTF rf = { ri.x, ri.y, ri.x + ri.w, ri.y + ri.h };
		pFW->DrawString(pContext, ri.text, mFontName, mConfig.fontSize, &rf, ri.color, NULL, NULL, ri.flags | FW1_RESTORESTATE);
	}
}


void OverlayRendererDX11::DrawText(int x, int y, TCHAR *text, DWORD Color) {
	RenderInfo rnd = { this, text, x, y, -1, -1, 0, Color };
	ImGui::GetWindowDrawList()->AddCallback(TextDrawCallbackExternal, (PVOID) textRenderList.size());
	textRenderList.push_back(rnd);
}

void OverlayRendererDX11::DrawText(int x, int y, int width, int height, TCHAR *text, DWORD Color, int align) {
	RenderInfo rnd = { this, text, x, y, width, height, align, Color };
	ImGui::GetWindowDrawList()->AddCallback(TextDrawCallbackExternal, (PVOID) textRenderList.size());
	textRenderList.push_back(rnd);
}

void OverlayRendererDX11::DrawTexture(int x, int y, int w, int h, PDXTEXTURETYPE tex) {
	ImGui::GetWindowDrawList()->AddImage((ImTextureID) tex, ImVec2(x, y), ImVec2(x + w, y + h));
}
void OverlayRendererDX11::MeasureText(RECT &rc, TCHAR *text, int flags) {
	FW1_RECTF rf = { rc.left, rc.top, rc.right, rc.bottom }, rf2;
	if (rf.Right - rf.Left <= 0)
		rf.Right = rf.Left + 2000;
	if (rf.Bottom - rf.Top <= 0)
		rf.Bottom = rf.Top + 2000;
	// Modified function - it uses GetMetrics instead of GetOverhangMetrics
	rf2 = pFW->MeasureString(text, mFontName, mConfig.fontSize, &rf, flags);
	rc = { rc.left, rc.top, (int) rf2.Right, (int) rf2.Bottom };
	/*
	char *utf8;
	int len = WideCharToMultiByte(CP_UTF8, 0, text, -1, nullptr, 0, 0, 0);
	utf8 = new char[len];
	WideCharToMultiByte(CP_UTF8, 0, text, -1, utf8, len, 0, 0);
	ImVec2 res = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, (flags & DT_WORDBREAK) ? rc.right - rc.left : 0, utf8);
	delete[] utf8;
	rc.right = rc.left + res.x;
	rc.bottom = rc.top + res.y;
	//*/
}

void OverlayRendererDX11::RenderOverlay() {
	if (mConfig.UseDrawOverlay) {
		std::lock_guard<std::recursive_mutex> guard(mWindows.getLock());

		RECT rect;
		GetClientRect(dll->ffxiv(), &rect);

		ID3D11RenderTargetView *backbuffer;
		ID3D11Texture2D *pBackBuffer;
		pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*) &pBackBuffer);
		pDevice->CreateRenderTargetView(pBackBuffer, NULL, &backbuffer);
		pBackBuffer->Release();
		pContext->OMSetRenderTargets(1, &backbuffer, NULL);

		D3D11_TEXTURE2D_DESC desc;
		pBackBuffer->GetDesc(&desc);
		D3D11_VIEWPORT viewportf = {0, 0, desc.Width, desc.Height, 0, 1};

		ImGui_ImplDX11_NewFrame();
		textRenderList.clear();

		mWindows.width = rect.right - rect.left;
		mWindows.height = rect.bottom - rect.top;
		mWindows.measure(this, rect, rect.right - rect.left, rect.bottom - rect.top, false);
		for (auto it = mWindows.children[0].begin(); it != mWindows.children[0].end(); ++it) {
			if ((*it)->relativeSize) {
				(*it)->xF = min(1 - (*it)->calcWidth / (*it)->getParent()->width, max(0, (*it)->xF));
				(*it)->yF = min(1 - (*it)->calcHeight / (*it)->getParent()->height, max(0, (*it)->yF));
			}
		}
		ImGui::SetNextWindowPos(ImVec2(0, 0));
		ImGui::SetNextWindowSizeConstraints(ImVec2(rect.right - rect.left, rect.bottom - rect.top), ImVec2(rect.right - rect.left, rect.bottom - rect.top));
		bool open = true;
		ImGui::Begin("CustomWindowOverlay", &open, ImVec2(0, 0), 0, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs);
		mWindows.draw(this);
		ImGui::End();
		mConfig.Render();
		ImGui::Render();
		pFW->Flush(pContext);

		backbuffer->Release();
	}
}

void OverlayRendererDX11::CheckCapture() {
	if (mDoCapture) {

	}
}

/*
void OverlayRendererDX11::CaptureBackgroundSave(IDirect3DSurface9 *buf) {
	std::lock_guard<std::recursive_mutex> lock(mCaptureMutex);
	mCaptureBuffers.push_back(buf);
	if (hSaverThread == INVALID_HANDLE_VALUE)
		hSaverThread = CreateThread(NULL, NULL, OverlayRendererDX11::CaptureBackgroundSaverExternal, this, NULL, NULL);

	dll->pipe()->AddChat("/e Saving screenshot...");
}
//*/

void OverlayRendererDX11::CaptureBackgroundSaverThread() {
}




//---------------------------------------------------------------------------------
template<class T> class ScopedObject {
public:
	explicit ScopedObject(T *p = 0) : _pointer(p) {}
	~ScopedObject() {
		if (_pointer) {
			_pointer->Release();
			_pointer = nullptr;
		}
	}

	bool IsNull() const { return (!_pointer); }

	T& operator*() { return *_pointer; }
	T* operator->() { return _pointer; }
	T** operator&() { return &_pointer; }

	void Reset(T *p = 0) { if (_pointer) { _pointer->Release(); } _pointer = p; }

	T* Get() const { return _pointer; }

private:
	ScopedObject(const ScopedObject&);
	ScopedObject& operator=(const ScopedObject&);

	T* _pointer;
};

//-------------------------------------------------------------------------------------
// WIC Pixel Format Translation Data
//-------------------------------------------------------------------------------------
struct WICTranslate {
	GUID                wic;
	DXGI_FORMAT         format;
};

static WICTranslate g_WICFormats[] =
{
	{ GUID_WICPixelFormat128bppRGBAFloat,       DXGI_FORMAT_R32G32B32A32_FLOAT },

	{ GUID_WICPixelFormat64bppRGBAHalf,         DXGI_FORMAT_R16G16B16A16_FLOAT },
	{ GUID_WICPixelFormat64bppRGBA,             DXGI_FORMAT_R16G16B16A16_UNORM },

	{ GUID_WICPixelFormat32bppRGBA,             DXGI_FORMAT_R8G8B8A8_UNORM },
	{ GUID_WICPixelFormat32bppBGRA,             DXGI_FORMAT_B8G8R8A8_UNORM }, // DXGI 1.1
	{ GUID_WICPixelFormat32bppBGR,              DXGI_FORMAT_B8G8R8X8_UNORM }, // DXGI 1.1

	{ GUID_WICPixelFormat32bppRGBA1010102XR,    DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM }, // DXGI 1.1
	{ GUID_WICPixelFormat32bppRGBA1010102,      DXGI_FORMAT_R10G10B10A2_UNORM },
	{ GUID_WICPixelFormat32bppRGBE,             DXGI_FORMAT_R9G9B9E5_SHAREDEXP },

#ifdef DXGI_1_2_FORMATS

	{ GUID_WICPixelFormat16bppBGRA5551,         DXGI_FORMAT_B5G5R5A1_UNORM },
	{ GUID_WICPixelFormat16bppBGR565,           DXGI_FORMAT_B5G6R5_UNORM },

#endif // DXGI_1_2_FORMATS

	{ GUID_WICPixelFormat32bppGrayFloat,        DXGI_FORMAT_R32_FLOAT },
	{ GUID_WICPixelFormat16bppGrayHalf,         DXGI_FORMAT_R16_FLOAT },
	{ GUID_WICPixelFormat16bppGray,             DXGI_FORMAT_R16_UNORM },
	{ GUID_WICPixelFormat8bppGray,              DXGI_FORMAT_R8_UNORM },

	{ GUID_WICPixelFormat8bppAlpha,             DXGI_FORMAT_A8_UNORM },

#if (_WIN32_WINNT >= 0x0602 /*_WIN32_WINNT_WIN8*/)
	{ GUID_WICPixelFormat96bppRGBFloat,         DXGI_FORMAT_R32G32B32_FLOAT },
#endif
};

//-------------------------------------------------------------------------------------
// WIC Pixel Format nearest conversion table
//-------------------------------------------------------------------------------------

struct WICConvert {
	GUID        source;
	GUID        target;
};

static WICConvert g_WICConvert[] =
{
	// Note target GUID in this conversion table must be one of those directly supported formats (above).

	{ GUID_WICPixelFormatBlackWhite,            GUID_WICPixelFormat8bppGray }, // DXGI_FORMAT_R8_UNORM

	{ GUID_WICPixelFormat1bppIndexed,           GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM 
	{ GUID_WICPixelFormat2bppIndexed,           GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM 
	{ GUID_WICPixelFormat4bppIndexed,           GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM 
	{ GUID_WICPixelFormat8bppIndexed,           GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM 

	{ GUID_WICPixelFormat2bppGray,              GUID_WICPixelFormat8bppGray }, // DXGI_FORMAT_R8_UNORM 
	{ GUID_WICPixelFormat4bppGray,              GUID_WICPixelFormat8bppGray }, // DXGI_FORMAT_R8_UNORM 

	{ GUID_WICPixelFormat16bppGrayFixedPoint,   GUID_WICPixelFormat16bppGrayHalf }, // DXGI_FORMAT_R16_FLOAT 
	{ GUID_WICPixelFormat32bppGrayFixedPoint,   GUID_WICPixelFormat32bppGrayFloat }, // DXGI_FORMAT_R32_FLOAT 

#ifdef DXGI_1_2_FORMATS

	{ GUID_WICPixelFormat16bppBGR555,           GUID_WICPixelFormat16bppBGRA5551 }, // DXGI_FORMAT_B5G5R5A1_UNORM

#else

	{ GUID_WICPixelFormat16bppBGR555,           GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM
	{ GUID_WICPixelFormat16bppBGRA5551,         GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM
	{ GUID_WICPixelFormat16bppBGR565,           GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM

#endif // DXGI_1_2_FORMATS

	{ GUID_WICPixelFormat32bppBGR101010,        GUID_WICPixelFormat32bppRGBA1010102 }, // DXGI_FORMAT_R10G10B10A2_UNORM

	{ GUID_WICPixelFormat24bppBGR,              GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM 
	{ GUID_WICPixelFormat24bppRGB,              GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM 
	{ GUID_WICPixelFormat32bppPBGRA,            GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM 
	{ GUID_WICPixelFormat32bppPRGBA,            GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM 

	{ GUID_WICPixelFormat48bppRGB,              GUID_WICPixelFormat64bppRGBA }, // DXGI_FORMAT_R16G16B16A16_UNORM
	{ GUID_WICPixelFormat48bppBGR,              GUID_WICPixelFormat64bppRGBA }, // DXGI_FORMAT_R16G16B16A16_UNORM
	{ GUID_WICPixelFormat64bppBGRA,             GUID_WICPixelFormat64bppRGBA }, // DXGI_FORMAT_R16G16B16A16_UNORM
	{ GUID_WICPixelFormat64bppPRGBA,            GUID_WICPixelFormat64bppRGBA }, // DXGI_FORMAT_R16G16B16A16_UNORM
	{ GUID_WICPixelFormat64bppPBGRA,            GUID_WICPixelFormat64bppRGBA }, // DXGI_FORMAT_R16G16B16A16_UNORM

	{ GUID_WICPixelFormat48bppRGBFixedPoint,    GUID_WICPixelFormat64bppRGBAHalf }, // DXGI_FORMAT_R16G16B16A16_FLOAT 
	{ GUID_WICPixelFormat48bppBGRFixedPoint,    GUID_WICPixelFormat64bppRGBAHalf }, // DXGI_FORMAT_R16G16B16A16_FLOAT 
	{ GUID_WICPixelFormat64bppRGBAFixedPoint,   GUID_WICPixelFormat64bppRGBAHalf }, // DXGI_FORMAT_R16G16B16A16_FLOAT 
	{ GUID_WICPixelFormat64bppBGRAFixedPoint,   GUID_WICPixelFormat64bppRGBAHalf }, // DXGI_FORMAT_R16G16B16A16_FLOAT 
	{ GUID_WICPixelFormat64bppRGBFixedPoint,    GUID_WICPixelFormat64bppRGBAHalf }, // DXGI_FORMAT_R16G16B16A16_FLOAT 
	{ GUID_WICPixelFormat64bppRGBHalf,          GUID_WICPixelFormat64bppRGBAHalf }, // DXGI_FORMAT_R16G16B16A16_FLOAT 
	{ GUID_WICPixelFormat48bppRGBHalf,          GUID_WICPixelFormat64bppRGBAHalf }, // DXGI_FORMAT_R16G16B16A16_FLOAT 

	{ GUID_WICPixelFormat96bppRGBFixedPoint,    GUID_WICPixelFormat128bppRGBAFloat }, // DXGI_FORMAT_R32G32B32A32_FLOAT 
	{ GUID_WICPixelFormat128bppPRGBAFloat,      GUID_WICPixelFormat128bppRGBAFloat }, // DXGI_FORMAT_R32G32B32A32_FLOAT 
	{ GUID_WICPixelFormat128bppRGBFloat,        GUID_WICPixelFormat128bppRGBAFloat }, // DXGI_FORMAT_R32G32B32A32_FLOAT 
	{ GUID_WICPixelFormat128bppRGBAFixedPoint,  GUID_WICPixelFormat128bppRGBAFloat }, // DXGI_FORMAT_R32G32B32A32_FLOAT 
	{ GUID_WICPixelFormat128bppRGBFixedPoint,   GUID_WICPixelFormat128bppRGBAFloat }, // DXGI_FORMAT_R32G32B32A32_FLOAT 

	{ GUID_WICPixelFormat32bppCMYK,             GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM 
	{ GUID_WICPixelFormat64bppCMYK,             GUID_WICPixelFormat64bppRGBA }, // DXGI_FORMAT_R16G16B16A16_UNORM
	{ GUID_WICPixelFormat40bppCMYKAlpha,        GUID_WICPixelFormat64bppRGBA }, // DXGI_FORMAT_R16G16B16A16_UNORM
	{ GUID_WICPixelFormat80bppCMYKAlpha,        GUID_WICPixelFormat64bppRGBA }, // DXGI_FORMAT_R16G16B16A16_UNORM

#if (_WIN32_WINNT >= 0x0602 /*_WIN32_WINNT_WIN8*/)
	{ GUID_WICPixelFormat32bppRGB,              GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM
	{ GUID_WICPixelFormat64bppRGB,              GUID_WICPixelFormat64bppRGBA }, // DXGI_FORMAT_R16G16B16A16_UNORM
	{ GUID_WICPixelFormat64bppPRGBAHalf,        GUID_WICPixelFormat64bppRGBAHalf }, // DXGI_FORMAT_R16G16B16A16_FLOAT 
#endif

																					// We don't support n-channel formats
};

//--------------------------------------------------------------------------------------
static IWICImagingFactory* _GetWIC() {
	static IWICImagingFactory* s_Factory = nullptr;

	if (s_Factory)
		return s_Factory;

	HRESULT hr = CoCreateInstance(
		CLSID_WICImagingFactory,
		nullptr,
		CLSCTX_INPROC_SERVER,
		__uuidof(IWICImagingFactory),
		(LPVOID*) &s_Factory
	);

	if (FAILED(hr)) {
		s_Factory = nullptr;
		return nullptr;
	}

	return s_Factory;
}

//---------------------------------------------------------------------------------
static DXGI_FORMAT _WICToDXGI(const GUID& guid) {
	for (size_t i = 0; i < _countof(g_WICFormats); ++i) {
		if (memcmp(&g_WICFormats[i].wic, &guid, sizeof(GUID)) == 0)
			return g_WICFormats[i].format;
	}

	return DXGI_FORMAT_UNKNOWN;
}

//---------------------------------------------------------------------------------
static size_t _WICBitsPerPixel(REFGUID targetGuid) {
	IWICImagingFactory* pWIC = _GetWIC();
	if (!pWIC)
		return 0;

	ScopedObject<IWICComponentInfo> cinfo;
	if (FAILED(pWIC->CreateComponentInfo(targetGuid, &cinfo)))
		return 0;

	WICComponentType type;
	if (FAILED(cinfo->GetComponentType(&type)))
		return 0;

	if (type != WICPixelFormat)
		return 0;

	ScopedObject<IWICPixelFormatInfo> pfinfo;
	if (FAILED(cinfo->QueryInterface(__uuidof(IWICPixelFormatInfo), reinterpret_cast<void**>(&pfinfo))))
		return 0;

	UINT bpp;
	if (FAILED(pfinfo->GetBitsPerPixel(&bpp)))
		return 0;

	return bpp;
}

//---------------------------------------------------------------------------------
static HRESULT CreateTextureFromWIC(_In_ ID3D11Device* d3dDevice,
	_In_opt_ ID3D11DeviceContext* d3dContext,
	_In_ IWICBitmapFrameDecode *frame,
	_Out_opt_ ID3D11Resource** texture,
	_Out_opt_ ID3D11ShaderResourceView** textureView,
	_In_ size_t maxsize) {
	UINT width, height;
	HRESULT hr = frame->GetSize(&width, &height);
	if (FAILED(hr))
		return hr;

	assert(width > 0 && height > 0);

	if (!maxsize) {
		// This is a bit conservative because the hardware could support larger textures than
		// the Feature Level defined minimums, but doing it this way is much easier and more
		// performant for WIC than the 'fail and retry' model used by DDSTextureLoader

		switch (d3dDevice->GetFeatureLevel()) {
			case D3D_FEATURE_LEVEL_9_1:
			case D3D_FEATURE_LEVEL_9_2:
				maxsize = 2048 /*D3D_FL9_1_REQ_TEXTURE2D_U_OR_V_DIMENSION*/;
				break;

			case D3D_FEATURE_LEVEL_9_3:
				maxsize = 4096 /*D3D_FL9_3_REQ_TEXTURE2D_U_OR_V_DIMENSION*/;
				break;

			case D3D_FEATURE_LEVEL_10_0:
			case D3D_FEATURE_LEVEL_10_1:
				maxsize = 8192 /*D3D10_REQ_TEXTURE2D_U_OR_V_DIMENSION*/;
				break;

			default:
				maxsize = D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION;
				break;
		}
	}

	assert(maxsize > 0);

	UINT twidth, theight;
	if (width > maxsize || height > maxsize) {
		float ar = static_cast<float>(height) / static_cast<float>(width);
		if (width > height) {
			twidth = static_cast<UINT>(maxsize);
			theight = static_cast<UINT>(static_cast<float>(maxsize) * ar);
		} else {
			theight = static_cast<UINT>(maxsize);
			twidth = static_cast<UINT>(static_cast<float>(maxsize) / ar);
		}
		assert(twidth <= maxsize && theight <= maxsize);
	} else {
		twidth = width;
		theight = height;
	}

	// Determine format
	WICPixelFormatGUID pixelFormat;
	hr = frame->GetPixelFormat(&pixelFormat);
	if (FAILED(hr))
		return hr;

	WICPixelFormatGUID convertGUID;
	memcpy(&convertGUID, &pixelFormat, sizeof(WICPixelFormatGUID));

	size_t bpp = 0;

	DXGI_FORMAT format = _WICToDXGI(pixelFormat);
	if (format == DXGI_FORMAT_UNKNOWN) {
		for (size_t i = 0; i < _countof(g_WICConvert); ++i) {
			if (memcmp(&g_WICConvert[i].source, &pixelFormat, sizeof(WICPixelFormatGUID)) == 0) {
				memcpy(&convertGUID, &g_WICConvert[i].target, sizeof(WICPixelFormatGUID));

				format = _WICToDXGI(g_WICConvert[i].target);
				assert(format != DXGI_FORMAT_UNKNOWN);
				bpp = _WICBitsPerPixel(convertGUID);
				break;
			}
		}

		if (format == DXGI_FORMAT_UNKNOWN)
			return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
	} else {
		bpp = _WICBitsPerPixel(pixelFormat);
	}

	if (!bpp)
		return E_FAIL;

	// Verify our target format is supported by the current device
	// (handles WDDM 1.0 or WDDM 1.1 device driver cases as well as DirectX 11.0 Runtime without 16bpp format support)
	UINT support = 0;
	hr = d3dDevice->CheckFormatSupport(format, &support);
	if (FAILED(hr) || !(support & D3D11_FORMAT_SUPPORT_TEXTURE2D)) {
		// Fallback to RGBA 32-bit format which is supported by all devices
		memcpy(&convertGUID, &GUID_WICPixelFormat32bppRGBA, sizeof(WICPixelFormatGUID));
		format = DXGI_FORMAT_R8G8B8A8_UNORM;
		bpp = 32;
	}

	// Allocate temporary memory for image
	size_t rowPitch = (twidth * bpp + 7) / 8;
	size_t imageSize = rowPitch * theight;

	std::unique_ptr<uint8_t[]> temp(new uint8_t[imageSize]);

	// Load image data
	if (memcmp(&convertGUID, &pixelFormat, sizeof(GUID)) == 0
		&& twidth == width
		&& theight == height) {
		// No format conversion or resize needed
		hr = frame->CopyPixels(0, static_cast<UINT>(rowPitch), static_cast<UINT>(imageSize), temp.get());
		if (FAILED(hr))
			return hr;
	} else if (twidth != width || theight != height) {
		// Resize
		IWICImagingFactory* pWIC = _GetWIC();
		if (!pWIC)
			return E_NOINTERFACE;

		ScopedObject<IWICBitmapScaler> scaler;
		hr = pWIC->CreateBitmapScaler(&scaler);
		if (FAILED(hr))
			return hr;

		hr = scaler->Initialize(frame, twidth, theight, WICBitmapInterpolationModeFant);
		if (FAILED(hr))
			return hr;

		WICPixelFormatGUID pfScaler;
		hr = scaler->GetPixelFormat(&pfScaler);
		if (FAILED(hr))
			return hr;

		if (memcmp(&convertGUID, &pfScaler, sizeof(GUID)) == 0) {
			// No format conversion needed
			hr = scaler->CopyPixels(0, static_cast<UINT>(rowPitch), static_cast<UINT>(imageSize), temp.get());
			if (FAILED(hr))
				return hr;
		} else {
			ScopedObject<IWICFormatConverter> FC;
			hr = pWIC->CreateFormatConverter(&FC);
			if (FAILED(hr))
				return hr;

			hr = FC->Initialize(scaler.Get(), convertGUID, WICBitmapDitherTypeErrorDiffusion, 0, 0, WICBitmapPaletteTypeCustom);
			if (FAILED(hr))
				return hr;

			hr = FC->CopyPixels(0, static_cast<UINT>(rowPitch), static_cast<UINT>(imageSize), temp.get());
			if (FAILED(hr))
				return hr;
		}
	} else {
		// Format conversion but no resize
		IWICImagingFactory* pWIC = _GetWIC();
		if (!pWIC)
			return E_NOINTERFACE;

		ScopedObject<IWICFormatConverter> FC;
		hr = pWIC->CreateFormatConverter(&FC);
		if (FAILED(hr))
			return hr;

		hr = FC->Initialize(frame, convertGUID, WICBitmapDitherTypeErrorDiffusion, 0, 0, WICBitmapPaletteTypeCustom);
		if (FAILED(hr))
			return hr;

		hr = FC->CopyPixels(0, static_cast<UINT>(rowPitch), static_cast<UINT>(imageSize), temp.get());
		if (FAILED(hr))
			return hr;
	}

	// See if format is supported for auto-gen mipmaps (varies by feature level)
	bool autogen = false;
	if (d3dContext != 0 && textureView != 0) // Must have context and shader-view to auto generate mipmaps
	{
		UINT fmtSupport = 0;
		hr = d3dDevice->CheckFormatSupport(format, &fmtSupport);
		if (SUCCEEDED(hr) && (fmtSupport & D3D11_FORMAT_SUPPORT_MIP_AUTOGEN)) {
			autogen = true;
		}
	}

	// Create texture
	D3D11_TEXTURE2D_DESC desc;
	desc.Width = twidth;
	desc.Height = theight;
	desc.MipLevels = (autogen) ? 0 : 1;
	desc.ArraySize = 1;
	desc.Format = format;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = (autogen) ? (D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET) : (D3D11_BIND_SHADER_RESOURCE);
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = (autogen) ? D3D11_RESOURCE_MISC_GENERATE_MIPS : 0;

	D3D11_SUBRESOURCE_DATA initData;
	initData.pSysMem = temp.get();
	initData.SysMemPitch = static_cast<UINT>(rowPitch);
	initData.SysMemSlicePitch = static_cast<UINT>(imageSize);

	ID3D11Texture2D* tex = nullptr;
	hr = d3dDevice->CreateTexture2D(&desc, (autogen) ? nullptr : &initData, &tex);
	if (SUCCEEDED(hr) && tex != 0) {
		if (textureView != 0) {
			D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
			memset(&SRVDesc, 0, sizeof(SRVDesc));
			SRVDesc.Format = format;
			SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
			SRVDesc.Texture2D.MipLevels = (autogen) ? -1 : 1;

			hr = d3dDevice->CreateShaderResourceView(tex, &SRVDesc, textureView);
			if (FAILED(hr)) {
				tex->Release();
				return hr;
			}

			if (autogen) {
				assert(d3dContext != 0);
				d3dContext->UpdateSubresource(tex, 0, nullptr, temp.get(), static_cast<UINT>(rowPitch), static_cast<UINT>(imageSize));
				d3dContext->GenerateMips(*textureView);
			}
		}

		if (texture != 0) {
			*texture = tex;
		} else {
#if defined(_DEBUG) || defined(PROFILE)
			tex->SetPrivateData(WKPDID_D3DDebugObjectName,
				sizeof("WICTextureLoader") - 1,
				"WICTextureLoader"
			);
#endif
			tex->Release();
		}
	}

	return hr;
}

//--------------------------------------------------------------------------------------
HRESULT CreateWICTextureFromMemory(_In_ ID3D11Device* d3dDevice,
	_In_opt_ ID3D11DeviceContext* d3dContext,
	_In_bytecount_(wicDataSize) const uint8_t* wicData,
	_In_ size_t wicDataSize,
	_Out_opt_ ID3D11Resource** texture,
	_Out_opt_ ID3D11ShaderResourceView** textureView,
	_In_ size_t maxsize
) {
	if (!d3dDevice || !wicData || (!texture && !textureView)) {
		return E_INVALIDARG;
	}

	if (!wicDataSize) {
		return E_FAIL;
	}

#ifdef _M_AMD64
	if (wicDataSize > 0xFFFFFFFF)
		return HRESULT_FROM_WIN32(ERROR_FILE_TOO_LARGE);
#endif

	IWICImagingFactory* pWIC = _GetWIC();
	if (!pWIC)
		return E_NOINTERFACE;

	// Create input stream for memory
	ScopedObject<IWICStream> stream;
	HRESULT hr = pWIC->CreateStream(&stream);
	if (FAILED(hr))
		return hr;

	hr = stream->InitializeFromMemory(const_cast<uint8_t*>(wicData), static_cast<DWORD>(wicDataSize));
	if (FAILED(hr))
		return hr;

	// Initialize WIC
	ScopedObject<IWICBitmapDecoder> decoder;
	hr = pWIC->CreateDecoderFromStream(stream.Get(), 0, WICDecodeMetadataCacheOnDemand, &decoder);
	if (FAILED(hr))
		return hr;

	ScopedObject<IWICBitmapFrameDecode> frame;
	hr = decoder->GetFrame(0, &frame);
	if (FAILED(hr))
		return hr;

	hr = CreateTextureFromWIC(d3dDevice, d3dContext, frame.Get(), texture, textureView, maxsize);
	if (FAILED(hr))
		return hr;

#if defined(_DEBUG) || defined(PROFILE)
	if (texture != 0 && *texture != 0) {
		(*texture)->SetPrivateData(WKPDID_D3DDebugObjectName,
			sizeof("WICTextureLoader") - 1,
			"WICTextureLoader"
		);
	}

	if (textureView != 0 && *textureView != 0) {
		(*textureView)->SetPrivateData(WKPDID_D3DDebugObjectName,
			sizeof("WICTextureLoader") - 1,
			"WICTextureLoader"
		);
	}
#endif

	return hr;
}

//--------------------------------------------------------------------------------------
HRESULT CreateWICTextureFromFile(_In_ ID3D11Device* d3dDevice,
	_In_opt_ ID3D11DeviceContext* d3dContext,
	_In_z_ const wchar_t* fileName,
	_Out_opt_ ID3D11Resource** texture,
	_Out_opt_ ID3D11ShaderResourceView** textureView,
	_In_ size_t maxsize) {
	if (!d3dDevice || !fileName || (!texture && !textureView)) {
		return E_INVALIDARG;
	}

	IWICImagingFactory* pWIC = _GetWIC();
	if (!pWIC)
		return E_NOINTERFACE;

	// Initialize WIC
	ScopedObject<IWICBitmapDecoder> decoder;
	HRESULT hr = pWIC->CreateDecoderFromFilename(fileName, 0, GENERIC_READ, WICDecodeMetadataCacheOnDemand, &decoder);
	if (FAILED(hr))
		return hr;

	ScopedObject<IWICBitmapFrameDecode> frame;
	hr = decoder->GetFrame(0, &frame);
	if (FAILED(hr))
		return hr;

	hr = CreateTextureFromWIC(d3dDevice, d3dContext, frame.Get(), texture, textureView, maxsize);
	if (FAILED(hr))
		return hr;

#if defined(_DEBUG) || defined(PROFILE)
	if (texture != 0 || textureView != 0) {
		CHAR strFileA[MAX_PATH];
		WideCharToMultiByte(CP_ACP,
			WC_NO_BEST_FIT_CHARS,
			fileName,
			-1,
			strFileA,
			MAX_PATH,
			nullptr,
			FALSE
		);
		const CHAR* pstrName = strrchr(strFileA, '\\');
		if (!pstrName) {
			pstrName = strFileA;
		} else {
			pstrName++;
		}

		if (texture != 0 && *texture != 0) {
			(*texture)->SetPrivateData(WKPDID_D3DDebugObjectName,
				static_cast<UINT>(strnlen_s(pstrName, MAX_PATH)),
				pstrName
			);
		}

		if (textureView != 0 && *textureView != 0) {
			(*textureView)->SetPrivateData(WKPDID_D3DDebugObjectName,
				static_cast<UINT>(strnlen_s(pstrName, MAX_PATH)),
				pstrName
			);
		}
	}
#endif

	return hr;
}

