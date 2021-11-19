import os

from autohatctl import AutoHATctl

from flask import Response, Blueprint, render_template, current_app, flash, redirect, url_for, g

from .pages import get_ip, get_url

ahc = Blueprint("ahc", __name__)

controlPath = "/run/autohatd/control"


def setControlPath(state):
    global controlPath
    aCP = state.app.config.get('AUTOHATCTL_PATH', controlPath)
    controlPath = aCP

ahc.record_once(setControlPath)

@ahc.route('/led/<int:num>/on')
def led_on(num):
    hat = AutoHATctl(controlPath)
    hat.lightCommand(num, 20)
    
    return redirect(url_for(".index"), 302)

@ahc.route('/led/<int:num>/off')
def led_off(num):
    hat = AutoHATctl(controlPath)
    hat.lightCommand(num, 0)
    return redirect(url_for(".index"), 302)

@ahc.route('/led/<int:num>/set/<int:brightness>')
def led_set(num, brightness):
    hat = AutoHATctl(controlPath)
    hat.lightCommand(num, brightness)
    return redirect(url_for(".index"), 302)

@ahc.route('/relay/<int:num>/on')
def relay_on(num):
    hat = AutoHATctl(controlPath)
    hat.relayCommand(num, 1)
    return redirect(url_for(".index"), 302)

@ahc.route('/relay/<int:num>/off')
def relay_off(num):
    hat = AutoHATctl(controlPath)
    hat.relayCommand(num, 0)
    return redirect(url_for(".index"), 302)

@ahc.route('/')
def index():
    g.ip = get_ip()
    g.ext_url = get_url()
    g.name = current_app.config['DEVICE_NAME']
    return render_template('autohat.html')
