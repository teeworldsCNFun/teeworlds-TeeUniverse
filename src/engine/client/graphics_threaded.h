#pragma once

#include <engine/graphics.h>

class CCommandBuffer
{
	class CBuffer
	{
		unsigned char *m_pData;
		unsigned m_Size;
		unsigned m_Used;
	public:
		CBuffer(unsigned BufferSize)
		{
			m_Size = BufferSize;
			m_pData = new unsigned char[m_Size];
			m_Used = 0;
		}

		~CBuffer()
		{
			delete [] m_pData;
			m_pData = 0x0;
			m_Used = 0;
			m_Size = 0;
		}

		void Reset()
		{
			m_Used = 0;
		}

		void *Alloc(unsigned Requested)
		{
			if(Requested + m_Used > m_Size)
				return 0;
			void *pPtr = &m_pData[m_Used];
			m_Used += Requested;
			return pPtr;
		}

		unsigned char *DataPtr() { return m_pData; }
		unsigned DataSize() const { return m_Size; }
		unsigned DataUsed() const { return m_Used; }
	};

public:
	CBuffer m_CmdBuffer;
	CBuffer m_DataBuffer;

	enum
	{
		MAX_TEXTURES=1024*4,
	};

	enum
	{
		// commadn groups
		CMDGROUP_CORE = 0, // commands that everyone has to implement
		CMDGROUP_PLATFORM_OPENGL = 10000, // commands specific to a platform
		CMDGROUP_PLATFORM_SDL = 20000,

		//
		CMD_NOP = CMDGROUP_CORE,

		//
		CMD_RUNBUFFER,

		// syncronization
		CMD_SIGNAL,

		// texture commands
		CMD_TEXTURE_CREATE,
		CMD_TEXTURE_DESTROY,
		CMD_TEXTURE_UPDATE,

		// rendering
		CMD_CLEAR,
		CMD_RENDER,

		// swap
		CMD_SWAP,

		// misc
		CMD_VSYNC,
		CMD_SCREENSHOT,
		CMD_VIDEOMODES,

	};

	enum
	{
		TEXFORMAT_INVALID = 0,
		TEXFORMAT_RGB,
		TEXFORMAT_RGBA,
		TEXFORMAT_ALPHA,

		TEXFLAG_NOMIPMAPS = 1,
		TEXFLAG_COMPRESSED = 2,
		TEXFLAG_QUALITY = 4,
		TEXFLAG_TEXTURE3D = 8,
		TEXFLAG_TEXTURE2D = 16,
	};

	enum
	{
		//
		PRIMTYPE_INVALID = 0,
		PRIMTYPE_LINES,	
		PRIMTYPE_QUADS,
	};

	enum
	{
		BLEND_NONE = 0,
		BLEND_ALPHA,
		BLEND_ADDITIVE,
	};

	struct SPoint { float x, y, z; };
	struct STexCoord { float u, v, i; };
	struct SColor { float r, g, b, a; };

	struct SVertex
	{
		SPoint m_Pos;
		STexCoord m_Tex;
		SColor m_Color;
	};

	struct SCommand
	{
	public:
		SCommand(unsigned Cmd) : m_Cmd(Cmd), m_Size(0) {}
		unsigned m_Cmd;
		unsigned m_Size;
	};

	struct SState
	{
		int m_BlendMode;
		int m_WrapModeU;
		int m_WrapModeV;
		int m_Texture;
		int m_Dimension;
		SPoint m_ScreenTL;
		SPoint m_ScreenBR;

		// clip
		bool m_ClipEnable;
		int m_ClipX;
		int m_ClipY;
		int m_ClipW;
		int m_ClipH;
	};
		
	struct SCommand_Clear : public SCommand
	{
		SCommand_Clear() : SCommand(CMD_CLEAR) {}
		SColor m_Color;
	};
		
	struct SCommand_Signal : public SCommand
	{
		SCommand_Signal() : SCommand(CMD_SIGNAL) {}
		semaphore *m_pSemaphore;
	};

	struct SCommand_RunBuffer : public SCommand
	{
		SCommand_RunBuffer() : SCommand(CMD_RUNBUFFER) {}
		CCommandBuffer *m_pOtherBuffer;
	};

	struct SCommand_Render : public SCommand
	{
		SCommand_Render() : SCommand(CMD_RENDER) {}
		SState m_State;
		unsigned m_PrimType;
		unsigned m_PrimCount;
		SVertex *m_pVertices; // you should use the command buffer data to allocate vertices for this command
	};

	struct SCommand_Screenshot : public SCommand
	{
		SCommand_Screenshot() : SCommand(CMD_SCREENSHOT) {}
		int m_X, m_Y, m_W, m_H; // specify rectangle size, -1 if fullscreen (width/height)
		CImageInfo *m_pImage; // processor will fill this out, the one who adds this command must free the data as well
	};

	struct SCommand_VideoModes : public SCommand
	{
		SCommand_VideoModes() : SCommand(CMD_VIDEOMODES) {}

		CVideoMode *m_pModes; // processor will fill this in
		int m_MaxModes; // maximum of modes the processor can write to the m_pModes
		int *m_pNumModes; // processor will write to this pointer
		int m_Screen;
	};

	struct SCommand_Swap : public SCommand
	{
		SCommand_Swap() : SCommand(CMD_SWAP) {}

		int m_Finish;
	};

	struct SCommand_VSync : public SCommand
	{
		SCommand_VSync() : SCommand(CMD_VSYNC) {}

		int m_VSync;
		bool *m_pRetOk;
	};

	struct SCommand_Texture_Create : public SCommand
	{
		SCommand_Texture_Create() : SCommand(CMD_TEXTURE_CREATE) {}

		// texture information
		int m_Slot;

		int m_Width;
		int m_Height;
		int m_GridWidth;
		int m_GridHeight;
		int m_PixelSize;
		int m_Format;
		int m_StoreFormat;
		int m_Flags;
		void *m_pData; // will be freed by the command processor
	};

	struct SCommand_Texture_Update : public SCommand
	{
		SCommand_Texture_Update() : SCommand(CMD_TEXTURE_UPDATE) {}

		// texture information
		int m_Slot;

		int m_X;
		int m_Y;
		int m_Width;
		int m_Height;
		int m_Format;
		void *m_pData; // will be freed by the command processor
	};


	struct SCommand_Texture_Destroy : public SCommand
	{
		SCommand_Texture_Destroy() : SCommand(CMD_TEXTURE_DESTROY) {}

		// texture information
		int m_Slot;
	};
	
	//
	CCommandBuffer(unsigned CmdBufferSize, unsigned DataBufferSize)
	: m_CmdBuffer(CmdBufferSize), m_DataBuffer(DataBufferSize)
	{
	}

	void *AllocData(unsigned WantedSize)
	{
		return m_DataBuffer.Alloc(WantedSize);
	}

	template<class T>
	bool AddCommand(const T &Command)
	{
		// make sure that we don't do something stupid like ->AddCommand(&Cmd);
		(void)static_cast<const SCommand *>(&Command);

		// allocate and copy the command into the buffer
		SCommand *pCmd = (SCommand *)m_CmdBuffer.Alloc(sizeof(Command));
		if(!pCmd)
			return false;
		mem_copy(pCmd, &Command, sizeof(Command));
		pCmd->m_Size = sizeof(Command);
		return true;
	}

	SCommand *GetCommand(unsigned *pIndex)
	{
		if(*pIndex >= m_CmdBuffer.DataUsed())
			return NULL;

		SCommand *pCommand = (SCommand *)&m_CmdBuffer.DataPtr()[*pIndex];
		*pIndex += pCommand->m_Size;
		return pCommand;
	}

	void Reset()
	{
		m_CmdBuffer.Reset();
		m_DataBuffer.Reset();
	}
};

// interface for the graphics backend
// all these functions are called on the main thread
class IGraphicsBackend
{
public:
	enum
	{
		INITFLAG_FULLSCREEN = 1,
		INITFLAG_VSYNC = 2,
		INITFLAG_RESIZABLE = 4,
		INITFLAG_BORDERLESS = 8,
	};

	virtual ~IGraphicsBackend() {}

	virtual int Init(const char *pName, int *Screen, int *pWidth, int *pHeight, int FsaaSamples, int Flags, int *pDesktopWidth, int *pDesktopHeight) = 0;
	virtual int Shutdown() = 0;

	virtual int MemoryUsage() const = 0;

	virtual int GetNumScreens() const = 0;

	virtual void Minimize() = 0;
	virtual void Maximize() = 0;
	virtual bool Fullscreen(bool State) = 0;
	virtual void SetWindowBordered(bool State) = 0;
	virtual bool SetWindowScreen(int Index) = 0;
	virtual int GetWindowScreen() = 0;
	virtual int WindowActive() = 0;
	virtual int WindowOpen() = 0;

	virtual void RunBuffer(CCommandBuffer *pBuffer) = 0;
	virtual bool IsIdle() const = 0;
	virtual void WaitForIdle() = 0;
};

class CGraphics_Threaded : public IEngineGraphics
{
	enum
	{
		NUM_CMDBUFFERS = 2,

		MAX_VERTICES = 32*1024,
		MAX_TEXTURES = 1024*4,
		
		DRAWING_QUADS=1,
		DRAWING_LINES=2
	};

	CCommandBuffer::SState m_State;
	IGraphicsBackend *m_pBackend;

	CCommandBuffer *m_apCommandBuffers[NUM_CMDBUFFERS];
	CCommandBuffer *m_pCommandBuffer;
	unsigned m_CurrentCommandBuffer;

	//
	class IStorage *m_pStorage;
	class IConsole *m_pConsole;

	CCommandBuffer::SVertex m_aVertices[MAX_VERTICES];
	int m_NumVertices;

	CCommandBuffer::SColor m_aColor[4];
	CCommandBuffer::STexCoord m_aTexture[4];

	bool m_RenderEnable;

	float m_Rotation;
	int m_Drawing;
	bool m_DoScreenshot;
	char m_aScreenshotName[128];

	CTextureHandle m_InvalidTexture;

	int m_aTextureIndices[MAX_TEXTURES];
	int m_FirstFreeTexture;
	int m_TextureMemoryUsage;

	void FlushVertices();
	void AddVertices(int Count);
	void Rotate4(const CCommandBuffer::SPoint &rCenter, CCommandBuffer::SVertex *pPoints);

	void KickCommandBuffer();

	int IssueInit();
	int InitWindow();
public:
	CGraphics_Threaded();

	virtual void ClipEnable(int x, int y, int w, int h);
	virtual void ClipDisable();

	virtual void BlendNone();
	virtual void BlendNormal();
	virtual void BlendAdditive();

	virtual void WrapNormal();
	virtual void WrapClamp();
	virtual void WrapMode(int WrapU, int WrapV);

	virtual int MemoryUsage() const;

	virtual void MapScreen(float TopLeftX, float TopLeftY, float BottomRightX, float BottomRightY);
	virtual void GetScreen(float *pTopLeftX, float *pTopLeftY, float *pBottomRightX, float *pBottomRightY);

	virtual void LinesBegin();
	virtual void LinesEnd();
	virtual void LinesDraw(const CLineItem *pArray, int Num);

	virtual int UnloadTexture(IGraphics::CTextureHandle *Index);
	virtual IGraphics::CTextureHandle LoadTextureRaw(int Width, int Height, int GridWidth, int GridHeight, int Format, const void *pData, int StoreFormat, int Flags);
	virtual int LoadTextureRawSub(IGraphics::CTextureHandle TextureID, int x, int y, int Width, int Height, int Format, const void *pData);

	// simple uncompressed RGBA loaders
	virtual IGraphics::CTextureHandle LoadTexture(const char *pFilename, int StorageType, int StoreFormat, int Flags);
	virtual int LoadPNG(CImageInfo *pImg, const char *pFilename, int StorageType);

	void ScreenshotDirect(const char *pFilename);

	virtual void TextureSet(CTextureHandle TextureID);

	virtual void Clear(float r, float g, float b);

	virtual void QuadsBegin();
	virtual void QuadsEnd();
	virtual void QuadsSetRotation(float Angle);

	virtual void SetColorVertex(const CColorVertex *pArray, int Num);
	virtual void SetColor(float r, float g, float b, float a);
	virtual void SetColor4(vec4 TopLeft, vec4 TopRight, vec4 BottomLeft, vec4 BottomRight);
	
	virtual void SetColor(vec4 rgba, bool AlphaBlend);
	virtual void SetColor2(vec4 Color0, vec4 Color1, bool AlphaBlend);
	virtual void SetColor4(vec4 TopLeft, vec4 TopRight, vec4 BottomLeft, vec4 BottomRight, bool AlphaBlend);

	virtual void QuadsSetSubset(float TlU, float TlV, float BrU, float BrV, int TextureIndex = -1);
	virtual void QuadsSetSubsetFree(
		float x0, float y0, float x1, float y1,
		float x2, float y2, float x3, float y3, int TextureIndex = -1);

	virtual void QuadsDraw(CQuadItem *pArray, int Num);
	virtual void QuadsDrawTL(const CQuadItem *pArray, int Num);
	virtual void QuadsDrawFreeform(const CFreeformItem *pArray, int Num);
	virtual void QuadsText(float x, float y, float Size, const char *pText);

	virtual int GetNumScreens() const;
	virtual void Minimize();
	virtual void Maximize();
	virtual bool Fullscreen(bool State);
	virtual void SetWindowBordered(bool State);
	virtual bool SetWindowScreen(int Index);
	virtual int GetWindowScreen();

	virtual int WindowActive();
	virtual int WindowOpen();

	virtual int Init();
	virtual void Shutdown();

	virtual void ReadBackbuffer(unsigned char **ppPixels, int x, int y, int w, int h);
	virtual void TakeScreenshot(const char *pFilename);
	virtual void Swap();
	virtual bool SetVSync(bool State);

	virtual int GetVideoModes(CVideoMode *pModes, int MaxModes, int Screen);

	virtual int GetDesktopScreenWidth() const { return m_DesktopScreenWidth; }
	virtual int GetDesktopScreenHeight() const { return m_DesktopScreenHeight; }

	// syncronization
	virtual void InsertSignal(semaphore *pSemaphore);
	virtual bool IsIdle() const;
	virtual void WaitForIdle();
};

extern IGraphicsBackend *CreateGraphicsBackend();
