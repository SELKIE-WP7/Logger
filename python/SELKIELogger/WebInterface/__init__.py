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
    app.config.from_mapping(DATA_PATH='/media/data/live/', SECRET_KEY='21u3o1i2hlbjkl1h3i1uo31y283oi213h12li3', SERVICE_NAME='datalogger', BOOTSTRAP_SERVE_LOCAL=True, DEVICE_NAME="Unnamed Device")
    app.config.from_pyfile(os.path.join(app.instance_path, 'config.py'), silent=True)

    Bootstrap(app)
    app.register_blueprint(pages)

    app.logger.info(f"Root path:    \t{app.root_path}")
    app.logger.info(f"Instance path:\t{app.instance_path}")
    app.logger.info(f"Data path:    \t{app.config['DATA_PATH']}")
    return app


def run_dev_server():
    app = create_app()
    app.run()
