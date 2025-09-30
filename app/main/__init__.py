# app/main/__init__.py

from flask import Blueprint

# Create a Blueprint instance
main = Blueprint('main', __name__)

# Import the routes and events so they are registered with the blueprint.
# This is done at the bottom to avoid circular import issues.
from . import routes, events