from match_client.match import Match
from match_client.match.ttypes import User

from thrift import Thrift
from thrift.transport import TSocket
from thrift.transport import TTransport
from thrift.protocol import TBinaryProtocol

from sys import stdin

def operator(op, user_id, username, score):
    # Make socket
    transport = TSocket.TSocket('127.0.0.1', 9090)

    # Buffering is critical. Raw so:ckets are very slow
    transport = TTransport.TBufferedTransport(transport)

    # Wrap in a protocol
    protocol = TBinaryProtocol.TBinaryProtocol(transport)

    # Create a client to use the protocol encoder
    client = Match.Client(protocol)

    # Connect!
    transport.open()

    user = User(user_id, username, score)
    
    if op == "add":
        client.add_user(user, "")
    elif op == "remove":
        client.remove_user(user, "")
    # Close!
    transport.close()


def main():
    for line in stdin:
        op, user_id, username, score = line.split(' ')
        operator(op, int(user_id), username, int(score))


if __name__ == "__main__":
    main()
