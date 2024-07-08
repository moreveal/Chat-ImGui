#include "pch.h"
#include "ChatImGui.h"
#include "snippets.hpp"

void ChatImGui::initialize()
{
	mChatLines.reserve(152);

	SetHook(mChatConstrHook, sampGetChatConstr(), &CChat__CChat);
	SetHook(mChatOnLostDeviceHook, sampGetChatOnLostDevice(), &CChat__OnLostDevice);
	SetHook(mChatEntryHook, sampGetAddEntryFuncPtr(), &CChat__AddEntry);
	SetHook(mRecalcFontSizeHook, sampGetRecalcFontSize(), &CChat__RecalcFontSize);
	SetHook(mChatRenderHook, sampGetChatRender(), &CChat__Render);
	SetHook(mChatDXUTScrollHook, sampGetChatScrollDXUTScrollCallPtr(), &CDXUTScrollBar__Scroll);
	SetHook(mChatScrollToBottomHook, sampGetChatScrollToBottomFuncPtr(), &CChat__ScrollToBottom);
	SetHook(mChatPageUpHook, sampGetChatPageUpFuncPtr(), &CChat__PageUp);
	SetHook(mChatPageDownHook, sampGetChatPageDownFuncPtr(), &CChat__PageDown);
	SetHook(mObjectEditRenderHook, sampGetObjectEditRender(), &CObjectEdit__Render);

	{ // Nops
		DWORD old_protection;
		const uintptr_t* nops = sampGetChatNopsData();
		void* address = reinterpret_cast<void*>(sampGetBase() + nops[0]);
		VirtualProtect(address, nops[1], PAGE_EXECUTE_READWRITE, &old_protection);

		memset(address, 0x90, nops[1]);

		VirtualProtect(address, nops[1], old_protection, nullptr);
	} // Nops

	//AllocConsole(); freopen("CONOUT$", "w", stdout); // Log

	std::thread([&]
		{
			while (!isSampAvailable()) {}

			sampRegisterChatCommand("alphachat", CMDPROC__AlphaChat);
			sampRegisterChatCommand("icc", CMDPROC__ICC);

			mChatAlphaEnabled = sampReadVariableFromConfig("alphachat");

			mLinesCount = sampGetPagesize();
			
			// WndProcHook
			while (!(*reinterpret_cast<uintptr_t*>(0x00C8D4C0) == 9 && sampGetBase() != 0)) std::this_thread::sleep_for(std::chrono::milliseconds(50));
			const HWND GameHWND = *(*reinterpret_cast<HWND**>(0xC17054));
			const auto originalWndProc = reinterpret_cast<uintptr_t>(reinterpret_cast<WNDPROC>(GetWindowLongPtrW(GameHWND, GWLP_WNDPROC)));
			SetHook(mWndProcHook, originalWndProc, &WndProcHandle);
		}
	).detach();
}

HRESULT __stdcall WndProcHandle(const decltype(gChat.mWndProcHook)& hook, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	wchar_t wch;
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, reinterpret_cast<char*>(&wParam), 1, &wch, 1);
	ImGui_ImplWin32_WndProcHandler(hWnd, msg, wch, lParam);
	
	if (msg == WM_KEYDOWN && wParam == VK_F2 && (GetKeyState(VK_SHIFT) & 0x8000) && (HIWORD(lParam) & KF_REPEAT) != KF_REPEAT) // Shift + F2 (1 press)
	{
		// TODO: ScreenChat (rendering chat to the DirectX texture and save it to file)
		//sampAddChatMessage("Save chat screen to: test\\xx-xx-xx.png", 0x88AA62);
	}

	// Close editline window by esc/enter + block this keys for game
	static bool editLineClosed = false;
	if (editLineClosed && msg == WM_CHAR && (wParam == VK_RETURN || wParam == VK_ESCAPE)) return true;
	if (msg != WM_QUIT && msg != WM_DESTROY && msg != WM_CLOSE && msg != WM_SYSCOMMAND) {
		if (editLineWindow || (editLineClosed && msg == WM_KEYUP))
		{
			editLineClosed = false;
			if (msg == WM_KEYDOWN && (wParam == VK_ESCAPE || wParam == VK_RETURN)) {
				editLineWindow = false;
				editLineClosed = true;
				pSelectedChatLine = nullptr;
			}
			return true;
		}
	}

	return hook.get_trampoline()(hWnd, msg, wParam, lParam);
}

void ChatImGui::rebuildFonts()
{
	std::string fontPath(256, '\0');

	if (SHGetSpecialFolderPathA(GetActiveWindow(), fontPath.data(), CSIDL_FONTS, false))
	{
		ImGui_ImplDX9_InvalidateDeviceObjects();

		fontPath.resize(fontPath.find('\0'));
		std::string fontName{ sampGetChatFontName() };
		if (fontName == "Arial")
			fontName += "Bd";
		fontPath += "\\" + fontName + ".ttf";

		auto& io = ImGui::GetIO();
		io.Fonts->Clear();
		ImVector<ImWchar> ranges;
		ImFontGlyphRangesBuilder builder;
		io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
		builder.AddRanges(io.Fonts->GetGlyphRangesCyrillic());
		builder.AddText(u8"‚„…†‡ˆ‰‹‘’“”•–—™›¹");
		builder.BuildRanges(&ranges);
		
		updateCurrentFontSize();
		io.Fonts->AddFontFromFileTTF(fontPath.c_str(), getCurrentFontSize() - 2, nullptr, ranges.Data);
		io.Fonts->Build();

		ImGui::GetStyle().ItemSpacing.y = 2;

		ImGui_ImplDX9_CreateDeviceObjects();
	}
}

void ChatImGui::renderOutline(const char* text__)
{
	if (sampGetChatDisplayMode() == 2) // If outlines are enabled
	{
		auto pos = ImGui::GetCursorScreenPos();
		auto drawlist = ImGui::GetBackgroundDrawList();
		ImU32 color = static_cast<ImU32>((ImGui::GetStyle().Alpha - 0.1f) * 255.f);
		color <<= 24;

#define DRAW_TEXT drawlist->AddText(pos, color, text__)

#if !USE_ONLY_OLD_OUTLINE_REALISATION
		if (*reinterpret_cast<int*>(0xC17044) > 1280)
		{
			pos.y -= 2;
			DRAW_TEXT;
			pos.x -= 1;
			pos.y += 1;
			DRAW_TEXT;
			pos.x += 2;
			DRAW_TEXT;
			pos.x -= 1;
			pos.y += 3;
			DRAW_TEXT;
			pos.x -= 1;
			pos.y -= 1;
			DRAW_TEXT;
			pos.x += 2;
			DRAW_TEXT;
			pos.y -= 1;
			pos.x -= 3;
			DRAW_TEXT;
			pos.x += 4;
			DRAW_TEXT;
		}
		else
#endif
		{
			pos.y -= 1;
			DRAW_TEXT;
			pos.y += 2;
			DRAW_TEXT;
			pos.y -= 1;
			pos.x -= 1;
			DRAW_TEXT;
			pos.x += 2;
			DRAW_TEXT;
		}
#undef DRAW_TEXT

	}
}

void ChatImGui::renderText(ImVec4& color, void* data, bool isTimestamp)
{
	auto text = reinterpret_cast<char*>(data);
	ChatImGui::renderOutline(text);
	ImGui::TextColored(color, text);
	ImGui::SameLine(0.0f, (isTimestamp ? 5.0f : 0.0f));
}

void ChatImGui::renderLine(chat_line_t& data)
{
	ImVec4 color(1, 1, 1, 1);
	
	for (size_t i = 0; i < data.size(); i++) {
		const auto& line = data[i];

		switch (line.type)
		{
		case COLOR:
			color = *reinterpret_cast<ImVec4*>(line.data);
			break;
		case TIMESTAMP:
			if (sampIsTimestampEnabled()) renderText(color, line.data, true);
			break;
		case TEXT:
			renderText(color, line.data);
			break;
		}
		
		if (ImGui::IsItemHovered())
		{
			const auto& cursorX = ImGui::GetCursorPosX();
			ImGui::SetCursorPosX(0.0f);
			ImGui::Text(">");
			ImGui::SameLine();
			ImGui::SetCursorPosX(cursorX);

			if (!TextContextMenuPopup)
			{
				if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
				{
					pSelectedChatLine = &data;
					TextContextMenuPopup = true;
				}
			}
		}
	}

	ImGui::NewLine();
}

void ChatImGui::pushColorToBuffer(ChatImGui::chat_line_t& line, ImVec4& color)
{
	line.push_back({ eLineMetadataType::COLOR, new ImVec4(color) });
}

void ChatImGui::pushTextToBuffer(ChatImGui::chat_line_t& line, std::string_view text, bool isTimestamp)
{
	char* out = new char[text.length() + 1];
	auto len = text.copy(out, text.length());
	out[len] = '\0';
	line.push_back({ (isTimestamp ? eLineMetadataType::TIMESTAMP : eLineMetadataType::TEXT), out });
}

void ChatImGui::pushTimestampToBuffer(ChatImGui::chat_line_t& line, std::string_view timestamp)
{
	pushTextToBuffer(line, timestamp, true);
}

auto ChatImGui::eraseFirstLine()
{
	auto ptr = gChat.mChatLines.begin();

	if (pSelectedChatLine != nullptr)
	{
		const size_t index = std::distance(gChat.mChatLines.data(), pSelectedChatLine);
		if (index > 0)
		{
			pSelectedChatLine = &gChat.mChatLines[index - 1];
		}
		else pSelectedChatLine = nullptr;
	}

	for (auto subPtr = ptr->begin(); subPtr != ptr->end(); subPtr++)
	{
		if (subPtr->data == nullptr) continue;

		if (subPtr->type == ChatImGui::eLineMetadataType::COLOR)
			delete(subPtr->data);
		else
			delete[](subPtr->data);
	}

	return gChat.mChatLines.erase(ptr);
}

ImVec4 ChatImGui::ParseColor(const std::string& colorStr) {
	ImVec4 color;
	if (colorStr.size() == 6) {
		int r, g, b;
		sscanf(colorStr.c_str(), "%02x%02x%02x", &r, &g, &b);
		color.x = static_cast<float>(r) / 255.0f;
		color.y = static_cast<float>(g) / 255.0f;
		color.z = static_cast<float>(b) / 255.0f;
		color.w = 1.0f;
	}
	else {
		// Default color if format is invalid
		color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
	}
	return color;
}

std::string ChatImGui::BuildTimestampString()
{
	std::string time_(64, '\0');
	std::time_t t = std::time(nullptr);

	if (reinterpret_cast<decltype(std::strftime)*>(sampGetStrftimeFuncPtr())
		(time_.data(), time_.size() + 1, "[%H:%M:%S]", std::localtime(&t))
		) {
		time_.resize(time_.find('\0'));
		return time_;
	}
	return "";
}

std::string ChatImGui::BuildChatString(const chat_line_t& data, bool colors, bool prefix)
{
	std::string text; bool prefixAdded = false;
	for (const auto& line : data) {
		switch (line.type)
		{
			case ChatImGui::COLOR:
			{
				if (!colors) break;
				const ImVec4 color = *static_cast<ImVec4*>(line.data);
				if (!prefixAdded)
				{
					prefixAdded = true;
					if (!prefix) continue;
				}

				char buf[8 + 1];
				snprintf(buf, sizeof(buf), "{%02X%02X%02X}", (int)(color.x * 255), (int)(color.y * 255), (int)(color.z * 255));
				text += buf;
				break;
			}
			case ChatImGui::TEXT:
				text += static_cast<char*>(line.data);
				break;
			default: break;
		}
	}
	return text;
}

ChatImGui::chat_line_t ChatImGui::BuildChatLine(const std::string& timestamp, const std::string& string) {
	chat_line_t result;
	std::string remainingText = string;
	bool timestampAdded = false;

	std::regex colorTagRegex(R"(\{([0-9A-Fa-f]{6})\})");
	std::smatch match;

	while (std::regex_search(remainingText, match, colorTagRegex)) {
		std::string textBefore = match.prefix();
		if (!textBefore.empty()) {
			pushTextToBuffer(result, textBefore, false);
		}

		if (match.size() >= 2) {
			std::string colorHex = match[1].str();
			ImVec4 color = ParseColor(colorHex);

			pushColorToBuffer(result, color);

			if (!timestampAdded) {
				pushTimestampToBuffer(result, timestamp);
				timestampAdded = true;
			}
		}
		else {
			pushTextToBuffer(result, match[0].str(), false);
		}

		remainingText = match.suffix();
	}

	if (!timestampAdded) {
		auto color = ImVec4(1.0, 1.0, 1.0, 1.0);
		pushColorToBuffer(result, color);
		pushTimestampToBuffer(result, timestamp);
	}

	if (!remainingText.empty()) {
		pushTextToBuffer(result, remainingText, false);
	}

	return result;
}

const char* ChatImGui::GetLineTimestamp(const chat_line_t& data)
{
	for (const auto& line : data)
	{
		if (line.type == TIMESTAMP)
		{
			return static_cast<const char*>(line.data);
		}
	}

	return nullptr;
}

ImVec4 ChatImGui::GetLineColor(const chat_line_t& data)
{
	for (const auto& line : data)
	{
		if (line.type == COLOR)
		{
			return *static_cast<ImVec4*>(line.data);
		}
	}

	return ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
}

void ChatImGui::SetLineColor(chat_line_t& data, ImVec4 color)
{
	for (auto& line : data)
	{
		if (line.type == COLOR)
		{
			line.data = reinterpret_cast<void*>(new ImVec4(color));
			return;
		}
	}
}


void* __fastcall CChat__CChat(const decltype(gChat.mChatConstrHook)& hook, void* ptr, void*, IDirect3DDevice9* pDevice, void* pFontRenderer, const char* pChatLogPath)
{
	ImGui::CreateContext();
	ImGui_ImplWin32_Init(GetActiveWindow());
	ImGui_ImplDX9_Init(pDevice);

	return hook.get_trampoline()(ptr, nullptr, pDevice, pFontRenderer, pChatLogPath);
}

void __fastcall CChat__AddEntry(const decltype(gChat.mChatEntryHook)& hook, void* ptr, void*, int nType, const char* szText, const char* szPrefix, uint32_t textColor, uint32_t prefixColor)
{
	std::string text(szText);
	ChatImGui::chat_line_t output;

	uint8_t r_ = ((textColor >> 16) & 0xff), g_ = ((textColor >> 8) & 0xff), b_ = (textColor & 0xff);
	ImVec4 color(r_ / 255.f, g_ / 255.f, b_ / 255.f, 1);
	ChatImGui::pushColorToBuffer(output, color);

	{
		const auto& time = ChatImGui::BuildTimestampString();
		ChatImGui::pushTimestampToBuffer(output, time);
	}

	if (nType == 2)
	{
		uint8_t r_ = ((prefixColor >> 16) & 0xff), g_ = ((prefixColor >> 8) & 0xff), b_ = (prefixColor & 0xff);
		ImVec4 colorPrefix(r_ / 255.f, g_ / 255.f, b_ / 255.f, 1);
		ChatImGui::pushColorToBuffer(output, colorPrefix);
		std::string prefix(szPrefix);
		prefix += ": ";
		ChatImGui::pushTextToBuffer(output, prefix);
		ChatImGui::pushColorToBuffer(output, color);
	}

	std::regex regex("\\{([a-fA-F0-9]{6})\\}");
	std::smatch color_match;

	bool found = std::regex_search(text, color_match, regex);

	auto a = color_match.position();
	auto b = a + 7;

	while (found)
	{
		auto t = text.substr(0, a);
		if (t.length() > 0)
		{
			auto t_ = cp1251_to_utf8(t);
			ChatImGui::pushTextToBuffer(output, t_);
		}

		const auto& clr = color_match[1];
		{
			auto clr_ = stoi(clr.str(), nullptr, 16);
			uint8_t r = ((clr_ >> 16) & 0xff), g = ((clr_ >> 8) & 0xff), b = (clr_ & 0xff);
			color = ImVec4(r / 255.f, g / 255.f, b / 255.f, 1);
			ChatImGui::pushColorToBuffer(output, color);
		}

		text = text.substr(b + 1);

		found = std::regex_search(text, color_match, regex);

		a = color_match.position();
		b = a + 7;
	}
	if (text.length() > 0)
	{
		auto t_ = cp1251_to_utf8(text);
		ChatImGui::pushTextToBuffer(output, t_);
	}
	gChat.mChatLines.push_back(output);

	gChat.increaseLinesCount();
	if (gChat.getLinesCount() > 150)
		gChat.eraseFirstLine();

	hook.get_trampoline()(ptr, nullptr, nType, szText, szPrefix, textColor, prefixColor);
}

void __fastcall CChat__RecalcFontSize(const decltype(gChat.mRecalcFontSizeHook)& hook, void* ptr, void*)
{
	hook.get_trampoline()(ptr, nullptr);

	gChat.rebuildFonts();
}

void __fastcall CObjectEdit__Render(const decltype(gChat.mObjectEditRenderHook)& hook, void* ptr, void*)
{
	// CObjectEdit render
	hook.get_trampoline()(ptr, nullptr);

	// Continued rendering [Over game elements]
	if (editLineWindow)
	{
		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.12f, 0.12f, 0.12f, 0.9f));
		if (ImGui::Begin("##EditLine", &editLineWindow, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground))
		{
			static const auto& io = ImGui::GetIO();
			ImGui::SetWindowPos(ImVec2((io.DisplaySize.x - ImGui::GetWindowWidth()) * 0.5f,
				(io.DisplaySize.y - ImGui::GetWindowHeight()) * 0.5f));

			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(12.0f, 12.0f));
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 12.0f));
			ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
			ImGui::PushItemWidth(io.DisplaySize.x * (750.0f / 1920) );

			if (ImGui::ColorButton("ColorButton", editLineColor, ImGuiColorEditFlags_NoTooltip))
			{
				ImGui::OpenPopup("ColorPickerPopup");
			}

			if (ImGui::BeginPopup("ColorPickerPopup"))
			{
				if (ImGui::ColorPicker3("##picker", (float*)&editLineColor, ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoSmallPreview))
				{
					ChatImGui::SetLineColor(*pSelectedChatLine, editLineColor);
				}
				ImGui::EndPopup();
			}

			ImGui::SameLine(0.0f, 5.0f);

			if (ImGui::InputText("##ChatLine", editLineBuffer, IM_ARRAYSIZE(editLineBuffer)))
			{
				if (pSelectedChatLine != nullptr)
				{
					auto& line = *pSelectedChatLine;
					const std::string originalLine = ChatImGui::BuildChatString(line, true, true);

					const auto lineData = ChatImGui::BuildChatLine(ChatImGui::GetLineTimestamp(line), originalLine.substr(0, 8) + editLineBuffer);
					line = lineData;
				}
			}
			ImVec2 inputPos = ImGui::GetItemRectMin();
			ImVec2 inputSize = ImGui::GetItemRectSize();
			ImDrawList* drawList = ImGui::GetWindowDrawList();
			drawList->AddRect(inputPos, ImVec2(inputPos.x + inputSize.x, inputPos.y + inputSize.y), IM_COL32(255, 255, 255, 255));

			ImGui::PopItemWidth();
			ImGui::PopStyleVar(2);
			ImGui::PopStyleColor();
			ImGui::End();
		}
		ImGui::PopStyleColor();
	}

	if (ImGui::Begin("ImGui Chat##PopupWrapper", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar))
	{
		if (TextContextMenuPopup && !ImGui::IsPopupOpen("TextContextMenu"))
		{
			ImGui::OpenPopup("TextContextMenu");
			TextContextMenuPopup = false;
		}

		if (ImGui::BeginPopupContextItem("TextContextMenu"))
		{
			if (pSelectedChatLine != nullptr)
			{
				ImGui::PushItemWidth(350.0f);
				ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(12.0f, 12.0f));
				ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.12f, 0.12f, 0.12f, 0.9f));
				ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
				if (ImGui::MenuItem("Copy"))
				{
					const bool with_colors = (GetKeyState(VK_SHIFT) & 0x8000) != 0; // Hold shift
					const auto text = ChatImGui::BuildChatString(*pSelectedChatLine, with_colors);
					ImGui::SetClipboardText(text.c_str());
				}
				if (ImGui::MenuItem("Edit"))
				{
					const auto& line = *pSelectedChatLine;
					const auto text = ChatImGui::BuildChatString(line, true, false);

					editLineBuffer[0] = '\0';
					strcat(editLineBuffer, text.substr(0, 256).c_str());

					editLineColor = ChatImGui::GetLineColor(*pSelectedChatLine);
					editLineWindow = true;
				}
				if (ImGui::MenuItem(u8"Delete"))
				{
					const auto it = std::find_if(gChat.mChatLines.begin(), gChat.mChatLines.end(), [&](const ChatImGui::chat_line_t& chatLine) {
						return &chatLine == pSelectedChatLine;
					});

					if (it != gChat.mChatLines.end()) gChat.mChatLines.erase(it);
				}
				ImGui::PopItemWidth();
				ImGui::PopStyleVar();
				ImGui::PopStyleColor(2);
			}

			ImGui::EndPopup();
		}

		ImGui::End();
	}

	ImGui::EndFrame();
	ImGui::Render();
	ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
}

void __fastcall CChat__Render(const decltype(gChat.mChatRenderHook)& hook, void* ptr, void*)
{
	// Continued rendering [Under game elements]
	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	if (gChat.isChatAlphaEnabled())
	{
		auto& alpha = ImGui::GetStyle().Alpha;
		if (!sampIsChatInputEnabled())
		{
			if (alpha >= 0.8f)
				alpha -= 0.005f;
		}
		else
		{
			if (alpha < 1.f)
				alpha += 0.005f;
		}
	}
	if (ImGui::Begin("Chat ImGui", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar))
	{
		ImGui::SetWindowPos(ImVec2(36, 18));
		ImGui::SetWindowSize(ImVec2(4096, ImGui::GetTextLineHeightWithSpacing() * sampGetPagesize()));

		static ImGuiListClipper clipper;
		auto linesCount = gChat.getLinesCount();
		clipper.Begin(linesCount);
		while (clipper.Step())
			for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++)
			{
				auto elementId = row - linesCount + gChat.mChatLines.size();
				if (elementId >= gChat.mChatLines.size()) ImGui::NewLine();
				else ChatImGui::renderLine(gChat.mChatLines[elementId]);
			}

		if (gChat.shouldWeScrollToBottom())
		{
			float current_scroll = ImGui::GetScrollY(), max_scroll = ImGui::GetScrollMaxY(), diff_scroll = max_scroll - current_scroll;
			if (diff_scroll > 300.f)
				ImGui::SetScrollY(current_scroll + 30.f);
			else if (diff_scroll > 150.f)
				ImGui::SetScrollY(current_scroll + 10.f);
			else if (diff_scroll > 0.f)
				ImGui::SetScrollY(current_scroll + 5.f);
			else if (diff_scroll < 0.f)
				ImGui::SetScrollY(max_scroll);
		}
		else
			ImGui::SetScrollY(ImGui::GetScrollMaxY() + (ImGui::GetTextLineHeightWithSpacing() * gChat.whereToScroll()));

		if (ImGui::GetScrollY() < ImGui::GetScrollMaxY() - ImGui::GetTextLineHeightWithSpacing() * 120)
			ImGui::SetScrollY(ImGui::GetScrollMaxY());

		ImGui::End();
	}
}

int __fastcall CChat__OnLostDevice(const decltype(gChat.mChatOnLostDeviceHook)& hook, void* ptr, void*)
{
	ImGui_ImplDX9_InvalidateDeviceObjects();

	return hook.get_trampoline()(ptr, nullptr);
}

int __fastcall CDXUTScrollBar__Scroll(const decltype(gChat.mChatDXUTScrollHook)& hook, void* ptr, void*, int nDelta)
{
	gChat.scrollTo(nDelta);

	return hook.get_trampoline()(ptr, nullptr, nDelta);
}

int __fastcall CChat__ScrollToBottom(const decltype(gChat.mChatScrollToBottomHook)& hook, void* ptr, void*)
{
	gChat.scrollToBottom();

	return hook.get_trampoline()(ptr, nullptr);
}

int __fastcall CChat__PageUp(const decltype(gChat.mChatPageUpHook)& hook, void* ptr, void*)
{
	gChat.scrollTo(-15);

	return hook.get_trampoline()(ptr, nullptr);
}

int __fastcall CChat__PageDown(const decltype(gChat.mChatPageDownHook)& hook, void* ptr, void*)
{
	gChat.scrollTo(15);

	return hook.get_trampoline()(ptr, nullptr);
}

void CMDPROC__AlphaChat(const char*)
{
	gChat.switchChatAlphaState();
	bool enabled = gChat.isChatAlphaEnabled();
	const auto text_ = std::string("-> Chat alpha ") + (enabled ? "enabled" : "disabled");
	sampAddChatMessage(text_.c_str(), 0xFF88AA62);

	sampSaveVariableToConfig("alphachat", enabled);

	if (!enabled)
		ImGui::GetStyle().Alpha = 1.f;
}

void CMDPROC__ICC(const char*)
{
	for (auto ptr = gChat.mChatLines.begin(); ptr != gChat.mChatLines.end();)
		ptr = gChat.eraseFirstLine();

	ChatImGui::chat_line_t line;
	for (auto i = 0; i < 150; i++)
		gChat.mChatLines.push_back(line);

	memset(reinterpret_cast<void*>(sampGetChatInfoPtr() + 0x132), 0x0, 0xFC);
}