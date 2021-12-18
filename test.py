#!/usr/bin/python3
#
# websocket connection

import websocket

websocket.enableTrace(True)

ws = websocket.WebSocket()
ws.connect("wss://httpstat.us/401")
print(ws.recv())
ws.close()
