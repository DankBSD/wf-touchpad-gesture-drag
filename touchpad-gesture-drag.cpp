#define WAYFIRE_PLUGIN
#define WLR_USE_UNSTABLE
#include <wayfire/nonstd/wlroots-full.hpp>
#include <wayfire/plugin.hpp>
#include <wayfire/singleton-plugin.hpp>
#include <wayfire/output.hpp>
#include <wayfire/core.hpp>
#include <wayfire/workspace-manager.hpp>
#include <wayfire/signal-definitions.hpp>
extern "C" {
#include <limits.h>
#include <linux/input-event-codes.h>
#include <wlr/backend/headless.h>
#include <wlr/backend/multi.h>
#include <wlr/interfaces/wlr_input_device.h>
#include <wlr/types/wlr_pointer.h>
#include <wlr/types/wlr_seat.h>
}

struct virtual_pointer : public wf::custom_data_t {
	struct wlr_backend *backend = nullptr;
	struct wlr_input_device *pointer = nullptr;
	virtual_pointer() {
		auto &core = wf::compositor_core_t::get();
		backend = wlr_headless_backend_create_with_renderer(core.display, core.renderer);
		if (!backend) {
			LOGE("wlr_headless_backend_create_with_renderer failed");
			return;
		}
		if (!wlr_multi_backend_add(core.backend, backend) || !wlr_backend_start(backend)) {
			LOGE("wlr_multi_backend_add/wlr_backend_start failed");
			wlr_backend_destroy(backend);
			backend = nullptr;
			return;
		}
		pointer = wlr_headless_add_input_device(backend, WLR_INPUT_DEVICE_POINTER);
		if (!pointer) {
			LOGE("wlr_headless_add_input_device failed");
			wlr_multi_backend_remove(core.backend, backend);
			wlr_backend_destroy(backend);
			backend = nullptr;
			return;
		}
	}
	~virtual_pointer() {
		if (!backend) return;
		auto &core = wf::compositor_core_t::get();
		wlr_multi_backend_remove(core.backend, backend);
		wlr_backend_destroy(backend);
	}
};

struct touchpad_gesture_drag : public wf::singleton_plugin_t<virtual_pointer> {
	// TODO: settings
	int fingers = 3;
	bool dragging = false;
	int64_t hold_start_msec = INT64_MIN;
	int64_t hold_end_msec = INT64_MIN;
	int hold_threshold_msec = 150;

	inline nonstd::observer_ptr<virtual_pointer> get_shared() {
		return &wf::get_core().get_data_safe<wf::detail::singleton_data_t<virtual_pointer>>()->ptr;
	}

	void init() override {
		singleton_plugin_t::init();
		wf::get_core().connect_signal("pointer_hold_begin", &on_hold_begin);
		wf::get_core().connect_signal("pointer_hold_end", &on_hold_end);
		wf::get_core().connect_signal("pointer_swipe_begin", &on_swipe_begin);
		wf::get_core().connect_signal("pointer_swipe_update", &on_swipe_update);
		wf::get_core().connect_signal("pointer_swipe_end", &on_swipe_end);
	}

	template <class wlr_event>
	using event = wf::input_event_signal<wlr_event>;

	wf::signal_connection_t on_hold_begin = [=](wf::signal_data_t *data) {
		auto wf_ev = static_cast<event<wlr_event_pointer_hold_begin> *>(data);
		auto ev = wf_ev->event;
		if (static_cast<int>(ev->fingers) != fingers) return;
		wf_ev->carried_out = true;
		hold_start_msec = ev->time_msec;
		hold_end_msec = INT64_MIN;
	};

	wf::signal_connection_t on_hold_end = [=](wf::signal_data_t *data) {
		auto wf_ev = static_cast<event<wlr_event_pointer_hold_end> *>(data);
		if (hold_start_msec == INT64_MIN) return;
		wf_ev->carried_out = true;
		auto ev = wf_ev->event;
		if (!ev->cancelled || ev->time_msec - hold_start_msec < hold_threshold_msec) return;
		hold_end_msec = ev->time_msec;
	};

	wf::signal_connection_t on_swipe_begin = [=](wf::signal_data_t *data) {
		auto wf_ev = static_cast<event<wlr_event_pointer_swipe_begin> *>(data);
		auto ev = wf_ev->event;
		if (wf_ev->carried_out || static_cast<int>(ev->fingers) != fingers ||
		    hold_end_msec == INT64_MIN || ev->time_msec != hold_end_msec)
			return;
		wf_ev->carried_out = true;
		dragging = true;
		auto *dev = get_shared()->pointer;
		wlr_event_pointer_button button{
		    .device = dev,
		    .time_msec = ev->time_msec,
		    .button = BTN_LEFT,
		    .state = WLR_BUTTON_PRESSED,
		};
		wl_signal_emit(&(dev->pointer->events.button), &button);
	};

	wf::signal_connection_t on_swipe_update = [&](wf::signal_data_t *data) {
		if (!dragging) return;
		auto wf_ev = static_cast<event<wlr_event_pointer_swipe_update> *>(data);
		if (wf_ev->carried_out) return;
		auto ev = wf_ev->event;

		wf_ev->carried_out = true;
		auto *dev = get_shared()->pointer;
		wlr_event_pointer_motion motion{
		    .device = dev,
		    .time_msec = ev->time_msec,
		    .delta_x = wl_fixed_to_double(ev->dx) * 100,
		    .delta_y = wl_fixed_to_double(ev->dy) * 100,
		    .unaccel_dx = wl_fixed_to_double(ev->dy) * 1,
		    .unaccel_dy = wl_fixed_to_double(ev->dy) * 1,
		};
		wl_signal_emit(&(dev->pointer->events.motion), &motion);
		wl_signal_emit(&(dev->pointer->events.frame), dev->pointer);
	};

	wf::signal_connection_t on_swipe_end = [=](wf::signal_data_t *data) {
		if (!dragging) return;
		auto wf_ev = static_cast<event<wlr_event_pointer_swipe_end> *>(data);
		if (wf_ev->carried_out) return;
		auto ev = wf_ev->event;
		wf_ev->carried_out = true;
		dragging = false;
		auto *dev = get_shared()->pointer;
		wlr_event_pointer_button button{
		    .device = dev,
		    .time_msec = ev->time_msec,
		    .button = BTN_LEFT,
		    .state = WLR_BUTTON_RELEASED,
		};
		wl_signal_emit(&(dev->pointer->events.button), &button);
	};
};

DECLARE_WAYFIRE_PLUGIN(touchpad_gesture_drag);
