class Tee(object):
    """
    Duplicate output to multiple file-like objects.
    """

    def __init__(self, *args):
        """
        Initialize the tee output object. Pass in a set of output
        file-like objects to duplicate writes to.
        """
        self.outs = args
        for f in self.outs:
            if not hasattr(f, 'write'):
                raise Error()

    def write(self, data):
        for f in self.outs:
            f.write(data)

    def flush(self):
        for f in self.outs:
            f.flush()
