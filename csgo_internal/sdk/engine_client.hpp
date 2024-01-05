#pragma once
#include "../src/utils/displacement.hpp"
#include "../src/utils/utils.hpp"
#include "../src/utils/vector.hpp"

namespace sdk {
	CONSTS(
			flow_outgoing = 0,
			flow_incoming = 1,
			max_flows = 2);

	struct radar_player_t {
		vec3d m_pos;
		vec3d m_angle;
		vec3d m_spotted_map_angle_related;
		uint32_t m_tab_related;
		uint8_t pad_0x0028[0xC];
		float m_spotted_time;
		float m_spotted_fraction;
		float m_time;
		uint8_t pad_0x0040[0x4];
		int32_t m_player_index;
		int32_t m_entity_index;
		char pad_0x004C[0x4];
		int32_t m_health;
		uint8_t m_name[32];
		uint8_t pad_0x0074[0x75];
		bool m_spotted;
		uint8_t pad_0x00EA[0x8A];
	};

	struct csgo_hud_radar_t {
		uint8_t pad_0x0000[0x14C];
		radar_player_t m_radar_info[65];
	};

	struct player_info_t {
		int64_t unknown;

		union {
			int64_t m_steam_id64;
			struct {
				int32_t m_xuid_low;
				int32_t m_xuid_high;
			};
		};

		char m_name[128];
		int m_user_id;
		char m_sz_steam_id[20];
		char pad_0x00A8[0x10];
		unsigned long m_steam_id;
		char m_friends_name[128];
		bool m_fakeplayer;
		bool m_ishltv;
		unsigned int m_customFiles[4];
		unsigned char m_files_downloaded;
	};

	struct net_channel_info_t {
		enum {
			GENERIC = 0,  // must be first and is default group
			LOCALPLAYER,  // bytes for local player entity update
			OTHERPLAYERS, // bytes for other players update
			ENTITIES,	  // all other entity bytes
			SOUNDS,		  // game sounds
			EVENTS,		  // event messages
			TEMPENTS,	  // temp entities
			USERMESSAGES, // user messages
			ENTMESSAGES,  // entity messages
			VOICE,		  // voice data
			STRINGTABLE,  // a stringtable update
			MOVE,		  // client move cmds
			STRINGCMD,	  // string command
			SIGNON,		  // various signondata
			TOTAL		  // must be last and is not a real group
		};

		virtual const char* get_name() const = 0;	  // get channel name
		virtual const char* get_address() const = 0;  // get channel IP address as string
		virtual float get_time() const = 0;			  // current net time
		virtual float get_time_connected() const = 0; // get connection time in seconds
		virtual int get_buffer_size() const = 0;	  // netchannel packet history size
		virtual int get_data_rate() const = 0;		  // send data rate in byte/sec

		virtual bool is_loopback() const = 0;	// true if loopback channel
		virtual bool is_timing_out() const = 0; // true if timing out
		virtual bool is_playback() const = 0;	// true if demo playback

		virtual float get_latency(int flow) const = 0;	   // current latency (RTT), more accurate but jittering
		virtual float get_avg_latency(int flow) const = 0; // average packet latency in seconds
		virtual float get_avg_loss(int flow) const = 0;	   // avg packet loss[0..1]
		virtual float get_avg_choke(int flow) const = 0;   // avg packet choke[0..1]
		virtual float get_avg_data(int flow) const = 0;	   // data flow in bytes/sec
		virtual float get_avg_packets(int flow) const = 0; // avg packets/sec
		virtual int get_total_data(int flow) const = 0;	   // total flow in/out in bytes
		virtual int get_total_packets(int flow) const = 0;
		virtual int get_sequence_nr(int flow) const = 0;								 // last send seq number
		virtual bool is_valid_packet(int flow, int frame_number) const = 0;				 // true if packet was not lost/dropped/chocked/flushed
		virtual float get_packet_time(int flow, int frame_number) const = 0;			 // time when packet was send
		virtual int get_packet_bytes(int flow, int frame_number, int group) const = 0;	 // group size of this packet
		virtual bool get_stream_progress(int flow, int* recieved, int* total) const = 0; // TCP progress if transmitting
		virtual float get_time_since_last_received() const = 0;							 // get time since last recieved packet in seconds
		virtual float get_command_interpolation_amount(int flow, int frame_number) const = 0;
		virtual void get_packet_response_latency(int flow, int frame_number, int* latency_msecs, int* choke) const = 0;
		virtual void get_remote_framerate(float* frametime, float* frametime_std_deviation) const = 0;
		virtual float get_timeout_seconds() const = 0;
	};

	struct net_message_t {
		virtual ~net_message_t() {}
		virtual void set_net_channel(void* pNetChannel) = 0;
		virtual void set_reliable(bool bState) = 0;
		virtual bool process() = 0;
		virtual bool read_from_buffer(/*bf_read& buffer*/) = 0;
		virtual bool write_to_buffer(/*bf_write& buffer*/) = 0;
		virtual bool is_reliable() const = 0;
		virtual int get_type() const = 0;
		virtual int get_group() const = 0;
		virtual const char* get_name() const = 0;
		virtual net_channel_info_t* get_net_channel_info() const = 0;
		virtual const char* to_string() const = 0;
	};

	struct engine_client_t {
		VFUNC(get_screen_size(int& width, int& height), bool(__thiscall*)(decltype(this), int&, int&), 5, width, height);
		VFUNC(get_player_info(int index, player_info_t* pinfo), bool(__thiscall*)(decltype(this), int, player_info_t*), 8, index, pinfo);
		VFUNC(get_player_for_user_id(int index), int(__thiscall*)(decltype(this), int), 9, index);
		VFUNC(is_console_open(), bool(__thiscall*)(decltype(this)), 11);
		VFUNC(get_local_player(), int(__thiscall*)(decltype(this)), 12);
		VFUNC(set_view_angles(const vec3d& angle), void(__thiscall*)(decltype(this), const vec3d&), 19, angle);
		VFUNC(get_max_clients(), int(__thiscall*)(decltype(this)), 20);
		VFUNC(is_in_game(), bool(__thiscall*)(decltype(this)), 26);
		VFUNC(is_connected(), bool(__thiscall*)(decltype(this)), 27);
		VFUNC(get_bsp_tree_query(), void*(__thiscall*)(decltype(this)), 43);
		VFUNC(is_hltv(), bool(__thiscall*)(decltype(this)), 93);
		VFUNC(get_net_channel(), net_channel_info_t*(__thiscall*)(decltype(this)), 78);
		VFUNC(get_engine_build_number(), int(__thiscall*)(decltype(this)), 104);
		VFUNC(execute_client_cmd(const char* cmd), void(__thiscall*)(decltype(this), const char*), 108, cmd);
		VFUNC(client_cmd_unrestricted(const char* cmd, const char* new_flag = 0), void(__thiscall*)(decltype(this), const char*, const char*), 114, cmd, new_flag);

		__forceinline vec3d get_view_angles() {
			vec3d angle;
			utils::vfunc<void(__thiscall*)(void*, vec3d&)>(this, XOR32(18))(this, angle);
			return angle;
		}
	};
} // namespace sdk