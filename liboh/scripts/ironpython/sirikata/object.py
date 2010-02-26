import uuid
import util

from Sirikata.Runtime import HostedObject


class Object:
    """
    Object is a base class for virtual world objects.  Although it is
    not required that a virtual world use this class as a base class
    (functionality can be accessed via Sirikata.Runtime.HostedObject),
    it provides a standard set of utility methods for objects and
    presents them in a Pythonic manner with appropriate Python
    wrappers for various types.  This helps avoid having to deal with
    the true underlying types and instead provides as pure a Python
    interface as possible.

    This base class handles only the most basic functionality --
    connecting to spaces, updating location, registering proximity
    queries and receiving their results, and communicating via ODP.
    Any non-core functionality should be handled by subclasses.
    """

    def __init__(self):
        pass


    def uuid(self):
        """Returns this object's internal unique ID."""
        system_uuid = HostedObject.InternalID()
        return util.SystemGuidToPythonUUID(system_uuid)

    def objectReference(self, space):
        """
        Returns this object's identifier in the given space.  If object is not
        connected to the space, returns None.
        """
        system_space = util.PythonUUIDToSystemGuid(space)
        system_objref = HostedObject.ObjectReference(system_space)
        if system_objref == None: return None
        return util.SystemGuidToPythonUUID(system_objref)
