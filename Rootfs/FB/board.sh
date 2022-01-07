# TODO: Add your board specific shell script here
killall udhcpc wpa_supplicant

kernel_ver=$(uname -r)
ko_path="/lib/modules/$kernel_ver"
insmod $ko_path/xr829.ko

wpa_supplicant -Dnl80211 -iwlan0 -c/etc/wpa_supplicant.conf &
sleep 1
udhcpc -i wlan0 &