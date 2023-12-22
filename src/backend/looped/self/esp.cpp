#include "backend/looped_command.hpp"

namespace big
{
	class esp : looped_command
	{
		using looped_command::looped_command;

		virtual void on_tick() override
		{
			//LOG(INFO) << "ESP ontick";
			//g.esp.enabled = !g.esp.enabled;
		}
	};

	esp g_esp("esp", "VIEW_OVERLAY_ESP", "BACKEND_LOOPED_ESP_DESC", g.esp.enabled);
}