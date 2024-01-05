#include "messagebox.hpp"
#include "../../../src/blur/blur.hpp"
#include "instance.hpp"
#include "render_wrapper.hpp"

#include <mutex>

namespace NS_GUI::msgbox {
	static std::vector<std::string> messages{};
	static std::mutex mtx{};
	static std::condition_variable cond{};

	static float alpha = 0.f;

	void render() {
		THREAD_SAFE(mtx);

		if (!is_opened)
			messages.clear();

		if (messages.empty()) {
			alpha = 0.f;
			return;
		}

		alpha = std::clamp(alpha + globals().time_delta * 8.f, 0.f, 1.f);
		shaders::create_blur(render::draw_list, {}, { render::screen_width, render::screen_height }, { 127, 127, 127, 255 });

		const auto center = vec2d{ render::screen_width, render::screen_height } * 0.5f;
		std::string text = STRS("Error!\n\n");
		for (const auto& msg: messages)
			text += msg + "\n";

		text += STRS("\n\nPress ESC to close this window");

		const auto text_size = render::calc_text_size(text.c_str(), fonts::menu_bold);
		const auto padding = dpi::scale(vec2d{ 32.f, 32.f });
		render::filled_rect(center - (text_size + padding) * 0.5f, text_size + padding, { 24, 24, 24, 127 }, dpi::scale(6.f));
		render::text(center.x, center.y, color_t{}.modify_alpha(alpha), render::centered, fonts::menu_bold, text);
	}
	void add(const std::string& str) {
#ifdef __NO_OBF
		MessageBoxA(0, str.c_str(), 0, 0);
#else
		{
			THREAD_SAFE(mtx);
			messages.emplace_back(str);
		}

		is_opened = true;
		/*std::unique_lock l{ mtx };
		cond.wait(l, []() { return is_opened == false; });*/
#endif
	}
}// namespace NS_GUI::msgbox