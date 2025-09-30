from app import create_app, socketio, motor_manager, dualsense_manager

app = create_app()

if __name__ == '__main__':
    print("Starting motor manager thread...")
    # Start the motor manager's background thread
    motor_manager.start()

    print('Starting DualSense manager thread...')
    dualsense_manager.start()

    print("Starting Flask-SocketIO server...")
    # Start the web server
    socketio.run(app, host='0.0.0.0', port=5000)