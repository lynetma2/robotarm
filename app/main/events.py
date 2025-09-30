from flask import request, current_app
from app import socketio, motor_manager

@socketio.on('connect')
def handle_connect():
    current_app.logger.info(f"Client connected: {request}")

@socketio.on('motor_command')
def handle_motor_command(data):
    """Receives a command from the client and adds it to the queue."""
    current_app.logger.info(f"Received command from client: {data}")
    # Add the received command dictionary to the motor manager's queue
    motor_manager.command_queue.put(data)