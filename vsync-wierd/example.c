/*

If you are using MSVC, you can compile with:
cl example.c

*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11_1.h>
#include <dxgi1_3.h>
#include <d3dcompiler.h>

#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "d3dcompiler.lib")

#define alen(x) (sizeof(x)/sizeof((x)[0]))

struct Vertex2d
{
    float x;
    float y;
    uint32_t color;
};

struct ShaderConstants
{
    int32_t width;
    int32_t height;
};

static const char *SHADER_SOURCE_VERTEX =
"cbuffer constants : register(b0)\n"
"{\n"
"    int width;\n"
"    int height;\n"
"};\n"
"\n"
"struct vs_in\n"
"{\n"
"    float2 position : POSITION;\n"
"    float4 color    : COLOR;\n"
"};\n"
"\n"
"struct vs_out\n"
"{\n"
"    float4 position : SV_POSITION;\n" // I spent 30 minutes trying to debug this shit, just to realize that you have to call this SV_POSITION, not just POSITION. Nice
"    float4 color : COLOR;\n"
"};\n"
"\n"
"vs_out main(vs_in input)\n"
"{\n"
"    float2 position = (input.position / float2(width, height))*2.0f - 1.0f;\n"
"    position.y = -position.y;\n"
"\n"
"    vs_out output;\n"
"    output.position = float4(position, 0.0f, 1.0f);\n"
"    output.color = float4(input.color.bgr, 1.0f);\n"
"    return(output);\n"
"}\n";

static const char *SHADER_SOURCE_PIXEL =
"struct vs_out\n"
"{\n"
"    float4 position : SV_POSITION;\n"
"    float4 color : COLOR;\n"
"};\n"

"float4 main(vs_out input) : SV_TARGET\n"
"{\n"
"    return(input.color);\n"
"}\n";


enum {
    MAX_VERTICES = 4 * 1024,
    MAX_INDICES  = 6 * 1024,
};


struct Context
{
    int width;
    int height;

    float dt[60]; // duration in seconds of the last 100 frames
    int dti;

    HWND window_handle;
    WINDOWPLACEMENT window_placement_before_fullscreen;

    ID3D11Device *dev;
    ID3D11DeviceContext1 *devctx;

    IDXGISwapChain *swap;
    HANDLE swap_waitable_object;
    int backbuffer_width, backbuffer_height;
    ID3D11RenderTargetView *backbuffer_view;
    ID3D11RasterizerState *raster_state;
    ID3D11BlendState *blend_state;

    ID3D11VertexShader *shader_2d_vertex;
    ID3D11PixelShader *shader_2d_pixel;
    ID3D11InputLayout *input_layout_2d;


    ID3D11Buffer *vertex_buffer;
    struct Vertex2d *mapped_vertex_buffer;
    int vertex_count;

    ID3D11Buffer *index_buffer;
    uint32_t *mapped_index_buffer;
    int index_count;

    ID3D11Buffer *constants_buffer;
};

static struct Context g_ctx;

static LRESULT CALLBACK my_window_proc(HWND, UINT, WPARAM, LPARAM);

static void renderer_create();
static void renderer_destroy();
static void renderer_redraw();
static void renderer_issue_draw();

static void draw_rect(float x0, float y0, float x1, float y1, uint32_t color);


static void d3d_assert(char *function, HRESULT res, char *file, int line);
#define d3d_call(f, a) d3d_assert(#f, f a, __FILE__, __LINE__)

int main()
{
    WNDCLASSA my_class = {0};
    my_class.lpfnWndProc = my_window_proc;
    my_class.hInstance = GetModuleHandleA(0);
    my_class.lpszClassName = "mortens_cool_window_class";
    my_class.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    assert(RegisterClassA(&my_class));

    HWND handle = CreateWindowA(my_class.lpszClassName, "My first Metal rendering project",
                                WS_OVERLAPPEDWINDOW,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                640, 480,
                                0, 0, my_class.hInstance, 0);
    assert(handle);

    int show_cmd = SW_SHOWNORMAL;
    STARTUPINFOA startup_info;
    startup_info.cb = sizeof(startup_info);
    GetStartupInfoA(&startup_info);
    if (startup_info.dwFlags & STARTF_USESHOWWINDOW) {
        show_cmd = startup_info.wShowWindow;
    }
    ShowWindow(handle, show_cmd);


    LARGE_INTEGER timer_frequency;
    QueryPerformanceFrequency(&timer_frequency);

    LARGE_INTEGER previous;
    QueryPerformanceCounter(&previous);

    enum { FRAMERATE_MEASURE_INTERVAL = 5 };
    int nonblocking_frames_counter = 0;

    while (1) {
        MSG message = {0};
        while (PeekMessageA(&message, 0, 0, 0, PM_REMOVE)) {
            if (message.message == WM_QUIT) {
                return(0);
            }
            TranslateMessage(&message);
            DispatchMessageA(&message);
        }

        renderer_redraw();

        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        float dt = ((float) (now.QuadPart - previous.QuadPart)) / ((float) timer_frequency.QuadPart);
        previous = now;

        g_ctx.dti = (g_ctx.dti + 1) % alen(g_ctx.dt);
        g_ctx.dt[g_ctx.dti] = dt;
    }

    return(0);
}

static void
redraw()
{
    float m = 1.5f;
    float s = (((float) g_ctx.height) - 2.0f*m) / 0.03f; // window height is 30ms
    float w = ((float) g_ctx.width) / ((float) alen(g_ctx.dt));
    float x = 0.0f;

    for (int i = 0; i < alen(g_ctx.dt); ++i) {
        uint32_t color = 0x3abaff;
        float h = ((float) g_ctx.dt[i]) * s;
        draw_rect(x + m, m, x + w - m, m + h, color);
        x += w;
    }

    float framerates[] = { 144.0f, 120.0f, 72.0f, 60.0f, 48.0f };
    for (int i = 0; i < alen(framerates); ++i) {
        float h = (1.0f / framerates[i])*s;
        draw_rect(0.0f, m + h - 1.0f, g_ctx.width, m + h + 1.0f, 0x000000);
    }
}

static int
is_in_fullscreen()
{
    LONG style = GetWindowLongW(g_ctx.window_handle, GWL_STYLE);
    return(!!(style & WS_POPUP));
}

static void
set_size_to_monitor_size()
{
    HMONITOR monitor = MonitorFromWindow(g_ctx.window_handle, MONITOR_DEFAULTTONEAREST);
    MONITORINFO monitor_info = {0};
    monitor_info.cbSize = sizeof(monitor_info);
    if (!GetMonitorInfoA(monitor, &monitor_info)) {
        DWORD error = GetLastError();
        printf("Window: Couldn't get monitor size: %xh\n", error);
        return;
    }

    int x = monitor_info.rcMonitor.left;
    int y = monitor_info.rcMonitor.top;
    int width = monitor_info.rcMonitor.right - x;
    int height = monitor_info.rcMonitor.bottom - y;

    LONG style = GetWindowLongW(g_ctx.window_handle, GWL_STYLE);
    style &= ~WS_OVERLAPPEDWINDOW;
    style |= WS_POPUP;
    SetWindowLongW(g_ctx.window_handle, GWL_STYLE, style);
    SetWindowPos(g_ctx.window_handle, HWND_TOP, x, y, width, height, SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
}

static void
toggle_fullscreen()
{
    LONG style = GetWindowLongW(g_ctx.window_handle, GWL_STYLE);
    if (!(style & WS_POPUP)) {
        // Enter fullscreen mode
        if (!GetWindowPlacement(g_ctx.window_handle, &g_ctx.window_placement_before_fullscreen)) {
            DWORD error = GetLastError();
            printf("Window: Couldn't get window placement: %xh\n", error);
            return;
        }

        set_size_to_monitor_size();
    } else {
        // Exit fullscreen mode
        style |= WS_OVERLAPPEDWINDOW;
        style &= ~WS_POPUP;
        SetWindowLongW(g_ctx.window_handle, GWL_STYLE, style);
        SetWindowPlacement(g_ctx.window_handle, &g_ctx.window_placement_before_fullscreen);
        SetWindowPos(g_ctx.window_handle, 0, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }
}

static LRESULT CALLBACK
my_window_proc(HWND handle, UINT message, WPARAM w, LPARAM l)
{
    switch (message) {

    case WM_CREATE:
        g_ctx.window_handle = handle;
        renderer_create();
        return(0);

    case WM_CLOSE:
        DestroyWindow(handle);
        return(0);

    case WM_KEYDOWN:
        if (w == VK_F11) {
            toggle_fullscreen();
        }
        return(0);

    case WM_DESTROY:
        renderer_destroy();
        PostQuitMessage(0);
        return(0);

    case WM_PAINT: {
        renderer_redraw();
    } break;

    case WM_SIZE: {
        RECT rect;
        GetClientRect(handle, &rect);
        g_ctx.width = rect.right - rect.left;
        g_ctx.height = rect.bottom - rect.top;
        if (is_in_fullscreen()) {
            set_size_to_monitor_size();
        }
        return(0);
    }

    }
    return(DefWindowProcA(handle, message, w, l));
}


static struct Vertex2d *
push_vertices(int vertex_count, ...)
{
    uint32_t indices[64];
    int index_count = 0;
    va_list args;
    va_start(args, vertex_count);
    int index = -1;
    while (1) {
        index = va_arg(args, int);
        if (index == -1 || index_count >= (sizeof(indices)/sizeof(indices[0]))) break;
        indices[index_count++] = index;
    }
    assert(index == -1);
    va_end(args);

    if (g_ctx.vertex_count + vertex_count > MAX_VERTICES || g_ctx.index_count + index_count > MAX_INDICES) {
        renderer_issue_draw();
    }

    if (!g_ctx.mapped_vertex_buffer) {
        assert(g_ctx.vertex_count == 0 && g_ctx.index_count == 0 && !g_ctx.mapped_index_buffer);

        D3D11_MAPPED_SUBRESOURCE vertex_mapping, index_mapping;
        d3d_call(ID3D11DeviceContext1_Map, (g_ctx.devctx, (ID3D11Resource *) g_ctx.vertex_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &vertex_mapping));
        d3d_call(ID3D11DeviceContext1_Map, (g_ctx.devctx, (ID3D11Resource *) g_ctx.index_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &index_mapping));
        g_ctx.mapped_vertex_buffer = vertex_mapping.pData;
        g_ctx.mapped_index_buffer = index_mapping.pData;
    }

    for (int i = 0; i < index_count; ++i) {
        g_ctx.mapped_index_buffer[g_ctx.index_count++] = indices[i] + g_ctx.vertex_count;
    }

    struct Vertex2d *vertices = g_ctx.mapped_vertex_buffer + g_ctx.vertex_count;
    g_ctx.vertex_count += vertex_count;

    return(vertices);
}

static void
draw_rect(float x0, float y0, float x1, float y1, uint32_t color)
{
    struct Vertex2d *v = push_vertices(4, 0, 1, 2, 1, 3, 2, -1);
    v[0] = (struct Vertex2d) { .x = x0, .y = y0, .color = color };
    v[1] = (struct Vertex2d) { .x = x1, .y = y0, .color = color };
    v[2] = (struct Vertex2d) { .x = x0, .y = y1, .color = color };
    v[3] = (struct Vertex2d) { .x = x1, .y = y1, .color = color };
}

static void
renderer_create()
{
    UINT dev_flags = D3D11_CREATE_DEVICE_SINGLETHREADED;
    D3D_FEATURE_LEVEL feature_levels[] = { D3D_FEATURE_LEVEL_11_0 };
    ID3D11DeviceContext *devctx0;
    d3d_call(D3D11CreateDevice, (0, D3D_DRIVER_TYPE_HARDWARE, 0, dev_flags, feature_levels, 1, D3D11_SDK_VERSION, &g_ctx.dev, 0, &devctx0));
    d3d_call(ID3D11DeviceContext_QueryInterface, (devctx0, &IID_ID3D11DeviceContext1, &g_ctx.devctx));
    ID3D11DeviceContext_Release(devctx0);

    IDXGIFactory *dxgi_factory;
    d3d_call(CreateDXGIFactory, (&IID_IDXGIFactory, &dxgi_factory));

    DXGI_SWAP_CHAIN_DESC swap_desc = {
        .BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .BufferDesc.Scaling = DXGI_MODE_SCALING_CENTERED,
        .SampleDesc.Count = 1,
        .SampleDesc.Quality = 0,
        .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
        .OutputWindow = g_ctx.window_handle,
        .Windowed = TRUE,
    };

    // _FLIP_SEQUENTIAL is only supported since windows 8.1
    #if 1
    swap_desc.BufferCount = 2;
    swap_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    swap_desc.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
    HRESULT modern_swap_create_result = IDXGIFactory_CreateSwapChain(dxgi_factory, (IUnknown *) g_ctx.dev, &swap_desc, &g_ctx.swap);
    #else
    HRESULT modern_swap_create_result = 1;
    #endif
    if (modern_swap_create_result == S_OK) {
        IDXGISwapChain2 *swap2 = 0;
        d3d_call(IDXGISwapChain_QueryInterface, (g_ctx.swap, &IID_IDXGISwapChain2, &swap2));
        assert(swap2);
        g_ctx.swap_waitable_object = IDXGISwapChain2_GetFrameLatencyWaitableObject(swap2);
        IDXGISwapChain2_Release(swap2);
    } else {
        printf("Window: Couldn't create modern swapchain (FLIP_SEQUENTIAL + FRAME_LATENCY_WAITABLE_OBJECT), falling back to old method. Error %xh.\n", modern_swap_create_result);
        swap_desc.BufferCount = 2;
        swap_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
        swap_desc.Flags = 0;
        d3d_call(IDXGIFactory_CreateSwapChain, (dxgi_factory, (IUnknown *) g_ctx.dev, &swap_desc, &g_ctx.swap));
    }

    IDXGIFactory_Release(dxgi_factory);

    // Rasterizer and blend state
    D3D11_RASTERIZER_DESC raster_desc = {
        .FillMode = D3D11_FILL_SOLID,
        .CullMode = D3D11_CULL_NONE,
    };
    d3d_call(ID3D11Device_CreateRasterizerState, (g_ctx.dev, &raster_desc, &g_ctx.raster_state));

    D3D11_BLEND_DESC blend_desc = {
        .AlphaToCoverageEnable = FALSE,
        .IndependentBlendEnable = FALSE,
        .RenderTarget[0] = {
            .BlendEnable = TRUE,
            .SrcBlend = D3D11_BLEND_SRC_ALPHA,
            .DestBlend = D3D11_BLEND_INV_SRC_ALPHA,
            .BlendOp = D3D11_BLEND_OP_ADD,
            .SrcBlendAlpha = D3D11_BLEND_ONE,
            .DestBlendAlpha = D3D11_BLEND_ZERO,
            .BlendOpAlpha = D3D11_BLEND_OP_ADD,
            .RenderTargetWriteMask = 0xf,
        },
    };
    d3d_call(ID3D11Device_CreateBlendState, (g_ctx.dev, &blend_desc, &g_ctx.blend_state));


    // Shaders
    ID3D10Blob *vs_blob;
    d3d_call(D3DCompile, (SHADER_SOURCE_VERTEX, strlen(SHADER_SOURCE_VERTEX), 0, 0, 0, "main", "vs_5_0", 0, 0, &vs_blob, 0));
    d3d_call(ID3D11Device_CreateVertexShader, (g_ctx.dev, ID3D10Blob_GetBufferPointer(vs_blob), ID3D10Blob_GetBufferSize(vs_blob), 0, &g_ctx.shader_2d_vertex));

    ID3D10Blob *ps_blob;
    d3d_call(D3DCompile, (SHADER_SOURCE_PIXEL, strlen(SHADER_SOURCE_PIXEL), 0, 0, 0, "main", "ps_5_0", 0, 0, &ps_blob, 0));
    d3d_call(ID3D11Device_CreatePixelShader, (g_ctx.dev, ID3D10Blob_GetBufferPointer(ps_blob), ID3D10Blob_GetBufferSize(ps_blob), 0, &g_ctx.shader_2d_pixel));

    D3D11_INPUT_ELEMENT_DESC input_desc_2d[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(struct Vertex2d, x), D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM,  0, offsetof(struct Vertex2d, color), D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    d3d_call(ID3D11Device_CreateInputLayout, (g_ctx.dev, input_desc_2d, 2, ID3D10Blob_GetBufferPointer(vs_blob), ID3D10Blob_GetBufferSize(vs_blob), &g_ctx.input_layout_2d));

    // Vertex2d and index buffer
    D3D11_BUFFER_DESC vertex_buffer_desc = {
        .ByteWidth = sizeof(struct Vertex2d) * MAX_VERTICES,
        .Usage = D3D11_USAGE_DYNAMIC,
        .BindFlags = D3D11_BIND_VERTEX_BUFFER,
        .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
    };
    d3d_call(ID3D11Device_CreateBuffer, (g_ctx.dev, &vertex_buffer_desc, 0, &g_ctx.vertex_buffer));

    D3D11_BUFFER_DESC index_buffer_desc = {
        .ByteWidth = sizeof(uint32_t) * MAX_INDICES,
        .Usage = D3D11_USAGE_DYNAMIC,
        .BindFlags = D3D11_BIND_INDEX_BUFFER,
        .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
    };
    d3d_call(ID3D11Device_CreateBuffer, (g_ctx.dev, &index_buffer_desc, 0, &g_ctx.index_buffer));

    // Constant buffer
    D3D11_BUFFER_DESC constants_buffer_desc = {
        .ByteWidth = (sizeof(struct ShaderConstants) + 15)&~15,
        .Usage = D3D11_USAGE_DYNAMIC,
        .BindFlags = D3D11_BIND_CONSTANT_BUFFER,
        .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
    };
    d3d_call(ID3D11Device_CreateBuffer, (g_ctx.dev, &constants_buffer_desc, 0, &g_ctx.constants_buffer));
}

static void
renderer_destroy()
{
    ID3D11DeviceContext1_ClearState(g_ctx.devctx);

    ID3D11RasterizerState_Release(g_ctx.raster_state);
    ID3D11BlendState_Release(g_ctx.blend_state);

    ID3D11VertexShader_Release(g_ctx.shader_2d_pixel);
    ID3D11PixelShader_Release(g_ctx.shader_2d_vertex);
    ID3D11InputLayout_Release(g_ctx.input_layout_2d);

    ID3D11Buffer_Release(g_ctx.vertex_buffer);
    ID3D11Buffer_Release(g_ctx.index_buffer);
    ID3D11Buffer_Release(g_ctx.constants_buffer);

    if (g_ctx.backbuffer_view) ID3D11RenderTargetView_Release(g_ctx.backbuffer_view);

    ID3D11DeviceContext1_Release(g_ctx.devctx);
    IDXGISwapChain_Release(g_ctx.swap);
    ID3D11Device_Release(g_ctx.dev);

    g_ctx.dev = 0;
    g_ctx.devctx = 0;
    g_ctx.swap = 0;
    g_ctx.backbuffer_width = 0;
    g_ctx.backbuffer_height = 0;
    g_ctx.backbuffer_view = 0;
    g_ctx.raster_state = 0;
    g_ctx.blend_state = 0;
    g_ctx.shader_2d_vertex = 0;
    g_ctx.shader_2d_pixel = 0;
    g_ctx.input_layout_2d = 0;
    g_ctx.vertex_buffer = 0;
    g_ctx.mapped_vertex_buffer = 0;
    g_ctx.vertex_count = 0;
    g_ctx.index_buffer = 0;
    g_ctx.mapped_index_buffer = 0;
    g_ctx.index_count = 0;
    g_ctx.constants_buffer = 0;
}

static void
renderer_redraw()
{
    if (g_ctx.width == 0 || g_ctx.height == 0) {
        return;
    }

    if (g_ctx.width != g_ctx.backbuffer_width || g_ctx.height != g_ctx.backbuffer_height) {
        g_ctx.backbuffer_width = g_ctx.width;
        g_ctx.backbuffer_height = g_ctx.height;

        ID3D11DeviceContext1_OMSetRenderTargets(g_ctx.devctx, 0, 0, 0);
        if (g_ctx.backbuffer_view) ID3D11RenderTargetView_Release(g_ctx.backbuffer_view);

        UINT swap_flags = 0;
        if (g_ctx.swap_waitable_object) {
            swap_flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
        }
        d3d_call(IDXGISwapChain_ResizeBuffers, (g_ctx.swap, 0, g_ctx.width, g_ctx.height, DXGI_FORMAT_UNKNOWN, swap_flags));

        ID3D11Texture2D *backbuffer_texture;
        d3d_call(IDXGISwapChain_GetBuffer, (g_ctx.swap, 0, &IID_ID3D11Texture2D, &backbuffer_texture));
        d3d_call(ID3D11Device_CreateRenderTargetView, (g_ctx.dev, (ID3D11Resource *) backbuffer_texture, 0, &g_ctx.backbuffer_view));
        ID3D11Texture2D_Release(backbuffer_texture);
    }

    D3D11_MAPPED_SUBRESOURCE constants_mapping;
    d3d_call(ID3D11DeviceContext1_Map, (g_ctx.devctx, (ID3D11Resource *) g_ctx.constants_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &constants_mapping));
    struct ShaderConstants *constants = constants_mapping.pData;
    constants->width = g_ctx.width;
    constants->height = g_ctx.height;
    ID3D11DeviceContext1_Unmap(g_ctx.devctx, (ID3D11Resource *) g_ctx.constants_buffer, 0);


    float clear_color[4] = { 0.96f, 0.96f, 0.95f, 1.0f };
    ID3D11DeviceContext1_ClearRenderTargetView(g_ctx.devctx, g_ctx.backbuffer_view, clear_color);

    ID3D11DeviceContext1_OMSetRenderTargets(g_ctx.devctx, 1, &g_ctx.backbuffer_view, 0);
    float blend_factors[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    ID3D11DeviceContext1_OMSetBlendState(g_ctx.devctx, g_ctx.blend_state, blend_factors, 0xffffffff);

    D3D11_VIEWPORT viewport = { 0.0f, 0.0f, (float) g_ctx.width, (float) g_ctx.height, 0.0f, 1.0f };
    ID3D11DeviceContext1_RSSetViewports(g_ctx.devctx, 1, &viewport);
    ID3D11DeviceContext1_RSSetState(g_ctx.devctx, g_ctx.raster_state);

    ID3D11DeviceContext1_IASetPrimitiveTopology(g_ctx.devctx, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ID3D11DeviceContext1_IASetInputLayout(g_ctx.devctx, g_ctx.input_layout_2d);
    UINT stride = sizeof(struct Vertex2d);
    UINT offset = 0;
    ID3D11DeviceContext1_IASetVertexBuffers(g_ctx.devctx, 0, 1, &g_ctx.vertex_buffer, &stride, &offset);
    ID3D11DeviceContext1_IASetIndexBuffer(g_ctx.devctx, g_ctx.index_buffer, DXGI_FORMAT_R32_UINT, 0);

    ID3D11DeviceContext1_VSSetShader(g_ctx.devctx, g_ctx.shader_2d_vertex, 0, 0);
    ID3D11DeviceContext1_VSSetConstantBuffers(g_ctx.devctx, 0, 1, &g_ctx.constants_buffer);

    ID3D11DeviceContext1_PSSetShader(g_ctx.devctx, g_ctx.shader_2d_pixel, 0, 0);

    redraw();

    renderer_issue_draw();

    UINT sync_interval = 1;
    UINT present_flags = 0;
    d3d_call(IDXGISwapChain_Present, (g_ctx.swap, sync_interval, present_flags));

    ID3D11DeviceContext1_DiscardView(g_ctx.devctx, (ID3D11View *) g_ctx.backbuffer_view);

    if (g_ctx.swap_waitable_object) {
        WaitForSingleObjectEx(g_ctx.swap_waitable_object, INFINITE, TRUE);
    }
}

static void
renderer_issue_draw()
{
    if (g_ctx.mapped_vertex_buffer) {
        ID3D11DeviceContext1_Unmap(g_ctx.devctx, (ID3D11Resource *) g_ctx.index_buffer, 0);
        ID3D11DeviceContext1_Unmap(g_ctx.devctx, (ID3D11Resource *) g_ctx.vertex_buffer, 0);
        g_ctx.mapped_vertex_buffer = 0;
        g_ctx.mapped_index_buffer = 0;
    }

    ID3D11DeviceContext1_DrawIndexed(g_ctx.devctx, g_ctx.index_count, 0, 0);

    g_ctx.vertex_count = 0;
    g_ctx.index_count = 0;
}

static void
d3d_assert(char *function, HRESULT res, char *file, int line)
{
    if (res != S_OK) {
        char *res_message = 0;
        FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, 0, res, MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), (LPSTR) &res_message, 0, 0);
        fprintf(stderr, res_message);
        exit(1);
    }
}