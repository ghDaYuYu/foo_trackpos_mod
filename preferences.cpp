#include "stdafx.h"
#include "resource.h"
#include "helpers/atl-misc.h"
#include "helpers/DarkMode.h"

// {7CA1DC3A-529E-46A6-B2F7-402B5EB62DE5} mod
static const GUID guid_cfgMovePlaylistContext = { 0x7ca1dc3a, 0x529e, 0x46a6, { 0xb2, 0xf7, 0x40, 0x2b, 0x5e, 0xb6, 0x2d, 0xe5 } };
bool default_cfgMovePlaylistContext = true;
cfg_bool cfgMovePlaylistContext(guid_cfgMovePlaylistContext, default_cfgMovePlaylistContext);

// {C52DCA9E-72B7-4462-89D9-3C475913CED2} mod
static const GUID guid_cfgMovePlaylistExists = { 0xc52dca9e, 0x72b7, 0x4462, { 0x89, 0xd9, 0x3c, 0x47, 0x59, 0x13, 0xce, 0xd2 } };
bool default_cfgMovePlaylistExists = false;
cfg_bool cfgMovePlaylistExists(guid_cfgMovePlaylistExists, default_cfgMovePlaylistExists);

// {553C8B87-D5D3-450C-ACF3-EEFD7500186F} mod
static const GUID guid_cfgFocus = { 0x553c8b87, 0xd5d3, 0x450c, { 0xac, 0xf3, 0xee, 0xfd, 0x75, 0x0, 0x18, 0x6f } };
bool default_cfgFocus = true;
cfg_bool cfgFocus(guid_cfgFocus, default_cfgFocus);

// {7AD9FE01-B86E-4AF5-942B-C426FA8AE427} mod
static const GUID guid_cfgEmptyQueue = { 0x7ad9fe01, 0xb86e, 0x4af5, { 0x94, 0x2b, 0xc4, 0x26, 0xfa, 0x8a, 0xe4, 0x27 } };
bool default_cfgEmptyQueue = true;
cfg_bool cfgEmptyQueue(guid_cfgEmptyQueue, default_cfgEmptyQueue);

// {8343EC4B-F04B-4E19-B74F-79B861C30D75} mod
static const GUID guid_cfgEnqueueOnLock = { 0x8343ec4b, 0xf04b, 0x4e19, { 0xb7, 0x4f, 0x79, 0xb8, 0x61, 0xc3, 0xd, 0x75 } };
bool default_cfgEnqueueOnLock = true;
cfg_bool cfgEnqueueOnLock(guid_cfgEnqueueOnLock, default_cfgEnqueueOnLock);

// {8DDE6D9C-72F8-4459-8853-DB8D1C9E2681} mod
static const GUID guid_cfgSetPlaybackOrder = { 0x8dde6d9c, 0x72f8, 0x4459, { 0x88, 0x53, 0xdb, 0x8d, 0x1c, 0x9e, 0x26, 0x81 } };
bool default_cfgSetPlaybackOrder = true;
cfg_bool cfgSetPlaybackOrder(guid_cfgSetPlaybackOrder, default_cfgSetPlaybackOrder);

// {F6E5508E-16A4-414E-928B-2CC2FAE29325} mod
static const GUID guid_cfgNoPlayPlace = { 0xf6e5508e, 0x16a4, 0x414e, { 0x92, 0x8b, 0x2c, 0xc2, 0xfa, 0xe2, 0x93, 0x25 } };
bool default_cfgNoPlayPlace = true;
cfg_bool cfgNoPlayPlace(guid_cfgNoPlayPlace, default_cfgNoPlayPlace);


std::unordered_map<int, std::pair<cfg_bool*, bool>> bool_var_map({
	{ IDC_EMPTY_QUEUE, { &cfgEmptyQueue, default_cfgEmptyQueue } },
	{ IDC_FOCUS, { &cfgFocus, default_cfgFocus } },
	{ IDC_MOVE_PLAYLIST_EXIST, { &cfgMovePlaylistExists, default_cfgMovePlaylistExists } },
	{ IDC_MOVE_PLAYLIST_CONTEXT, { &cfgMovePlaylistContext, default_cfgMovePlaylistContext } },
	{ IDC_LOCK_ENQUEUE, { &cfgEnqueueOnLock, default_cfgEnqueueOnLock } },
	{ IDC_PLAYBACK_ORDER, { &cfgSetPlaybackOrder, default_cfgSetPlaybackOrder } },
	{ IDC_NOPLAY_PLACE, { &cfgNoPlayPlace, default_cfgNoPlayPlace } },
});


class pref_page_window : public CDialogImpl<pref_page_window>, public preferences_page_instance
{
	preferences_page_callback::ptr callback;
	fb2k::CDarkModeHooks m_dark;
public:
	enum { IDD = IDD_CONFIG };

	pref_page_window(preferences_page_callback::ptr callback) : callback(callback) {}

	BEGIN_MSG_MAP(pref_page_window)
		MSG_WM_INITDIALOG(OnInitDialog)
		MSG_WM_COMMAND(OnCommand)
	END_MSG_MAP()

	t_uint32 get_state() {

		t_uint32 state = preferences_state::resettable | preferences_state::dark_mode_supported;

		if (isDirty()) state |= preferences_state::changed;
		return state;

	}
	
	void apply(){
		for (auto e : bool_var_map){
			*e.second.first = IsDlgButtonChecked(e.first) != 0;
			callback->on_state_changed();
		}
	};

	bool isDirty(){
		for (auto e : bool_var_map){
			if (*(e.second.first) != (IsDlgButtonChecked(e.first) != 0)){
				return true;
			}
		}
		return false;
	}

	void reset(){
		for (auto e: bool_var_map){
			CheckDlgButton(e.first, e.second.second);
		}
		callback->on_state_changed();
	};

	LRESULT OnInitDialog(CWindow wndFocus, LPARAM lInitParam){
		for (auto e : bool_var_map){
			CheckDlgButton(e.first, *(e.second.first));
		}
		m_dark.AddDialogWithControls(*this);
		return 0;
	}

	void OnCommand(UINT uNotifyCode, int nID, CWindow wndCtl){
		callback->on_state_changed();
	}
};

class pref_page : public preferences_page_impl<pref_page_window>
{
	const char * get_name(){
		return "Track Positioner";
	}

	GUID get_guid(){
		// {9BDD1E59-F14F-4275-874B-9E8BB813BFB4} mod
		static const GUID guid_preferences = { 0x9bdd1e59, 0xf14f, 0x4275, { 0x87, 0x4b, 0x9e, 0x8b, 0xb8, 0x13, 0xbf, 0xb4 } };

		return guid_preferences;
	}

	GUID get_parent_guid(){
		return preferences_page::guid_tools;
	}
};

static preferences_page_factory_t<pref_page> g_pref;