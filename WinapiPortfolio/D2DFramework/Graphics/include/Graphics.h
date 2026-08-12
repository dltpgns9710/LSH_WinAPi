#pragma once

#include <Windows.h>

#include <d3d11.h>
#include <dxgi1_2.h>

#include <d2d1_1.h>
#include <d2d1_1helper.h>

#include <wrl/client.h>
using Microsoft::WRL::ComPtr;

class Graphics
{
public:
	Graphics();
	~Graphics();

	bool Init(HWND hWnd);

	inline void BeginDraw() { deviceContext->BeginDraw(); }
	inline void EndDraw() { deviceContext->EndDraw(); swapChain->Present(1, 0); }
	//inline void Resize(UINT width, UINT height) { deviceContext->Resize(D2D1::SizeU(width, height)); }

	void ClearScreen(float r, float g, float b);
	void DrawCircle(float x, float y, float radius, float r, float g, float b, float a);

	//inline const ComPtr<ID2D1HwndRenderTarget>& GetRenderTarget() {return renderTarget;}
	inline const ComPtr<ID2D1DeviceContext>& GetDeviceContext() { return deviceContext; }
	inline const ComPtr<ID2D1Factory1>& GetFactory() { return factory; }
	inline const ComPtr<ID2D1Device>& GetD2DDevice() { return d2dDevice; }

private:
	// D3D11
	ComPtr<ID3D11Device>        d3dDevice;
	ComPtr<ID3D11DeviceContext> d3dContext;
	ComPtr<IDXGISwapChain1>     swapChain;

	// D2D1
	ComPtr<ID2D1Factory1>       factory;
	ComPtr<ID2D1Device>         d2dDevice;
	ComPtr<ID2D1DeviceContext>  deviceContext;  // used instead of HwndRenderTarget
	ComPtr<ID2D1Bitmap1>        targetBitmap;   // bitmap wrapping the SwapChain buffer

	// brush
	ComPtr<ID2D1SolidColorBrush> brush;
};
