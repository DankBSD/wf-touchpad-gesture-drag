project('wf-touchpad-gesture-drag', ['cpp'], version: '0.0.1', license: 'Unlicense',
	default_options: ['cpp_std=c++17', 'warning_level=2'])

wayfire = dependency('wayfire')
wlroots = dependency('wlroots', fallback: ['wlroots', 'wlroots'])

plugin_touchpad_gesture_drag = shared_module('touchpad-gesture-drag',
	'touchpad-gesture-drag.cpp',
	dependencies: [wayfire, wlroots],
	install: true,
	install_dir: 'lib/wayfire')

install_data('touchpad-gesture-drag.xml', install_dir: 'share/wayfire/metadata')
