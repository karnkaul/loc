#include <algorithm>
#include <common.hpp>
#include <file_list_generator.hpp>

namespace
{
bool skip_file(stdfs::path const& path)
{
	if (!stdfs::is_regular_file(path))
	{
		return true;
	}
	auto const ext = path.extension();
	if (ext.empty())
	{
		return true;
	}
	auto const ext_str = ext.generic_string();
	if (std::any_of(cfg::g_skip_exts.begin(), cfg::g_skip_exts.end(), [ext_str](auto skip) -> bool { return skip == ext_str; }))
	{
		return true;
	}
	auto p = path;
	while (!p.empty() && p.has_parent_path())
	{
		auto const name = p.filename().generic_string();
		auto const path = p.generic_string();
		if (name.size() > 1 && name.at(0) == '.' && name.at(1) != '.')
		{
			return true;
		}
		if (std::any_of(cfg::g_skip_substrs.begin(), cfg::g_skip_substrs.end(), [path](auto skip) -> bool { return path.find(skip) != locc::null_index; }))
		{
			return true;
		}
		p = p.parent_path();
	}
	if (path.generic_string().find(".git") != std::string::npos)
	{
		return true;
	}
	return false;
}
} // namespace

std::deque<stdfs::path> locc::file_list(std::deque<locc::entry> const& entries)
{
	std::deque<stdfs::path> ret;
	bool reading_files = false;
	for (auto& [key, value] : entries)
	{
		if (!reading_files)
		{
			auto k = key;
			if (key == "." || key == "..")
			{
				k += "/";
			}
			if (stdfs::is_directory(stdfs::absolute(k)))
			{
				auto recurse = stdfs::recursive_directory_iterator(k, stdfs::directory_options::skip_permission_denied);
				for (auto iter = stdfs::begin(recurse); iter != stdfs::end(recurse); ++iter)
				{
					try
					{
						if (!iter->is_symlink())
						{
							auto path = iter->path();
							if (!skip_file(path))
							{
								locc::log(cfg::test(cfg::flag::debug), "  -- tracking ", path.generic_string(), "\n");
								ret.push_back(std::move(path));
							}
						}
					}
					catch (std::exception const& e)
					{
						locc::err("Exception caught: ", e.what(), "\n");
						continue;
					}
				}
				return ret;
			}
			reading_files = true;
		}
		if (reading_files)
		{
			if (!skip_file(stdfs::absolute(key)))
			{
				ret.push_back(std::move(key));
			}
		}
	}
	return ret;
}
