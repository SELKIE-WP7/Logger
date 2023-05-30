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

from flask import Flask
from flask_bootstrap import Bootstrap

from .pages import pages


def create_app(test_config=None):
    app = Flask(__name__, instance_relative_config=True)
    # ensure the instance folder exists
    try:
        os.makedirs(app.instance_path)
    except OSError:
        pass
    app.config.from_mapping(
        DATA_PATH="/media/data/live/",
        SECRET_KEY="21u3o1i2hlbjkl1h3i1uo31y283oi213h12li3",
        SERVICE_NAME="datalogger",
        BOOTSTRAP_SERVE_LOCAL=True,
        DEVICE_NAME="Unnamed Device",
        STATE_NAME="SLogger.state",
    )
    app.config.from_pyfile(os.path.join(app.instance_path, "config.py"), silent=True)

    Bootstrap(app)
    app.register_blueprint(pages)

    def handle_unknown_component(*args, **kwargs):
        return False

    app.url_build_error_handlers.append(handle_unknown_component)

    try:
        import autohatctl
        from .autohat import ahc

        app.register_blueprint(ahc, url_prefix="/autohat")
    except ModuleNotFoundError:
        app.logger.warning("Automation HAT support disabled")

    app.logger.info(f"Root path:    \t{app.root_path}")
    app.logger.info(f"Instance path:\t{app.instance_path}")
    app.logger.info(f"Data path:    \t{app.config['DATA_PATH']}")
    return app


def run_dev_server():
    app = create_app()
    app.run()
