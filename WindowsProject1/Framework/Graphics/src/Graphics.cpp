#include "../include/Graphics.h"

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")

Graphics::Graphics() {}

Graphics::~Graphics() {}

bool Graphics::Init(HWND hWnd)
{
	HRESULT hr;

	// 1. Create D3D11 Device + SwapChain
	UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT; // required for D2D interop
#ifdef _DEBUG
	creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0 };
	D3D_FEATURE_LEVEL featureLevel;

	hr = D3D11CreateDevice(
		nullptr,                        // default adapter
		D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
		creationFlags,
		featureLevels, ARRAYSIZE(featureLevels),
		D3D11_SDK_VERSION,
		d3dDevice.GetAddressOf(),
		&featureLevel,
		d3dContext.GetAddressOf()
	);
	if (FAILED(hr)) return false;

	// DXGI Factory -> SwapChain
	ComPtr<IDXGIDevice1> dxgiDevice;
	hr = d3dDevice.As(&dxgiDevice);
	if (FAILED(hr)) return false;

	ComPtr<IDXGIAdapter> dxgiAdapter;
	hr = dxgiDevice->GetAdapter(dxgiAdapter.GetAddressOf());
	if (FAILED(hr)) return false;

	ComPtr<IDXGIFactory2> dxgiFactory;
	hr = dxgiAdapter->GetParent(IID_PPV_ARGS(dxgiFactory.GetAddressOf()));
	if (FAILED(hr)) return false;

	RECT rc;
	GetClientRect(hWnd, &rc);
	UINT width = rc.right - rc.left;
	UINT height = rc.bottom - rc.top;

	DXGI_SWAP_CHAIN_DESC1 swapDesc = {};
	swapDesc.Width = width;
	swapDesc.Height = height;
	swapDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; // recommended format for D2D
	swapDesc.SampleDesc.Count = 1;
	swapDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapDesc.BufferCount = 2;
	swapDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;

	hr = dxgiFactory->CreateSwapChainForHwnd(
		d3dDevice.Get(), hWnd,
		&swapDesc, nullptr, nullptr,
		swapChain.GetAddressOf()
	);
	if (FAILED(hr)) return false;

	// 2. D2D1 Factory -> Device -> DeviceContext
	D2D1_FACTORY_OPTIONS options = {};
#ifdef _DEBUG
	options.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
#endif

	hr = D2D1CreateFactory(
		D2D1_FACTORY_TYPE_SINGLE_THREADED,
		__uuidof(ID2D1Factory1),
		&options,
		reinterpret_cast<void**>(factory.GetAddressOf())
	);
	if (FAILED(hr)) return false;

	hr = factory->CreateDevice(dxgiDevice.Get(), d2dDevice.GetAddressOf());
	if (FAILED(hr)) return false;

	hr = d2dDevice->CreateDeviceContext(
		D2D1_DEVICE_CONTEXT_OPTIONS_NONE,
		deviceContext.GetAddressOf()
	);
	if (FAILED(hr)) return false;

	// 3. SwapChain back buffer -> D2D1Bitmap1 (bind as render target)
	ComPtr<IDXGISurface> dxgiSurface;
	hr = swapChain->GetBuffer(0, IID_PPV_ARGS(dxgiSurface.GetAddressOf()));
	if (FAILED(hr)) return false;

	D2D1_BITMAP_PROPERTIES1 bitmapProps = D2D1::BitmapProperties1(
		D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
		D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
	);

	hr = deviceContext->CreateBitmapFromDxgiSurface(
		dxgiSurface.Get(),
		&bitmapProps,
		targetBitmap.GetAddressOf()
	);
	if (FAILED(hr)) return false;

	// Set DeviceContext's render target to the back buffer bitmap
	deviceContext->SetTarget(targetBitmap.Get());

	// 4. Create brush
	hr = deviceContext->CreateSolidColorBrush(
		D2D1::ColorF(0, 0, 0, 0),
		brush.GetAddressOf()
	);
	if (FAILED(hr)) return false;

	return true;
}

void Graphics::ClearScreen(float r, float g, float b)
{
	deviceContext->Clear(D2D1::ColorF(r, g, b));
}

void Graphics::DrawCircle(float x, float y, float radius, float r, float g, float b, float a)
{
	brush->SetColor(D2D1::ColorF(r, g, b, a));
	deviceContext->DrawEllipse(D2D1::Ellipse(D2D1::Point2F(x, y), radius, radius), brush.Get(), 3.0f);
}
