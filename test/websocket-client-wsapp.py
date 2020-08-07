import sys       # builtin
import websocket # from `pip install websocket-client`
import msgpack   # from `pip install msgpack`
try:
    import thread
except ImportError:
    import _thread as thread
import time

imu_obj_list = []

def on_message(ws, message):
    # print(msgpack.unpackb(message))
    if type(message) in (bytes, bytearray):
        imu_obj_list.append(msgpack.unpackb(message))
        print(len(imu_obj_list))
    else:
        print("message = -->{}<--".format(message))

def on_data(ws, data, _type, _continue):
    print("### data ###")

def on_error(ws, error):
    print("ERR: " + str(error))

def on_close(ws):
    print("### closed ###")

def on_open(ws):
    print("### opened ###")

def imu_fetch():
    # url = "ws://m5stickc-streamer:42000"
    ws_url = "ws://192.168.68.130:42000"
    websocket.enableTrace(True)
    ws = websocket.WebSocketApp(
        ws_url,
        on_message = on_message,
        on_error = on_error,
        on_close = on_close,
        on_open  = on_open)
    ws.run_forever()

def imu_aggregate():
    out = imu_obj_list.pop(0) # from front of list
    while len(imu_obj_list):
        nxt = imu_obj_list.pop(0) # from front of list
        for k in out.keys():
            out[k].extend(nxt[k])
    return out

def imu_save( _imu_obj ):
    import json
    from datetime import datetime
    fname = datetime.now().strftime("%Y_%m_%d___%H_%M_%S.json")
    with open(fname, "w") as f:
        json.dump(_imu_obj, f)

def mkfig(w=12, h=6, nrow=1, ncol=1, dpi=100, style='seaborn', **kwargs):
    import matplotlib.pyplot as plt
    plt.style.use(style)
    return plt.subplots(
        nrow, ncol, figsize=(w, h), dpi=dpi, 
        facecolor='lightgray', edgecolor='k', **kwargs)

def imu_plot( _imu_obj ):
    import numpy as np
    import matplotlib.pyplot as plt
    fig, axes = mkfig(10, 6, 3, 1, gridspec_kw=dict(height_ratios=[3, 3, 1]))
    tvec = np.arange(len(_imu_obj['ax'])) * np.median(_imu_obj['micros']) / 1e6
    axes[0].plot(tvec, _imu_obj['ax'])
    axes[0].plot(tvec, _imu_obj['ay'])
    axes[0].plot(tvec, _imu_obj['az'])

    axes[1].plot(tvec, _imu_obj['gx'])
    axes[1].plot(tvec, _imu_obj['gy'])
    axes[1].plot(tvec, _imu_obj['gz'])

    axes[2].plot(_imu_obj['micros'])

    fig.tight_layout()
    plt.show()  

if __name__ == "__main__":
    try:
        imu_fetch()
    except KeyboardInterrupt:
        print("stopped fetching imu data")
    # except Exception:
    #     print("catching any generic exception...")

    imu_obj = imu_aggregate()
    imu_save(imu_obj)
    imu_plot(imu_obj)
    sys.exit()



