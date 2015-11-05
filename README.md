ARP request ping

To discover all device in your subnet input command below:

~# arping 192.168.0.1/24

the first argument is a start address for sending arp request, after slash '/' network mask.


Usage:
        arping    <address>/<prefixlen>   [ -vqr ]  [-c cycles]   [-t interval]  [-i interface]
	
	-v	verbose mode
        -q	quiet mode, all discovered devices will be output at the end of process
        -r 	ramdom delay, add to delay interval the random component [ delay = interval + rand mod interval ]
        -t 	interval, change delay interval during arp requests
        -c 	cycles, quantity of cycles by ip adresses on netmask
        -i 	interface, network interface

This application start to send arp-request for each address in the subnet. Then app wait the response 
