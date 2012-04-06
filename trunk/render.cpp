#include "render.hpp"
#include "capturer.hpp"
#include "detector.hpp"
#include "master.hpp"

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <stdexcept>
#include <algorithm>

extern LRESULT WINAPI MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static bool CopyImageToTexture(UCHAR *pSrc, int xWidth, int yHeight, LPDIRECT3DTEXTURE9 pTex, int xPos, int yPos)
{
    D3DLOCKED_RECT d3drc;
    UCHAR r, g, b;
    UINT *pDest;
    int nRow, nPixel;

    if (FAILED(pTex->LockRect(0, &d3drc, NULL, 0)))
    {
        return false;
    }
    d3drc.Pitch >>= 2;
    for (nRow = 0; nRow < yHeight; nRow++)
    {
        pDest = (UINT*)d3drc.pBits + (nRow + yPos) * d3drc.Pitch + xPos;

        for (nPixel = 0; nPixel < xWidth; nPixel++)
        {
            r = *pSrc++;
            g = *pSrc++;
            b = *pSrc++;
            (*pDest++) = 0xFF000000 | (r << 16) | (g << 8) | b;
        }
    }
    pTex->UnlockRect(0);
    return true;
}

const static wchar_t wnd_class_name[] = L"D3D Test";

Render::Render(Master *master_)
    : Subsystem(master_)
{
    cv::Mat sizing_frame;
    while (sizing_frame.empty())
    {
        sizing_frame = master().subsystem<Capturer>().frame();
        boost::this_thread::interruptible_wait(100u);
    }

    _width = sizing_frame.cols;
    _height = sizing_frame.rows;

    WNDCLASSEX wc =
    {
        sizeof(WNDCLASSEX), CS_CLASSDC, MsgProc, 0L, 0L,
        GetModuleHandle(NULL), NULL, NULL, NULL, NULL,
        wnd_class_name, NULL
    };
    _wc = wc;

    RegisterClassEx(&_wc);

    _hwnd = CreateWindow(wnd_class_name, L"Capture - Face detection",
                         WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, _width, _height,
                         NULL, NULL, _wc.hInstance, NULL);


    if (FAILED(init_d3d()))
    {
        throw std::runtime_error("Unable to initialize Direct3D");
    }

    if (FAILED(init_geometry()))
    {
        throw std::runtime_error("Unable to initialize geometry");
    }

    ShowWindow(_hwnd, SW_SHOWDEFAULT);
    UpdateWindow(_hwnd);
}

Render::~Render()
{
    UnregisterClass(wnd_class_name, _wc.hInstance);
}

HRESULT Render::init_d3d()
{
    _pD3D = Direct3DCreate9(D3D_SDK_VERSION);
    if (!_pD3D())
    {
        return E_FAIL;
    }

    D3DPRESENT_PARAMETERS d3dpp;
    ZeroMemory(&d3dpp, sizeof(d3dpp));
    d3dpp.Windowed = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
    d3dpp.EnableAutoDepthStencil = TRUE;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D16;

    if (FAILED(_pD3D()->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, 
                                    _hwnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, 
                                    &d3dpp, &_pd3dDevice())))
    {
        return E_FAIL;
    }

    if (FAILED(_pd3dDevice()->CreateTexture(1, 1, 1, 0, D3DFMT_A8R8G8B8,
                                            D3DPOOL_MANAGED, &_pDefaultTex(), NULL)))
    {
        return E_FAIL;
    }

    D3DLOCKED_RECT lr;
    if (FAILED(_pDefaultTex()->LockRect(0, &lr, NULL, 0)))
    {
        return E_FAIL;
    }

    *(LPDWORD)lr.pBits = D3DCOLOR_RGBA( 255, 255, 255, 255 );
    if (FAILED(_pDefaultTex()->UnlockRect(0)))
    {
        return E_FAIL;
    }

    if (FAILED(D3DXCreateSprite(_pd3dDevice(), &_d3dDrawMgr())))
    {
        return E_FAIL;
    }

    if (FAILED(_pd3dDevice()->CreateTexture(_width, _height, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &_bground(), NULL)))
    {
        return E_FAIL;
    }

    _pd3dDevice()->SetRenderState(D3DRS_ZENABLE, TRUE);

    return S_OK;
}


HRESULT Render::init_geometry()
{
    HRESULT hr = S_OK;
    COM_ptr<ID3DXBuffer> pD3DXMtrlBuffer;
    COM_ptr<ID3DXMesh> pMeshTemp;

    DWORD NumMaterials = 0L;
    hr = D3DXLoadMeshFromX(L"skullocc.x", D3DXMESH_SYSTEMMEM, _pd3dDevice(),
                           NULL, &pD3DXMtrlBuffer(), NULL,
                           &NumMaterials, &_pMesh());
    if (FAILED(hr))
    {
        return hr;
    }

    D3DXMATERIAL *d3dxMaterials = (D3DXMATERIAL *)pD3DXMtrlBuffer()->GetBufferPointer();
    _MeshMaterials.resize(NumMaterials);
    _MeshTextures.resize(NumMaterials);

    for (size_t i = 0; i < _MeshMaterials.size(); i++)
    {
        _MeshMaterials[i] = d3dxMaterials[i].MatD3D;
        _MeshMaterials[i].Ambient = _MeshMaterials[i].Diffuse;
        _MeshTextures[i] = NULL;

        if(d3dxMaterials[i].pTextureFilename)
        {
            D3DXCreateTextureFromFileA(_pd3dDevice(), d3dxMaterials[i].pTextureFilename, &_MeshTextures[i]());
        }

        if (!_MeshTextures[i]())
        {
            _MeshTextures[i] = _pDefaultTex;
        }
    }

    // remember if there were normals in the file, before possible clone operation
    bool bNormalsInFile = (_pMesh()->GetFVF() & D3DFVF_NORMAL) != 0;

    // force 32 byte vertices
    if (_pMesh()->GetFVF() != (D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1))
    {
        hr = _pMesh()->CloneMeshFVF(_pMesh()->GetOptions(), D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1,
                                    _pd3dDevice(), &pMeshTemp());
        if (FAILED(hr))
        {
            return hr;
        }

        _pMesh = pMeshTemp;
    }

    // Compute normals for the mesh, if not present
    if (!bNormalsInFile)
    {
        D3DXComputeNormals(_pMesh(), NULL);
    }

    return hr;
}


void Render::setup_matrices()
{
    std::vector<cv::Rect> faces = master().subsystem<Detector>().faces();
    if (!faces.empty())
    {
        static cv::Rect prev_face; // face interpolation

        struct SquareCompare
        {
            bool operator ()(const cv::Rect &a, const cv::Rect &b)
            {
                return a.width * a.height < b.width * b.height;
            }
        };
        cv::Rect &f = *std::max_element(faces.begin(), faces.end(), SquareCompare());

        f.x = (f.x + prev_face.x) / 2;
        f.y = (f.y + prev_face.y) / 2;
        f.width = (f.width + prev_face.width) / 2;
        f.height = (f.height + prev_face.height) / 2;

        prev_face = f;

        float z_scale = 0.2f;

        float scale = 0.033f; 
        int x_center = _width / 2;
        int y_center = _height / 2;

        D3DXMATRIXA16 matWorld;
        D3DXMatrixTranslation(&matWorld, ((f.x + f.width / 2) - x_center) * scale, (y_center - (f.y + f.height / 2)) * scale, 0.f);
        _pd3dDevice()->SetTransform(D3DTS_WORLD, &matWorld);

        D3DXVECTOR3 vEyePt(0.0f, 10.0f, -24.0f + (f.width - 160.f) * z_scale);
        D3DXVECTOR3 vLookatPt(0.0f, 3.0f, 0.0f);
        D3DXVECTOR3 vUpVec(0.0f, 1.0f, 0.0f);
        D3DXMATRIXA16 matView;
        D3DXMatrixLookAtLH(&matView, &vEyePt, &vLookatPt, &vUpVec);
        _pd3dDevice()->SetTransform(D3DTS_VIEW, &matView);
    }
    else
    {
        D3DXVECTOR3 vEyePt(0.0f, 10.0f, -24.0f);
        D3DXVECTOR3 vLookatPt(0.0f, 3.0f, 0.0f);
        D3DXVECTOR3 vUpVec(0.0f, 1.0f, 0.0f);
        D3DXMATRIXA16 matView;
        D3DXMatrixLookAtLH(&matView, &vEyePt, &vLookatPt, &vUpVec);
        _pd3dDevice()->SetTransform(D3DTS_VIEW, &matView);
    }

    D3DXMATRIXA16 matProj;
    D3DXMatrixPerspectiveFovLH(&matProj, D3DX_PI / 4, 1.0f, 1.0f, 100.0f);
    _pd3dDevice()->SetTransform(D3DTS_PROJECTION, &matProj);
}


void Render::setup_lights()
{
    D3DXVECTOR3 vecDir;
    D3DLIGHT9 light;
    ZeroMemory(&light, sizeof(D3DLIGHT9));
    light.Type = D3DLIGHT_DIRECTIONAL;
    light.Diffuse.r = 1.0f;
    light.Diffuse.g = 1.0f;
    light.Diffuse.b = 1.0f;
    vecDir = D3DXVECTOR3( 1.0f,
                          1.0f,
                          1.0f );
    D3DXVec3Normalize((D3DXVECTOR3 *)&light.Direction, &vecDir);
    light.Range = 1000.0f;
    _pd3dDevice()->SetLight(0, &light);
    _pd3dDevice()->LightEnable(0, TRUE);
    _pd3dDevice()->SetRenderState(D3DRS_LIGHTING, TRUE);

    _pd3dDevice()->SetRenderState(D3DRS_AMBIENT, 0x00505050);
}

void Render::mark(cv::Mat &frame)
{
    std::vector<cv::Rect> faces = master().subsystem<Detector>().faces();

    for (size_t i = 0; i < faces.size(); i++)
    {
        cv::Point center((int)(faces[i].x + faces[i].width*0.5), (int)(faces[i].y + faces[i].height*0.5));
        cv::ellipse(frame, center, cv::Size( (int)(faces[i].width*0.5), (int)(faces[i].height*0.5)), 0, 0, 360, cv::Scalar( 255, 0, 255 ), 4, 8, 0);
    }
}

void Render::render_frame()
{
    cv::Mat frame = master().subsystem<Capturer>().frame();

    if (!frame.empty())
    { 
        //mark(frame); 

        cv::Mat rgb;
        cvtColor(frame, rgb, CV_BGR2RGB);

        _d3dDrawMgr()->OnResetDevice();

        CopyImageToTexture(rgb.data, _width, _height, _bground(), 0, 0);
    }

    _pd3dDevice()->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_ARGB(0, 0, 0, 0), 1.0f, 0);

    if (FAILED(_pd3dDevice()->BeginScene()))
    {
        return;
    }

    if (!frame.empty())
    { 
        _d3dDrawMgr()->Begin(NULL);
        D3DXVECTOR3 pos(0.f, 0.f, 1.f);
        _d3dDrawMgr()->Draw(_bground(), NULL, NULL, &pos, 0XFFFFFFFF);
        _d3dDrawMgr()->End();
    }

    setup_lights();

    setup_matrices();

    for(size_t i = 0; i < _MeshMaterials.size(); i++)
    {
        _pd3dDevice()->SetMaterial(&_MeshMaterials[i]);
        _pd3dDevice()->SetTexture(0, _MeshTextures[i]());
        _pMesh()->DrawSubset(i);
    }

    _pd3dDevice()->EndScene();

    _pd3dDevice()->Present(NULL, NULL, NULL, NULL);
}