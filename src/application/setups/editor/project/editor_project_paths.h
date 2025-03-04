#pragma once
#include "augs/filesystem/path.h"
#include "application/arena/intercosm_paths.h"

struct editor_project_paths {
	std::string arena_name;
	augs::path_type project_folder;

	augs::path_type editor_view;

	augs::path_type project_json;
	augs::path_type miniature;

	augs::path_type signature;

	/* cache */
	augs::path_type compressed_json;
	augs::path_type resource_hashes;

	augs::path_type fast_load_bin;

	editor_project_paths(const augs::path_type& target_folder);
};
