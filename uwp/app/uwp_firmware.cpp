// PS3 firmware (PS3UPDAT.PUP) installation without Qt. Same steps as desktop RPCS3's
// main_window::HandlePupInstallation: validate the PUP, take the update_files TAR, decrypt
// each dev_flash_* package and extract it into dev_flash.

#include "stdafx.h"
#include "uwp_firmware.h"

#include "Crypto/key_vault.h"
#include "Crypto/unself.h"
#include "Emu/VFS.h"
#include "Emu/vfs_config.h"
#include "Loader/PUP.h"
#include "Loader/TAR.h"
#include "Utilities/File.h"
#include "util/sysinfo.hpp"

#include <algorithm>

LOG_CHANNEL(fw_log, "UWPFW");

namespace uwp_firmware
{
	std::string install(const std::string& path, progress& prog)
	{
		fs::file pup_f(path);
		if (!pup_f)
		{
			fw_log.error("Error opening PUP file %s (%s)", path, fs::g_tls_error);
			return "The firmware file couldn't be opened.";
		}

		pup_object pup(std::move(pup_f));

		switch (pup.operator pup_error())
		{
		case pup_error::header_read:
			fw_log.error("%s", pup.get_formatted_error());
			return "The firmware file is empty.";
		case pup_error::header_magic:
			fw_log.error("Not a PUP file: %s", path);
			return "The file is not a PS3 firmware (PUP) file.";
		case pup_error::expected_size:
			fw_log.error("%s", pup.get_formatted_error());
			return "The firmware file is incomplete. Try downloading it again.";
		case pup_error::header_file_count:
		case pup_error::file_entries:
		case pup_error::stream:
			fw_log.error("PUP file is invalid: %s", pup.get_formatted_error());
			return "The firmware file is corrupted.";
		case pup_error::hash_mismatch:
			fw_log.error("PUP hash check failed");
			return "The firmware file's contents are corrupted.";
		case pup_error::ok:
			break;
		}

		fs::file update_files_f = pup.get_file(0x300);
		const usz update_files_size = update_files_f ? update_files_f.size() : 0;
		if (!update_files_size)
		{
			fw_log.error("Couldn't find the installation packages database");
			return "The firmware file's contents are corrupted.";
		}

		fs::device_stat dev_stat{};
		if (fs::statfs(g_cfg_vfs.get_dev_flash(), dev_stat) && dev_stat.avail_free < update_files_size)
		{
			fw_log.error("Out of disk space ('%s', needed: %d bytes)", g_cfg_vfs.get_dev_flash(), update_files_size - dev_stat.avail_free);
			return "Not enough free space.";
		}

		tar_object update_files(update_files_f);

		// The main TAR holds dev_flash_* entries, which are encrypted TARs of the dev_flash files.
		auto names = update_files.get_filenames();
		names.erase(std::remove_if(names.begin(), names.end(), [](const std::string& s) { return s.find("dev_flash_") == umax; }), names.end());
		if (names.empty())
		{
			fw_log.error("No dev_flash_* packages were found");
			return "The firmware file's contents are corrupted.";
		}

		std::string version;
		if (fs::file v = pup.get_file(0x100))
			version = v.to_string();
		if (const usz pos = version.find('\n'); pos != umax)
			version.erase(pos);
		if (version.empty())
		{
			fw_log.error("No version data was found");
			return "The firmware file's contents are corrupted.";
		}

		prog.version = version;
		prog.total = ::size32(names);
		prog.done = 0;

		if (const std::string installed = utils::get_firmware_version(); !installed.empty())
			fw_log.warning("Reinstalling firmware: old=%s, new=%s", installed, version);
		fw_log.notice("Installing firmware %s from %s (%u packages)", version, path, prog.total.load());

		// tar_object::extract() writes through this mount point.
		fs::create_path(g_cfg_vfs.get_dev_flash());
		vfs::mount("/dev_flash", g_cfg_vfs.get_dev_flash());

		for (const auto& name : names)
		{
			auto stream = update_files.get_file(name);
			if (stream->m_file_handler)
			{
				// Forcefully read all the data
				stream->m_file_handler->handle_file_op(*stream, 0, stream->get_size(umax), nullptr);
			}

			fs::file update_file = fs::make_stream(std::move(stream->data));

			SCEDecrypter self_dec(update_file);
			self_dec.LoadHeaders();
			self_dec.LoadMetadata(SCEPKG_ERK, SCEPKG_RIV);
			self_dec.DecryptData();

			auto dev_flash_tar_f = self_dec.MakeFile();
			if (dev_flash_tar_f.size() < 3)
			{
				fw_log.error("PUP contents are invalid (package=%s)", name);
				return "The firmware could not be decompressed.";
			}

			tar_object dev_flash_tar(dev_flash_tar_f[2]);
			if (!dev_flash_tar.extract())
			{
				fw_log.error("TAR contents are invalid (package=%s)", name);
				return "The firmware contents could not be extracted.";
			}

			prog.done++;
		}

		fw_log.success("Installed PS3 firmware version %s", version);
		return {};
	}
}
