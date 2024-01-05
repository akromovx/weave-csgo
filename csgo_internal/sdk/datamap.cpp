#include "datamap.hpp"
#include <cstring>
#include "../src/utils/fnva1.hpp"

namespace sdk {
	uint32_t find_in_datamap(datamap_t* map, uint32_t hash) {
		while (map != nullptr) {
			for (int i = 0; i < map->m_data_num_fields; i++) {
				auto& data_desc = map->m_data_desc[i];
				if (data_desc.m_field_name == NULL)
					continue;

				if (fnva1(data_desc.m_field_name) == hash)
					return data_desc.m_field_offset[TD_OFFSET_NORMAL];

				if (data_desc.m_field_type == 10) {
					if (data_desc.m_td) {
						uint32_t offset{};
						if ((offset = find_in_datamap(data_desc.m_td, hash)) != 0)
							return offset;
					}
				}
			}

			map = map->m_base_map;
		}
		return 0;
	}
} // namespace sdk