class Scheduler:
    """Schedule and run incoming jobs according to Weighted Fair
    Queueing.

    Given a limited resource (the radio network) and potentially
    unbounded demand (connected clients' animations), a scheduler is
    necessary to arbitrate access to the resource.

    This class implements Weighted Fair Queueing. WFQ is a
    packet-based approximation of Generalized Processor Sharing.
    In this case, the radio represents the processor and each animation
    represents a packet. For this to be feasible, each animation must
    specify a duration to serve as the packet length. This restriction
    prevents clients from running infinite animations but also
    guarantees their atomicity. Clients wishing to use infinite
    animations can instead schedule multiple discrete animations.

    """

    def __init__(self):
        """Initialize data structures needed by the scheduler."""
        # TODO
        pass

    def start(self):
        """Start scheduling jobs for the radio."""
        # TODO
        pass

    def insertJob(self, client, duration, weight):
        """Add a job to the list."""
        # TODO
        pass
