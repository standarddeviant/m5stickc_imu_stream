from msgpack import Unpacker
import json

unpacked = []
unpacker = Unpacker()
fname = r"test.bin"
with open(fname, 'rb') as f:
    unpacker.feed(f.read())
    for o in unpacker:
        print(json.dumps(o, indent=2))