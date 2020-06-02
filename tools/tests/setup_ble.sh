HCI="${HCI_DEV:=0}"
echo 6 > /sys/kernel/debug/bluetooth/hci${HCI}/conn_min_interval
echo 6 > /sys/kernel/debug/bluetooth/hci${HCI}/conn_max_interval
setcap cap_net_raw+ep `which hcitool`
