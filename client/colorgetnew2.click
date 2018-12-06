//TimedSource(INTERVAL 0.001)->color_get_new()->EtherEncap(0x0800,FF:FF:FF:FF:FF:FF, FF:FF:FF:FF:FF:FF)->Queue(1000)->ToDevice(eth0);


RatedSource(RATE 10000)->color_get_new()->EtherEncap(0x0800,FF:FF:FF:FF:FF:FF, FF:FF:FF:FF:FF:FF)->Queue(1000)->ToDevice(eth0);

//TimedSource(INTERVAL 4)->color_get_new()->EtherEncap(0x0800,FF:FF:FF:FF:FF:FF, FF:FF:FF:FF:FF:FF)->Queue(1000)->ToDevice(eth0);
