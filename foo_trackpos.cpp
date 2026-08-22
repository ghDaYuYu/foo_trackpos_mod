#include "stdafx.h"
#include "version.h"

DECLARE_COMPONENT_VERSION(COMPONENT_NAME_HC, FOO_COMPONENT_VERSION,
	"This compoment allows you to place tracks in the playlist after the currently playing track\n"
	"by Chronial\n"
	"\n"
	"Mod: dark mode and x64 binary (v1.1.1) by da yuyu\n"
	"\n"
	"Version: " FOO_COMPONENT_VERSION "\n"
	"Compiled: " __DATE__ "\n"
	"fb2k SDK: " PLUGIN_FB2K_SDK);

VALIDATE_COMPONENT_FILENAME("foo_trackpos_mod.dll");


extern cfg_bool cfgMovePlaylistContext;
extern cfg_bool cfgMovePlaylistExists;
extern cfg_bool cfgFocus;
extern cfg_bool cfgEmptyQueue;
extern cfg_bool cfgEnqueueOnLock;
extern cfg_bool cfgSetPlaybackOrder;
extern cfg_bool cfgNoPlayPlace;


bool getCurrentLocation(t_size * playlist, t_size * trackindex){
	static_api_ptr_t<playlist_manager> pm;
	if (!pm->get_playing_item_location(playlist,trackindex)){
		if (!cfgNoPlayPlace)
			return false;
		*trackindex = pm->activeplaylist_get_focus_item();
		if (*trackindex == ~0){
			*trackindex = 0;
		}
		*playlist = pm->get_active_playlist();
		if (*playlist == ~0)
			return false;
	}
	return true;
}

void placeAfterCurrent(const pfc::list_base_const_t<metadb_handle_ptr> & p_data, bool playlistCalled){
	static_api_ptr_t<playlist_manager> pm;
	t_size playingList, playingIndex;
	if (!getCurrentLocation(&playingList,&playingIndex))
		return;
	bool wasQueued = false;
	pfc::list_base_const_t<metadb_handle_ptr> const * insertItems = 0;
	t_size playlistLength = pm->playlist_get_item_count(playingList);
	bit_array_bittable moveMask(playlistLength);
	t_size moveCount = 0;

	pm->playlist_undo_backup(playingList);

	if (playlistCalled && (cfgMovePlaylistContext || cfgMovePlaylistExists)&& (playingList == pm->get_active_playlist())){
		pm->playlist_set_selection_single(playingList,playingIndex,false);
		pm->activeplaylist_get_selection_mask(moveMask);
		moveCount = pm->activeplaylist_get_selection_count(~0);
	} else {
		if (cfgMovePlaylistExists){
			pfc::list_t<metadb_handle_ptr> listContent;
			bit_array_bittable deleteMask(playlistLength);
			pm->playlist_get_all_items(playingList,listContent);
			for (t_size i=0; i < p_data.get_count(); i++){
				metadb_handle_ptr item = p_data.get_item(i);
				for (t_size j = 0; j < listContent.get_count(); j++){
					if (listContent.get_item(j) == item && j != playingIndex){
						deleteMask.set(j,true);
					}
				}
			}
			pm->playlist_remove_items(playingList,deleteMask);
			getCurrentLocation(&playingList,&playingIndex);
			playlistLength = pm->playlist_get_item_count(playingList);
		}
		insertItems = &p_data;
	}

	if (moveCount > 0){
		t_size * newOrder = new t_size[playlistLength];
		int delta = 0;
		t_size * toInsert = new t_size[moveCount];
		unsigned int n = 0;
		int insertPos = -1;
		for (t_size i = 0; i < playlistLength; i++){
			if (!moveMask.get(i)){
				newOrder[i + delta] = i;
				if (i == playingIndex){
					playingIndex = i + delta; // we need this to focus the track later.
											  // can do this as delta will be <= 0 when we reach playingIndex so i == playingIndex won't be true again
					insertPos = static_cast<int>(i + delta + 1);
					for (t_size j = 0; j < n; j++){
						newOrder[insertPos++] = toInsert[j];
					}
					delta += static_cast<int>(moveCount);
				}
			} else if (insertPos == -1){
				toInsert[n++] = i;
				delta--;
			} else {
				newOrder[insertPos++] = i;
				delta--;
			}
		}
		if(!pm->playlist_reorder_items(playingList,newOrder,playlistLength)){
			if (cfgEnqueueOnLock){
				for (t_size i = 0; i < playlistLength; i++){
					if (moveMask.get(i)){
						metadb_handle_ptr toQueue;
						pm->playlist_get_item_handle(toQueue,playingList,i);
						pm->queue_add_item(toQueue);
					}
				}
			}
			wasQueued = true;
		}
		delete[] newOrder;
		delete[] toInsert;
	}
	if (insertItems != 0 && insertItems->get_count() > 0){
		bit_array_val * selection;
		if (playingList == pm->get_active_playlist()){
			pm->playlist_clear_selection(playingList);
			selection = new bit_array_val(true);
		} else {
			selection = new bit_array_val(false);
		}
		if (pm->playlist_insert_items(playingList,playingIndex+1,*insertItems,*selection) == -1){
			if (cfgEnqueueOnLock){
				for (t_size i = 0; i < insertItems->get_count(); i++){
					pm->queue_add_item(insertItems->get_item(i));
				}
			}
			wasQueued = true;
		}
		delete selection;
	}
	if (cfgFocus && (playingList == pm->get_active_playlist()) && !wasQueued){
		pm->playlist_set_focus_item(playingList,playingIndex+1);
	}
	if (cfgEmptyQueue && !wasQueued)
		pm->queue_flush();
	if (cfgSetPlaybackOrder && !wasQueued){
		static const GUID guid_playbackOrder_default = { 0xbfc61179, 0x49ad, 0x4e95, { 0x8d, 0x60, 0xa2, 0x27, 0x6, 0x48, 0x55, 0x5 } };
		static const GUID guid_playbackOrder_repeatPlaylist = { 0x681cc6ea, 0x60ae, 0x4bf9, { 0x91, 0x3b, 0xbb, 0x5f, 0x4e, 0x86, 0x4f, 0x2a } };
		//static const GUID guid_playbackOrder_repeatTrack = { 0x4bf4b280, 0xbb4, 0x4dd0, { 0x8e, 0x84, 0x37, 0xc3, 0x20, 0x9c, 0x3d, 0xa2 } };
		GUID activeOrder = pm->playback_order_get_guid(pm->playback_order_get_active());
		if (activeOrder != guid_playbackOrder_default && activeOrder != guid_playbackOrder_repeatPlaylist){
			for (t_size i = 0; i < pm->playback_order_get_count(); i++){
				if (pm->playback_order_get_guid(i) == guid_playbackOrder_default){
					pm->playback_order_set_active(i);
				}
			}
		}
	}
}

class my_contextmenu : public contextmenu_item_simple {
	virtual unsigned get_num_items(){
		return 1;
	};
	virtual void get_item_name(unsigned p_index,pfc::string_base & p_out){
		p_out = "Place after currently playing";
	};
	virtual void context_command(unsigned p_index,const pfc::list_base_const_t<metadb_handle_ptr> & p_data,const GUID& p_caller){
		placeAfterCurrent(p_data,(p_caller == this->caller_playlist));
	};
	virtual GUID get_item_guid(unsigned p_index){
		// {B25EA664-4CF5-480B-8372-85A6B6F82DD0} mod
		static const GUID guid_contextAddAfterPlaying = { 0xb25ea664, 0x4cf5, 0x480b, { 0x83, 0x72, 0x85, 0xa6, 0xb6, 0xf8, 0x2d, 0xd0 } };
		return guid_contextAddAfterPlaying;
	}
	virtual bool get_item_description(unsigned p_index,pfc::string_base & p_out){
		p_out = "Places the selected song in the playlist after the currently playling song";
		return true;
	}
	double get_sort_priority() {
		return contextmenu_priorities::root_queue - 100;
	}
};
static contextmenu_item_factory_t< my_contextmenu > foo_contextmenu;