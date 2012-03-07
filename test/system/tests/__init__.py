import sys
import os

# Import all python files and subdirectories that are modules so we
# fill the module automatically, allowing tests to be auto-discovered
thisdir = os.path.dirname(__file__)
for sub in os.listdir(thisdir):
    subfull = os.path.join(thisdir, sub)
    if os.path.isdir(subfull) and os.path.exists(os.path.join(subfull, '__init__.py')):
        __import__(sub, globals())
    elif sub.endswith('.py') and not sub.startswith('__'):
        __import__(sub[:-3], globals())
