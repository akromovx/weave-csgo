#pragma once

namespace sdk {
	enum animstate_layer_t {
		ANIMATION_LAYER_AIMMATRIX = 0,
		ANIMATION_LAYER_WEAPON_ACTION,
		ANIMATION_LAYER_WEAPON_ACTION_RECROUCH,
		ANIMATION_LAYER_ADJUST,
		ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL,
		ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB,
		ANIMATION_LAYER_MOVEMENT_MOVE,
		ANIMATION_LAYER_MOVEMENT_STRAFECHANGE,
		ANIMATION_LAYER_WHOLE_BODY,
		ANIMATION_LAYER_FLASHED,
		ANIMATION_LAYER_FLINCH,
		ANIMATION_LAYER_ALIVELOOP,
		ANIMATION_LAYER_LEAN,
		ANIMATION_LAYER_COUNT,
	};

	struct animation_layer_t {
		bool m_client_blend;
		float m_blend_in;
		void* m_studio_hdr;
		int m_dispatched_src;
		int m_dispatched_dst;
		unsigned int m_order;
		unsigned int m_sequence;
		float m_prev_cycle;
		float m_weight;
		float m_weight_delta_rate;
		float m_playback_rate;
		float m_cycle;
		void* m_owner;
		char pad_0038[4];
	};

	using animation_layers_t = animation_layer_t[13];
} // namespace sdk