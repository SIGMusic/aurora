#!/usr/bin/env python3

import asyncio
import websockets

async def producer():
    """ Produce a message to be sent to the client """
    # TODO
    await asyncio.sleep(1)
    return "Test"

async def consumer(message):
    """ Handle an incoming message from the client """
    # TODO
    print(message)

async def handler(websocket, path):
    """ Handle sending to and receiving from client """

    print("Connected")
    # print(vars(websocket))
    
    # global connected
    # # Register.
    # connected.add(websocket)
    # try:
    #     # Implement logic here.
    #     await asyncio.wait([ws.send("Hello!") for ws in connected])
    #     await asyncio.sleep(10)
    # finally:
    #     # Unregister.
    #     connected.remove(websocket)

    while True:
        listener_task = asyncio.ensure_future(websocket.recv())
        producer_task = asyncio.ensure_future(producer())
        done, pending = await asyncio.wait(
            [listener_task, producer_task],
            return_when=asyncio.FIRST_COMPLETED)

        if listener_task in done:
            message = listener_task.result()
            await consumer(message)
        else:
            listener_task.cancel()

        if producer_task in done:
            message = producer_task.result()
            await websocket.send(message)
        else:
            producer_task.cancel()

start_server = websockets.serve(handler, "localhost", 7445)

asyncio.get_event_loop().run_until_complete(start_server)
asyncio.get_event_loop().run_forever()