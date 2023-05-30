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
from datetime import datetime
from math import isnan

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

from SELKIELogger.Specs import LocatorSpec

pages = Blueprint("pages", __name__)


def get_ip():
    import socket

    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        s.connect(("52.214.37.129", 22))
        IP = s.getsockname()[0]
    except:
        IP = "127.0.0.1"
    finally:
        s.close()
    return IP


def get_url():
    try:
        addr = current_app.config["EXTERNAL_ADDR"]
        return addr
    except:
        pass

    return ""


def get_status():
    service = current_app.config["SERVICE_NAME"]
    return os.system(f"/bin/systemctl is-active --quiet '{service}'") == 0


@pages.route("/control/<action>")
def control_service(action):
    service = current_app.config["SERVICE_NAME"]
    if action == "start":
        status = os.system(f"/usr/bin/sudo /usr/local/bin/dlctl start")
    elif action == "stop":
        status = os.system(f"/usr/bin/sudo /usr/local/bin/dlctl stop")
    if status == 0:
        flash(f"Service {action} command completed successfully", "success")
    else:
        flash(
            f"Unable to carry out service {action} command. Error code {status}.",
            "danger",
        )
    return redirect(url_for(".index"), 302)


@pages.route("/")
def index():
    g.ip = get_ip()
    g.ext_url = get_url()
    g.name = current_app.config["DEVICE_NAME"]
    g.data_path = current_app.config["DATA_PATH"]
    g.is_running = get_status()
    return render_template("index.html")


@pages.route("/data/")
def show_data():
    g.ip = get_ip()
    g.ext_url = get_url()
    g.name = current_app.config["DEVICE_NAME"]
    g.files = get_run_files()
    return render_template("data.html")


@pages.route("/data/<fileName>/<fileFormat>")
def download_file(fileName, fileFormat="raw"):
    if fileFormat == "raw":
        mime = "application/octet-stream"
        if fileName.endswith(".log"):
            mime = "text/plain"
        return Response(
            stream_data_file(os.path.join(current_app.config["DATA_PATH"], fileName)),
            mimetype=mime,
            headers={"Content-Disposition": f'attachment; filename="{fileName}"'},
        )
    elif fileFormat == "csv" and fileName.endswith(".dat"):
        from .. import SLFiles as SLF

        fileName = os.path.join(current_app.config["DATA_PATH"], fileName)
        vf = os.path.splitext(fileName)[0] + ".var"
        if not os.path.exists(vf):
            flash(
                "No channel map file identified - automatic export not available",
                "danger",
            )
            return redirect(url_for(".index"), 302)

        vf = SLF.VarFile(vf)
        smap = vf.getSourceMap()
        df = SLF.DatFile(fileName, pcs=0x02)
        df.addSourceMap(smap)
        from tempfile import TemporaryDirectory

        def streamCSV():
            headerSent = False
            i = 0
            with TemporaryDirectory() as td:
                for data in df.yieldDataFrame(dropna=False, resample="1s"):
                    tfn = os.path.join(td, f"part{i:04d}")
                    if data is None:
                        return
                    data.to_csv(tfn, header=(not headerSent))
                    headerSent = True
                    yield "".join(open(tfn, "r").readlines())

        return Response(
            streamCSV(),
            mimetype="text/csv",
            headers={
                "Content-Disposition": f'attachment; filename="{os.path.basename(fileName).replace(".dat","-1s.csv")}"'
            },
        )
    flash("Unsupported file format requested", "danger")
    return redirect(url_for(".index"), 302)


@pages.route("/state/")
def show_state():
    from ..SLFiles import StateFile

    try:
        sf = StateFile(
            os.path.join(
                current_app.config["DATA_PATH"], current_app.config["STATE_NAME"]
            )
        )
        stats = sf.parse()
    except FileNotFoundError as e:
        flash("State file missing or inaccessible", "danger")
        current_app.logger.error(f"Unable to load state file: {str(e)}")
        return redirect(url_for(".index"), 303)
    except Exception as e:
        flash("Error processing state file - unable to parse data", "danger")
        current_app.logger.error(f"Unable to process state file: {str(e)}")
        return redirect(url_for(".index"), 303)

    g.sourcemap = sf._vf
    g.stats = stats.to_records()
    g.lastTS = sf.timestamp()
    g.lastDT = sf.to_clocktime(sf.timestamp()).strftime("%Y-%m-%d %H:%M:%S")
    g.ip = get_ip()
    g.ext_url = get_url()
    g.name = current_app.config["DEVICE_NAME"]
    return render_template("state.html")


@pages.route("/map/")
def show_map():
    g.ip = get_ip()
    g.ext_url = get_url()
    g.name = current_app.config["DEVICE_NAME"]

    from ..SLFiles import StateFile

    try:
        sf = StateFile(
            os.path.join(
                current_app.config["DATA_PATH"], current_app.config["STATE_NAME"]
            )
        )
        stats = sf.parse()
    except Exception as e:
        flash("Unable to load state data: No live positions available", "warning")
        current_app.logger.error(f"Unable to load state data: {str(e)}")
        return render_template("map.html")

    ## Extract GPS markers and display
    markers = current_app.config.get("GPS_LOCATORS", [])
    ml = []
    alat = []
    alon = []
    for m in markers:
        m = LocatorSpec(m)
        c = m.check(stats)
        if not (isnan(c[2][0]) or isnan(c[2][1])):
            ml.extend([[c, m]])
            alat.extend([c[2][0]])
            alon.extend([c[2][1]])
        else:
            ml.extend([[(c[0],c[1],(None,None),None),m]])
    g.markers = ml
    if (len(alat) > 0) and (len(alon) > 0):
        g.pos = [sum(alat) / len(alat), sum(alon) / len(alon)]
    else:
        g.pos = [51.62007, -3.8761]
    return render_template("map.html")


def get_run_files():
    files = []
    with os.scandir(current_app.config["DATA_PATH"]) as dataDir:
        for path in dataDir:
            name, ext = os.path.splitext(path.name)
            if (ext in [".log", ".var", ".dat"]) and path.is_file():
                s = path.stat()
                files.append((path.name, datetime.fromtimestamp(s.st_mtime), s.st_size))
    files.sort(key=lambda x: x[1], reverse=True)
    return files


def stream_data_file(fileName):
    f = open(fileName, "rb")

    data = f.read(1024)
    try:
        while len(data) > 0:
            yield data
            data = f.read(1024)
    finally:
        if not f.closed:
            f.close()
    return
