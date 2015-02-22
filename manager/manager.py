import bluetooth

target_name = "Light"
target_address = None

nearby_devices = bluetooth.discover_devices()

for addr in nearby_devices:
    if bluetooth.lookup_name(addr).startswith("Light"):
        target_address = addr
        break

if target_address is not None:
    print "found target bluetooth device with address ", target_address
else:
    print "could not find target bluetooth device nearby"

#test