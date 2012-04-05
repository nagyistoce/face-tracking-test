#ifndef __RENDER_HPP__
# define __RENDER_HPP__

# include "subsystem.hpp"

# include <opencv2/core/core.hpp>
# include <Windows.h>
# include <d3dx9.h>
# include <vector>

template <typename T>
struct COM_ptr
{
    COM_ptr()
        : _ptr(NULL)
    {
    }

    COM_ptr(const T *ptr)
        : _ptr(ptr)
    {
    }

    ~COM_ptr()
    {
        if (_ptr)
        {
            _ptr->Release();
            _ptr = NULL;
        }
    }

    T * & operator()()
    {
        return _ptr;
    }

    COM_ptr<T> & operator=(T *other_ptr)
    {
        _ptr = other_ptr;
        return *this;
    }

    COM_ptr<T> & operator=(const COM_ptr<T> &other_com_ptr)
    {
        _ptr = other_com_ptr._ptr;
        _ptr->AddRef();
        return *this;
    }

private:
    T *_ptr;
};

class Render : public Subsystem
{
public:
  
    void render_frame();

    ~Render();
private:
    friend class Master;
    explicit Render(Master *master);

    HRESULT init_d3d();
    HRESULT init_geometry();
    void setup_matrices();
    void setup_lights();
    void mark(cv::Mat &frame);

    WNDCLASSEX _wc;
    HWND _hwnd;

    int _width;
    int _height;

    COM_ptr<IDirect3D9> _pD3D; 
    COM_ptr<IDirect3DDevice9> _pd3dDevice;

    COM_ptr<ID3DXMesh> _pMesh; 
	std::vector<D3DMATERIAL9> _MeshMaterials; 
    std::vector<COM_ptr<IDirect3DTexture9> > _MeshTextures;

    COM_ptr<IDirect3DTexture9> _pDefaultTex; 

    COM_ptr<IDirect3DTexture9> _bground;
    COM_ptr<ID3DXSprite> _d3dDrawMgr;
};


#endif //__RENDER_HPP__
