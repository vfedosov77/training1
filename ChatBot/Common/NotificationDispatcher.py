class NotificationDispatcher:
    def __init__(self):
        self.progress_observers = []
        self.events_observers = []

    def add_progress_observer(self, observer):
        self.progress_observers.append(observer)

    def add_events_observer(self, observer):
        self.events_observers.append(observer)

    def on_progress(self, module: str, progress: float):
        for observer in self.progress_observers:
            observer(module, progress)

    def on_event(self, summary: str, details: str, kind: int):
        for observer in self.events_observers:
            observer(summary, details, kind)


class NotificationDispatcherStub:
    def add_progress_observer(self, observer):
        pass

    def add_events_observer(self, observer):
        pass

    def on_progress(self, module: str, progress: float):
        pass

    def on_event(self, summary: str, details: str, kind: int):
        pass
