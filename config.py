import os

class Config:
    """
    Base configuration class. Contains default settings and settings
    loaded from environment variables.
    """
    SECRET_KEY = os.environ.get('SECRET_KEY') or 'a-very-secret-and-long-random-string'

    TMC2209_SERIAL_PORT = '/dev/ttyUSB0'  # For Linux. On Windows it might be 'COM3'
    TMC2209_BAUD_RATE = 115200