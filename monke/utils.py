from enum import Enum, unique


class NoSocketCreated(RuntimeError):
    def __init__(self, args):
        self.args = args


class ConnectionNotEstablished(RuntimeError):
    def __init__(self, args):
        self.args = args


class HandshakeFailure(RuntimeError):
    def __init__(self, args):
        self.args = args


@unique
class MESSAGE_TYPES(Enum):
    HELLO = 1
    DATA = 2
    ACCEP = 4
    REJECC = 8
    PING = 16


@ unique
class CONNECTION_STATES(Enum):
    NO_CONNECTION = 0
    HANDSHAKE = 1
    CONNECTED = 2


@ unique
class SERVER_STATES(Enum):
    pass


@ unique
class CLIENT_STATE(Enum):
    pass
