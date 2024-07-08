#pragma once

#include <d3d9.h>
#include <string>
#include <vector>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

#define CHATIMGUI_VERSION 120 // 1.2.0

#define USE_ONLY_OLD_OUTLINE_REALISATION 0

using dWndProcHook = LRESULT(__cdecl*)(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
using dChatConstructor = void* (__fastcall*)(void* ptr, void*, IDirect3DDevice9* pDevice, void* pFontRenderer, const char* pChatLogPath);
using dChatEntry = void(__fastcall*)(void* ptr, void*, int nType, const char* szText, const char* szPrefix, uint32_t textColor, uint32_t prefixColor);
using dRecalcFontSize = void(__fastcall*)(void* ptr, void*);
using dObjectEditRender = void(__fastcall*)(void* ptr, void*);
using dChatRender = void(__fastcall*)(void* ptr, void*);
using dChatOnLostDevice = int(__fastcall*)(void* ptr, void*);
using dDXUTScrollBarScroll = int(__fastcall*)(void* ptr, void*, int nDelta);
using dChatScrollToBottom = int(__fastcall*)(void* ptr, void*);
using dChatPageUp = int(__fastcall*)(void* ptr, void*);
using dChatPageDown = int(__fastcall*)(void* ptr, void*);

class ChatImGui
{
private:
	bool				mChatAlphaEnabled = false;

	float				mCurrentFontSize = 18;
	float				mWhereToScroll = 0.f;

	size_t				mLinesCount = 0;

public:
	kthook::kthook_simple<dWndProcHook>				mWndProcHook;
	kthook::kthook_simple<dChatConstructor>			mChatConstrHook;
	kthook::kthook_simple<dChatEntry>				mChatEntryHook;
	kthook::kthook_simple<dRecalcFontSize>			mRecalcFontSizeHook;
	kthook::kthook_simple<dObjectEditRender>		mObjectEditRenderHook;
	kthook::kthook_simple<dChatRender>				mChatRenderHook;
	kthook::kthook_simple<dChatOnLostDevice>		mChatOnLostDeviceHook;
	kthook::kthook_simple<dDXUTScrollBarScroll>		mChatDXUTScrollHook;
	kthook::kthook_simple<dChatScrollToBottom>		mChatScrollToBottomHook;
	kthook::kthook_simple<dChatPageUp>				mChatPageUpHook;
	kthook::kthook_simple<dChatPageDown>			mChatPageDownHook;

	template <typename HookType, typename CallbackType>
	void SetHook(HookType& hook, uintptr_t dest, CallbackType callback) {
		hook.set_dest(dest);
		hook.set_cb(callback);
		hook.install();
	}

	enum eLineMetadataType : uint8_t { COLOR, TIMESTAMP, TEXT };
	struct chat_line_data_t {
		eLineMetadataType type;
		void* data;
	};
	typedef std::vector<chat_line_data_t> chat_line_t;
	std::vector<chat_line_t>	mChatLines;

	void initialize();
	
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

HRESULT __stdcall WndProcHandle(const decltype(gChat.mWndProcHook)& hook, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
void* __fastcall CChat__CChat(const decltype(gChat.mChatConstrHook)& hook, void* ptr, void*, IDirect3DDevice9* pDevice, void* pFontRenderer, const char* pChatLogPath);
void __fastcall CChat__AddEntry(const decltype(gChat.mChatEntryHook)& hook, void* ptr, void*, int nType, const char* szText, const char* szPrefix, uint32_t textColor, uint32_t prefixColor);
void __fastcall CChat__RecalcFontSize(const decltype(gChat.mRecalcFontSizeHook)& hook, void* ptr, void*);
void __fastcall CObjectEdit__Render(const decltype(gChat.mObjectEditRenderHook)& hook, void* ptr, void*);
void __fastcall CChat__Render(const decltype(gChat.mChatRenderHook)& hook, void* ptr, void*);
int __fastcall CChat__OnLostDevice(const decltype(gChat.mChatOnLostDeviceHook)& hook, void* ptr, void*);

int __fastcall CDXUTScrollBar__Scroll(const decltype(gChat.mChatDXUTScrollHook)& hook, void* ptr, void*, int nDelta);
int __fastcall CChat__ScrollToBottom(const decltype(gChat.mChatScrollToBottomHook)& hook, void* ptr, void*);
int __fastcall CChat__PageUp(const decltype(gChat.mChatPageUpHook)& hook, void* ptr, void*);
int __fastcall CChat__PageDown(const decltype(gChat.mChatPageDownHook)& hook, void* ptr, void*);

void CMDPROC__AlphaChat(const char*);
void CMDPROC__ICC(const char*);