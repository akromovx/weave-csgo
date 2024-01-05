#pragma once
//#include "../../deps/http-transmit/httptransmit.h"
#include "../utils/obfuscation.hpp"
#include <map>
#include <optional>
#include <string>
#include <vector>

#define CLOUD XOR32S(50120)
#define DEV XOR32S(52001)
#define RESOURCES XOR32S(50001)

class network final {
private:
	static bool m_setuped;
	//static bool heartbeat();
	static void longpool();
	static bool get_user_id();
	//static inline c_httptransmit* connection;

public:
	static inline int user_id;
	static inline std::string username;
	static inline time_t expire_date;
	static inline std::vector<uint8_t> avatar;

	struct item_t {
		std::string m_id{};
		std::string m_name{};
		std::string m_author{}, m_last_update_by{};
		int m_created_at{}, m_last_update_at{};
		bool m_private{};
		std::string m_last_uploaded_hash{};
	};

	struct config_t : item_t {};
	struct script_t : item_t {
		bool m_exist_on_server{};
		bool m_loaded_on_server{};
	};

	struct user_t {
		int m_id{};
		std::string m_name{};
		std::string m_activity{};
		std::string m_ip{};
	};

	struct online_info_t {
		int m_total{};
		std::vector<user_t> m_users{};
	};

	struct session_info_t {
		int m_session_time{};
		int m_total_time{};
	};

	using configs_t = std::vector<config_t>;
	using scripts_t = std::vector<script_t>;
	using localization_t = std::unordered_map<std::string, std::string>;

	static void on_frame_render_start();

	static std::vector<uint8_t> simple_get(const std::string& url, int service);
	static bool create_connection();

	// menu
	static bool auth();
	static bool setup();
	static bool get(std::vector<uint8_t>& buffer, int tab, int additional = -1);
	static bool search(std::vector<uint8_t>& buffer, const std::string& phrase);

	// configs
	static bool get_configs(configs_t& configs);
	static bool get_config(config_t& cfg, const std::string& id);
	static bool load_config(std::vector<uint8_t>& buffer, const std::string& id);
	static bool rollback_config(std::vector<uint8_t>& buffer, const std::string& id);
	static bool create_config(const std::string& name);
	static bool delete_config(const std::string& id);
	static bool save_config(const std::string& title, const std::string& id, bool personal);
	static bool import_config(const std::string& id);

	// scripts
	static bool get_scripts(scripts_t& script);
	static std::optional<std::string> get_script(const std::string& id);
	static bool create_script(script_t& script, std::string code);
	static bool upload_script(const std::string& id, const std::string& name, const std::string& code, bool personal);
	static bool delete_script(const std::string& id);
	static bool import_script(const std::string& id);

	// resources
	static std::vector<std::string> get_images();
	static std::vector<std::string> get_fonts();

	// misc
	static void get_subscriptions(int user_id);
	static bool download_avatar(int user_id, std::vector<uint8_t>& buffer);
	static std::optional<online_info_t> get_online(bool crop = false);
	static std::optional<session_info_t> get_session();
	static void suicide_note(const std::string& text);
	static std::string get_file_hash(const std::string& name);

	// localization
	static std::optional<localization_t> get_localization();

	static inline std::optional<online_info_t> online{};
};