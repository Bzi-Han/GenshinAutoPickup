#ifndef DXGI_RECORDER_H // !DXGI_RECORDER_H
#define DXGI_RECORDER_H

#include <Windows.h>
#include <wrl.h>
#include <d3d11.h>
#include <dxgi1_2.h>

#include <memory>
#include <vector>

class DXGIRecorder
{
public:
    virtual ~DXGIRecorder() {}
    DXGIRecorder(size_t maxTryAcquireNextFrame = 5)
        : m_maxTryAcquireNextFrame(maxTryAcquireNextFrame)
    {
        if (S_OK != D3D11CreateDevice(
                        nullptr, // Adapter
                        D3D_DRIVER_TYPE_HARDWARE,
                        nullptr, // Module
                        D3D11_CREATE_DEVICE_BGRA_SUPPORT,
                        nullptr,
                        0, // Highest available feature level
                        D3D11_SDK_VERSION,
                        &m_d3dDevice,
                        nullptr, // Actual feature level
                        &m_d3dDeviceContext))
        {
            throw std::exception("Failed to create d3d11 device");
        }
        if (S_OK != m_d3dDevice.As(&m_dxgiDevice))
            throw std::exception("Failed to query interface IDXGIDevice");
        if (S_OK != m_dxgiDevice->GetParent(__uuidof(IDXGIAdapter), reinterpret_cast<void **>(m_dxgiAdapter.GetAddressOf())))
            throw std::exception("Failed to query interface IDXGIAdapter");

        for (UINT i = 0; DXGI_ERROR_NOT_FOUND != m_dxgiAdapter->EnumOutputs(i, m_dxgiOutput.ReleaseAndGetAddressOf()); ++i)
        {
            if (S_OK == m_dxgiOutput.As(&m_dxgiOutput1))
                break;
        }
        if (nullptr == m_dxgiOutput1)
            throw std::exception("Failed to query interface IDXGIOutput1");

        if (S_OK != m_dxgiOutput1->DuplicateOutput(m_d3dDevice.Get(), m_dxgiOutputDuplication.GetAddressOf()))
            throw std::exception("Failed to duplication output device");
    }

    size_t GetNextFrameData(void *buffer, size_t bufferSize)
    {
        HRESULT handleResult;

        for (size_t i = 0; i < m_maxTryAcquireNextFrame; ++i)
        {
            m_dxgiOutputDuplication->ReleaseFrame();
            handleResult = m_dxgiOutputDuplication->AcquireNextFrame(500, &m_dxgiOutputFrameInfo, m_dxgiResource.ReleaseAndGetAddressOf());
            if (S_OK == handleResult)
            {
                if (0 == m_dxgiOutputFrameInfo.LastPresentTime.QuadPart)
                    continue;
                else
                    break;
            }
        }
        if (S_OK != handleResult || nullptr == m_dxgiResource)
        {
            // std::cout << "[-] IDXGIOutputDuplication::AcquireNextFrame failed: " << handleResult << std::endl;
            return 0;
        }

        if (S_OK != m_dxgiResource->QueryInterface(
                        __uuidof(ID3D11Texture2D),
                        reinterpret_cast<void **>(m_desktopTexture.ReleaseAndGetAddressOf())))
        {
            // std::cout << "[-] Failed to query interface ID3D11Texture2D" << std::endl;
            return 0;
        }

        m_desktopTexture->GetDesc(&m_d3d11TextureDesc);
        m_d3d11TextureDesc.BindFlags = 0;
        m_d3d11TextureDesc.MiscFlags = 0;
        m_d3d11TextureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        m_d3d11TextureDesc.Usage = D3D11_USAGE_STAGING;
        handleResult = m_d3dDevice->CreateTexture2D(&m_d3d11TextureDesc, nullptr, m_desktopTextureCache.ReleaseAndGetAddressOf());
        if (S_OK != handleResult)
        {
            // std::cout << "[-] Failed to create texture2d" << std::endl;
            return 0;
        }
        m_d3dDeviceContext->CopyResource(m_desktopTextureCache.Get(), m_desktopTexture.Get());

        handleResult = m_d3dDeviceContext->Map(m_desktopTextureCache.Get(), 0, D3D11_MAP_READ, 0, &m_d3d11MappedSubResource);
        if (S_OK != handleResult)
        {
            // std::cout << "[-] Failed to mapping resource: " << handleResult << std::endl;
            return 0;
        }
        size_t copySize = m_d3d11TextureDesc.Height * m_d3d11MappedSubResource.RowPitch > bufferSize ? bufferSize : m_d3d11TextureDesc.Height * m_d3d11MappedSubResource.RowPitch;
        memcpy(
            buffer,
            m_d3d11MappedSubResource.pData,
            copySize);
        m_d3dDeviceContext->Unmap(m_desktopTextureCache.Get(), 0);

        return copySize;
    }

    const std::vector<uint8_t> &GetNextFrameData()
    {
        if (nullptr == m_frameDataCache)
        {
            m_frameDataCache = std::make_unique<std::vector<uint8_t>>(
                ::GetSystemMetrics(SM_CXSCREEN) * ::GetSystemMetrics(SM_CYSCREEN) * 4,
                0);
        }

        auto frameSize = GetNextFrameData(m_frameDataCache->data(), m_frameDataCache->capacity());
        m_frameDataCache->resize(frameSize);

        return *m_frameDataCache;
    }

private:
    Microsoft::WRL::ComPtr<ID3D11Device> m_d3dDevice;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_d3dDeviceContext;
    Microsoft::WRL::ComPtr<IDXGIDevice> m_dxgiDevice;
    Microsoft::WRL::ComPtr<IDXGIAdapter> m_dxgiAdapter;
    Microsoft::WRL::ComPtr<IDXGIOutput> m_dxgiOutput;
    Microsoft::WRL::ComPtr<IDXGIOutput1> m_dxgiOutput1;
    Microsoft::WRL::ComPtr<IDXGIOutputDuplication> m_dxgiOutputDuplication;

    DXGI_OUTDUPL_FRAME_INFO m_dxgiOutputFrameInfo;
    Microsoft::WRL::ComPtr<IDXGIResource> m_dxgiResource;
    D3D11_TEXTURE2D_DESC m_d3d11TextureDesc;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> m_desktopTexture;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> m_desktopTextureCache;
    D3D11_MAPPED_SUBRESOURCE m_d3d11MappedSubResource;

    std::unique_ptr<std::vector<uint8_t>> m_frameDataCache;

    size_t m_maxTryAcquireNextFrame;
};

#endif // !DXGI_RECORDER_H