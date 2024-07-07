#pragma once

#include <d3d9.h>
#include <string>
#include <vector>

#define CHATIMGUI_VERSION 120 // 1.2.0

#define USE_ONLY_OLD_OUTLINE_REALISATION 0

class ChatImGui
{
private:
	rtdhook_call*		mWndProcHook = nullptr;
	rtdhook*			mChatConstrHook = nullptr;
	rtdhook*			mChatOnLostDeviceHook = nullptr;
	rtdhook*			mChatAddEntryHook = nullptr;
	rtdhook*			mRecalcFontSizeHook = nullptr;
	rtdhook_call*		mChatRenderHook = nullptr;

	rtdhook*			mChatScrollToBottomHook = nullptr;
	rtdhook_call*		mChatDXUTScrollHook = nullptr;
	rtdhook*			mChatPageUpHook = nullptr;
	rtdhook*			mChatPageDownHook = nullptr;
	
	rtdhook*			mObjectEditRenderHook = nullptr;

	bool				mChatAlphaEnabled = false;

	float				mCurrentFontSize = 18;
	float				mWhereToScroll = 0.f;

	size_t				mLinesCount = 0;

public:
	enum eLineMetadataType : uint8_t { COLOR, TIMESTAMP, TEXT };
	struct chat_line_data_t {
		eLineMetadataType type;
		void* data;
	};
	typedef std::vector<chat_line_data_t> chat_line_t;
	std::vector<chat_line_t>	mChatLines;

	void initialize();

	inline rtdhook*				getConstrHook() { return mChatConstrHook; }
	inline rtdhook*				getOnLostDeviceHook() { return mChatOnLostDeviceHook; }
	inline rtdhook*				getAddEntryHook() { return mChatAddEntryHook; }
	inline rtdhook*				getRecalcFontSizeHook() { return mRecalcFontSizeHook; }
	inline rtdhook_call*		getDXUTScrollHook() { return mChatDXUTScrollHook; }
	inline rtdhook*				getScrollToBottomHook() { return mChatScrollToBottomHook; }
	inline rtdhook*				getPageUpHook() { return mChatPageUpHook; }
	inline rtdhook*				getPageDownHook() { return mChatPageDownHook; }
	inline rtdhook_call*		getWndProcHook() { return mWndProcHook; }
	inline rtdhook*				getObjectEditRenderHook() { return mObjectEditRenderHook; }
	
	static inline void			pushColorToBuffer(ChatImGui::chat_line_t& line, ImVec4& color);
	static inline void			pushTextToBuffer(ChatImGui::chat_line_t& line, std::string_view text, bool isTimestamp = false);
	static inline void			pushTimestampToBuffer(ChatImGui::chat_line_t& line, std::string_view timestamp);

	static void					renderOutline(const char* text__);
	static void					renderText(ImVec4& color, void* data, bool isTimestamp = false);
	static void					renderLine(ChatImGui::chat_line_t& data);

	static inline ImVec4		ParseColor(const std::string& colorStr);

	static inline const char*	GetLineTimestamp(const chat_line_t& data);
	static inline ImVec4		GetLineColor(const chat_line_t& data);
	static void					SetLineColor(chat_line_t& data, ImVec4 color);

	static inline chat_line_t	BuildChatLine(const std::string& timestamp, const std::string& string);
	static inline std::string	BuildChatString(const chat_line_t& data, bool colors = false, bool prefix = false);
	static inline std::string	BuildTimestampString();

	void						rebuildFonts();

	void						updateCurrentFontSize() { mCurrentFontSize = sampGetFontsize(); }
	float						getCurrentFontSize() { return mCurrentFontSize; }

	inline void					scrollToBottom() { mWhereToScroll = 0.f; }
	inline void					scrollTo(int to) {
		mWhereToScroll += to;
		if (mWhereToScroll > 0.f)
			mWhereToScroll = 0;
		else if (mWhereToScroll < -100.f)
			mWhereToScroll = -100.f;
	}
	inline bool					shouldWeScrollToBottom() { return mWhereToScroll == 0.f; }
	inline float				whereToScroll() { return mWhereToScroll; }

	inline bool					isChatAlphaEnabled() { return mChatAlphaEnabled; }
	inline void					switchChatAlphaState() { mChatAlphaEnabled = !mChatAlphaEnabled; }

	inline uint32_t				getLinesCount() { return mLinesCount; }
	inline void					increaseLinesCount() { mLinesCount++; }
	
	auto						eraseFirstLine();
};

inline ChatImGui gChat;
inline bool editLineWindow = false;
inline char editLineBuffer[256];
inline ImVec4 editLineColor;
inline bool TextContextMenuPopup = false;

inline ChatImGui::chat_line_t* pSelectedChatLine = nullptr;

LRESULT CALLBACK WndProcHandle(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

void* __fastcall CChat__CChat(void* ptr, void*, IDirect3DDevice9* pDevice, void* pFontRenderer, const char* pChatLogPath);
void __fastcall CChat__AddEntry(void* ptr, void*, int nType, const char* szText, const char* szPrefix, uint32_t textColor, uint32_t prefixColor);
void __fastcall CChat__Render(void* ptr, void*);
int __fastcall CChat__OnLostDevice(void* ptr, void*);

int __fastcall CDXUTScrollBar__Scroll(void* ptr, void*, int nDelta);
int __fastcall CChat__ScrollToBottom(void* ptr, void*);
int __fastcall CChat__PageUp(void* ptr, void*);
int __fastcall CChat__PageDown(void* ptr, void*);

void __fastcall CDialog__Draw(void* ptr, void*);
void __fastcall CObjectEdit__Render(void* ptr, void*);
void __fastcall CChat__RecalcFontSize(void* ptr, void*);

void CMDPROC__AlphaChat(const char*);
void CMDPROC__ICC(const char*);