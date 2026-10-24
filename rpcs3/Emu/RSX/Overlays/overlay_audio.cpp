#include "stdafx.h"
#include "overlay_audio.h"
#include "Emu/emu_callbacks.h"

namespace rsx
{
	namespace overlays
	{
		audio_player::audio_player(const std::string& audio_path, bool audio_in_archive, const std::string& iso_path)
		{
			init_audio(audio_path, audio_in_archive, iso_path);
		}

		void audio_player::init_audio(const std::string& audio_path, bool audio_in_archive, const std::string& iso_path)
		{
			if (audio_path.empty()) return;

			// Frontends without a media player (headless, UWP) return nullptr: skip the music instead of failing
			if (g_emu_callbacks.make_video_source)
			{
				m_video_source = g_emu_callbacks.make_video_source();
			}

			if (!m_video_source)
			{
				rsx_log.notice("audio_player: no media player available, not playing '%s'", audio_path);
				return;
			}

			m_video_source->set_audio_path(audio_path, audio_in_archive);

			if (audio_in_archive)
			{
				m_video_source->set_iso_path(iso_path);
			}
		}

		void audio_player::set_active(bool active)
		{
			if (m_video_source)
			{
				m_video_source->set_active(active);
			}
		}
	}
}
