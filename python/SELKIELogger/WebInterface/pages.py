import os
from datetime import datetime

from flask import Response, Blueprint, render_template, current_app, flash, redirect, url_for, g

pages = Blueprint("pages", __name__)

def get_ip():
    import socket
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        s.connect(('52.214.37.129', 22))
        IP = s.getsockname()[0]
    except:
        IP = '127.0.0.1'
    finally:
        s.close()
    return IP

def get_url():
    try:
        addr = current_app.config['EXTERNAL_ADDR']
        return addr
    except:
        pass

    return ""


def get_status():
    service = current_app.config['SERVICE_NAME']
    return os.system(f"/bin/systemctl is-active --quiet '{service}'") == 0

@pages.route('/control/<action>')
def control_service(action):
    service = current_app.config['SERVICE_NAME']
    if action == "start":
        status = os.system(f"/usr/bin/sudo /usr/local/bin/dlctl start")
    elif action == "stop":
        status = os.system(f"/usr/bin/sudo /usr/local/bin/dlctl stop")
    if status == 0:
        flash(f"Service {action} command completed successfully", 'success')
    else:
        flash(f"Unable to carry out service {action} command. Error code {status}.", 'danger')
    return redirect(url_for(".index"), 302)

@pages.route('/')
def index():
    g.ip = get_ip()
    g.ext_url = get_url()
    g.name = current_app.config['DEVICE_NAME']
    g.data_path = current_app.config['DATA_PATH']
    g.is_running = get_status()
    return render_template('index.html')

@pages.route('/data/')
def show_data():
    g.ip = get_ip()
    g.ext_url = get_url()
    g.name = current_app.config['DEVICE_NAME']
    g.files = get_run_files()
    return render_template('data.html')

@pages.route('/data/<fileName>/<fileFormat>')
def download_file(fileName, fileFormat='raw'):
    if fileFormat == "raw":
        return Response(stream_data_file(os.path.join(current_app.config['DATA_PATH'], fileName)), mimetype="application/octet-stream", headers={'Content-Disposition': f'attachment, filename="{fileName}"'})
    elif fileFormat == "csv":
        from ..RaceTech import RTCSVExporter
        rtcsv = RTCSVExporter(os.path.join(current_app.config['DATA_PATH'], fileName))
        return Response(rtcsv.stream(), mimetype="text/csv", headers={'Content-Disposition': f'attachment, filename="{fileName.replace(".run",".csv")}"'})


def get_run_files():
    files = []
    with os.scandir(current_app.config['DATA_PATH']) as dataDir:
        for path in dataDir:
            name, ext = os.path.splitext(path.name)
            if (ext in [".log", ".var", ".dat"]) and path.is_file():
                s = path.stat()
                files.append((path.name, datetime.fromtimestamp(s.st_mtime), s.st_size))
    files.sort(reverse=True)
    return files

def stream_data_file(fileName):
    f = open(fileName, 'rb')
    data = f.read(1024)
    try:
        while len(data) > 0:
            yield data
            data = f.read(1024)
    finally:
        if not f.closed:
            f.close()
    return

