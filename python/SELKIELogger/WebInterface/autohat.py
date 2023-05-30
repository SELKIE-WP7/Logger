# Copyright (C) 2023 Swansea University
#
# This file is part of the SELKIELogger suite of tools.
#
# SELKIELogger is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the Free
# Software Foundation, either version 3 of the License, or (at your option)
# any later version.
#
# SELKIELogger is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
# more details.
#
# You should have received a copy of the GNU General Public License along
# with this SELKIELogger product.
# If not, see <http://www.gnu.org/licenses/>.

import os

from autohatctl import AutoHATctl

from flask import (
    Response,
    Blueprint,
    render_template,
    current_app,
    flash,
    redirect,
    url_for,
    g,
)

from .pages import get_ip, get_url

ahc = Blueprint("ahc", __name__)

controlPath = "/run/autohatd/control"


def setControlPath(state):
    global controlPath
    aCP = state.app.config.get("AUTOHATCTL_PATH", controlPath)
    controlPath = aCP


ahc.record_once(setControlPath)


@ahc.route("/led/<int:num>/on")
def led_on(num):
    hat = AutoHATctl(controlPath)
    hat.lightCommand(num, 20)

    return redirect(url_for(".index"), 302)


@ahc.route("/led/<int:num>/off")
def led_off(num):
    hat = AutoHATctl(controlPath)
    hat.lightCommand(num, 0)
    return redirect(url_for(".index"), 302)


@ahc.route("/led/<int:num>/set/<int:brightness>")
def led_set(num, brightness):
    hat = AutoHATctl(controlPath)
    hat.lightCommand(num, brightness)
    return redirect(url_for(".index"), 302)


@ahc.route("/relay/<int:num>/on")
def relay_on(num):
    hat = AutoHATctl(controlPath)
    hat.relayCommand(num, 1)
    return redirect(url_for(".index"), 302)


@ahc.route("/relay/<int:num>/off")
def relay_off(num):
    hat = AutoHATctl(controlPath)
    hat.relayCommand(num, 0)
    return redirect(url_for(".index"), 302)


@ahc.route("/")
def index():
    g.ip = get_ip()
    g.ext_url = get_url()
    g.name = current_app.config["DEVICE_NAME"]
    return render_template("autohat.html")
