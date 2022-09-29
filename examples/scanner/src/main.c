
/*

  A simple demo application showing how to scan for
  advertising Bluetooth LE devices using the host cpu of the XPLR-IOT-1.

  Please note thats this examples needs that the network cpu
  is flashed with the correct firmware. Accomplished by using the command:

  do flash_net

  after a successful build of this example. This is only needed to be done once.

*/

#include <stdio.h>
#include <zephyr/bluetooth/bluetooth.h>

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
                         struct net_buf_simple *ad)
{
    char addr_str[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));
    printf("%s (RSSI %d), type %u, AD data len %u\n",
           addr_str, rssi, type, ad->len);
}

void main(void)
{
    struct bt_le_scan_param scan_param = {
        .type = BT_LE_SCAN_TYPE_PASSIVE,
        .options = BT_LE_SCAN_OPT_FILTER_DUPLICATE,
        .interval = BT_GAP_SCAN_FAST_INTERVAL,
        .window = BT_GAP_SCAN_FAST_WINDOW,
    };

    int err;
    err = bt_enable(NULL);
    if (err) {
        printf("* Bluetooth init failed (err %d)\n", err);
    } else {
        err = bt_le_scan_start(&scan_param, device_found);
        if (err) {
            printf("* Failed to start advertising %d\n", err);
        } else {
            printf("iBeacon advertising started\n");
        }
    }
}
