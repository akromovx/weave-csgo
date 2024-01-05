#pragma once
#include <cstdint>

namespace sdk {
	enum {
		TD_OFFSET_NORMAL = 0,
		TD_OFFSET_PACKED = 1,
		TD_OFFSET_COUNT,
	};

	struct datamap_t;
	struct typedescription_t {
		int32_t m_field_type;
		char* m_field_name;
		int m_field_offset[TD_OFFSET_COUNT];
		int16_t m_fieldSize_unk;
		int16_t m_flags_unk;
		char pad_0014[12];
		datamap_t* m_td;
		char pad_0024[24];
	};

	struct datamap_t {
		typedescription_t* m_data_desc;
		int m_data_num_fields;
		char const* m_data_class_name;
		datamap_t* m_base_map;
		bool m_chains_validated;
		bool m_packed_offsets_computed;
		int m_packed_size;
	};

	extern uint32_t find_in_datamap(datamap_t* map, uint32_t hash);
} // namespace sdk