/*
Plugin Name
Copyright (C) <Year> <Developer> <Email Address>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

#include <obs-module.h>
#include <plugin-support.h>
#include <obs-frontend-api.h>
#include <QMainWindow>
#include <QDockWidget>
#include <detector.hpp>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

static Detector *detector = nullptr;

bool obs_module_load(void)
{
	obs_log(LOG_INFO, "plugin loaded successfully (version %s)", PLUGIN_VERSION);
	const auto main_window = static_cast<QMainWindow *>(obs_frontend_get_main_window());
	obs_frontend_push_ui_translation(obs_module_get_string);
	detector = new Detector(main_window);

	obs_frontend_add_dock_by_id("detector", obs_module_text("Detector"), detector);
	obs_frontend_pop_ui_translation();

	obs_log(LOG_INFO, "plugin loaded successfully (version %s)", PLUGIN_VERSION);
	return true;
}

void obs_module_post_load(void) {
	// QMainWindow *window = (QMainWindow *)obs_frontend_get_main_window();
	// QDockWidget *dock = new QDockWidget(window);
	// QDockWidget *myDock = new QDockWidget();
	// dock->setWidget(myDock);

	// dock->setWindowTitle(QString::fromUtf8(obs_module_text("My Doc Sample"), -1));
	// dock->resize(100, 100);
	// dock->setFloating(true);
	// dock->hide();

	// obs_frontend_add_dock_by_id("my_dock", obs_module_text("My Doc Sample"), dock);
	if (detector)
		detector->PostLoad();
}

void obs_module_unload(void)
{
	obs_log(LOG_INFO, "plugin unloaded");
}
