import os
import logging
from logging.handlers import RotatingFileHandler
from flask import Flask
from flask_socketio import SocketIO
from config import Config
from .controller_input.dualsense_manager import DualSenseManager
from .motor_control.motor_manager import MotorManager

# Initialize extensions but don't attach them to an app yet
socketio = SocketIO(async_mode='threading')

# Create instances of the managers.
motor_manager = MotorManager()
dualsense_manager = DualSenseManager(motor_manager.command_queue)

def create_app(config_class=Config):
    """The application factory."""
    app = Flask(__name__)
    app.config.from_object(config_class)

    # initialize the extensions with the app
    socketio.init_app(app)

    # Inject the web context into both managers
    motor_manager.set_app_context(app, socketio)
    dualsense_manager.set_app_context(app, socketio)

    # Logging setup
    if not app.debug and not app.testing:
        if not os.path.exists('logs'):
            os.mkdir('logs')

        file_handler = RotatingFileHandler('logs/robotarm.log', maxBytes=10240, backupCount=2)

        # Configure log format
        log_format = logging.Formatter(
            '%(asctime)s %(levelname)s: %(message)s [in %(pathname)s:%(lineno)d]'
        )
        file_handler.setFormatter(log_format)

        # Log level
        app.logger.setLevel(logging.INFO)
        file_handler.setLevel(logging.INFO)

        # Log startup
        app.logger.info('Robotarm startup')

    # Register blueprints
    from .main import main as main_blueprint
    app.register_blueprint(main_blueprint)

    return app