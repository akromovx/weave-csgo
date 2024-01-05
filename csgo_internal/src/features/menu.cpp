#include "menu.hpp"
#include <mutex>
#include "misc.hpp"
#include "../interfaces.hpp"
#include "../../deps/weave-gui/include.hpp"

using namespace gui;

namespace menu {
	static void welcome_tab() {
		ImGui::Text(STRC("Welcome!\nCheat has been loaded and ready to use\n\nBuild date: %s %s"), STRC(__DATE__), STRC(__TIME__));
	}

	static void ragebot_tab() {
		static int group_tab = 0;

		ImGui::Columns(2, nullptr, false);

		begin_child(STR("General"), dpi::scale(200.f));
		{
			ImGui::Checkbox(STRC("Enable"), &settings->ragebot.enable);
			if (settings->ragebot.enable) {
				ImGui::Checkbox(STRC("Auto-fire"), &settings->ragebot.autofire);
				ImGui::Checkbox(STRC("Silent"), &settings->ragebot.silent);
#ifdef __NO_OBF
				ImGui::Checkbox(STRC("Multi-threading"), &settings->multithreading);
#endif
			}
		}
		end_child();

		begin_child(STR("Weapons"), dpi::scale(400.f));
		{
			if (settings->ragebot.enable) {
				combo(STR("Weapon group"), &group_tab, { STR("Default"), STR("Snipers"), STR("Auto-snipers"), STR("Heavy pistols"), STR("Pistols"), STR("Rifles"), STR("Heavies"), STR("Shotguns"), STR("SMGs") });

				group_tab = std::clamp(group_tab, 0, 8);
				auto weapon = &settings->ragebot.weapons[group_tab];

				if (group_tab > 0) {
					switch (group_tab) {
						case incheat_vars::rage_group_snipers:
							combo(STR("Current weapon"), &weapon->tab, { STR("Default"), STR("SSG-08"), STR("AWP") });
							break;

						case incheat_vars::rage_group_autosnipers:
							combo(STR("Current weapon"), &weapon->tab, { STR("Default"), STR("G3SG1"), STR("SCAR-20") });
							break;

						case incheat_vars::rage_group_heavy_pistols:
							combo(STR("Current weapon"), &weapon->tab, { STR("Default"), STR("Desert Eagle"), STR("Revolver") });
							break;

						case incheat_vars::rage_group_pistols:
							combo(STR("Current weapon"), &weapon->tab,
								  { STR("Default"), STR("Glock-18"), STR("P2000"), STR("USP-S"), STR("Dual Berettas"), STR("P250"), STR("Tec-9"), STR("Five-Seven"), STR("CZ-75") });
							break;

						case incheat_vars::rage_group_rifles:
							combo(STR("Current weapon"), &weapon->tab,
								  { STR("Default"), STR("Galil AR"), STR("Famas"), STR("AK-47"), STR("M4A4"), STR("M4A1-S"), STR("SG-553"), STR("AUG") });
							break;

						case incheat_vars::rage_group_heavies:
							combo(STR("Current weapon"), &weapon->tab, { STR("Default"), STR("M249"), STR("Negev") });
							break;

						case incheat_vars::rage_group_shotguns:
							combo(STR("Current weapon"), &weapon->tab, { STR("Default"), STR("Nova"), STR("XM1014"), STR("Sawed-off"), STR("MAG-7") });
							break;

						case incheat_vars::rage_group_smgs:
							combo(STR("Current weapon"), &weapon->tab,
								  { STR("Default"), STR("Mac-10"), STR("MP9"), STR("MP7"), STR("MP5-SD"), STR("UMP-45"), STR("P90"), STR("PP-Bizon") });
							break;
					}

					if (weapon->tab == 0)
						ImGui::Checkbox(STRC("Override group"), &weapon->override_default);
				} else
					weapon->tab = 0;

				auto set = &weapon->settings[weapon->tab];

				if (group_tab > 0 && weapon->tab != 0)
					ImGui::Checkbox(STRC("Override weapon"), &set->override_default);

				if (group_tab == 0 || settings->ragebot.weapons[group_tab].override_default || set->override_default) {
					ImGui::Checkbox(STRC("Quick stop"), &set->quick_stop);
					ImGui::Checkbox(STRC("Automatic scope"), &set->autoscope);
					//ImGui::Checkbox(STRC("Visible only"), &set->visible_only);

					multicombo(STR("Hitboxes"), &set->hitboxes, {
																		STR("Head"),
																		STR("Upper chest"),
																		STR("Chest"),
																		STR("Lower chest"),
																		STR("Pelvis"),
																		STR("Stomach"),
																		STR("Legs"),
																		STR("Feet"),
																		STR("Arms"),
																});

					slider_int(STR("Hit chance"), &set->hitchance, 0, 100, STR("%d%%"));

					if (set->hitchance > 0)
						ImGui::Checkbox(STRC("Strict hitchance"), &set->strict_hitchance);

					slider_int(STR("Minimum damage"), &set->mindamage, 0, 120, set->mindamage > 100 ? STR("Health + %dhp") : STR("%dhp"));

					if (hotkeys->override_damage.is_valid())
						slider_int(STR("Minimum damage override"), &set->mindamage_override, 0, 120, STR("%dhp"));

					slider_int(STR("Head scale"), &set->head_scale, 0, 100, set->head_scale == 0 ? STR("Auto") : STR("%d%%"));
					slider_int(STR("Body scale"), &set->body_scale, 0, 100, set->body_scale == 0 ? STR("Auto") : STR("%d%%"));

					multicombo(STR("Force safepoint"), &set->force_safepoint, {
																					  STR("Head"),
																					  STR("Body"),
																					  STR("Limbs"),
																			  });

					ImGui::Checkbox(STRC("Prefer safepoint"), &set->prefer_safepoint);

					ImGui::Spacing();
					ImGui::Separator();
					ImGui::Spacing();

					if (group_tab > 0) {
						if (ImGui::Button(STRC("Import from default"))) {
							std::memcpy(set, &settings->ragebot.weapons[0].settings[0], sizeof(incheat_vars::ragebot_settings_t));
							set->override_default = true;
						}

						if (weapon->settings[0].override_default) {
							if (ImGui::Button(STRC("Import from group"))) {
								std::memcpy(set, &weapon->settings[0], sizeof(incheat_vars::ragebot_settings_t));
								set->override_default = true;
							}
						}
					}
				}
			}
		}
		end_child();

		ImGui::NextColumn();

		begin_child(STR("Anti-aims"), dpi::scale(400.f));
		{
			static int trigger_tab = 0;
			ImGui::Checkbox(STRC("Enable"), &settings->antiaim.enable);
			multicombo(STR("Defensive options"), &settings->exploits.defensive_flags, { STR("On ground"), STR("In air"), STR("Safe peek"), STR("Anti-aim") });
			ImGui::Checkbox(STRC("Immediate teleport"), &settings->exploits.immediate_teleport);

			if (settings->antiaim.enable) {
				combo(STR("Trigger"), &trigger_tab, { STR("Default"), STR("Standing"), STR("Moving"), STR("Jumping"), STR("Slow walking"), STR("Crouching") });
				auto set = &settings->antiaim.triggers[trigger_tab];
				if (trigger_tab > 0)
					ImGui::Checkbox(STRC("Override default"), &set->override_default);

				if (trigger_tab == 0 || set->override_default) {
					slider_int(STR("Desync"), &set->desync_amount, 0, 100, STR("%d%%"));
					combo(STR("Align by target"), &set->align_by_target, { STR("Off"), STR("When on screen"), STR("Always") });
					//ImGui::Checkbox(STRC("Align by target"), &set->align_by_target);
					slider_int(STR("Yaw add"), &set->yaw_add, -180, 180);
					slider_int(STR("Jitter range"), &set->jitter_angle, -90, 90);
					if (set->jitter_angle > 2)
						ImGui::Checkbox(STRC("Randomize jitter"), &set->randomize_jitter);

					//ImGui::Checkbox(STRC("Align desync"), &set->align_desync);
					ImGui::Checkbox(STRC("Desync jitter"), &set->desync_jitter);
					ImGui::Checkbox(STRC("Spin"), &set->spin);
					if (set->spin) {
						slider_int(STR("Spin speed"), &set->spin_speed, 1, 90);
						slider_int(STR("Spin range"), &set->spin_range, 2, 90);
					}

					//ImGui::Checkbox(STRC("Edge yaw on peek"), &set->edge_yaw_on_peek);
				}
			}
		}
		end_child();

		begin_child(STR("Other"), dpi::scale(200.f));
		{
			combo(STR("Fake-lag"), &settings->antiaim.fakelag, { STR("Off"), STR("Static"), STR("Dynamic") });
		}
		end_child();
	}

	namespace esp {
		static void glow_settings(std::string name, incheat_vars::glow_esp_settings_t& set) {
			checkbox_with_color(name + STR("##") + std::to_string((uintptr_t)&set), &set.enable, set.color.base());
		}

		static void bar_settings(std::string name, incheat_vars::esp::bar_t& set) {
			std::string id = STR("##") + name;
			ImGui::Checkbox(name.c_str(), &set.value);

			if (set.value) {
				ImGui::SameLine();
				if (ImGui::Button((STR("...") + id).c_str()))
					ImGui::OpenPopup(ImGui::GetID(id.c_str()));
			}

			if (ImGui::BeginPopup(id.c_str())) {
				if (&set == &settings->player_esp.health) {
					ImGui::Checkbox(STRC("Custom color"), &settings->player_esp.override_health_color);

					if (settings->player_esp.override_health_color) {
						ImGui::SameLine();
						ImGui::ColorEdit3(STRC("##health_color1"), set.colors[0].base());
						ImGui::SameLine();
						ImGui::ColorEdit3(STRC("##health_color2"), set.colors[1].base());
					}
				} else {
					ImGui::Text(STRC("Color"));
					ImGui::SameLine();
					ImGui::ColorEdit3(STRC("##first_color"), set.colors[0].base());
					ImGui::SameLine();
					ImGui::ColorEdit3(STRC("##second_color"), set.colors[1].base());
				}

				combo(STR("Position"), &set.position, { STR("Bottom"), STR("Top"), STR("Left"), STR("Right") });

				ImGui::EndPopup();
			}
		}

		static void text_settings(std::string name, incheat_vars::esp::text_t& set) {
			std::string id = STR("##") + name;
			ImGui::Checkbox(name.c_str(), &set.value);

			if (set.value) {
				ImGui::SameLine();
				if (ImGui::Button((STR("...") + id).c_str()))
					ImGui::OpenPopup(ImGui::GetID(id.c_str()));
			}

			if (ImGui::BeginPopup(id.c_str())) {
				ImGui::Text(STRC("Color"));
				ImGui::SameLine();
				ImGui::ColorEdit3(STRC("##color1"), set.colors[0].base());
				ImGui::SameLine();
				ImGui::ColorEdit3(STRC("##color2"), set.colors[1].base());

				combo(STR("Position"), &set.position, { STR("Bottom"), STR("Top"), STR("Left"), STR("Right") });

				combo(STR("Font"), &set.font, {
													  STR("Default"),
													  STR("Bold"),
													  STR("Small pixel"),
											  });

				ImGui::EndPopup();
			}
		}

		static void text_array_settings(std::string name, std::vector<std::string> elements, incheat_vars::esp::text_array_t& set) {
			std::string id = STR("##") + name;
			multicombo(name, &set.value, elements);

			if (set.value.get() != 0) {
				ImGui::SameLine();
				if (ImGui::Button((STR("...") + id).c_str()))
					ImGui::OpenPopup(ImGui::GetID(id.c_str()));
			}

			if (ImGui::BeginPopup(id.c_str())) {
				ImGui::ColorEdit3(STRC("Color"), set.colors[0].base());
				combo(STR("Position"), &set.position, { STR("Bottom"), STR("Top"), STR("Left"), STR("Right") });

				combo(STR("Font"), &set.font, {
													  STR("Default"),
													  STR("Bold"),
													  STR("Small pixel"),
											  });

				ImGui::EndPopup();
			}
		}
	} // namespace esp

	static void players_tab() {
		ImGui::Columns(2, nullptr, false);

		begin_child(STR("ESP"));
		{
			ImGui::Checkbox(STRC("Enable"), &settings->player_esp.enable);
			if (settings->player_esp.enable) {
				checkbox_with_color(STR("Box"), &settings->player_esp.box, settings->player_esp.box_color.base());
				esp::text_settings(STR("Name"), settings->player_esp.name);
				esp::bar_settings(STR("Health"), settings->player_esp.health);
				esp::bar_settings(STR("Ammo"), settings->player_esp.ammo);

				esp::text_array_settings(STR("Flags"),
										 { STR("Helmet & Kevlar"), STR("Scope"), STR("Flash"), STR("Defuser"), STR("Distance"), STR("Ping") },
										 settings->player_esp.flags);

				esp::text_array_settings(STR("Weapon"), { STR("Name"), STR("Icon") }, settings->player_esp.weapon);
			}
		}
		end_child();

		ImGui::NextColumn();

		begin_child(STR("Models"));
		{
			static int chams_tab = 0;

			combo(STR("Chams"), &chams_tab, {
													STR("Enemy Visible"),
													STR("Enemy XQZ"),
													STR("Local player"),
													STR("Enemy Backtrack"),
													STR("Shot"),
													STR("Viewmodel arms"),
													STR("Viewmodel weapon"),
													STR("Local attachments"),
											});

			auto& set = settings->player_esp.chams[chams_tab];
			ImGui::Checkbox(STRC("Enable"), &set.enable);

			if (chams_tab == 2) {
				ImGui::Checkbox(STRC("Transparent in scope"), &settings->player_esp.local_blend_in_scope);

				if (settings->player_esp.local_blend_in_scope)
					slider_int(STR("Transparency amount"), &settings->player_esp.local_blend_in_scope_amount, 0, 100, STR("%d%%"));
			}

			if (set.enable) {
				if ((chams_tab == 3 || chams_tab == 4) && !settings->ragebot.enable) {
					ImGui::Text(STRC("Please turn on ragebot before setup this feature!"));
				} else {
					if (chams_tab == 4)
						ImGui::Checkbox(STRC("Last shot only"), &settings->player_esp.shot_chams_last_only);

					combo(STR("Material"), &set.material, { STR("Default"), STR("Flat"), STR("Metallic"), STR("Randomized") });

					ImGui::ColorEdit4(STRC("Material color"), set.color.base(), ImGuiColorEditFlags_AlphaBar);

					if (set.material == 2) {
						ImGui::ColorEdit4(STRC("Metallic color"), set.metallic_color.base(), ImGuiColorEditFlags_AlphaBar);

						slider_int(STR("Phong"), &set.phong_amount, 0, 100, STR("%d%%"));
						slider_int(STR("Rim"), &set.rim_amount, -100, 100, STR("%d%%"));
					}

					const std::vector<std::string> overlays_name = {
						STR("Glow fade"),
						STR("Glow line"),
						STR("Wireframe"),
						STR("Glass"),
						STR("Animated")
					};

					multicombo(STR("Overlays"), &set.overlays, overlays_name);

					for (uint8_t i = 0; i < overlays_name.size(); ++i)
						if (set.overlays.at(i))
							ImGui::ColorEdit4((overlays_name[i] + STR(" color")).c_str(), set.overlays_colors[i].base(), ImGuiColorEditFlags_AlphaBar);
				}
			}

			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			esp::glow_settings(STR("Enemy glow"), settings->player_esp.glow);
			esp::glow_settings(STR("Local glow"), settings->player_esp.local_glow);
		}
		end_child();
	}

	static void world_tab() {
		ImGui::Columns(2, nullptr, false);

		begin_child(STR("World modulation"), dpi::scale(250.f));
		{
			ImGui::Checkbox(STRC("Night mode"), &settings->visuals.nightmode.enable);
			if (settings->visuals.nightmode.enable) {
				ImGui::ColorEdit3(STRC("World color"), settings->visuals.nightmode.color.base());
				ImGui::ColorEdit3(STRC("Skybox color"), settings->visuals.nightmode.skybox_color.base());
				ImGui::ColorEdit4(STRC("Props color"), settings->visuals.nightmode.prop_color.base(), ImGuiColorEditFlags_AlphaBar);
			}
		}
		end_child();

		begin_child(STR("Weapons & Projectiles"), dpi::scale(250.f));
		{
			ImGui::Checkbox(STRC("Weapon ESP"), &settings->weapon_esp.enable);
			if (settings->weapon_esp.enable) {
				checkbox_with_color(STR("Box"), &settings->weapon_esp.box, settings->weapon_esp.box_color.base());

				esp::text_array_settings(STR("Name"), { STR("Text"), STR("Icon") }, settings->weapon_esp.name);
				esp::bar_settings(STR("Ammo"), settings->weapon_esp.ammo);

				esp::glow_settings(STR("Glow"), settings->weapon_esp.glow);
			}

			ImGui::Separator();

			ImGui::Checkbox(STRC("Projectiles warning"), &settings->grenade_esp.enable);
		}
		end_child();

		ImGui::NextColumn();

		begin_child(STR("Other"));
		{
			static std::vector<std::string> knife_models = {
				STR("Default"), STR("Bayonet"), STR("Bowie Knife"), STR("Butterfly Knife"),
				STR("Falchion Knife"), STR("Flip Knife"), STR("Gut Knife"),
				STR("Huntsman Knife"), STR("Karambit"), STR("M9 Bayonet"),
				STR("Shadow Daggers"), STR("Navaja Knife"), STR("Stiletto Knife"),
				STR("Ursus Knife"), STR("Talon Knife"), STR("Classic Knife"),
				STR("Skeleton Knife"), STR("Nomad Knife"), STR("Survival Knife"), STR("Paracord Knife")
			};

			static const std::vector<std::string> agent_skins = {
				STR("Default"),
				STR("Special Agent Ava"),
				STR("Operator"),
				STR("Markus Delrow"),
				STR("Michael Syfers"),
				STR("B Squadron Officer"),
				STR("Seal Team 6 Soldier"),
				STR("Buckshot"),
				STR("Lt. Commander Ricksaw"),
				STR("Third Commando Company"),
				STR("'Two Times' McCoy"),
				STR("Dragomir"),
				STR("Rezan The Ready"),
				STR("'The Doctor' Romanov"),
				STR("Maximus"),
				STR("Blackwolf"),
				STR("The Elite Mr. Muhlik"),
				STR("Ground Rebel"),
				STR("Osiris"),
				STR("Prof. Shahmat"),
				STR("Enforcer"),
				STR("Slingshot"),
				STR("Soldier"),
				STR("Street Soldier"),
				STR("'Blueberries' Buckshot"),
				STR("'Two Times' McCoy"),
				STR("Rezan the Redshirt"),
				STR("Dragomir"),
				STR("Cmdr. Mae 'Dead Cold' Jamison"),
				STR("001st Lieutenant Farlow"),
				STR("John 'Van Healen' Kask"),
				STR("Bio-Haz Specialist"),
				STR("Sergeant Bombson"),
				STR("Chem-Haz Specialist"),
				STR("Sir Bloody Miami Darryl"),
				STR("Sir Bloody Silent Darryl"),
				STR("Sir Bloody Skullhead Darryl"),
				STR("Sir Bloody Darryl Royale"),
				STR("Sir Bloody Loudmouth Darryl"),
				STR("Safecracker Voltzmanns"),
				STR("Little Kev"),
				STR("Number K"),
				STR("Getaway Sally"),
				STR("Cmdr. Davida 'Goggles' Fernandez"),
				STR("Cmdr. Frank 'Wet Sox' Baroud"),
				STR("Lieutenant Rex Krikey"),
				STR("Sous-Lieutenant Medic"),
				STR("Chem-Haz Capitaine"),
				STR("Chef d'Escadron Rouchard"),
				STR("Aspirant"),
				STR("Officer Jacques Beltram"),
				STR("D Squadron Officer"),
				STR("Primeiro Tenente"),
				STR("Lieutenant 'Tree Hugger' Farlow"),
				STR("Bloody Darryl The Strapped"),
				STR("Mr. Muhlik"),
				STR("Trapper"),
				STR("Trapper Aggressor"),
				STR("Vypa Sista of the Revolution"),
				STR("Col. Mangos Dabisi"),
				STR("'Medium Rare' Crasswater"),
				STR("Crasswater The Forgotten"),
				STR("Elite Trapper Solman"),
				STR("tm_pirate"),
				STR("tm_pirate_varianta"),
				STR("tm_pirate_variantb"),
				STR("tm_pirate_variantc"),
				STR("tm_pirate_variantd"),
				STR("tm_anarchist"),
				STR("tm_anarchist_varianta"),
				STR("tm_anarchist_variantb"),
				STR("tm_anarchist_variantc"),
				STR("tm_anarchist_variantd"),
				STR("tm_balkan_varianta"),
				STR("tm_balkan_variantb"),
				STR("tm_balkan_variantc"),
				STR("tm_balkan_variantd"),
				STR("tm_balkan_variante"),
				STR("tm_jumpsuit_varianta"),
				STR("tm_jumpsuit_variantb"),
				STR("tm_jumpsuit_variantc"),
			};

			combo(STR("Override knife"), &settings->skins.knife_model, knife_models);
			combo(STR("Agent T"), &settings->skins.agent_t, agent_skins);
			combo(STR("Agent CT"), &settings->skins.agent_ct, agent_skins);

			combo(STR("Bullet tracer"), &settings->bullets.tracer, { STR("Off"), STR("Line"), STR("Beam") });

			ImGui::Checkbox(STRC("Server impacts"), &settings->bullets.server_impacts);
			if (settings->bullets.server_impacts) {
				for (auto& color: settings->bullets.server_impact_colors) {
					ImGui::SameLine();
					ImGui::ColorEdit4((STR("##color") + std::to_string((uintptr_t)color.base())).c_str(), color.base(), ImGuiColorEditFlags_AlphaBar);
				}
			}

			ImGui::Checkbox(STRC("Client impacts"), &settings->bullets.client_impacts);

			if (settings->bullets.client_impacts) {
				for (auto& color: settings->bullets.client_impact_colors) {
					ImGui::SameLine();
					ImGui::ColorEdit4((STR("##color") + std::to_string((uintptr_t)color.base())).c_str(), color.base(), ImGuiColorEditFlags_AlphaBar);
				}
			}

			if (settings->bullets.server_impacts || settings->bullets.client_impacts)
				slider_int(STR("Impacts size"), &settings->bullets.impacts_size, 1, 30);

			ImGui::Separator();

			multicombo(STR("Removals"), &settings->visuals.removals, { STR("Visual recoil"), STR("Smoke"), STR("Flash"), STR("Scope"), STR("Post processing"), STR("Fog"), STR("World shadow"), STR("Foot shadow"), STR("Viewmodel bob") });

			if (settings->visuals.removals.at(3)) {
				combo(STR("Override scope"), &settings->visuals.override_scope, {
																						STR("Off"),
																						STR("Default"),
																						STR("Static"),
																				});

				if (settings->visuals.override_scope > 1) {
					ImGui::ColorEdit4(STRC("Scope color-in"), settings->visuals.scope_color[0].base(), ImGuiColorEditFlags_AlphaBar);
					ImGui::ColorEdit4(STRC("Scope color-out"), settings->visuals.scope_color[1].base(), ImGuiColorEditFlags_AlphaBar);

					slider_int(STR("Scope size"), &settings->visuals.scope_size, 2, 300);
					slider_int(STR("Scope thickness"), &settings->visuals.scope_thickness, 1, 5);
					slider_int(STR("Scope gap"), &settings->visuals.scope_gap, 0, 100);
				}
			}

			slider_int(STR("World FOV"), &settings->visuals.world_fov, 0, 50);
			slider_int(STR("Zoom FOV"), &settings->visuals.zoom_fov, 0, 100, STR("%d%%"));
		}
		end_child();
	}

	static void hotkeys_tab() {
		ImGui::Columns(2, nullptr, false);

		ImGui::Checkbox(STRC("Hotkeys list"), &settings->misc.hotkeys_list);
		ImGui::Spacing();

		if (settings->ragebot.enable) {
			ImGui::Text(STRC("Ragebot"));
			//	ImGui::Separator();
			{
				hotkey(STR("Double-tap"), &hotkeys->doubletap);
				hotkey(STR("Hide-shot"), &hotkeys->hide_shot);
				hotkey(STR("Override damage"), &hotkeys->override_damage);
			}
		}

		ImGui::Text(STRC("Movement"));
		//ImGui::Separator();
		{
			hotkey(STR("Peek assist"), &hotkeys->peek_assist);
			hotkey(STR("Fake duck"), &hotkeys->fake_duck);
		}

		ImGui::Text(STRC("Other"));
		//ImGui::Separator();
		{
			hotkey(STR("Thirdperson"), &hotkeys->thirdperson);
		}

		ImGui::NextColumn();

		if (settings->antiaim.enable) {
			ImGui::Text(STRC("Anti-aim"));
			//	ImGui::Separator();
			{
				hotkey(STR("Manual right"), &hotkeys->manual_right);
				hotkey(STR("Manual left"), &hotkeys->manual_left);
				hotkey(STR("Manual back"), &hotkeys->manual_back);
				hotkey(STR("Manual forward"), &hotkeys->manual_forward);

				hotkey(STR("Desync inverter"), &hotkeys->desync_inverter);
				hotkey(STR("Slow walk"), &hotkeys->slow_walk);
				hotkey(STR("Freestand"), &hotkeys->freestand);
			}
		}
	}

	static void misc_tab() {
		ImGui::Columns(2, nullptr, false);

		begin_child(STR("Movement"), dpi::scale(200.f));
		{
			ImGui::Checkbox(STRC("Bunny-hop"), &settings->movement.bhop);
			ImGui::Checkbox(STRC("Auto-strafe"), &settings->movement.autostrafe);

			if (settings->movement.autostrafe)
				slider_int(STR("Strafe smoothness"), &settings->movement.strafe_smooth, 0, 200);

			ImGui::Checkbox(STRC("Fast stop"), &settings->movement.fast_stop);
			combo(STR("Legs movement"), &settings->movement.leg_movement, { STR("Avoid slide"), STR("Always slide") });

			ImGui::Spacing();
			ImGui::Text(STRC("Peek assist"));
			ImGui::SameLine();
			ImGui::ColorEdit4(STRC("##peek_assist_color_0"), settings->movement.peek_assist_colors[0].base(), ImGuiColorEditFlags_AlphaBar);
			ImGui::SameLine();
			ImGui::ColorEdit4(STRC("##peek_assist_color_1"), settings->movement.peek_assist_colors[1].base(), ImGuiColorEditFlags_AlphaBar);
			ImGui::Checkbox(STRC("Retreat on key release"), &settings->movement.peek_assist_retreat_on_key);
		}
		end_child();

		ImGui::NextColumn();

		begin_child(STR("Other"));
		{
			ImGui::Checkbox(STRC("Hit sound"), &settings->misc.hitsound);
			if (settings->misc.hitsound)
				slider_int(STR("Hit sound volume"), &settings->misc.hitsound_volume, 1, 100, STR("%d%%"));

			ImGui::Checkbox(STRC("Unlock inventory"), &settings->misc.unlock_inventory);
			ImGui::Checkbox(STRC("Unlock hidden convars"), &settings->misc.unlock_cvars);

#ifdef _DEBUG
			if (ImGui::Button(STRC("Unload"), ImVec2{ dpi::scale(56.f), dpi::scale(24.f) }))
				ctx->unload = true;
#endif

			ImGui::Checkbox(STRC("Auto-buy"), &settings->misc.autobuy.enable);

			combo(STR("Main weapon"), &settings->misc.autobuy.main, { STR("None"), STR("Auto sniper"), STR("Scout"), STR("AWP"), STR("Negev"), STR("M249"), STR("Rifle"), STR("AUG/SG553") });

			combo(STR("Pistol"), &settings->misc.autobuy.pistol, { STR("None"), STR("Dual berettas"), STR("P250"), STR("CZ75-Auto"), STR("Deagle/Revolver") });

			multicombo(STR("Additional"), &settings->misc.autobuy.additional, {
																					  STR("Head helmet"),  // 1
																					  STR("Other helmet"), // 2
																					  STR("HE grenade"),   // 4
																					  STR("Molotov"),	   // 8
																					  STR("Smoke"),		   // 16
																					  STR("Taser"),		   // 32
																					  STR("Defuse kit"),   // 64
																			  });
		}
		end_child();
	}

	static void configs_tab() {
		ImGui::Columns(2, nullptr, false);

		begin_child(STR("Actions"));
		{
			if (ImGui::Button(STRC("Save"), ImVec2{ dpi::scale(56.f), dpi::scale(24.f) }))
				settings->save(STR("settings"));

			if (ImGui::Button(STRC("Load"), ImVec2{ dpi::scale(56.f), dpi::scale(24.f) }))
				settings->load(STR("settings"));
		}
		end_child();

		ImGui::NextColumn();

		begin_child(STR("Config's list"));
		{
		}
		end_child();
	}

	STFI void render_menu() {
		for (size_t i = 0; i < tabs_list.size(); ++i) {
			if (i > 0)
				ImGui::SameLine();

			if (ImGui::Button(tabs_list[i].c_str(), ImVec2{ dpi::scale(56.f), dpi::scale(24.f) }))
				current_tab = (e_tabs)i;
		}

		ImGui::Spacing();

		switch (current_tab) {
			case e_tabs::ragebot: return ragebot_tab();
			case e_tabs::players: return players_tab();
			case e_tabs::world: return world_tab();
			case e_tabs::hotkeys: return hotkeys_tab();
			case e_tabs::misc: return misc_tab();
			case e_tabs::configs: return configs_tab();
			case e_tabs::scripts: return;
			default: return welcome_tab();
		}
	}

	__forceinline void render() {
		if (!open)
			return;

		// ActivateGameOverlayToWebPage
		ImVec2 window_size = ImVec2{ dpi::scale(456.f), dpi::scale(450.f) };
		ImVec2 screen_size = ImGui::GetIO().DisplaySize;

		ImGui::PushFont(fonts::menu_desc);
		ImGui::SetNextWindowPos(ImVec2((screen_size.x - window_size.x) / 2.f, (screen_size.y - window_size.y) / 2.f), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(window_size, ImGuiCond_FirstUseEver);
		ImGui::SetColorEditOptions(ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_PickerHueBar);
		ImGui::Begin(dformat(STRS("CS:GO Internal | Build: {} {}"), STR(__DATE__), STR(__TIME__)).c_str(), &open,
					 ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize);

		render_menu();

		ImGui::End();
		ImGui::PopFont();
	}
} // namespace menu