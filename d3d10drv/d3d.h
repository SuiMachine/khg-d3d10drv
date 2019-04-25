/**
\file d3d.h
*/

#pragma once
#include <D3D10.h>
//#include <D3DX10.h>
#include "vertexformats.h"
#include "shader.h"

class D3D
{
private:
	static int createRenderTargetViews();
	static int findAALevel();		
	static bool initShaders();
	
public:
	enum ShaderName {SHADER_TILE,SHADER_GOURAUDPOLYGON,SHADER_COMPLEXSURFACE,SHADER_FOGSURFACE,SHADER_FIRSTPASS,SHADER_HDR,SHADER_FINALPASS,DUMMY_NUM_SHADERS};
	

	/**
	Custom flags to set render state from renderer interface.
	*/
	//enum Polyflag {};

	/** Options, some user configurable */
	static struct Options
	{
		int samples; /**< Number of MSAA samples */
		int VSync; /**< VSync on/off */
		int aniso; /**< Anisotropic filtering levels */
		int LODBias; /**< Mipmap LOD bias */
		int POM; /**< Parallax occlusion mapping */
		int bumpMapping; /**<Bumpmapping */
		int alphaToCoverage; /**< Alpha to coverage support */
		int classicLighting; /**< Lighting that matches old renderers */
		int simulateMultipassTexturing; /**< Simulate look of multi-pass world texturing */
	};
	
	/**@name API initialization/upkeep */
	//@{
	static int init(HWND hwnd,D3D::Options &createOptions);
	static void uninit();
	static int resize(int X, int Y, bool fullScreen);
	static ID3D10Device *getDevice();
	static DXGI_FORMAT getDrawPassFormat();
	//@}
	
	/**@name Setup/clear frame */
	//@{
	static void newFrame(float time);
	//@}

	/**@name Prepare and render buffers */
	//@{
	static void render();
	static void postprocess();
	static void present();
	//@}

	
	
	/**@name Set state (projection, flags) */
	//@{
	static void switchToShader(int index);
	static Shader* getShader(int index);
	static void setViewPort(int X, int Y, int left, int top);	
	//@}
		
	/**@name Misc */
	//@{
	static void flash(Vec4 &color);
	static TCHAR *getModes();
	static void getScreenshot(Vec4_byte* buf);
	static void setBrightness(float brightness);
	//@}
};